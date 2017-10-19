#pragma once

//---------------------------------------------------------------------------

#include "../recompiler_core/build.h"
#include "../recompiler_core/platformCPU.h"
#include "../recompiler_core/decodingInstruction.h"
#include "../recompiler_core/decodingInstructionInfo.h"

//---------------------------------------------------------------------------

class IInstructiondDecompilerXenon : public platform::CPUInstructionNativeDecompiler
{
public:
	virtual ~IInstructiondDecompilerXenon(){};

	// platform::CPUInstructionNativeDecompiler interface implementation
	virtual bool GetExtendedText(const class decoding::Instruction& instr, const uint64 codeAddress, char* outText, const uint32 outTextSize) const override;
	virtual bool GetCommentText(const class decoding::Instruction& instr, const uint64 codeAddress, char* outText, const uint32 outTextSize) const override;

	// Decompile instruction
	virtual const bool Decompile(class ILogOutput& output, const uint32 address, const decoding::Instruction& op, const decoding::InstructionExtendedInfo& exInfo, class code::IGenerator& codeGen, std::string& outCode) const = 0;

public:
	const bool CheckArgs(const class decoding::Instruction& instr, decoding::Instruction::Type arg0 = decoding::Instruction::eType_None, decoding::Instruction::Type arg1 = decoding::Instruction::eType_None, decoding::Instruction::Type arg2 = decoding::Instruction::eType_None, decoding::Instruction::Type arg3 = decoding::Instruction::eType_None, decoding::Instruction::Type arg4 = decoding::Instruction::eType_None, decoding::Instruction::Type arg5 = decoding::Instruction::eType_None) const;
	const bool ErrInvalidMode(const class decoding::Instruction& instr) const;
	const bool ErrInvalidArgs(const class decoding::Instruction& instr) const;
	const bool ErrToManyRegisters(const class decoding::Instruction& instr) const;
};

//---------------------------------------------------------------------------

struct MemReg
{
	const platform::CPURegister* m_reg;
	const platform::CPURegister* m_index;
	uint32						m_offset;

	inline MemReg(const platform::CPURegister* reg, const uint32 offset)
		: m_reg(reg)
		, m_offset(offset)
		, m_index(NULL)
	{}

	inline MemReg(const platform::CPURegister* reg, const platform::CPURegister* index)
		: m_reg(reg)
		, m_index(index)
		, m_offset(0)
	{}

	inline MemReg(const uint32 offset)
		: m_reg(NULL)
		, m_index(NULL)
		, m_offset(offset)
	{}
};

//---------------------------------------------------------------------------

class CPU_XenonPPC : public platform::CPU, public platform::CPUInstructionNativeDecoder
{
public:
	enum ERegister
	{
		eRegister_LR,
		eRegister_CTR,
		eRegister_MSR,
		eRegister_CR,

		eRegister_XER,
		eRegister_XER_OV,
		eRegister_XER_CA,
		eRegister_XER_SO,

		eRegister_CR0,
		eRegister_CR1,
		eRegister_CR2,
		eRegister_CR3,
		eRegister_CR4,
		eRegister_CR5,
		eRegister_CR6,
		eRegister_CR7,

		eRegister_CR0_LT,
		eRegister_CR0_GT,
		eRegister_CR0_EQ,
		eRegister_CR0_SO,

		eRegister_CR1_LT,
		eRegister_CR1_GT,
		eRegister_CR1_EQ,
		eRegister_CR1_SO,

		eRegister_CR2_LT,
		eRegister_CR2_GT,
		eRegister_CR2_EQ,
		eRegister_CR2_SO,

		eRegister_CR3_LT,
		eRegister_CR3_GT,
		eRegister_CR3_EQ,
		eRegister_CR3_SO,

		eRegister_CR4_LT,
		eRegister_CR4_GT,
		eRegister_CR4_EQ,
		eRegister_CR4_SO,

		eRegister_CR5_LT,
		eRegister_CR5_GT,
		eRegister_CR5_EQ,
		eRegister_CR5_SO,

		eRegister_CR6_LT,
		eRegister_CR6_GT,
		eRegister_CR6_EQ,
		eRegister_CR6_SO,

		eRegister_CR7_LT,
		eRegister_CR7_GT,
		eRegister_CR7_EQ,		
		eRegister_CR7_SO,

		eRegister_FPSCR,

		eRegister_FPSCR0,
		eRegister_FPSCR1,
		eRegister_FPSCR2,
		eRegister_FPSCR3,
		eRegister_FPSCR4,
		eRegister_FPSCR5,
		eRegister_FPSCR6,
		eRegister_FPSCR7,

		eRegister_FPSCR0_FX,		//  0: Floating-Point Exception Summary
		eRegister_FPSCR0_FEX,		//  1: Floating-Point Enabled Exception Summary
		eRegister_FPSCR0_VX,		//  2: Floating-Point Invalid Operation Exception Summary
		eRegister_FPSCR0_OX,		//  3: Floating-Point Overflow Exception
		eRegister_FPSCR1_UX,		//  4: Floating-Point Underflow Exception
		eRegister_FPSCR1_ZX,		//  5: Floating-Point Zero Divide Exception
		eRegister_FPSCR1_XX,		//  6: Floating-Point Inexact Exception
		eRegister_FPSCR1_VXSNAN,	//  7: Floating-Point Invalid Operation Exception (SNaN)
		eRegister_FPSCR2_VXISI,	//  8: Floating-Point Invalid Operation Exception (inf-inf)
		eRegister_FPSCR2_VXIDI,	//  9: Floating-Point Invalid Operation Exception (inf/inf)
		eRegister_FPSCR2_VXZDZ,	// 10: Floating-Point Invalid Operation Exception (0/0)
		eRegister_FPSCR2_VXIMZ,	// 11: Floating-Point Invalid Operation Exception (inf*0)
		eRegister_FPSCR3_VXVC,	// 12: Floating-Point Invalid Operation Exception (invalid compare)
		eRegister_FPSCR3_FR,		// 13: Floating-Point Fraction Rounded
		eRegister_FPSCR3_FI,		// 14: Floating-Point Fraction Inexact
		eRegister_FPSCR3_C,		// 15: Floating-Point Result Class Descriptor (C)
		eRegister_FPSCR4_FL,		// 16: Floating-Point Less Than or Negative (LT)
		eRegister_FPSCR4_FG,		// 17: Floating-Point Greater Than or Positive (GT)
		eRegister_FPSCR4_FE,		// 18: Floating-Point Equal or Zero
		eRegister_FPSCR4_FU,		// 19: Floating-Point Unordered or NaN
		eRegister_FPSCR5_RES,	// 20: Reserved
		eRegister_FPSCR5_VXSOFT,	// 21: Floating-Point Invalid Operation Exception (Software)
		eRegister_FPSCR5_VXSQRT,	// 22: Floating-Point Invalid Operation Exception (Invalid Square Root)
		eRegister_FPSCR5_VXCVI,	// 23: Floating-Point Invalid Operation Exception (Invalid Integer Convert)
		eRegister_FPSCR6_VE,		// 24: Floating-Point Invalid Operation Exception Enable
		eRegister_FPSCR6_VO,		// 25: Floating-Point Overflow Exception Enable
		eRegister_FPSCR6_UE,		// 26: Floating-Point Underflow Exception Enable
		eRegister_FPSCR6_ZE,		// 27: Floating-Point Zero Divide Exception Enable
		eRegister_FPSCR7_XE,		// 28: Floating-Point Inexact Exception Enable
		eRegister_FPSCR7_NI,		// 29: Floating-Point Non-IEEE Mode
		eRegister_FPSCR7_RN0,		// 30: Floating-Point Rounding Control
		eRegister_FPSCR7_RN1,		// 31: Floating-Point Rounding Control

		eRegister_R0,
		eRegister_R1,
		eRegister_R2,
		eRegister_R3,
		eRegister_R4,
		eRegister_R5,
		eRegister_R6,
		eRegister_R7,
		eRegister_R8,
		eRegister_R9,
		eRegister_R10,
		eRegister_R11,
		eRegister_R12,
		eRegister_R13,
		eRegister_R14,
		eRegister_R15,
		eRegister_R16,
		eRegister_R17,
		eRegister_R18,
		eRegister_R19,
		eRegister_R20,
		eRegister_R21,
		eRegister_R22,
		eRegister_R23,
		eRegister_R24,
		eRegister_R25,
		eRegister_R26,
		eRegister_R27,
		eRegister_R28,
		eRegister_R29,
		eRegister_R30,
		eRegister_R31,

		eRegister_FR0,
		eRegister_FR1,
		eRegister_FR2,
		eRegister_FR3,
		eRegister_FR4,
		eRegister_FR5,
		eRegister_FR6,
		eRegister_FR7,
		eRegister_FR8,
		eRegister_FR9,
		eRegister_FR10,
		eRegister_FR11,
		eRegister_FR12,
		eRegister_FR13,
		eRegister_FR14,
		eRegister_FR15,
		eRegister_FR16,
		eRegister_FR17,
		eRegister_FR18,
		eRegister_FR19,
		eRegister_FR20,
		eRegister_FR21,
		eRegister_FR22,
		eRegister_FR23,
		eRegister_FR24,
		eRegister_FR25,
		eRegister_FR26,
		eRegister_FR27,
		eRegister_FR28,
		eRegister_FR29,
		eRegister_FR30,
		eRegister_FR31,
		eRegister_FR32,

		eRegister_VR0,
		eRegister_VR1,
		eRegister_VR2,
		eRegister_VR3,
		eRegister_VR4,
		eRegister_VR5,
		eRegister_VR6,
		eRegister_VR7,
		eRegister_VR8,
		eRegister_VR9,
		eRegister_VR10,
		eRegister_VR11,
		eRegister_VR12,
		eRegister_VR13,
		eRegister_VR14,
		eRegister_VR15,
		eRegister_VR16,
		eRegister_VR17,
		eRegister_VR18,
		eRegister_VR19,
		eRegister_VR20,
		eRegister_VR21,
		eRegister_VR22,
		eRegister_VR23,
		eRegister_VR24,
		eRegister_VR25,
		eRegister_VR26,
		eRegister_VR27,
		eRegister_VR28,
		eRegister_VR29,
		eRegister_VR30,
		eRegister_VR31,
		eRegister_VR32,
		eRegister_VR33,
		eRegister_VR34,
		eRegister_VR35,
		eRegister_VR36,
		eRegister_VR37,
		eRegister_VR38,
		eRegister_VR39,
		eRegister_VR40,
		eRegister_VR41,
		eRegister_VR42,
		eRegister_VR43,
		eRegister_VR44,
		eRegister_VR45,
		eRegister_VR46,
		eRegister_VR47,
		eRegister_VR48,
		eRegister_VR49,
		eRegister_VR50,
		eRegister_VR51,
		eRegister_VR52,
		eRegister_VR53,
		eRegister_VR54,
		eRegister_VR55,
		eRegister_VR56,
		eRegister_VR57,
		eRegister_VR58,
		eRegister_VR59,
		eRegister_VR60,
		eRegister_VR61,
		eRegister_VR62,
		eRegister_VR63,
		eRegister_VR64,
		eRegister_VR65,
		eRegister_VR66,
		eRegister_VR67,
		eRegister_VR68,
		eRegister_VR69,
		eRegister_VR70,
		eRegister_VR71,
		eRegister_VR72,
		eRegister_VR73,
		eRegister_VR74,
		eRegister_VR75,
		eRegister_VR76,
		eRegister_VR77,
		eRegister_VR78,
		eRegister_VR79,
		eRegister_VR80,
		eRegister_VR81,
		eRegister_VR82,
		eRegister_VR83,
		eRegister_VR84,
		eRegister_VR85,
		eRegister_VR86,
		eRegister_VR87,
		eRegister_VR88,
		eRegister_VR89,
		eRegister_VR90,
		eRegister_VR91,
		eRegister_VR92,
		eRegister_VR93,
		eRegister_VR94,
		eRegister_VR95,
		eRegister_VR96,
		eRegister_VR97,
		eRegister_VR98,
		eRegister_VR99,
		eRegister_VR100,
		eRegister_VR101,
		eRegister_VR102,
		eRegister_VR103,
		eRegister_VR104,
		eRegister_VR105,
		eRegister_VR106,
		eRegister_VR107,
		eRegister_VR108,
		eRegister_VR109,
		eRegister_VR110,
		eRegister_VR111,
		eRegister_VR112,
		eRegister_VR113,
		eRegister_VR114,
		eRegister_VR115,
		eRegister_VR116,
		eRegister_VR117,
		eRegister_VR118,
		eRegister_VR119,
		eRegister_VR120,
		eRegister_VR121,
		eRegister_VR122,
		eRegister_VR123,
		eRegister_VR124,
		eRegister_VR125,
		eRegister_VR126,
		eRegister_VR127,

		eRegister_MAX,
	};

	enum EInstruction
	{
		eInstruction_nop,

		// move register (with move to/from special registers)
		eInstruction_mr,
		eInstruction_mfspr,
		eInstruction_mtspr,
		eInstruction_mfmsr,
		eInstruction_mtmsrd,
		eInstruction_mtmsree,
		eInstruction_mfcr,
		eInstruction_mtcrf,
		eInstruction_mfocrf,
		eInstruction_mtocrf,
		eInstruction_mcrf,
		eInstruction_mftb,

		// load memory
		eInstruction_lbz,
		eInstruction_lhz,
		eInstruction_lwz,
		eInstruction_lbzu,
		eInstruction_lhzu,
		eInstruction_lwzu,
		eInstruction_lba,
		eInstruction_lha,
		eInstruction_lwa,
		eInstruction_lbau,
		eInstruction_lhau,
		eInstruction_lwau,
		eInstruction_ld,
		eInstruction_ldu,

		// load memory indexed
		eInstruction_lbzx,
		eInstruction_lhzx,
		eInstruction_lwzx,
		eInstruction_lbzux,
		eInstruction_lhzux,
		eInstruction_lwzux,
		eInstruction_lbax,
		eInstruction_lhax,
		eInstruction_lwax,
		eInstruction_lbaux,
		eInstruction_lhaux,
		eInstruction_lwaux,
		eInstruction_ldx,
		eInstruction_ldux,

		// store memory
		eInstruction_stb,
		eInstruction_sth,
		eInstruction_stw,
		eInstruction_std,
		eInstruction_stbu,
		eInstruction_sthu,
		eInstruction_stwu,
		eInstruction_stdu,

		// store memory indexed
		eInstruction_stbx,
		eInstruction_sthx,
		eInstruction_stwx,
		eInstruction_stdx,
		eInstruction_stbux,
		eInstruction_sthux,
		eInstruction_stwux,
		eInstruction_stdux,

		// branch instructions
		eInstruction_b,
		eInstruction_ba,
		eInstruction_bl,
		eInstruction_bla,
		eInstruction_bc,
		eInstruction_bcl,
		eInstruction_bca,
		eInstruction_bcla,
		eInstruction_bclr,
		eInstruction_bclrl,
		eInstruction_bcctr,
		eInstruction_bcctrl,
	
		// lwarx/stwcx
		eInstruction_lwarx,
		eInstruction_ldarx,
		eInstruction_stwcxRC, //.
		eInstruction_stdcxRC, //.

		// memory bariers
		eInstruction_sync,
		eInstruction_lwsync,
		eInstruction_ptesync,
		eInstruction_eieio,

		// custom
		//eInstruction_cmp,
		eInstruction_fcmpu,
		eInstruction_fcmpo,
		eInstruction_twi,
		eInstruction_tdi,
		eInstruction_tw,
		eInstruction_td,

		// generic instructions
		eInstruction_sc,

		// condition register operators
		eInstruction_crand,
		eInstruction_cror,
		eInstruction_crxor,
		eInstruction_crnand,
		eInstruction_crnor,
		eInstruction_creqv,
		eInstruction_crandc,
		eInstruction_crorc,

		// load immediate
		eInstruction_li,
		eInstruction_lis,
	
		// math with immediate value
		eInstruction_addic,
		eInstruction_addicRC,
		eInstruction_addi,
		eInstruction_addis,
		eInstruction_subfic,
		eInstruction_mulli,

		// add XO-form
		eInstruction_add,
		eInstruction_addRC,
		eInstruction_addo,
		eInstruction_addoRC,

		// subtract grom XO-form
		eInstruction_subf,
		eInstruction_subfRC,
		eInstruction_subfo,
		eInstruction_subfoRC,

		// add carrying XO-form
		eInstruction_addc,
		eInstruction_addcRC,
		eInstruction_addco,
		eInstruction_addcoRC,

		// subtract From carrying XO-form
		eInstruction_subfc,
		eInstruction_subfcRC,
		eInstruction_subfco,
		eInstruction_subfcoRC,

		// add extended XO-form
		eInstruction_adde,
		eInstruction_addeRC,
		eInstruction_addeo,
		eInstruction_addeoRC,

		// subtract from extended XO-form
		eInstruction_subfe,
		eInstruction_subfeRC,
		eInstruction_subfeo,
		eInstruction_subfeoRC,

		// add to minus one extended XO-form
		eInstruction_addme,
		eInstruction_addmeRC,
		eInstruction_addmeo,
		eInstruction_addmeoRC,

		// subtract from minus one extended XO-form
		eInstruction_subfme,
		eInstruction_subfmeRC,
		eInstruction_subfmeo,
		eInstruction_subfmeoRC,
	
		// add to zero extended XO-form
		eInstruction_addze,
		eInstruction_addzeRC,
		eInstruction_addzeo,
		eInstruction_addzeoRC,

		// subtract from zero extended XO-form
		eInstruction_subfze,
		eInstruction_subfzeRC,
		eInstruction_subfzeo,
		eInstruction_subfzeoRC,

		// negate
		eInstruction_neg,
		eInstruction_negRC,
		eInstruction_nego,
		eInstruction_negoRC,

		// multiply low doubleword XO-form
		eInstruction_mulld,
		eInstruction_mulldRC,
		eInstruction_mulldo,
		eInstruction_mulldoRC,
	
		// multiply low word XO-form
		eInstruction_mullw,
		eInstruction_mullwRC,
		eInstruction_mullwo,
		eInstruction_mullwoRC,

		// multiply high doubleword XO-form
		eInstruction_mullhd,
		eInstruction_mullhdRC,

		// multiply High Word XO-form
		eInstruction_mullhw,
		eInstruction_mullhwRC,

		// multiply high doubleword unsigned XO-form
		eInstruction_mulhdu,
		eInstruction_mulhduRC,
	
		// multiply High Word Unsigned XO-form
		eInstruction_mulhwu,
		eInstruction_mulhwuRC,

		// divide doubleword XO-form
		eInstruction_divd,
		eInstruction_divdRC,
		eInstruction_divdo,
		eInstruction_divdoRC,

		// divide word XO-form
		eInstruction_divw,
		eInstruction_divwRC,
		eInstruction_divwo,
		eInstruction_divwoRC,

		// divide doubleword unsigned XO-form
		eInstruction_divdu,
		eInstruction_divduRC,
		eInstruction_divduo,
		eInstruction_divduoRC,

		// divide word unsigned XO-form
		eInstruction_divwu,
		eInstruction_divwuRC,
		eInstruction_divwuo,
		eInstruction_divwuoRC,
	
		// compare
		eInstruction_cmpwi, 
		eInstruction_cmpdi, 
		eInstruction_cmplwi,
		eInstruction_cmpldi,
		eInstruction_cmpw,
		eInstruction_cmpd,
		eInstruction_cmplw, 
		eInstruction_cmpld, 

		// logical instructions with immediate values
		eInstruction_andiRC,
		eInstruction_andisRC,
		eInstruction_ori,
		eInstruction_oris,
		eInstruction_xori,
		eInstruction_xoris, 

		// logical instructions
		eInstruction_and,
		eInstruction_andRC,
		eInstruction_or,
		eInstruction_orRC, 
		eInstruction_xor,
		eInstruction_xorRC,
		eInstruction_nand, 
		eInstruction_nandRC,
		eInstruction_nor,
		eInstruction_norRC,
		eInstruction_eqv,
		eInstruction_eqvRC,
		eInstruction_andc, 
		eInstruction_andcRC,
		eInstruction_orc,
		eInstruction_orcRC,

		// byte extend
		eInstruction_extsb,
		eInstruction_extsbRC,
		eInstruction_extsh,
		eInstruction_extshRC,
		eInstruction_extsw,
		eInstruction_extswRC,

		// population count
		eInstruction_popcntb,
		eInstruction_cntlzw,
		eInstruction_cntlzwRC,
		eInstruction_cntlzd,
		eInstruction_cntlzdRC,

		// Shift instructions with immediate params -->
		// REG,REG,IMM,IMM
		eInstruction_rldicl,
		eInstruction_rldiclRC,
		eInstruction_rldicr,
		eInstruction_rldicrRC,
		eInstruction_rldic,
		eInstruction_rldicRC,
		eInstruction_rldimi,
		eInstruction_rldimiRC,

		// REG,REG,IMM,IMM,IMM
		eInstruction_rlwinm, 
		eInstruction_rlwinmRC,
		eInstruction_rlwimi,
		eInstruction_rlwimiRC,

		// REG,REG,REG,IMM
		eInstruction_rldcl,
		eInstruction_rldclRC,
		eInstruction_rldcr,
		eInstruction_rldcrRC,

		// REG,REG,REG,IMM,IMM
		eInstruction_rlwnm,
		eInstruction_rlwnmRC,

		// Extended shifts
		eInstruction_sld,
		eInstruction_sldRC,
		eInstruction_slw,
		eInstruction_slwRC,
		eInstruction_srd,
		eInstruction_srdRC,
		eInstruction_srw,
		eInstruction_srwRC,
		eInstruction_srad,
		eInstruction_sradRC,
		eInstruction_sraw,
		eInstruction_srawRC,

		// Extended shifts with immediate... geez..
		eInstruction_srawi,
		eInstruction_srawiRC,
		eInstruction_sradi,
		eInstruction_sradiRC,

		// Cache operation instructions
		eInstruction_dcbf,
		eInstruction_dcbst,
		eInstruction_dcbt,
		eInstruction_dcbtst,
		eInstruction_dcbz,

		//  Floating point load instructions
		eInstruction_lfs,
		eInstruction_lfsu,
		eInstruction_lfsx,
		eInstruction_lfsux,
		eInstruction_lfd,
		eInstruction_lfdu,
		eInstruction_lfdx,
		eInstruction_lfdux,

		//  Floating point store instructions
		eInstruction_stfs,
		eInstruction_stfsu,
		eInstruction_stfsx,
		eInstruction_stfsux,
		eInstruction_stfd,
		eInstruction_stfdu,
		eInstruction_stfdx,
		eInstruction_stfdux,

		// Store float point as integer word indexed
		eInstruction_stfiwx,

		// Floating point move and single op instructions
		eInstruction_fmr,
		eInstruction_fmrRC,
		eInstruction_fneg,
		eInstruction_fnegRC,
		eInstruction_fabs,
		eInstruction_fabsRC,
		eInstruction_fnabs,
		eInstruction_fnabsRC,

		// Floating point arithmetic instructions
		eInstruction_fadd,
		eInstruction_faddRC,
		eInstruction_fadds,
		eInstruction_faddsRC,
		eInstruction_fsub,
		eInstruction_fsubRC,
		eInstruction_fsubs,
		eInstruction_fsubsRC,
		eInstruction_fmul,
		eInstruction_fmulRC,
		eInstruction_fmuls,
		eInstruction_fmulsRC,
		eInstruction_fdiv,
		eInstruction_fdivRC,
		eInstruction_fdivs,
		eInstruction_fdivsRC,
		eInstruction_fsqrt,
		eInstruction_fsqrtRC,
		eInstruction_frsqrtx,
		eInstruction_frsqrtxRC,
		eInstruction_fre,
		eInstruction_freRC,

		// Floating point mad instructions
		eInstruction_fmadd,
		eInstruction_fmaddRC,
		eInstruction_fmadds,
		eInstruction_fmaddsRC,
		eInstruction_fmsub,
		eInstruction_fmsubRC,
		eInstruction_fmsubs,
		eInstruction_fmsubsRC,
		eInstruction_fnmadd,
		eInstruction_fnmaddRC,
		eInstruction_fnmadds,
		eInstruction_fnmaddsRC,
		eInstruction_fnmsub,
		eInstruction_fnmsubRC,
		eInstruction_fnmsubs,
		eInstruction_fnmsubsRC,

		// Floating point rounding and conversion
		eInstruction_frsp,
		eInstruction_frspRC,
		eInstruction_fctid,
		eInstruction_fctidRC,
		eInstruction_fctidz,
		eInstruction_fctidzRC,
		eInstruction_fctiw,
		eInstruction_fctiwRC,
		eInstruction_fctiwz,
		eInstruction_fctiwzRC,
		eInstruction_fcfid,
		eInstruction_fcfidRC,

		// Floating point select
		eInstruction_fsel,
		eInstruction_fselRC,

		// Floating point control registers
		eInstruction_mffs,
		eInstruction_mffsRC,
		eInstruction_mcrfs,
		eInstruction_mtfsfi,
		eInstruction_mtfsfiRC,
		eInstruction_mtfsf,
		eInstruction_mtfsfRC,
		eInstruction_mtfsb0,
		eInstruction_mtfsb0RC,
		eInstruction_mtfsb1,
		eInstruction_mtfsb1RC,

		// VMX instructions
		eInstruction_lvebx,
		eInstruction_lvehx,
		eInstruction_lvewx,		//+
		eInstruction_lvlx,		//+
		eInstruction_lvlxl,		//+
		eInstruction_lvrx,		//+
		eInstruction_lvrxl,		//+
		eInstruction_lvsl,
		eInstruction_lvsr, 
		eInstruction_lvx,			//+
		eInstruction_lvxl,		//+
		eInstruction_mfvscr,
		eInstruction_mtvscr, 
		eInstruction_stvebx,
		eInstruction_stvehx,
		eInstruction_stvewx,		//+
		eInstruction_stvlx,		//+
		eInstruction_stvlxl,		//+
		eInstruction_stvrx,		//+
		eInstruction_stvrxl,		//+
		eInstruction_stvx,		//+
		eInstruction_stvxl,		//+
		eInstruction_lhbrx,
		eInstruction_lwbrx,
		eInstruction_sthbrx,
		eInstruction_stwbrx,
		eInstruction_lswi,
		eInstruction_lswx,
		eInstruction_stswi,
		eInstruction_stswx,
		eInstruction_vaddcuw,
		eInstruction_vaddfp,		//+
		eInstruction_vaddsbs,
		eInstruction_vaddshs,
		eInstruction_vaddsws,
		eInstruction_vaddubm,
		eInstruction_vaddubs,
		eInstruction_vadduhm,
		eInstruction_vadduhs,
		eInstruction_vadduwm,
		eInstruction_vadduws,
		eInstruction_vand,		//+
		eInstruction_vandc,		//+
		eInstruction_vavgsb,
		eInstruction_vavgsh,
		eInstruction_vavgsw,
		eInstruction_vavgub,
		eInstruction_vavguh,
		eInstruction_vavguw, 
		eInstruction_vcfpsxws,	//+
		eInstruction_vcfpuxws,	//+
		eInstruction_vcsxwfp, //+
		eInstruction_vcuxwfp, //+
		eInstruction_vctsxs,	//+
		eInstruction_vctuxs,		//+
		eInstruction_vcfsx,	//+
		eInstruction_vcfux,	//+
		eInstruction_vcmpbfp,
		eInstruction_vcmpbfpRC,
		eInstruction_vcmpeqfp,
		eInstruction_vcmpeqfpRC,
		eInstruction_vcmpequb,
		eInstruction_vcmpequbRC,
		eInstruction_vcmpequh,
		eInstruction_vcmpequhRC,
		eInstruction_vcmpequw,
		eInstruction_vcmpequwRC,
		eInstruction_vcmpgefp,
		eInstruction_vcmpgefpRC,
		eInstruction_vcmpgtfp,
		eInstruction_vcmpgtfpRC,
		eInstruction_vcmpgtsb,
		eInstruction_vcmpgtsbRC,
		eInstruction_vcmpgtsh,
		eInstruction_vcmpgtshRC,
		eInstruction_vcmpgtsw,
		eInstruction_vcmpgtswRC,
		eInstruction_vcmpgtub,
		eInstruction_vcmpgtubRC,
		eInstruction_vcmpgtuh,
		eInstruction_vcmpgtuhRC,
		eInstruction_vcmpgtuw,
		eInstruction_vcmpgtuwRC,
		eInstruction_vexptefp,	//+
		eInstruction_vlogefp,		//+
		eInstruction_vmaddcfp128, 
		eInstruction_vmaddfp,		//+
		eInstruction_vmaxfp,		//+
		eInstruction_vmaxsb,
		eInstruction_vmaxsh, 
		eInstruction_vmaxsw, 
		eInstruction_vmaxub,
		eInstruction_vmaxuh, 
		eInstruction_vmaxuw, 
		eInstruction_vminfp,		//+
		eInstruction_vminsb, 
		eInstruction_vminsh, 
		eInstruction_vminsw, 
		eInstruction_vminub, 
		eInstruction_vminuh, 
		eInstruction_vminuw, 
		eInstruction_vmrghb, 
		eInstruction_vmrghh, 
		eInstruction_vmrghw,		//+
		eInstruction_vmrglb, 
		eInstruction_vmrglh, 
		eInstruction_vmrglw,		//+
		eInstruction_vdot3fp,		//+
		eInstruction_vdot4fp,		//+
		eInstruction_vmulfp128, 
		eInstruction_vnmsubfp,	//+
		eInstruction_vnor,		//+
		eInstruction_vor,			//+
		eInstruction_vperm,		//+
		eInstruction_vpermwi128, 
		eInstruction_vpkd3d128,
		eInstruction_vpkpx,
		eInstruction_vpkshss,		//+
		eInstruction_vpkshus,		//+
		eInstruction_vpkswss,		//+
		eInstruction_vpkswus,		//+
		eInstruction_vpkuhum,		//+
		eInstruction_vpkuhus,		//+
		eInstruction_vpkuwum,		//+
		eInstruction_vpkuwus,		//+
		eInstruction_vrefp,		//+
		eInstruction_vrfim,		//+
		eInstruction_vrfin,		//+
		eInstruction_vrfip,		//+
		eInstruction_vrfiz,		//+
		eInstruction_vrlb, 
		eInstruction_vrlh, 
		eInstruction_vrlimi128, 
		eInstruction_vrlw,		//+
		eInstruction_vrsqrtefp,	//+
		eInstruction_vsel,		//+
		eInstruction_vsl, 
		eInstruction_vslb, 
		eInstruction_vsldoi,		//+
		eInstruction_vslh, 
		eInstruction_vslo,		//+
		eInstruction_vslw,		//+
		eInstruction_vspltb, 
		eInstruction_vsplth, 
		eInstruction_vspltisb, 
		eInstruction_vspltish, 
		eInstruction_vspltisw,	//+
		eInstruction_vspltw,		//+
		eInstruction_vsr, 
		eInstruction_vsrab, 
		eInstruction_vsrah, 
		eInstruction_vsraw,		//+
		eInstruction_vsrb, 
		eInstruction_vsrh, 
		eInstruction_vsro,		//+
		eInstruction_vsrw,		//+
		eInstruction_vsubcuw, 
		eInstruction_vsubfp,		//+
		eInstruction_vsubsbs,
		eInstruction_vsubshs,
		eInstruction_vsubsws,
		eInstruction_vsububm,
		eInstruction_vsububs,
		eInstruction_vsubuhm,
		eInstruction_vsubuhs,
		eInstruction_vsubuwm,
		eInstruction_vsubuws,
		eInstruction_vupkd3d128,
		eInstruction_vupkhpx,
		eInstruction_vupkhsb,		//+
		eInstruction_vupkhsh,	//+
		eInstruction_vupklpx,
		eInstruction_vupklsb,		//+
		eInstruction_vupklsh,		//+
		eInstruction_vxor,		//+

		eInstruction_MAX,
	};

	virtual uint32 ValidateInstruction(const uint8* inputStream) const override final;
	virtual uint32 DecodeInstruction(const uint8* inputStream, class decoding::Instruction& outDecodedInstruction) const override final;

	static CPU_XenonPPC& GetInstance();

private:
	// internal registers map
	const class platform::CPURegister* m_regMap[ eRegister_MAX + (128 * (16 + 8 + 4 + 2))];

	// internal instruction map
	const class platform::CPUInstruction* m_opMap[ eInstruction_MAX ];

	// get fixed point register
	inline const platform::CPURegister* REG(const uint32 val) const
	{
		if (val > 31) return NULL;
		return m_regMap[eRegister_R0 + val];
	}

	// get fixed point register, reg R0 is no reg
	inline const platform::CPURegister* REG0(const uint32 val) const
	{
		if (val == 0 ) return NULL;
		if (val > 31) return NULL;
		return m_regMap[eRegister_R0 + val];
	}

	// special register
	inline const platform::CPURegister* SREG(const uint32 val) const
	{
		if (val == 1) return m_regMap[eRegister_XER];
		if (val == 8) return m_regMap[eRegister_LR];
		if (val == 9) return m_regMap[eRegister_CTR];
		return NULL;
	}

	// get floating point register
	inline const platform::CPURegister* FREG(const uint32 val) const
	{
		if (val > 31) return NULL;
		return m_regMap[eRegister_FR0 + val];
	}

	// get vector register
	inline const platform::CPURegister* VREG(const uint32 val) const
	{
		if (val > 127) return NULL;
		return m_regMap[eRegister_VR0 + val];
	}

	// get control flag register
	inline const platform::CPURegister* CREG(const uint32 val) const
	{
		if (val > 7) return NULL;
		return m_regMap[eRegister_CR0 + val];
	}	

	// get control bit register
	inline const platform::CPURegister* CBIT(const uint32 val) const
	{
		if (val > 31) return NULL;
		return m_regMap[eRegister_CR0_LT + val];
	}

	// get floating point control flag register
	inline const platform::CPURegister* FCREG(const uint32 val) const
	{
		if (val > 7) return NULL;
		return m_regMap[eRegister_FPSCR0 + val];
	}

	// get floating point control bit register
	inline const platform::CPURegister* FCBIT(const uint32 val) const
	{
		if (val > 31) return NULL;
		return m_regMap[eRegister_FPSCR0_FX + val];
	}

	// mem reg wrapper
	inline const MemReg MEMREG(const uint32 reg0, const uint32 reg1) const
	{
		const platform::CPURegister* base = REG(reg0);
		const platform::CPURegister* index = REG(reg1);
		return MemReg(base, index);
	}

	// mem reg wrapper with reg0 support
	inline const MemReg MEMREG0(const uint32 reg0, const uint32 reg1) const
	{
		const platform::CPURegister* base = REG(reg0);
		const platform::CPURegister* index = REG(reg1);
		return reg0 ? MemReg(base, index) : MemReg(index,(uint32)0);
	}

	// mem reg wrapper
	inline const MemReg MEMOFS(const uint32 reg0, const uint32 ofs) const
	{
		const platform::CPURegister* base = REG(reg0);
		return MemReg(base, ofs);
	}

	// mem reg wrapper with reg0 support
	inline const MemReg MEMOFS0(const uint32 reg0, const uint32 ofs) const
	{
		const platform::CPURegister* base = REG(reg0);
		return base ? MemReg(base, ofs) : MemReg(ofs);
	}

	CPU_XenonPPC();
	virtual ~CPU_XenonPPC();
};

//---------------------------------------------------------------------------
