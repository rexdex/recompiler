#include "build.h"
#include "xenonGPUUtils.h"
#include "dx11GeometryBuffer.h"
#include "dx11Utils.h"

//#pragma optimize( "",off )

CDX11GeometryBuffer::CDX11GeometryBuffer( ID3D11Device* dev, ID3D11DeviceContext* context )
	: m_device( dev )
	, m_mainContext( context )
	, m_geometryDataGeneration( 1 )
	, m_geometryDataTransferSize( 256 * 1024 )
	, m_geometryDataAllocator( 1024 * 1024 * 32 )
	, m_geomeryDataSwapped(false)
	, m_geometryData( nullptr )
{
	// create the vertex buffer cache
	{
		D3D11_BUFFER_DESC bufferDesc;
		bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		bufferDesc.ByteWidth = m_geometryDataAllocator.GetSize();
		bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bufferDesc.MiscFlags = 0;
		bufferDesc.StructureByteStride = 0;
		bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		HRESULT hRet = dev->CreateBuffer( &bufferDesc, NULL, &m_geometryData );
		DEBUG_CHECK( SUCCEEDED(hRet) );
	}

	// create secondary buffer (second frame)
	{
		D3D11_BUFFER_DESC bufferDesc;
		bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		bufferDesc.ByteWidth = m_geometryDataAllocator.GetSize();
		bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bufferDesc.MiscFlags = 0;
		bufferDesc.StructureByteStride = 0;
		bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		HRESULT hRet = dev->CreateBuffer( &bufferDesc, NULL, &m_geometryDataSecondary );
		DEBUG_CHECK( SUCCEEDED(hRet) );
	}	
}

CDX11GeometryBuffer::~CDX11GeometryBuffer()
{
	// release the helper buffer
	if ( m_geometryDataSecondary )
	{
		m_geometryDataSecondary->Release();
		m_geometryDataSecondary = nullptr;
	}

	// release the vertex buffer cache
	if ( m_geometryData )
	{
		m_geometryData->Release();
		m_geometryData = nullptr;
	}
}

void CDX11GeometryBuffer::Swap()
{
	if (!m_geomeryDataSwapped)
	{
		std::swap(m_geometryDataSecondary, m_geometryData);
		m_geomeryDataSwapped = true;
		m_geometryDataAllocator.Reset();
	}

}
const bool CDX11GeometryBuffer::IsBufferUsable( BufferHandle handle ) const
{
	if (handle.m_type == BUFFER_VERTEX)
	{
		return (handle.m_generation == m_geometryDataGeneration) && (handle.m_size != 0);
	}
	else if (handle.m_type == BUFFER_INDEX)
	{
		return (handle.m_generation == m_geometryDataGeneration) && (handle.m_size != 0);
	}


	return false;
}

CDX11GeometryBuffer::LinearAllocator::LinearAllocator( const uint32 size )
	: m_offset(0)
	, m_size(size)
{
}

void CDX11GeometryBuffer::LinearAllocator::Reset()
{
	m_offset = 0;
}

uint32 CDX11GeometryBuffer::LinearAllocator::Alloc( const uint32 size, bool& wrapped )
{
	DEBUG_CHECK( size <= m_size );

	// fits
	if ( m_offset + size < m_size )
	{
		uint32 retOffset = m_offset;
		m_offset += size;
		wrapped = false;
		return retOffset;
	}

	// wraps
	wrapped = true;
	m_offset = 0;
	return 0;
}

bool CDX11GeometryBuffer::AllocStagingBuffer( const uint32 sourceMemorySize, void*& outPtr, uint32& outOffset )
{
	DEBUG_CHECK( (sourceMemorySize & 3) == 0 );	

	// allocate space in the vertex cache
	bool vertexDataWrapped = false;
	const uint32 vertexDataOffset = m_geometryDataAllocator.Alloc( sourceMemorySize, vertexDataWrapped );
	DEBUG_CHECK( !vertexDataWrapped );

	// data wrapped around - invalid all of the previously stored buffers
	if ( vertexDataWrapped )
		m_geometryDataGeneration += 1;

	// map helper buffer, discard on first map each frame
	D3D11_MAPPED_SUBRESOURCE subRes;
	const auto mapMode = D3D11_MAP_WRITE_NO_OVERWRITE;// m_geomeryDataSwapped ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE;
	HRESULT hRet = m_mainContext->Map(m_geometryData, 0, mapMode, 0, &subRes);
	DEBUG_CHECK(SUCCEEDED(hRet) && subRes.pData);

	// reset flag
	m_geomeryDataSwapped = false;

	// done
	outPtr = (char*)subRes.pData + vertexDataOffset;
	outOffset = vertexDataOffset;
	return true;
}

CDX11GeometryBuffer::BufferHandle CDX11GeometryBuffer::UploadStagingData( const uint32 sourceMemorySize, const uint32 dataOffset, const uint32 type )
{
	// umap
	m_mainContext->Unmap( m_geometryData, 0 );

	// return allocated region
	BufferHandle ret;
	ret.m_generation = m_geometryDataGeneration;
	ret.m_offset = dataOffset;
	ret.m_size = sourceMemorySize;
	ret.m_type = type;
	return ret;
}

const CDX11GeometryBuffer::BufferHandle CDX11GeometryBuffer::UploadVertices( const void* sourceMemory, const uint32 sourceMemorySize, const bool verticesNeedSwapping )
{
	DEBUG_CHECK( sourceMemory != nullptr );
	DEBUG_CHECK( (sourceMemorySize & 3) == 0 );

	// get data
	void* stagingPtr = nullptr;
	uint32 dataOffset = 0;
	if ( !AllocStagingBuffer( sourceMemorySize, stagingPtr, dataOffset ) )
		return BufferHandle();

	// copy data SWAPPING it along the way
	uint32* dest = (uint32*) stagingPtr;
	const uint32* src = (const uint32*) sourceMemory;
	if ( verticesNeedSwapping )
	{
		for ( uint32 i=0; i<(sourceMemorySize/4); ++i )
		{
			*dest++ = _byteswap_ulong( *src++ );
		}
	}
	else
	{
		memcpy( dest, src, sourceMemorySize );
	}

	// upload staging data
	return UploadStagingData( sourceMemorySize, dataOffset, BUFFER_VERTEX );	
}

const CDX11GeometryBuffer::BufferHandle CDX11GeometryBuffer::UploadIndices16( const void* sourceMemory, const uint32 numIndices, const bool indicesNeedSwapping )
{
	DEBUG_CHECK( sourceMemory != nullptr );

	// count size of memory
	const uint32 sourceMemorySize = numIndices * sizeof(uint32);

	// get data
	void* stagingPtr = nullptr;
	uint32 dataOffset = 0;
	if ( !AllocStagingBuffer( sourceMemorySize, stagingPtr, dataOffset ) )
		return BufferHandle();

	// copy data SWAPPING it along the way
	uint32* dest = (uint32*) stagingPtr;
	const uint16* src = (const uint16*) sourceMemory;
	if ( indicesNeedSwapping )
	{
		for ( uint32 i=0; i<numIndices; ++i )
		{
			*dest++ = _byteswap_ushort( *src++ ); // notice: 16 bit swap
		}
	}
	else
	{
		for ( uint32 i=0; i<numIndices; ++i )
		{
			*dest++ = *src++; 
		}
	}

	// upload staging data
	return UploadStagingData( sourceMemorySize, dataOffset, BUFFER_INDEX );
}

const CDX11GeometryBuffer::BufferHandle CDX11GeometryBuffer::UploadIndices32( const void* sourceMemory, const uint32 numIndices, const bool indicesNeedSwapping )
{
	DEBUG_CHECK( sourceMemory != nullptr );

	// count size of memory
	const uint32 sourceMemorySize = numIndices * sizeof(uint32);

	// get data
	void* stagingPtr = nullptr;
	uint32 dataOffset = 0;
	if ( !AllocStagingBuffer( sourceMemorySize, stagingPtr, dataOffset ) )
		return BufferHandle();

	// copy data SWAPPING it along the way
	uint32* dest = (uint32*) stagingPtr;
	const uint16* src = (const uint16*) sourceMemory;
	if ( indicesNeedSwapping )
	{
		for ( uint32 i=0; i<numIndices; ++i )
		{
			*dest++ = _byteswap_ulong( *src++ ); // notice: 32 bit swap
		}
	}
	else
	{
		memcpy( dest, src, sourceMemorySize );
	}

	// upload staging data
	return UploadStagingData( sourceMemorySize, dataOffset, BUFFER_INDEX );
}

const bool CDX11GeometryBuffer::BindData( BufferHandle data, const uint32 bufferIndex )
{
	DEBUG_CHECK( IsBufferUsable( data ) );
	DEBUG_CHECK( data.m_type == BUFFER_VERTEX || data.m_type == BUFFER_INDEX );
	DEBUG_CHECK( (data.m_offset & 3) == 0 );
	DEBUG_CHECK( (data.m_size & 3) == 0 );

	// create buffer view at given offset
	D3D11_SHADER_RESOURCE_VIEW_DESC bufferView;
	bufferView.Format = DXGI_FORMAT_R32_UINT;
	bufferView.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	bufferView.Buffer.FirstElement = data.m_offset / 4;
	bufferView.Buffer.NumElements = data.m_size / 4;

	// create view
	ID3D11ShaderResourceView* view = nullptr;
	HRESULT hRet = m_device->CreateShaderResourceView( m_geometryData, &bufferView, &view );
	DEBUG_CHECK( SUCCEEDED(hRet) && view );

	// bind the buffer view
	ID3D11ShaderResourceView* views[] = {view};
	m_mainContext->VSSetShaderResources( bufferIndex, 1, views );

	// done, no longer needed
	view->Release();
	return true;
}