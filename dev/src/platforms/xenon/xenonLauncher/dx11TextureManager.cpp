#include "build.h"
#include "xenonGPUUtils.h"
#include "xenonGPUTextures.h"
#include "dx11TextureManager.h"
#include "dx11Staging.h"
#include <xutility>

#undef min
#pragma optimize( "", off )

//---------------------------------------------------------------------------

CDX11AbstractSurface::CDX11AbstractSurface()
	: m_sourceFormat( XenonTextureFormat::Format_Unknown )
	, m_sourceWidth( 0 )
	, m_sourceHeight( 0 )
	, m_depth( 0 )
	, m_width( 0 )
	, m_height( 0 )
	, m_sourceRowPitch( 0 )
	, m_sourceSlicePitch( 0 )
	, m_sourceMemoryOffset( 0 )
	, m_sourceBlockSize( 0 )
	, m_sourcePackedTileOffsetX( 0 )
	, m_sourcePackedTileOffsetY( 0 )
	, m_sourceEndianess( XenonGPUEndianFormat::FormatUnspecified )
	, m_runtimeFormat( DXGI_FORMAT_UNKNOWN )
{
}

CDX11AbstractSurface::~CDX11AbstractSurface()
{
}

XenonTextureFormat CDX11AbstractSurface::GetFormat() const
{
	return m_sourceFormat;
}

uint32 CDX11AbstractSurface::GetWidth() const
{
	return m_width;
}

uint32 CDX11AbstractSurface::GetHeight() const
{
	return m_height;
}

uint32 CDX11AbstractSurface::GetDepth() const
{
	return m_depth;
}

uint32 CDX11AbstractSurface::GetRowPitch() const
{
	return m_sourceRowPitch;
}

uint32 CDX11AbstractSurface::GetSlicePitch() const
{
	return m_sourceSlicePitch;
}

uint32 CDX11AbstractSurface::GetSourceMemoryAddress() const
{
	return m_sourceMemoryOffset;
}

namespace Helper
{
	static void ConvertGPUEndianess( void* dest, const void* src, const uint32 length, XenonGPUEndianFormat endianess )
	{
		// endianess
		if ( endianess == XenonGPUEndianFormat::Format8in16 )
		{
			for ( uint32 i=0; i<length/2; ++i )
				*((unsigned short*)dest + i) = _byteswap_ushort( *((const unsigned short*)src + i) );
		}
		else if ( endianess == XenonGPUEndianFormat::Format8in32 )
		{
			for ( uint32 i=0; i<length/4; ++i )
				*((unsigned int*)dest + i) = _byteswap_ulong( *((const unsigned int*)src + i) );
		}
		else if ( endianess == XenonGPUEndianFormat::FormatUnspecified )
		{
			memcpy( dest, src, length );
		}
		else
		{
			DEBUG_CHECK( !"Unknown endianess format" );
		}
	}
}

void CDX11AbstractSurface::Upload( const void* srcTextureData, void* destData, uint32 destRowPitch, uint32 destSlicePitch ) const
{
	uint8 TempData[ 8192 ];

	for ( uint32 z=0; z<m_depth; ++z )
	{
		// get source target memory pointers for given slice
		const uint8* srcData = (const uint8*)((uint32)srcTextureData + m_sourceMemoryOffset + m_sourceSlicePitch*z);
		uint8* targetData = (uint8*)destData + (z*destSlicePitch);

		// copy data
		// texture is tiled - hard case
		if ( m_sourceIsTiled )
		{
			// Size of the block to copy (assumed to be the same in source and dest data)
			const uint32 blockSize = m_sourceBlockSize;

			// Log of BPP (1, 2, 3)
			const uint32 bppLog = (m_sourceBlockSize >> 2) + ((m_sourceBlockSize >> 1) >> (m_sourceBlockSize >> 2));

			// copy range
			const uint32 copyWidth = m_sourceBlockWidth;
			const uint32 copyHeight = std::min(m_sourceBlockHeight, m_height);

			// Tiled textures can be packed; get the offset into the packed texture
			// The packing place depends on the size of the tiled texture
			uint32 destOffsetY = 0;
			for ( uint32 y=0, outputOffset=0; y<copyHeight; ++y, destOffsetY += destRowPitch )
			{
				const uint32 srcOffsetY = XenonTextureInfo::TiledOffset2DOuter(m_sourcePackedTileOffsetY + y, m_sourceWidth / m_sourceFormatBlockWidth, bppLog);

				uint32 destOffsetX = destOffsetY;
				for ( uint32 x=0; x<copyWidth; ++x, destOffsetX += blockSize )
				{
					const uint32 srcX = m_sourcePackedTileOffsetX + x;
					const uint32 srcY = m_sourcePackedTileOffsetY + y;
					const uint32 srcOffset = XenonTextureInfo::TiledOffset2DInner( srcX, srcY, bppLog, srcOffsetY ) >> bppLog;
					Helper::ConvertGPUEndianess( targetData + destOffsetX, srcData + srcOffset*blockSize, blockSize, m_sourceEndianess );
				}
			}
		}
		// texture is not tiled and the pitch matches
		else if ( m_sourceRowPitch == destRowPitch )
		{
			const uint32 totalSize = destRowPitch * m_sourceHeight;
			Helper::ConvertGPUEndianess( targetData, srcData, totalSize, m_sourceEndianess );
		}
		// custom format support
		else if ( m_sourceFormat == XenonTextureFormat::Format_4_4_4_4 )
		{
			const uint32 copyPitch = std::min( m_sourceRowPitch, destRowPitch );
			uint32 copyHeight = std::min( m_sourceBlockHeight, m_height );
			uint32 copyWidth = m_width;
			for ( uint32 y=0; y<copyHeight; ++y )
			{
				Helper::ConvertGPUEndianess( TempData, srcData, copyPitch, m_sourceEndianess );
				srcData += m_sourceRowPitch;

				{
					const uint16* tempPtr = (const uint16*) &TempData;
					uint8* destWrite = (uint8*) targetData;
					for ( uint32 x=0; x<copyWidth; ++x, ++tempPtr, destWrite += 4 )
					{
						destWrite[0] = ((*tempPtr >> 0) & 0xF) << 4;
						destWrite[1] = ((*tempPtr >> 4) & 0xF) << 4;
						destWrite[2] = ((*tempPtr >> 8) & 0xF) << 4;
						destWrite[3] = ((*tempPtr >> 12) & 0xF) << 4;
					}
				}

				targetData += destRowPitch;
			}

			/*FILE* f = fopen("H:\\shaderdump\\tex512_512.raw", "wb");
			if ( f)
			{
				fwrite( destData, m_height * destRowPitch, 1, f );
				fclose( f );
			}*/
		}
		else
		{
			// slow row-by-row copy
			const uint32 copyPitch = std::min( m_sourceRowPitch, destRowPitch );
			uint32 copyHeight = std::min( m_sourceBlockHeight, m_sourceHeight );
			for ( uint32 y=0; y<copyHeight; ++y )
			{
				Helper::ConvertGPUEndianess( targetData, srcData, copyPitch, m_sourceEndianess );
				targetData += destRowPitch;
				srcData += m_sourceRowPitch;
			}
		}
	}
}

//---------------------------------------------------------------------------

CDX11AbstractTexture::CDX11AbstractTexture()
	: m_sourceType( XenonTextureType::Texture_2D )
	, m_sourceMips( 0 )
	, m_sourceArraySlices( 0 )
	, m_runtimeFormat( DXGI_FORMAT_UNKNOWN )
	, m_runtimeTexture( nullptr )
	, m_viewFormat( DXGI_FORMAT_UNKNOWN )
	, m_view( nullptr )
	, m_initialDirty( true )
{
}

CDX11AbstractTexture::~CDX11AbstractTexture()
{
	// release the view for the shaders
	if ( m_view )
	{
		m_view->Release();
		m_view = nullptr;
	}

	// release the runtime texture
	if ( m_runtimeTexture )
	{
		m_runtimeTexture->Release();
		m_runtimeTexture = nullptr;
	}

	// delete actual surfaces
	for ( auto* ptr : m_surfaces )
		delete ptr;
}

uint32 CDX11AbstractTexture::GetBaseAddress() const
{
	return m_sourceInfo.m_address;
}

XenonTextureFormat CDX11AbstractTexture::GetFormat() const
{
	return m_sourceInfo.m_format->m_format;
}

XenonTextureType CDX11AbstractTexture::GetType() const
{
	return m_sourceType;
}

uint32 CDX11AbstractTexture::GetBaseWidth() const
{
	if ( m_sourceType == XenonTextureType::Texture_2D )
		return m_sourceInfo.m_size2D.m_logicalWidth;

	return 0;
}

uint32 CDX11AbstractTexture::GetBaseHeight() const
{
	if ( m_sourceType == XenonTextureType::Texture_2D )
		return m_sourceInfo.m_size2D.m_logicalHeight;

	return 0;
}

uint32 CDX11AbstractTexture::GetBaseDepth() const
{
	if ( m_sourceType == XenonTextureType::Texture_2D )
		return 1;

	return 0;
}

uint32 CDX11AbstractTexture::GetNumMipLevels() const
{
	return m_sourceMips;
}

uint32 CDX11AbstractTexture::GetNumArraySlices() const
{
	return m_sourceArraySlices;
}

IXenonGPUAbstractSurface* CDX11AbstractTexture::GetSurface( const uint32 slice, const uint32 mip )
{
	DEBUG_CHECK( slice < m_sourceArraySlices );
	DEBUG_CHECK( mip < m_sourceMips );

	// out of bounds
	if ( slice >= m_sourceArraySlices || mip >= m_sourceMips )
		return nullptr;

	// get the surface
	return m_surfaces[ (slice*m_sourceMips) + mip ];
}

bool CDX11AbstractTexture::IsBlockCompressedFormat( XenonTextureFormat sourceFormat )
{
	switch (  sourceFormat )
	{
		case XenonTextureFormat::Format_DXT1: 
		case XenonTextureFormat::Format_DXT1_AS_16_16_16_16: 
		case XenonTextureFormat::Format_DXT2_3:
		case XenonTextureFormat::Format_DXT2_3_AS_16_16_16_16:
		case XenonTextureFormat::Format_DXT4_5:
		case XenonTextureFormat::Format_DXT4_5_AS_16_16_16_16:
			return true;
	}

	return false;
}

bool CDX11AbstractTexture::MapFormat( XenonTextureFormat sourceFormat, DXGI_FORMAT& runtimeFormat, DXGI_FORMAT& viewFormat)
{
	switch ( sourceFormat )
	{
		// formats that will fit in standard 8888 texture
		case XenonTextureFormat::Format_1: 
		case XenonTextureFormat::Format_8:
		case XenonTextureFormat::Format_1_5_5_5:
		case XenonTextureFormat::Format_5_6_5:
		case XenonTextureFormat::Format_6_5_5:
		case XenonTextureFormat::Format_8_8_8_8:
		case XenonTextureFormat::Format_8_8_8_8_AS_16_16_16_16:
		case XenonTextureFormat::Format_4_4_4_4:
			runtimeFormat = DXGI_FORMAT_R8G8B8A8_TYPELESS;
			viewFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
			return true;

		// poors man HDR - formats that will fit in standard 4x16 texture
		case XenonTextureFormat::Format_2_10_10_10:
		case XenonTextureFormat::Format_10_11_11:
		case XenonTextureFormat::Format_11_11_10:
		case XenonTextureFormat::Format_16_16_16_16:
		case XenonTextureFormat::Format_2_10_10_10_AS_16_16_16_16:
		case XenonTextureFormat::Format_16_16_16_16_EXPAND:
			runtimeFormat = DXGI_FORMAT_R16G16B16A16_TYPELESS;
			viewFormat = DXGI_FORMAT_R16G16B16A16_UNORM;
			return true;
		
		// DXT
		case XenonTextureFormat::Format_DXT1: 
		case XenonTextureFormat::Format_DXT1_AS_16_16_16_16: 
			runtimeFormat = DXGI_FORMAT_BC1_UNORM;
			viewFormat = DXGI_FORMAT_BC1_UNORM;
			return true;

		case XenonTextureFormat::Format_DXT2_3:
		case XenonTextureFormat::Format_DXT2_3_AS_16_16_16_16:
			runtimeFormat = DXGI_FORMAT_BC2_UNORM;
			viewFormat = DXGI_FORMAT_BC2_UNORM;
			return true;

		case XenonTextureFormat::Format_DXT4_5:
		case XenonTextureFormat::Format_DXT4_5_AS_16_16_16_16:
			runtimeFormat = DXGI_FORMAT_BC3_UNORM;
			viewFormat = DXGI_FORMAT_BC3_UNORM;
			return true;

		// Special formats
		case XenonTextureFormat::Format_24_8:
			runtimeFormat = DXGI_FORMAT_R24G8_TYPELESS;
			viewFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS; // no stencil mapping
			return true;

		case XenonTextureFormat::Format_24_8_FLOAT:
			runtimeFormat = DXGI_FORMAT_R24G8_TYPELESS;
			viewFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS; // no stencil mapping
			return true;

		case XenonTextureFormat::Format_16:
		case XenonTextureFormat::Format_16_EXPAND:
			runtimeFormat = DXGI_FORMAT_R16_TYPELESS;
			viewFormat = DXGI_FORMAT_R16G16_UNORM;
			return true;

		case XenonTextureFormat::Format_16_16:
		case XenonTextureFormat::Format_16_16_EXPAND:
			runtimeFormat = DXGI_FORMAT_R16G16_TYPELESS;
			viewFormat = DXGI_FORMAT_R16G16_UNORM;
			return true;

		case XenonTextureFormat::Format_16_FLOAT:
			runtimeFormat = DXGI_FORMAT_R16_TYPELESS;
			viewFormat = DXGI_FORMAT_R16_FLOAT;
			return true;

		case XenonTextureFormat::Format_16_16_FLOAT:
			runtimeFormat = DXGI_FORMAT_R16G16_TYPELESS;
			viewFormat = DXGI_FORMAT_R16G16_FLOAT;
			return true;

		case XenonTextureFormat::Format_16_16_16_16_FLOAT:
			runtimeFormat = DXGI_FORMAT_R16G16B16A16_TYPELESS;
			viewFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
			return true;

		case XenonTextureFormat::Format_32:
			runtimeFormat = DXGI_FORMAT_R32_TYPELESS;
			viewFormat = DXGI_FORMAT_R32_UINT;
			return true;

		case XenonTextureFormat::Format_32_32:
			runtimeFormat = DXGI_FORMAT_R32G32_TYPELESS;
			viewFormat = DXGI_FORMAT_R32G32_UINT;
			return true;

		case XenonTextureFormat::Format_32_32_32_32:
			runtimeFormat = DXGI_FORMAT_R32G32B32A32_TYPELESS;
			viewFormat = DXGI_FORMAT_R32G32B32A32_UINT;
			return true;

		case XenonTextureFormat::Format_32_FLOAT:
			runtimeFormat = DXGI_FORMAT_R32_TYPELESS;
			viewFormat = DXGI_FORMAT_R32G32_FLOAT;
			return true;

		case XenonTextureFormat::Format_32_32_FLOAT:
			runtimeFormat = DXGI_FORMAT_R32G32_TYPELESS;
			viewFormat = DXGI_FORMAT_R32G32_FLOAT;
			return true;

		case XenonTextureFormat::Format_32_32_32_32_FLOAT:
			runtimeFormat = DXGI_FORMAT_R32G32B32A32_TYPELESS;
			viewFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
			return true;
	}

	DEBUG_CHECK( !"Unusable source texture format, runtime texture not created" );
	return false;
}

bool CDX11AbstractTexture::CreateResources( ID3D11Device* device )
{
	// 2D
	if ( m_sourceType == XenonTextureType::Texture_2D )
	{
		DEBUG_CHECK( m_runtimeTexture == nullptr );
		DEBUG_CHECK( m_sourceArraySlices == 1 ); // for now

		// setup texture descriptor
		D3D11_TEXTURE2D_DESC texDesc;
		memset( &texDesc, 0, sizeof(texDesc) );
		texDesc.ArraySize = m_sourceArraySlices;
		texDesc.MipLevels = m_sourceMips;
		texDesc.Format = m_runtimeFormat;
		texDesc.Width = m_sourceInfo.m_size2D.m_logicalWidth;
		texDesc.Height = m_sourceInfo.m_size2D.m_logicalHeight;
		texDesc.CPUAccessFlags = 0; // never accessed by CPU
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE; // never a render target but can be accessed via UAV
		texDesc.MiscFlags = 0;
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;

		// create texture object
		ID3D11Texture2D* tex = nullptr;
		HRESULT hRet = device->CreateTexture2D( &texDesc, NULL, &tex );
		if ( FAILED(hRet) || !tex )
		{
			GLog.Err( "D3D: Error creating 2D texture: 0x%08X", hRet );
			return false;
		}

		// setup view description (using the view format)
		D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
		memset( &viewDesc, 0, sizeof(viewDesc) );
		viewDesc.Format = m_viewFormat;
		viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		viewDesc.Texture2D.MipLevels = m_sourceMips;
		viewDesc.Texture2D.MostDetailedMip = 0;

		// create basic shader resource view
		ID3D11ShaderResourceView* view = nullptr;
		hRet = device->CreateShaderResourceView( tex, &viewDesc, &view );
		if ( FAILED(hRet) || !view )
		{
			tex->Release();
			GLog.Err( "D3D: Error creating view for 2D texture: 0x%08X", hRet );
			return false;
		}

		// setup
		m_view = view;
		m_runtimeTexture = tex;
		return true;
	}

	// not done
	return false;
}

bool CDX11AbstractTexture::CreateSurfaces( ID3D11Device* device )
{
	// 2D
	if ( m_sourceType == XenonTextureType::Texture_2D )
	{
		DEBUG_CHECK( m_sourceArraySlices == 1 );
		DEBUG_CHECK( m_sourceMips == 1 );
		DEBUG_CHECK( m_runtimeTexture != nullptr );

		for ( uint32 slice=0; slice<m_sourceArraySlices; ++slice )
		{
			for ( uint32 mip=0; mip<m_sourceMips; ++mip )
			{
				const uint32 memoryOffset = 0; // TODO

				// create surface info
				CDX11AbstractSurface* surf = new CDX11AbstractSurface();
				surf->m_sourceFormat = m_sourceInfo.m_format->m_format;
				surf->m_sourceFormatBlockWidth = m_sourceInfo.m_format->m_blockWidth;
				surf->m_width = std::max<uint32>( 1, m_sourceInfo.m_size2D.m_logicalWidth >> mip );
				surf->m_height = std::max<uint32>( 1, m_sourceInfo.m_size2D.m_logicalHeight >> mip );
				surf->m_depth = 1;

				surf->m_sourceWidth = std::max<uint32>( 1, m_sourceInfo.m_size2D.m_actualWidth >> mip );
				surf->m_sourceHeight = std::max<uint32>( 1, m_sourceInfo.m_size2D.m_actualHeight >> mip );
				surf->m_sourceBlockWidth = std::max<uint32>( 1, m_sourceInfo.m_size2D.m_actualBlockWidth >> mip );
				surf->m_sourceBlockHeight = std::max<uint32>( 1, m_sourceInfo.m_size2D.m_actualBlockHeight >> mip );
				surf->m_sourceRowPitch = m_sourceInfo.m_size2D.m_actualPitch;
				surf->m_sourceSlicePitch = 0;
				surf->m_sourceMemoryOffset = memoryOffset;
				surf->m_sourceIsTiled = m_sourceInfo.m_isTiled;
				surf->m_sourceEndianess = m_sourceInfo.m_endianness;
				surf->m_isBlockCompressed = IsBlockCompressedFormat( m_sourceInfo.m_format->m_format );

				// size of the single data block
				const uint32 blockSize = m_sourceInfo.m_format->GetBlockSizeInBytes();
				surf->m_sourceBlockSize = blockSize;

				// tiling offset
				m_sourceInfo.GetPackedTileOffset( surf->m_sourcePackedTileOffsetX, surf->m_sourcePackedTileOffsetY );

				// runtime format of the surface is the same
				surf->m_runtimeFormat = m_runtimeFormat;

				// add to surface list
				m_surfaces.push_back( surf );
			}
		}

		// done
		return true;
	}
	
	// not done
	return false;
}

bool CDX11AbstractTexture::CreateStagingBuffers( CDX11StagingTextureCache* stagingDataCache )
{
	for ( uint32 mip=0; mip<m_sourceMips; ++mip )
	{
		uint32 width=1, height=1, depth=1, dims=1;

		if ( m_sourceType == XenonTextureType::Texture_1D )
		{
			width = std::max<uint32>( 1, m_sourceInfo.m_size1D.m_width >> mip );
			dims = 1;
		}
		else if ( m_sourceType == XenonTextureType::Texture_2D || m_sourceType == XenonTextureType::Texture_Cube )
		{
			width = std::max<uint32>( 1, m_sourceInfo.m_size2D.m_logicalWidth >> mip );
			height = std::max<uint32>( 1, m_sourceInfo.m_size2D.m_logicalHeight >> mip );
			dims = 2;
		}
		else if ( m_sourceType == XenonTextureType::Texture_3D )
		{
			continue;
		}

		auto* stagingBuffer = stagingDataCache->GetStagingTexture( dims, m_runtimeFormat, width, height, depth );
		if ( !stagingBuffer )
			return false;

		m_stagingBuffers.push_back( stagingBuffer );
	}

	// created
	return true;
}

CDX11AbstractTexture* CDX11AbstractTexture::Create( ID3D11Device* device, CDX11StagingTextureCache* stagingCache, const struct XenonTextureInfo* textureInfo )
{
	// empty texture
	if ( !textureInfo || !textureInfo->m_address )
		return nullptr;

	// convert format
	DXGI_FORMAT runtimeFormat, viewFormat;
	const auto* format = textureInfo->m_format;
	if ( !MapFormat( format->m_format, runtimeFormat, viewFormat) )
	{
		GLog.Err( "D3D: Unable to create wrapper for 2D texture of format %hs, width=%d, height=%d", 
			XenonGPUTextureFormatName( format->m_format ), textureInfo->m_width, textureInfo->m_height );
		return nullptr;
	}

	// Create texture wrapper
	std::auto_ptr< CDX11AbstractTexture > ret( new CDX11AbstractTexture() );
	if ( textureInfo->m_dimension == XenonTextureDimension::Dimmension_2D )
	{
		ret->m_sourceInfo = *textureInfo;
		ret->m_sourceType = XenonTextureType::Texture_2D;
		ret->m_sourceArraySlices = 1;
		ret->m_sourceMips = 1;
		ret->m_runtimeFormat = runtimeFormat;
		ret->m_viewFormat = viewFormat;
		ret->m_initialDirty = true;
	}
	else if ( textureInfo->m_dimension == XenonTextureDimension::Dimmension_3D )
	{
		ret->m_sourceInfo = *textureInfo;
		ret->m_sourceType = XenonTextureType::Texture_3D;
		ret->m_sourceArraySlices = 1;
		ret->m_sourceMips = 1;
		ret->m_runtimeFormat = runtimeFormat;
		ret->m_viewFormat = viewFormat;
		ret->m_initialDirty = true;
	}
	else
	{
		return nullptr;
	}

	// create the resource
	if ( !ret->CreateResources( device ) )
		return nullptr;

	// create the surfaces
	if ( !ret->CreateSurfaces( device ) )
		return nullptr;

	// create staging buffers
	if ( stagingCache )
		if ( !ret->CreateStagingBuffers( stagingCache ) )
			return nullptr;

	// return created texture
	return ret.release();
}

bool CDX11AbstractTexture::ShouldBeUpdated() const
{
	// texture has never been updated
	if ( m_initialDirty )
		return true;

	// TODO: memory range check :)

	return false;
}

void CDX11AbstractTexture::EnsureUpToDate()
{
	// never update if not needed
	if ( !ShouldBeUpdated() )
		return;

	// upload mips
	Update();

}

void CDX11AbstractTexture::Update()
{
	// get address for the SOURCE texture data
	const uint32 baseAddress = GetBaseAddress();
	const void* srcData = (const void*) GPlatform.GetMemory().TranslatePhysicalAddress( baseAddress );

	// upload ALL mips and slices
	for ( uint32 slice=0; slice<m_sourceArraySlices; ++slice )
	{
		for ( uint32 mip=0; mip<m_sourceMips; ++mip )
		{
			// target mip
			const uint32 runtimeSliceIndex = slice*m_sourceMips + mip;
			auto* surfacePtr = m_surfaces[ runtimeSliceIndex ];
			if ( !surfacePtr )
				continue;

			// no staging buffer
			if ( mip >= m_stagingBuffers.size() )
				continue;

			// invalid staging buffer
			auto* stagingPtr = m_stagingBuffers[ mip ];
			if ( !stagingPtr )
				continue;

			// lock the staging buffer for update
			uint32 destRowPitch=0, destSlicePitch=0;
			void* destPtr = stagingPtr->Lock( destRowPitch, destSlicePitch );
			if ( !destPtr )
				continue;

			// upload surface
			surfacePtr->Upload( srcData, destPtr, destRowPitch, destSlicePitch );

			// upload staging buffer
			stagingPtr->Flush( m_runtimeTexture, runtimeSliceIndex );
		}
	}

	// mark as updated at least once
	m_initialDirty = false;
}


//---------------------------------------------------------------------------

CDX11TextureManager::CDX11TextureManager( ID3D11Device* device, ID3D11DeviceContext* context )
	: m_device( device )
	, m_context( context )
{
	// create staging texture cache
	m_stagingCache = new CDX11StagingTextureCache( device, context );
}

CDX11TextureManager::~CDX11TextureManager()
{
	// release staging texture cache
	if ( m_stagingCache )
	{
		delete m_stagingCache;
		m_stagingCache = nullptr;
	}
}

void CDX11TextureManager::EvictTextures( const EvictionPolicy policy )
{
	TScopeLock lock( m_lock );

}

CDX11AbstractTexture* CDX11TextureManager::FindTexture( const uint32 baseAddress )
{
	TScopeLock lock( m_lock );

	// ensure the address is a GPU address
	DEBUG_CHECK( (baseAddress & 0xF0000000) == 0 );

	// look in existing textures
	// TODO: this is ultra lame
	for ( auto* ptr : m_textures )
	{
		if ( ptr->GetBaseAddress() == baseAddress )
		{
			return ptr;
		}
	}

	// not found
	return nullptr;
}

CDX11AbstractTexture* CDX11TextureManager::GetTexture( const uint32 baseAddress, const uint32 width, const uint32 height, const XenonTextureFormat format )
{
	TScopeLock lock( m_lock );

	// ensure the address is a GPU address
	DEBUG_CHECK( (baseAddress & 0xF0000000) == 0 );

	// look in existing textures
	// TODO: this is ultra lame
	for ( auto* ptr : m_textures )
	{
		if ( ptr->GetBaseAddress() == baseAddress )
		{
			if ( ptr->GetNumMipLevels() == 1 && ptr->GetNumArraySlices() == 1 && ptr->GetFormat() == format )
			{
				const CDX11AbstractSurface* mip0 = static_cast<const CDX11AbstractSurface*>( ptr->GetSurface(0,0));

				if ( mip0->GetWidth() == width && mip0->GetHeight() == height && mip0->GetDepth() == 1 )
				{
					return ptr;
				}
			}
		}
	}

	// create new 2D textures
	XenonTextureInfo info;
	memset( &info, 0, sizeof(info) );
	info.m_address = baseAddress;
	info.m_depth = 1;
	info.m_dimension = XenonTextureDimension::Dimmension_2D;
	info.m_format = XenonTextureFormatInfo::Get( (uint32) format );
	info.m_isTiled = false; // RT's are never tiles
	info.m_width = width;
	info.m_height = height;
	info.m_size2D.m_logicalWidth = width;
	info.m_size2D.m_logicalHeight = height;
	info.m_size2D.m_actualWidth = width;
	info.m_size2D.m_actualHeight = height;
	info.m_size2D.m_actualPitch = width * 4;
	info.m_size2D.m_actualBlockWidth = Helper::RoundUp( width, 32 );
	info.m_size2D.m_actualBlockHeight = Helper::RoundUp( height, 32 );
	info.m_endianness = XenonGPUEndianFormat::FormatUnspecified;

	// create the texture using the requested data
	CDX11AbstractTexture* tex = CDX11AbstractTexture::Create( m_device, nullptr, &info );
	if ( !tex )
	{
		GLog.Err( "D3D: Unable to create RT texture %dx%d, format %hs at 0x%08X", width, height, XenonGPUTextureFormatName(format), baseAddress );
		return nullptr;
	}

	// created
	GLog.Log( "D3D: Created RT texture %dx%d, format %hs at 0x%08X", width, height, XenonGPUTextureFormatName(format), baseAddress );

	// add to cache
	m_textures.push_back( tex );
	return tex;
}

CDX11AbstractTexture* CDX11TextureManager::GetTexture( const XenonTextureInfo* textureInfo )
{
	TScopeLock lock( m_lock );

	// look in existing textures
	// TODO: this is ultra lame
	for ( auto* ptr : m_textures )
	{
		if ( ptr->GetBaseAddress() == textureInfo->m_address )
		{
			return ptr;
		}
	}

	// create the texture using the requested data
	CDX11AbstractTexture* tex = CDX11AbstractTexture::Create( m_device, m_stagingCache, textureInfo );
	if ( !tex )
	{
		GLog.Err( "D3D: Unable to upload texture %dx%d, format %hs at 0x%08X", textureInfo->m_width, textureInfo->m_height, XenonGPUTextureFormatName(textureInfo->m_format->m_format), textureInfo->m_address );
		return nullptr;
	}

	// created
	GLog.Log( "D3D: Created texture %dx%d, format %hs at 0x%08X", textureInfo->m_width, textureInfo->m_height, XenonGPUTextureFormatName(textureInfo->m_format->m_format), textureInfo->m_address );

	// add to cache
	m_textures.push_back( tex );
	return tex;
}

//---------------------------------------------------------------------------
