#include "xenonCPU.h"

//---------------------------------------------------------------------------

#define REPORT_DECODING_ERRORS

//---------------------------------------------------------------------------

template< const uint32 bestart, const uint32 length, const bool signExtend = false, const int shift = 0 >
class BitField
{
public:
	static const uint32 start = (32-bestart) - length;

	BitField(const uint32 input)
	{
		const uint32 mask = ((1<<length) - 1);
		uint32 bitSize = length;
		uint32 ext = (input >> start) & mask;

		// additional left shift
		if (shift > 0)
		{
			ext <<= shift;
			bitSize += shift;
		}

		// if signed expand copy the sign bit
		if (signExtend)
		{
			const uint32 signBitValue = ext & (1 << (bitSize-1));
			if (signBitValue != 0)
			{
				const uint32 signMask = ((uint32)-1) << bitSize;
				ext |= signMask;
			}
		}

		// right shift (always do that after sign conversion)
		if (shift < 0)
		{
			ext >>= (-shift);
		}

		// save value
		m_val = ext;
	}

	inline const uint32 Get() const
	{
		return m_val;
	}

	inline operator uint32() const
	{
		return m_val;
	}

private:
	uint32 m_val;
};

#ifdef REPORT_DECODING_ERRORS

	extern "C" __declspec(dllimport) void __stdcall OutputDebugStringA(const char* lpOutputString);

	void ReportDecodingError(const char* txt, ...)
	{
		char buf[512];
		va_list args;

		va_start(args, txt);
		vsprintf_s(buf, txt, args);
		va_end(args);

		strcat_s(buf, "\n");
		OutputDebugStringA(buf);
	}

	#define ERROR(x,...) ReportDecodingError(x, __VA_ARGS__); return false;

#else

	#define ERROR(x,...) return false;

#endif

template< typename T >
static decoding::Instruction::Operand MakeOperand( T data )
{
	decoding::Instruction::Operand ret;
	ret.m_type = decoding::Instruction::eType_None;
	return ret;
}

template<>
static decoding::Instruction::Operand MakeOperand( const uint32 data )
{
	decoding::Instruction::Operand ret;
	ret.m_type = decoding::Instruction::eType_Imm;
	ret.m_imm = data;
	return ret;
}

template< const uint32 bestart, const uint32 length, const bool signExtend, const int shift >
static decoding::Instruction::Operand MakeOperand( const BitField<bestart, length, signExtend, shift> data )
{
	decoding::Instruction::Operand ret;
	ret.m_type = decoding::Instruction::eType_Imm;
	ret.m_imm = (uint32)data;
	return ret;
}

template<>
static decoding::Instruction::Operand MakeOperand( const platform::CPURegister* reg )
{
	decoding::Instruction::Operand ret;

	if (!reg)
	{
		ret.m_type = decoding::Instruction::eType_Imm;
		ret.m_imm = 0;
	}
	else
	{
		ret.m_type = decoding::Instruction::eType_Reg;
		ret.m_reg = reg;
	}

	return ret;
}

template<>
static decoding::Instruction::Operand MakeOperand( const MemReg memreg )
{
	decoding::Instruction::Operand ret;
	ret.m_type = decoding::Instruction::eType_Mem;
	ret.m_reg = memreg.m_reg;
	ret.m_index = memreg.m_index;
	ret.m_imm = memreg.m_offset;
	ret.m_scale = 1;
	return ret;
}

static inline void SetupInstruction(decoding::Instruction& outOp, const platform::CPUInstruction* opcode)
{
	outOp.Setup(4, opcode);
}

template<typename Arg0>
static inline void SetupInstruction(decoding::Instruction& outOp, const platform::CPUInstruction* opcode, Arg0 arg0)
{
	outOp.Setup(4, opcode, MakeOperand(arg0));
}

template<typename Arg0, typename Arg1>
static inline void SetupInstruction(decoding::Instruction& outOp, const platform::CPUInstruction* opcode, Arg0 arg0, Arg1 arg1)
{
	outOp.Setup(4, opcode, MakeOperand(arg0), MakeOperand(arg1));
}

template<typename Arg0, typename Arg1, typename Arg2>
static inline void SetupInstruction(decoding::Instruction& outOp, const platform::CPUInstruction* opcode, Arg0 arg0, Arg1 arg1, Arg2 arg2)
{
	outOp.Setup(4, opcode, MakeOperand(arg0), MakeOperand(arg1), MakeOperand(arg2));
}

template<typename Arg0, typename Arg1, typename Arg2, typename Arg3>
static inline void SetupInstruction(decoding::Instruction& outOp, const platform::CPUInstruction* opcode, Arg0 arg0, Arg1 arg1, Arg2 arg2, Arg3 arg3)
{
	outOp.Setup(4, opcode, MakeOperand(arg0), MakeOperand(arg1), MakeOperand(arg2), MakeOperand(arg3));
}

template<typename Arg0, typename Arg1, typename Arg2, typename Arg3, typename Arg4>
static inline void SetupInstruction(decoding::Instruction& outOp, const platform::CPUInstruction* opcode, Arg0 arg0, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4)
{
	outOp.Setup(4, opcode, MakeOperand(arg0), MakeOperand(arg1), MakeOperand(arg2), MakeOperand(arg3), MakeOperand(arg4));
}

template<typename Arg0, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
static inline void SetupInstruction(decoding::Instruction& outOp, const platform::CPUInstruction* opcode, Arg0 arg0, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5)
{
	outOp.Setup(4, opcode, MakeOperand(arg0), MakeOperand(arg1), MakeOperand(arg2), MakeOperand(arg3), MakeOperand(arg4), MakeOperand(arg5));
}

#define CHECK(x) { if (!(x)) { ERROR("Decode: Usupported instruction format for opcode %d", pri); return false; } }
#define EMIT(opname, ...) { SetupInstruction(outOp, m_opMap[ eInstruction_##opname ], __VA_ARGS__ ); return 4; }

#define EMIT_RC(opname, ...)							\
	if (!rc) EMIT(opname, __VA_ARGS__ );				\
	EMIT(##opname##RC, __VA_ARGS__);

#define EMIT_MATH(opname, ...)							\
	if (!oe && !rc) EMIT(opname, __VA_ARGS__ );			\
	if (!oe && rc) EMIT(##opname##RC, __VA_ARGS__);		\
	if (oe && !rc) EMIT(##opname##o, __VA_ARGS__);		\
	EMIT(##opname##oRC, __VA_ARGS__ );

static inline uint32 EndianReverse32( const uint32 v )
{
	uint32 ret;
	const uint8* p = (const uint8*)&v;
	uint8* q = (uint8*)&ret;
	q[0] = p[3]; 
	q[1] = p[2]; 
	q[2] = p[1]; 
	q[3] = p[0];
	return ret;
}

uint32 CPU_XenonPPC::DecodeInstruction(const uint8* inputStream, class decoding::Instruction& outOp) const	
{
	const uint32 instrWord = EndianReverse32( *(const uint32*)inputStream );

	const BitField<0,6> opcd(instrWord);
	const BitField<30,1> aa(instrWord);
	const BitField<31,1> lk(instrWord);
	const BitField<6,5> b6(instrWord);
	const BitField<6,2> b6x2(instrWord);
	const BitField<11,5> b11(instrWord);
	const BitField<11,1> b11x1(instrWord);
	const BitField<16,5> b16(instrWord);
	const BitField<21,5> b21(instrWord);
	const BitField<9,2> b9x2(instrWord);
	const BitField<21,10> xo21(instrWord);

	const BitField<6,24,true,2> li(instrWord); // immediate value
	const BitField<16,14,true,2> bd(instrWord); // branch destination
	const BitField<19,1> bh(instrWord);
	const BitField<6,3> bf(instrWord);
	const BitField<11,3> bfa(instrWord);
	const BitField<20,7> lev(instrWord);
	const BitField<16,16,true> d(instrWord);
	const BitField<16,14,true> ds(instrWord);
	const BitField<16,14,true,2> ds4(instrWord);
	const BitField<16,16,true> si(instrWord);
	const BitField<16,16> ui(instrWord);
	const BitField<21,1> oe(instrWord);
	const BitField<31,1> rc(instrWord);

	const BitField<16,5> sh16(instrWord);
	const BitField<21,5> mb21(instrWord);
	const BitField<26,1> mb26x1(instrWord);
	const BitField<26,5> me26(instrWord);
	const BitField<11,10> spr(instrWord);
	const BitField<11,5> spr11(instrWord);
	const BitField<16,5> spr16(instrWord);
	const BitField<12,8> fxm(instrWord);
	const BitField<16,4> u16x4(instrWord);
	const BitField<7,7> flm(instrWord);
	const BitField<16,4> frb(instrWord);

	// or nops/moves
	if (opcd == 31 && xo21 == 444 && b11==b16 && b11==b6 && !rc)
	{
		EMIT(nop);
	}
	else if (opcd == 31 && xo21 == 444 && b6==b16 && b11!=b6 && !rc)
	{
		EMIT(mr, REG(b11), REG(b6));
	}

	// big shift values
	const BitField<28,2,false,5> vshift28(instrWord); // *32
	const BitField<30,2,false,5> vshift30(instrWord); // *32
	const BitField<21,1,false,6> vshift26a(instrWord); // *64
	const BitField<26,1,false,5> vshift26b(instrWord); // *32

	// extended regs (full 128 regs)
	const platform::CPURegister* vreg6 = VREG(vshift28 + b6);
	const platform::CPURegister* vreg11 = VREG(vshift26a + vshift26b + b11);
	const platform::CPURegister* vreg16 = VREG(vshift30 + b16);

	// match by primary opcode
	const uint32 pri = opcd.Get();
	switch (pri)
	{
		default: ERROR("Decode: Unmatched primary opcode %d", pri);

		// nop
		case 0: EMIT(nop);

		// tdi, twi
		case 2: EMIT(tdi, b6, REG(b11), si);
		case 3: EMIT(twi, b6, REG(b11), si);

		// VMX
		case 4:
		{
			const BitField<26,6> vxo26(instrWord);
			const BitField<21,11> vxo21(instrWord);
			const BitField<21,7> vxo21e(instrWord);
			const BitField<30,2> vxo30(instrWord);
			const BitField<27,1> vxo27b1(instrWord);
			const BitField<22,4> vxo22b4(instrWord);

			// full opcode
			switch (vxo21)
			{
				// move to/from special reg
				case 1540: CHECK(b11==0); CHECK(b16==0); EMIT(mfvscr, VREG(b6));
				case 1604: CHECK(b6==0); CHECK(b11==0); EMIT(mtvscr, VREG(b16));

				// vadd
				case 384: EMIT(vaddcuw, VREG(b6), VREG(b11), VREG(b16));
				case 10: EMIT(vaddfp, VREG(b6), VREG(b11), VREG(b16));
				case 768: EMIT(vaddsbs, VREG(b6), VREG(b11), VREG(b16));
				case 832: EMIT(vaddshs, VREG(b6), VREG(b11), VREG(b16));
				case 896: EMIT(vaddsws, VREG(b6), VREG(b11), VREG(b16));
				case 0: EMIT(vaddubm, VREG(b6), VREG(b11), VREG(b16));
				case 512: EMIT(vaddubs, VREG(b6), VREG(b11), VREG(b16));
				case 64: EMIT(vadduhm, VREG(b6), VREG(b11), VREG(b16));
				case 576: EMIT(vadduhs, VREG(b6), VREG(b11), VREG(b16));
				case 128: EMIT(vadduwm, VREG(b6), VREG(b11), VREG(b16));
				case 640: EMIT(vadduws, VREG(b6), VREG(b11), VREG(b16));

				case 1408: EMIT(vsubcuw, VREG(b6), VREG(b11), VREG(b16));
				case 1792: EMIT(vsubsbs, VREG(b6), VREG(b11), VREG(b16));
				case 1856: EMIT(vsubshs, VREG(b6), VREG(b11), VREG(b16));
				case 1920: EMIT(vsubsws, VREG(b6), VREG(b11), VREG(b16));
				case 1024: EMIT(vsububm, VREG(b6), VREG(b11), VREG(b16));
				case 1536: EMIT(vsububs, VREG(b6), VREG(b11), VREG(b16));
				case 1088: EMIT(vsubuhm, VREG(b6), VREG(b11), VREG(b16));
				case 1600: EMIT(vsubuhs, VREG(b6), VREG(b11), VREG(b16));
				case 1152: EMIT(vsubuwm, VREG(b6), VREG(b11), VREG(b16));
				case 1664: EMIT(vsubuws, VREG(b6), VREG(b11), VREG(b16));

				case 1028: EMIT(vand, VREG(b6), VREG(b11), VREG(b16));
				case 1092: EMIT(vandc, VREG(b6), VREG(b11), VREG(b16));
				case 1282: EMIT(vavgsb, VREG(b6), VREG(b11), VREG(b16));
				case 1346: EMIT(vavgsh, VREG(b6), VREG(b11), VREG(b16));
				case 1410: EMIT(vavgsw, VREG(b6), VREG(b11), VREG(b16));
				case 1026: EMIT(vavgub, VREG(b6), VREG(b11), VREG(b16));
				case 1090: EMIT(vavguh, VREG(b6), VREG(b11), VREG(b16));
				case 1154: EMIT(vavguw , VREG(b6), VREG(b11), VREG(b16));

				case 970: EMIT(vctsxs, VREG(b6), VREG(b16), b11);
				case 906: EMIT(vctuxs, VREG(b6), VREG(b16), b11);
				case 842: EMIT(vcfsx, VREG(b6), VREG(b16), b11);
				case 778: EMIT(vcfux, VREG(b6), VREG(b16), b11);

				case 966: EMIT(vcmpbfp, VREG(b6), VREG(b11), VREG(b16));
				case 1990: EMIT(vcmpbfpRC, VREG(b6), VREG(b11), VREG(b16));
				
				case 198: EMIT(vcmpeqfp, VREG(b6), VREG(b11), VREG(b16));
				case 1222: EMIT(vcmpeqfpRC, VREG(b6), VREG(b11), VREG(b16));
				case 6: EMIT(vcmpequb, VREG(b6), VREG(b11), VREG(b16));
				case 1030: EMIT(vcmpequbRC, VREG(b6), VREG(b11), VREG(b16));
				case 70: EMIT(vcmpequh, VREG(b6), VREG(b11), VREG(b16));
				case 1094: EMIT(vcmpequhRC, VREG(b6), VREG(b11), VREG(b16));
				case 134: EMIT(vcmpequw, VREG(b6), VREG(b11), VREG(b16));
				case 1158: EMIT(vcmpequwRC, VREG(b6), VREG(b11), VREG(b16));
				case 454: EMIT(vcmpgefp, VREG(b6), VREG(b11), VREG(b16));
				case 1478: EMIT(vcmpgefpRC, VREG(b6), VREG(b11), VREG(b16));
				case 710: EMIT(vcmpgtfp, VREG(b6), VREG(b11), VREG(b16));
				case 1734: EMIT(vcmpgtfpRC, VREG(b6), VREG(b11), VREG(b16));
				case 774: EMIT(vcmpgtsb, VREG(b6), VREG(b11), VREG(b16));
				case 1798: EMIT(vcmpgtsbRC, VREG(b6), VREG(b11), VREG(b16));
				case 838: EMIT(vcmpgtsh, VREG(b6), VREG(b11), VREG(b16));
				case 1862: EMIT(vcmpgtshRC, VREG(b6), VREG(b11), VREG(b16));
				case 902: EMIT(vcmpgtsw, VREG(b6), VREG(b11), VREG(b16));
				case 1926: EMIT(vcmpgtswRC, VREG(b6), VREG(b11), VREG(b16));
				case 518: EMIT(vcmpgtub, VREG(b6), VREG(b11), VREG(b16));
				case 1542: EMIT(vcmpgtubRC, VREG(b6), VREG(b11), VREG(b16));
				case 582: EMIT(vcmpgtuh, VREG(b6), VREG(b11), VREG(b16));
				case 1606: EMIT(vcmpgtuhRC, VREG(b6), VREG(b11), VREG(b16));
				case 646: EMIT(vcmpgtuw, VREG(b6), VREG(b11), VREG(b16));
				case 1670: EMIT(vcmpgtuwRC, VREG(b6), VREG(b11), VREG(b16));

				case 394: CHECK(b11==0); EMIT(vexptefp, VREG(b6), VREG(b16));
				case 458: CHECK(b11==0); EMIT(vlogefp, VREG(b6), VREG(b16));

				case 1034: EMIT(vmaxfp, VREG(b6), VREG(b11), VREG(b16));
				case 258: EMIT(vmaxsb, VREG(b6), VREG(b11), VREG(b16));
				case 322: EMIT(vmaxsh, VREG(b6), VREG(b11), VREG(b16));
				case 386: EMIT(vmaxsw, VREG(b6), VREG(b11), VREG(b16));
				case 2: EMIT(vmaxub, VREG(b6), VREG(b11), VREG(b16));
				case 66: EMIT(vmaxuh, VREG(b6), VREG(b11), VREG(b16));
				case 130: EMIT(vmaxuw, VREG(b6), VREG(b11), VREG(b16));
				case 1098: EMIT(vminfp, VREG(b6), VREG(b11), VREG(b16));
				case 770: EMIT(vminsb, VREG(b6), VREG(b11), VREG(b16));
				case 834: EMIT(vminsh, VREG(b6), VREG(b11), VREG(b16));
				case 898: EMIT(vminsw, VREG(b6), VREG(b11), VREG(b16));
				case 514: EMIT(vminub, VREG(b6), VREG(b11), VREG(b16));
				case 578: EMIT(vminuh, VREG(b6), VREG(b11), VREG(b16));
				case 642: EMIT(vminuw, VREG(b6), VREG(b11), VREG(b16));

				case 524: EMIT(vspltb, VREG(b6), VREG(b16), b11);
				case 588: EMIT(vsplth, VREG(b6), VREG(b16), b11);
				case 652: EMIT(vspltw, VREG(b6), VREG(b16), b11);
				case 780: EMIT(vspltisb, VREG(b6), b11);
				case 844: EMIT(vspltish, VREG(b6), b11);
				case 908: EMIT(vspltisw, VREG(b6), b11);

				case 398: EMIT(vpkshss, VREG(b6), VREG(b11), VREG(b16));
				case 270: EMIT(vpkshus, VREG(b6), VREG(b11), VREG(b16));
				case 462: EMIT(vpkswss, VREG(b6), VREG(b11), VREG(b16));
				case 334: EMIT(vpkswus, VREG(b6), VREG(b11), VREG(b16));
				case 14: EMIT(vpkuhum, VREG(b6), VREG(b11), VREG(b16));
				case 142: EMIT(vpkuhus, VREG(b6), VREG(b11), VREG(b16));
				case 78: EMIT(vpkuwum, VREG(b6), VREG(b11), VREG(b16));
				case 206: EMIT(vpkuwus, VREG(b6), VREG(b11), VREG(b16));

				case 74: EMIT(vsubfp, VREG(b6), VREG(b11), VREG(b16));
				case 1284: EMIT(vnor, VREG(b6), VREG(b11), VREG(b16));
				case 1220: EMIT(vxor, VREG(b6), VREG(b11), VREG(b16));
				case 1036: EMIT(vslo, VREG(b6), VREG(b11), VREG(b16));
				case 1100: EMIT(vsro, VREG(b6), VREG(b11), VREG(b16));

				case 708: EMIT(vsr, VREG(b6), VREG(b11), VREG(b16));
				case 772: EMIT(vsrab, VREG(b6), VREG(b11), VREG(b16)); 
				case 836: EMIT(vsrah, VREG(b6), VREG(b11), VREG(b16)); 
				case 900: EMIT(vsraw, VREG(b6), VREG(b11), VREG(b16)); 
				case 516: EMIT(vsrb, VREG(b6), VREG(b11), VREG(b16));
				case 580: EMIT(vsrh, VREG(b6), VREG(b11), VREG(b16));
				case 644: EMIT(vsrw, VREG(b6), VREG(b11), VREG(b16));
				case 388: EMIT(vslw, VREG(b6), VREG(b11), VREG(b16));
				case 324: EMIT(vslh, VREG(b6), VREG(b11), VREG(b16));
				case 260: EMIT(vslb, VREG(b6), VREG(b11), VREG(b16));

				case 266: CHECK(b11==0); EMIT(vrefp, VREG(b6), VREG(b16));
				case 714: CHECK(b11==0); EMIT(vrfim, VREG(b6), VREG(b16));
				case 522: CHECK(b11==0); EMIT(vrfin, VREG(b6), VREG(b16));
				case 650: CHECK(b11==0); EMIT(vrfip, VREG(b6), VREG(b16));
				case 586: CHECK(b11==0); EMIT(vrfiz, VREG(b6), VREG(b16));

				case 1156:
				{
					if (b11 == b16)
					{
						EMIT(mr, VREG(b6), VREG(b11));
					}
					else
					{
						EMIT(vor, VREG(b6), VREG(b11), VREG(b16));
					}
				}

				case 12: EMIT(vmrghb, VREG(b6), VREG(b11), VREG(b16));
				case 76: EMIT(vmrghh, VREG(b6), VREG(b11), VREG(b16));
				case 140: EMIT(vmrghw, VREG(b6), VREG(b11), VREG(b16));
				case 268: EMIT(vmrglb, VREG(b6), VREG(b11), VREG(b16));
				case 332: EMIT(vmrglh, VREG(b6), VREG(b11), VREG(b16));
				case 396: EMIT(vmrglw, VREG(b6), VREG(b11), VREG(b16));

				case 4: EMIT(vrlb, VREG(b6), VREG(b11), VREG(b16));
				case 68: EMIT(vrlh, VREG(b6), VREG(b11), VREG(b16));
				case 132: EMIT(vrlw, VREG(b6), VREG(b11), VREG(b16));

				case 590: CHECK(b11==0); EMIT(vupkhsh, VREG(b6), VREG(b16));
				case 526: CHECK(b11==0); EMIT(vupkhsb, VREG(b6), VREG(b16));
				case 846: CHECK(b11==0); EMIT(vupkhpx, VREG(b6), VREG(b16));
				case 974: CHECK(b11==0); EMIT(vupklpx, VREG(b6), VREG(b16));
				case 654: CHECK(b11==0); EMIT(vupklsb, VREG(b6), VREG(b16));					
				case 718: CHECK(b11 == 0); EMIT(vupklsh, VREG(b6), VREG(b16));

				case 330: CHECK(b11==0); EMIT(vrsqrtefp, VREG(b6), VREG(b16));
			}

			// extended op code
			switch (vxo26)
			{
				case 42: EMIT(vsel, VREG(b6), VREG(b11), VREG(b16), VREG(b21));
				case 43: EMIT(vperm, VREG(b6), VREG(b11), VREG(b16), VREG(b21));
				case 44: EMIT(vsldoi, VREG(b6), VREG(b11), VREG(b16), vxo22b4);
				case 46: EMIT(vmaddfp, VREG(b6), VREG(b11), VREG(b21), VREG(b16));
				case 47: EMIT(vnmsubfp, VREG(b6), VREG(b11), VREG(b21), VREG(b16));
			}

			// extended set
			switch (vxo21e)
			{
				// loads
				case 64: CHECK(vxo30==3); EMIT(lvlx, vreg6, MEMREG0(b11,b16));
				case 96: CHECK(vxo30==3); EMIT(lvlxl, vreg6, MEMREG0(b11,b16));
				case 68: CHECK(vxo30==3); EMIT(lvrx, vreg6, MEMREG0(b11,b16));
				case 100: CHECK(vxo30==3); EMIT(lvrxl, vreg6, MEMREG0(b11,b16));
				case 12: CHECK(vxo30==3); EMIT(lvx, vreg6,  MEMREG0(b11,b16));
				case 44: CHECK(vxo30==3); EMIT(lvxl, vreg6, MEMREG0(b11,b16));

				// stores
				case 24: CHECK(vxo30==3); EMIT(stvewx, vreg6, MEMREG0(b11,b16));
				case 80: CHECK(vxo30==3); EMIT(stvlx, vreg6, MEMREG0(b11,b16));
				case 112: CHECK(vxo30==3); EMIT(stvlxl, vreg6, MEMREG0(b11,b16));
				case 84: CHECK(vxo30==3); EMIT(stvrx, vreg6, MEMREG0(b11,b16));
				case 116: CHECK(vxo30==3); EMIT(stvrxl, vreg6, MEMREG0(b11,b16));
				case 28: CHECK(vxo30==3); EMIT(stvx, vreg6, MEMREG0(b11,b16));
				case 60: CHECK(vxo30==3); EMIT(stvxl, vreg6, MEMREG0(b11,b16));
			}

			// vsldoi128
			if (vxo27b1)
			{
				EMIT(vsldoi, vreg6, vreg11, vreg16, vxo22b4 );
			}

			// unknown
			ERROR("Decode: Unmatched secondary opcode xo21=%d, xo26=%d, xo21e=%d for primary opcode %d", 
				vxo21, vxo26, vxo21e, pri);
		}

		// extended VMX
		case 5:
		{
			const BitField<22,4> vxo22(instrWord);
			const BitField<27,1> vxo27(instrWord);

			// match
			if ( vxo27 == 0 )
			{
				switch (vxo22)
				{
					case 0: EMIT(vperm, vreg6, vreg11, vreg16, VREG(0));
					case 1: EMIT(vperm, vreg6, vreg11, vreg16, VREG(1));
					case 2: EMIT(vperm, vreg6, vreg11, vreg16, VREG(2));
					case 3: EMIT(vperm, vreg6, vreg11, vreg16, VREG(3));
					case 4: EMIT(vperm, vreg6, vreg11, vreg16, VREG(4));
					case 5: EMIT(vperm, vreg6, vreg11, vreg16, VREG(5));
					case 6: EMIT(vperm, vreg6, vreg11, vreg16, VREG(6));
					case 7: EMIT(vperm, vreg6, vreg11, vreg16, VREG(7));

					case 8: EMIT(vpkshss, vreg6, vreg11, vreg16);
					case 9: EMIT(vpkshus, vreg6, vreg11, vreg16);
					case 10: EMIT(vpkswss, vreg6, vreg11, vreg16);
					case 11: EMIT(vpkswus, vreg6, vreg11, vreg16);
					case 12: EMIT(vpkuhum, vreg6, vreg11, vreg16);
					case 13: EMIT(vpkuhus, vreg6, vreg11, vreg16);
					case 14: EMIT(vpkuwum, vreg6, vreg11, vreg16);
					case 15: EMIT(vpkuwus, vreg6, vreg11, vreg16);
				}
			}
			else
			{
				switch (vxo22)
				{
					case 0: EMIT(vaddfp, vreg6, vreg11, vreg16);
					case 1: EMIT(vsubfp, vreg6, vreg11, vreg16);
					case 2: EMIT(vmulfp128, vreg6, vreg11, vreg16);

					case 3: EMIT(vmaddfp, vreg6, vreg11, vreg16, vreg6);
					case 4: EMIT(vmaddfp, vreg6, vreg11, vreg6, vreg16); // addc

					case 5: EMIT(vnmsubfp, vreg6, vreg11, vreg16, vreg6);

					case 6: EMIT(vdot3fp, vreg6, vreg11, vreg16);
					case 7: EMIT(vdot4fp, vreg6, vreg11, vreg16);

					case 8: EMIT(vand, vreg6, vreg11, vreg16);
					case 9: EMIT(vandc, vreg6, vreg11, vreg16);
					case 10: EMIT(vnor, vreg6, vreg11, vreg16);
					case 12: EMIT(vxor, vreg6, vreg11, vreg16);
					case 13: EMIT(vsel, vreg6, vreg11, vreg16, vreg6); // arg3 == arg0
					case 14: EMIT(vslo, vreg6, vreg11, vreg16);
					case 15: EMIT(vsro, vreg6, vreg11, vreg16);

					case 11:
					{
						if (vreg11 == vreg16)
						{
							EMIT(mr, vreg6, vreg11);
						}
						else
						{
							EMIT(vor, vreg6, vreg11, vreg16);
						}
					}
				}
			}

			// unknown
			ERROR("Decode: Unmatched secondary opcode xo22=%d for primary opcode %d", 
				vxo22, pri);
		}

		// extended VMX
		case 6:
		{
			const BitField<26,6> vxo26(instrWord);
			const BitField<21,11> vxo21(instrWord);
			const BitField<21,7> vxo21e(instrWord);
			const BitField<22,4> vxo22(instrWord);
			const BitField<27,1> vxo27(instrWord);
			const BitField<30,2> vxo30(instrWord);

			const BitField<11,3> b11x3(instrWord);

			// match
			switch (vxo21e)
			{
				case (33+(0*4)): EMIT(vpermwi128, vreg6, vreg16, b11+0*32);
				case (33+(1*4)): EMIT(vpermwi128, vreg6, vreg16, b11+1*32);
				case (33+(2*4)): EMIT(vpermwi128, vreg6, vreg16, b11+2*32);
				case (33+(3*4)): EMIT(vpermwi128, vreg6, vreg16, b11+3*32);
				case (33+(4*4)): EMIT(vpermwi128, vreg6, vreg16, b11+4*32);
				case (33+(5*4)): EMIT(vpermwi128, vreg6, vreg16, b11+5*32);
				case (33+(6*4)): EMIT(vpermwi128, vreg6, vreg16, b11+6*32);
				case (33+(7*4)): EMIT(vpermwi128, vreg6, vreg16, b11+7*32);

				case 35: EMIT(vcfpsxws, vreg6, vreg16, b11);
				case 39: EMIT(vcfpuxws, vreg6, vreg16, b11);

				case 43: EMIT(vcsxwfp, vreg6, vreg16, b11);
				case 47: EMIT(vcuxwfp, vreg6, vreg16, b11);
					
				case 51: CHECK(b11==0); EMIT(vrfim, vreg6, vreg16);
				case 55: CHECK(b11==0); EMIT(vrfin, vreg6, vreg16);
				case 63: CHECK(b11==0); EMIT(vrfiz, vreg6, vreg16);

				case 56: CHECK(b11==0); EMIT(vupkhsb, vreg6, vreg16);
				case 122: CHECK(b11==0); EMIT(vupkhsh, vreg6, vreg16);
				case 60: CHECK(b11==0); EMIT(vupklsb, vreg6, vreg16);
				case 126: CHECK(b11==0); EMIT(vupklsh, vreg6, vreg16);

				case 97: EMIT(vpkd3d128, vreg6, vreg16, (uint32)(b11/4), (uint32)(b11&3), (uint32)(0));
				case 101: EMIT(vpkd3d128, vreg6, vreg16, (uint32)(b11/4),(uint32)( b11&3), (uint32)(1));
				case 105: EMIT(vpkd3d128, vreg6, vreg16, (uint32)(b11/4), (uint32)(b11&3), (uint32)(2));
				case 109: EMIT(vpkd3d128, vreg6, vreg16, (uint32)(b11/4), (uint32)(b11&3), (uint32)(3));

				case 111: CHECK(b11==0); EMIT(vlogefp, vreg6, vreg16);
				case 107: CHECK(b11==0); EMIT(vexptefp, vreg6, vreg16);

				case 99: CHECK(b11==0); EMIT(vrefp, vreg6, vreg16);

				case 103: CHECK(b11==0); EMIT(vrsqrtefp, vreg6, vreg16);

				case 113: EMIT(vrlimi128, vreg6, vreg16, b11, (uint32)0);
				case 117: EMIT(vrlimi128, vreg6, vreg16, b11, (uint32)1);
				case 121: EMIT(vrlimi128, vreg6, vreg16, b11, (uint32)2);
				case 125: EMIT(vrlimi128, vreg6, vreg16, b11, (uint32)3);
				case 115: EMIT(vspltw, vreg6, vreg16, b11);
				case 119: EMIT(vspltisw, vreg6, b11);

				case 127: EMIT(vupkd3d128, vreg6, vreg16, b11x3 );
			}

			// match
			if ( vxo27 == 0 )
			{
				switch (vxo22)
				{
					case 0: EMIT(vcmpeqfp, vreg6, vreg11, vreg16);
					case 1: EMIT(vcmpeqfpRC, vreg6, vreg11, vreg16);
					case 2: EMIT(vcmpgefp, vreg6, vreg11, vreg16);
					case 3: EMIT(vcmpgefpRC, vreg6, vreg11, vreg16);
					case 4: EMIT(vcmpgtfp, vreg6, vreg11, vreg16);
					case 5: EMIT(vcmpgtfpRC, vreg6, vreg11, vreg16);
					case 6: EMIT(vcmpbfp, vreg6, vreg11, vreg16);
					case 7: EMIT(vcmpbfpRC, vreg6, vreg11, vreg16);
					case 8: EMIT(vcmpequw, vreg6, vreg11, vreg16);
					case 9: EMIT(vcmpequwRC, vreg6, vreg11, vreg16);
					case 10: EMIT(vmaxfp, vreg6, vreg11, vreg16);
					case 11: EMIT(vminfp, vreg6, vreg11, vreg16);
					case 12: EMIT(vmrghw, vreg6, vreg11, vreg16);
					case 13: EMIT(vmrglw, vreg6, vreg11, vreg16);
				}
			}
			else
			{
				switch (vxo22)
				{
					case 1: EMIT(vrlw, vreg6, vreg11, vreg16);
					case 3: EMIT(vslw, vreg6, vreg11, vreg16);
					case 5: EMIT(vsraw, vreg6, vreg11, vreg16);
					case 7: EMIT(vsrw, vreg6, vreg11, vreg16);

					case 12: EMIT(vrfim, vreg6, vreg16);
					case 13: EMIT(vrfin, vreg6, vreg16);
					case 14: EMIT(vrfip, vreg6, vreg16);
					case 15: EMIT(vrfiz, vreg6, vreg16);
				}
			}

			// unknown
			ERROR("Decode: Unmatched secondary opcode xo21=%d, xo26=%d, xo21e=%d, xo22=%d for primary opcode %d", 
				vxo21, vxo26, vxo21e, vxo22, pri);
		}

		// muli (multiply with immediate)
		case 7: EMIT(mulli, REG(b6), REG(b11), si);

		// subf( subtract from)
		case 8: EMIT(subfic, REG(b6), REG(b11), si);

		// cmplwi, cmpldi
		case 10:
		{
			const BitField<10,1> l(instrWord);
			if (!l) EMIT(cmplwi, CREG(bf), REG(b11), ui);
			EMIT(cmpldi, CREG(bf), REG(b11), ui);
		}

		// cmpwi, cmpdi
		case 11:
		{
			const BitField<10,1> l(instrWord);
			if (!l) EMIT(cmpwi, CREG(bf), REG(b11), si);
			EMIT(cmpdi, CREG(bf), REG(b11), si);
		}

		// addic, addic.
		case 12: EMIT( addic, REG(b6), REG(b11), si );
		case 13: EMIT( addicRC, REG(b6), REG(b11), si );

		// addi, addis, li, lis
		case 14: 
			if (b11==0) EMIT( li, REG(b6), si );
			EMIT( addi, REG(b6), REG(b11), si );

		case 15:
			if (b11==0) EMIT( lis, REG(b6), si );
			EMIT( addis, REG(b6), REG(b11), si );

		// bc, bca, bcl, bcla
		case 16:
			if (!aa && !lk)	EMIT(bc, b6, CBIT(b11), bd);
			if (!aa && lk)  EMIT(bcl, b6, CBIT(b11), bd);
			if (aa && !lk)  EMIT(bca, b6, CBIT(b11), bd);
			EMIT(bcla, b6, CBIT(b11), bd);;

		// sc (system call)
		case 17:
			EMIT(sc, lev);

		// b,ba,bl,bla
		case 18:
			if (!aa && !lk)	EMIT(b, li);
			if (!aa && lk)  EMIT(bl, li);
			if (aa && !lk)  EMIT(ba, li);
			EMIT(bla, li);

		// bclr, bclrl
		case 19:
		{
			switch (xo21)
			{
				default: ERROR("Decode: Unmatched secondary opcode %d for primary opcode %d", xo21, pri);

				// bclr, bclrl
				case 16: 
					if (lk)	EMIT(bclrl, b6, CBIT(b11));
					EMIT(bclr, b6, CBIT(b11));

				// bcctr, bcctrl
				case 528: 
					if (lk)	EMIT(bcctrl, b6, CBIT(b11));
					EMIT(bcctr, b6, CBIT(b11));

				// conditonal register operations
				case 257: EMIT(crand, CBIT(b6), CBIT(b11), CBIT(b16));
				case 449: EMIT(cror, CBIT(b6), CBIT(b11), CBIT(b16));
				case 193: EMIT(crxor, CBIT(b6), CBIT(b11), CBIT(b16));
				case 225: EMIT(crnand, CBIT(b6), CBIT(b11), CBIT(b16));
				case 33: EMIT(crnor, CBIT(b6), CBIT(b11), CBIT(b16));
				case 289: EMIT(creqv, CBIT(b6), CBIT(b11), CBIT(b16));
				case 129: EMIT(crandc, CBIT(b6), CBIT(b11), CBIT(b16));
				case 417: EMIT(crorc, CBIT(b6), CBIT(b11), CBIT(b16));

				// mcrf (move conditional register flag)
				case 0: EMIT(mcrf, CREG(b6), CREG(b11));
			}
		}

	    // rlwimi, rlwimi., rlwinm, rlwinm., rlwnm, rlwnm.
		case 20: EMIT_RC(rlwimi, REG(b11), REG(b6), sh16, mb21, me26);
		case 21: EMIT_RC(rlwinm, REG(b11), REG(b6), sh16, mb21, me26);
		case 23: EMIT_RC(rlwnm, REG(b11), REG(b6), REG(b16), mb21, me26);

   		// andi., andis., ori, oris, xori, xoris
		case 24: EMIT(ori, REG(b11), REG(b6), ui);
		case 25: EMIT(oris, REG(b11), REG(b6), ui);
		case 26: EMIT(xori, REG(b11), REG(b6), ui);
		case 27: EMIT(xoris, REG(b11), REG(b6), ui);
		case 28: EMIT(andiRC, REG(b11), REG(b6), ui);
		case 29: EMIT(andisRC, REG(b11), REG(b6), ui);
    
		// shift opcodes
		case 30:
		{
			const BitField<27,3> xo27(instrWord);
			switch (xo27)
			{
				// rldicl, rldicl., rldicr, rldicr., rldic, rldic., rldimi, rldimi.
				case 0: EMIT_RC(rldicl, REG(b11), REG(b6), sh16 + (aa?32:0), mb21 + (mb26x1?32:0));
				case 1: EMIT_RC(rldicr, REG(b11), REG(b6), sh16 + (aa?32:0), mb21 + (mb26x1?32:0));
				case 2: EMIT_RC(rldic, REG(b11), REG(b6), sh16 + (aa?32:0), mb21 + (mb26x1?32:0));
				case 3: EMIT_RC(rldimi, REG(b11), REG(b6), sh16 + (aa?32:0), mb21 + (mb26x1?32:0));
			}

			const BitField<27,4> xo27x4(instrWord);
			switch (xo27x4)
			{
				// rldcl, rldcl., rldcr, rldcr.
				case 8: EMIT_RC(rldcl, REG(b11), REG(b6), REG(b16), mb21 + (mb26x1?32:0));
				case 9: EMIT_RC(rldcr, REG(b11), REG(b6), REG(b16), mb21 + (mb26x1?32:0));
			}

			ERROR("Decode: Unmatched secondary opcode %d for primary %d", xo27x4, pri);
		}

		// extended opcodes
		case 31:
		{
			const BitField<21,10> xo21(instrWord);
			switch (xo21)
			{
				// lbzx, lbzxu
				case 87: EMIT(lbzx, REG(b6), MEMREG0(b11,b16));
				case 119: EMIT(lbzux, REG(b6), MEMREG(b11,b16));

				// lhzx, lhzux, lhax, lhaux 
				case 279: EMIT(lhzx, REG(b6), MEMREG0(b11,b16));
				case 311: EMIT(lhzux, REG(b6), MEMREG(b11,b16));
				case 343: EMIT(lhax, REG(b6), MEMREG0(b11,b16));
				case 375: EMIT(lhaux, REG(b6), MEMREG(b11,b16));

				// lwzx, lwzux, lwax, lwaux
				case 23: EMIT(lwzx, REG(b6), MEMREG0(b11,b16));
				case 55: EMIT(lwzux, REG(b6), MEMREG(b11,b16));
				case 341: EMIT(lwax, REG(b6), MEMREG0(b11,b16));
				case 373: EMIT(lwaux, REG(b6), MEMREG(b11,b16));

				// ldx, ldux
				case 21: EMIT(ldx, REG(b6), MEMREG0(b11,b16));
				case 53: EMIT(ldux, REG(b6), MEMREG(b11,b16));

				// stbx, stbux
				case 215: EMIT(stbx, REG(b6), MEMREG0(b11,b16));
				case 247: EMIT(stbux, REG(b6), MEMREG(b11,b16));

				// sthx, sthux
				case 407: EMIT(sthx, REG(b6), MEMREG0(b11,b16));
				case 439: EMIT(sthux, REG(b6), MEMREG(b11,b16));

				// stwx, stwux
				case 151: EMIT(stwx, REG(b6), MEMREG0(b11,b16));
				case 183: EMIT(stwux, REG(b6), MEMREG(b11,b16));

				// stdx, stdux
				case 149: EMIT(stdx, REG(b6), MEMREG0(b11,b16));
				case 181: EMIT(stdux, REG(b6), MEMREG(b11,b16));

				// lhbrx, lwbrx, sthbrx, stwbrx
				case 790: EMIT(lhbrx, REG(b6), MEMREG0(b11,b16));
				case 534: EMIT(lwbrx, REG(b6), MEMREG0(b11,b16));
				case 918: EMIT(sthbrx, REG(b6), MEMREG0(b11,b16));
				case 662: EMIT(stwbrx, REG(b6), MEMREG0(b11,b16));

				// lswi, lswx, stswi, stswx
				case 597: EMIT(lswi, REG(b6), REG0(b11), b16);
				case 533: EMIT(lswx, REG(b6), REG0(b11), REG(b16));
				case 725: EMIT(stswi, REG(b6), REG0(b11), b16);
				case 661: EMIT(stswx, REG(b6), REG0(b11), REG(b16));

				// lvehx, lvebx, lvewx
				case 39: EMIT(lvehx, VREG(b6), MEMREG0(b11, b16));
				case 7: EMIT(lvebx, VREG(b6), MEMREG0(b11, b16));
				case 71: EMIT(lvewx, VREG(b6), MEMREG0(b11, b16));

				// stvebx, stvehx, stvewx
				case 135: EMIT(stvebx, VREG(b6), MEMREG0(b11, b16));
				case 167: EMIT(stvehx, VREG(b6), MEMREG0(b11, b16));
				case 199: EMIT(stvewx, VREG(b6), MEMREG0(b11, b16));

				// cmpw, cmpd
				case 0:
				{
					const BitField<10,1> l(instrWord);
					if (!l) EMIT(cmpw, CREG(bf), REG(b11), REG(b16));
					EMIT(cmpd, CREG(bf), REG(b11), REG(b16));
				}

				// cmplw, cmpld
				case 32:
				{
					const BitField<10,1> l(instrWord);
					if (!l) EMIT(cmplw, CREG(bf), REG(b11), REG(b16));
					EMIT(cmpld, CREG(bf), REG(b11), REG(b16));
				}

				// td, tw
				case 68: EMIT(td, b6, REG(b11), REG(b16));
				case 4: EMIT(tw, b6, REG(b11), REG(b16));

				// and, and., or, or., xor, xor., nand, nand, nor, nor., eqv, eqv., andc, andc., orc, orc
				case 28: EMIT_RC(and, REG(b11), REG(b6), REG(b16));
				case 444: EMIT_RC(or, REG(b11), REG(b6), REG(b16));
				case 316: EMIT_RC(xor, REG(b11), REG(b6), REG(b16));
				case 476: EMIT_RC(nand, REG(b11), REG(b6), REG(b16));
				case 124: EMIT_RC(nor, REG(b11), REG(b6), REG(b16));
				case 284: EMIT_RC(eqv, REG(b11), REG(b6), REG(b16));
				case 60: EMIT_RC(andc, REG(b11), REG(b6), REG(b16));
				case 412: EMIT_RC(orc, REG(b11), REG(b6), REG(b16));

				// extsb, extsb., extsh, extsh., extsw, extsw., popcntb, popcntb., cntlzw, cntlzw., cntlzd, cntlzd.
				case 954: EMIT_RC(extsb, REG(b11), REG(b6));
				case 922: EMIT_RC(extsh, REG(b11), REG(b6));
				case 986: EMIT_RC(extsw, REG(b11), REG(b6));
				case 122: EMIT(popcntb, REG(b11), REG(b6));
				case 26: EMIT_RC(cntlzw, REG(b11), REG(b6));
				case 58: EMIT_RC(cntlzd, REG(b11), REG(b6));

				// sld, sld., slw, slw., srd, srd., srw, srw., srad, srad., sraw, sraw.
				case 27: EMIT_RC(sld, REG(b11), REG(b6), REG(b16));
				case 24: EMIT_RC(slw, REG(b11), REG(b6), REG(b16));
				case 539: EMIT_RC(srd, REG(b11), REG(b6), REG(b16));
				case 536: EMIT_RC(srw, REG(b11), REG(b6), REG(b16));
				case 794: EMIT_RC(srad, REG(b11), REG(b6), REG(b16));
				case 792: EMIT_RC(sraw, REG(b11), REG(b6), REG(b16));

				// sradi, sradi., srawi, srawi.
				case 824: EMIT_RC(srawi, REG(b11), REG(b6), sh16);
				case 826: EMIT_RC(sradi, REG(b11), REG(b6), sh16); // 413*3+0
				case 827: EMIT_RC(sradi, REG(b11), REG(b6), sh16 + 32); // 413*3+1

				// mtspr, mfspr (move to/from system register)
				case 467: CHECK(SREG(spr11)); EMIT(mtspr, SREG(spr11), REG(b6));
				case 339: CHECK(SREG(spr11)); EMIT(mfspr, REG(b6), SREG(spr11));

				// mtcrf, mtocrf
				case 144:
					if (b11x1==0) EMIT(mtcrf, fxm, REG(b6));
					EMIT(mtocrf, fxm, REG(b6));

				// mfcr, mfocrf
				case 19:
					if (b11x1==0) EMIT(mfcr, REG(b6));
					EMIT(mfocrf, fxm, REG(b6));

				// mfmsr (move to/from machine state register)
				case 83: EMIT(mfmsr, REG(b6), m_regMap[eRegister_MSR]);
				case 178:
					if (b11x1==0) EMIT(mtmsrd, m_regMap[eRegister_MSR], REG(b6));
					EMIT(mtmsree, m_regMap[eRegister_MSR], REG(b6));

				// mftb (move from time base register)
				case 371: EMIT(mftb, REG(b6), b11, b16);

				// lwarx/stwcx
				case 20: EMIT(lwarx, REG(b6), MEMREG0(b11,b16));
				case 84: EMIT(ldarx, REG(b6), MEMREG0(b11,b16));
				case 150: CHECK(rc==1); EMIT(stwcxRC, REG(b6), MEMREG0(b11,b16));
				case 214: CHECK(rc==1); EMIT(stdcxRC, REG(b6), MEMREG0(b11,b16));

				// sync (memory bariers)
				case 598: if (b9x2==0) EMIT(sync);
					if (b9x2==1) EMIT(lwsync);
					if (b9x2==2) EMIT(ptesync);
					ERROR("Decode: Invalid sync instruction type");

				// eieio
				case 854: EMIT(eieio);

				// cache operations
				case 86: EMIT(dcbf, REG(b11), REG(b16));
				case 54: EMIT(dcbst, REG(b11), REG(b16));
				case 278: EMIT(dcbt, REG(b11), REG(b16));
				case 246: EMIT(dcbtst, REG(b11), REG(b16));
				case 1014: EMIT(dcbz, MEMREG0(b11,b16));

				// lfsx, lfsux, lfdx, lfdux
				case 535: EMIT(lfsx, FREG(b6), MEMREG0(b11, b16));
				case 567: EMIT(lfsux, FREG(b6), MEMREG(b11, b16));
				case 599: EMIT(lfdx, FREG(b6), MEMREG0(b11, b16));
				case 631: EMIT(lfdux, FREG(b6), MEMREG(b11, b16));

				// stfsx, stfsux, stfdx, stfdux, stfiwx
				case 663: EMIT(stfsx, FREG(b6), MEMREG0(b11, b16));
				case 695: EMIT(stfsux, FREG(b6), MEMREG(b11, b16));
				case 727: EMIT(stfdx, FREG(b6), MEMREG0(b11, b16));
				case 759: EMIT(stfdux, FREG(b6), MEMREG(b11, b16));
				case 983: EMIT(stfiwx, FREG(b6), MEMREG0(b11, b16));
			}

			// xo21 ext
			const BitField<21,11> xo21ext(instrWord);
			switch (xo21ext)
			{
				// VMX loads
				case 1038: EMIT(lvlx, VREG(b6), MEMREG0(b11, b16));
				case 1550: EMIT(lvlxl, VREG(b6), MEMREG0(b11, b16));
				case 1102: EMIT(lvrx, VREG(b6), MEMREG0(b11, b16));
				case 1614: EMIT(lvrxl, VREG(b6), MEMREG0(b11, b16));
				case 206: EMIT(lvx, VREG(b6), MEMREG0(b11, b16));
				case 718: EMIT(lvxl, VREG(b6), MEMREG0(b11, b16));
				case 12: EMIT(lvsl, VREG(b6), REG(b11), REG(b16));
				case 76: EMIT(lvsr, VREG(b6), REG(b11), REG(b16));

				// VMX stores
				case 270: EMIT(stvebx, VREG(b6), MEMREG0(b11, b16));
				case 334: EMIT(stvehx, VREG(b6), MEMREG0(b11, b16));
				case 398: EMIT(stvewx, VREG(b6), MEMREG0(b11, b16));
				case 1294: EMIT(stvlx, VREG(b6), MEMREG0(b11, b16));
				case 1806: EMIT(stvlxl, VREG(b6), MEMREG0(b11, b16));
				case 1358: EMIT(stvrx, VREG(b6), MEMREG0(b11, b16));
				case 1870: EMIT(stvrxl, VREG(b6), MEMREG0(b11, b16));
				case 462: EMIT(stvx, VREG(b6), MEMREG0(b11, b16));
				case 974: EMIT(stvxl, VREG(b6), MEMREG0(b11, b16));
			}

			// xo22 (math)
			const BitField<22,9> xo22(instrWord);
			switch (xo22)
			{
				// add, add., addo, addo.
				case 266: EMIT_MATH(add, REG(b6), REG(b11), REG(b16));

				// addc, addc., addco, addco.
				case 10: EMIT_MATH(addc, REG(b6), REG(b11), REG(b16));

				// adde, adde., addeo, addeo.
				case 138: EMIT_MATH(adde, REG(b6), REG(b11), REG(b16));

				// addme, addme., addmeo, addmeo.
				case 234: EMIT_MATH(addme, REG(b6), REG(b11));

				// addze, addze., addzeo, addzeo.
				case 202: EMIT_MATH(addze, REG(b6), REG(b11));

				// subf, subf., subfo, subfo.
				case 40: EMIT_MATH(subf, REG(b6), REG(b11), REG(b16));

				// subfc, subfc., subfco, subfco.
				case 8: EMIT_MATH(subfc, REG(b6), REG(b11), REG(b16));

				// subfe, subfe., subfeo, subfeo.
				case 136: EMIT_MATH(subfe, REG(b6), REG(b11), REG(b16));

				// subfme, subfme., subfmeo, subfmeo.
				case 232: EMIT_MATH(subfme, REG(b6), REG(b11));

				// subfze, subfze., subfzeo, subfzeo.
				case 200: EMIT_MATH(subfze, REG(b6), REG(b11));
				
			    // neg, neg., nego, nego.
				case 104: EMIT_MATH(neg, REG(b6), REG(b11));

				// mulld, mulld., mulldo, mulldo.
				case 233: EMIT_MATH(mulld, REG(b6), REG(b11), REG(b16));

				// mullw, mullw., mullwo, mullwo.
				case 235: EMIT_MATH(mullw, REG(b6), REG(b11), REG(b16));

				// mullhd, mullhd.
				case 73: EMIT_RC(mullhd, REG(b6), REG(b11), REG(b16));

				// mullhw, mullhw.
				case 75: EMIT_RC(mullhw, REG(b6), REG(b11), REG(b16));

				// mullhdu, mullhdu.
				case 9: EMIT_RC(mulhdu, REG(b6), REG(b11), REG(b16));

				// mullhwu, mullhwu.
				case 11: EMIT_RC(mulhwu, REG(b6), REG(b11), REG(b16));

				// divd, divd., divdo, divdo.
				case 489: EMIT_MATH(divd, REG(b6), REG(b11), REG(b16));

				// divw, divw., divwo, divwo.
				case 491: EMIT_MATH(divw, REG(b6), REG(b11), REG(b16));

				// divdu, divdu., divduo, divduo.
				case 457: EMIT_MATH(divdu, REG(b6), REG(b11), REG(b16));

				// divwu, divwu., divwuo, divwuo.
				case 459: EMIT_MATH(divwu, REG(b6), REG(b11), REG(b16));
			}

			ERROR("Decode: Unmatched secondary opcode %d for primary %d", xo21, pri);
		}

		// lbz, lbzu
		case 34: EMIT( lbz, REG(b6), MEMOFS0(b11,d) );
		case 35: EMIT( lbzu, REG(b6), MEMOFS0(b11,d) );

		// lhz, lhzu, lha, lhau
		case 40: EMIT( lhz, REG(b6), MEMOFS0(b11,d) );
		case 41: EMIT( lhzu, REG(b6), MEMOFS0(b11,d) );
		case 42: EMIT( lha, REG(b6), MEMOFS0(b11,d) );
		case 43: EMIT( lhau, REG(b6), MEMOFS0(b11,d) );

		// lwz, lwzu
		case 32: EMIT( lwz, REG(b6), MEMOFS0(b11,d) );
		case 33: EMIT( lwzu, REG(b6), MEMOFS0(b11,d) );

		// lwa, ld, ldu
		case 58: 
			if (aa && !lk) EMIT( lwa, REG(b6), MEMOFS0(b11,ds4) ); // offset is *4
			if (!aa && !lk) EMIT( ld, REG(b6), MEMOFS0(b11,ds4) ); // offset is *4
			if (!aa && lk) EMIT( ldu, REG(b6), MEMOFS0(b11,ds4) ); // offset is *4
			ERROR("Decode: unsupported format of opcode %d", pri);

		// stb, stbu
		case 38: EMIT( stb, REG(b6), MEMOFS0(b11,d) );
		case 39: EMIT( stbu, REG(b6), MEMOFS0(b11,d) );

		// sth, sthu
		case 44: EMIT( sth, REG(b6), MEMOFS0(b11,d) );
		case 45: EMIT( sthu, REG(b6), MEMOFS0(b11,d) );

		// stw, stwu
		case 36: EMIT( stw, REG(b6), MEMOFS0(b11,d) );
		case 37: EMIT( stwu, REG(b6), MEMOFS0(b11,d) );

	    // lmw, stmw (oad/store multiple words)
		//case 46: EMIT( lmw, REG(b6), MEMOFS0(b11,d) );
		//case 47: EMIT( stmw, REG(b6), MEMOFS0(b11,d) );

		// lfs, lfsu, lfd, lfdu
		case 48: EMIT( lfs, FREG(b6), MEMOFS0(b11,d) );
		case 49: EMIT( lfsu, FREG(b6), MEMOFS0(b11,d) );
		case 50: EMIT( lfd, FREG(b6), MEMOFS0(b11,d) );
		case 51: EMIT( lfdu, FREG(b6), MEMOFS0(b11,d) );

		// stfs, stfsu, stfd, stfdu
		case 52: EMIT( stfs, FREG(b6), MEMOFS0(b11,d) );
		case 53: EMIT( stfsu, FREG(b6), MEMOFS0(b11,d) );
		case 54: EMIT( stfd, FREG(b6), MEMOFS0(b11,d) );
		case 55: EMIT( stfdu, FREG(b6), MEMOFS0(b11,d) );

		// floating point math (single precission)
		case 59:
		{
			const BitField<26,5> xo26(instrWord);
			switch (xo26)
			{
				// fadd, fsub, fmul, fdiv
				case 21: EMIT_RC(fadds, FREG(b6), FREG(b11), FREG(b16));
				case 20: EMIT_RC(fsubs, FREG(b6), FREG(b11), FREG(b16));
				case 25: EMIT_RC(fmuls, FREG(b6), FREG(b11), FREG(b21));
				case 18: EMIT_RC(fdivs, FREG(b6), FREG(b11), FREG(b16));

				// fmadd, fmsub, fnmadd, fnmsub
				case 29: EMIT_RC(fmadds, FREG(b6), FREG(b11), FREG(b21), FREG(b16));
				case 28: EMIT_RC(fmsubs, FREG(b6), FREG(b11), FREG(b21), FREG(b16));
				case 31: EMIT_RC(fnmadds, FREG(b6), FREG(b11), FREG(b21), FREG(b16));
				case 30: EMIT_RC(fnmsubs, FREG(b6), FREG(b11), FREG(b21), FREG(b16));

				// fsqrts, fres
				case 22: EMIT_RC(fsqrt, FREG(b6), FREG(b16));
				case 24: EMIT_RC(fre, FREG(b6), FREG(b16));
			}

			ERROR("Decode: Unmatched secondary opcode xo26=%d for primary %d", xo26, pri);
		}

		// std, stdu
		case 62:
			if (!aa && !lk) EMIT( std, REG(b6), MEMOFS0(b11,ds4) ); // offset is *4
			if (!aa && lk) EMIT( stdu, REG(b6), MEMOFS0(b11,ds4) ); // offset is *4
			ERROR("Decode: unsupported format of opcode %d", pri);

		// floating point math (double precission)
		case 63:
		{
			const BitField<21,10> xo21(instrWord);
			switch (xo21)
			{
				// fmr, fneg, fabs, fnabs
				case 72: EMIT_RC(fmr, FREG(b6), FREG(b16));
				case 40: EMIT_RC(fneg, FREG(b6), FREG(b16));
				case 264: EMIT_RC(fabs, FREG(b6), FREG(b16));
				case 136: EMIT_RC(fnabs, FREG(b6), FREG(b16));

				// frsp, fctid, fctidz, fctiw, fctiwz, fcfid
				case 12: EMIT_RC(frsp, FREG(b6), FREG(b16));
				case 814: EMIT_RC(fctid, FREG(b6), FREG(b16));
				case 815: EMIT_RC(fctidz, FREG(b6), FREG(b16));
				case 14: EMIT_RC(fctiw, FREG(b6), FREG(b16));
				case 15: EMIT_RC(fctiwz, FREG(b6), FREG(b16));
				case 846: EMIT_RC(fcfid, FREG(b6), FREG(b16));

				// fcmpu, fcmpo
				case 0: EMIT(fcmpu, CREG(bf), FREG(b11), FREG(b16));
				case 32: EMIT(fcmpo, CREG(bf), FREG(b11), FREG(b16));

				// mffs, mcrfs, mtfsfi, mtfsf, mtfsb0, mtfsb1
				case 583: EMIT_RC(mffs, FREG(b6));
				case 64: EMIT(mcrfs, CREG(bf), FCREG(bfa));
				case 134: EMIT_RC(mtfsfi, FCREG(bf), u16x4);
				case 711: EMIT_RC(mtfsf, flm, FREG(frb));
				case 70: EMIT_RC(mtfsb0, FCBIT(b6));
				case 38: EMIT_RC(mtfsb1, FCBIT(b6));
			}

			const BitField<26,5> xo26(instrWord);
			switch (xo26)
			{
				// fadd, fsub, fmul, fdiv
				case 21: EMIT_RC(fadd, FREG(b6), FREG(b11), FREG(b16));
				case 20: EMIT_RC(fsub, FREG(b6), FREG(b11), FREG(b16));
				case 25: EMIT_RC(fmul, FREG(b6), FREG(b11), FREG(b21)); // BEWARE, the second arg is at bit 21
				case 18: EMIT_RC(fdiv, FREG(b6), FREG(b11), FREG(b16));

				// fmadd, fmsub, fnmadd, fnmsub
				case 29: EMIT_RC(fmadd, FREG(b6), FREG(b11), FREG(b21), FREG(b16));
				case 28: EMIT_RC(fmsub, FREG(b6), FREG(b11), FREG(b21), FREG(b16));
				case 31: EMIT_RC(fnmadd, FREG(b6), FREG(b11), FREG(b21), FREG(b16));
				case 30: EMIT_RC(fnmsub, FREG(b6), FREG(b11), FREG(b21), FREG(b16));

				// fsel
				case 23: EMIT_RC(fsel, FREG(b6), FREG(b11), FREG(b21), FREG(b16));

				// fsqrt and some other optional instructions
				case 22: EMIT_RC(fsqrt, FREG(b6), FREG(b16));
				case 24: EMIT_RC(fre, FREG(b6), FREG(b16));
				case 26: EMIT_RC(frsqrtx, FREG(b6), FREG(b16));					
			}


			ERROR("Decode: Unmatched secondary opcode x21=%d, xo26=%d for primary %d", xo21, xo26, pri);
		}
	}

	// valid instruction parsed
	return 0;
}

