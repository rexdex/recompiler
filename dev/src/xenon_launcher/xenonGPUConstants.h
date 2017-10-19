#pragma once

enum class XenonGPUEndianFormat : uint32
{
	FormatUnspecified = 0,
	Format8in16 = 1,
	Format8in32 = 2,
	Format16in32 = 3,
};

enum class XenonGPUEndianFormat128 : uint32
{
	FormatUnspecified = 0,
	Format8in16 = 1,
	Format8in32 = 2,
	Format16in32 = 3,
	Format8in64 = 4,
	Format8in128 = 5,
};

enum class XenonPrimitiveType : uint32
{
	PrimitiveNone = 0x00,
	PrimitivePointList = 0x01,
	PrimitiveLineList = 0x02,
	PrimitiveLineStrip = 0x03,
	PrimitiveTriangleList = 0x04,
	PrimitiveTriangleFan = 0x05,
	PrimitiveTriangleStrip = 0x06,
	PrimitiveUnknown0x07 = 0x07,
	PrimitiveRectangleList = 0x08,
	PrimitiveLineLoop = 0x0C,
	PrimitiveQuadList = 0x0D,
	PrimitiveQuadStrip = 0x0E,
};

enum class XenonIndexFormat : uint32
{
	Index16,
	Index32,
};

enum class XenonShaderType : uint32
{
	ShaderVertex = 0,
	ShaderPixel = 1,
};

enum class XenonMsaaSamples : uint32
{
	MSSA1X = 0,
	MSSA2X = 1,
	MSSA4X = 2,
};

enum class XenonColorRenderTargetFormat : uint32
{
	Format_8_8_8_8 = 0,        // D3DFMT_A8R8G8B8 (or ABGR?)
	Format_8_8_8_8_GAMMA = 1,  // D3DFMT_A8R8G8B8 with gamma correction
	Format_2_10_10_10 = 2,
	Format_2_10_10_10_FLOAT = 3,
	Format_16_16 = 4,
	Format_16_16_16_16 = 5,
	Format_16_16_FLOAT = 6,
	Format_16_16_16_16_FLOAT = 7,
	Format_2_10_10_10_unknown = 10,
	Format_2_10_10_10_FLOAT_unknown = 12,
	Format_32_FLOAT = 14,
	Format_32_32_FLOAT = 15,
};

enum class XenonDepthRenderTargetFormat : uint32
{
	Format_D24S8 = 0,
	Format_D24FS8 = 1,
};

enum class XenonModeControl : uint32
{
	Ignore = 0,
	ColorDepth = 4,
	Depth = 5,
	Copy = 6,
};

enum class XenonCopyCommand : uint32
{
	Raw = 0,
	Convert = 1,
	ConstantOne = 2,
	Null = 3,  // ?
};

// Subset of a2xx_sq_surfaceformat.
enum class XenonColorFormat : uint32
{
	Unknown = 0,
	Format_8 = 2,
	Format_1_5_5_5 = 3,
	Format_5_6_5 = 4,
	Format_6_5_5 = 5,
	Format_8_8_8_8 = 6,
	Format_2_10_10_10 = 7,
	Format_8_A = 8,
	Format_8_B = 9,
	Format_8_8 = 10,
	Format_8_8_8_8_A = 14,
	Format_4_4_4_4 = 15,
	Format_10_11_11 = 16,
	Format_11_11_10 = 17,
	Format_16 = 24,
	Format_16_16 = 25,
	Format_16_16_16_16 = 26,
	Format_16_FLOAT = 30,
	Format_16_16_FLOAT = 31,
	Format_16_16_16_16_FLOAT = 32,
	Format_32_FLOAT = 36,
	Format_32_32_FLOAT = 37,
	Format_32_32_32_32_FLOAT = 38,
	Format_2_10_10_10_FLOAT = 62,
};

enum class XenonVertexFormat : uint32
{
	Format_8_8_8_8 = 6,
	Format_2_10_10_10 = 7,
	Format_10_11_11 = 16,
	Format_11_11_10 = 17,
	Format_16_16 = 25,
	Format_16_16_16_16 = 26,
	Format_16_16_FLOAT = 31,
	Format_16_16_16_16_FLOAT = 32,
	Format_32 = 33,
	Format_32_32 = 34,
	Format_32_32_32_32 = 35,
	Format_32_FLOAT = 36,
	Format_32_32_FLOAT = 37,
	Format_32_32_32_32_FLOAT = 38,
	Format_32_32_32_FLOAT = 57,
};

inline int GetVertexFormatComponentCount(XenonVertexFormat format)
{
	switch (format)
	{
		case XenonVertexFormat::Format_32:
		case XenonVertexFormat::Format_32_FLOAT:
			return 1;
			break;

		case XenonVertexFormat::Format_16_16:
		case XenonVertexFormat::Format_16_16_FLOAT:
		case XenonVertexFormat::Format_32_32:
		case XenonVertexFormat::Format_32_32_FLOAT:
			return 2;
			break;

		case XenonVertexFormat::Format_10_11_11:
		case XenonVertexFormat::Format_11_11_10:
		case XenonVertexFormat::Format_32_32_32_FLOAT:
			return 3;
			break;

		case XenonVertexFormat::Format_8_8_8_8:
		case XenonVertexFormat::Format_2_10_10_10:
		case XenonVertexFormat::Format_16_16_16_16:
		case XenonVertexFormat::Format_16_16_16_16_FLOAT:
		case XenonVertexFormat::Format_32_32_32_32:
		case XenonVertexFormat::Format_32_32_32_32_FLOAT:
			return 4;
			break;
	}

	DEBUG_CHECK(!"Unhandled vertex format");
	return 0;
}

enum class XenonCmpFunc : uint32
{
	Never,
	Less,
	Equal,
	LessEqual,
	Greater,
	NotEqual,
	GreaterEqual,
	Always,
};

enum class XenonStencilOp : uint32
{
	Keep,
	Zero,
	Replace,
	IncrWrap,
	DecrWrap,
	Invert,
	Incr,
	Decr,
};

enum class XenonBlendArg : uint32
{
	Zero = 0,
	One = 1,
	Unknown1 = 2, //?? 
	Unknown2 = 3, //?? 
	SrcColor = 4,
	OneMinusSrcColor = 5,
	SrcAlpha = 6,
	OneMinusSrcAlpha = 7,
	DestColor = 8,
	OneMinusDestColor = 9,
	DestAlpha = 10,
	OneMinusDestAlpha = 11,
	ConstColor = 12,
	OneMinusConstColor = 13,
	ConstAlpha = 14,
	OneMinusConstAlpha = 15,
	SrcAlphaSaturate = 16,
};

enum class XenonBlendOp : uint32
{
	Add = 0,
	Subtract = 1,
	Min = 2,
	Max = 3,
	ReverseSubtract = 4,
};

enum class XenonCullMode : uint32
{
	None,
	Front,
	Back,
};

enum class XenonFrontFace : uint32
{
	CW,
	CCW,
};

enum class XenonFillMode : uint32
{
	Point,
	Line,
	Solid,
};

// type of xenon texture
enum class XenonTextureType : uint32
{
	Texture_1D,
	Texture_2D,
	Texture_3D,
	Texture_Cube,
};

// copied from a2xx_sq_surfaceformat
enum class XenonTextureFormat : uint32
{
	Format_1_REVERSE = 0,
	Format_1 = 1,
	Format_8 = 2,
	Format_1_5_5_5 = 3,
	Format_5_6_5 = 4,
	Format_6_5_5 = 5,
	Format_8_8_8_8 = 6,
	Format_2_10_10_10 = 7,
	Format_8_A = 8,
	Format_8_B = 9,
	Format_8_8 = 10,
	Format_Cr_Y1_Cb_Y0 = 11,
	Format_Y1_Cr_Y0_Cb = 12,
	Format_8_8_8_8_A = 14,
	Format_4_4_4_4 = 15,
	Format_10_11_11 = 16,
	Format_11_11_10 = 17,
	Format_DXT1 = 18,
	Format_DXT2_3 = 19,
	Format_DXT4_5 = 20,
	Format_24_8 = 22,
	Format_24_8_FLOAT = 23,
	Format_16 = 24,
	Format_16_16 = 25,
	Format_16_16_16_16 = 26,
	Format_16_EXPAND = 27,
	Format_16_16_EXPAND = 28,
	Format_16_16_16_16_EXPAND = 29,
	Format_16_FLOAT = 30,
	Format_16_16_FLOAT = 31,
	Format_16_16_16_16_FLOAT = 32,
	Format_32 = 33,
	Format_32_32 = 34,
	Format_32_32_32_32 = 35,
	Format_32_FLOAT = 36,
	Format_32_32_FLOAT = 37,
	Format_32_32_32_32_FLOAT = 38,
	Format_32_AS_8 = 39,
	Format_32_AS_8_8 = 40,
	Format_16_MPEG = 41,
	Format_16_16_MPEG = 42,
	Format_8_INTERLACED = 43,
	Format_32_AS_8_INTERLACED = 44,
	Format_32_AS_8_8_INTERLACED = 45,
	Format_16_INTERLACED = 46,
	Format_16_MPEG_INTERLACED = 47,
	Format_16_16_MPEG_INTERLACED = 48,
	Format_DXN = 49,
	Format_8_8_8_8_AS_16_16_16_16 = 50,
	Format_DXT1_AS_16_16_16_16 = 51,
	Format_DXT2_3_AS_16_16_16_16 = 52,
	Format_DXT4_5_AS_16_16_16_16 = 53,
	Format_2_10_10_10_AS_16_16_16_16 = 54,
	Format_10_11_11_AS_16_16_16_16 = 55,
	Format_11_11_10_AS_16_16_16_16 = 56,
	Format_32_32_32_FLOAT = 57,
	Format_DXT3A = 58,
	Format_DXT5A = 59,
	Format_CTX1 = 60,
	Format_DXT3A_AS_1_1_1_1 = 61,
	Format_2_10_10_10_FLOAT = 62,

	Format_Unknown = 0xFFFFFFFFu,
};

enum class XenonTextureDimension : uint32
{
	Dimmension_1D = 0,
	Dimmension_2D = 1,
	Dimmension_3D = 2,
	Dimmension_Cube = 3,
};

enum class XenonTextureFormatType : uint32
{
	Uncompressed,
	Compressed,
};

enum class XenonClampMode : uint32
{
	Repeat = 0,
	MirroredRepeat = 1,
	ClampToEdge = 2,
	MirrorClampToEdge = 3,
	ClampToHalfway = 4,
	MirrorClampToHalfway = 5,
	ClampToBorder = 6,
	MirrorClampToBorder = 7,
};

enum class XenonTextureFilter : uint32
{
	Point = 0,
	Linear = 1,
	BaseMap = 2,
	UseFetchConst = 3,
};

enum class XenonAnisoFilter : uint32
{
	Disabled = 0,
	Max_1_1 = 1,
	Max_2_1 = 2,
	Max_4_1 = 3,
	Max_8_1 = 4,
	Max_16_1 = 5,
	UseFetchConst = 7,
};

enum class XenonBorderColor : uint32
{
	AGBR_Black = 0,
	AGBR_White = 1,
	ACBYCR_BLACK = 2,
	ACBCRY_BLACK = 3,
};
