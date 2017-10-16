#include "build.h"
#include "dx11MicrocodeDecompiler.h"
#include "dx11MicrocodeConstants.h"

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

const char* CDX11MicrocodeDecompiler::st_tabLevels[] =
{
    "",                 "\t",                 "\t\t",         "\t\t\t",
    "\t\t\t\t",         "\t\t\t\t\t",         "\t\t\t\t\t\t", "\t\t\t\t\t\t\t",
    "\t\t\t\t\t\t\t\t", "\t\t\t\t\t\t\t\t\t", "x",            "x",
    "x",                "x",                  "x",            "x",
};

const char CDX11MicrocodeDecompiler::st_chanNames[] =
{
    'x', 'y', 'z', 'w',
    /* these only apply to FETCH dst's: */
    '0', '1', '?', '_',
};

#define INSTR(opname, args) { args, #opname }
CDX11MicrocodeDecompiler::VectorInstr CDX11MicrocodeDecompiler::st_vectorInstructions[0x20] =
{
	INSTR(ADDv, 2),               // 0
	INSTR(MULv, 2),               // 1
	INSTR(MAXv, 2),               // 2
	INSTR(MINv, 2),               // 3
	INSTR(SETEv, 2),              // 4
	INSTR(SETGTv, 2),             // 5
	INSTR(SETGTEv, 2),            // 6
	INSTR(SETNEv, 2),             // 7
	INSTR(FRACv, 1),              // 8
	INSTR(TRUNCv, 1),             // 9
	INSTR(FLOORv, 1),             // 10
	INSTR(MULADDv, 3),            // 111
	INSTR(CNDEv, 3),              // 12
	INSTR(CNDGTEv, 3),            // 13
	INSTR(CNDGTv, 3),             // 14
	INSTR(DOT4v, 2),              // 15
	INSTR(DOT3v, 2),              // 16
	INSTR(DOT2ADDv, 3),           // 17 -- ???
	INSTR(CUBEv, 2),              // 18
	INSTR(MAX4v, 1),              // 19
	INSTR(PRED_SETE_PUSHv, 2),    // 20
	INSTR(PRED_SETNE_PUSHv, 2),   // 21
	INSTR(PRED_SETGT_PUSHv, 2),   // 22
	INSTR(PRED_SETGTE_PUSHv, 2),  // 23
	INSTR(KILLEv, 2),             // 24
	INSTR(KILLGTv, 2),            // 25
	INSTR(KILLGTEv, 2),           // 26
	INSTR(KILLNEv, 2),            // 27
	INSTR(DSTv, 2),               // 28
	INSTR(MOVAv, 1),              // 29
};

CDX11MicrocodeDecompiler::ScalarInstr CDX11MicrocodeDecompiler::st_scalarInstructions[0x40] =
{
	INSTR(ADDs, 1),               // 0
	INSTR(ADD_PREVs, 1),          // 1
	INSTR(MULs, 1),               // 2
	INSTR(MUL_PREVs, 1),          // 3
	INSTR(MUL_PREV2s, 1),         // 4
	INSTR(MAXs, 1),               // 5
	INSTR(MINs, 1),               // 6
	INSTR(SETEs, 1),              // 7
	INSTR(SETGTs, 1),             // 8
	INSTR(SETGTEs, 1),            // 9
	INSTR(SETNEs, 1),             // 10
	INSTR(FRACs, 1),              // 11
	INSTR(TRUNCs, 1),             // 12
	INSTR(FLOORs, 1),             // 13
	INSTR(EXP_IEEE, 1),           // 14
	INSTR(LOG_CLAMP, 1),          // 15
	INSTR(LOG_IEEE, 1),           // 16
	INSTR(RECIP_CLAMP, 1),        // 17
	INSTR(RECIP_FF, 1),           // 18
	INSTR(RECIP_IEEE, 1),         // 19
	INSTR(RECIPSQ_CLAMP, 1),      // 20
	INSTR(RECIPSQ_FF, 1),         // 21
	INSTR(RECIPSQ_IEEE, 1),       // 22
	INSTR(MOVAs, 1),              // 23
	INSTR(MOVA_FLOORs, 1),        // 24
	INSTR(SUBs, 1),               // 25
	INSTR(SUB_PREVs, 1),          // 26
	INSTR(PRED_SETEs, 1),         // 27
	INSTR(PRED_SETNEs, 1),        // 28
	INSTR(PRED_SETGTs, 1),        // 29
	INSTR(PRED_SETGTEs, 1),       // 30
	INSTR(PRED_SET_INVs, 1),      // 31
	INSTR(PRED_SET_POPs, 1),      // 32
	INSTR(PRED_SET_CLRs, 1),      // 33
	INSTR(PRED_SET_RESTOREs, 1),  // 34
	INSTR(KILLEs, 1),             // 35
	INSTR(KILLGTs, 1),            // 36
	INSTR(KILLGTEs, 1),           // 37
	INSTR(KILLNEs, 1),            // 38
	INSTR(KILLONEs, 1),           // 39
	INSTR(SQRT_IEEE, 1),          // 40
	{0, 0},
	INSTR(MUL_CONST_0, 2),  // 42
	INSTR(MUL_CONST_1, 2),  // 43
	INSTR(ADD_CONST_0, 2),  // 44
	INSTR(ADD_CONST_1, 2),  // 45
	INSTR(SUB_CONST_0, 2),  // 46
	INSTR(SUB_CONST_1, 2),  // 47
	INSTR(SIN, 1),          // 48
	INSTR(COS, 1),          // 49
	INSTR(RETAIN_PREV, 1),  // 50
};

#define FETCH_TYPE(x) { #x }
CDX11MicrocodeDecompiler::FetchType	CDX11MicrocodeDecompiler::st_fetchTypes[0xFF] =
{
	FETCH_TYPE(FMT_1_REVERSE),  // 0
	{0},
	FETCH_TYPE(FMT_8),  // 2
	{0},
	{0},
	{0},
	FETCH_TYPE(FMT_8_8_8_8),     // 6
	FETCH_TYPE(FMT_2_10_10_10),  // 7
	{0},
	{0},
	FETCH_TYPE(FMT_8_8),  // 10
	{0},
	{0},
	{0},
	{0},
	{0},
	{0},
	{0},
	{0},
	{0},
	{0},
	{0},
	{0},
	{0},
	FETCH_TYPE(FMT_16),           // 24
	FETCH_TYPE(FMT_16_16),        // 25
	FETCH_TYPE(FMT_16_16_16_16),  // 26
	{0},
	{0},
	{0},
	{0},
	{0},
	{0},
	FETCH_TYPE(FMT_32),                 // 33
	FETCH_TYPE(FMT_32_32),              // 34
	FETCH_TYPE(FMT_32_32_32_32),        // 35
	FETCH_TYPE(FMT_32_FLOAT),           // 36
	FETCH_TYPE(FMT_32_32_FLOAT),        // 37
	FETCH_TYPE(FMT_32_32_32_32_FLOAT),  // 38
	{0},
	{0},
	{0},
	{0},
	{0},
	{0},
	{0},
	{0},
	{0},
	{0},
	{0},
	{0},
	{0},
	{0},
	{0},
	{0},
	{0},
	{0},
	FETCH_TYPE(FMT_32_32_32_FLOAT),  // 57
#undef FETCH_TYPE
};

#define FETCH_INSTR( opc, name, funcType ) { name, funcType }
CDX11MicrocodeDecompiler::FetchInstr CDX11MicrocodeDecompiler::st_fetchInstr[28] =
{
	FETCH_INSTR(VTX_FETCH, "VERTEX", 0),  // 0
	FETCH_INSTR(TEX_FETCH, "SAMPLE", 1),  // 1
	{0, -1},
	{0, -1},
	{0, -1},
	{0, -1},
	{0, -1},
	{0, -1},
	{0, -1},
	{0, -1},
	{0, -1},
	{0, -1},
	{0, -1},
	{0, -1},
	{0, -1},
	{0, -1},
	FETCH_INSTR(TEX_GET_BORDER_COLOR_FRAC, "?", 1),  // 16
	FETCH_INSTR(TEX_GET_COMP_TEX_LOD, "?", 1),       // 17
	FETCH_INSTR(TEX_GET_GRADIENTS, "?", 1),          // 18
	FETCH_INSTR(TEX_GET_WEIGHTS, "?", 1),            // 19
	{0, -1},
	{0, -1},
	{0, -1},
	{0, -1},
	FETCH_INSTR(TEX_SET_TEX_LOD, "SET_TEX_LOD", 1),  // 24
	FETCH_INSTR(TEX_SET_GRADIENTS_H, "?", 1),        // 25
	FETCH_INSTR(TEX_SET_GRADIENTS_V, "?", 1),        // 26
	FETCH_INSTR(TEX_RESERVED_4, "?", 1),             // 27
#undef FETCH_INSTR
};

#define FLOW_INSTR(opc, type) { #opc, type }
CDX11MicrocodeDecompiler::FlowInstr	CDX11MicrocodeDecompiler::st_flowInstr[] = 
{
	FLOW_INSTR(NOP, 0),
	FLOW_INSTR(EXEC, 1),
	FLOW_INSTR(EXEC_END, 1),
	FLOW_INSTR(COND_EXEC, 1),
	FLOW_INSTR(COND_EXEC_END, 1),
	FLOW_INSTR(COND_PRED_EXEC, 1),
	FLOW_INSTR(COND_PRED_EXEC_END, 1),
	FLOW_INSTR(LOOP_START, 2),
	FLOW_INSTR(LOOP_END, 2),
	FLOW_INSTR(COND_CALL, 3),
	FLOW_INSTR(RETURN, 3),
	FLOW_INSTR(COND_JMP, 3),
	FLOW_INSTR(ALLOC, 4),
	FLOW_INSTR(COND_EXEC_PRED_CLEAN, 1),
	FLOW_INSTR(COND_EXEC_PRED_CLEAN_END, 1),
	FLOW_INSTR(MARK_VS_FETCH_DONE, 0),  // ??
#undef FLOW_INSTR
};


void CDX11MicrocodeDecompiler::PrintSrcReg( ICodeWriter& writer, const uint32 num, const uint32 type, const uint32 swiz, const uint32 negate, const uint32 abs_constants )
{
	if (negate)
		writer.Append( "-" );

	if (type)
	{
		if (num & 0x80)
			writer.Append("abs(");

		writer.Appendf("R%u", num & 0x7F);

		if (num & 0x80)
			writer.Append(")");
	}
	else
	{
	    if (abs_constants)
			writer.Append("|");

		const uint32 constBias = ((m_shaderType == XenonShaderType::ShaderPixel) ? 256 : 0);
		writer.Appendf("C%u", num + constBias);

		if (abs_constants)
			writer.Append("|");
	}

	if (swiz)
	{
		writer.Append(".");

		for ( int i = 0; i < 4; i++ )
		{
			const uint32 chanSwiz = swiz >> (2*i);
			writer.Appendf("%c", st_chanNames[(chanSwiz + i) & 0x3]); // NOTE: the +i permutes the channel list, so the swiz=0 will produce XYZW pattern
		}
	}
}

void CDX11MicrocodeDecompiler::PrintDestReg( ICodeWriter& writer, const uint32 num, const uint32 mask, const uint32 destExp )
{
	writer.Appendf( "%s%u", destExp ? "export" : "R", num);

	if (mask != 0xf)
	{
	    writer.Append(".");

		for (int i = 0; i < 4; i++)
		{
			const uint32 chanMask = mask >> i;
			writer.Appendf("%c", (chanMask & 0x1) ? st_chanNames[i] : '_');
		}
	}
}

void CDX11MicrocodeDecompiler::PrintExportComment( ICodeWriter& writer, const uint32 num )
{
	const char* name = NULL;
	if ( m_shaderType == XenonShaderType::ShaderVertex )
	{
		if ( num == 62 )
		{
			name = "POSITION";
		}
		else if ( num == 64 )
		{
			name = "POINTSIZE";
		}
	}
	else if ( m_shaderType == XenonShaderType::ShaderPixel )
	{
		if ( num == 0 )
		{
			name = "COLOR";
		}
	}
  
	if (name)
		writer.Appendf("\t; %s", name);
}

void CDX11MicrocodeDecompiler::DisasembleALU( ICodeWriter& writer, const uint32* words, const uint32 aluOff, const uint32 level, const uint32 sync )
{
	const instr_alu_t* alu = (const instr_alu_t*) words;

	writer.Append(st_tabLevels[level]);
	writer.Appendf("%02x: %08x %08x %08x\t", aluOff, words[0], words[1], words[2]);

	writer.Appendf("   %sALU:\t", sync ? "(S)" : "   ");

	if (!alu->scalar_write_mask && !alu->vector_write_mask)
	{
	    writer.Append("   <nop>\n");
	}

	if (alu->vector_write_mask)
	{
		const auto& instr = st_vectorInstructions[alu->vector_opc];
		writer.Append(instr.m_name);

		if (alu->pred_select & 0x2)
		{
			// seems to work similar to conditional execution in ARM instruction
			// set, so let's use a similar syntax for now:
			writer.Append((alu->pred_select & 0x1) ? "EQ" : "NE");
		}

		writer.Append("\t");

		PrintDestReg( writer, alu->vector_dest, alu->vector_write_mask, alu->export_data );
		writer.Append(" = ");

		if ( instr.m_numSources == 3 )
		{
			PrintSrcReg( writer, alu->src3_reg, alu->src3_sel, alu->src3_swiz, alu->src3_reg_negate, alu->abs_constants );
			writer.Append(", ");
		}

		PrintSrcReg( writer, alu->src1_reg, alu->src1_sel, alu->src1_swiz, alu->src1_reg_negate, alu->abs_constants );

		if ( instr.m_numSources > 1 )
		{
			writer.Append(", ");
			PrintSrcReg( writer, alu->src2_reg, alu->src2_sel, alu->src2_swiz, alu->src2_reg_negate, alu->abs_constants );
		}

		if (alu->vector_clamp)
			writer.Append(" CLAMP");

	    if (alu->pred_select)
			writer.Appendf(" COND(%d)", alu->pred_condition);

		if (alu->export_data)
			PrintExportComment( writer, alu->vector_dest );

	    writer.Append("\n");
	}	

	if (alu->scalar_write_mask || !alu->vector_write_mask)
	{
		if (alu->vector_write_mask)
		{
			writer.Append(st_tabLevels[level]);
			writer.Append("                          \t\t\t\t\t\t    \t");
		}

		const auto& instr = st_scalarInstructions[ alu->scalar_opc ];
		if ( instr.m_name )
		{
			writer.Appendf("%s\t", instr.m_name );
		}
		else
		{
			writer.Appendf("OP(%u)\t", alu->scalar_opc);
		}

		PrintDestReg( writer, alu->scalar_dest, alu->scalar_write_mask, alu->export_data );\
		writer.Append(" = ");

		if ( instr.m_numSources == 2 )
		{
			// MUL/ADD/etc
			// Clever, CONST_0 and CONST_1 are just an extra storage bit.
			// ADD_CONST_0 dest, [const], [reg]
			const uint32 src3_swiz = alu->src3_swiz & ~0x3C;
			const uint32 swiz_a = ((src3_swiz >> 6) - 1) & 0x3;
			const uint32 swiz_b = (src3_swiz & 0x3);
			PrintSrcReg( writer, alu->src3_reg, 0, 0, alu->src3_reg_negate, alu->abs_constants );
			writer.Appendf(".%c", st_chanNames[swiz_a]);
			writer.Append(", ");

			const uint32 reg2 = (alu->scalar_opc & 1) | (alu->src3_swiz & 0x3C) | (alu->src3_sel << 1);
			PrintSrcReg( writer, reg2, 1, 0, alu->src3_reg_negate, alu->abs_constants );
			writer.Appendf(".%c", st_chanNames[swiz_b]);
		}
		else
		{
			PrintSrcReg( writer, alu->src3_reg, alu->src3_sel, alu->src3_swiz, alu->src3_reg_negate, alu->abs_constants );
		}

		if (alu->scalar_clamp)
			writer.Append(" CLAMP");

	    if (alu->export_data)
			PrintExportComment( writer, alu->scalar_dest );

	    writer.Append("\n");
	}
}

void CDX11MicrocodeDecompiler::PrintFetchDest( ICodeWriter& writer, const uint32 dstReg, const uint32 dstSwiz )
{
	writer.Appendf("\tR%u.", dstReg );

	for (int i = 0; i < 4; i++)
	{
		const uint32 chanSwiz = dstSwiz >> (3*i);
		writer.Appendf("%c", st_chanNames[chanSwiz & 0x7]);
	}
}

void CDX11MicrocodeDecompiler::PrintFetchVtx( ICodeWriter& writer, const instr_fetch_t* fetch )
{
	const auto* vtx = &fetch->vtx;

	// seems to work similar to conditional execution in ARM instruction
	// set, so let's use a similar syntax for now:
	if (vtx->pred_select)
	    writer.Append( vtx->pred_condition ? "EQ" : "NE");

	PrintFetchDest( writer, vtx->dst_reg, vtx->dst_swiz );
	writer.Appendf(" = R%u.", vtx->src_reg);
	writer.Appendf("%c ", st_chanNames[vtx->src_swiz & 0x3]);

	const auto& fetchType = st_fetchTypes[vtx->format];
	if ( fetchType.m_name )
		writer.Append( fetchType.m_name );
	else
		writer.Appendf("FETCH_TYPE(0x%x)", vtx->format);
  
	writer.Append( vtx->format_comp_all ? "SIGNED" : "UNSIGNED");

	if (!vtx->num_format_all)
		writer.Append(" NORMALIZED");

	writer.Appendf(" STRIDE(%u)", vtx->stride);

	if (vtx->offset)
		writer.Appendf(" OFFSET(%u)", vtx->offset);

	writer.Appendf(" CONST(%u, %u)", vtx->const_index, vtx->const_index_sel);

	if (vtx->pred_select)
		writer.Appendf(" COND(%d)", vtx->pred_condition);

	if (1)
	{
		writer.Appendf(" src_reg_am=%u", vtx->src_reg_am);
		writer.Appendf(" dst_reg_am=%u", vtx->dst_reg_am);
		writer.Appendf(" num_format_all=%u", vtx->num_format_all);
		writer.Appendf(" signed_rf_mode_all=%u", vtx->signed_rf_mode_all);
		writer.Appendf(" exp_adjust_all=%u", vtx->exp_adjust_all);
	}
}

void CDX11MicrocodeDecompiler::PrintFetchTex( ICodeWriter& writer, const instr_fetch_t* fetch )
{
	static const char* filter[] = {
		"POINT",    // TEX_FILTER_POINT
		"LINEAR",   // TEX_FILTER_LINEAR
		"BASEMAP",  // TEX_FILTER_BASEMAP
	};
	static const char* aniso_filter[] = {
		"DISABLED",  // ANISO_FILTER_DISABLED
		"MAX_1_1",   // ANISO_FILTER_MAX_1_1
		"MAX_2_1",   // ANISO_FILTER_MAX_2_1
		"MAX_4_1",   // ANISO_FILTER_MAX_4_1
		"MAX_8_1",   // ANISO_FILTER_MAX_8_1
		"MAX_16_1",  // ANISO_FILTER_MAX_16_1
	};
	static const char* arbitrary_filter[] = {
		"2x4_SYM",   // ARBITRARY_FILTER_2X4_SYM
		"2x4_ASYM",  // ARBITRARY_FILTER_2X4_ASYM
		"4x2_SYM",   // ARBITRARY_FILTER_4X2_SYM
		"4x2_ASYM",  // ARBITRARY_FILTER_4X2_ASYM
		"4x4_SYM",   // ARBITRARY_FILTER_4X4_SYM
		"4x4_ASYM",  // ARBITRARY_FILTER_4X4_ASYM
	};
	static const char* sample_loc[] = {
		"CENTROID",  // SAMPLE_CENTROID
		"CENTER",    // SAMPLE_CENTER
	};

	const auto* tex = &fetch->tex;
	const uint32 src_swiz = tex->src_swiz;

	// seems to work similar to conditional execution in ARM instruction
	// set, so let's use a similar syntax for now:
	if (tex->pred_select)
		writer.Append( tex->pred_condition ? "EQ" : "NE" );

	PrintFetchDest( writer, tex->dst_reg, tex->dst_swiz);
	writer.Appendf(" = R%u.", tex->src_reg);

	for (int i = 0; i < 3; i++)
	{
		const uint32 chanSwiz = src_swiz >> (i*2);
		writer.Appendf("%c", st_chanNames[chanSwiz & 0x3]);
	}

	writer.Appendf(" CONST(%u)", tex->const_idx);

	if (tex->fetch_valid_only)
		writer.Append(" VALID_ONLY");

	if (tex->tx_coord_denorm)
		writer.Append(" DENORM");

	if (tex->mag_filter != TEX_FILTER_USE_FETCH_CONST)
		writer.Appendf(" MAG(%s)", filter[tex->mag_filter]);

	if (tex->min_filter != TEX_FILTER_USE_FETCH_CONST)
		writer.Appendf(" MIN(%s)", filter[tex->min_filter]);

	if (tex->mip_filter != TEX_FILTER_USE_FETCH_CONST)
		writer.Appendf(" MIP(%s)", filter[tex->mip_filter]);

	if (tex->aniso_filter != ANISO_FILTER_USE_FETCH_CONST)
		writer.Appendf(" ANISO(%s)", aniso_filter[tex->aniso_filter]);

	if (tex->arbitrary_filter != ARBITRARY_FILTER_USE_FETCH_CONST)
		writer.Appendf(" ARBITRARY(%s)", arbitrary_filter[tex->arbitrary_filter]);

	if (tex->vol_mag_filter != TEX_FILTER_USE_FETCH_CONST)
		writer.Appendf(" VOL_MAG(%s)", filter[tex->vol_mag_filter]);

	if (tex->vol_min_filter != TEX_FILTER_USE_FETCH_CONST)
		writer.Appendf(" VOL_MIN(%s)", filter[tex->vol_min_filter]);

	if (!tex->use_comp_lod)
	{
		writer.Appendf(" LOD(%u)", tex->use_comp_lod);
		writer.Appendf(" LOD_BIAS(%u)", tex->lod_bias);
	}

	if (tex->use_reg_lod)
		writer.Appendf(" REG_LOD(%u)", tex->use_reg_lod);

	if (tex->use_reg_gradients)
		writer.Append(" USE_REG_GRADIENTS");

	writer.Appendf(" LOCATION(%s)", sample_loc[tex->sample_location]);

	if (tex->offset_x || tex->offset_y || tex->offset_z)
		writer.Appendf(" OFFSET(%u,%u,%u)", tex->offset_x, tex->offset_y, tex->offset_z);

	if (tex->pred_select)
		writer.Appendf(" COND(%d)", tex->pred_condition);
}

void CDX11MicrocodeDecompiler::DisasembleFETCH( ICodeWriter& writer, const uint32* words, const uint32 aluOff, const uint32 level, const uint32 sync )
{
	const auto* fetch = (const instr_fetch_t*) words;

	writer.Append( st_tabLevels[level]);
	writer.Appendf("%02x: %08x %08x %08x\t", aluOff, words[0], words[1], words[2]);

	writer.Appendf("   %sFETCH:\t", sync ? "(S)" : "   ");

	const auto& instr = st_fetchInstr[fetch->opc];
	if ( instr.m_type == 0 )
	{
		writer.Append( instr.m_name );
		PrintFetchVtx( writer, fetch );
	}
	else if ( instr.m_type == 1 )
	{
		writer.Append( instr.m_name );
		PrintFetchTex( writer, fetch );
	}
	else
	{
		writer.Append("???");
	}

	writer.Append("\n");
}

void CDX11MicrocodeDecompiler::PrintFlowNop( ICodeWriter& writer, const instr_cf_t* cf )
{
}

void CDX11MicrocodeDecompiler::PrintFlowExec( ICodeWriter& writer, const instr_cf_t* cf )
{
	writer.Appendf(" ADDR(0x%x) CNT(0x%x)", cf->exec.address, cf->exec.count);

	if (cf->exec.yeild)
		writer.Append(" YIELD");
  
	const auto vc = cf->exec.vc_hi | (cf->exec.vc_lo << 2);

	  if (vc)
		writer.Appendf(" VC(0x%x)", vc);

	if (cf->exec.bool_addr)
		writer.Appendf(" BOOL_ADDR(0x%x)", cf->exec.bool_addr);

	if (cf->exec.address_mode == ABSOLUTE_ADDR)
		writer.Append(" ABSOLUTE_ADDR");

	if (cf->is_cond_exec())
		writer.Appendf(" COND(%d)", cf->exec.pred_condition);
}

void CDX11MicrocodeDecompiler::PrintFlowLoop( ICodeWriter& writer, const instr_cf_t* cf )
{
	writer.Appendf(" ADDR(0x%x) LOOP_ID(%d)", cf->loop.address, cf->loop.loop_id);

	if (cf->loop.address_mode == ABSOLUTE_ADDR)
	{
		writer.Append(" ABSOLUTE_ADDR");
	}
}

void CDX11MicrocodeDecompiler::PrintFlowJmpCall( ICodeWriter& writer, const instr_cf_t* cf )
{
	writer.Appendf(" ADDR(0x%x) DIR(%d)", cf->jmp_call.address, cf->jmp_call.direction );

	if (cf->jmp_call.force_call)
		writer.Append(" FORCE_CALL");
  
	if (cf->jmp_call.predicated_jmp)
		writer.Appendf(" COND(%d)", cf->jmp_call.condition);

	if (cf->jmp_call.bool_addr)
		writer.Appendf(" BOOL_ADDR(0x%x)", cf->jmp_call.bool_addr);

	if (cf->jmp_call.address_mode == ABSOLUTE_ADDR)
		writer.Append(" ABSOLUTE_ADDR");
}

void CDX11MicrocodeDecompiler::PrintFlowAlloc( ICodeWriter& writer, const instr_cf_t* cf )
{
  static const char* bufname[] = {
      "NO ALLOC",     // SQ_NO_ALLOC
      "POSITION",     // SQ_POSITION
      "PARAM/PIXEL",  // SQ_PARAMETER_PIXEL
      "MEMORY",       // SQ_MEMORY
  };

  writer.Appendf(" %s SIZE(0x%x)", bufname[cf->alloc.buffer_select], cf->alloc.size);

  if (cf->alloc.no_serial)
		writer.Append(" NO_SERIAL");

  if (cf->alloc.alloc_mode)
		writer.Append(" ALLOC_MODE");
}

void CDX11MicrocodeDecompiler::PrintFlow( ICodeWriter& writer, const instr_cf_t* cf, const uint32 level )
{
	writer.Append(st_tabLevels[level]);

	const uint32* words = (const uint32*)cf;
	writer.Appendf("      %08x %08x	    \t", words[0], words[1] );

	if ( cf->opc < ARRAYSIZE(st_flowInstr) )
	{
		const auto& instr = st_flowInstr[ cf->opc ];
		writer.Append( instr.m_name );

		if ( instr.m_type == 0 )
		{
			PrintFlowNop( writer, cf );
		}
		else if ( instr.m_type == 1 )
		{
			PrintFlowExec( writer, cf );
		}
		else if ( instr.m_type == 2 )
		{
			PrintFlowLoop( writer, cf );
		}
		else if ( instr.m_type == 3 )
		{
			PrintFlowJmpCall( writer, cf );
		}
		else if ( instr.m_type == 4 )
		{
			PrintFlowAlloc( writer, cf );
		}
	}
	else
	{
		writer.Appendf( "??? (%d)", cf->opc );
	}

	writer.Append("\n");
}

void CDX11MicrocodeDecompiler::DisasembleEXEC( ICodeWriter& writer, const uint32* words, const uint32 numWords, const uint32 level, const instr_cf_t* cf )
{
	uint32 sequence = cf->exec.serialize;

	for ( uint32 i = 0; i < cf->exec.count; i++)
	{
	    const uint32 aluOff = (cf->exec.address + i);
		const uint32 sync = sequence & 0x2;

		if (sequence & 0x1)
		{
			DisasembleFETCH( writer, words + aluOff*3, aluOff, level, sync );
		}
		else
		{
			DisasembleALU( writer, words + aluOff * 3, aluOff, level, sync );
		}

		sequence >>= 2;
	}
}

CDX11MicrocodeDecompiler::CDX11MicrocodeDecompiler( XenonShaderType shaderType )
	: m_shaderType( shaderType )
{
}

CDX11MicrocodeDecompiler::~CDX11MicrocodeDecompiler()
{

}

void CDX11MicrocodeDecompiler::DisassembleShader( ICodeWriter& writer, const uint32* words, const uint32 numWords )
{
	instr_cf_t cfa;
	instr_cf_t cfb;

	for ( uint32 idx = 0; idx < numWords; idx += 3)
	{
		const uint32 dword_0 = words[idx + 0];
		const uint32 dword_1 = words[idx + 1];
		const uint32 dword_2 = words[idx + 2];

		cfa.dword_0 = dword_0;
		cfa.dword_1 = dword_1 & 0xFFFF;
		cfb.dword_0 = (dword_1 >> 16) | (dword_2 << 16);
		cfb.dword_1 = dword_2 >> 16;

		PrintFlow( writer, &cfa, 0 );

		if (cfa.is_exec())
			DisasembleEXEC( writer, words, numWords, 0, &cfa);

	    PrintFlow( writer, &cfb, 0 );

		if (cfb.is_exec())
			DisasembleEXEC( writer, words, numWords, 0, &cfb);

		if (cfa.opc == EXEC_END || cfb.opc == EXEC_END)
			break;
	}
}


