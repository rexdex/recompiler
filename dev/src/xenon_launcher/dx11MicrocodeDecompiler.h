#pragma once

#include "xenonGPUConstants.h"

namespace ucode
{
	union instr_fetch_t;
	union instr_cf_t;
}

/// Debug (print only) decompiler for Xbox360 shaders
class CDX11MicrocodeDecompiler
{
public:
	CDX11MicrocodeDecompiler( XenonShaderType shaderType );
	~CDX11MicrocodeDecompiler();

	class ICodeWriter
	{
	public:
		virtual ~ICodeWriter() {};

		virtual void Append( const char* txt ) = 0;
		virtual void Appendf( const char* txt, ... ) = 0;
	};

	/// Decompile shader
	void DisassembleShader( ICodeWriter& writer, const uint32* words, const uint32 numWords );

private:
	struct VectorInstr
	{
		uint32			m_numSources;
		const char*		m_name;
	};

	struct ScalarInstr
	{
		uint32			m_numSources;
		const char*		m_name;
	};

	struct FetchType
	{
		const char*		m_name;
	};

	struct FetchInstr
	{
		const char*		m_name;
		int32			m_type;
	};

	struct FlowInstr
	{
		const char*		m_name;
		int32			m_type;
	};

	static const char*	st_tabLevels[]; // tab levels
	static const char	st_chanNames[]; // channel names
	static VectorInstr	st_vectorInstructions[0x20]; // vector instruction names
	static ScalarInstr	st_scalarInstructions[0x40]; // scalar instruction names
	static FetchType	st_fetchTypes[0xFF]; // data types for fetch instruction
	static FetchInstr	st_fetchInstr[28]; // fetch instruction
	static FlowInstr	st_flowInstr[]; // control flow instructions

	XenonShaderType			m_shaderType;

	void PrintSrcReg( ICodeWriter& writer, const uint32 num, const uint32 type, const uint32 swiz, const uint32 negate, const uint32 abs_constants );
	void PrintDestReg( ICodeWriter& writer, const uint32 num, const uint32 mask, const uint32 destExp );
	void PrintExportComment( ICodeWriter& writer, const uint32 num );
	void PrintFetchDest( ICodeWriter& writer, const uint32 dstReg, const uint32 dstSwiz );
	void PrintFetchVtx( ICodeWriter& writer, const ucode::instr_fetch_t* fetch );
	void PrintFetchTex( ICodeWriter& writer, const ucode::instr_fetch_t* fetch );

	void PrintFlowNop( ICodeWriter& writer, const ucode::instr_cf_t* cf );
	void PrintFlowExec( ICodeWriter& writer, const ucode::instr_cf_t* cf );
	void PrintFlowLoop( ICodeWriter& writer, const ucode::instr_cf_t* cf );
	void PrintFlowJmpCall( ICodeWriter& writer, const ucode::instr_cf_t* cf );
	void PrintFlowAlloc( ICodeWriter& writer, const ucode::instr_cf_t* cf );
	void PrintFlow( ICodeWriter& writer, const ucode::instr_cf_t* cf, const uint32 level );

	void DisasembleALU( ICodeWriter& writer, const uint32* words, const uint32 aluOff, const uint32 level, const uint32 sync );
	void DisasembleFETCH( ICodeWriter& writer, const uint32* words, const uint32 aluOff, const uint32 level, const uint32 sync );
	void DisasembleEXEC( ICodeWriter& writer, const uint32* words, const uint32 numWords, const uint32 level, const ucode::instr_cf_t* cf );
};

