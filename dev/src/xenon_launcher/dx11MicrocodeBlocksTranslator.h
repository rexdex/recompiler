#pragma once

#include "xenonGPUMicrocodeConstants.h"
#include "xenonGPUMicrocodeTransformer.h"

//----------------------------------------------------------------------------

namespace DX11Microcode
{
	class BlockTranslator : public CXenonGPUMicrocodeTransformer::ICodeWriter
	{
	public:
		BlockTranslator();

		// block extraction
		inline const uint32 GetNumCreatedBlocks() const { return (uint32) m_createdBlocks.size(); }
		inline Block* GetCreatedBlock( const uint32 index ) const { return m_createdBlocks[index]; }

	protected:
		virtual CodeExpr EmitReadReg( int regIndex ) override final;
		virtual CodeExpr EmitWriteReg( bool pixelShader, bool exported, int regIndex ) override final;
		virtual CodeExpr EmitBoolConst( bool pixelShader, int constIndex ) override final; // access boolean constant
		virtual CodeExpr EmitFloatConst( bool pixelShader, int regIndex ) override final; // access to float const table at given index
		virtual CodeExpr EmitFloatConstRel( bool pixelShader, int regOffset ) override final; // access to float const table at given index relative to index register (a0)
		virtual CodeExpr EmitGetPredicate() override final; // current predicate register
		virtual CodeExpr EmitAbs( CodeExpr code ) override final;
		virtual CodeExpr EmitNegate( CodeExpr code ) override final;
		virtual CodeExpr EmitNot( CodeExpr code ) override final;
		virtual CodeExpr EmitReadSwizzle( CodeExpr src, CXenonGPUMicrocodeTransformer::ESwizzle x, CXenonGPUMicrocodeTransformer::ESwizzle y, CXenonGPUMicrocodeTransformer::ESwizzle z, CXenonGPUMicrocodeTransformer::ESwizzle w ) override final;
		virtual CodeExpr EmitSaturate( CodeExpr dest ) override final;

		virtual CodeExpr EmitVertexFetch( CodeExpr src, int fetchSlot, int fetchOffset, uint32 stride, CXenonGPUMicrocodeTransformer::EFetchFormat format, bool isFloat, bool isSigned, bool isNormalized ) override final;

		virtual CodeExpr EmitTextureSample1D( CodeExpr src, int fetchSlot ) override final;
		virtual CodeExpr EmitTextureSample2D( CodeExpr src, int fetchSlot ) override final;
		virtual CodeExpr EmitTextureSample3D( CodeExpr src, int fetchSlot ) override final;
		virtual CodeExpr EmitTextureSampleCube( CodeExpr src, int fetchSlot ) override final;

		virtual CodeStatement EmitMergeStatements( CodeStatement prev, CodeStatement next ) override final; // builds the list of statements to execute
		virtual CodeStatement EmitConditionalStatement( CodeExpr condition, CodeStatement code ) override final; // conditional wrapper around statement
		virtual CodeStatement EmitWriteWithSwizzleStatement( CodeExpr dest, CodeExpr src, CXenonGPUMicrocodeTransformer::ESwizzle x, CXenonGPUMicrocodeTransformer::ESwizzle y, CXenonGPUMicrocodeTransformer::ESwizzle z, CXenonGPUMicrocodeTransformer::ESwizzle w ) override final; // writes specified expression to output with specified swizzles (the only general expression -> statement conversion)
		virtual CodeStatement EmitSetPredicateStatement( CodeExpr value ) override final;

		virtual CodeExpr EmitVectorInstruction1( CXenonGPUMicrocodeTransformer::EVectorInstr instr, CodeExpr a ) override final; // wrapper for function evaluation
		virtual CodeExpr EmitVectorInstruction2( CXenonGPUMicrocodeTransformer::EVectorInstr instr, CodeExpr a, CodeExpr b ) override final; // wrapper for function evaluation
		virtual CodeExpr EmitVectorInstruction3( CXenonGPUMicrocodeTransformer::EVectorInstr instr, CodeExpr a, CodeExpr b, CodeExpr c) override final; // wrapper for function evaluation
		virtual CodeExpr EmitScalarInstruction1( CXenonGPUMicrocodeTransformer::EScalarInstr instr, CodeExpr a ) override final; // wrapper for function evaluation
		virtual CodeExpr EmitScalarInstruction2( CXenonGPUMicrocodeTransformer::EScalarInstr instr, CodeExpr a, CodeExpr b ) override final;		 // wrapper for function evaluation

		virtual void EmitNop() override final;
		virtual void EmitExec( const uint32 codeAddr, CXenonGPUMicrocodeTransformer::EFlowBlock type, CodeStatement preamble, CodeStatement code, CodeExpr condition, const bool endOfShader ) override final;
		virtual void EmitJump( const uint32 targetAddr, CodeStatement preamble, CodeExpr condition ) override final;
		virtual void EmitCall( const uint32 targetAddr, CodeStatement preamble, CodeExpr condition ) override final;

		virtual void EmitExportAllocPosition() override final;
		virtual void EmitExportAllocParam( const uint32 size ) override final;
		virtual void EmitExportAllocMemExport( const uint32 size ) override final;

	private:
		std::vector< Block* >	m_createdBlocks;

		bool					m_positionExported;
		uint32					m_numParamExports;
		uint32					m_numMemoryExports;

		static const char* GetVectorFuncName1( const CXenonGPUMicrocodeTransformer::EVectorInstr instr );
		static const char* GetVectorFuncName2( const CXenonGPUMicrocodeTransformer::EVectorInstr instr );
		static const char* GetVectorFuncName3( const CXenonGPUMicrocodeTransformer::EVectorInstr instr );

		static const char* GetScalarFuncName1( const CXenonGPUMicrocodeTransformer::EScalarInstr instr );
		static const char* GetScalarFuncName2( const CXenonGPUMicrocodeTransformer::EScalarInstr instr );
	};
}
