#include "build.h"
#include "dx11MicrocodeNodes.h"
#include "dx11MicrocodeBlocks.h"
#include "dx11MicrocodeBlocksTranslator.h"

//#pragma optimize( "", off )

using namespace DX11Microcode;

BlockTranslator::BlockTranslator()
{
	m_numParamExports = 0;
	m_numMemoryExports = 0;
	m_positionExported = 0;
}

CodeExpr BlockTranslator::EmitReadReg(int regIndex)
{
	return new ExprReadReg(regIndex);
}

void BlockTranslator::EmitExportAllocPosition()
{
	DEBUG_CHECK(!m_positionExported);
	m_positionExported = true;
}

void BlockTranslator::EmitExportAllocParam(const uint32 size)
{
	m_numParamExports += size;
}

void BlockTranslator::EmitExportAllocMemExport(const uint32 size)
{
	m_numMemoryExports += size;
}

CodeExpr BlockTranslator::EmitWriteReg(bool pixelShader, bool exported, int regIndex)
{
	if (exported)
	{
		// special cases
		if (pixelShader)
		{
			if (regIndex == 0)
			{
				return new ExprWriteExportReg(ExprWriteExportReg::EExportReg::COLOR0);
			}
			else if (regIndex == 1)
			{
				return new ExprWriteExportReg(ExprWriteExportReg::EExportReg::COLOR1);
			}
			else if (regIndex == 2)
			{
				return new ExprWriteExportReg(ExprWriteExportReg::EExportReg::COLOR2);
			}
			else if (regIndex == 3)
			{
				return new ExprWriteExportReg(ExprWriteExportReg::EExportReg::COLOR3);
			}
		}
		else
		{
			// special cases for vertex shader
			if (regIndex == 62)
			{
				return new ExprWriteExportReg(ExprWriteExportReg::EExportReg::POSITION);
			}
			else if (regIndex == 62)
			{
				return new ExprWriteExportReg(ExprWriteExportReg::EExportReg::POINTSIZE);
			}

			// exported ?
			if (regIndex >= (int)m_numParamExports)
			{
				GLog.Warn("Microcode: Vertex shader register %d used for export but not allocated", regIndex);
				m_numParamExports = regIndex + 1;
			}

			// export
			switch (regIndex)
			{
				case 0: return new ExprWriteExportReg(ExprWriteExportReg::EExportReg::INTERP0);
				case 1: return new ExprWriteExportReg(ExprWriteExportReg::EExportReg::INTERP1);
				case 2: return new ExprWriteExportReg(ExprWriteExportReg::EExportReg::INTERP2);
				case 3: return new ExprWriteExportReg(ExprWriteExportReg::EExportReg::INTERP3);
				case 4: return new ExprWriteExportReg(ExprWriteExportReg::EExportReg::INTERP4);
				case 5: return new ExprWriteExportReg(ExprWriteExportReg::EExportReg::INTERP5);
				case 6: return new ExprWriteExportReg(ExprWriteExportReg::EExportReg::INTERP6);
				case 7: return new ExprWriteExportReg(ExprWriteExportReg::EExportReg::INTERP7);

				default:
				{
					GLog.Err("Microcode: Vertex shader register %d is out of usable export range", regIndex);
				}
			}
		}
	}

	// just a normal register
	return new ExprWriteReg(regIndex);
}

CodeExpr BlockTranslator::EmitBoolConst(bool pixelShader, int constIndex)
{
	return new ExprBoolConst(pixelShader, constIndex);
}

CodeExpr BlockTranslator::EmitFloatConst(bool pixelShader, int regIndex)
{
	return new ExprFloatConst(pixelShader, regIndex);
}

CodeExpr BlockTranslator::EmitFloatConstRel(bool pixelShader, int regOffset)
{
	return new ExprFloatRelativeConst(pixelShader, regOffset);
}

CodeExpr BlockTranslator::EmitGetPredicate()
{
	return new ExprGetPredicate();
}

CodeExpr BlockTranslator::EmitAbs(CodeExpr code)
{
	if (code)
		return new ExprAbs(code.Get<ExprNode>());

	return nullptr;
}

CodeExpr BlockTranslator::EmitNegate(CodeExpr code)
{
	if (code)
		return new ExprNegate(code.Get<ExprNode>());

	return nullptr;
}

CodeExpr BlockTranslator::EmitNot(CodeExpr code)
{
	if (code)
		return new ExprNot(code.Get<ExprNode>());

	return nullptr;
}

CodeExpr BlockTranslator::EmitReadSwizzle(CodeExpr src, CXenonGPUMicrocodeTransformer::ESwizzle x, CXenonGPUMicrocodeTransformer::ESwizzle y, CXenonGPUMicrocodeTransformer::ESwizzle z, CXenonGPUMicrocodeTransformer::ESwizzle w)
{
	if (src)
	{
		// TODO: swizzle constant translation ?
		return new ExprReadSwizzle(src.Get<ExprNode>(), x, y, z, w);
	}

	return nullptr;
}

CodeExpr BlockTranslator::EmitSaturate(CodeExpr dest)
{
	if (dest)
		return new ExprSaturate(dest.Get<ExprNode>());

	return nullptr;
}

CodeExpr BlockTranslator::EmitVertexFetch(CodeExpr src, int fetchSlot, int fetchOffset, uint32 stride, CXenonGPUMicrocodeTransformer::EFetchFormat format, bool isFloat, bool isSigned, bool isNormalized)
{
	DEBUG_CHECK(stride != 0);
	return new ExprVertexFetch(src.Get<ExprNode>(), fetchSlot, fetchOffset, stride, format, isFloat, isSigned, isNormalized);
}

CodeExpr BlockTranslator::EmitTextureSample1D(CodeExpr src, int fetchSlot)
{
	const auto type = ExprTextureFetch::TextureType::Type1D;
	return new ExprTextureFetch(src.Get<ExprNode>(), fetchSlot, type);
}

CodeExpr BlockTranslator::EmitTextureSample2D(CodeExpr src, int fetchSlot)
{
	const auto type = ExprTextureFetch::TextureType::Type2D;
	return new ExprTextureFetch(src.Get<ExprNode>(), fetchSlot, type);
}

CodeExpr BlockTranslator::EmitTextureSample3D(CodeExpr src, int fetchSlot)
{
	const auto type = ExprTextureFetch::TextureType::Type2D;
	return new ExprTextureFetch(src.Get<ExprNode>(), fetchSlot, type);
}

CodeExpr BlockTranslator::EmitTextureSampleCube(CodeExpr src, int fetchSlot)
{
	return new ExprTextureFetch(src.Get<ExprNode>(), fetchSlot, ExprTextureFetch::TextureType::TypeCube);
}

CodeStatement BlockTranslator::EmitMergeStatements(CodeStatement prev, CodeStatement next)
{
	if (!prev)
		return next;

	if (!next)
		return prev;

	return new ListStatement(prev.Get<Statement>(), next.Get<Statement>());
}

CodeStatement BlockTranslator::EmitConditionalStatement(CodeExpr condition, CodeStatement code)
{
	if (!condition)
		return code;

	if (!code)
		return nullptr;

	return new ConditionalStatement(code.Get<Statement>(), condition.Get<ExprNode>());
}

CodeStatement BlockTranslator::EmitWriteWithSwizzleStatement(CodeExpr dest, CodeExpr src, CXenonGPUMicrocodeTransformer::ESwizzle x, CXenonGPUMicrocodeTransformer::ESwizzle y, CXenonGPUMicrocodeTransformer::ESwizzle z, CXenonGPUMicrocodeTransformer::ESwizzle w)
{
	if (!dest)
		return nullptr;

	if (!src)
		return nullptr;

	// TODO: translate write mask ?
	return new WriteWithMaskStatement(dest.Get<ExprNode>(), src.Get<ExprNode>(), x, y, z, w);
}

CodeStatement BlockTranslator::EmitSetPredicateStatement(CodeExpr value)
{
	if (!value)
		return nullptr;

	return new SetPredicateStatement(value.Get<ExprNode>());
}

CodeExpr BlockTranslator::EmitVectorInstruction1(CXenonGPUMicrocodeTransformer::EVectorInstr instr, CodeExpr a)
{
	const char* name = GetVectorFuncName1(instr);
	DEBUG_CHECK(name != nullptr);

	if (!name)
		return nullptr;

	if (!a)
		return nullptr;

	return new ExprVectorFunc1(name, a.Get<ExprNode>());
}

CodeExpr BlockTranslator::EmitVectorInstruction2(CXenonGPUMicrocodeTransformer::EVectorInstr instr, CodeExpr a, CodeExpr b)
{
	const char* name = GetVectorFuncName2(instr);
	DEBUG_CHECK(name != nullptr);

	if (!name)
		return nullptr;

	if (!a)
		return nullptr;

	if (!b)
		return nullptr;

	return new ExprVectorFunc2(name, a.Get<ExprNode>(), b.Get<ExprNode>());
}

CodeExpr BlockTranslator::EmitVectorInstruction3(CXenonGPUMicrocodeTransformer::EVectorInstr instr, CodeExpr a, CodeExpr b, CodeExpr c)
{
	const char* name = GetVectorFuncName3(instr);
	DEBUG_CHECK(name != nullptr);

	if (!name)
		return nullptr;

	if (!a)
		return nullptr;

	if (!b)
		return nullptr;

	if (!c)
		return nullptr;

	return new ExprVectorFunc3(name, a.Get<ExprNode>(), b.Get<ExprNode>(), c.Get<ExprNode>());
}

CodeExpr BlockTranslator::EmitScalarInstruction1(CXenonGPUMicrocodeTransformer::EScalarInstr instr, CodeExpr a)
{
	const char* name = GetScalarFuncName1(instr);
	DEBUG_CHECK(name != nullptr);

	if (!name)
		return nullptr;

	if (!a)
		return nullptr;

	return new ExprScalarFunc1(name, a.Get<ExprNode>());
}

CodeExpr BlockTranslator::EmitScalarInstruction2(CXenonGPUMicrocodeTransformer::EScalarInstr instr, CodeExpr a, CodeExpr b)
{
	const char* name = GetScalarFuncName2(instr);
	DEBUG_CHECK(name != nullptr);

	if (!name)
		return nullptr;

	if (!a)
		return nullptr;

	if (!b)
		return nullptr;

	return new ExprScalarFunc2(name, a.Get<ExprNode>(), b.Get<ExprNode>());
}

void BlockTranslator::EmitNop()
{
	// NOP blocks are NOT emitted - there's not reason for it
}

void BlockTranslator::EmitExec(const uint32 codeAddr, CXenonGPUMicrocodeTransformer::EFlowBlock type, CodeStatement preamble, CodeStatement code, CodeExpr condition, const bool endOfShader)
{
	// no code to execute
	if (!code)
		return;

	// exec block
	Block* block = new Block(codeAddr, preamble.Get<Statement>(), code.Get<Statement>(), condition.Get<ExprNode>());
	m_createdBlocks.push_back(block);

	// end block
	if (endOfShader)
	{
		m_createdBlocks.push_back(new Block(nullptr, 0, Block::EBlockType::END));
	}
}

void BlockTranslator::EmitJump(const uint32 targetAddr, CodeStatement preamble, CodeExpr condition)
{
	Block* block = new Block(condition.Get<ExprNode>(), targetAddr, Block::EBlockType::JUMP);
	m_createdBlocks.push_back(block);
}

void BlockTranslator::EmitCall(const uint32 targetAddr, CodeStatement preamble, CodeExpr condition)
{
	Block* block = new Block(condition.Get<ExprNode>(), targetAddr, Block::EBlockType::CALL);
	m_createdBlocks.push_back(block);
}

const char* BlockTranslator::GetVectorFuncName1(const CXenonGPUMicrocodeTransformer::EVectorInstr instr)
{
	switch (instr)
	{
		case CXenonGPUMicrocodeTransformer::EVectorInstr::FRACv: return "FRACv";
		case CXenonGPUMicrocodeTransformer::EVectorInstr::TRUNCv: return "TRUNCv";
		case CXenonGPUMicrocodeTransformer::EVectorInstr::FLOORv: return "FLOORv";
		case CXenonGPUMicrocodeTransformer::EVectorInstr::MAX4v: return "MAX4v";
		case CXenonGPUMicrocodeTransformer::EVectorInstr::MOVAv: return "MOVAv";
	}

	DEBUG_CHECK(!"Invalid function name");
	return nullptr;
}

const char* BlockTranslator::GetVectorFuncName2(const CXenonGPUMicrocodeTransformer::EVectorInstr instr)
{
	switch (instr)
	{
		case CXenonGPUMicrocodeTransformer::EVectorInstr::ADDv: return "ADDv";
		case CXenonGPUMicrocodeTransformer::EVectorInstr::MULv: return "MULv";
		case CXenonGPUMicrocodeTransformer::EVectorInstr::MAXv: return "MAXv";
		case CXenonGPUMicrocodeTransformer::EVectorInstr::MINv: return "MINv";
		case CXenonGPUMicrocodeTransformer::EVectorInstr::SETEv: return "SETEv";
		case CXenonGPUMicrocodeTransformer::EVectorInstr::SETGTv: return "SETGTv";
		case CXenonGPUMicrocodeTransformer::EVectorInstr::SETGTEv: return "SETGTEv";
		case CXenonGPUMicrocodeTransformer::EVectorInstr::SETNEv: return "SETNEv";
		case CXenonGPUMicrocodeTransformer::EVectorInstr::DOT4v: return "DOT4v";
		case CXenonGPUMicrocodeTransformer::EVectorInstr::DOT3v: return "DOT3v";
		case CXenonGPUMicrocodeTransformer::EVectorInstr::CUBEv: return "CUBEv";
		case CXenonGPUMicrocodeTransformer::EVectorInstr::PRED_SETE_PUSHv: return "PRED_SETE_PUSHv";
		case CXenonGPUMicrocodeTransformer::EVectorInstr::PRED_SETNE_PUSHv: return "PRED_SETNE_PUSHv";
		case CXenonGPUMicrocodeTransformer::EVectorInstr::PRED_SETGT_PUSHv: return "PRED_SETGT_PUSHv";
		case CXenonGPUMicrocodeTransformer::EVectorInstr::PRED_SETGTE_PUSHv: return "PRED_SETGTE_PUSHv";
		case CXenonGPUMicrocodeTransformer::EVectorInstr::KILLEv: return "KILLEv";
		case CXenonGPUMicrocodeTransformer::EVectorInstr::KILLGTv: return "KILLGTv";
		case CXenonGPUMicrocodeTransformer::EVectorInstr::KILLGTEv: return "KILLGTEv";
		case CXenonGPUMicrocodeTransformer::EVectorInstr::KILLNEv: return "KILLNEv";
		case CXenonGPUMicrocodeTransformer::EVectorInstr::DSTv: return "DSTv";
	}

	DEBUG_CHECK(!"Invalid function name");
	return nullptr;
}

const char* BlockTranslator::GetVectorFuncName3(const CXenonGPUMicrocodeTransformer::EVectorInstr instr)
{
	switch (instr)
	{
		case CXenonGPUMicrocodeTransformer::EVectorInstr::MULADDv: return "MULLADDv";
		case CXenonGPUMicrocodeTransformer::EVectorInstr::CNDEv: return "CNDEv";
		case CXenonGPUMicrocodeTransformer::EVectorInstr::CNDGTEv: return "CNDGTEv";
		case CXenonGPUMicrocodeTransformer::EVectorInstr::CNDGTv: return "CNDGTv";
		case CXenonGPUMicrocodeTransformer::EVectorInstr::DOT2ADDv: return "DOT2ADDv";
	}

	DEBUG_CHECK(!"Invalid function name");
	return nullptr;
}

const char* BlockTranslator::GetScalarFuncName1(const CXenonGPUMicrocodeTransformer::EScalarInstr instr)
{
	switch (instr)
	{
		case CXenonGPUMicrocodeTransformer::EScalarInstr::ADDs: return "ADDs";
		case CXenonGPUMicrocodeTransformer::EScalarInstr::ADD_PREVs: return "ADD_PREVs";
		case CXenonGPUMicrocodeTransformer::EScalarInstr::MULs: return "MULs";
		case CXenonGPUMicrocodeTransformer::EScalarInstr::MUL_PREVs: return "MUL_PREVs";
		case CXenonGPUMicrocodeTransformer::EScalarInstr::MUL_PREV2s: return "MUL_PREV2s";
		case CXenonGPUMicrocodeTransformer::EScalarInstr::MAXs: return "MAXs";
		case CXenonGPUMicrocodeTransformer::EScalarInstr::MINs: return "MINs";
		case CXenonGPUMicrocodeTransformer::EScalarInstr::SETEs: return "SETEs";
		case CXenonGPUMicrocodeTransformer::EScalarInstr::SETGTs: return "SETGTs";
		case CXenonGPUMicrocodeTransformer::EScalarInstr::SETGTEs: return "SETGTEs";
		case CXenonGPUMicrocodeTransformer::EScalarInstr::SETNEs: return "SETNEs";
		case CXenonGPUMicrocodeTransformer::EScalarInstr::FRACs: return "FRACs";
		case CXenonGPUMicrocodeTransformer::EScalarInstr::TRUNCs: return "TRUNCs";
		case CXenonGPUMicrocodeTransformer::EScalarInstr::FLOORs: return "FLOORs";
		case CXenonGPUMicrocodeTransformer::EScalarInstr::EXP_IEEE: return "EXP_IEEE";
		case CXenonGPUMicrocodeTransformer::EScalarInstr::LOG_CLAMP: return "LOG_CLAMP";
		case CXenonGPUMicrocodeTransformer::EScalarInstr::LOG_IEEE: return "LOG_IEEE";
		case CXenonGPUMicrocodeTransformer::EScalarInstr::RECIP_CLAMP: return "RECIP_CLAMP";
		case CXenonGPUMicrocodeTransformer::EScalarInstr::RECIP_FF: return "RECIP_FF";
		case CXenonGPUMicrocodeTransformer::EScalarInstr::RECIP_IEEE: return "RECIP_IEEE";
		case CXenonGPUMicrocodeTransformer::EScalarInstr::RECIPSQ_CLAMP: return "RECIPSQ_CLAMP";
		case CXenonGPUMicrocodeTransformer::EScalarInstr::RECIPSQ_FF: return "RECIPSQ_FF";
		case CXenonGPUMicrocodeTransformer::EScalarInstr::RECIPSQ_IEEE: return "RECIPSQ_IEEE";
		case CXenonGPUMicrocodeTransformer::EScalarInstr::MOVAs: return "MOVAs";
		case CXenonGPUMicrocodeTransformer::EScalarInstr::MOVA_FLOORs: return "MOVA_FLOORs";
		case CXenonGPUMicrocodeTransformer::EScalarInstr::SUBs: return "SUBs";
		case CXenonGPUMicrocodeTransformer::EScalarInstr::SUB_PREVs: return "SUB_PREVs";
		case CXenonGPUMicrocodeTransformer::EScalarInstr::PRED_SETEs: return "PRED_SETEs";
		case CXenonGPUMicrocodeTransformer::EScalarInstr::PRED_SETNEs: return "PRED_SETNEs";
		case CXenonGPUMicrocodeTransformer::EScalarInstr::PRED_SETGTs: return "PRED_SETGTs";
		case CXenonGPUMicrocodeTransformer::EScalarInstr::PRED_SETGTEs: return "PRED_SETGTEs";
		case CXenonGPUMicrocodeTransformer::EScalarInstr::PRED_SET_INVs: return "PRED_SET_INVs";
		case CXenonGPUMicrocodeTransformer::EScalarInstr::PRED_SET_POPs: return "PRED_SET_POPs";
		case CXenonGPUMicrocodeTransformer::EScalarInstr::PRED_SET_CLRs: return "PRED_SET_CLRs";
		case CXenonGPUMicrocodeTransformer::EScalarInstr::PRED_SET_RESTOREs: return "PRED_SET_RESTOREs";
		case CXenonGPUMicrocodeTransformer::EScalarInstr::KILLEs: return "KILLEs";
		case CXenonGPUMicrocodeTransformer::EScalarInstr::KILLGTs: return "KILLGTs";
		case CXenonGPUMicrocodeTransformer::EScalarInstr::KILLGTEs: return "KILLGTEs";
		case CXenonGPUMicrocodeTransformer::EScalarInstr::KILLNEs: return "KILLNEs";
		case CXenonGPUMicrocodeTransformer::EScalarInstr::KILLONEs: return "KILLONEs";
		case CXenonGPUMicrocodeTransformer::EScalarInstr::SQRT_IEEE: return "SQRT_IEEE";
		case CXenonGPUMicrocodeTransformer::EScalarInstr::SIN: return "SIN";
		case CXenonGPUMicrocodeTransformer::EScalarInstr::COS: return "COS";
		case CXenonGPUMicrocodeTransformer::EScalarInstr::RETAIN_PREV: return "RETAIN_PREV";
	}

	DEBUG_CHECK(!"Invalid function name");
	return nullptr;
}

const char* BlockTranslator::GetScalarFuncName2(const CXenonGPUMicrocodeTransformer::EScalarInstr instr)
{
	switch (instr)
	{
		case CXenonGPUMicrocodeTransformer::EScalarInstr::MUL_CONST_0: return "MUL_CONST_0";
		case CXenonGPUMicrocodeTransformer::EScalarInstr::MUL_CONST_1: return "MUL_CONST_1";
		case CXenonGPUMicrocodeTransformer::EScalarInstr::ADD_CONST_0: return "ADD_CONST_0";
		case CXenonGPUMicrocodeTransformer::EScalarInstr::ADD_CONST_1: return "ADD_CONST_1";
		case CXenonGPUMicrocodeTransformer::EScalarInstr::SUB_CONST_0: return "SUB_CONST_0";
		case CXenonGPUMicrocodeTransformer::EScalarInstr::SUB_CONST_1: return "SUB_CONST_1";
	}

	DEBUG_CHECK(!"Invalid function name");
	return nullptr;
}
