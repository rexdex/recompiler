#pragma once

#include "xenonGPUConstants.h"
#include "xenonGPUUtils.h"

namespace ucode
{
	union instr_fetch_t;
	union instr_cf_t;
	struct instr_alu_t;
	struct instr_cf_exec_t;
	struct instr_fetch_tex_t;
	struct instr_fetch_vtx_t;
}

/// Shader transformer for Xbox360 shaders
class CXenonGPUMicrocodeTransformer
{
public:
	CXenonGPUMicrocodeTransformer( XenonShaderType shaderType );
	~CXenonGPUMicrocodeTransformer();

	enum class ESwizzle : uint8
	{
		// reads/writes:
		X, 
		Y, 
		Z, 
		W,

		// writes only:
		ZERO,			// 0 - component is forced to zero
		ONE,			// 1 - component is forced to one
		DONTCARE,		// don't care about this component
		NOTUSED,		// masked out (not modified by write operation)
	};

	enum class EVectorInstr : uint32
	{
		ADDv, // 2 args
		MULv, // 2 args
		MAXv, // 2 args
		MINv, // 2 args
		SETEv, // 2 args
		SETGTv, // 2 args
		SETGTEv, // 2 args
		SETNEv, // 2 args
		FRACv, // 1 arg
		TRUNCv, // 1 arg
		FLOORv, // 1 arg
		MULADDv, // 3 args
		CNDEv, // 3 args
		CNDGTEv, // 3 args
		CNDGTv, // 3 args
		DOT4v, // 2 args
		DOT3v, // 2 args
		DOT2ADDv, // 3 args
		CUBEv, // 2 args
		MAX4v, // 1 arg
		PRED_SETE_PUSHv, // 2 args
		PRED_SETNE_PUSHv, // 2 args
		PRED_SETGT_PUSHv, // 2 args
		PRED_SETGTE_PUSHv, // 2 args
		KILLEv, // 2 args
		KILLGTv, // 2 args
		KILLGTEv, // 2 args
		KILLNEv, // 2 args
		DSTv, // 2 args
		MOVAv, // 1 arg
	};

	enum class EScalarInstr : uint32
	{
		ADDs, // 1 arg
		ADD_PREVs, // 1 arg
		MULs, // 1 arg
		MUL_PREVs, // 1 arg
		MUL_PREV2s, // 1 arg
		MAXs, // 1 arg
		MINs, // 1 arg
		SETEs, // 1 arg
		SETGTs, // 1 arg
		SETGTEs, // 1 arg
		SETNEs, // 1 arg
		FRACs, // 1 arg
		TRUNCs, // 1 arg
		FLOORs, // 1 arg
		EXP_IEEE, // 1 arg
		LOG_CLAMP, // 1 arg
		LOG_IEEE, // 1 arg
		RECIP_CLAMP, // 1 arg
		RECIP_FF, // 1 arg
		RECIP_IEEE, // 1 arg
		RECIPSQ_CLAMP, // 1 arg
		RECIPSQ_FF, // 1 arg
		RECIPSQ_IEEE, // 1 arg
		MOVAs, // 1 arg
		MOVA_FLOORs, // 1 arg
		SUBs, // 1 arg
		SUB_PREVs, // 1 arg
		PRED_SETEs, // 1 arg
		PRED_SETNEs, // 1 arg
		PRED_SETGTs, // 1 arg
		PRED_SETGTEs, // 1 arg
		PRED_SET_INVs, // 1 arg
		PRED_SET_POPs, // 1 arg
		PRED_SET_CLRs, // 1 arg
		PRED_SET_RESTOREs, // 1 arg
		KILLEs, // 1 arg
		KILLGTs, // 1 arg
		KILLGTEs, // 1 arg
		KILLNEs, // 1 arg
		KILLONEs, // 1 arg
		SQRT_IEEE, // 1 arg
		UNKNOWN,
		MUL_CONST_0, // 2 args
		MUL_CONST_1, // 2 args
		ADD_CONST_0, // 2 args
		ADD_CONST_1, // 2 args
		SUB_CONST_0, // 2 args
		SUB_CONST_1, // 2 args
		SIN, // 1 arg
		COS, // 1 arg
		RETAIN_PREV, // 1 arg
	};

	enum class EFlowBlock : uint32
	{
		NOP,
		EXEC,
		EXEC_END,
		COND_EXEC,
		COND_EXEC_END,
		COND_PRED_EXEC,
		COND_PRED_EXEC_END,
		LOOP_START,
		LOOP_END,
		COND_CALL,
		RETURN,
		COND_JMP,
		ALLOC,
		COND_EXEC_PRED_CLEAN,
		COND_EXEC_PRED_CLEAN_END,
		MARK_VS_FETCH_DONE
	};

	enum class EFetchFormat : uint32
	{
		FMT_1_REVERSE = 0,
		FMT_8 = 2,
		FMT_8_8_8_8 = 6,
		FMT_2_10_10_10 = 7,
		FMT_8_8 = 10,
		FMT_16 = 24,
		FMT_16_16 = 25,
		FMT_16_16_16_16 = 26,
		FMT_32 = 33,
		FMT_32_32 = 34,
		FMT_32_32_32_32 = 35,
		FMT_32_FLOAT = 36,
		FMT_32_32_FLOAT = 37,
		FMT_32_32_32_32_FLOAT = 38,	
		FMT_32_32_32_FLOAT = 57,
	};

	class ICodeWriter
	{
	public:
		virtual ~ICodeWriter() {};

		// building blocks
		virtual CodeExpr EmitReadReg( int regIndex ) = 0;
		virtual CodeExpr EmitWriteReg( bool pixelShader, bool exported, int regIndex ) = 0;
		virtual CodeExpr EmitBoolConst( bool pixelShader, int constIndex ) = 0; // access boolean constant
		virtual CodeExpr EmitFloatConst( bool pixelShader, int regIndex ) = 0; // access to float const table at given index
		virtual CodeExpr EmitFloatConstRel( bool pixelShader, int regOffset ) = 0; // access to float const table at given index relative to index register (a0)
		virtual CodeExpr EmitGetPredicate() = 0; // current predicate register
		virtual CodeExpr EmitAbs( CodeExpr code ) = 0;
		virtual CodeExpr EmitNegate( CodeExpr code ) = 0;
		virtual CodeExpr EmitNot( CodeExpr code ) = 0;
		virtual CodeExpr EmitReadSwizzle( CodeExpr src, ESwizzle x, ESwizzle y, ESwizzle z, ESwizzle w ) = 0;
		virtual CodeExpr EmitSaturate( CodeExpr dest ) = 0;

		// vertex data fetch
		virtual CodeExpr EmitVertexFetch( CodeExpr src, int fetchSlot, int fetchOffset, uint32 stride, EFetchFormat format, bool isFloat, bool isSigned, bool isNormalized ) = 0;

		// texture sample
		virtual CodeExpr EmitTextureSample1D( CodeExpr src, int fetchSlot ) = 0;
		virtual CodeExpr EmitTextureSample2D( CodeExpr src, int fetchSlot ) = 0;
		virtual CodeExpr EmitTextureSample3D( CodeExpr src, int fetchSlot ) = 0;
		virtual CodeExpr EmitTextureSampleCube( CodeExpr src, int fetchSlot ) = 0;

		// statements
		virtual CodeStatement EmitMergeStatements( CodeStatement prev, CodeStatement next ) = 0; // builds the list of statements to execute
		virtual CodeStatement EmitConditionalStatement( CodeExpr condition, CodeStatement code ) = 0; // conditional wrapper around statement
		virtual CodeStatement EmitWriteWithSwizzleStatement( CodeExpr dest, CodeExpr src, ESwizzle x, ESwizzle y, ESwizzle z, ESwizzle w ) = 0; // writes specified expression to output with specified swizzles (the only general expression -> statement conversion)
		virtual CodeStatement EmitSetPredicateStatement( CodeExpr value ) = 0; // sets new value for predicate

		// instructions
		virtual CodeExpr EmitVectorInstruction1( EVectorInstr instr, CodeExpr a ) = 0; // wrapper for function evaluation
		virtual CodeExpr EmitVectorInstruction2( EVectorInstr instr, CodeExpr a, CodeExpr b ) = 0; // wrapper for function evaluation
		virtual CodeExpr EmitVectorInstruction3( EVectorInstr instr, CodeExpr a, CodeExpr b, CodeExpr c) = 0; // wrapper for function evaluation
		virtual CodeExpr EmitScalarInstruction1( EScalarInstr instr, CodeExpr a ) = 0; // wrapper for function evaluation
		virtual CodeExpr EmitScalarInstruction2( EScalarInstr instr, CodeExpr a, CodeExpr b ) = 0;		 // wrapper for function evaluation

		// control flow blocks (top level), NOTE: blocks are implicitly remembered inside
		virtual void EmitNop() = 0;
		virtual void EmitExec( const uint32 codeAddr, EFlowBlock type, CodeStatement preamble, CodeStatement code, CodeExpr condition, const bool endOfShader ) = 0;
		virtual void EmitJump( const uint32 targetAddr, CodeStatement preamble, CodeExpr condition ) = 0;
		virtual void EmitCall( const uint32 targetAddr, CodeStatement preamble, CodeExpr condition ) = 0;

		// exports
		virtual void EmitExportAllocPosition() = 0;
		virtual void EmitExportAllocParam( const uint32 size ) = 0;
		virtual void EmitExportAllocMemExport( const uint32 size ) = 0;
	};

	/// Decompile shader
	void TransformShader( ICodeWriter& writer, const uint32* words, const uint32 numWords );

private:
	XenonShaderType			m_shaderType;
	uint32					m_lastVertexStride;

	static const uint32 GetArgCount( EVectorInstr instr );
	static const uint32 GetArgCount( EScalarInstr instr );

	void TransformBlock( ICodeWriter& writer, const uint32* words, const uint32 numWords, const ucode::instr_cf_t* cf, uint32& codePC );

	CodeStatement EmitExec( ICodeWriter& writer, const uint32* words, const uint32 numWords, const ucode::instr_cf_exec_t& exec );
	CodeStatement EmitALU( ICodeWriter& writer, const ucode::instr_alu_t& alu, const bool bSync );
	CodeStatement EmitVertexFetch( ICodeWriter& writer, const ucode::instr_fetch_vtx_t& alu, const bool bSync );
	CodeStatement EmitTextureFetch( ICodeWriter& writer, const ucode::instr_fetch_tex_t& alu, const bool bSync );
	CodeStatement EmitPredicateTest( ICodeWriter& writer, const CodeStatement& code, const bool bIsConditional, const uint32 flowPredCondition, const uint32 predSelect, const uint32 predCondition );

	CodeExpr EmitSrcReg( ICodeWriter& writer, const ucode::instr_alu_t& op, const uint32 num, const uint32 type, const uint32 swiz, const uint32 negate, const int constSlot );
	CodeExpr EmitSrcReg( ICodeWriter& writer, const ucode::instr_alu_t& op, const uint32 argIndex );
	CodeExpr EmitSrcScalarReg1( ICodeWriter& writer, const ucode::instr_alu_t& op );
	CodeExpr EmitSrcScalarReg2( ICodeWriter& writer, const ucode::instr_alu_t& op );
	CodeStatement EmitVectorResult( ICodeWriter& writer, const ucode::instr_alu_t& op, const CodeExpr& code );
	CodeStatement EmitScalarResult( ICodeWriter& writer, const ucode::instr_alu_t& op, const CodeExpr& code );
};
