#pragma once

#include "xenonGPUConstants.h"

#pragma pack(push, 4)
union XenonGPUTextureFetch
{
	struct
	{
		uint32 type               : 2;  // dword_0
		uint32 sign_x             : 2;
		uint32 sign_y             : 2;
		uint32 sign_z             : 2;
		uint32 sign_w             : 2;
		uint32 clamp_x            : 3;
		uint32 clamp_y            : 3;
		uint32 clamp_z            : 3;
		uint32 unk0               : 3;
		uint32 pitch              : 9;
		uint32 tiled              : 1;
		uint32 format             : 6;  // dword_1
		uint32 endianness         : 2;
		uint32 unk1               : 4;
		uint32 address            : 20;
		union {                           // dword_2
			struct {
				uint32 width          : 24;
				uint32 unused         : 8;
			} size_1d;
			struct {
				uint32 width          : 13;
				uint32 height         : 13;
				uint32 unused         : 6;
			} size_2d;
			struct {
				uint32 width          : 13;
				uint32 height         : 13;
				uint32 depth          : 6;
			} size_stack;
			struct {
				uint32 width          : 11;
				uint32 height         : 11;
				uint32 depth          : 10;
			} size_3d;
		};
		uint32 unk3_0             :  1; // dword_3
		uint32 swizzle            :  12; // xyzw, 3b each (XE_GPU_SWIZZLE)
		uint32 unk3_1             :  6;
		uint32 mag_filter         :  2;
		uint32 min_filter         :  2;
		uint32 mip_filter         :  2;
		uint32 unk3_2             :  6;
		uint32 border             :  1;
		uint32 unk4_0             :  2; // dword_4
		uint32 mip_min_level      :  4;
		uint32 mip_max_level      :  4;
		uint32 unk4_1             : 22;
		uint32 unk5               :  9;  // dword_5
		uint32 dimension          :  2;
		uint32 unk5b              : 21;
	};

	struct{
		uint32 dword_0;
		uint32 dword_1;
		uint32 dword_2;
		uint32 dword_3;
		uint32 dword_4;
		uint32 dword_5;
	};
};
#pragma pack(pop)

struct XenonTextureFormatInfo
{
	XenonTextureFormat		m_format;
	XenonTextureFormatType	m_type;
	uint32					m_blockWidth;
	uint32					m_blockHeight;;
	uint32					m_bitsPerBlock;

	const uint32 GetBlockSizeInBytes() const;

	static const XenonTextureFormatInfo* Get( const uint32 gpuFormat );
};

struct XenonTextureInfo
{
	uint32							m_address;			// address in physical memory
	uint32							m_swizzle;			// texture swizzling
	XenonTextureDimension			m_dimension;		// texture dimensions (1D, 2D, 3D, Cube)
	uint32							m_width;			// width of texture data (as rendered)
	uint32							m_height;			// height of the texture data (as rendered)
	uint32							m_depth;			// depth of the texture data (as rendered)
	const XenonTextureFormatInfo*	m_format;			// information about the data format
	XenonGPUEndianFormat			m_endianness;		// endianess (BE or LE)
	bool							m_isTiled;			// is the texture tiles ? (affects upload)

	inline const bool IsCompressed() const
	{
		return m_format->m_type == XenonTextureFormatType::Compressed;
	}

	union
	{
		struct
		{
			uint32	m_width;
		} m_size1D;

		struct
		{
			uint32 m_logicalWidth; // size limits for rendering
			uint32 m_logicalHeight; // size limits for rendering
			uint32 m_actualBlockWidth; // actual memory layout blocks
			uint32 m_actualBlockHeight; // actual memory layout blocks
			uint32 m_actualWidth;
			uint32 m_actualHeight;
			uint32 m_actualPitch;
		}
		m_size2D;

		struct
		{
			uint32 m_logicalWidth;
			uint32 m_logicalHeight;
			uint32 m_actualBlockWidth;
			uint32 m_actualBlockHeight;
			uint32 m_actualWidth;
			uint32 m_actualHeight;
			uint32 m_actualPitch;
			uint32 m_actualFaceLength;
		}
		m_sizeCube;
	};

	static bool Parse( const XenonGPUTextureFetch& fetchInfo, XenonTextureInfo& outInfo );

	void GetPackedTileOffset( uint32& outOffsetX, uint32& outOffsetY ) const;

	static uint32 TiledOffset2DOuter( const uint32 y, const uint32 width, const uint32 log_bpp );
	static uint32 TiledOffset2DInner( const uint32 x, const uint32 y, const uint32 bpp, const uint32 baseOffset );

	const uint32 CalculateMemoryRegionSize() const;

	const uint64 GetHash() const;

private:
	void CalculateTextureSizes1D( const XenonGPUTextureFetch& fetch );
	void CalculateTextureSizes2D( const XenonGPUTextureFetch& fetch );
	void CalculateTextureSizesCube( const XenonGPUTextureFetch& fetch );
};