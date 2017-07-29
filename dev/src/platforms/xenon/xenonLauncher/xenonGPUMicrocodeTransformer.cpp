#include "build.h"
#include "xenonGPUMicrocodeTransformer.h"
#include "xenonGPUMicrocodeConstants.h"

//#pragma optimize ("",off)

using namespace ucode;

// BASED ON THE FOLLOWING:

/*
 * Copyright (c) 2012 Rob Clark <robdclark@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

CXenonGPUMicrocodeTransformer::CXenonGPUMicrocodeTransformer( XenonShaderType shaderType )
	: m_shaderType( shaderType )
	, m_lastVertexStride( 0 )
{
}

CXenonGPUMicrocodeTransformer::~CXenonGPUMicrocodeTransformer()
{
}

void CXenonGPUMicrocodeTransformer::TransformShader( ICodeWriter& writer, const uint32* words, const uint32 numWords )
{
	instr_cf_t cfa;
	instr_cf_t cfb;

	uint32 codePC = 0;
	for ( uint32 idx = 0; idx < numWords; idx += 3)
	{
		const uint32 dword_0 = words[idx + 0];
		const uint32 dword_1 = words[idx + 1];
		const uint32 dword_2 = words[idx + 2];

		cfa.dword_0 = dword_0;
		cfa.dword_1 = dword_1 & 0xFFFF;
		cfb.dword_0 = (dword_1 >> 16) | (dword_2 << 16);
		cfb.dword_1 = dword_2 >> 16;

		// there are two CF blocks in 3 dwords
		TransformBlock( writer, words, numWords, &cfa, codePC );
		TransformBlock( writer, words, numWords, &cfb, codePC );

		if (cfa.opc == EXEC_END || cfb.opc == EXEC_END)
			break;
	}
}

CodeStatement CXenonGPUMicrocodeTransformer::EmitExec( ICodeWriter& writer, const uint32* words, const uint32 numWords, const ucode::instr_cf_exec_t& cf )
{
	const bool bIsConditional = cf.is_cond_exec();

	// sequence bytes
	const uint32 sequence = cf.serialize;

	// reset vertex stride for vfetch groups
	m_lastVertexStride = 0;

	// return code (statement list)
	CodeStatement execCode;

	// process instructions
	for ( uint32 i=0; i < cf.count; i++ )
	{
		// get actual ALU data for this instruction
		const uint32 aluOffset = (cf.address + i);

		// decode instruction type 
		const uint32 seqCode = sequence >> (i*2);
		const bool bFetch = (0 != (seqCode & 0x1));
		const bool bSync = (0 != (seqCode & 0x2));
		
		if ( bFetch )
		{
			const auto& fetch =	*(const instr_fetch_t*)(words + aluOffset*3);
			switch (fetch.opc)
			{
				case VTX_FETCH:
				{
					// evaluate fetch
					CodeStatement code = EmitVertexFetch( writer, fetch.vtx, bSync );
					code = EmitPredicateTest( writer, code, bIsConditional, cf.pred_condition, fetch.vtx.pred_select, fetch.vtx.pred_condition );
					execCode = writer.EmitMergeStatements( execCode, code );
					break;
				}

				case TEX_FETCH:
				{
					CodeStatement code = EmitTextureFetch( writer, fetch.tex, bSync );
					code = EmitPredicateTest( writer, code, bIsConditional, cf.pred_condition, fetch.tex.pred_select, fetch.tex.pred_condition );
					execCode = writer.EmitMergeStatements( execCode, code );
					break;
				}

				case TEX_GET_BORDER_COLOR_FRAC:
				case TEX_GET_COMP_TEX_LOD:
				case TEX_GET_GRADIENTS:
				case TEX_GET_WEIGHTS:
				case TEX_SET_TEX_LOD:
				case TEX_SET_GRADIENTS_H:
				case TEX_SET_GRADIENTS_V:
				default:
					DEBUG_CHECK( !"Unsupported fetch" );
					break;
			}
		}
		else
		{
			const auto& alu = *(const instr_alu_t*)(words + aluOffset*3);

			CodeStatement code = EmitALU( writer, alu, bSync );
			code = EmitPredicateTest( writer, code, bIsConditional, cf.pred_condition, alu.pred_select, alu.pred_condition );
			execCode = writer.EmitMergeStatements( execCode, code );
		}
	}

	// return final code for EXEC (statement list)
	return execCode;
}

const uint32 CXenonGPUMicrocodeTransformer::GetArgCount( EVectorInstr instr )
{
	switch ( instr ) 
	{
		case EVectorInstr::ADDv: return 2;
		case EVectorInstr::MULv: return 2;
		case EVectorInstr::MAXv: return 2;
		case EVectorInstr::MINv: return 2;
		case EVectorInstr::SETEv: return 2;
		case EVectorInstr::SETGTv: return 2;
		case EVectorInstr::SETGTEv: return 2;
		case EVectorInstr::SETNEv: return 2;
		case EVectorInstr::FRACv: return 1;
		case EVectorInstr::TRUNCv: return 1;
		case EVectorInstr::FLOORv: return 1;
		case EVectorInstr::MULADDv: return 3;
		case EVectorInstr::CNDEv: return 3;
		case EVectorInstr::CNDGTEv: return 3;
		case EVectorInstr::CNDGTv: return 3;
		case EVectorInstr::DOT4v: return 2;
		case EVectorInstr::DOT3v: return 2;
		case EVectorInstr::DOT2ADDv: return 3;
		case EVectorInstr::CUBEv: return 2;
		case EVectorInstr::MAX4v: return 1;
		case EVectorInstr::PRED_SETE_PUSHv: return 2;
		case EVectorInstr::PRED_SETNE_PUSHv: return 2;
		case EVectorInstr::PRED_SETGT_PUSHv: return 2;
		case EVectorInstr::PRED_SETGTE_PUSHv: return 2;
		case EVectorInstr::KILLEv: return 2;
		case EVectorInstr::KILLGTv: return 2;
		case EVectorInstr::KILLGTEv: return 2;
		case EVectorInstr::KILLNEv: return 2;
		case EVectorInstr::DSTv: return 2;
		case EVectorInstr::MOVAv: return 1;
	}

	DEBUG_CHECK( !"Unknown vector instruction" );
	return 0;
}

const uint32 CXenonGPUMicrocodeTransformer::GetArgCount( EScalarInstr instr )
{
	switch ( instr ) 
	{
		case EScalarInstr::ADDs: return 1;
		case EScalarInstr::ADD_PREVs: return 1;
		case EScalarInstr::MULs: return 1;
		case EScalarInstr::MUL_PREVs: return 1;
		case EScalarInstr::MUL_PREV2s: return 1;
		case EScalarInstr::MAXs: return 1;
		case EScalarInstr::MINs: return 1;
		case EScalarInstr::SETEs: return 1;
		case EScalarInstr::SETGTs: return 1;
		case EScalarInstr::SETGTEs: return 1;
		case EScalarInstr::SETNEs: return 1;
		case EScalarInstr::FRACs: return 1;
		case EScalarInstr::TRUNCs: return 1;
		case EScalarInstr::FLOORs: return 1;
		case EScalarInstr::EXP_IEEE: return 1;
		case EScalarInstr::LOG_CLAMP: return 1;
		case EScalarInstr::LOG_IEEE: return 1;
		case EScalarInstr::RECIP_CLAMP: return 1;
		case EScalarInstr::RECIP_FF: return 1;
		case EScalarInstr::RECIP_IEEE: return 1;
		case EScalarInstr::RECIPSQ_CLAMP: return 1;
		case EScalarInstr::RECIPSQ_FF: return 1;
		case EScalarInstr::RECIPSQ_IEEE: return 1;
		case EScalarInstr::MOVAs: return 1;
		case EScalarInstr::MOVA_FLOORs: return 1;
		case EScalarInstr::SUBs: return 1;
		case EScalarInstr::SUB_PREVs: return 1;
		case EScalarInstr::PRED_SETEs: return 1;
		case EScalarInstr::PRED_SETNEs: return 1;
		case EScalarInstr::PRED_SETGTs: return 1;
		case EScalarInstr::PRED_SETGTEs: return 1;
		case EScalarInstr::PRED_SET_INVs: return 1;
		case EScalarInstr::PRED_SET_POPs: return 1;
		case EScalarInstr::PRED_SET_CLRs: return 1;
		case EScalarInstr::PRED_SET_RESTOREs: return 1;
		case EScalarInstr::KILLEs: return 1;
		case EScalarInstr::KILLGTs: return 1;
		case EScalarInstr::KILLGTEs: return 1;
		case EScalarInstr::KILLNEs: return 1;
		case EScalarInstr::KILLONEs: return 1;
		case EScalarInstr::SQRT_IEEE: return 1;
		case EScalarInstr::MUL_CONST_0: return 2;
		case EScalarInstr::MUL_CONST_1: return 2;
		case EScalarInstr::ADD_CONST_0: return 2;
		case EScalarInstr::ADD_CONST_1: return 2;
		case EScalarInstr::SUB_CONST_0: return 2;
		case EScalarInstr::SUB_CONST_1: return 2;
		case EScalarInstr::SIN: return 1;
		case EScalarInstr::COS: return 1;
		case EScalarInstr::RETAIN_PREV: return 1;
	}

	DEBUG_CHECK( !"Unknown vector instruction" );
	return 0;
}

CodeExpr CXenonGPUMicrocodeTransformer::EmitSrcReg( ICodeWriter& writer, const ucode::instr_alu_t& op, const uint32 num, const uint32 type, const uint32 swiz, const uint32 negate, const int constSlot )
{
	// value source
	CodeExpr regValue;
	if (type)
	{
		// runtime register
		const uint32 regIndex = num & 0x7F;
		regValue = writer.EmitReadReg( regIndex );

		// take absolute value
		if ( num & 0x80 )
			regValue = writer.EmitAbs( regValue ); // add abs(x) around the value
	}
	else
	{
		const bool isPixelShader = (m_shaderType == XenonShaderType::ShaderPixel);
		if ((constSlot == 0 && op.const_0_rel_abs) || (constSlot == 1 && op.const_1_rel_abs))
		{
			if (op.relative_addr)
			{
				DEBUG_CHECK( num < 256 );
				regValue = writer.EmitFloatConstRel( isPixelShader, num ); // consts[a0 + num]
			}
			else
			{
				regValue = writer.EmitFloatConstRel( isPixelShader, 0 ); // consts[a0]
			}				
		}
		else
		{
			DEBUG_CHECK( num < 256 );
			regValue = writer.EmitFloatConst( isPixelShader, num ); // consts[num]
		}
		
		// take absolute value
		if (op.abs_constants)
			regValue = writer.EmitAbs( regValue ); // add abs(x) around the value
	}

	// negate the result
	if ( negate )
		regValue = writer.EmitNegate( regValue );

	// add swizzle select
	if (swiz)
	{
		// note that neutral (zero) swizzle pattern represents XYZW swizzle and the whole numbering is wrapped around
		const auto x = (const ESwizzle)( ((swiz >> 0) + 0) & 0x3 );
		const auto y = (const ESwizzle)( ((swiz >> 2) + 1) & 0x3 );
		const auto z = (const ESwizzle)( ((swiz >> 4) + 2) & 0x3 );
		const auto w = (const ESwizzle)( ((swiz >> 6) + 3) & 0x3 );
		regValue = writer.EmitReadSwizzle( regValue, x, y, z, w );
	}

	// return sampled register
	return regValue;
}

CodeExpr CXenonGPUMicrocodeTransformer::EmitSrcReg( ICodeWriter& writer, const ucode::instr_alu_t& op, const uint32 argIndex )
{
	// extract information about particular registers from the alu op
	if ( argIndex == 0 )
	{
		const int constSlot = 0;
		return EmitSrcReg( writer, op, op.src1_reg, op.src1_sel, op.src1_swiz, op.src1_reg_negate, constSlot );
	}
	else if ( argIndex == 1 )
	{
		const int constSlot = op.src1_sel ? 1 : 0;
		return EmitSrcReg( writer, op, op.src2_reg, op.src2_sel, op.src2_swiz, op.src2_reg_negate, constSlot );
	}
	else if ( argIndex == 2 )
	{
		const int constSlot = (op.src1_sel || op.src2_sel) ? 1 : 0;
		return EmitSrcReg( writer, op, op.src3_reg, op.src3_sel, op.src3_swiz, op.src3_reg_negate, constSlot );
	}
	else
	{
		DEBUG_CHECK( !"Invalid argument index" );
		return CodeExpr();
	}
}

inline uint32 MakeSwizzle4( uint32 x, uint32 y, uint32 z, uint32 w )
{
	return ((x&3) << 0) | ((y&3) << 0) | ((z&3) << 0) | ((w&3) << 0);
}

CodeExpr CXenonGPUMicrocodeTransformer::EmitSrcScalarReg1( ICodeWriter& writer, const ucode::instr_alu_t& op )
{
	const int constSlot = (op.src1_sel || op.src2_sel) ? 1 : 0;
	return EmitSrcReg( writer, op, op.src3_reg, op.src3_sel, MakeSwizzle4( 0,0,0,0 ), op.src3_reg_negate, constSlot );
}

CodeExpr CXenonGPUMicrocodeTransformer::EmitSrcScalarReg2( ICodeWriter& writer, const ucode::instr_alu_t& op )
{
	const int constSlot = (op.src1_sel || op.src2_sel) ? 1 : 0;
	return EmitSrcReg( writer, op, op.src3_reg, op.src3_sel, MakeSwizzle4( 1,1,1,1 ), op.src3_reg_negate, constSlot );
}

CodeStatement CXenonGPUMicrocodeTransformer::EmitVectorResult( ICodeWriter& writer, const ucode::instr_alu_t& op, const CodeExpr& code )
{
	// clamp value to 0-1 range
	CodeExpr input;
	if ( op.vector_clamp )
		input = writer.EmitSaturate( code );
	else
		input = code;

	// get name of destination register (the target)
	const bool bExported = (op.export_data != 0);
	const bool bPixelShader = (m_shaderType == XenonShaderType::ShaderPixel);
	const CodeExpr dest = writer.EmitWriteReg( bPixelShader, bExported, op.vector_dest ); // returns Rnum or exportPosition, etc, just the name

	// prepare write swizzle
	ESwizzle swz[4] = { ESwizzle::NOTUSED, ESwizzle::NOTUSED, ESwizzle::NOTUSED, ESwizzle::NOTUSED };
	if (op.export_data)
	{
		// For exported values we can have forced 1 and 0 appear in the mask
		const uint32 writeMask = op.vector_write_mask;
		const uint32 const1Mask = op.scalar_write_mask;
		for ( uint32 i=0; i<4; ++i )
		{
			const uint32 channelMask = 1 << i;
			if ( writeMask & channelMask )
			{
				if (const1Mask & channelMask )
					swz[i] = ESwizzle::ONE; // write 1.0 in the target
				else
					swz[i] = (ESwizzle)i; // same component
			}
			else if ( op.scalar_dest_rel )
			{
				swz[i] = ESwizzle::ZERO; // write 0.0 in the target, normaly we would not write anything but it's an export
			}
		}
	}
	else
	{
		// Normal swizzle
		const uint32 writeMask = op.vector_write_mask;
		for ( uint32 i=0; i<4; ++i )
		{
			const uint32 channelMask = 1 << i;
			if ( writeMask & channelMask )
				swz[i] = (ESwizzle)i; // same component
			else
				swz[i] = ESwizzle::NOTUSED; // do not write
		}
	}

	// having the input code and the swizzle mask return the final output code
	return writer.EmitWriteWithSwizzleStatement( dest, input, swz[0], swz[1], swz[2], swz[3] );	
}

CodeStatement CXenonGPUMicrocodeTransformer::EmitScalarResult( ICodeWriter& writer, const ucode::instr_alu_t& op, const CodeExpr& code )
{
	// clamp value to 0-1 range
	CodeExpr input;
	if ( op.vector_clamp )
		input = writer.EmitSaturate( code );
	else
		input = code;

	// select destination
	CodeExpr dest;
	uint32 writeMask;
	if ( op.export_data )
	{
		// during export scalar operation can still write into the vector
		const bool bPixelShader = (m_shaderType == XenonShaderType::ShaderPixel);
		dest = writer.EmitWriteReg( bPixelShader, true, op.vector_dest );
		writeMask = op.scalar_write_mask & ~op.vector_write_mask;
	}
	else
	{
		// pure scalar op
		const bool bPixelShader = (m_shaderType == XenonShaderType::ShaderPixel);
		dest = writer.EmitWriteReg( bPixelShader, false, op.scalar_dest );
		writeMask = op.scalar_write_mask;
	}

	// prepare write swizzle
	ESwizzle swz[4];
	for ( uint32 i=0; i<4; ++i )
	{
		const uint32 channelMask = 1 << i;
		if ( writeMask & channelMask )
			swz[i] = (ESwizzle)i; // same component
		else
			swz[i] = ESwizzle::NOTUSED; // do not write
	}

	// emit output
	return writer.EmitWriteWithSwizzleStatement( dest, input, swz[0], swz[1], swz[2], swz[3] );	
}

CodeStatement CXenonGPUMicrocodeTransformer::EmitALU( ICodeWriter& writer, const ucode::instr_alu_t& alu, const bool bSync )
{
	CodeStatement vectorPart, scalarPart, predPart;

	// Fast case - no magic stuff around
	if ( alu.vector_write_mask || (alu.export_data && alu.scalar_dest_rel) )
	{
		const auto vectorInstr = (EVectorInstr) alu.vector_opc;
		const uint32 argCount = GetArgCount( vectorInstr );

		// process function depending on the argument count
		if ( argCount == 1 )
		{
			CodeExpr arg1 = EmitSrcReg( writer, alu, 0 );
			CodeExpr func = writer.EmitVectorInstruction1( vectorInstr, arg1 );
			vectorPart = EmitVectorResult( writer, alu, func );
		}
		else if ( argCount == 2 )
		{
			CodeExpr arg1 = EmitSrcReg( writer, alu, 0 );
			CodeExpr arg2 = EmitSrcReg( writer, alu, 1 );
			CodeExpr func = writer.EmitVectorInstruction2( vectorInstr, arg1, arg2 );			
			vectorPart = EmitVectorResult( writer, alu, func );
		}
		else if ( argCount == 3 )
		{
			CodeExpr arg1 = EmitSrcReg( writer, alu, 0 );
			CodeExpr arg2 = EmitSrcReg( writer, alu, 1 );
			CodeExpr arg3 = EmitSrcReg( writer, alu, 2 );
			CodeExpr func = writer.EmitVectorInstruction3( vectorInstr, arg1, arg2, arg3 );
			vectorPart = EmitVectorResult( writer, alu, func );
		}
		else
		{
			DEBUG_CHECK( !"Unsupported argument count for vector instruction" );
		}
	}

	// Additional scalar instruction
	if ( alu.scalar_write_mask || !alu.vector_write_mask)
	{
		const auto scalarInstr = (EScalarInstr) alu.scalar_opc;
		const uint32 argCount = GetArgCount( scalarInstr );

		// process function depending on the argument count
		if ( argCount == 1 )
		{
			CodeExpr arg1 = EmitSrcReg( writer, alu, 2 );
			CodeExpr func = writer.EmitScalarInstruction1(scalarInstr, arg1);
			scalarPart = EmitScalarResult( writer, alu, func );
		}
		else if ( argCount == 2 )
		{
			CodeExpr arg1, arg2;
			if ( scalarInstr == EScalarInstr::MUL_CONST_0 || scalarInstr == EScalarInstr::MUL_CONST_1 || scalarInstr == EScalarInstr::ADD_CONST_0 || scalarInstr == EScalarInstr::ADD_CONST_1 || scalarInstr == EScalarInstr::SUB_CONST_0 || scalarInstr == EScalarInstr::SUB_CONST_1 )
			{
				const uint32 src3 = alu.src3_swiz & ~0x3C;
				const ESwizzle swizA = (ESwizzle)( ((src3 >> 6) - 1) & 0x3 );
				const ESwizzle swizB = (ESwizzle)( (src3 & 0x3) );

				arg1 = writer.EmitReadSwizzle( EmitSrcReg( writer, alu, alu.src3_reg, 0, 0, alu.src3_reg_negate, 0 ), swizA, swizA, swizA, swizA );

				const uint32 regB = (alu.scalar_opc & 1) | (alu.src3_swiz & 0x3C) | (alu.src3_sel << 1);
				const int const_slot = (alu.src1_sel || alu.src2_sel) ? 1 : 0;

				arg2 = writer.EmitReadSwizzle( EmitSrcReg( writer, alu, regB, 1, 0, 0, const_slot ), swizB, swizB, swizB, swizB );
			}
			else
			{
				DEBUG_CHECK( alu.vector_write_mask == 0 );
				arg1 = EmitSrcReg( writer, alu, 0 );
				arg2 = EmitSrcReg( writer, alu, 1 );
			}
			
			CodeExpr func = writer.EmitScalarInstruction2( scalarInstr, arg1, arg2 );
			scalarPart = EmitScalarResult( writer, alu, func );
		}
		else
		{
			DEBUG_CHECK( !"Unsupported argument count for scalar instruction" );
		}		
	}

	// Concatenate both operations
	const auto mergedStatement = writer.EmitMergeStatements( vectorPart, writer.EmitMergeStatements( predPart, scalarPart ) );
	return mergedStatement;
}
	
namespace Helper
{
	static bool IsFetchFormatFloat( const CXenonGPUMicrocodeTransformer::EFetchFormat format )
	{
		switch ( format )
		{
			case CXenonGPUMicrocodeTransformer::EFetchFormat::FMT_32_FLOAT: return true;
			case CXenonGPUMicrocodeTransformer::EFetchFormat::FMT_32_32_FLOAT: return true;
			case CXenonGPUMicrocodeTransformer::EFetchFormat::FMT_32_32_32_32_FLOAT: return true;
			case CXenonGPUMicrocodeTransformer::EFetchFormat::FMT_32_32_32_FLOAT: return true;
			case CXenonGPUMicrocodeTransformer::EFetchFormat::FMT_16_16_FLOAT: return true;
			case CXenonGPUMicrocodeTransformer::EFetchFormat::FMT_16_16_16_16_FLOAT: return true;
		}

		return false;
	}
}

CodeStatement CXenonGPUMicrocodeTransformer::EmitVertexFetch( ICodeWriter& writer, const ucode::instr_fetch_vtx_t& vtx, const bool bSync )
{
	// fetch slot and format
	const uint32 fetchSlot = vtx.const_index * 3 + vtx.const_index_sel;
	const uint32 fetchOffset = vtx.offset;
	const uint32 fetchStride = vtx.stride ? vtx.stride : m_lastVertexStride;
	const auto fetchFormat = (const EFetchFormat ) vtx.format;

	// update vertex stride (for vfetch + vfetch_mini combo)
	if ( vtx.stride )
		m_lastVertexStride = vtx.stride; 

	// fetch formats
	const bool isFloat = Helper::IsFetchFormatFloat( fetchFormat );
	const bool isSigned = (vtx.format_comp_all != 0);
	const bool isNormalized = (0 == vtx.num_format_all);

	// get the source register
	DEBUG_CHECK( vtx.src_reg_am == 0 );
	DEBUG_CHECK( vtx.dst_reg_am == 0 );
	CodeExpr sourceReg = writer.EmitReadReg( vtx.src_reg );
	
	// create the value fetcher (returns single expression with fetch result)
	const CodeExpr fetchedData = writer.EmitVertexFetch( sourceReg, fetchSlot, fetchOffset, fetchStride, fetchFormat, isFloat, isSigned, isNormalized );

	// destination register
	const bool bPixelShader = (m_shaderType == XenonShaderType::ShaderPixel);
	const CodeExpr destReg = writer.EmitWriteReg( bPixelShader, false, vtx.dst_reg );

	// setup target swizzle
	ESwizzle swz[4];
	for ( uint32 i=0; i<4; i++)
	{
		const uint32 swizMask = (vtx.dst_swiz >> (3*i)) & 0x7;

		if ( swizMask == 4 )
			swz[i] = ESwizzle::ZERO;
		else if ( swizMask == 5 )
			swz[i] = ESwizzle::ONE;
		else if ( swizMask == 6 )
			swz[i] = ESwizzle::DONTCARE; // we don't have to do anything
		else if ( swizMask == 7 )
			swz[i] = ESwizzle::NOTUSED; // do not override
		else if ( swizMask < 4 )
			swz[i] = (ESwizzle) swizMask;
	}

	// copy data
	return writer.EmitWriteWithSwizzleStatement( destReg, fetchedData, swz[0], swz[1], swz[2], swz[3] );
}

CodeStatement CXenonGPUMicrocodeTransformer::EmitTextureFetch( ICodeWriter& writer, const ucode::instr_fetch_tex_t& tex, const bool bSync )
{
	// destination register
	const bool bPixelShader = (m_shaderType == XenonShaderType::ShaderPixel);
	const CodeExpr destReg = writer.EmitWriteReg( bPixelShader, false, tex.dst_reg );

	// source register (texcoords)
	const CodeExpr srcReg = writer.EmitReadReg( tex.src_reg );
	const ESwizzle srcX = (ESwizzle)( (tex.src_swiz >> 0) & 3 );
	const ESwizzle srcY = (ESwizzle)( (tex.src_swiz >> 2) & 3 );
	const ESwizzle srcZ = (ESwizzle)( (tex.src_swiz >> 4) & 3 );
	const ESwizzle srcW = (ESwizzle)( (tex.src_swiz >> 6) & 3 );
	const CodeExpr srcRegSwiz = writer.EmitReadSwizzle( srcReg, srcX, srcY, srcZ, srcW );

	// sample the shit
	CodeExpr sample;
	if ( tex.dimension == ucode::DIMENSION_1D )
	{
		sample = writer.EmitTextureSample1D( srcRegSwiz, tex.const_idx );
	}
	else if ( tex.dimension == ucode::DIMENSION_2D )
	{
		sample = writer.EmitTextureSample2D( srcRegSwiz, tex.const_idx );
	}
	else if ( tex.dimension == ucode::DIMENSION_3D )
	{
		sample = writer.EmitTextureSample3D( srcRegSwiz, tex.const_idx );
	}
	else if ( tex.dimension == ucode::DIMENSION_CUBE )
	{
		sample = writer.EmitTextureSampleCube( srcRegSwiz, tex.const_idx );
	}

	// write back swizzled
	const ESwizzle destX = (ESwizzle)( (tex.dst_swiz >> 0) & 7 );
	const ESwizzle destY = (ESwizzle)( (tex.dst_swiz >> 3) & 7 );
	const ESwizzle destZ = (ESwizzle)( (tex.dst_swiz >> 6) & 7 );
	const ESwizzle destW = (ESwizzle)( (tex.dst_swiz >> 9) & 7 );
	return writer.EmitWriteWithSwizzleStatement( destReg, sample, destX, destY, destZ, destW );
}

CodeStatement CXenonGPUMicrocodeTransformer::EmitPredicateTest( ICodeWriter& writer, const CodeStatement& code, const bool bIsConditional, const uint32 flowPredCondition, const uint32 predSelect, const uint32 predCondition )
{
	if ( predSelect && (!bIsConditional || flowPredCondition != predCondition))
	{
		// get predicate register
		CodeExpr predicate = writer.EmitGetPredicate();

		// invert condition
		if ( !predCondition )
			predicate = writer.EmitNot(predicate);

		// wrap in a condition
		return writer.EmitConditionalStatement( predicate, code );
	}
	else
	{
		// no change
		return code;
	}
}

void CXenonGPUMicrocodeTransformer::TransformBlock( ICodeWriter& writer, const uint32* words, const uint32 numWords, const ucode::instr_cf_t* cfblock, uint32& codePC )
{
	// emit flow opcode
	const auto cfType = (EFlowBlock) cfblock->opc;
	switch ( cfType )
	{
		case EFlowBlock::NOP:
		{
			writer.EmitNop();
			break;
		}

		case EFlowBlock::ALLOC:
		{
			const auto& alloc = cfblock->alloc;

			if ( alloc.buffer_select == 1 )
			{
				writer.EmitExportAllocPosition();
			}
			else if ( alloc.buffer_select == 2 )
			{
				const uint32 count = 1 + alloc.size;
				writer.EmitExportAllocParam( count );
			}
			else if ( alloc.buffer_select == 3 )
			{
				const uint32 count = 1 + alloc.size;
				writer.EmitExportAllocMemExport( count );
			}

			break;
		}

		// conditional execution block
		case EFlowBlock::COND_EXEC:
		case EFlowBlock::COND_EXEC_END:
		case EFlowBlock::COND_PRED_EXEC:
		case EFlowBlock::COND_PRED_EXEC_END:
		case EFlowBlock::COND_EXEC_PRED_CLEAN:
		case EFlowBlock::COND_EXEC_PRED_CLEAN_END:
		{
			const auto& cf = cfblock->exec;
			
			// condition argument
			CodeStatement preamble, statement;

			// evaluate condition
			CodeExpr condition;
			if ( (cfType == EFlowBlock::COND_EXEC_PRED_CLEAN) || (cfType == EFlowBlock::COND_EXEC_PRED_CLEAN_END) )
			{
				const bool bPixelShader = (m_shaderType == XenonShaderType::ShaderPixel);
				const CodeExpr boolExpr = writer.EmitBoolConst( bPixelShader, cf.bool_addr );    // set new predication
				preamble = writer.EmitSetPredicateStatement( boolExpr );
				condition = boolExpr;
			}
			else
			{
				condition = writer.EmitGetPredicate();					// use existing predication
			}

			// invert condition
			if ( !cf.pred_condition )
				condition = writer.EmitNot( condition );

			// evaluate code
			const CodeStatement code = EmitExec( writer, words, numWords, cf );

			// emit execution block with that code (no condition)
			const bool bEndOfShader = (cfType == EFlowBlock::COND_EXEC_END) || (cfType == EFlowBlock::COND_PRED_EXEC_END) || (cfType == EFlowBlock::COND_EXEC_PRED_CLEAN_END);
			writer.EmitExec( cf.address, cfType, preamble, code, condition, bEndOfShader );

			// update PC if known
			codePC = cf.address + cf.count;
			break;
		}

		// execution block
		case EFlowBlock::EXEC:
		case EFlowBlock::EXEC_END:
		{
			// evaluate code
			const auto& cf = cfblock->exec;
			const CodeStatement code = EmitExec( writer, words, numWords, cf );

			// emit execution block with that code (no condition)
			const bool bEndOfShader = (cfType == EFlowBlock::COND_EXEC_END) || (cfType == EFlowBlock::COND_PRED_EXEC_END) || (cfType == EFlowBlock::EXEC_END);
			writer.EmitExec( cf.address, cfType, CodeStatement(), code, CodeExpr(), bEndOfShader );

			// update PC if known
			codePC = cf.address + cf.count;
			break;
		}

		// conditional flow control change
		case EFlowBlock::COND_CALL:
		case EFlowBlock::COND_JMP:
		{
			const auto& cf = cfblock->jmp_call;

			// calculate target address
			uint32 targetAddress = 0;
			if (cf.address_mode == ABSOLUTE_ADDR)
			{
				targetAddress = cf.address; // absolute address
			}
			else
			{
				DEBUG_CHECK( cf.direction == 0 ); // for now
				targetAddress = codePC + cf.address; // relative jump
			}

			// evaluate condition
			CodeExpr condition;
			CodeStatement preamble;
			if ( cf.force_call )
			{
				// always jump
			}
			else if (cf.predicated_jmp)
			{
				condition = writer.EmitGetPredicate();		// use existing predication
			}
			else
			{
				const bool bPixelShader = (m_shaderType == XenonShaderType::ShaderPixel);
				const CodeExpr boolExpr = writer.EmitBoolConst( bPixelShader, cf.bool_addr );    // set new predication
				preamble = writer.EmitSetPredicateStatement( boolExpr );
				condition = boolExpr;
			}

			// invert condition
			if ( !cf.condition )
				condition = writer.EmitNot( condition );

			// emit jump instruction
			if ( cfType == EFlowBlock::COND_CALL )
				writer.EmitJump( targetAddress, preamble, condition );
			else
				writer.EmitJump( targetAddress, preamble, condition );
			break;
		}

		case EFlowBlock::RETURN:
		case EFlowBlock::LOOP_START:
		case EFlowBlock::LOOP_END:
		case EFlowBlock::MARK_VS_FETCH_DONE:
		{
			GLog.Err( "D3D: Encountered unsupported control flow %d", cfType );
			break;
		}
	}
}
