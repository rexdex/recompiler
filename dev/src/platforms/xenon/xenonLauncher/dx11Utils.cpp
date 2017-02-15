#include "build.h"
#include "dx11Utils.h"

//#pragma optimize( "", off )

//-----------------

CDX11ComputeShader::CDX11ComputeShader( const char* name )
	: m_shader( nullptr )
	, m_name( name )
{
}

CDX11ComputeShader::~CDX11ComputeShader()
{
	// release shader resource
	if ( m_shader )
	{
		m_shader->Release();
		m_shader = nullptr;
	}
}

bool CDX11ComputeShader::Load( ID3D11Device* dev, const void* data, const uint32 dataSize )
{
	if ( !m_shader )
	{
		HRESULT hRet = dev->CreateComputeShader( data, dataSize, NULL, &m_shader );
		if ( !m_shader || FAILED(hRet) )
		{
			GLog.Err( "D3D: Failed to load compute shader '%hs'", m_name );
			return false;
		}
	}

	return true;
}

//-----------------

CDX11ConstantBuffer::CDX11ConstantBuffer( void* dataPtr, const uint32 dataSize )
	: m_dataPtr( dataPtr )
	, m_dataSize( dataSize )
	, m_buffer( nullptr )
	, m_modified( false )
{
}

CDX11ConstantBuffer::~CDX11ConstantBuffer()
{
	// release buffer resource
	if ( m_buffer )
	{
		m_buffer->Release();
		m_buffer = nullptr;
	}
}

bool CDX11ConstantBuffer::Create( ID3D11Device* dev )
{
	if ( !m_buffer )
	{
		D3D11_BUFFER_DESC bufferDesc;
		memset( &bufferDesc, 0, sizeof(bufferDesc) );
		bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufferDesc.ByteWidth = (m_dataSize + 15) & ~15;
		bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bufferDesc.Usage = D3D11_USAGE_DYNAMIC;

		D3D11_SUBRESOURCE_DATA initData;
		memset( &initData, 0, sizeof(initData) );
		initData.pSysMem = m_dataPtr;

		HRESULT hRet = dev->CreateBuffer( &bufferDesc, &initData, &m_buffer );
		if ( !m_buffer || FAILED(hRet) ) 
		{
			GLog.Err( "D3D: Failed to create constant buffer. Error = 0x%08X", hRet );
			return false;
		}
	}

	return true;
}

ID3D11Buffer** CDX11ConstantBuffer::GetBufferForBinding( ID3D11DeviceContext* context )
{
	// flush data
	FlushChanges( context );

	// return binding data
	return &m_buffer;
}

bool CDX11ConstantBuffer::FlushChanges( ID3D11DeviceContext* context )
{
	DEBUG_CHECK( m_buffer != nullptr );

	if ( m_modified )
	{
		D3D11_MAPPED_SUBRESOURCE data;
		memset( &data, 0, sizeof(data) );

		// gain access to constant buffer memory
		HRESULT hRet = context->Map( m_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &data );
		if ( FAILED( hRet ) )
		{
			GLog.Err( "D3D: Failed to map constant buffer memory. Error = 0x%08X", hRet );
			return false;
		}

		// copy data
		memcpy( data.pData, m_dataPtr, m_dataSize );

		// release access
		context->Unmap( m_buffer, 0 );
		m_modified = false;
	}

	return true;
}

//-----------------

CDX11BufferRing::CDX11BufferRing( ID3D11Device* dev, ID3D11DeviceContext* context, const uint32 size )
	: m_device( dev )
	, m_context( context )
	, m_pos( 0 )
	, m_size( size )
	, m_generation( 0 )
	, m_isMapped( false )
	, m_buffer( nullptr )
{
}

CDX11BufferRing::~CDX11BufferRing()
{
	if ( m_buffer )
	{
		m_buffer->Release();
		m_buffer = nullptr;
	}
}

void* CDX11BufferRing::Acquire( uint32 size, uint32 alignment, AllocInfo& outAlloc )
{
	DWORD mapFlags = 0;

	// do not map twice
	DEBUG_CHECK( !m_isMapped );
	if ( m_isMapped)
		return nullptr;

	// allocate offset
	D3D11_MAP mapType;
	const uint32 alignedPos = (m_pos + (alignment-1)) & ~(alignment-1);
	if ( alignedPos + size > m_size )
	{
		// rewind
		outAlloc.m_offset = 0;
		outAlloc.m_generation = m_generation += 1;
		outAlloc.m_size = size;

		// discard previous content
		mapType = D3D11_MAP_WRITE_DISCARD;
	}
	else
	{
		// keep adding
		outAlloc.m_offset = alignedPos;
		outAlloc.m_generation = m_generation;
		outAlloc.m_size = size;

		// discard previous content
		mapType = D3D11_MAP_WRITE_NO_OVERWRITE;
	}

	// map buffer
	D3D11_MAPPED_SUBRESOURCE mapData;
	HRESULT hRet = m_context->Map( m_buffer, 0, mapType, 0, &mapData );
	if ( FAILED(hRet) )
	{
		GLog.Err( "D3D: Map error in circular buffer: size=%d, error=0x%08X", size, hRet );
		return nullptr;
	}

	// advance
	m_pos = outAlloc.m_offset + m_size;
	m_isMapped = true;

	// return valid pointer for writing
	return (uint8*)mapData.pData + outAlloc.m_offset;
}

void CDX11BufferRing::Commit( void* memory )
{
	DEBUG_CHECK( m_isMapped );
	if ( !m_isMapped )
		return;

	// unmap
	m_context->Unmap( m_buffer, 0 );
	m_isMapped = false;
}

//-----------------
