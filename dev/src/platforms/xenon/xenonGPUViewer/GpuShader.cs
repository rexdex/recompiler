using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace xenonGPUViewer
{
    public enum GPUSwizzle
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

    enum GPUShaderType
    {
    	ShaderVertex = 0,
    	ShaderPixel = 1,
    };

    public enum GPUSurfaceFormat
    {
	  FMT_1_REVERSE = 0,
	  FMT_1 = 1,
	  FMT_8 = 2,
	  FMT_1_5_5_5 = 3,
	  FMT_5_6_5 = 4,
	  FMT_6_5_5 = 5,
	  FMT_8_8_8_8 = 6,
	  FMT_2_10_10_10 = 7,
	  FMT_8_A = 8,
	  FMT_8_B = 9,
	  FMT_8_8 = 10,
	  FMT_Cr_Y1_Cb_Y0 = 11,
	  FMT_Y1_Cr_Y0_Cb = 12,
	  FMT_5_5_5_1 = 13,
	  FMT_8_8_8_8_A = 14,
	  FMT_4_4_4_4 = 15,
	  FMT_10_11_11 = 16,
	  FMT_11_11_10 = 17,
	  FMT_DXT1 = 18,
	  FMT_DXT2_3 = 19,
	  FMT_DXT4_5 = 20,
	  FMT_24_8 = 22,
	  FMT_24_8_FLOAT = 23,
	  FMT_16 = 24,
	  FMT_16_16 = 25,
	  FMT_16_16_16_16 = 26,
	  FMT_16_EXPAND = 27,
	  FMT_16_16_EXPAND = 28,
	  FMT_16_16_16_16_EXPAND = 29,
	  FMT_16_FLOAT = 30,
	  FMT_16_16_FLOAT = 31,
	  FMT_16_16_16_16_FLOAT = 32,
	  FMT_32 = 33,
	  FMT_32_32 = 34,
	  FMT_32_32_32_32 = 35,
	  FMT_32_FLOAT = 36,
	  FMT_32_32_FLOAT = 37,
	  FMT_32_32_32_32_FLOAT = 38,
	  FMT_32_AS_8 = 39,
	  FMT_32_AS_8_8 = 40,
	  FMT_16_MPEG = 41,
	  FMT_16_16_MPEG = 42,
	  FMT_8_INTERLACED = 43,
	  FMT_32_AS_8_INTERLACED = 44,
	  FMT_32_AS_8_8_INTERLACED = 45,
	  FMT_16_INTERLACED = 46,
	  FMT_16_MPEG_INTERLACED = 47,
	  FMT_16_16_MPEG_INTERLACED = 48,
	  FMT_DXN = 49,
	  FMT_8_8_8_8_AS_16_16_16_16 = 50,
	  FMT_DXT1_AS_16_16_16_16 = 51,
	  FMT_DXT2_3_AS_16_16_16_16 = 52,
	  FMT_DXT4_5_AS_16_16_16_16 = 53,
	  FMT_2_10_10_10_AS_16_16_16_16 = 54,
	  FMT_10_11_11_AS_16_16_16_16 = 55,
	  FMT_11_11_10_AS_16_16_16_16 = 56,
	  FMT_32_32_32_FLOAT = 57,
	  FMT_DXT3A = 58,
	  FMT_DXT5A = 59,
	  FMT_CTX1 = 60,
	  FMT_DXT3A_AS_1_1_1_1 = 61,
	};

    public enum GPUFetchFormat
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

    public enum GPUFetchDataType
    {
        UNORM,
        UINT,
        SNORM,
        SINT,
        FLOAT,
    };

    public enum GPUScalarALU
    {
	  ADDs = 0,
	  ADD_PREVs = 1,
	  MULs = 2,
	  MUL_PREVs = 3,
	  MUL_PREV2s = 4,
	  MAXs = 5,
	  MINs = 6,
	  SETEs = 7,
	  SETGTs = 8,
	  SETGTEs = 9,
	  SETNEs = 10,
	  FRACs = 11,
	  TRUNCs = 12,
	  FLOORs = 13,
	  EXP_IEEE = 14,
	  LOG_CLAMP = 15,
	  LOG_IEEE = 16,
	  RECIP_CLAMP = 17,
	  RECIP_FF = 18,
	  RECIP_IEEE = 19,
	  RECIPSQ_CLAMP = 20,
	  RECIPSQ_FF = 21,
	  RECIPSQ_IEEE = 22,
	  MOVAs = 23,
	  MOVA_FLOORs = 24,
	  SUBs = 25,
	  SUB_PREVs = 26,
	  PRED_SETEs = 27,
	  PRED_SETNEs = 28,
	  PRED_SETGTs = 29,
	  PRED_SETGTEs = 30,
	  PRED_SET_INVs = 31,
	  PRED_SET_POPs = 32,
	  PRED_SET_CLRs = 33,
	  PRED_SET_RESTOREs = 34,
	  KILLEs = 35,
	  KILLGTs = 36,
	  KILLGTEs = 37,
	  KILLNEs = 38,
	  KILLONEs = 39,
	  SQRT_IEEE = 40,
	  MUL_CONST_0 = 42,
	  MUL_CONST_1 = 43,
	  ADD_CONST_0 = 44,
	  ADD_CONST_1 = 45,
	  SUB_CONST_0 = 46,
	  SUB_CONST_1 = 47,
	  SIN = 48,
	  COS = 49,
	  RETAIN_PREV = 50,
	};

    public enum GPUVectorALU
    {
	  ADDv = 0,
	  MULv = 1,
	  MAXv = 2,
	  MINv = 3,
	  SETEv = 4,
	  SETGTv = 5,
	  SETGTEv = 6,
	  SETNEv = 7,
	  FRACv = 8,
	  TRUNCv = 9,
	  FLOORv = 10,
	  MULADDv = 11,
	  CNDEv = 12,
	  CNDGTEv = 13,
	  CNDGTv = 14,
	  DOT4v = 15,
	  DOT3v = 16,
	  DOT2ADDv = 17,
	  CUBEv = 18,
	  MAX4v = 19,
	  PRED_SETE_PUSHv = 20,
	  PRED_SETNE_PUSHv = 21,
	  PRED_SETGT_PUSHv = 22,
	  PRED_SETGTE_PUSHv = 23,
	  KILLEv = 24,
	  KILLGTv = 25,
	  KILLGTEv = 26,
	  KILLNEv = 27,
	  DSTv = 28,
	  MOVAv = 29,
	};

    class BitUnpacker
    {
        public BitUnpacker( UInt32 val, UInt32 maxBits=32 )
        {
            _value = val;
            _bit = 0;
            _maxBits = maxBits;
        }

        public BitUnpacker Get( UInt32 bitCount, out UInt32 ret )
        {
            uint mask = (UInt32)(((int)1 << (int)bitCount) - 1);
            int bitShift = (int) _bit;
            //int bitShift = (int)(_maxBits - (int)_bit - (int)bitCount);
            if (bitShift < 0) throw new Exception("Dupa!");
            ret = (UInt32)( _value >> bitShift ) & mask;
            _bit += bitCount;
            return this;
        }

        private UInt32 _maxBits;
        private UInt32 _value;
        private UInt32 _bit;
    }

    public struct GPUInstrALU
	{
	    /* dword0: */
        public UInt32 vector_dest;// : 6;
        public UInt32 vector_dest_rel;// : 1;
        public UInt32 abs_constants;// : 1;
        public UInt32 scalar_dest;// : 6;
        public UInt32 scalar_dest_rel;// : 1;
        public UInt32 export_data;// : 1;
        public UInt32 vector_write_mask;// : 4;
        public UInt32 scalar_write_mask;// : 4;
        public UInt32 vector_clamp;// : 1;
        public UInt32 scalar_clamp;// : 1;
        public UInt32 scalar_opc;// : 6;  // instr_scalar_opc_t

        /* dword1: */
        public UInt32 src3_swiz;// : 8;
        public UInt32 src2_swiz;// : 8;
        public UInt32 src1_swiz;// : 8;
        public UInt32 src3_reg_negate;// : 1;
        public UInt32 src2_reg_negate;// : 1;
        public UInt32 src1_reg_negate;// : 1;
        public UInt32 pred_condition;// : 1;
        public UInt32 pred_select;// : 1;
        public UInt32 relative_addr;// : 1;
        public UInt32 const_1_rel_abs;// : 1;
        public UInt32 const_0_rel_abs;// : 1;

	    /* dword2: */
        public UInt32 src3_reg;// : 8;
        public UInt32 src2_reg;// : 8;
        public UInt32 src1_reg;// : 8;
        public UInt32 vector_opc;// : 5;  // instr_vector_opc_t
        public UInt32 src3_sel;// : 1;
        public UInt32 src2_sel;// : 1;
        public UInt32 src1_sel;// : 1;
	  
        public GPUInstrALU( UInt32 a, UInt32 b, UInt32 c )
        {
            (new BitUnpacker(a)).
                Get(6, out vector_dest).
                Get(1, out vector_dest_rel).
                Get(1, out abs_constants).
                Get(6, out scalar_dest).
                Get(1, out scalar_dest_rel).
                Get(1, out export_data).
                Get(4, out vector_write_mask).
                Get(4, out scalar_write_mask).
                Get(1, out vector_clamp).
                Get(1, out scalar_clamp).
                Get(6, out scalar_opc);

            (new BitUnpacker(b)).
                Get(8, out src3_swiz).
                Get(8, out src2_swiz).
                Get(8, out src1_swiz).
                Get(1, out src3_reg_negate).
                Get(1, out src2_reg_negate).
                Get(1, out src1_reg_negate).
                Get(1, out pred_condition).
                Get(1, out pred_select).
                Get(1, out relative_addr).
                Get(1, out const_1_rel_abs).
                Get(1, out const_0_rel_abs);

            (new BitUnpacker(c)).
                Get(8, out src3_reg).
                Get(8, out src2_reg).
                Get(8, out src1_reg).
                Get(5, out vector_opc).
                Get(1, out src3_sel).
                Get(1, out src2_sel).
                Get(1, out src1_sel);
        }
	};

	/*
	 * CF instructions:
	 */

    public enum GPUControlFlow
	{
	  NOP = 0,
	  EXEC = 1,
	  EXEC_END = 2,
	  COND_EXEC = 3,
	  COND_EXEC_END = 4,
	  COND_PRED_EXEC = 5,
	  COND_PRED_EXEC_END = 6,
	  LOOP_START = 7,
	  LOOP_END = 8,
	  COND_CALL = 9,
	  RETURN = 10,
	  COND_JMP = 11,
	  ALLOC = 12,
	  COND_EXEC_PRED_CLEAN = 13,
	  COND_EXEC_PRED_CLEAN_END = 14,
	  MARK_VS_FETCH_DONE = 15,
	};

    public enum GPUAddressMode
	{
	  RELATIVE_ADDR = 0,
	  ABSOLUTE_ADDR = 1,
	};

    public enum GPUAllocType
	{
	  SQ_NO_ALLOC = 0,
	  SQ_POSITION = 1,
	  SQ_PARAMETER_PIXEL = 2,
	  SQ_MEMORY = 3,
	};

	public struct GPUInstrCfExec
	{
        public UInt32 address;// : 12;
        public UInt32 count;// : 3;
        public UInt32 yeild;// : 1;
        public UInt32 serialize;//: 12;
        public UInt32 vc_hi;// : 4;

        public UInt32 vc_lo;// : 2; /* vertex cache? */
        public UInt32 bool_addr;// : 8;
        public UInt32 pred_condition;// : 1;
        public UInt32 address_mode;// : 1;  // instr_addr_mode_t
        public UInt32 opc;//: 4;           // instr_cf_opc_t

	    public bool IsCondExec()
        {
            GPUControlFlow _opc = (GPUControlFlow) opc;
		    return ((_opc == GPUControlFlow.COND_EXEC) || (_opc == GPUControlFlow.COND_EXEC_END) ||
			       (_opc == GPUControlFlow.COND_PRED_EXEC) || (_opc == GPUControlFlow.COND_PRED_EXEC_END) ||
			       (_opc == GPUControlFlow.COND_EXEC_PRED_CLEAN) ||
			       (_opc == GPUControlFlow.COND_EXEC_PRED_CLEAN_END));
	    }

        public GPUInstrCfExec( UInt32 a, UInt32 b )
        {
            (new BitUnpacker(a)).
                Get(12, out address).
                Get(3, out count).
                Get(1, out yeild).
                Get(12, out serialize).
                Get(4, out vc_hi);

            (new BitUnpacker(b, 16)).
                Get(2, out vc_lo).
                Get(8, out bool_addr).
                Get(1, out pred_condition).
                Get(1, out address_mode).
                Get(4, out opc);
        }
	};

    public struct GPUInstrCfLoop
	{
        // dword0
        public UInt32 address;// : 13;
        public UInt32 repeat;// : 1;
        public UInt32 reserved0;// : 2;
        public UInt32 loop_id;// : 5;
        public UInt32 pred_break;// : 1;
        public UInt32 reserved1_hi;// : 10;

        // dword1
        public UInt32 reserved1_lo;// : 10;
        public UInt32 condition;// : 1;
        public UInt32 address_mode;// : 1;  // instr_addr_mode_t
        public UInt32 opc;// : 4;           // instr_cf_opc_t

        public GPUInstrCfLoop( UInt32 a, UInt32 b )
        {
            (new BitUnpacker(a)).
                Get(13, out address).
                Get(1, out repeat).
                Get(2, out reserved0).
                Get(5, out loop_id).
                Get(1, out pred_break).
                Get(10, out reserved1_hi);

            (new BitUnpacker(b, 16)).
                Get(10, out reserved1_lo).
                Get(1, out condition).
                Get(1, out address_mode).
                Get(4, out opc);
        }
	};

    public struct GPUInstrCfJmpCall
	{
        // dword0
        public UInt32 address;// : 13;
        public UInt32 force_call;// : 1;
        public UInt32 predicated_jmp;// : 1;
        public UInt32 reserved1_hi;// : 17;

        // dword1
        public UInt32 reserved1_lo;// : 1;
        public UInt32 direction;// : 1;
        public UInt32 bool_addr;// : 8;
        public UInt32 condition;// : 1;
        public UInt32 address_mode;// : 1;  // instr_addr_mode_t
        public UInt32 opc;// : 4;           // instr_cf_opc_t

        public GPUInstrCfJmpCall( UInt32 a, UInt32 b )
        {
            (new BitUnpacker(a)).
                Get(13, out address).
                Get(1, out force_call).
                Get(1, out predicated_jmp).
                Get(17, out reserved1_hi);

            (new BitUnpacker(b, 16)).
                Get(1, out reserved1_lo).
                Get(1, out direction).
                Get(8, out bool_addr).
                Get(1, out condition).
                Get(1, out address_mode).
                Get(4, out opc);
        }
	};

    public struct GPUInstrCfAlloc
	{
        // dword0
		public UInt32 size;// : 3;
        public UInt32 reserved0_hi;// : 29;

        // dword1
        public UInt32 reserved0_lo;// : 8;
        public UInt32 no_serial;// : 1;
        public UInt32 buffer_select;// : 2;  // instr_alloc_type_t
        public UInt32 alloc_mode;// : 1;
        public UInt32 opc;// : 4;  // instr_cf_opc_t

        public GPUInstrCfAlloc( UInt32 a, UInt32 b )
        {
            (new BitUnpacker(a)).
                Get(3, out size).
                Get(29, out reserved0_hi);

            (new BitUnpacker(b, 16)).
                Get(8, out reserved0_lo).
                Get(1, out no_serial).
                Get(2, out buffer_select).
                Get(1, out alloc_mode).
                Get(4, out opc);
        }
	};

	public enum GPUFetchOpcode
	{
	  VTX_FETCH = 0,
	  TEX_FETCH = 1,
	  TEX_GET_BORDER_COLOR_FRAC = 16,
	  TEX_GET_COMP_TEX_LOD = 17,
	  TEX_GET_GRADIENTS = 18,
	  TEX_GET_WEIGHTS = 19,
	  TEX_SET_TEX_LOD = 24,
	  TEX_SET_GRADIENTS_H = 25,
	  TEX_SET_GRADIENTS_V = 26,
	  TEX_RESERVED_4 = 27,
	};

	public enum GPUTextureFilterMode
	{
	  TEX_FILTER_POINT = 0,
	  TEX_FILTER_LINEAR = 1,
	  TEX_FILTER_BASEMAP = 2, /* only applicable for mip-filter */
	  TEX_FILTER_USE_FETCH_CONST = 3,
	};

	public enum GPUTextureAnisoMode
	{
	  ANISO_FILTER_DISABLED = 0,
	  ANISO_FILTER_MAX_1_1 = 1,
	  ANISO_FILTER_MAX_2_1 = 2,
	  ANISO_FILTER_MAX_4_1 = 3,
	  ANISO_FILTER_MAX_8_1 = 4,
	  ANISO_FILTER_MAX_16_1 = 5,
	  ANISO_FILTER_USE_FETCH_CONST = 7,
	};

	public enum GPUTextureArbitraryFilterMode
	{
	  ARBITRARY_FILTER_2X4_SYM = 0,
	  ARBITRARY_FILTER_2X4_ASYM = 1,
	  ARBITRARY_FILTER_4X2_SYM = 2,
	  ARBITRARY_FILTER_4X2_ASYM = 3,
	  ARBITRARY_FILTER_4X4_SYM = 4,
	  ARBITRARY_FILTER_4X4_ASYM = 5,
	  ARBITRARY_FILTER_USE_FETCH_CONST = 7,
	};

	public enum GPUTextureSampleLocation
	{
	  SAMPLE_CENTROID = 0,
	  SAMPLE_CENTER = 1,
	};

	public enum GPUTextureDimension
	{
	  DIMENSION_1D = 0,
	  DIMENSION_2D = 1,
	  DIMENSION_3D = 2,
	  DIMENSION_CUBE = 3,
	};

    public struct GPUInstrFetchTex
	{
        // dword0
        public UInt32 opc;// : 5;  // instr_fetch_opc_t
        public UInt32 src_reg;// : 6;
        public UInt32 src_reg_am;// : 1;
        public UInt32 dst_reg;// : 6;
        public UInt32 dst_reg_am;// : 1;
        public UInt32 fetch_valid_only;// : 1;
        public UInt32 const_idx;// : 5;
        public UInt32 tx_coord_denorm;// : 1;
        public UInt32 src_swiz;// : 6;  // xyz

	    // dword1
        public UInt32 dst_swiz;// : 12;         // xyzw
        public UInt32 mag_filter;// : 2;        // instr_tex_filter_t
        public UInt32 min_filter;// : 2;        // instr_tex_filter_t
        public UInt32 mip_filter;// : 2;        // instr_tex_filter_t
        public UInt32 aniso_filter;// : 3;      // instr_aniso_filter_t
        public UInt32 arbitrary_filter;// : 3;  // instr_arbitrary_filter_t
        public UInt32 vol_mag_filter;// : 2;    // instr_tex_filter_t
        public UInt32 vol_min_filter;// : 2;    // instr_tex_filter_t
        public UInt32 use_comp_lod;// : 1;
        public UInt32 use_reg_lod;// : 1;
        public UInt32 unk;// : 1;
        public UInt32 pred_select;// : 1;

        // dword2
        public UInt32 use_reg_gradients;// : 1;
        public UInt32 sample_location;// : 1;  // instr_sample_loc_t
        public UInt32 lod_bias;// : 7;
        public UInt32 unused;// : 5;
        public UInt32 dimension;// : 2;  // instr_dimension_t
        public UInt32 offset_x;// : 5;
        public UInt32 offset_y;// : 5;
        public UInt32 offset_z;// : 5;
        public UInt32 pred_condition;// : 1;

        public GPUInstrFetchTex(UInt32 a, UInt32 b, UInt32 c)
        {
            (new BitUnpacker(a)).
                Get(5, out opc).
                Get(6, out src_reg).
                Get(1, out src_reg_am).
                Get(6, out dst_reg).
                Get(1, out dst_reg_am).
                Get(1, out fetch_valid_only).
                Get(5, out const_idx).
                Get(1, out tx_coord_denorm).
                Get(6, out src_swiz);

            (new BitUnpacker(b)).
                Get(12, out dst_swiz).
                Get(2, out mag_filter).
                Get(2, out min_filter).
                Get(2, out mip_filter).
                Get(3, out aniso_filter).
                Get(3, out arbitrary_filter).
                Get(2, out vol_mag_filter).
                Get(2, out vol_min_filter).
                Get(1, out use_comp_lod).
                Get(1, out use_reg_lod).
                Get(1, out unk).
                Get(1, out pred_select);

            (new BitUnpacker(c)).
                Get(1, out use_reg_gradients).
                Get(1, out sample_location).
                Get(7, out lod_bias).
                Get(5, out unused).
                Get(2, out dimension).
                Get(5, out offset_x).
                Get(5, out offset_y).
                Get(5, out offset_z).
                Get(1, out pred_condition);
        }
	};

    public struct GPUVertexFetch
    {
        public GPUFetchFormat format;
        public GPUFetchDataType dataType;
        public UInt32 slot;
        public UInt32 stride;
        public UInt32 offset;

        public string ToFormatStr()
        {
            string ret = "";

            ret += offset.ToString();
            ret += ",";
            ret += Enum.GetName(typeof(GPUFetchFormat), format);
            ret += ",";
            ret += Enum.GetName(typeof(GPUFetchDataType), dataType);

            return ret;
        }
    }

    public struct GPUInstrFetchVtx
	{
        /* dword0: */
        public UInt32 opc;// : 5;  // instr_fetch_opc_t
        public UInt32 src_reg;// : 6;
        public UInt32 src_reg_am;// : 1;
        public UInt32 dst_reg;// : 6;
        public UInt32 dst_reg_am;// : 1;
        public UInt32 must_be_one;// : 1;
        public UInt32 const_index;// : 5;
        public UInt32 const_index_sel;// : 2;
        public UInt32 reserved0;// : 3;
        public UInt32 src_swiz;// : 2;

        /* dword1: */
        public UInt32 dst_swiz;// : 12;
        public UInt32 format_comp_all;// : 1; /* '1' for signed, '0' for unsigned? */
        public UInt32 num_format_all;// : 1;  /* '0' for normalized, '1' for unnormalized */
        public UInt32 signed_rf_mode_all;// : 1;
        public UInt32 reserved1;// : 1;
        public UInt32 format;// : 6;  // instr_surf_fmt_t
        public UInt32 reserved2;// : 1;
        public UInt32 exp_adjust_all;// : 7;
        public UInt32 reserved3;// : 1;
        public UInt32 pred_select;// : 1;

	    /* dword2: */
        public UInt32 stride;// : 8;
        public UInt32 offset;// : 23;
        public UInt32 pred_condition;// : 1;

        public GPUInstrFetchVtx(UInt32 a, UInt32 b, UInt32 c)
        {
            (new BitUnpacker(a)).
                Get(5, out opc).
                Get(6, out src_reg).
                Get(1, out src_reg_am).
                Get(6, out dst_reg).
                Get(1, out dst_reg_am).
                Get(1, out must_be_one).
                Get(5, out const_index).
                Get(2, out const_index_sel).
                Get(3, out reserved0).
                Get(2, out src_swiz);

            (new BitUnpacker(b)).
                Get(12, out dst_swiz).
                Get(1, out format_comp_all).
                Get(1, out num_format_all).
                Get(1, out signed_rf_mode_all).
                Get(1, out reserved1).
                Get(6, out format).
                Get(1, out reserved2).
                Get(7, out exp_adjust_all).
                Get(1, out reserved3).
                Get(1, out pred_select);

            (new BitUnpacker(c)).
                Get(8, out stride).
                Get(23, out offset).
                Get(1, out pred_condition);
         }
	};

    public class GPUTextureFetch
    {
        public UInt32 slot;
        public GPUTextureDimension dimm;
        public GPUTextureFilterMode minFilter;
        public GPUTextureFilterMode mipFilter;
        public GPUTextureFilterMode magFilter;
        public GPUTextureAnisoMode anisoFilter;
        public GPUTextureArbitraryFilterMode arbitraryFilter;
        public UInt32 vol_mag_filter;
        public UInt32 vol_min_filter;
        public UInt32 use_comp_lod;
        public UInt32 use_reg_lod;
        public UInt32 use_reg_gradients;
        public UInt32 sample_location;
        public Int32 lod_bias; //7
        public Int32 offset_x; //5
        public Int32 offset_y; //5
        public Int32 offset_z; //5

        public GPUTextureFetch()
        {
            slot = 0;
            dimm = GPUTextureDimension.DIMENSION_2D;
            minFilter = GPUTextureFilterMode.TEX_FILTER_LINEAR;
            mipFilter = GPUTextureFilterMode.TEX_FILTER_LINEAR;
            magFilter = GPUTextureFilterMode.TEX_FILTER_LINEAR;
            anisoFilter = GPUTextureAnisoMode.ANISO_FILTER_DISABLED;
            arbitraryFilter = GPUTextureArbitraryFilterMode.ARBITRARY_FILTER_USE_FETCH_CONST;
            vol_mag_filter = 0;
            vol_min_filter = 0;
            use_comp_lod = 0;
            use_reg_lod = 0;
            use_reg_gradients = 0;
            sample_location = 0;
            lod_bias = 0; //7
            offset_x = 0; //5
            offset_y = 0; //5
            offset_z = 0; //5
        }

        public void Descibe(ref string txt)
        {
            txt += "<b>Slot: " + slot.ToString() + "</b><br>";
            txt += "Dimension: " + Enum.GetName(typeof(GPUTextureDimension), dimm) + "<br>";
            txt += "MinFilter: " + Enum.GetName(typeof(GPUTextureFilterMode), minFilter) + "<br>";
            txt += "MipFilter: " + Enum.GetName(typeof(GPUTextureFilterMode), mipFilter) + "<br>";
            txt += "MagFilter: " + Enum.GetName(typeof(GPUTextureFilterMode), magFilter) + "<br>";
            txt += "AnisoFilter: " + Enum.GetName(typeof(GPUTextureAnisoMode), anisoFilter) + "<br>";
            txt += "ArbitraryFilter: " + Enum.GetName(typeof(GPUTextureArbitraryFilterMode), arbitraryFilter) + "<br>";
            txt += "VolMinFilter: " + vol_min_filter.ToString()+ "<br>";
            txt += "VolMagFilter: " + vol_mag_filter.ToString() + "<br>";
            txt += "UseCompLod: " + use_comp_lod.ToString() + "<br>";
            txt += "UseRegLod: " + use_reg_lod.ToString() + "<br>";
            txt += "UseRegGradients: " + use_reg_gradients.ToString() + "<br>";
            txt += "SampleLocation: " + sample_location.ToString() + "<br>";
            txt += "LodBias: " + lod_bias.ToString() + "<br>";
            txt += "Offset: [" + offset_x.ToString() + "," + offset_y.ToString() + "," + offset_z.ToString() + "]<br>";
        }
    }

    public class GPUShader
    {
        private bool _PixelShader;
        private List<string> _Decompiled;
        private List<GPUVertexFetch> _VertexFetches;
        private List<GPUTextureFetch> _TextureFetches;
        private SortedSet<UInt32> _UsedRegisters;
        private UInt32 _LastVertexStride;

        public bool PixelShader { get { return _PixelShader; } }
        public List<string> Decompiled { get { return _Decompiled; } }
        public List<GPUVertexFetch> VertexFetches { get { return _VertexFetches; } }
        public List<GPUTextureFetch> TextureFetches { get { return _TextureFetches; } }
        public SortedSet<UInt32> UsedRegisters { get { return _UsedRegisters; } }

        protected GPUShader()
        {
            _Decompiled = new List<string>();
            _VertexFetches = new List<GPUVertexFetch>();
            _TextureFetches = new List<GPUTextureFetch>();
            _UsedRegisters = new SortedSet<UInt32>();
            _LastVertexStride = 0;
        }

        public static GPUShader Decompile(bool pixelShader, UInt32[] words)
        {
            GPUShader ret = new GPUShader();
            ret._PixelShader = pixelShader;

            for (UInt32 idx = 0; idx < words.Length; idx += 3)
            {
                UInt32 dword_0 = words[idx + 0];
                UInt32 dword_1 = words[idx + 1];
                UInt32 dword_2 = words[idx + 2];

                bool end = false;
                {
                    UInt32 a = dword_0;
                    UInt32 b = dword_1 & 0xFFFF;
                    end |= ret.TransformBlock(a, b, words);
                }

                {
                    UInt32 a = (dword_1 >> 16) | (dword_2 << 16);
                    UInt32 b = dword_2 >> 16;
                    end |= ret.TransformBlock(a, b, words);
                }

                if (end)
                    break;
            }

            return ret;
        }

        private bool TransformBlock(UInt32 a, UInt32 b, UInt32[] words)
        {
            //GPUControlFlow opc = (GPUControlFlow)((b >> 28) & 0xF);
            GPUControlFlow opc = (GPUControlFlow)((b >> 12) & 0xF);
            bool end = false;

            switch (opc)
            {
                case GPUControlFlow.NOP:
                    break;

                case GPUControlFlow.EXEC:
	            case GPUControlFlow.EXEC_END:
                {
                    end = (opc == GPUControlFlow.EXEC_END);

                    _Decompiled.Add("EXEC");
                    GPUInstrCfExec instr = new GPUInstrCfExec(a,b);
                    String condition = "    ";
                    EmitExec( condition, instr, words);
                    break;
                }

                case GPUControlFlow.ALLOC:
                {
                    GPUInstrCfAlloc instr = new GPUInstrCfAlloc (a,b);
                    EmitAlloc( instr );
                    break;
                }

	            case GPUControlFlow.COND_EXEC:
	            case GPUControlFlow.COND_EXEC_END:
	            case GPUControlFlow.COND_PRED_EXEC:
	            case GPUControlFlow.COND_PRED_EXEC_END:
	            case GPUControlFlow.COND_EXEC_PRED_CLEAN:
	            case GPUControlFlow.COND_EXEC_PRED_CLEAN_END:
                {
                    GPUInstrCfExec instr = new GPUInstrCfExec(a,b);

                    _Decompiled.Add("EXEC");
                    end = (opc == GPUControlFlow.COND_EXEC_END) || (opc == GPUControlFlow.COND_PRED_EXEC_END) || (opc == GPUControlFlow.COND_EXEC_PRED_CLEAN_END);

			        // evaluate condition
			        if ( (opc == GPUControlFlow.COND_EXEC_PRED_CLEAN) || (opc == GPUControlFlow.COND_EXEC_PRED_CLEAN_END) )
			        {
                        _Decompiled.Add( String.Format("P = BOOL[{0}]", instr.bool_addr ));
			         }

   			         // invert condition
                    String condition = "( P)";
			         if ( instr.pred_condition != 0 )
				             condition = "(!P)";

                    EmitExec( condition, instr, words);
                    break;
                }

                case GPUControlFlow.COND_CALL:
	            case GPUControlFlow.COND_JMP:
                    break;

	            case GPUControlFlow.LOOP_START:
	            case GPUControlFlow.LOOP_END:
	            case GPUControlFlow.RETURN:
                    _Decompiled.Add( String.Format("UNKNOWN CF: 0x{0:X}", (int)opc ));
                    break;

                case GPUControlFlow.MARK_VS_FETCH_DONE:
                    _Decompiled.Add("VFETCH DONE");
                    break;

            }

            return end;
        }

        private void EmitNOP()
        {
            _Decompiled.Add("NOP");
        }

        private void EmitAlloc(GPUInstrCfAlloc instr)
        {
            if (instr.buffer_select == 1)
            {
                EmitExportAllocPosition();
            }
            else if (instr.buffer_select == 2)
            {
                UInt32 count = 1 + instr.size;
                EmitExportAllocParam(count);
            }
            else if (instr.buffer_select == 3)
            {
                UInt32 count = 1 + instr.size;
                EmitExportAllocMemExport(count);
            }
        }

        private void EmitExportAllocPosition()
        {
            _Decompiled.Add("Alloc(POSITION);");
        }

        private void EmitExportAllocParam( UInt32 size )
        {
            _Decompiled.Add(String.Format("Alloc(PARAMETERS);", size));
        }

        private void EmitExportAllocMemExport(UInt32 size)
        {
            _Decompiled.Add(String.Format("Alloc(MEMEXPORT);", size));
        }

        private string GetCondition(string condition, UInt32 cf_pred_condition, UInt32 instr_pred_select, UInt32 instr_pred_condition)
        {
            var bIsConditional = (condition != "");
            if ((0 != instr_pred_select) && (!bIsConditional || cf_pred_condition != instr_pred_condition))
            {
                // invert condition
                if (0 == instr_pred_condition)
                    return "( P)";
                else
                    return "(!P)";
            }
            else
            {
                // no change
                return condition;
            }
        }

        private void EmitExec(String condition, GPUInstrCfExec instr, UInt32[] words)
        {
	        UInt32 sequence = instr.serialize;
            UInt32 cf_pred_condition = instr.pred_condition;

        	for ( UInt32 i=0; i<instr.count; i++ )
	        {
                UInt32 aluOffset = (instr.address + i);

		        UInt32 seqCode = sequence >> (int)(i*2);
		        bool bFetch = (0 != (seqCode & 0x1));
		        bool bSync = (0 != (seqCode & 0x2));

                UInt32 dword0 = words[ aluOffset*3 + 0 ];
                UInt32 dword1 = words[ aluOffset*3 + 1 ];
                UInt32 dword2 = words[ aluOffset*3 + 2 ];
		
		        if ( bFetch )
		        {
                    //GPUFetchOpcode opc = (GPUFetchOpcode)((dword0 >> 27) & 0x1F);
                    GPUFetchOpcode opc = (GPUFetchOpcode)((dword0 >> 0) & 0x1F);

			        switch ( opc )
			        {
				        case GPUFetchOpcode.VTX_FETCH:
				        {
                            GPUInstrFetchVtx vfetch = new GPUInstrFetchVtx ( dword0, dword1, dword2 );
                            string cnd = GetCondition( condition, cf_pred_condition, vfetch.pred_select, vfetch.pred_condition );
                            EmitVertexFetch( cnd, vfetch, bSync );
                            break;
                        }

                        case GPUFetchOpcode.TEX_FETCH:
                        {
                            GPUInstrFetchTex tfetch = new GPUInstrFetchTex( dword0, dword1, dword2 );
                            string cnd = GetCondition( condition, cf_pred_condition, tfetch.pred_select, tfetch.pred_condition );
                            EmitTextureFetch( cnd, tfetch, bSync );
                            break;
                        }

				        case GPUFetchOpcode.TEX_GET_BORDER_COLOR_FRAC:
				        case GPUFetchOpcode.TEX_GET_COMP_TEX_LOD:
				        case GPUFetchOpcode.TEX_GET_GRADIENTS:
				        case GPUFetchOpcode.TEX_GET_WEIGHTS:
				        case GPUFetchOpcode.TEX_SET_TEX_LOD:
				        case GPUFetchOpcode.TEX_SET_GRADIENTS_H:
				        case GPUFetchOpcode.TEX_SET_GRADIENTS_V:
                            _Decompiled.Add( String.Format("INVALID TEXTURE FETCH {0}", (UInt32) opc ) );
                            break;
                    }
                }
                else
                {
                    GPUInstrALU alu = new GPUInstrALU( dword0, dword1, dword2 );
                    string cnd = GetCondition( condition, cf_pred_condition, alu.pred_select, alu.pred_condition );
                    EmitALU(cnd, alu, bSync);
		        }
	        }
        }

        private static bool IsFetchFormatFloat( GPUFetchFormat format )
	    {
		    switch ( format )
		    {
                case GPUFetchFormat.FMT_32_FLOAT: return true;
                case GPUFetchFormat.FMT_32_32_FLOAT: return true;
                case GPUFetchFormat.FMT_32_32_32_32_FLOAT: return true;
                case GPUFetchFormat.FMT_32_32_32_FLOAT: return true;
		    }

		    return false;
	    }

        private void EmitVertexFetch(String cnd, GPUInstrFetchVtx vtx, bool bSync)
        {
            // fetch slot and format
	        var fetchSlot = vtx.const_index * 3 + vtx.const_index_sel;
	        var fetchOffset = vtx.offset;
	        var fetchStride = (0 != vtx.stride) ? vtx.stride : _LastVertexStride;
	        var fetchFormat = (GPUFetchFormat) vtx.format;

    	    // update vertex stride (for vfetch + vfetch_mini combo)
	        if ( 0 != vtx.stride )
                _LastVertexStride = vtx.stride; 
            else 
                vtx.stride = _LastVertexStride;

	        // fetch formats
	        var isFloat = IsFetchFormatFloat( fetchFormat );
	        var isSigned = (vtx.format_comp_all != 0);
	        var isNormalized = (0 == vtx.num_format_all);

            // determine data type
            GPUFetchDataType dataType;
            if (isFloat)
            {
                dataType = GPUFetchDataType.FLOAT;
            }
            else if (isSigned)
            {
                dataType = isNormalized ? GPUFetchDataType.SNORM : GPUFetchDataType.SINT;
            }
            else
            {
                dataType = isNormalized ? GPUFetchDataType.UNORM : GPUFetchDataType.UINT;
            }

            // prepare fetch info
            GPUVertexFetch fetchInfo = new GPUVertexFetch ();
            fetchInfo.format = fetchFormat;
            fetchInfo.dataType = dataType;
            fetchInfo.offset = fetchOffset;
            fetchInfo.stride = fetchStride;
            fetchInfo.slot = fetchSlot;
            _VertexFetches.Add(fetchInfo);

	        // get the source register
	        var sourceReg = String.Format( "R{0}", vtx.src_reg );
	
	        // create the value fetcher (returns single expression with fetch result)
            var fetchedData = String.Format("VFETCH( SLOT={0}, STRIDE={1}, FORMAT={2} )", 
                fetchInfo.slot, fetchInfo.stride, fetchInfo.ToFormatStr() );

	        // destination register
	        var destReg = EmitWriteReg( false, vtx.dst_reg );

        	// setup target swizzle
	        GPUSwizzle[] swz = { GPUSwizzle.DONTCARE, GPUSwizzle.DONTCARE, GPUSwizzle.DONTCARE, GPUSwizzle.DONTCARE };
	        for ( int i=0; i<4; i++)
	        {
		        int swizMask = (int) ((vtx.dst_swiz >> (3*i)) & 0x7);

		        if ( swizMask == 4 )
			        swz[i] = GPUSwizzle.ZERO;
		        else if ( swizMask == 5 )
			        swz[i] = GPUSwizzle.ONE;
		        else if ( swizMask == 6 )
			        swz[i] = GPUSwizzle.DONTCARE; // we don't have to do anything
		        else if ( swizMask == 7 )
                    swz[i] = GPUSwizzle.NOTUSED; // do not override
		        else if ( swizMask < 4 )
			        swz[i] = (GPUSwizzle) swizMask;
	        }

	        // copy data
	        _Decompiled.Add(EmitWriteWithSwizzleStatement( destReg, fetchedData, swz[0], swz[1], swz[2], swz[3] ));
        }

        private void EmitTextureFetch(String cnd, GPUInstrFetchTex tex, bool bSync)
        {
            // destination register
        	var destReg = EmitWriteReg( false, tex.dst_reg );

	        // source register (texcoords)
	        var srcReg = String.Format( "R{0}", tex.src_reg );
	        var srcX = (GPUSwizzle)( (tex.src_swiz >> 0) & 3 );
	        var srcY = (GPUSwizzle)( (tex.src_swiz >> 2) & 3 );
	        var srcZ = (GPUSwizzle)( (tex.src_swiz >> 4) & 3 );
	        var srcW = (GPUSwizzle)( (tex.src_swiz >> 6) & 3 );
	        var srcRegSwiz = EmitReadSwizzle( srcReg, srcX, srcY, srcZ, srcW );

        	// sample the shit
	        var sample = "";
	        if ( tex.dimension == (UInt32) GPUTextureDimension.DIMENSION_1D )
	        {
		        sample = String.Format( "TEX1D( {0}, {1} );", srcRegSwiz, tex.const_idx );
        	}
            else if (tex.dimension == (UInt32)GPUTextureDimension.DIMENSION_2D)
	        {
                sample = String.Format("TEX2D( {0}, {1} );", srcRegSwiz, tex.const_idx);
            }
            else if (tex.dimension == (UInt32)GPUTextureDimension.DIMENSION_3D)
	        {
                sample = String.Format("TEX3D( {0}, {1} );", srcRegSwiz, tex.const_idx);
            }
            else if (tex.dimension == (UInt32)GPUTextureDimension.DIMENSION_CUBE)
	        {
                sample = String.Format("TEXCUBE( {0}, {1} );", srcRegSwiz, tex.const_idx);
            }

	        // write back swizzled
           var destX = (GPUSwizzle)((tex.dst_swiz >> 0) & 7);
           var destY = (GPUSwizzle)((tex.dst_swiz >> 3) & 7);
           var destZ = (GPUSwizzle)((tex.dst_swiz >> 6) & 7);
           var destW = (GPUSwizzle)((tex.dst_swiz >> 9) & 7);
            _Decompiled.Add(EmitWriteWithSwizzleStatement(destReg, sample, destX, destY, destZ, destW));

            // setup
            GPUTextureFetch info = new GPUTextureFetch();
            info.slot = tex.const_idx & 15;
            info.offset_x = (int)tex.offset_x - 16;
            info.offset_y = (int)tex.offset_y - 16;
            info.offset_z = (int)tex.offset_z - 16;
            info.lod_bias = (int)tex.lod_bias - 32;
            info.dimm = (GPUTextureDimension)tex.dimension;
            info.magFilter = (GPUTextureFilterMode)tex.mag_filter;
            info.minFilter = (GPUTextureFilterMode)tex.min_filter;
            info.mipFilter = (GPUTextureFilterMode)tex.mip_filter;
            info.sample_location = tex.sample_location;
            info.use_comp_lod = tex.use_comp_lod;
            info.use_reg_gradients = tex.use_reg_gradients;
            info.use_reg_lod = tex.use_reg_lod;
            info.vol_min_filter = tex.vol_min_filter;
            info.vol_mag_filter = tex.vol_mag_filter;
            info.anisoFilter = (GPUTextureAnisoMode)tex.aniso_filter;
            info.arbitraryFilter = (GPUTextureArbitraryFilterMode)tex.arbitrary_filter;
            _TextureFetches.Add(info);
        }

        private static UInt32 GetArgCount( GPUVectorALU instr )
        {
	        switch ( instr ) 
    	    {
		        case GPUVectorALU.ADDv: return 2;
		        case GPUVectorALU.MULv: return 2;
		        case GPUVectorALU.MAXv: return 2;
		        case GPUVectorALU.MINv: return 2;
		        case GPUVectorALU.SETEv: return 2;
		        case GPUVectorALU.SETGTv: return 2;
		        case GPUVectorALU.SETGTEv: return 2;
		        case GPUVectorALU.SETNEv: return 2;
		        case GPUVectorALU.FRACv: return 1;
		        case GPUVectorALU.TRUNCv: return 1;
		        case GPUVectorALU.FLOORv: return 1;
		        case GPUVectorALU.MULADDv: return 3;
		        case GPUVectorALU.CNDEv: return 3;
		        case GPUVectorALU.CNDGTEv: return 3;
		        case GPUVectorALU.CNDGTv: return 3;
		        case GPUVectorALU.DOT4v: return 2;
		        case GPUVectorALU.DOT3v: return 2;
		        case GPUVectorALU.DOT2ADDv: return 3;
		        case GPUVectorALU.CUBEv: return 2;
		        case GPUVectorALU.MAX4v: return 1;
		        case GPUVectorALU.PRED_SETE_PUSHv: return 2;
		        case GPUVectorALU.PRED_SETNE_PUSHv: return 2;
		        case GPUVectorALU.PRED_SETGT_PUSHv: return 2;
		        case GPUVectorALU.PRED_SETGTE_PUSHv: return 2;
		        case GPUVectorALU.KILLEv: return 2;
		        case GPUVectorALU.KILLGTv: return 2;
		        case GPUVectorALU.KILLGTEv: return 2;
		        case GPUVectorALU.KILLNEv: return 2;
		        case GPUVectorALU.DSTv: return 2;
		        case GPUVectorALU.MOVAv: return 1;
	        }       
	        return 0;
        }

        private static UInt32 GetArgCount( GPUScalarALU instr )
        {
	        switch ( instr ) 
	        {
                case GPUScalarALU.ADDs: return 1;
                case GPUScalarALU.ADD_PREVs: return 1;
                case GPUScalarALU.MULs: return 1;
                case GPUScalarALU.MUL_PREVs: return 1;
                case GPUScalarALU.MUL_PREV2s: return 1;
                case GPUScalarALU.MAXs: return 1;
                case GPUScalarALU.MINs: return 1;
                case GPUScalarALU.SETEs: return 1;
                case GPUScalarALU.SETGTs: return 1;
                case GPUScalarALU.SETGTEs: return 1;
                case GPUScalarALU.SETNEs: return 1;
                case GPUScalarALU.FRACs: return 1;
                case GPUScalarALU.TRUNCs: return 1;
                case GPUScalarALU.FLOORs: return 1;
                case GPUScalarALU.EXP_IEEE: return 1;
                case GPUScalarALU.LOG_CLAMP: return 1;
                case GPUScalarALU.LOG_IEEE: return 1;
                case GPUScalarALU.RECIP_CLAMP: return 1;
                case GPUScalarALU.RECIP_FF: return 1;
                case GPUScalarALU.RECIP_IEEE: return 1;
                case GPUScalarALU.RECIPSQ_CLAMP: return 1;
                case GPUScalarALU.RECIPSQ_FF: return 1;
                case GPUScalarALU.RECIPSQ_IEEE: return 1;
                case GPUScalarALU.MOVAs: return 1;
                case GPUScalarALU.MOVA_FLOORs: return 1;
                case GPUScalarALU.SUBs: return 1;
                case GPUScalarALU.SUB_PREVs: return 1;
                case GPUScalarALU.PRED_SETEs: return 1;
                case GPUScalarALU.PRED_SETNEs: return 1;
                case GPUScalarALU.PRED_SETGTs: return 1;
                case GPUScalarALU.PRED_SETGTEs: return 1;
                case GPUScalarALU.PRED_SET_INVs: return 1;
                case GPUScalarALU.PRED_SET_POPs: return 1;
                case GPUScalarALU.PRED_SET_CLRs: return 1;
                case GPUScalarALU.PRED_SET_RESTOREs: return 1;
                case GPUScalarALU.KILLEs: return 1;
                case GPUScalarALU.KILLGTs: return 1;
                case GPUScalarALU.KILLGTEs: return 1;
                case GPUScalarALU.KILLNEs: return 1;
                case GPUScalarALU.KILLONEs: return 1;
                case GPUScalarALU.SQRT_IEEE: return 1;
                case GPUScalarALU.MUL_CONST_0: return 2;
                case GPUScalarALU.MUL_CONST_1: return 2;
                case GPUScalarALU.ADD_CONST_0: return 2;
                case GPUScalarALU.ADD_CONST_1: return 2;
                case GPUScalarALU.SUB_CONST_0: return 2;
                case GPUScalarALU.SUB_CONST_1: return 2;
                case GPUScalarALU.SIN: return 1;
                case GPUScalarALU.COS: return 1;
                case GPUScalarALU.RETAIN_PREV: return 1;
	        }

	        return 0;
        }

        private static string GetSwizzleText(GPUSwizzle swiz)
        {
            switch (swiz)
            {
                case GPUSwizzle.X: return "x";
                case GPUSwizzle.Y: return "y";
                case GPUSwizzle.Z: return "z";
                case GPUSwizzle.W: return "w";
                case GPUSwizzle.ZERO: return "0";
                case GPUSwizzle.ONE: return "1";
                case GPUSwizzle.NOTUSED: return "_";
                case GPUSwizzle.DONTCARE: return "?";
            }

            return "";
        }

        private string EmitReadSwizzle( string txt, GPUSwizzle x, GPUSwizzle y, GPUSwizzle z, GPUSwizzle w )
        {
            string ret = txt;
            ret += ".";
            ret += GetSwizzleText( x );
            ret += GetSwizzleText( y );
            ret += GetSwizzleText( z );
            ret += GetSwizzleText( w );
            return ret;
        }

        private string EmitWriteSwizzle( string txt, GPUSwizzle x, GPUSwizzle y, GPUSwizzle z, GPUSwizzle w )
        {
            string ret = txt;
            ret += ".";
            ret += GetSwizzleText( x );
            ret += GetSwizzleText( y );
            ret += GetSwizzleText( z );
            ret += GetSwizzleText( w );
            return ret;
        }

        private string EmitSrcReg( GPUInstrALU op, UInt32 num, UInt32 type, UInt32 swiz, UInt32 negate, UInt32 constSlot )
        {
            string ret = "";

	        if (type != 0)
	        {
		        UInt32 regIndex = num & 0x7F;
		        ret = String.Format( "R{0}", regIndex );

		        // take absolute value
		        if ( 0 != (num & 0x80) )
			        ret = "ABS(" + ret + ")";
	        }
	        else
	        {
		        if ( (constSlot == 0 && op.const_0_rel_abs == 1) || (constSlot == 1 && op.const_1_rel_abs == 1))
		        {
			        if (op.relative_addr != 0)
			        {
                        ret = String.Format( "Const[A0 + {0}]", num );
			        }
			        else
			        {
                        ret = String.Format( "Const[A0]", num );
			        }				
		        }
		        else
		        {
                    ret = String.Format( "Const[{0}]", num );
                    _UsedRegisters.Add(num);
		        }
		
		        // take absolute value
		        if ( op.abs_constants != 0 )
			        ret = "ABS(" + ret + ")";
	        }

	        // negate the result
	        if ( negate != 0)
		        ret = "-" + ret;

	        // add swizzle select
	        if (swiz != 0)
	        {
		        // note that neutral (zero) swizzle pattern represents XYZW swizzle and the whole numbering is wrapped around
		        var x = (GPUSwizzle)( ((swiz >> 0) + 0) & 0x3 );
		        var y = (GPUSwizzle)( ((swiz >> 2) + 1) & 0x3 );
		        var z = (GPUSwizzle)( ((swiz >> 4) + 2) & 0x3 );
		        var w = (GPUSwizzle)( ((swiz >> 6) + 3) & 0x3 );
		        ret  = EmitReadSwizzle( ret, x, y, z, w );
	        }

	        // return sampled register
	        return ret;
        }

        private string EmitSrcReg( GPUInstrALU op, UInt32 argIndex )
        {
	        if ( argIndex == 0 )
	        {
		        UInt32 constSlot = 0;
		        return EmitSrcReg( op, op.src1_reg, op.src1_sel, op.src1_swiz, op.src1_reg_negate, constSlot );
	        }
	        else if ( argIndex == 1 )
	        {
		        UInt32 constSlot = (op.src1_sel!=0) ? 1U : 0U;
		        return EmitSrcReg( op, op.src2_reg, op.src2_sel, op.src2_swiz, op.src2_reg_negate, constSlot );
	        }
	        else if ( argIndex == 2 )
	        {
		        UInt32 constSlot = ((op.src1_sel!=0) || (op.src2_sel!=0)) ? 1U : 0U;
		        return EmitSrcReg( op, op.src3_reg, op.src3_sel, op.src3_swiz, op.src3_reg_negate, constSlot );
	        }
	        else
	        {
                return "<INVALID SRC>";
	        }
        }

        private UInt32 MakeSwizzle4( UInt32 x, UInt32  y, UInt32  z, UInt32  w )
        {
	        return ((x&3) << 0) | ((y&3) << 0) | ((z&3) << 0) | ((w&3) << 0);
        }

        private string EmitSrcScalarReg1( GPUInstrALU op )
        {
	        UInt32 constSlot = ((op.src1_sel!=0) || (op.src2_sel!=0)) ? 1U : 0U;
	        return EmitSrcReg( op, op.src3_reg, op.src3_sel, MakeSwizzle4( 0,0,0,0 ), op.src3_reg_negate, constSlot );
        }

        private string EmitSrcScalarReg2( GPUInstrALU op )
        {
	        UInt32 constSlot = ((op.src1_sel!=0) || (op.src2_sel!=0)) ? 1U : 0U;
	        return EmitSrcReg( op, op.src3_reg, op.src3_sel, MakeSwizzle4( 1,1,1,1 ), op.src3_reg_negate, constSlot );
        }

        private string EmitWriteReg( bool exported, UInt32 regIndex )
        {
            if ( exported && PixelShader )
	        {
       			if ( regIndex == 0 ) return "COLOR0";
       			else if ( regIndex == 0 ) return "COLOR1";
       			else if ( regIndex == 0 ) return "COLOR2";
       			else if ( regIndex == 0 ) return "COLOR3";
            }
            else if ( exported && !PixelShader )
            {
       			if ( regIndex == 62 ) return "POSITION";
       			else if ( regIndex == 63 ) return "POINTSIZE";
       			else if ( regIndex == 0 ) return "INTERP0";
       			else if ( regIndex == 1 ) return "INTERP1";
       			else if ( regIndex == 2 ) return "INTERP2";
       			else if ( regIndex == 3 ) return "INTERP3";
       			else if ( regIndex == 4 ) return "INTERP4";
       			else if ( regIndex == 5 ) return "INTERP5";
       			else if ( regIndex == 6 ) return "INTERP6";
       			else if ( regIndex == 7 ) return "INTERP7";
            }

            if ( exported )
                return String.Format( "EXPORT{0}", regIndex );

            return String.Format( "R{0}", regIndex );
        }

        private string EmitVectorResult( GPUInstrALU op, string code )
        {
	        // clamp value to 0-1 range
	        if ( 0 != op.vector_clamp )
		        code = "SATURATE(" + code + ")";

	        // get name of destination register (the target)
	        bool bExported = (op.export_data != 0);
	        string dest = EmitWriteReg( bExported, op.vector_dest ); // returns Rnum or exportPosition, etc, just the name

	        // prepare write swizzle
	        GPUSwizzle[] swz = { GPUSwizzle.NOTUSED, GPUSwizzle.NOTUSED, GPUSwizzle.NOTUSED, GPUSwizzle.NOTUSED };
	        if ( 0 != op.export_data )
	        {
		        // For exported values we can have forced 1 and 0 appear in the mask
		        UInt32 writeMask = op.vector_write_mask;
		        UInt32 const1Mask = op.scalar_write_mask;
		        for ( int i=0; i<4; ++i )
		        {
			        int channelMask = 1 << i;
			        if ( 0 != (writeMask & channelMask) )
			        {
				        if (0 != (const1Mask & channelMask) )
					        swz[i] = GPUSwizzle.ONE; // write 1.0 in the target
				        else
					        swz[i] = (GPUSwizzle)i; // same component
			        }
			        else if ( 0 != op.scalar_dest_rel )
			        {
				        swz[i] = GPUSwizzle.ZERO; // write 0.0 in the target, normaly we would not write anything but it's an export
			        }
		        }
	        }
	        else
	        {
		        // Normal swizzle
		        UInt32 writeMask = op.vector_write_mask;
		        for ( int i=0; i<4; ++i )
		        {
			        int channelMask = 1 << i;
			        if ( 0 != (writeMask & channelMask) )
				        swz[i] = (GPUSwizzle)i; // same component
			        else
				        swz[i] = GPUSwizzle.NOTUSED; // do not write
		        }
	        }

	        // having the input code and the swizzle mask return the final output code
	        return EmitWriteWithSwizzleStatement( dest, code, swz[0], swz[1], swz[2], swz[3] );	
        }

        private string EmitWriteWithSwizzleStatement( string dest, string src, GPUSwizzle x, GPUSwizzle y, GPUSwizzle z, GPUSwizzle w )
        {
            return EmitWriteSwizzle(dest, x,y,z,w) + " = " + src;
        }

        private string EmitScalarResult( GPUInstrALU op, string code ) 
        {
	        // clamp value to 0-1 range
	        if ( 0 != op.vector_clamp )
		        code = "SATURATE(" + code + ")";

	        // select destination
	        string dest;
	        UInt32 writeMask;
	        if ( 0 != op.export_data )
	        {
		        // during export scalar operation can still write into the vector
		        dest = EmitWriteReg( true, op.vector_dest );
		        writeMask = op.scalar_write_mask & ~op.vector_write_mask;
	        }
	        else
	        {
		        // pure scalar op
		        dest = EmitWriteReg( false, op.scalar_dest );
		        writeMask = op.scalar_write_mask;
	        }

	        // prepare write swizzle
	        GPUSwizzle[] swz = { GPUSwizzle.NOTUSED, GPUSwizzle.NOTUSED, GPUSwizzle.NOTUSED, GPUSwizzle.NOTUSED };
	        for ( int i=0; i<4; ++i )
	        {
		        int channelMask = 1 << i;
                if (0 != (writeMask & channelMask))
                    swz[i] = (GPUSwizzle)i; // same component
                else
                    swz[i] = GPUSwizzle.NOTUSED;// do not write
	        }

	        // emit output
	        return EmitWriteWithSwizzleStatement( dest, code, swz[0], swz[1], swz[2], swz[3] );	
        }

        private string EmitVectorInstruction1(GPUVectorALU op, string arg1)
        {
            string opName = Enum.GetName( typeof(GPUVectorALU), op );
            return opName + "(" + arg1 + ")";
        }

        private string EmitVectorInstruction2(GPUVectorALU op, string arg1, string arg2)
        {
            string opName = Enum.GetName(typeof(GPUVectorALU), op);
            return opName + "(" + arg1 + "," + arg2 + ")";
        }

        private string EmitVectorInstruction3(GPUVectorALU op, string arg1, string arg2, string arg3)
        {
            string opName = Enum.GetName(typeof(GPUVectorALU), op);
            return opName + "(" + arg1 + "," + arg2 + "," + arg3 + ")";
        }

        private string EmitScalarInstruction1(GPUScalarALU op, string arg1)
        {
            string opName = Enum.GetName(typeof(GPUScalarALU), op);
            return opName + "(" + arg1 + ")";
        }

        private string EmitScalarInstruction2(GPUScalarALU op, string arg1, string arg2)
        {
            string opName = Enum.GetName(typeof(GPUScalarALU), op);
            return opName + "(" + arg1 + "," + arg2 + ")";
        }

        private string EmitScalarInstruction3(GPUScalarALU op, string arg1, string arg2, string arg3)
        {
            string opName = Enum.GetName(typeof(GPUScalarALU), op);
            return opName + "(" + arg1 + "," + arg2 + "," + arg3 + ")";
        }

        private void EmitALU(String cnd, GPUInstrALU alu, bool bSync)
        {
            string vectorPart="", scalarPart="", predPart="";

	        // Fast case - no magic stuff around
	        if ( (0 != alu.vector_write_mask) || ((0 != alu.export_data) && (0 != alu.scalar_dest_rel)) )
	        {
		        var vectorInstr = (GPUVectorALU) alu.vector_opc;
		        var argCount = GetArgCount( vectorInstr );

		        // process function depending on the argument count
		        if ( argCount == 1 )
		        {
			        var arg1 = EmitSrcReg( alu, 0 );
			        var func = EmitVectorInstruction1( vectorInstr, arg1 );
			        vectorPart = EmitVectorResult( alu, func );
		        }
		        else if ( argCount == 2 )
		        {
			        var arg1 = EmitSrcReg( alu, 0 );
			        var arg2 = EmitSrcReg( alu, 1 );
			        var func = EmitVectorInstruction2( vectorInstr, arg1, arg2 );
			        vectorPart = EmitVectorResult( alu, func );
		        }
		        else if ( argCount == 3 )
		        {
			        var arg1 = EmitSrcReg( alu, 0 );
			        var arg2 = EmitSrcReg( alu, 1 );
			        var arg3 = EmitSrcReg( alu, 2 );
			        var func = EmitVectorInstruction3( vectorInstr, arg1, arg2, arg3 );
			        vectorPart = EmitVectorResult( alu, func );
		        }
	        }

	        // Additional scalar instruction
	        if ( (0!=alu.scalar_write_mask) || (0 == alu.vector_write_mask) )
	        {
		        var scalarInstr = (GPUScalarALU) alu.scalar_opc;
		        var argCount = GetArgCount( scalarInstr );

		        // process function depending on the argument count
		        if ( argCount == 1 )
		        {
			        var arg1 = EmitSrcReg( alu, 2 );
			        var func = EmitScalarInstruction1( scalarInstr, arg1 );

			        if ( scalarInstr == GPUScalarALU.PRED_SETNEs || scalarInstr == GPUScalarALU.PRED_SETEs || scalarInstr == GPUScalarALU.PRED_SETGTEs || scalarInstr == GPUScalarALU.PRED_SETGTs )
			        {                        
				        predPart = "SetPredicate(" + func + ")";
			        }


			        scalarPart = EmitScalarResult( alu, func );
		        }
		        else if ( argCount == 2 )
		        {
			        string arg1, arg2;
			        if ( scalarInstr == GPUScalarALU.MUL_CONST_0 || scalarInstr == GPUScalarALU.MUL_CONST_1 || scalarInstr == GPUScalarALU.ADD_CONST_0 || scalarInstr == GPUScalarALU.ADD_CONST_1 || scalarInstr == GPUScalarALU.SUB_CONST_0 || scalarInstr == GPUScalarALU.SUB_CONST_1 )
			        {
				        var src3 = alu.src3_swiz & ~0x3C;
				        GPUSwizzle swizA = (GPUSwizzle )( ((src3 >> 6) - 1) & 0x3 );
				        GPUSwizzle swizB = (GPUSwizzle )( (src3 & 0x3) );

				        arg1 = EmitReadSwizzle( EmitSrcReg( alu, alu.src3_reg, 0, 0, alu.src3_reg_negate, 0 ), swizA, swizA, swizA, swizA );

				        var regB = (alu.scalar_opc & 1) | (alu.src3_swiz & 0x3C) | (alu.src3_sel << 1);
				        UInt32 const_slot = ((0 != alu.src1_sel) || (0 != alu.src2_sel)) ? 1U : 0U;

				        arg2 = EmitReadSwizzle( EmitSrcReg( alu, regB, 1, 0, 0, const_slot ), swizB, swizB, swizB, swizB );
			        }
			        else
			        {
				        arg1 = EmitSrcReg( alu, 0 );
				        arg2 = EmitSrcReg( alu, 1 );
			        }

			        var func = EmitScalarInstruction2( scalarInstr, arg1, arg2 );
			        scalarPart = EmitScalarResult( alu, func );
		        }
	        }

            if (predPart != "")
                _Decompiled.Add(cnd + predPart + ";");

            if (vectorPart != "")
                _Decompiled.Add(cnd + vectorPart + ";");

            if (scalarPart != "")
                _Decompiled.Add(cnd + scalarPart + ";");
        }
    }
}
