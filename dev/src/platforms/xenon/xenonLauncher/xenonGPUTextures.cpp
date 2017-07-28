#include "build.h"
#include "xenonGPUTextures.h"
#include "xenonGPUUtils.h"

//#pragma optimize("",off)

//------------------------------------

const uint32 XenonTextureFormatInfo::GetBlockSizeInBytes() const
{
	return (m_blockHeight * m_blockWidth * m_bitsPerBlock) / 8;
}

const XenonTextureFormatInfo* XenonTextureFormatInfo::Get( const uint32 gpuFormat )
{
	static const XenonTextureFormatInfo intoTable[64] =
	{
		{XenonTextureFormat::Format_1_REVERSE, XenonTextureFormatType::Uncompressed, 1, 1, 1},
		{XenonTextureFormat::Format_1, XenonTextureFormatType::Uncompressed, 1, 1, 1},
		{XenonTextureFormat::Format_8, XenonTextureFormatType::Uncompressed, 1, 1, 8},
		{XenonTextureFormat::Format_1_5_5_5, XenonTextureFormatType::Uncompressed, 1, 1, 16},
		{XenonTextureFormat::Format_5_6_5, XenonTextureFormatType::Uncompressed, 1, 1, 16},
		{XenonTextureFormat::Format_6_5_5, XenonTextureFormatType::Uncompressed, 1, 1, 16},
		{XenonTextureFormat::Format_8_8_8_8, XenonTextureFormatType::Uncompressed, 1, 1, 32},
		{XenonTextureFormat::Format_2_10_10_10, XenonTextureFormatType::Uncompressed, 1, 1, 32},
		{XenonTextureFormat::Format_8_A, XenonTextureFormatType::Uncompressed, 1, 1, 8},
		{XenonTextureFormat::Format_8_B, XenonTextureFormatType::Uncompressed, 1, 1, 8},
		{XenonTextureFormat::Format_8_8, XenonTextureFormatType::Uncompressed, 1, 1, 16},
		{XenonTextureFormat::Format_Cr_Y1_Cb_Y0, XenonTextureFormatType::Compressed, 2, 1, 16},
		{XenonTextureFormat::Format_Y1_Cr_Y0_Cb, XenonTextureFormatType::Compressed, 2, 1, 16},
		{XenonTextureFormat::Format_Unknown, XenonTextureFormatType::Uncompressed, 0, 0},
		{XenonTextureFormat::Format_8_8_8_8_A, XenonTextureFormatType::Uncompressed, 1, 1, 32},
		{XenonTextureFormat::Format_4_4_4_4, XenonTextureFormatType::Uncompressed, 1, 1, 16},
		{XenonTextureFormat::Format_10_11_11, XenonTextureFormatType::Uncompressed, 1, 1, 32},
		{XenonTextureFormat::Format_11_11_10, XenonTextureFormatType::Uncompressed, 1, 1, 32},
		{XenonTextureFormat::Format_DXT1, XenonTextureFormatType::Compressed, 4, 4, 4},
		{XenonTextureFormat::Format_DXT2_3, XenonTextureFormatType::Compressed, 4, 4, 8},
		{XenonTextureFormat::Format_DXT4_5, XenonTextureFormatType::Compressed, 4, 4, 8},
		{XenonTextureFormat::Format_Unknown, XenonTextureFormatType::Uncompressed, 0, 0},
		{XenonTextureFormat::Format_24_8, XenonTextureFormatType::Uncompressed, 1, 1, 32},
		{XenonTextureFormat::Format_24_8_FLOAT, XenonTextureFormatType::Uncompressed, 1, 1, 32},
		{XenonTextureFormat::Format_16, XenonTextureFormatType::Uncompressed, 1, 1, 16},
		{XenonTextureFormat::Format_16_16, XenonTextureFormatType::Uncompressed, 1, 1, 32},
		{XenonTextureFormat::Format_16_16_16_16, XenonTextureFormatType::Uncompressed, 1, 1, 64},
		{XenonTextureFormat::Format_16_EXPAND, XenonTextureFormatType::Uncompressed, 1, 1, 16},
		{XenonTextureFormat::Format_16_16_EXPAND, XenonTextureFormatType::Uncompressed, 1, 1, 32},
		{XenonTextureFormat::Format_16_16_16_16_EXPAND, XenonTextureFormatType::Uncompressed, 1, 1, 64},
		{XenonTextureFormat::Format_16_FLOAT, XenonTextureFormatType::Uncompressed, 1, 1, 16},
		{XenonTextureFormat::Format_16_16_FLOAT, XenonTextureFormatType::Uncompressed, 1, 1, 32},
		{XenonTextureFormat::Format_16_16_16_16_FLOAT, XenonTextureFormatType::Uncompressed, 1, 1, 64},
		{XenonTextureFormat::Format_32, XenonTextureFormatType::Uncompressed, 1, 1, 32},
		{XenonTextureFormat::Format_32_32, XenonTextureFormatType::Uncompressed, 1, 1, 64},
		{XenonTextureFormat::Format_32_32_32_32, XenonTextureFormatType::Uncompressed, 1, 1, 128},
		{XenonTextureFormat::Format_32_FLOAT, XenonTextureFormatType::Uncompressed, 1, 1, 32},
		{XenonTextureFormat::Format_32_32_FLOAT, XenonTextureFormatType::Uncompressed, 1, 1, 64},
		{XenonTextureFormat::Format_32_32_32_32_FLOAT, XenonTextureFormatType::Uncompressed, 1, 1, 128},
		{XenonTextureFormat::Format_32_AS_8, XenonTextureFormatType::Compressed, 4, 1, 8},
		{XenonTextureFormat::Format_32_AS_8_8, XenonTextureFormatType::Compressed, 2, 1, 16},
		{XenonTextureFormat::Format_16_MPEG, XenonTextureFormatType::Uncompressed, 1, 1, 16},
		{XenonTextureFormat::Format_16_16_MPEG, XenonTextureFormatType::Uncompressed, 1, 1, 32},
		{XenonTextureFormat::Format_8_INTERLACED, XenonTextureFormatType::Uncompressed, 1, 1, 8},
		{XenonTextureFormat::Format_32_AS_8_INTERLACED, XenonTextureFormatType::Compressed, 4, 1, 8},
		{XenonTextureFormat::Format_32_AS_8_8_INTERLACED, XenonTextureFormatType::Compressed, 1, 1, 16},	
		{XenonTextureFormat::Format_16_INTERLACED, XenonTextureFormatType::Uncompressed, 1, 1, 16},
		{XenonTextureFormat::Format_16_MPEG_INTERLACED, XenonTextureFormatType::Uncompressed, 1, 1, 16},
		{XenonTextureFormat::Format_16_16_MPEG_INTERLACED, XenonTextureFormatType::Uncompressed, 1, 1, 32},
		{XenonTextureFormat::Format_DXN, XenonTextureFormatType::Compressed, 4, 4, 8},
		{XenonTextureFormat::Format_8_8_8_8_AS_16_16_16_16, XenonTextureFormatType::Uncompressed, 1, 1, 32},
		{XenonTextureFormat::Format_DXT1_AS_16_16_16_16, XenonTextureFormatType::Compressed, 4, 4, 4},
		{XenonTextureFormat::Format_DXT2_3_AS_16_16_16_16, XenonTextureFormatType::Compressed, 4, 4, 8},
		{XenonTextureFormat::Format_DXT4_5_AS_16_16_16_16, XenonTextureFormatType::Compressed, 4, 4,	8},
		{XenonTextureFormat::Format_2_10_10_10_AS_16_16_16_16, XenonTextureFormatType::Uncompressed, 1, 1, 32},
		{XenonTextureFormat::Format_10_11_11_AS_16_16_16_16, XenonTextureFormatType::Uncompressed, 1, 1, 32},
		{XenonTextureFormat::Format_11_11_10_AS_16_16_16_16, XenonTextureFormatType::Uncompressed, 1, 1, 32},
		{XenonTextureFormat::Format_32_32_32_FLOAT, XenonTextureFormatType::Uncompressed, 1, 1, 96},
		{XenonTextureFormat::Format_DXT3A, XenonTextureFormatType::Compressed, 4, 4, 4},
		{XenonTextureFormat::Format_DXT5A, XenonTextureFormatType::Compressed, 4, 4, 4},
		{XenonTextureFormat::Format_CTX1, XenonTextureFormatType::Compressed, 4, 4, 4},
		{XenonTextureFormat::Format_DXT3A_AS_1_1_1_1, XenonTextureFormatType::Compressed, 4, 4, 4},
		{XenonTextureFormat::Format_Unknown, XenonTextureFormatType::Uncompressed, 0, 0},
		{XenonTextureFormat::Format_Unknown, XenonTextureFormatType::Uncompressed, 0, 0},
	};

	DEBUG_CHECK( gpuFormat < ARRAYSIZE(intoTable) );
	return &intoTable[ gpuFormat ];
}

//------------------------------------

bool XenonTextureInfo::Parse( const XenonGPUTextureFetch& fetchInfo, XenonTextureInfo& outInfo )
{
	// reset output
	std::memset( &outInfo, 0, sizeof(outInfo) );

	// http://msdn.microsoft.com/en-us/library/windows/desktop/cc308051(v=vs.85).aspx
	// a2xx_sq_surfaceformat
	outInfo.m_address = fetchInfo.address << 12; // 4KB alignment :)(
	outInfo.m_swizzle = fetchInfo.swizzle;

	// get size
	switch ( fetchInfo.dimension )
	{
		case XenonTextureDimension::Dimmension_1D:
		{
			outInfo.m_dimension = XenonTextureDimension::Dimmension_1D;
			outInfo.m_width = fetchInfo.size_1d.width;
			outInfo.m_height = 0;
			outInfo.m_depth = 0;
			break;
		}

		case XenonTextureDimension::Dimmension_2D:
		{
			outInfo.m_dimension = XenonTextureDimension::Dimmension_2D;
			outInfo.m_width = fetchInfo.size_2d.width;
			outInfo.m_height = fetchInfo.size_2d.height;
			outInfo.m_depth = 0;
			break;
		}

		case XenonTextureDimension::Dimmension_3D:
		{
			outInfo.m_dimension = XenonTextureDimension::Dimmension_3D;
			outInfo.m_width = fetchInfo.size_3d.width + 1;
			outInfo.m_height = fetchInfo.size_3d.height + 1;
			outInfo.m_depth = fetchInfo.size_3d.depth + 1;
			break;
		}

		case XenonTextureDimension::Dimmension_Cube:
		{
			outInfo.m_dimension = XenonTextureDimension::Dimmension_Cube;
			outInfo.m_width = fetchInfo.size_stack.width + 1;
			outInfo.m_height = fetchInfo.size_stack.height + 1;
			outInfo.m_depth = fetchInfo.size_stack.depth + 1;
			break;
		}
	}

	outInfo.m_format = XenonTextureFormatInfo::Get( fetchInfo.format );
	outInfo.m_endianness = (XenonGPUEndianFormat) fetchInfo.endianness;
	outInfo.m_isTiled = fetchInfo.tiled;

	// we don't support this texture format
	DEBUG_CHECK( outInfo.m_format->m_format != XenonTextureFormat::Format_Unknown );
	if ( outInfo.m_format->m_format == XenonTextureFormat::Format_Unknown )
		return false;

	// Must be called here when we know the format.
	switch ( outInfo.m_dimension )
	{
		case XenonTextureDimension::Dimmension_1D:
			outInfo.CalculateTextureSizes1D( fetchInfo );
			break;

		case XenonTextureDimension::Dimmension_2D:
			outInfo.CalculateTextureSizes2D( fetchInfo );
			break;

		case XenonTextureDimension::Dimmension_Cube:
			outInfo.CalculateTextureSizesCube( fetchInfo );
			break;
	}

	// valid texture information parsed
	return true;
}

const uint32 XenonTextureInfo::CalculateMemoryRegionSize() const
{
	if ( m_dimension == XenonTextureDimension::Dimmension_2D )
	{
		return m_size2D.m_actualHeight * m_size2D.m_actualPitch;
	}

	// no data
	return 0;
}

void XenonTextureInfo::GetPackedTileOffset( uint32& outOffsetX, uint32& outOffsetY ) const
{
	// Tile size is 32x32, and once textures go <=16 they are packed into a
	// single tile together. The math here is insane. Most sourced
	// from graph paper and looking at dds dumps.
	//   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	// 0         +.4x4.+ +.....8x8.....+ +............16x16............+
	// 1         +.4x4.+ +.....8x8.....+ +............16x16............+
	// 2         +.4x4.+ +.....8x8.....+ +............16x16............+
	// 3         +.4x4.+ +.....8x8.....+ +............16x16............+
	// 4 x               +.....8x8.....+ +............16x16............+
	// 5                 +.....8x8.....+ +............16x16............+
	// 6                 +.....8x8.....+ +............16x16............+
	// 7                 +.....8x8.....+ +............16x16............+
	// 8 2x2                             +............16x16............+
	// 9 2x2                             +............16x16............+
	// 0                                 +............16x16............+
	// ...                                            .....
	// This only works for square textures, or textures that are some non-pot
	// <= square. As soon as the aspect ratio goes weird, the textures start to
	// stretch across tiles.
	// if (tile_aligned(w) > tile_aligned(h)) {
	//   // wider than tall, so packed horizontally
	// } else if (tile_aligned(w) < tile_aligned(h)) {
	//   // taller than wide, so packed vertically
	// } else {
	//   square
	// }
	// It's important to use logical sizes here, as the input sizes will be
	// for the entire packed tile set, not the actual texture.
	// The minimum dimension is what matters most: if either width or height
	// is <= 16 this mode kicks in.

	if ( min( m_size2D.m_logicalWidth, m_size2D.m_logicalHeight ) > 16 )
	{
		// Too big, not packed.
		outOffsetX = 0;
		outOffsetY = 0;
		return;
	}

	if ( Helper::Log2Ceil( m_size2D.m_logicalWidth ) > Helper::Log2Ceil( m_size2D.m_logicalHeight ) )
	{
		// Wider than tall. Laid out vertically.
		outOffsetX = 0;
		outOffsetY = 16;
	}
	else
	{
		// Taller than wide. Laid out horizontally.
		outOffsetX = 16;
		outOffsetY = 0;
	}

	outOffsetX /= m_format->m_blockWidth;
	outOffsetY /= m_format->m_blockHeight;
}

// https://code.google.com/p/crunch/source/browse/trunk/inc/crn_decomp.h#4104
uint32 XenonTextureInfo::TiledOffset2DOuter( const uint32 y, const uint32 width, const uint32 log_bpp )
{
	const uint32 macro = ((y >> 5) * (width >> 5)) << (log_bpp + 7);
	const uint32 micro = ((y & 6) << 2) << log_bpp;
	return macro + ((micro & ~15) << 1) + (micro & 15) + ((y & 8) << (3 + log_bpp)) + ((y & 1) << 4);
}

// https://code.google.com/p/crunch/source/browse/trunk/inc/crn_decomp.h#4104
uint32 XenonTextureInfo::TiledOffset2DInner( const uint32 x, const uint32 y, const uint32 bpp, const uint32 baseOffset )
{
	// copy pasted....
	const uint32 macro = (x >> 5) << (bpp + 7);
	const uint32 micro = (x & 7) << bpp;
	const uint32 offset = baseOffset + (macro + ((micro & ~15) << 1) + (micro & 15));
	return ((offset & ~511) << 3) + ((offset & 448) << 2) + (offset & 63) + ((y & 16) << 7) + (((((y & 8) >> 2) + (x >> 3)) & 3) << 6);
}

const uint64 XenonTextureInfo::GetHash() const
{
	return 0;
}

void XenonTextureInfo::CalculateTextureSizes1D( const XenonGPUTextureFetch& fetch )
{
	m_size1D.m_width = fetch.size_1d.width;
}

void XenonTextureInfo::CalculateTextureSizes2D( const XenonGPUTextureFetch& fetch )
{
	m_size2D.m_logicalWidth = 1 + fetch.size_2d.width;
	m_size2D.m_logicalHeight = 1 + fetch.size_2d.height;

	// w/h in blocks must be a multiple of block size.
	const uint32 blockWidth = Helper::RoundUp( m_size2D.m_logicalWidth, m_format->m_blockWidth ) / m_format->m_blockWidth;
	const uint32 blockHeight = Helper::RoundUp( m_size2D.m_logicalHeight, m_format->m_blockHeight ) / m_format->m_blockHeight;

	// Tiles are 32x32 blocks. All textures must be multiples of tile dimensions.
	const uint32 tileWidth = (uint32)( std::ceilf( blockWidth / 32.0f ) );
	const uint32 tileHeight = (uint32)( std::ceilf( blockHeight / 32.0f ) );
	m_size2D.m_actualBlockWidth = tileWidth * 32;
	m_size2D.m_actualBlockHeight = tileHeight * 32;

	const uint32 bytePerBlock = (m_format->m_blockWidth * m_format->m_blockHeight * m_format->m_bitsPerBlock) / 8;
	uint32 bytePitch = tileWidth * 32 * bytePerBlock;

	// Each row must be a multiple of 256 in linear textures.
	if ( !m_isTiled)
		bytePitch = Helper::RoundUp( bytePitch, 256 );

	m_size2D.m_actualWidth = tileWidth * 32 * m_format->m_blockWidth;
	m_size2D.m_actualHeight = tileHeight * 32 * m_format->m_blockHeight;
	m_size2D.m_actualPitch = bytePitch;
}

void XenonTextureInfo::CalculateTextureSizesCube( const XenonGPUTextureFetch& fetch )
{
	DEBUG_CHECK( fetch.size_stack.depth + 1 == 6 );
	m_sizeCube.m_logicalWidth = 1 + fetch.size_stack.width;
	m_sizeCube.m_logicalHeight = 1 + fetch.size_stack.height;

	// w/h in blocks must be a multiple of block size.
	const uint32 blockWidth = Helper::RoundUp( m_sizeCube.m_logicalWidth, m_format->m_blockWidth) / m_format->m_blockWidth;
	const uint32 blockHeight = Helper::RoundUp( m_sizeCube.m_logicalHeight, m_format->m_blockHeight) / m_format->m_blockHeight;

	// Tiles are 32x32 blocks. All textures must be multiples of tile dimensions.
	const int32 tileWidth = (uint32)( std::ceilf(blockWidth / 32.0f) );
	const int32 tileHeight = (uint32)( std::ceilf(blockHeight / 32.0f) );
	m_sizeCube.m_actualBlockWidth = tileWidth * 32;
	m_sizeCube.m_actualBlockHeight = tileHeight * 32;

	const uint32 bytesPerBlock = (m_format->m_blockWidth * m_format->m_blockHeight * m_format->m_bitsPerBlock) / 8;
	uint32 bytePitch = tileWidth * 32 * bytesPerBlock;

	// Each row must be a multiple of 256 in linear textures.
	if ( !m_isTiled )
		bytePitch = Helper::RoundUp( bytePitch, 256 );

	m_sizeCube.m_actualWidth = tileWidth * 32 * m_format->m_blockWidth;
	m_sizeCube.m_actualHeight = tileHeight * 32 * m_format->m_blockHeight;
	m_sizeCube.m_actualPitch = bytePitch;
	m_sizeCube.m_actualFaceLength = m_sizeCube.m_actualPitch * m_sizeCube.m_actualBlockHeight;
}

//------------------------------------


XenonSamplerInfo::XenonSamplerInfo()
	: m_minFilter(XenonTextureFilter::Linear)
	, m_magFilter(XenonTextureFilter::Linear)
	, m_mipFilter(XenonTextureFilter::Linear)
	, m_clampU(XenonClampMode::Repeat)
	, m_clampV(XenonClampMode::Repeat)
	, m_clampW(XenonClampMode::Repeat)
	, m_anisoFilter(XenonAnisoFilter::Max_1_1)
	, m_borderColor(XenonBorderColor::AGBR_Black)
	, m_lodBias(0.0f)
{
}

uint32 XenonSamplerInfo::GetHash() const
{
	uint32 hash = 0;
	hash = (hash * 4) + (uint32)m_minFilter;
	hash = (hash * 4) + (uint32)m_magFilter;
	hash = (hash * 4) + (uint32)m_mipFilter;
	hash = (hash * 8) + (uint32)m_clampU;
	hash = (hash * 8) + (uint32)m_clampV;
	hash = (hash * 8) + (uint32)m_clampW;
	hash = (hash * 4) + (uint32)m_borderColor;
	hash = (hash * 4) + (uint32)m_anisoFilter;
	hash ^= (uint32&)m_lodBias;
	return hash;
}

XenonSamplerInfo XenonSamplerInfo::Parse(const XenonGPUTextureFetch& info)
{
	XenonSamplerInfo ret;

	ret.m_minFilter = (XenonTextureFilter)info.min_filter;
	ret.m_magFilter = (XenonTextureFilter)info.mag_filter;
	ret.m_mipFilter = (XenonTextureFilter)info.mip_filter;
	ret.m_clampU = (XenonClampMode)info.clamp_x;
	ret.m_clampV = (XenonClampMode)info.clamp_y;
	ret.m_clampW = (XenonClampMode)info.clamp_z;
	//ret.m_anisoFilter = (XenonAnisoFilter)info.;
	ret.m_borderColor = (XenonBorderColor)info.border;
	//ret.m_lodBias = (XenonAnisoFilter)info.mip_min_level;

	return ret;
}