#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>

#include "dx11MicrocodeShader.h"

/// Layout of data for input assembly
/// Extracted from microcode fetch instructions
/// NOTE: due to arbitrary vfetches this DOES NOT MAP to normal IA pipeline in DX11 
/// (cont) therefore all vertex data fetching is done through SV_VertexID and custom buffer with custom argument decode
class CDX11FetchLayout
{
public:
	// data format
	enum class EFormat : uint8
	{
		FMT_UNKNOWN,

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

	// fetch limits (related to Xenon itself)
	static const uint32 MAX_ELEMS = 16;
	static const uint32 MAX_STREAMS = 16;

	// single element in stream
	struct ElementDesc
	{
		EFormat		m_format;		// data format
		uint16		m_offset;		// offset from the stream start, in dwords
	};

	// stream description
	struct StreamDesc
	{
		uint16		m_slot;					// original fetch slot
		uint16		m_stride;				// stride (in dword, arbitraty)

		uint8		m_numElems;				// number of elements in stream
		ElementDesc m_elems[MAX_ELEMS];
	};

public:
	CDX11FetchLayout();
	~CDX11FetchLayout();

	// Get number of streams
	inline const uint32 GetNumStreams() const { return m_numStreams; }

	// Get n-th stream
	inline const StreamDesc& GetStream(const uint32 index) const { DEBUG_CHECK(index < m_numStreams); return m_streams[index]; }

	// Build fetch layout from vertex shader microcode
	static CDX11FetchLayout* ExtractFromMicrocode(CDX11MicrocodeShader* sourceMicrocode);

private:
	StreamDesc		m_streams[MAX_STREAMS];
	uint32			m_numStreams;

	// another format conversion :)
	static const EFormat ConvertFetchFormat(const CDX11MicrocodeShader::EFetchFormat shaderFetchFormat);
};
