#include "build.h"
#include "xenonGPURegisters.h"
#include "xenonGPUUtils.h"
#include "dx11GeometryDrawer.h"
#include "dx11GeometryBuffer.h"
#include "dx11MicrocodeShader.h"
#include "dx11MicrocodeCache.h"
#include "dx11FetchLayout.h"
#include "dx11ShaderCache.h"
#include "dx11SamplerCache.h"
#include "dx11Utils.h"
#include "dx11TextureManager.h"
#include "xenonGPUDumpWriter.h"

#pragma optimize( "",off )

//----------------------

CDX11GeometryDrawer::ShaderData::ShaderData()
	: m_changed( false )
	, m_microcode( nullptr )
{
}

bool CDX11GeometryDrawer::ShaderData::SetData( CDX11MicrocodeCache* cache, const void* data, const uint32 numWords )
{
	// swap data
	uint32 words[ 8192 ];
	for ( uint32 i=0; i<numWords; ++i )
	{
		words[i] = _byteswap_ulong( ((const uint32*)data)[i] );
	}

	// decompile and cache the microcode
	if ( m_type == XenonShaderType::ShaderPixel )
	{
		CDX11MicrocodeShader* microcode = cache->GetCachedPixelShader( words, numWords * sizeof(uint32) );
		m_changed = (m_microcode != microcode);
		m_microcode = microcode;
	}
	else if ( m_type == XenonShaderType::ShaderVertex )
	{
		CDX11MicrocodeShader* microcode = cache->GetCachedVertexShader( words, numWords * sizeof(uint32) );
		m_changed = (m_microcode != microcode);
		m_microcode = microcode;
	}

	return m_changed;
}

//----------------------

CDX11GeometryDrawer::CDX11GeometryDrawer( ID3D11Device* dev, ID3D11DeviceContext* context )
	: m_device( dev )
	, m_mainContext( context )
	, m_defaultTexture1D( nullptr )
	, m_defaultTexture1DView( nullptr )
	, m_defaultTexture2D( nullptr )
	, m_defaultTexture2DView( nullptr )
	, m_defaultTexture3D( nullptr )
	, m_defaultTexture3DView( nullptr )
	, m_shaderTextureFetchSlots( 0 )
{
	// texture list
	memset( m_textures, 0, sizeof(m_textures) );

	// shader types
	m_pixelShader.m_type = XenonShaderType::ShaderPixel;
	m_vertexShader.m_type = XenonShaderType::ShaderVertex;

	// create geometry buffer
	m_geometryBuffer = new CDX11GeometryBuffer( dev, context );

	// create sampler cache
	m_samplerCache = new CDX11SamplerCache( dev );

	// create shader cache
	m_shaderCache = new CDX11ShaderCache( dev );
	m_shaderCache->SetDumpPath( L"Q://shaderdump//" );

	// create microcode cache
	m_microcodeCache = new CDX11MicrocodeCache();
	m_microcodeCache->SetDumpPath( L"Q://shaderdump//" );

	// create constant buffer
	m_vertexViewportState.Create( dev );

	// create textures
	CreateDefaultTextures();
}

CDX11GeometryDrawer::~CDX11GeometryDrawer()
{
	// release default textures
	ReleaseDefaultTextres();

	// delete shader cache
	delete m_shaderCache;

	// delete sampler cache
	delete m_samplerCache;

	// delete microcode cache
	delete m_microcodeCache;

	// delete geometry buffer
	delete m_geometryBuffer;
}

void CDX11GeometryDrawer::Reset()
{
	m_geometryBuffer->Swap();
}

void CDX11GeometryDrawer::SetShaderDumpDirector( const std::wstring& dumpDir )
{
	m_shaderDumpDirectory = dumpDir;
}

void CDX11GeometryDrawer::SetViewportVertexFormat( const bool xyDivied, const bool zDivied, const bool wNotInverted )
{
	m_vertexViewportState.Get().m_xyDivided = xyDivied;
	m_vertexViewportState.Get().m_zDivided = zDivied;
	m_vertexViewportState.Get().m_wNotInverted = wNotInverted;
}

void CDX11GeometryDrawer::SetViewportWindowScale( const bool bNormalizedXYCoordinates )
{
	m_vertexViewportState.Get().m_normalizedCoordinates = bNormalizedXYCoordinates;
}

void CDX11GeometryDrawer::SetPhysicalSize( const uint32 width, const uint32 height )
{
	if ( width && height )
	{
		m_vertexViewportState.Get().m_physicalWidth = (float)width;
		m_vertexViewportState.Get().m_physicalHeight = (float)height;
		m_vertexViewportState.Get().m_physicalInvWidth = 1.0f / (float)width;
		m_vertexViewportState.Get().m_physicalInvHeight = -1.0f / (float)height;
	}
}

void CDX11GeometryDrawer::RefreshTextureFetchSlotMask()
{
	m_shaderTextureFetchSlots = 0;

	if ( m_pixelShader.m_microcode )
		m_shaderTextureFetchSlots |= m_pixelShader.m_microcode->GetTextureFetchSlotMask();

	if ( m_vertexShader.m_microcode )
		m_shaderTextureFetchSlots |= m_vertexShader.m_microcode->GetTextureFetchSlotMask();
}

void CDX11GeometryDrawer::SetPixelShaderdCode( const void* data, const uint32 numWords )
{
	const bool newShader = m_pixelShader.SetData( m_microcodeCache, data, numWords );
	m_shaderDirty |= newShader;

	// keep the fetch slot mask ALWAYS up to date
	if ( newShader )
		RefreshTextureFetchSlotMask();
}

void CDX11GeometryDrawer::SetVertexShaderdCode( const void* data, const uint32 numWords )
{
	const bool newShader = m_vertexShader.SetData( m_microcodeCache, data, numWords );
	m_shaderDirty |= newShader;

	// keep the fetch slot mask ALWAYS up to date
	if ( newShader )
		RefreshTextureFetchSlotMask();
}

const uint32 CDX11GeometryDrawer::GetActiveTextureFetchSlotMask() const
{
	return m_shaderTextureFetchSlots;
}

void CDX11GeometryDrawer::SetTexture( const uint32 fetchSlot, CDX11AbstractTexture* runtimeTexture )
{
	DEBUG_CHECK( fetchSlot < MAX_TEXTURE_FETCH_SLOTS );
	m_textures[ fetchSlot ] = runtimeTexture;
}

void CDX11GeometryDrawer::CreateDefaultTextures()
{
	// 2D texture
	{
		const uint32 size = 256;

		// texture memory
		uint8* data = new uint8[ size*size*4 ];
		uint8* writePtr = data;
		for ( uint32 y=0; y<size; ++y )
		{
			for ( uint32 x=0; x<size; ++x, writePtr += 4 )
			{
				writePtr[0] = x^y;
				writePtr[1] = x^y;
				writePtr[2] = x^y;
				writePtr[3] = 255;
			}
		}

		// setup texture
		D3D11_TEXTURE2D_DESC desc;
		memset( &desc, 0, sizeof(desc) );
		desc.ArraySize = 1;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.MipLevels = 1;
		desc.Width = size;
		desc.Height = size;
		desc.MiscFlags = 0;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_IMMUTABLE;

		// setup source data
		D3D11_SUBRESOURCE_DATA texData;
		texData.pSysMem = data;
		texData.SysMemPitch = 4*size;
		texData.SysMemSlicePitch = 4*size*size;

		// create texture
		HRESULT hRet = m_device->CreateTexture2D( &desc, &texData, &m_defaultTexture2D );
		DEBUG_CHECK( SUCCEEDED(hRet) && m_defaultTexture2D );

		// release temp texture memory
		delete [] data;

		// setup texture view
		D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
		viewDesc.Format = desc.Format;
		viewDesc.Texture2D.MipLevels = 1;
		viewDesc.Texture2D.MostDetailedMip = 0;
		viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;

		// create texture view
		hRet = m_device->CreateShaderResourceView( m_defaultTexture2D, &viewDesc, &m_defaultTexture2DView );
		DEBUG_CHECK( SUCCEEDED(hRet) && m_defaultTexture2DView );		
	}

	// 3D texture
	{
		const uint32 size = 64;

		// texture memory
		uint8* data = new uint8[ size*size*size*4 ];
		uint8* writePtr = data;
		for ( uint32 z=0; z<size; ++z )
		{
			for ( uint32 y=0; y<size; ++y )
			{
				for ( uint32 x=0; x<size; ++x, writePtr += 4 )
				{
					const uint32 col = ( (x^y)<<2 ) ^ (17*z);
					writePtr[0] = col;
					writePtr[1] = col;
					writePtr[2] = col;
					writePtr[3] = 255;
				}
			}
		}

		// setup texture
		D3D11_TEXTURE3D_DESC desc;
		memset( &desc, 0, sizeof(desc) );
		desc.Depth = size;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.MipLevels = 1;
		desc.Width = size;
		desc.Height = size;
		desc.MiscFlags = 0;
		desc.Usage = D3D11_USAGE_IMMUTABLE;

		// setup source data
		D3D11_SUBRESOURCE_DATA texData;
		texData.pSysMem = data;
		texData.SysMemPitch = 4*size;
		texData.SysMemSlicePitch = 4*size*size;

		// create texture
		HRESULT hRet = m_device->CreateTexture3D( &desc, &texData, &m_defaultTexture3D );
		DEBUG_CHECK( SUCCEEDED(hRet) && m_defaultTexture3D );

		// release temp texture memory
		delete [] data;

		// setup texture view
		D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
		viewDesc.Format = desc.Format;
		viewDesc.Texture3D.MipLevels = 1;
		viewDesc.Texture3D.MostDetailedMip = 0;
		viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;

		// create texture view
		hRet = m_device->CreateShaderResourceView( m_defaultTexture3D, &viewDesc, &m_defaultTexture3DView );
		DEBUG_CHECK( SUCCEEDED(hRet) && m_defaultTexture3DView );		
	};

	// 2D array texture
	{
		const uint32 size = 64;

		// texture memory
		uint8* data = new uint8[ size*size*size*4 ];
		uint8* writePtr = data;
		for ( uint32 z=0; z<size; ++z )
		{
			for ( uint32 y=0; y<size; ++y )
			{
				for ( uint32 x=0; x<size; ++x, writePtr += 4 )
				{
					const uint32 col = ( (x^y)<<2 ) ^ z;
					writePtr[0] = col;
					writePtr[1] = col;
					writePtr[2] = col;
					writePtr[3] = 255;
				}
			}
		}

		// setup texture
		D3D11_TEXTURE2D_DESC desc;
		memset( &desc, 0, sizeof(desc) );
		desc.ArraySize = size;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.MipLevels = 1;
		desc.Width = size;
		desc.Height = size;
		desc.MiscFlags = 0;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_IMMUTABLE;

		// setup source data
		D3D11_SUBRESOURCE_DATA* texData = new D3D11_SUBRESOURCE_DATA[size];
		for ( uint32 z=0; z<size; ++z )
		{
			texData[z].pSysMem = data + (4*size*size * z);
			texData[z].SysMemPitch = 4*size;
			texData[z].SysMemSlicePitch = 4*size*size;
		}

		// create texture
		HRESULT hRet = m_device->CreateTexture2D( &desc, texData, &m_defaultTexture2DArray );
		DEBUG_CHECK( SUCCEEDED(hRet) && m_defaultTexture2DArray );

		// release temp texture memory
		delete [] texData;
		delete [] data;

		// setup texture view
		D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
		viewDesc.Format = desc.Format;
		viewDesc.Texture2DArray.MipLevels = 1;
		viewDesc.Texture2DArray.MostDetailedMip = 0;
		viewDesc.Texture2DArray.ArraySize = size;
		viewDesc.Texture2DArray.FirstArraySlice = 0;
		viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;

		// create texture view
		hRet = m_device->CreateShaderResourceView( m_defaultTexture2DArray, &viewDesc, &m_defaultTexture2DArrayView );
		DEBUG_CHECK( SUCCEEDED(hRet) && m_defaultTexture2DArray );		
	};
}

void CDX11GeometryDrawer::ReleaseDefaultTextres()
{
	if ( m_defaultTexture1DView )
	{
		m_defaultTexture1DView->Release();
		m_defaultTexture1DView = nullptr;
	}

	if ( m_defaultTexture1D )
	{
		m_defaultTexture1D->Release();
		m_defaultTexture1D = nullptr;
	}

	if ( m_defaultTexture2DView )
	{
		m_defaultTexture2DView->Release();
		m_defaultTexture2DView = nullptr;
	}

	if ( m_defaultTexture2D )
	{
		m_defaultTexture2D->Release();
		m_defaultTexture2D = nullptr;
	}

	if ( m_defaultTexture2DArrayView )
	{
		m_defaultTexture2DArrayView->Release();
		m_defaultTexture2DArrayView = nullptr;
	}

	if ( m_defaultTexture2DArray )
	{
		m_defaultTexture2DArray->Release();
		m_defaultTexture2DArray = nullptr;
	}

	if ( m_defaultTexture3DView )
	{
		m_defaultTexture3DView->Release();
		m_defaultTexture3DView = nullptr;
	}

	if ( m_defaultTexture3D )
	{
		m_defaultTexture3D->Release();
		m_defaultTexture3D = nullptr;
	}
}

bool CDX11GeometryDrawer::RealizeShaders()
{
	// shaders have not changed since last draw
	if ( !m_shaderDirty )
		return true;

	// get pixel shader microcode
	if ( !m_pixelShader.m_microcode || !m_vertexShader.m_microcode )
		return false;

	// validate
	m_shaderDirty = false;
	return true;
}

bool CDX11GeometryDrawer::RealizeVertexBuffers(  const class CXenonGPURegisters& regs, class IXenonGPUDumpWriter* traceDump, const class CDX11FetchLayout* layout )
{
	// we must have valid vertex layout
	DEBUG_CHECK( layout != nullptr );
	if ( !layout )
		return false;
	
	// prepare vertex geometry sources
	const uint32 numStreams = layout->GetNumStreams();
	for ( uint32 i=0; i<numStreams; ++i )
	{
		// get source geometry stream
		const auto& stream = layout->GetStream(i);

		// calculate fetch slot
		const uint32 fetchRegBase = (uint32) XenonGPURegister::REG_SHADER_CONSTANT_FETCH_00_0 + (stream.m_slot * 2);
		const auto fetchEntry = &regs.GetStructAt<XenonGPUVertexFetchData>( (XenonGPURegister) fetchRegBase );

		// we only support BE for now
		DEBUG_CHECK( fetchEntry->endian == 2 );
		if ( fetchEntry->endian != 2 )
			return false;

		// get the source memory and size
		const void* sourceMemory = (const void*) GPlatform.GetMemory().TranslatePhysicalAddress( fetchEntry->address << 2 ); // address aligned to 4
		const uint32 sourceSize = fetchEntry->size << 2; // size in dwords

		// dump the vertex streams
		if ( traceDump )
		{
			auto memoryAddr = GPlatform.GetMemory().TranslatePhysicalAddress( fetchEntry->address << 2 ); // address aligned to 4
			traceDump->MemoryAccessRead( memoryAddr, sourceSize, "VertexBuffer" );
		}

		// move the data to GPU memory cache
		const auto bufferData = m_geometryBuffer->UploadVertices( sourceMemory, sourceSize, true );
		DEBUG_CHECK( m_geometryBuffer->IsBufferUsable( bufferData ) );
		if ( !m_geometryBuffer->IsBufferUsable( bufferData ) )
			return false;

		// bind the data
		m_geometryBuffer->BindData( bufferData, 4+i ); // hardcoded
	}

	// vertex data is valid
	return true;
}

static uint16 TempIndices[ 1024*1024 ];

bool CDX11GeometryDrawer::RealizeIndexBuffer( struct CXenonGPUState::DrawIndexState& ds )
{	
	// we NEVER use normal index data
	m_mainContext->IASetIndexBuffer( NULL, DXGI_FORMAT_R32_UINT, 0 );

	// index buffer may be needed to handle unsupported types of geometry
	const auto primitiveType = ds.m_primitiveType;
	if ( primitiveType == XenonPrimitiveType::PrimitiveQuadList )
	{
		// get number of quads in the list
		const uint32 quadCount = ds.m_indexCount/4;

		// create or convert index buffer
		if ( ds.m_indexData == nullptr )
		{
			// convert quad indices to triangle list indices
			uint16* writePtr = TempIndices;
			for ( uint32 i=0; i<quadCount; ++i, writePtr += 6 )
			{
				const uint16 baseIndex = i*4;
				writePtr[0] = baseIndex + 0;
				writePtr[1] = baseIndex + 1;
				writePtr[2] = baseIndex + 2;
				writePtr[3] = baseIndex + 0;
				writePtr[4] = baseIndex + 3;
				writePtr[5] = baseIndex + 2;
			}
		}
		else
		{
			// convert quad indices to triangle list indices
			uint16* writePtr = TempIndices;
			const uint16* readPtr = (const uint16*) ds.m_indexData;
			for ( uint32 i=0; i<quadCount; ++i, readPtr += 4, writePtr += 6 )
			{
				writePtr[0] = readPtr[0];
				writePtr[1] = readPtr[1];
				writePtr[2] = readPtr[2];
				writePtr[3] = readPtr[0];
				writePtr[4] = readPtr[2];
				writePtr[5] = readPtr[3];
			}
		}

		// use the converted buffer
		ds.m_indexCount = quadCount * 6;
		ds.m_indexData = TempIndices;
		ds.m_indexEndianess = XenonGPUEndianFormat::FormatUnspecified;
		ds.m_indexFormat = XenonIndexFormat::Index16;
		ds.m_primitiveType = XenonPrimitiveType::PrimitiveTriangleList;
		ds.m_baseVertexIndex = 0;
	}
	else if ( primitiveType == XenonPrimitiveType::PrimitiveQuadStrip )
	{
		return false;
	}
	else if ( primitiveType == XenonPrimitiveType::PrimitivePointList )
	{
		//return true;
	}
	else if ( primitiveType == XenonPrimitiveType::PrimitiveRectangleList )
	{
		return false;
	}

	// set data
	if ( nullptr == ds.m_indexData )
	{
		// no index data
		m_vertexViewportState.Get().m_indexMode = 0;
		m_vertexViewportState.Get().m_baseVertex = 0;
	}
	else
	{
		// indexed mode
		m_vertexViewportState.Get().m_indexMode = 1;
		m_vertexViewportState.Get().m_baseVertex = ds.m_baseVertexIndex;
		DEBUG_CHECK( m_vertexViewportState.Get().m_baseVertex == 0 ); // for now

		// upload index data to geometry cache
		if ( ds.m_indexFormat == XenonIndexFormat::Index16 )
		{
			auto buffer = m_geometryBuffer->UploadIndices16( ds.m_indexData, ds.m_indexCount, (ds.m_indexEndianess != XenonGPUEndianFormat::FormatUnspecified) );
			m_geometryBuffer->BindData( buffer, 15 ); // hardcoded
		}
		else if ( ds.m_indexFormat== XenonIndexFormat::Index32 )
		{
			auto buffer = m_geometryBuffer->UploadIndices32( ds.m_indexData, ds.m_indexCount, (ds.m_indexEndianess != XenonGPUEndianFormat::FormatUnspecified) );
			m_geometryBuffer->BindData( buffer, 15 ); // hardcoded
		}
		else
		{
			DEBUG_CHECK( !"Invalid vertex data format" );
		}
	}

	// done
	return true;
}

bool CDX11GeometryDrawer::TranslatePrimitiveType( const XenonPrimitiveType primitiveType, D3D11_PRIMITIVE& outPrimitive, D3D11_PRIMITIVE_TOPOLOGY& outTopology )
{
	switch ( primitiveType )
	{
		case XenonPrimitiveType::PrimitivePointList: 
			outPrimitive = D3D11_PRIMITIVE_POINT; 
			outTopology = D3D_PRIMITIVE_TOPOLOGY_POINTLIST; 
			return true;

		case XenonPrimitiveType::PrimitiveLineList:
			outPrimitive = D3D11_PRIMITIVE_LINE; 
			outTopology = D3D_PRIMITIVE_TOPOLOGY_LINELIST; 
			return true;

		case XenonPrimitiveType::PrimitiveLineStrip:
			outPrimitive = D3D11_PRIMITIVE_LINE; 
			outTopology = D3D_PRIMITIVE_TOPOLOGY_LINESTRIP; 
			return true;

		case XenonPrimitiveType::PrimitiveTriangleList:
			outPrimitive = D3D11_PRIMITIVE_TRIANGLE; 
			outTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST; 
			return true;

		case XenonPrimitiveType::PrimitiveTriangleStrip:
			outPrimitive = D3D11_PRIMITIVE_TRIANGLE; 
			outTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP; 
			return true;

		case XenonPrimitiveType::PrimitiveRectangleList:
			outPrimitive = D3D11_PRIMITIVE_TRIANGLE; 
			outTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST; 
			return true;

		case XenonPrimitiveType::PrimitiveLineLoop:
		case XenonPrimitiveType::PrimitiveQuadStrip:
		case XenonPrimitiveType::PrimitiveTriangleFan:
			DEBUG_CHECK( !"Unsupported primitive type" );
			return false;

		// quad list is drawn using triangles + a geometry shader
		case XenonPrimitiveType::PrimitiveQuadList:
			outPrimitive = D3D11_PRIMITIVE_TRIANGLE; 
			outTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST; 
			return true;
	}

	DEBUG_CHECK( !"Unknown primitive type" );
	return false;
}

bool CDX11GeometryDrawer::Draw( const class CXenonGPURegisters& regs, class IXenonGPUDumpWriter* traceDump, const struct CXenonGPUState::DrawIndexState& ds )
{
	if ( !RealizeShaders() )
		return false;

	// get the shader control from GPU state
	const auto& programCtnl = regs.GetStructAt< XenonGPUProgramCntl >( XenonGPURegister::REG_SQ_PROGRAM_CNTL );

	// get pixel shader drawing context
	CDX11PixelShader::DrawingContext psContext;
	psContext.m_bSRGBWrite = false;
	psContext.m_bAlphaTestEnabled = false;
	psContext.m_psRegs = programCtnl.ps_regs;
	psContext.m_vsRegs = programCtnl.vs_regs;
	psContext.m_interp = m_vertexShader.m_microcode->GetNumUsedInterpolators();

	// get vertex shader drawing context
	CDX11VertexShader::DrawingContext vsContext;
	vsContext.m_bTest = false;
	vsContext.m_psRegs = programCtnl.ps_regs;
	vsContext.m_vsRegs = programCtnl.vs_regs;

	// compile actual renderable DX11 pixel & vertex shaders
	CDX11VertexShader* vs = m_shaderCache->GetVertexShader( m_vertexShader.m_microcode, vsContext );
	CDX11PixelShader* ps = m_shaderCache->GetPixelShader( m_pixelShader.m_microcode, psContext );

	// no drawable shaders
	if ( !vs || !ps )
		return false;

	// quad list - generate ad hock vertex&index buffer
	//if ( ds.m_primitiveType == XenonPrimitiveType::PrimitiveQuadList )
		//return true;

	// bind shaders
	m_mainContext->PSSetShader( ps->GetBindableShader(), nullptr, 0 );
	m_mainContext->VSSetShader( vs->GetBindableShader(), nullptr, 0 );

	// dump the source code for the shaders :)
	if ( traceDump )
	{
		const auto& psCode = ps->GetSourceCode();
		traceDump->MemoryAccessRead( (uint32) psCode.c_str(), (uint32)psCode.length(), "PSSourceCode" );

		const auto& vsCode = vs->GetSourceCode();
		traceDump->MemoryAccessRead( (uint32) vsCode.c_str(), (uint32)vsCode.length(), "VSSourceCode" );
	}

	// get textures
	std::vector< CDX11MicrocodeShader::TextureInfo > textures;
	ps->GetMicrocode()->GetUsedTextures( textures );

	// bind pixel shader textures
	{
		// get texture to bind
		for ( const auto& it : textures )
		{
			CDX11AbstractTexture* tex = m_textures[ it.m_fetchSlot ];

			// make sure texture is up to date (may re upload the data from CPU to GPU)
			if ( tex )
				tex->EnsureUpToDate();

			// bind
			ID3D11ShaderResourceView* textureView = nullptr;
			if ( it.m_type == CDX11MicrocodeShader::ETextureType::TYPE_1D )
			{
				textureView = tex ? tex->GetView() : m_defaultTexture1DView;
			}
			else if ( it.m_type == CDX11MicrocodeShader::ETextureType::TYPE_2D )
			{
				textureView = (tex && tex->GetType() == XenonTextureType::Texture_2D) ? tex->GetView() : m_defaultTexture2DView;
			}
			else if ( it.m_type == CDX11MicrocodeShader::ETextureType::TYPE_Array2D )
			{
				textureView = tex ? tex->GetView() : m_defaultTexture2DArrayView;
			}
			else if ( it.m_type == CDX11MicrocodeShader::ETextureType::TYPE_3D )
			{
				textureView = (tex && tex->GetType() == XenonTextureType::Texture_3D) ? tex->GetView() : m_defaultTexture3DView;
			}
			else
			{
				DEBUG_CHECK( !"Unsupported pixel shader texture type" );
			}

			// get sampler
			CDX11SamplerCache::SamplerInfo samplerInfo;
			auto* samplerState = m_samplerCache->GetSamplerState( samplerInfo );

			// bind sampler and texture
			m_mainContext->PSSetShaderResources( it.m_runtimeSlot, 1, &textureView );
			m_mainContext->PSSetSamplers( it.m_runtimeSlot, 1, &samplerState );
		}
	}

	// get geometry layout
	const CDX11FetchLayout* fetchLayout = vs->GetFetchLayout();

	// prepare vertex geometry buffers using geometry layout for the shader
	if ( !RealizeVertexBuffers( regs, traceDump, fetchLayout ) )
		return false;

	// translate primitive type
	D3D11_PRIMITIVE primitiveType;
	D3D11_PRIMITIVE_TOPOLOGY primitiveTopology;
	if ( !TranslatePrimitiveType( ds.m_primitiveType, primitiveType, primitiveTopology ) )
		return false;

	// prepare index buffer
	auto dsCopy = ds;
	if ( !RealizeIndexBuffer( dsCopy ) )
		return false;

	// set primitive type and topology
	m_mainContext->IASetPrimitiveTopology( primitiveTopology );
	m_mainContext->IASetInputLayout( vs->GetBindableInputLayout() );

	// cra
	D3D11_VIEWPORT viewport[10];
	UINT numViewports = 10;
	m_mainContext->RSGetViewports( &numViewports, &viewport[0] );

	// final touch - bind the extra "fake pipeline" stuff for vertex shader
	m_mainContext->VSSetConstantBuffers( 2, 1, m_vertexViewportState.GetBufferForBinding( m_mainContext ) );

	// do we have index data?
	// it determines if we should do DrawIndexedPrimitive, DrawPrimitive or DrawAuto
	if ( dsCopy.m_indexData == nullptr )
	{
		// draw without index buffer - direct index mapping
		const uint32 vertexCount = dsCopy.m_indexCount;
		m_mainContext->Draw( vertexCount, 0 );
	}
	else
	{
		// draw with index buffer - data re indexed using additional buffer
		const uint32 vertexCount = dsCopy.m_indexCount;
		m_mainContext->Draw( vertexCount, 0 );
	}

	// reset texture list
	memset( m_textures, 0, sizeof(m_textures) );

	// unbind shader resources
	{
		for ( const auto& it : textures )
		{
			ID3D11ShaderResourceView* textureView = nullptr;
			m_mainContext->PSSetShaderResources( it.m_runtimeSlot, 1, &textureView );
		}
	}

	// geometry rendered
	return true;
}


//----------------------
