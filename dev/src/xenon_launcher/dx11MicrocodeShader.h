#pragma once

namespace DX11Microcode
{
	class Block;
	class Shader;
	class IHLSLWriter;
};

/// DX11 shader made out disassembled microcode
class CDX11MicrocodeShader
{
public:
	CDX11MicrocodeShader();
	~CDX11MicrocodeShader();

	/// Get shader hash
	inline const uint64 GetHash() const { return m_microcodeHash; }

	/// Is this a pixel shader ?
	inline const bool IsPixelShader() const { return m_bIsPixelShader; }

	/// Build from raw code
	static CDX11MicrocodeShader* ExtractPixelShader( const void* shaderCode, const uint32 shaderCodeSize, const uint64 microcodeHash );
	static CDX11MicrocodeShader* ExtractVertexShader( const void* shaderCode, const uint32 shaderCodeSize, const uint64 microcodeHash );

	// type information
	enum class EFetchFormat : uint8 
	{
		FMT_1_REVERSE,
		FMT_8,// uint stored, signed/unsigned, can be renormalized
		FMT_8_8_8_8,// uint stored, signed/unsigned, can be renormalized
		FMT_2_10_10_10,// uint stored, signed/unsigned, can be renormalized
		FMT_8_8,// uint stored, signed/unsigned, can be renormalized
		FMT_16,// uint stored, signed/unsigned, can be renormalized
		FMT_16_16,// uint stored, signed/unsigned, can be renormalized
		FMT_16_16_16_16,// uint stored, signed/unsigned, can be renormalized
		FMT_16_16_FLOAT,// uint stored, signed/unsigned, can be renormalized
		FMT_16_16_16_16_FLOAT,// uint stored, signed/unsigned, can be renormalized
		FMT_32,// uint stored, signed/unsigned, can be renormalized
		FMT_32_32,// uint stored, signed/unsigned, can be renormalized
		FMT_32_32_32_32, // uint stored, signed/unsigned, can be renormalized
		FMT_32_FLOAT, // float stored
		FMT_32_32_FLOAT, // float stored
		FMT_32_32_32_32_FLOAT, // float stored
		FMT_32_32_32_FLOAT, // float stored
	};

	// texture type information
	enum class ETextureType : uint8
	{
		TYPE_1D,
		TYPE_Array1D,
		TYPE_2D,
		TYPE_Array2D,
		TYPE_3D,
		TYPE_CUBE,
	};

	// get number of channels in the fetch format
	static uint32 GetFetchFormatChannels( const EFetchFormat format );

	// get name of the fetch format
	static const char* GetFetchFormatName( const EFetchFormat format );

	// information about vertex fetches
	struct FetchInfo
	{		
		EFetchFormat	m_format;		// type of the value
		uint8			m_channels;		// number of channels in data
		uint16			m_layoutSlot;	// slot in the vertex layout
		uint16			m_rawSlot;		// raw fetch slot
		uint16			m_rawOffset;	// raw fetch offset
		uint16			m_stride;		// data stride
		bool			m_signed;		// is data signed
		bool			m_normalized;	// is data normalized
		bool			m_float;		// is data floating point (if not it's fetched as int/uint)
	};

	// information about texture fetch
	struct TextureInfo
	{
		uint8			m_runtimeSlot;	// slot we are bounded to AT RUNTIME (sampler index in shader)
		uint32			m_fetchSlot;	// source fetch slot
		ETextureType	m_type;			// type of the texture (NOTE: it STILL may be an array)
	};

	// information about stuff outputted by shader
	struct OutputInfo
	{
		CodeChunk m_type;
		CodeChunk m_name;
		CodeChunk m_semantic;
		int		  m_index;

		inline OutputInfo( const char* type, const char* name, const char* semantic, const int index )
			: m_name( name )
			, m_type( type )
			, m_semantic( semantic )
			, m_index( index )
		{};
	};

	/// Get shader outputs
	void GetOutputs( std::vector< OutputInfo >& outputs ) const;

	/// Get vertex data fetches
	void GetVertexFetches( std::vector< FetchInfo >& fetches ) const;

	/// Get used registers
	void GetUsedRegisters( std::vector< uint32 >& outUsedRegs ) const;

	/// Get used textures
	void GetUsedTextures( std::vector< TextureInfo >& outTextures ) const;

	/// Get entry point address
	uint32 GetEntryPointAddress() const;

	/// Get number of used interpolators
	uint32 GetNumUsedInterpolators() const;

	/// Get mask for used texture fetch slots
	uint32 GetTextureFetchSlotMask() const;

	/// Dumping (debug)
	void DumpShader( const std::wstring& absoluteFilePath ) const;

	/// Emit to HLSL
	void EmitHLSL( class DX11Microcode::IHLSLWriter& writer ) const;

private:
	// is this a pixel shader ?
	bool						m_bIsPixelShader;

	// original microcode hash
	uint64						m_microcodeHash;

	// decompiled microcode
	DX11Microcode::Shader*		m_decompiledShader;
};
