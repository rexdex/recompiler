#include "xenonCPU.h"

#include "../recompiler_core/decodingContext.h"
#include "../recompiler_core/decodingMemoryMap.h"
#include "../recompiler_core/codeGenerator.h"

////#pragma optimize("",off)

extern "C" __declspec(dllimport) void __stdcall OutputDebugStringA(const char* lpOutputString);

#define OV		0x01
#define CA_OUT	0x02
#define CA_IN	0x04
#define CR0		0x08
#define CR1		0x10
#define SO		0x20
#define CR6		0x40

#define REG (decoding::Instruction::eType_Reg)
#define IMM (decoding::Instruction::eType_Imm)
#define MREG (decoding::Instruction::eType_Mem)

const platform::CPURegister* RegLR = NULL;
const platform::CPURegister* RegCTR = NULL;
const platform::CPURegister* RegXER_SO = NULL;
const platform::CPURegister* RegXER_OV = NULL;
const platform::CPURegister* RegXER_CA = NULL;
const platform::CPURegister* RegCR0 = NULL;
const platform::CPURegister* RegCR1 = NULL;
const platform::CPURegister* RegCR6 = NULL;

template< uint32 flags >
static inline bool AddRegs(class decoding::InstructionExtendedInfo& outInfo)
{
	if (flags & CR0)
	{
		if (!outInfo.AddRegisterDependency(RegXER_SO))
			return false;
		if (!outInfo.AddRegisterOutput(RegCR0))
			return false;
	}

	if (flags & CR1)
	{
		if (!outInfo.AddRegisterOutput(RegCR1))
			return false;
	}

	if (flags & CR6)
	{
		if (!outInfo.AddRegisterOutput(RegCR6))
			return false;
	}

	if (flags & OV)
	{
		if (!outInfo.AddRegisterOutput(RegXER_OV))
			return false;
		if (!outInfo.AddRegisterOutput(RegXER_SO))
			return false;
	}

	if (flags & CA_OUT)
		if (!outInfo.AddRegisterOutput(RegXER_CA))
			return false;

	if (flags & CA_IN)
		if (!outInfo.AddRegisterDependency(RegXER_CA))
			return false;

	if (flags & SO)
		if (!outInfo.AddRegisterDependency(RegXER_SO))
			return false;

	return true;
}

static inline void PrintCodef(std::string& outCode, const char* txt, ... )
{
	char buf[512];
	va_list args;

	va_start(args, txt);
	vsprintf_s(buf, txt, args);
	va_end(args);

	outCode += buf;
}

struct OpcodeInfo
{
	char	m_name[32];
	bool	m_ctrl;

	OpcodeInfo(const decoding::Instruction& op)
	{
		strcpy_s(m_name, op.GetOpcode()->GetName());
		const size_t opcodeNameLen = strlen(m_name);
		m_ctrl = (m_name[opcodeNameLen-1] == '.');
		if (m_ctrl) m_name[opcodeNameLen-1] = 0;
	}
};

static const char* GetBranchTypeName(const decoding::Instruction& instr, const char* infix, const char* postfix, const bool simple)
{
	char regName[16];
	strcpy_s(regName, instr.GetArg1().m_reg->GetName());

	// extract reg short name and flag
	char* regFlag = strchr(regName,'_');
	if (regFlag)
	{
		*regFlag = 0;
		++regFlag;
	}

	// extract flags
	const uint32 type = instr.GetArg0().m_imm;
	const bool b0 = (type & 16) != 0;
	const bool b1 = (type & 8) != 0;
	const bool b2 = (type & 4) != 0;
	const bool b3 = (type & 2) != 0;
	const bool b4 = (type & 1) != 0;
	
	// determine flag type
	char* flagReg = "?";
	char* flagName = "?";
	if (b0)
	{
		flagReg = "";
		flagName = "";
	}
	else if (regFlag)
	{
		flagReg = regName;

		if (0 == _stricmp(regFlag,"LT"))
			flagName = b1 ? "lt" : "ge";
		else if (0 == _stricmp(regFlag,"GT"))
			flagName = b1 ? "gt" : "le";
		else if (0 == _stricmp(regFlag,"EQ"))
			flagName = b1 ? "eq" : "ne";
		else if (0 == _stricmp(regFlag,"SO"))
			flagName = b1 ? "so" : "ns";
	}

	// CTR decrement
	const bool ctrTest = !simple && !b2; // decrement and test the CTR
	const char* ctrCmp = ctrTest ? (b3 ? "dz" : "dnz") : ""; // the way we compare the CTR

	// branch selection
	static char buf[512];
	sprintf_s(buf, "b%s%s%s%s %s", 
		ctrCmp, flagName, infix, postfix, flagReg );

	return buf;
}

static bool GenerateBranchInfo(const decoding::Instruction& instr, const uint64 codeAddress, char* outInfoCode, const uint32 outInfoSize)
{
	// invalid branch type
	if ( instr.GetArg0().m_type != decoding::Instruction::eType_Imm )
		return false;

	// just branch
	if ( instr.GetArg1().m_type == decoding::Instruction::eType_None )
	{
		bool isAbsoluteAddress = false;
		if (instr == "ba" || instr == "bla" )
			isAbsoluteAddress = true;

		// format shortened branch instruction
		const auto targetAddress = isAbsoluteAddress ? instr.GetArg0().m_imm : (codeAddress + instr.GetArg0().m_imm);
		sprintf_s(outInfoCode, outInfoSize, "%s <:L%06llXh>", instr.GetOpcode()->GetName(), targetAddress );
		return true;
	}

	// invalid condition register
	if ( instr.GetArg1().m_type != decoding::Instruction::eType_Reg )
		return false;

	// postfix (l or a)
	const char* infix = "";
	const char* postfix = "";
	bool hasAddress = true;
	bool simpleBranchCode = false;
	if (instr == "bcl" || instr == "bclrl" || instr == "bcctrl")
		postfix = "l";
	else if (instr == "bca")
		postfix = "a";
	else if (instr == "bcla")
		postfix = "la";

	// infix (lr or ctr)
	if (instr == "bclr" || instr == "bclrl")
	{
		infix = "lr";
		hasAddress = false;
	}
	else if (instr == "bcctr" || instr == "bcctrl")
	{
		infix = "ctr";
		hasAddress = false;
		simpleBranchCode = true;
	}

	// format shortened branch instruction
	const char* branchCode = GetBranchTypeName(instr, infix, postfix, simpleBranchCode);
	if (hasAddress)
	{
		// invalid address
		if ( instr.GetArg2().m_type != decoding::Instruction::eType_Imm )
			return false;

		const auto targetAddress = codeAddress + instr.GetArg2().m_imm;
		//sprintf_s(outInfoCode, outInfoSize, "(b%d) %s, <:L%06Xh>", instr.GetArg0().m_imm, branchCode, targetAddress );
		sprintf_s(outInfoCode, outInfoSize, "%s, <:L%06llXh>", branchCode, targetAddress);
	}
	else
	{
		//sprintf_s(outInfoCode, outInfoSize, "(b%d) %s", instr.GetArg0().m_imm, branchCode );
		sprintf_s(outInfoCode, outInfoSize, "%s", branchCode );
	}
	return true;
}

bool IInstructiondDecompilerXenon::GetExtendedText(const class decoding::Instruction& instr, const uint64 codeAddress, char* outText, const uint32 outTextSize) const
{
	return false; // no special info
}

bool IInstructiondDecompilerXenon::GetCommentText(const class decoding::Instruction& instr, const uint64 codeAddress, char* outText, const uint32 outTextSize) const
{
	return false; // no special comment
}

const bool IInstructiondDecompilerXenon::CheckArgs(const class decoding::Instruction& op, 
											decoding::Instruction::Type arg0 /*= decoding::Instruction::eType_None*/, 
											decoding::Instruction::Type arg1 /*= decoding::Instruction::eType_None,*/,
											decoding::Instruction::Type arg2 /*= decoding::Instruction::eType_None,*/,
											decoding::Instruction::Type arg3 /*= decoding::Instruction::eType_None,*/,
											decoding::Instruction::Type arg4 /*= decoding::Instruction::eType_None,*/,
											decoding::Instruction::Type arg5 /*= decoding::Instruction::eType_None*/) const
{
	if (op.GetArg0().m_type != arg0) return false;
	if (op.GetArg1().m_type != arg1) return false;
	if (op.GetArg2().m_type != arg2) return false;
	if (op.GetArg3().m_type != arg3) return false;
	if (op.GetArg4().m_type != arg4) return false;
	if (op.GetArg5().m_type != arg5) return false;
	return true;
}

__declspec(noinline) const bool IInstructiondDecompilerXenon::ErrInvalidMode(const class decoding::Instruction& op) const
{
	OutputDebugStringA("ErrInvalidMode\n");
	//log.Error( "Decoding: Instruction '%s' at %08Xh is used in unsupported mode", op.GetOpcode()->GetName(), op.GetCodeAddress() );
	return false;//eErrorInvalidMode;
}

__declspec(noinline) const bool IInstructiondDecompilerXenon::ErrInvalidArgs(const class decoding::Instruction& op) const
{
	OutputDebugStringA("ErrInvalidArgs\n");
	//log.Error( "Decoding: Instruction '%s' at %08Xh is in unsupported format", op.GetOpcode()->GetName(), op.GetCodeAddress() );
	return false;//eErrorInvalidArgs;
}

__declspec(noinline) const bool IInstructiondDecompilerXenon::ErrToManyRegisters(const class decoding::Instruction& op) const
{
	OutputDebugStringA("ErrToManyRegisters\n");
	//log.Error( "Decoding: Instruction '%s' at %08Xh uses to many registersis in ", op.GetOpcode()->GetName(), op.GetCodeAddress() );
	return false;//eErrorToManyRegisters;
}

//---------------------------------------------------------------------------

class CInstructiondDecoderXenon_NOP : public IInstructiondDecompilerXenon
{
public:
	CInstructiondDecoderXenon_NOP()
	{}

	virtual bool GetExtendedInfo(const class decoding::Instruction& op, const uint64 codeAddress, const decoding::Context& context, class decoding::InstructionExtendedInfo& outInfo) const override
	{
		if (!CheckArgs(op))
			return ErrInvalidArgs(op);

		outInfo.m_codeFlags |= decoding::InstructionExtendedInfo::eInstructionFlag_Nop;
		return true;
	}

	virtual const bool Decompile(class ILogOutput& output, const uint32 address, const decoding::Instruction& op, const decoding::InstructionExtendedInfo& exInfo, class code::IGenerator& codeGen, std::string& outCode) const
	{
		// no code generated
		outCode = "cpu::op::nop();";
		return true;
	}
};

//---------------------------------------------------------------------------

class CInstructiondDecoderXenon_MR : public IInstructiondDecompilerXenon
{
public:
	CInstructiondDecoderXenon_MR()
	{}

	virtual bool GetExtendedInfo(const class decoding::Instruction& op, const uint64 codeAddress, const decoding::Context& context, class decoding::InstructionExtendedInfo& outInfo) const override
	{
		if (!CheckArgs(op, REG, REG))
			return ErrInvalidArgs(op);

		if (!outInfo.AddRegisterOutput(op.GetArg0().m_reg))
			return ErrToManyRegisters(op);

		if (!outInfo.AddRegisterDependency(op.GetArg1().m_reg))
			return ErrToManyRegisters(op);

		return true;
	}

	virtual const bool Decompile(class ILogOutput& output, const uint32 address, const decoding::Instruction& op, const decoding::InstructionExtendedInfo& exInfo, class code::IGenerator& codeGen, std::string& outCode) const
	{
		PrintCodef(outCode, "regs.%s = regs.%s;", 
			op.GetArg0().m_reg->GetName(),
			op.GetArg1().m_reg->GetName() );
		return true;
	}
};

enum EMemoryType
{
	eMemoryType_Logical,
	eMemoryType_Algebraic,
	eMemoryType_Float,
	eMemoryType_Atomic,
	eMemoryType_Reversed,
	eMemoryType_VMX,
	eMemoryType_VMLX,
	eMemoryType_VMRX,
	eMemoryType_VMXIndexed,
};

template< const uint32 BitSize, EMemoryType type >
const char* GetMemoryLoadFunction()
{
	if (type == eMemoryType_Float)
	{
		if (BitSize == 64)
			return "load64f";
		else if (BitSize == 32)
			return "load32f";
		else
			return NULL;
	}
	else if (type == eMemoryType_Algebraic)
	{
		if (BitSize == 64)
			return "load64";
		else if (BitSize == 32)
			return "load32a";
		else if (BitSize == 16)
			return "load16a";
		else if (BitSize == 8)
			return  "load8a";
		else
			return NULL;
	}
	else if (type == eMemoryType_Logical)
	{
		if (BitSize == 64)
			return "load64";
		else if (BitSize == 32)
			return "load32z";
		else if (BitSize == 16)
			return "load16z";
		else if (BitSize == 8)
			return "load8z";
		else
			return NULL;
	}
	else if (type == eMemoryType_Reversed)
	{
		if (BitSize == 16)
			return "lhbrx";
		else if (BitSize == 32)
			return "lwbrx";
		else
			return NULL;
	}
	else if (type == eMemoryType_Atomic)
	{
		if (BitSize == 64)
			return "ldarx";
		else if (BitSize == 32)
			return "lwarx";
		else
			return NULL;
	}
	else if (type == eMemoryType_VMX)
	{
		if (BitSize == 128)
			return "lvx";
	}
	else if (type == eMemoryType_VMXIndexed)
	{
		if (BitSize == 32)
			return "lvewx";
		else if (BitSize == 16)
			return "lvehx";
		else if (BitSize == 8)
			return "lvebx";
		else
			return NULL;
	}
	else if (type == eMemoryType_VMLX)
	{
		if (BitSize == 128)
			return "lvlx";
	}
	else if (type == eMemoryType_VMRX)
	{
		if (BitSize == 128)
			return "lvrx";
	}

	return NULL;
}


template< const uint32 BitSize, EMemoryType type >
const char* GetMemoryStoreFunction()
{
	if (type == eMemoryType_Float)
	{
		if (BitSize == 64)
			return "store64f";
		else if (BitSize == 32)
			return "store32f";
		else
			return NULL;
	}
	else if (type == eMemoryType_Algebraic || type == eMemoryType_Logical)
	{
		if (BitSize == 64)
			return "store64";
		else if (BitSize == 32)
			return "store32";
		else if (BitSize == 16)
			return "store16";
		else if (BitSize == 8)
			return  "store8";
		else
			return NULL;
	}
	else if (type == eMemoryType_Atomic)
	{
		if (BitSize == 64)
			return "stdcx";
		else if (BitSize == 32)
			return "stwcx";
		else
			return NULL;
	}
	else if (type == eMemoryType_Reversed)
	{
		if (BitSize == 16)
			return "sthbrx";
		else if (BitSize == 32)
			return "stwbrx";
		else
			return NULL;
	}
	else if (type == eMemoryType_VMX)
	{
		if (BitSize == 128)
			return "stvx";
		else
			return NULL;
	}
	else if (type == eMemoryType_VMXIndexed)
	{
		if (BitSize == 32)
			return "stvewx";
		else if (BitSize == 16)
			return "stvehx";
		else if (BitSize == 8)
			return "stvebx";
		else
			return NULL;
	}
	else if (type == eMemoryType_VMLX)
	{
		if (BitSize == 128)
			return "stvlx";
	}
	else if (type == eMemoryType_VMRX)
	{
		if (BitSize == 128)
			return "stvrx";
	}

	return NULL;
}

template< const uint32 BitSize, const bool UpdateAddress, EMemoryType type >
class CInstructiondDecoderXenon_MEM_LOAD : public IInstructiondDecompilerXenon
{
public:
	virtual bool GetExtendedInfo(const class decoding::Instruction& op, const uint64 codeAddress, const decoding::Context& context, class decoding::InstructionExtendedInfo& outInfo) const override
	{
		// first operand is always a register
		if (!CheckArgs(op, REG, MREG))
			return ErrInvalidArgs(op);

		// registe address
		if (op.GetArg1().m_reg && !outInfo.AddRegisterDependency(op.GetArg1().m_reg))
			return ErrToManyRegisters(op);
		if (op.GetArg1().m_index && !outInfo.AddRegisterDependency(op.GetArg1().m_index))
			return ErrToManyRegisters(op);

		// address update
		if (UpdateAddress)
		{
			if (!outInfo.AddRegisterOutput(op.GetArg1().m_reg, true))
				return ErrToManyRegisters(op);
		}

		// register output
		if (!outInfo.AddRegisterOutput(op.GetArg0().m_reg, true))
			return ErrToManyRegisters(op);

		// invalid function type
		if (!GetMemoryLoadFunction<BitSize, type>())
			return ErrInvalidArgs(op);

		// memory read
		outInfo.m_memoryFlags |= decoding::InstructionExtendedInfo::eMemoryFlags_Read;

		// type of read
		if (type == eMemoryType_Float)
			outInfo.m_memoryFlags |= decoding::InstructionExtendedInfo::eMemoryFlags_Float;

		// direct memory access
		if ( context.GetMemoryMap().GetMemoryInfo(codeAddress).GetInstructionFlags().IsMappedMemory() )
			outInfo.m_memoryFlags |= decoding::InstructionExtendedInfo::eMemoryFlags_DirectMap;

		// output memory address
		outInfo.m_memoryAddressOffset = op.GetArg1().m_imm;
		outInfo.m_memoryAddressBase = op.GetArg1().m_reg;
		outInfo.m_memoryAddressIndex = op.GetArg1().m_index;
		outInfo.m_memoryAddressScale = op.GetArg1().m_scale;
		outInfo.m_memorySize = BitSize/8;
		return true;
	}

	virtual const bool Decompile(class ILogOutput& output, const uint32 address, const decoding::Instruction& op, const decoding::InstructionExtendedInfo& exInfo, class code::IGenerator& codeGen, std::string& outCode) const
	{
		if (!CheckArgs(op, REG, MREG))
			return false;

		// get load function type
		const char* functionPrefix = "cpu::mem::";
		const char* functionName = GetMemoryLoadFunction<BitSize, type>();
		if (!functionName)
			return false;

		// direct memory access
		if (exInfo.m_memoryFlags & decoding::InstructionExtendedInfo::eMemoryFlags_DirectMap)
		{
			functionPrefix = "";
			functionName = "regs.IO->MEM_READ";
		}

		// format address calculation code
		char addressCode[128];
		auto& arg1 = op.GetArg1();
		if ( !arg1.m_reg )
		{
			// immediate address
			sprintf_s( addressCode, "0x%08X", op.GetArg1().m_imm ); 
		}
		else if ( arg1.m_index )
		{
			if ( arg1.m_scale != 1 )
			{
				sprintf_s( addressCode, "regs.%s + %d*regs.%s + 0x%08X", 
					arg1.m_reg->GetName(),
					arg1.m_scale,
					arg1.m_index->GetName(),
					arg1.m_imm );
			}
			else
			{
				sprintf_s( addressCode, "regs.%s + regs.%s + 0x%08X", 
					arg1.m_reg->GetName(),
					arg1.m_index->GetName(),
					arg1.m_imm );
			}
		}
		else
		{
			sprintf_s( addressCode, "regs.%s + 0x%08X", 
				arg1.m_reg->GetName(),
				arg1.m_imm );
		}

		// emit code
		if (exInfo.m_memoryFlags & decoding::InstructionExtendedInfo::eMemoryFlags_DirectMap)
		{
			PrintCodef(outCode, "%s%s( 0x%08X, (uint32)(%s), %d, &regs.%s );",
				functionPrefix, functionName,
				address,
				addressCode, exInfo.m_memorySize, op.GetArg0().m_reg->GetName());
		}
		else
		{
			PrintCodef(outCode, "%s%s( regs, &regs.%s, (uint32)(%s) );",
				functionPrefix, functionName,
				op.GetArg0().m_reg->GetName(),
				addressCode);
		}

		// address update
		if (UpdateAddress)
		{
			PrintCodef( outCode, "\nregs.%s = (uint32)(%s);",
				arg1.m_reg->GetName(),
				addressCode );
		}

		return true;
	}
};

//---------------------------------------------------------------------------

template< const uint32 BitSize, const bool UpdateAddress, EMemoryType type >
class CInstructiondDecoderXenon_MEM_STORE : public IInstructiondDecompilerXenon
{
public:
	virtual bool GetExtendedInfo(const class decoding::Instruction& op, const uint64 codeAddress, const decoding::Context& context, class decoding::InstructionExtendedInfo& outInfo) const override
	{
		// first operand is always a register
		if (!CheckArgs(op, REG, MREG))
			return ErrInvalidArgs(op);

		// argument 0 is the value to write
		if (!outInfo.AddRegisterDependency(op.GetArg0().m_reg))
			return ErrToManyRegisters(op);

		// address deps
		if (op.GetArg1().m_reg && !outInfo.AddRegisterDependency(op.GetArg1().m_reg))
			return ErrToManyRegisters(op);
		if (op.GetArg1().m_index && !outInfo.AddRegisterDependency(op.GetArg1().m_index))
			return ErrToManyRegisters(op);

		// address update
		if (UpdateAddress)
		{
			if (!outInfo.AddRegisterOutput(op.GetArg1().m_reg, true))
				return ErrToManyRegisters(op);
		}

		// invalid function type
		if (!GetMemoryStoreFunction<BitSize, type>())
			return ErrInvalidArgs(op);

		// memory access
		outInfo.m_memoryFlags |= decoding::InstructionExtendedInfo::eMemoryFlags_Write;
		outInfo.m_memoryAddressOffset = op.GetArg1().m_imm;
		outInfo.m_memoryAddressBase = op.GetArg1().m_reg;
		outInfo.m_memoryAddressIndex = op.GetArg1().m_index;
		outInfo.m_memoryAddressScale = op.GetArg1().m_scale;
		outInfo.m_memorySize = BitSize/8;

		// direct memory access
		if ( context.GetMemoryMap().GetMemoryInfo(codeAddress).GetInstructionFlags().IsMappedMemory() )
			outInfo.m_memoryFlags |= decoding::InstructionExtendedInfo::eMemoryFlags_DirectMap;

		// floating point op
		if (type == eMemoryType_Float)
			outInfo.m_memoryFlags |= decoding::InstructionExtendedInfo::eMemoryFlags_Float;

		return true;
	}

	virtual const bool Decompile(class ILogOutput& output, const uint32 address, const decoding::Instruction& op, const decoding::InstructionExtendedInfo& exInfo, class code::IGenerator& codeGen, std::string& outCode) const
	{
		if (!CheckArgs(op, REG, MREG))
			return false;

		const char* functionPrefix = "cpu::mem::";
		const char* functionName = GetMemoryStoreFunction<BitSize, type>();
		if (!functionName)
			return false;

		// direct memory access
		if ( exInfo.m_memoryFlags & decoding::InstructionExtendedInfo::eMemoryFlags_DirectMap )
		{
			functionPrefix = "";
			functionName = "regs.IO->MEM_WRITE";
		}

		// format address calculation code
		char addressCode[128];
		const auto& arg1 = op.GetArg1();
		if ( !arg1.m_reg )
		{
			// immediate address
			sprintf_s( addressCode, "0x%08X", arg1.m_imm ); 
		}
		else if ( arg1.m_index )
		{
			if ( arg1.m_scale != 1 )
			{
				sprintf_s( addressCode, "regs.%s + %d*regs.%s + 0x%08X", 
					arg1.m_reg->GetName(),
					arg1.m_scale,
					arg1.m_index->GetName(),
					arg1.m_imm );
			}
			else
			{
				sprintf_s( addressCode, "regs.%s + regs.%s + 0x%08X", 
					arg1.m_reg->GetName(),
					arg1.m_index->GetName(),
					arg1.m_imm );
			}
		}
		else
		{
			sprintf_s( addressCode, "regs.%s + 0x%08X", 
				arg1.m_reg->GetName(),
				arg1.m_imm );
		}

		// emit code
		if (exInfo.m_memoryFlags & decoding::InstructionExtendedInfo::eMemoryFlags_DirectMap)
		{
			PrintCodef(outCode, "%s%s( 0x%08X, (uint32)(%s), %d, &regs.%s );",
				functionPrefix, functionName,
				address,
				addressCode, exInfo.m_memorySize, op.GetArg0().m_reg->GetName());
		}
		else
		{
			PrintCodef(outCode, "%s%s( regs, regs.%s, (uint32)(%s) );",
				functionPrefix, functionName,
				op.GetArg0().m_reg->GetName(),
				addressCode);
		}

		// address update
		if (UpdateAddress)
		{
			PrintCodef( outCode, "\nregs.%s = (uint32)(%s);",
				arg1.m_reg->GetName(),
				addressCode );
		}
		
		// done
		return true;
	}
};

//---------------------------------------------------------------------------

class CInstructiondDecoderXenon_DCBZ : public IInstructiondDecompilerXenon
{
public:
	virtual bool GetExtendedInfo(const class decoding::Instruction& op, const uint64 codeAddress, const decoding::Context& context, class decoding::InstructionExtendedInfo& outInfo) const override
	{
		// first operand is always a register
		if (!CheckArgs(op, MREG))
			return ErrInvalidArgs(op);

		// address deps
		if (op.GetArg0().m_reg && !outInfo.AddRegisterDependency(op.GetArg0().m_reg))
			return ErrToManyRegisters(op);
		if (op.GetArg0().m_index && !outInfo.AddRegisterDependency(op.GetArg0().m_index))
			return ErrToManyRegisters(op);
				
		// memory access
		outInfo.m_memoryFlags |= decoding::InstructionExtendedInfo::eMemoryFlags_Write;
		outInfo.m_memoryAddressOffset = op.GetArg1().m_imm;
		outInfo.m_memoryAddressBase = op.GetArg1().m_reg;
		outInfo.m_memoryAddressIndex = op.GetArg1().m_index;
		outInfo.m_memoryAddressScale = op.GetArg1().m_scale;
		outInfo.m_memoryAddressMask = ~31;
		outInfo.m_memorySize = 32;
		return true;
	}

	virtual const bool Decompile(class ILogOutput& output, const uint32 address, const decoding::Instruction& op, const decoding::InstructionExtendedInfo& exInfo, class code::IGenerator& codeGen, std::string& outCode) const
	{
		// format address calculation code
		char addressCode[128];
		const auto& arg1 = op.GetArg0();
		if (!arg1.m_reg)
		{
			// immediate address
			sprintf_s(addressCode, "0x%08X", arg1.m_imm);
		}
		else if (arg1.m_index)
		{
			if (arg1.m_scale != 1)
			{
				sprintf_s(addressCode, "regs.%s + %d*regs.%s + 0x%08X",
					arg1.m_reg->GetName(),
					arg1.m_scale,
					arg1.m_index->GetName(),
					arg1.m_imm);
			}
			else
			{
				sprintf_s(addressCode, "regs.%s + regs.%s + 0x%08X",
					arg1.m_reg->GetName(),
					arg1.m_index->GetName(),
					arg1.m_imm);
			}
		}
		else
		{
			sprintf_s(addressCode, "regs.%s + 0x%08X",
				arg1.m_reg->GetName(),
				arg1.m_imm);
		}

		outCode = "cpu::op::dcbz<0>(regs, ";
		outCode += addressCode;
		outCode += ");";
		return true;
	}
};

//---------------------------------------------------------------------------

template< uint32 flags,
		decoding::Instruction::Type arg0 = decoding::Instruction::eType_None,
		decoding::Instruction::Type arg1 = decoding::Instruction::eType_None,
		decoding::Instruction::Type arg2 = decoding::Instruction::eType_None,
		decoding::Instruction::Type arg3 = decoding::Instruction::eType_None,
		decoding::Instruction::Type arg4 = decoding::Instruction::eType_None,
		decoding::Instruction::Type arg5 = decoding::Instruction::eType_None >
class CInstructiondDecoderXenon_GENERIC : public IInstructiondDecompilerXenon
{
public:
	static inline bool AppendValue(char*& outText, uint32& outTextSize, const uint32 val, const uint32 size)
	{
		char temp[32];

		if (size == 1) sprintf_s(temp, "%02Xh ", (uint8)val);
		else if (size == 2) sprintf_s(temp, "%04Xh ", (uint16)val);
		else if (size == 4) sprintf_s(temp, "%08Xh ", val);
		else return false;

		const uint32 len = (uint32)strlen(temp);
		if (len < outTextSize)
		{
			strcat_s(outText, outTextSize, temp);
			outText += len;
			outTextSize -= len;
			return true;
		}

		return false;
	}

	static inline bool AppendImmediate(char*& outText, uint32& outTextSize, const decoding::Instruction::Operand& op, const bool force, const uint32 size)
	{
		if (op.m_type == decoding::Instruction::eType_Imm)
		{
			const int val = *(const int*)&op.m_imm;
			if ( val < -16 || val > 15 || force )
			{
				return AppendValue(outText, outTextSize, op.m_imm, size);
			}
		}

		return false;
	}

	virtual bool GetCommentText(const class decoding::Instruction& instr, const uint64 codeAddress, char* outText, const uint32 outTextSize) const
	{
		outText[0] = 0;

		int forceSize = 4;
		bool forceShow = false;
		if (instr == "li") { forceSize = 2; forceShow = true; }
		else if (instr == "lis") { forceSize = 2; forceShow = true; }
		else if (instr == "addi") { forceSize = 2; forceShow = true; }
		else if (instr == "addis") { forceSize = 2; forceShow = true; }

		bool result = false;
		uint32 sizeLeft = outTextSize;
		result |= AppendImmediate(outText, sizeLeft, instr.GetArg0(), forceShow, forceSize);
		result |= AppendImmediate(outText, sizeLeft, instr.GetArg1(), forceShow, forceSize);
		result |= AppendImmediate(outText, sizeLeft, instr.GetArg2(), forceShow, forceSize);
		result |= AppendImmediate(outText, sizeLeft, instr.GetArg3(), forceShow, forceSize);
		result |= AppendImmediate(outText, sizeLeft, instr.GetArg4(), forceShow, forceSize);
		return result;
	}

	virtual bool GetExtendedInfo(const class decoding::Instruction& op, const uint64 codeAddress, const decoding::Context& context, class decoding::InstructionExtendedInfo& outInfo) const override
	{
		// first operand is always a register
		if (!CheckArgs(op, arg0, arg1, arg2, arg3, arg4, arg5))
			return ErrInvalidArgs(op);

		// Add generic regs
		if (!AddRegs<flags>(outInfo))
			return false;

		// register main output
		if (arg0 == REG)
			if (!outInfo.AddRegisterOutput(op.GetArg0().m_reg))
				return ErrToManyRegisters(op);

		// register additional arg1 input
		if (arg1 == REG)
			if (!outInfo.AddRegisterDependency(op.GetArg1().m_reg))
				return ErrToManyRegisters(op);

		// register additional arg2 input
		if (arg2 == REG)
			if (!outInfo.AddRegisterDependency(op.GetArg2().m_reg))
				return ErrToManyRegisters(op);

		// register additional arg3 input
		if (arg3 == REG)
			if (!outInfo.AddRegisterDependency(op.GetArg3().m_reg))
				return ErrToManyRegisters(op);

		// register additional arg4 input
		if (arg4 == REG)
			if (!outInfo.AddRegisterDependency(op.GetArg4().m_reg))
				return ErrToManyRegisters(op);

		// register additional arg5 input
		if (arg5 == REG)
			if (!outInfo.AddRegisterDependency(op.GetArg5().m_reg))
				return ErrToManyRegisters(op);

		// decoded without errors
		return true;
	}
	
	static inline bool ExportOperandRet(std::string& txt, const decoding::Instruction::Operand& op)
	{
		if (op.m_type != decoding::Instruction::eType_None)
		{
			txt += ",";

			if (op.m_type == decoding::Instruction::eType_Reg)
			{
				if (!op.m_reg)
					return false;

				txt += "&regs.";
				txt += op.m_reg->GetName();
			}
			else if (op.m_type == decoding::Instruction::eType_Imm)
			{
				char s[16];
				sprintf_s(s, 16, "0x%X", op.m_imm);
				txt += s;
			}		
		}

		return true;
	}

	static inline bool ExportOperand(std::string& txt, const decoding::Instruction::Operand& op)
	{
		if (op.m_type != decoding::Instruction::eType_None)
		{
			txt += ",";

			if (op.m_type == decoding::Instruction::eType_Reg)
			{
				if (!op.m_reg)
					return false;

				txt += "regs.";
				txt += op.m_reg->GetName();
			}
			else if (op.m_type == decoding::Instruction::eType_Imm)
			{
				char s[16];
				sprintf_s(s, 16, "0x%X", op.m_imm);
				txt += s;
			}		
		}

		return true;
	}

	virtual const bool Decompile(class ILogOutput& output, const uint32 address, const decoding::Instruction& op, const decoding::InstructionExtendedInfo& exInfo, class code::IGenerator& codeGen, std::string& outCode) const
	{
		OpcodeInfo opInfo(op);

		// invalid op
		if ( !op.IsValid() )
			return false;

		// preamble
		outCode = "cpu::op::";
		outCode += opInfo.m_name;
		outCode += opInfo.m_ctrl ? "<1>" : "<0>";

		// parameters
		{
			outCode += "(regs";
			if (!ExportOperandRet(outCode, op.GetArg0())) return false;
			if (!ExportOperand(outCode, op.GetArg1())) return false;
			if (!ExportOperand(outCode, op.GetArg2())) return false;
			if (!ExportOperand(outCode, op.GetArg3())) return false;
			if (!ExportOperand(outCode, op.GetArg4())) return false;
			if (!ExportOperand(outCode, op.GetArg5())) return false;
			outCode += ");";
		}

		return true;
	}
};

//---------------------------------------------------------------------------

template< uint32 flags,
		decoding::Instruction::Type arg0 = decoding::Instruction::eType_None,
		decoding::Instruction::Type arg1 = decoding::Instruction::eType_None,
		decoding::Instruction::Type arg2 = decoding::Instruction::eType_None,
		decoding::Instruction::Type arg3 = decoding::Instruction::eType_None,
		decoding::Instruction::Type arg4 = decoding::Instruction::eType_None,
		decoding::Instruction::Type arg5 = decoding::Instruction::eType_None >
class CInstructiondDecoderXenon_GENERIC_CIMM
	: public CInstructiondDecoderXenon_GENERIC<flags, arg0, arg1, arg2, arg3, arg4, arg5>
{
public:
	virtual const bool Decompile(class ILogOutput& output, const uint32 address, const decoding::Instruction& op, const decoding::InstructionExtendedInfo& exInfo, class code::IGenerator& codeGen, std::string& outCode) const
	{
		OpcodeInfo opInfo(op);

		// invalid op
		if ( !op.IsValid() )
			return false;

		// preamble
		outCode = "cpu::op::";
		outCode += opInfo.m_name;
		outCode += opInfo.m_ctrl ? "<1" : "<0";

		// compile time immediate values
		if (op.GetArg1().m_type == IMM)
			PrintCodef( outCode, ",%d", op.GetArg1().m_imm );
		if (op.GetArg2().m_type == IMM)
			PrintCodef( outCode, ",%d", op.GetArg2().m_imm );
		if (op.GetArg3().m_type == IMM)
			PrintCodef( outCode, ",%d", op.GetArg3().m_imm );
		if (op.GetArg4().m_type == IMM)
			PrintCodef( outCode, ",%d", op.GetArg4().m_imm );
		if (op.GetArg5().m_type == IMM)
			PrintCodef( outCode, ",%d", op.GetArg5().m_imm );

		// end of preamble
		outCode += ">";

		// parameters
		{
			outCode += "(regs";
			if (!ExportOperandRet(outCode, op.GetArg0())) return false;
			if (op.GetArg1().m_type != IMM && !ExportOperand(outCode, op.GetArg1())) return false;
			if (op.GetArg2().m_type != IMM && !ExportOperand(outCode, op.GetArg2())) return false;
			if (op.GetArg3().m_type != IMM && !ExportOperand(outCode, op.GetArg3())) return false;
			if (op.GetArg4().m_type != IMM && !ExportOperand(outCode, op.GetArg4())) return false;
			if (op.GetArg5().m_type != IMM && !ExportOperand(outCode, op.GetArg5())) return false;
			outCode += ");";
		}

		return true;
	}
};

//---------------------------------------------------------------------------

template< const bool Absolute >
class CInstructiondDecoderXenon_B : public IInstructiondDecompilerXenon
{
public:
	CInstructiondDecoderXenon_B()
	{}

	virtual bool GetExtendedInfo(const class decoding::Instruction& op, const uint64 codeAddress, const decoding::Context& context, class decoding::InstructionExtendedInfo& outInfo) const override
	{
		if (!CheckArgs(op, IMM))
			return ErrInvalidArgs(op);

		outInfo.m_codeFlags |= decoding::InstructionExtendedInfo::eInstructionFlag_Jump;
		outInfo.m_codeFlags |= decoding::InstructionExtendedInfo::eInstructionFlag_Static;
		outInfo.m_branchTargetAddress = Absolute ? op.GetArg0().m_imm : (codeAddress + op.GetArg0().m_imm);

		return true;
	}

	virtual const bool Decompile(class ILogOutput& output, const uint32 address, const decoding::Instruction& op, const decoding::InstructionExtendedInfo& exInfo, class code::IGenerator& codeGen, std::string& outCode) const
	{
		if (!CheckArgs(op, IMM))
			return false;

		const uint32 targetAddr = (uint32)exInfo.m_branchTargetAddress;
		PrintCodef(outCode, "return 0x%08X;", targetAddr);
		return true;
	}
};

//---------------------------------------------------------------------------

template< const bool Absolute >
class CInstructiondDecoderXenon_BL : public IInstructiondDecompilerXenon
{
public:
	CInstructiondDecoderXenon_BL()
	{}

	virtual bool GetExtendedInfo(const class decoding::Instruction& op, const uint64 codeAddress, const decoding::Context& context, class decoding::InstructionExtendedInfo& outInfo) const override
	{
		if (!CheckArgs(op, IMM))
			return ErrInvalidArgs(op);

		if (!outInfo.AddRegisterOutput(RegLR))
			return ErrToManyRegisters(op);

		outInfo.m_codeFlags |= decoding::InstructionExtendedInfo::eInstructionFlag_Call;
		outInfo.m_codeFlags |= decoding::InstructionExtendedInfo::eInstructionFlag_Static;
		outInfo.m_branchTargetAddress = Absolute ? op.GetArg0().m_imm : (codeAddress + op.GetArg0().m_imm);
		return true;
	}

	virtual const bool Decompile(class ILogOutput& output, const uint32 address, const decoding::Instruction& op, const decoding::InstructionExtendedInfo& exInfo, class code::IGenerator& codeGen, std::string& outCode) const
	{
		if (!CheckArgs(op, IMM))
			return false;

		const uint32 targetAddr = (uint32)exInfo.m_branchTargetAddress;
		const uint32 nextAddr = address + op.GetCodeSize();
		PrintCodef(outCode, "regs.LR = 0x%08X; return 0x%08X;", nextAddr, targetAddr);
		return true;
	}
};

//---------------------------------------------------------------------------

class CInstructiondDecoderXenon_CMP : public IInstructiondDecompilerXenon
{
public:
	CInstructiondDecoderXenon_CMP()
	{}

	virtual bool GetExtendedInfo(const class decoding::Instruction& op, const uint64 codeAddress, const decoding::Context& context, class decoding::InstructionExtendedInfo& outInfo) const override
	{
		if (CheckArgs(op, REG, REG, IMM))
		{
			if (!outInfo.AddRegisterOutput(op.GetArg0().m_reg))
				return ErrToManyRegisters(op);
			if (!outInfo.AddRegisterDependency(op.GetArg1().m_reg))
				return ErrToManyRegisters(op);

			return true;
		}
		else if (CheckArgs(op, REG, REG, REG))
		{
			if (!outInfo.AddRegisterOutput(op.GetArg0().m_reg))
				return ErrToManyRegisters(op);
			if (!outInfo.AddRegisterDependency(op.GetArg1().m_reg))
				return ErrToManyRegisters(op);
			if (!outInfo.AddRegisterDependency(op.GetArg2().m_reg))
				return ErrToManyRegisters(op);

			return true;
		}

		return ErrInvalidArgs(op);
	}

	virtual const bool Decompile(class ILogOutput& output, const uint32 address, const decoding::Instruction& op, const decoding::InstructionExtendedInfo& exInfo, class code::IGenerator& codeGen, std::string& outCode) const
	{
		const OpcodeInfo opInfo(op);

		if (CheckArgs(op, REG, REG, IMM))
		{
			const int crIndex = atoi(op.GetArg0().m_reg->GetName() + 2);

			PrintCodef( outCode, "cpu::op::%s<%d>(regs,regs.%s,0x%08X);", 
				opInfo.m_name,
				crIndex, 
				op.GetArg1().m_reg->GetName(),
				op.GetArg2().m_imm );

			return true;
		}
		else if (CheckArgs(op, REG, REG, REG))
		{
			const int crIndex = atoi(op.GetArg0().m_reg->GetName() + 2);

			PrintCodef( outCode, "cpu::op::%s<%d>(regs,regs.%s,regs.%s);", 
				opInfo.m_name,
				crIndex, 
				op.GetArg1().m_reg->GetName(),
				op.GetArg2().m_reg->GetName() );

			return true;
		}

		return false;
	}
};

//---------------------------------------------------------------------------

class CInstructiondDecoderXenon_FCMP : public IInstructiondDecompilerXenon
{
public:
	CInstructiondDecoderXenon_FCMP()
	{}

	virtual bool GetExtendedInfo(const class decoding::Instruction& op, const uint64 codeAddress, const decoding::Context& context, class decoding::InstructionExtendedInfo& outInfo) const override
	{
		if (CheckArgs(op, REG, REG, REG))
		{
			if (!outInfo.AddRegisterOutput(op.GetArg0().m_reg))
				return ErrToManyRegisters(op);
			if (!outInfo.AddRegisterDependency(op.GetArg1().m_reg))
				return ErrToManyRegisters(op);
			if (!outInfo.AddRegisterDependency(op.GetArg2().m_reg))
				return ErrToManyRegisters(op);

			return true;
		}

		return ErrInvalidArgs(op);
	}

	virtual const bool Decompile(class ILogOutput& output, const uint32 address, const decoding::Instruction& op, const decoding::InstructionExtendedInfo& exInfo, class code::IGenerator& codeGen, std::string& outCode) const
	{
		if (!CheckArgs(op, REG, REG, REG))
			return false;

		const int crIndex = atoi( op.GetArg0().m_reg->GetName() + 2 );

		const OpcodeInfo opInfo(op);

		PrintCodef( outCode, "cpu::op::%s<%d>(regs,regs.%s,regs.%s);", 
			opInfo.m_name,
			crIndex, 
			op.GetArg1().m_reg->GetName(),
			op.GetArg2().m_reg->GetName() );

		return true;
	}
};

//---------------------------------------------------------------------------

template< const uint32 BitSize >
class CInstructiondDecoderXenon_TRAP : public IInstructiondDecompilerXenon
{
public:
	CInstructiondDecoderXenon_TRAP()
	{}

	virtual bool GetExtendedInfo(const class decoding::Instruction& op, const uint64 codeAddress, const decoding::Context& context, class decoding::InstructionExtendedInfo& outInfo) const override
	{
		if (!CheckArgs(op, IMM, REG, IMM))
			return ErrInvalidArgs(op);

		if (!outInfo.AddRegisterDependency(op.GetArg1().m_reg))
			return ErrToManyRegisters(op);

		return true;
	}

	virtual const bool Decompile(class ILogOutput& output, const uint32 address, const decoding::Instruction& op, const decoding::InstructionExtendedInfo& exInfo, class code::IGenerator& codeGen, std::string& outCode) const
	{
		if (!CheckArgs(op, IMM, REG, IMM))
			return false;

		const uint32 flags = op.GetArg0().m_imm & 0x1F;
		if (flags == 0x1F)
		{
			PrintCodef(outCode, "cpu::op::trap(regs, 0x%08X, regs.%s, 0x%08X);",
				address,
				op.GetArg1().m_reg->GetName(),
				op.GetArg2().m_imm);

			return true;
		}

		// generic trap
		PrintCodef( outCode, "cpu::op::t%s<%d>(regs, 0x%08X, regs.%s, 0x%08X);",
			BitSize == 32 ? "w" : "d",
			flags,
			address,
			op.GetArg1().m_reg->GetName(),
			op.GetArg2().m_imm );

		return true;
	}
};


template< const uint32 BitSize >
class CInstructiondDecoderXenon_TRAP_REG : public IInstructiondDecompilerXenon
{
public:
	CInstructiondDecoderXenon_TRAP_REG()
	{}

	virtual bool GetExtendedInfo(const class decoding::Instruction& op, const uint64 codeAddress, const decoding::Context& context, class decoding::InstructionExtendedInfo& outInfo) const override
	{
		if (!CheckArgs(op, IMM, REG, REG))
			return ErrInvalidArgs(op);

		if (!outInfo.AddRegisterDependency(op.GetArg1().m_reg))
			return ErrToManyRegisters(op);

		if (!outInfo.AddRegisterDependency(op.GetArg2().m_reg))
			return ErrToManyRegisters(op);

		return true;
	}

	virtual const bool Decompile(class ILogOutput& output, const uint32 address, const decoding::Instruction& op, const decoding::InstructionExtendedInfo& exInfo, class code::IGenerator& codeGen, std::string& outCode) const
	{
		if (!CheckArgs(op, IMM, REG, REG))
			return false;

		// trap always - sys call
		const uint32 flags = op.GetArg0().m_imm & 0x1F;
		if (flags == 0x1F)
		{
			// general trap
			PrintCodef(outCode, "cpu::op::trap(regs, 0x%08X, regs.%s, regs.%s);",
				BitSize == 32 ? "w" : "d",
				address,
				op.GetArg1().m_reg->GetName(),
				op.GetArg2().m_reg->GetName());
			return true;
		}

		// general trap
		PrintCodef( outCode, "cpu::op::t%s<%d>(regs, 0x%08X, regs.%s, regs.%s);",
			BitSize == 32 ? "w" : "d",
			flags,
			address,
			op.GetArg1().m_reg->GetName(),
			op.GetArg2().m_reg->GetName() );

		return true;
	}
};

//---------------------------------------------------------------------------

static inline const bool GenerateBranchCode(const uint32 address, const uint32 branchType, const char* ctrRegName, const char* branchCode, const bool simple, std::string& outBranchCode)
{
	/*
	if (64-bit mode)
	then M  0
	else M  32
	if ¬BO2 then CTR  CTR - 1
	ctr_ok  BO2 | ((CTRM:63  0)  BO3)
	cond_ok  BO0 | (CRBI  BO1)
	if ctr_ok & cond_ok then
	if AA then NIA iea EXTS(BD || 0b00)
	else NIA iea CIA + EXTS(BD || 0b00)
	if LK then LR iea CIA + 4
	*/

	// extract flags
	const bool b0 = (branchType & 16) != 0;
	const bool b1 = (branchType & 8) != 0;
	const bool b2 = (branchType & 4) != 0;
	const bool b3 = (branchType & 2) != 0;
	const bool b4 = (branchType & 1) != 0;
	
	// CTR decrement
	const bool ctrTest = !simple && !b2; // decrement and test the CTR
	const char* ctrCmp = b3 ? "==" : "!=";

	// Flag negation
	const bool flagTest = (b0 == 0);
	const char* flagCmp = b1 ? "" : "!";

	// b2==0 Decrement the CTR
	if (!b2)
		PrintCodef( outBranchCode, "regs.CTR -= 1;\n"); 

	// branch code
	if (ctrTest && flagTest)
		PrintCodef( outBranchCode, "if ( (uint32)regs.CTR %s 0 && %sregs.%s ) { %s }", ctrCmp, flagCmp, ctrRegName, branchCode );
	else if (ctrTest && !flagTest)
		PrintCodef( outBranchCode, "if ( (uint32)regs.CTR %s 0 ) { %s }", ctrCmp, branchCode );
	else if (!ctrTest && flagTest)
		PrintCodef( outBranchCode, "if ( %sregs.%s ) { %s }", flagCmp, ctrRegName, branchCode );
	else if (!ctrTest && !flagTest)
		PrintCodef( outBranchCode, "if ( 1 ) { %s }", branchCode );

	return true;
}



template< bool L, bool A >
class CInstructiondDecoderXenon_BC : public IInstructiondDecompilerXenon
{
public:
	CInstructiondDecoderXenon_BC()
	{}

	virtual bool GetExtendedText(const class decoding::Instruction& instr, const uint64 codeAddress, char* outText, const uint32 outTextSize) const
	{
		// get the branch code
		if (GenerateBranchInfo(instr, codeAddress, outText, outTextSize))
			return true;

		// no extra text
		return false;
	}

	virtual bool GetExtendedInfo(const class decoding::Instruction& op, const uint64 codeAddress, const decoding::Context& context, class decoding::InstructionExtendedInfo& outInfo) const override
	{
		if (!CheckArgs(op, IMM, REG, IMM))
			return ErrInvalidArgs(op);

		if (!op.GetArg1().m_reg->GetParent())
			return ErrInvalidArgs(op);

		// the call modifies the LR register
		if ( L && !outInfo.AddRegisterOutput(RegLR))
			return ErrToManyRegisters(op);

		// CTR dependency
		const uint32 branchType = op.GetArg0().m_imm;
		const bool ctrTest = (branchType & 4) == 0;
		if (ctrTest)
		{
			if ( !outInfo.AddRegisterOutput(RegCTR))
				return ErrToManyRegisters(op);
			if ( !outInfo.AddRegisterDependency(RegCTR))
				return ErrToManyRegisters(op);
		}

		// Flag dependency
		const bool flagTest = (branchType & 16) == 0;
		if (flagTest)
		{
			// dependency on the control register
			if ( !outInfo.AddRegisterDependency(op.GetArg1().m_reg))
				return ErrToManyRegisters(op);
		}

		// setup flags
		outInfo.m_codeFlags |= L ? 
			decoding::InstructionExtendedInfo::eInstructionFlag_Call :
			decoding::InstructionExtendedInfo::eInstructionFlag_Jump;
		outInfo.m_codeFlags |= (flagTest || ctrTest) ? decoding::InstructionExtendedInfo::eInstructionFlag_Conditional : 0;
		outInfo.m_codeFlags |= decoding::InstructionExtendedInfo::eInstructionFlag_Diverge;
		outInfo.m_codeFlags |= A ?
			decoding::InstructionExtendedInfo::eInstructionFlag_Static : // absolute jump/call address
			0;

		// compute branch target
		outInfo.m_branchTargetAddress = A ? op.GetArg2().m_imm : (codeAddress + op.GetArg2().m_imm);
		return true;
	}

	virtual const bool Decompile(class ILogOutput& output, const uint32 address, const decoding::Instruction& op, const decoding::InstructionExtendedInfo& exInfo, class code::IGenerator& codeGen, std::string& outCode) const
	{
		if (!CheckArgs(op, IMM, REG, IMM) || !op.GetArg1().m_reg->GetParent())
			return false;

		const int branchType = op.GetArg0().m_imm;

		// format the full test register name
		char ctrRegName[16];
		{
			const int ctrReg = op.GetArg1().m_reg->GetParent()->GetChildIndex();
			const int ctrTypeReg = op.GetArg1().m_reg->GetChildIndex();
			const char* ctrFlags[4] = { ".lt", ".gt", ".eq", ".so" };
			sprintf_s( ctrRegName, sizeof(ctrRegName), "CR[%d]%s", ctrReg, ctrFlags[ ctrTypeReg ] );
		}

		// format the full jump code
		char branchCode[128];
		if (L)
		{
			sprintf_s(branchCode, sizeof(branchCode), "regs.LR = 0x%08X; return 0x%08X; %s", 
				(uint32)(address+4), 
				(uint32)(exInfo.m_branchTargetAddress),
				A ? "/*ABSOLUTE*/" : "" );
		}
		else
		{
			sprintf_s( branchCode, sizeof(branchCode), "return 0x%08X; %s", 
				(uint32)(exInfo.m_branchTargetAddress),
				A ? "/*ABSOLUTE*/" : "" );
		}

		GenerateBranchCode(address, branchType, ctrRegName, branchCode, false, outCode);
		return true;
	}
};

template< bool L >
class CInstructiondDecoderXenon_BCLR : public IInstructiondDecompilerXenon
{
public:
	CInstructiondDecoderXenon_BCLR()
	{}

	virtual bool GetExtendedText(const class decoding::Instruction& instr, const uint64 codeAddress, char* outText, const uint32 outTextSize) const
	{
		// get the branch code
		if (GenerateBranchInfo(instr, codeAddress, outText, outTextSize))
			return true;

		// no extra text
		return false;
	}

	virtual bool GetExtendedInfo(const class decoding::Instruction& op, const uint64 codeAddress, const decoding::Context& context, class decoding::InstructionExtendedInfo& outInfo) const override
	{
		if (!CheckArgs(op, IMM, REG))
			return ErrInvalidArgs(op);

		if (!op.GetArg1().m_reg->GetParent())
			return ErrInvalidArgs(op);

		// we always depend on the LR here
		if ( !outInfo.AddRegisterDependency(RegLR))
			return ErrToManyRegisters(op);

		// the call modifies the LR register
		if ( L && !outInfo.AddRegisterOutput(RegLR))
			return ErrToManyRegisters(op);

		// CTR dependency
		const uint32 branchType = op.GetArg0().m_imm;
		const bool ctrTest = (branchType & 4) == 0;
		if (ctrTest)
		{
			if ( !outInfo.AddRegisterOutput(RegCTR))
				return ErrToManyRegisters(op);
			if ( !outInfo.AddRegisterDependency(RegCTR))
				return ErrToManyRegisters(op);
		}

		// Flag dependency
		const bool flagTest = (branchType & 16) == 0;
		if (flagTest)
		{
			// dependency on the control register
			if ( !outInfo.AddRegisterDependency(op.GetArg1().m_reg))
				return ErrToManyRegisters(op);
		}

		// setup flags
		outInfo.m_codeFlags |= L ? 
			decoding::InstructionExtendedInfo::eInstructionFlag_Call :
			decoding::InstructionExtendedInfo::eInstructionFlag_Return;
		outInfo.m_codeFlags |= (flagTest || ctrTest) ? decoding::InstructionExtendedInfo::eInstructionFlag_Conditional : 0;
		outInfo.m_codeFlags |= decoding::InstructionExtendedInfo::eInstructionFlag_Diverge;

		// compute branch target
		outInfo.m_branchTargetAddress = 0;
		outInfo.m_branchTargetReg = RegLR;
		return true;
	}

	virtual const bool Decompile(class ILogOutput& output, const uint32 address, const decoding::Instruction& op, const decoding::InstructionExtendedInfo& exInfo, class code::IGenerator& codeGen, std::string& outCode) const
	{
		if (!CheckArgs(op, IMM, REG) || !op.GetArg1().m_reg->GetParent())
			return false;

		const int branchType = op.GetArg0().m_imm;

		// format the full test register name
		char ctrRegName[16];
		{
			const int ctrReg = op.GetArg1().m_reg->GetParent()->GetChildIndex();
			const int ctrTypeReg = op.GetArg1().m_reg->GetChildIndex();
			const char* ctrFlags[4] = { ".lt", ".gt", ".eq", ".so" };
			sprintf_s( ctrRegName, sizeof(ctrRegName), "CR[%d]%s", ctrReg, ctrFlags[ ctrTypeReg ] );
		}

		// format the full jump code
		char branchCode[128];
		if (L)
		{
			sprintf_s(branchCode, sizeof(branchCode), "auto tempLR = regs.LR; regs.LR = 0x%08X; return (uint32)tempLR;",
				address+4 );
		}
		else
		{
			sprintf_s(branchCode, sizeof(branchCode), "return (uint32)regs.LR;");
		}

		GenerateBranchCode(address, branchType, ctrRegName, branchCode, false, outCode);
		return true;
	}
};

template< bool L >
class CInstructiondDecoderXenon_BCCTRR : public IInstructiondDecompilerXenon
{
public:
	CInstructiondDecoderXenon_BCCTRR()
	{}

	virtual bool GetExtendedText(const class decoding::Instruction& instr, const uint64 codeAddress, char* outText, const uint32 outTextSize) const
	{
		// get the branch code
		if (GenerateBranchInfo(instr, codeAddress, outText, outTextSize))
			return true;

		// no extra text
		return false;
	}

	virtual bool GetExtendedInfo(const class decoding::Instruction& op, const uint64 codeAddress, const decoding::Context& context, class decoding::InstructionExtendedInfo& outInfo) const override
	{
		if (!CheckArgs(op, IMM, REG))
			return ErrInvalidArgs(op);

		if (!op.GetArg1().m_reg->GetParent())
			return ErrInvalidArgs(op);

		// we always depend on the CTR
		if ( !outInfo.AddRegisterDependency(RegCTR))
			return ErrToManyRegisters(op);

		// the call modifies the LR register
		if ( L && !outInfo.AddRegisterOutput(RegLR))
			return ErrToManyRegisters(op);
		
		// invalid
		const uint32 branchType = op.GetArg0().m_imm;
		const bool ctrTest = (branchType & 4) == 0;
		if (ctrTest)
			return ErrInvalidArgs(op);

		// Flag dependency
		const bool flagTest = (branchType & 16) == 0;
		if (flagTest)
		{
			// dependency on the control register
			if ( !outInfo.AddRegisterDependency(op.GetArg1().m_reg))
				return ErrToManyRegisters(op);
		}

		// setup flags
		outInfo.m_codeFlags |= L ? 
			decoding::InstructionExtendedInfo::eInstructionFlag_Call :
			decoding::InstructionExtendedInfo::eInstructionFlag_Jump;
		outInfo.m_codeFlags |= flagTest ? decoding::InstructionExtendedInfo::eInstructionFlag_Conditional : 0;
		outInfo.m_codeFlags |= decoding::InstructionExtendedInfo::eInstructionFlag_Diverge;

		// compute branch target
		outInfo.m_branchTargetAddress = 0;
		outInfo.m_branchTargetReg = RegCTR;
		return true;
	}

	virtual const bool Decompile(class ILogOutput& output, const uint32 address, const decoding::Instruction& op, const decoding::InstructionExtendedInfo& exInfo, class code::IGenerator& codeGen, std::string& outCode) const
	{
		if (!CheckArgs(op, IMM, REG) || !op.GetArg1().m_reg->GetParent())
			return false;

		const int branchType = op.GetArg0().m_imm;

		// format the full test register name
		char ctrRegName[16];
		{
			const int ctrReg = op.GetArg1().m_reg->GetParent()->GetChildIndex();
			const int ctrTypeReg = op.GetArg1().m_reg->GetChildIndex();
			const char* ctrFlags[4] = { ".lt", ".gt", ".eq", ".so" };
			sprintf_s( ctrRegName, sizeof(ctrRegName), "CR[%d]%s", ctrReg, ctrFlags[ ctrTypeReg ] );
		}

		// format the full jump code
		char branchCode[128];
		if (L)
		{
			sprintf_s(branchCode, sizeof(branchCode), "regs.LR = 0x%08X; return (uint32)regs.CTR;",
				address+4 );
		}
		else
		{
			sprintf_s(branchCode, sizeof(branchCode), "return (uint32)regs.CTR;");
		}

		GenerateBranchCode(address, branchType, ctrRegName, branchCode, true, outCode);
		return true;
	}
};

//---------------------------------------------------------------------------

template< uint32 size, uint32 flags >
class CInstructiondDecoderXenon_SRAI : public IInstructiondDecompilerXenon
{
public:
	virtual bool GetExtendedInfo(const class decoding::Instruction& op, const uint64 codeAddress, const decoding::Context& context, class decoding::InstructionExtendedInfo& outInfo) const override
	{
		if (!CheckArgs(op, REG, REG, IMM))
			return ErrInvalidArgs(op);

		if (!AddRegs<flags>(outInfo))
			return false;

		if (!outInfo.AddRegisterOutput(op.GetArg0().m_reg))
			return ErrToManyRegisters(op);
		if (!outInfo.AddRegisterDependency(op.GetArg1().m_reg))
			return ErrToManyRegisters(op);

		return true;
	}

	virtual const bool Decompile(class ILogOutput& output, const uint32 address, const decoding::Instruction& op, const decoding::InstructionExtendedInfo& exInfo, class code::IGenerator& codeGen, std::string& outCode) const
	{
		if (!CheckArgs(op, REG, REG, IMM))
			return false;

		const OpcodeInfo opInfo(op);

		PrintCodef(outCode, "cpu::op::%s<%d,%d>(regs,&regs.%s,regs.%s);", 
			opInfo.m_name,
			opInfo.m_ctrl ? 1 : 0,
			op.GetArg2().m_imm, // shift val
			op.GetArg0().m_reg->GetName(),
			op.GetArg1().m_reg->GetName());

		return true;
	}
};

//---------------------------------------------------------------------------

template< uint32 size, uint32 flags >
class CInstructiondDecoderXenon_RL : public IInstructiondDecompilerXenon
{
public:
	CInstructiondDecoderXenon_RL()
	{}

	virtual bool GetExtendedInfo(const class decoding::Instruction& op, const uint64 codeAddress, const decoding::Context& context, class decoding::InstructionExtendedInfo& outInfo) const override
	{
		if (!CheckArgs(op, REG, REG, REG, IMM, IMM))
			return ErrInvalidArgs(op);

		if (!AddRegs<flags>(outInfo))
			return ErrToManyRegisters(op);

		if (!outInfo.AddRegisterOutput(op.GetArg0().m_reg))
			return ErrToManyRegisters(op);
		if (!outInfo.AddRegisterDependency(op.GetArg1().m_reg))
			return ErrToManyRegisters(op);
		if (!outInfo.AddRegisterDependency(op.GetArg2().m_reg))
			return ErrToManyRegisters(op);

		return true;
	}

	virtual const bool Decompile(class ILogOutput& output, const uint32 address, const decoding::Instruction& op, const decoding::InstructionExtendedInfo& exInfo, class code::IGenerator& codeGen, std::string& outCode) const
	{
		if (!CheckArgs(op, REG, REG, REG, IMM, IMM))
			return false;

		OpcodeInfo opInfo(op);

		PrintCodef(outCode, "cpu::op::%s<%d,%d,%d>(regs,&regs.%s,regs.%s,regs.%s);", 
			opInfo.m_name,
			opInfo.m_ctrl ? 1 : 0,
			op.GetArg3().m_imm,
			op.GetArg4().m_imm,
			op.GetArg0().m_reg->GetName(),
			op.GetArg1().m_reg->GetName(),
			op.GetArg2().m_reg->GetName() );

		return true;
	}
};

template< uint32 size, uint32 flags >
class CInstructiondDecoderXenon_RLI : public IInstructiondDecompilerXenon
{
public:
	CInstructiondDecoderXenon_RLI()
	{}

	virtual bool GetExtendedInfo(const class decoding::Instruction& op, const uint64 codeAddress, const decoding::Context& context, class decoding::InstructionExtendedInfo& outInfo) const override
	{
		if (!CheckArgs(op, REG, REG, IMM, IMM))
			return ErrInvalidArgs(op);

		if (!AddRegs<flags>(outInfo))
			return ErrToManyRegisters(op);

		if (!outInfo.AddRegisterOutput(op.GetArg0().m_reg))
			return ErrToManyRegisters(op);
		if (!outInfo.AddRegisterDependency(op.GetArg1().m_reg))
			return ErrToManyRegisters(op);

		return true;
	}

	virtual const bool Decompile(class ILogOutput& output, const uint32 address, const decoding::Instruction& op, const decoding::InstructionExtendedInfo& exInfo, class code::IGenerator& codeGen, std::string& outCode) const
	{
		if (!CheckArgs(op, REG, REG, IMM, IMM))
			return false;

		OpcodeInfo opInfo(op);

		PrintCodef(outCode, "cpu::op::%s<%d,%d,%d>(regs,&regs.%s,regs.%s);", 
			opInfo.m_name,
			opInfo.m_ctrl ? 1 : 0,
			op.GetArg2().m_imm,
			op.GetArg3().m_imm,
			op.GetArg0().m_reg->GetName(),
			op.GetArg1().m_reg->GetName());

		return true;
	}
};

template< uint32 size, uint32 flags >
class CInstructiondDecoderXenon_RLIM : public IInstructiondDecompilerXenon
{
public:
	CInstructiondDecoderXenon_RLIM()
	{}

	virtual bool GetExtendedInfo(const class decoding::Instruction& op, const uint64 codeAddress, const decoding::Context& context, class decoding::InstructionExtendedInfo& outInfo) const override
	{
		if (!CheckArgs(op, REG, REG, IMM, IMM, IMM))
			return ErrInvalidArgs(op);

		if (!AddRegs<flags>(outInfo))
			return ErrToManyRegisters(op);

		if (!outInfo.AddRegisterOutput(op.GetArg0().m_reg))
			return ErrToManyRegisters(op);
		if (!outInfo.AddRegisterDependency(op.GetArg1().m_reg))
			return ErrToManyRegisters(op);

		return true;
	}

	virtual const bool Decompile(class ILogOutput& output, const uint32 address, const decoding::Instruction& op, const decoding::InstructionExtendedInfo& exInfo, class code::IGenerator& codeGen, std::string& outCode) const
	{
		if (!CheckArgs(op, REG, REG, IMM, IMM, IMM))
			return false;

		OpcodeInfo opInfo(op);

		PrintCodef(outCode, "cpu::op::%s<%d,%d,%d,%d>(regs,&regs.%s,regs.%s);", 
			opInfo.m_name,
			opInfo.m_ctrl ? 1 : 0,
			op.GetArg2().m_imm,
			op.GetArg3().m_imm,
			op.GetArg4().m_imm,
			op.GetArg0().m_reg->GetName(),
			op.GetArg1().m_reg->GetName());

		return true;
	}
};

template< uint32 size, uint32 flags >
class CInstructiondDecoderXenon_RLM : public IInstructiondDecompilerXenon
{
public:
	CInstructiondDecoderXenon_RLM()
	{}

	virtual bool GetExtendedInfo(const class decoding::Instruction& op, const uint64 codeAddress, const decoding::Context& context, class decoding::InstructionExtendedInfo& outInfo) const override
	{
		if (!CheckArgs(op, REG, REG, REG, IMM, IMM))
			return ErrInvalidArgs(op);

		if (!AddRegs<flags>(outInfo))
			return ErrToManyRegisters(op);

		if (!outInfo.AddRegisterOutput(op.GetArg0().m_reg))
			return ErrToManyRegisters(op);
		if (!outInfo.AddRegisterDependency(op.GetArg1().m_reg))
			return ErrToManyRegisters(op);
		if (!outInfo.AddRegisterDependency(op.GetArg2().m_reg))
			return ErrToManyRegisters(op);

		return true;
	}

	virtual const bool Decompile(class ILogOutput& output, const uint32 address, const decoding::Instruction& op, const decoding::InstructionExtendedInfo& exInfo, class code::IGenerator& codeGen, std::string& outCode) const
	{
		if (!CheckArgs(op, REG, REG, REG, IMM, IMM))
			return false;

		OpcodeInfo opInfo(op);
		
		PrintCodef(outCode, "cpu::op::%s<%d,%d>(regs,&regs.%s,regs.%s,regs.%s);", 
			opInfo.m_name,
			opInfo.m_ctrl ? 1 : 0,
			op.GetArg3().m_imm,
			op.GetArg0().m_reg->GetName(),
			op.GetArg1().m_reg->GetName(),
			op.GetArg2().m_reg->GetName());

		return true;
	}
};

//---------------------------------------------------------------------------

////#pragma optimize("",off)

CPU_XenonPPC::CPU_XenonPPC()
	: platform::CPU("ppc", this)
{
	// cleanup
	memset(m_regMap, 0, sizeof(m_regMap));
	memset(m_opMap, 0, sizeof(m_opMap));

	// create registers
	{
		//const CPURegister* AddRootRegister(const char* name, const int nativeIndex, const uint32 bitSize, const CPURegisterType type);
		//const CPURegister* AddChildRegister(const char* parentName, const char* name, const int nativeIndex, const uint32 bitSize, const uint32 bitOffset, const CPURegisterType type);

		#define ADD_REG( name, bitSize, regType ) m_regMap[eRegister_##name] = AddRootRegister( #name, eRegister_##name, bitSize, regType )
		#define ADD_CHILD_REG( parent, name, bitSize, bitOffset, regType ) m_regMap[eRegister_##name] = AddChildRegister( #parent, #name, eRegister_##name, bitSize, bitOffset, regType )
		#undef CR0
		#undef CR1

		ADD_REG( LR, 32, platform::CPURegisterType::Control );
		ADD_REG( CTR, 64, platform::CPURegisterType::Control);
		ADD_REG( MSR, 64, platform::CPURegisterType::Control);
		ADD_REG( CR, 32, platform::CPURegisterType::Control);

		ADD_CHILD_REG(CR, CR0, 4, 0, platform::CPURegisterType::Control);
		ADD_CHILD_REG(CR, CR1, 4, 4, platform::CPURegisterType::Control);
		ADD_CHILD_REG(CR, CR2, 4, 8, platform::CPURegisterType::Control);
		ADD_CHILD_REG(CR, CR3, 4, 12, platform::CPURegisterType::Control);
		ADD_CHILD_REG(CR, CR4, 4, 16, platform::CPURegisterType::Control);
		ADD_CHILD_REG(CR, CR5, 4, 20, platform::CPURegisterType::Control);
		ADD_CHILD_REG(CR, CR6, 4, 24, platform::CPURegisterType::Control);
		ADD_CHILD_REG(CR, CR7, 4, 28, platform::CPURegisterType::Control);

		ADD_CHILD_REG(CR0, CR0_LT, 1, 0, platform::CPURegisterType::Control);
		ADD_CHILD_REG(CR0, CR0_GT, 1, 1, platform::CPURegisterType::Control);
		ADD_CHILD_REG(CR0, CR0_EQ, 1, 2, platform::CPURegisterType::Control);
		ADD_CHILD_REG(CR0, CR0_SO, 1, 3, platform::CPURegisterType::Control);
		ADD_CHILD_REG(CR1, CR1_LT, 1, 0, platform::CPURegisterType::Control);
		ADD_CHILD_REG(CR1, CR1_GT, 1, 1, platform::CPURegisterType::Control);
		ADD_CHILD_REG(CR1, CR1_EQ, 1, 2, platform::CPURegisterType::Control);
		ADD_CHILD_REG(CR1, CR1_SO, 1, 3, platform::CPURegisterType::Control);
		ADD_CHILD_REG(CR2, CR2_LT, 1, 0, platform::CPURegisterType::Control);
		ADD_CHILD_REG(CR2, CR2_GT, 1, 1, platform::CPURegisterType::Control);
		ADD_CHILD_REG(CR2, CR2_EQ, 1, 2, platform::CPURegisterType::Control);
		ADD_CHILD_REG(CR2, CR2_SO, 1, 3, platform::CPURegisterType::Control);
		ADD_CHILD_REG(CR3, CR3_LT, 1, 0, platform::CPURegisterType::Control);
		ADD_CHILD_REG(CR3, CR3_GT, 1, 1, platform::CPURegisterType::Control);
		ADD_CHILD_REG(CR3, CR3_EQ, 1, 2, platform::CPURegisterType::Control);
		ADD_CHILD_REG(CR3, CR3_SO, 1, 3, platform::CPURegisterType::Control);
		ADD_CHILD_REG(CR4, CR4_LT, 1, 0, platform::CPURegisterType::Control);
		ADD_CHILD_REG(CR4, CR4_GT, 1, 1, platform::CPURegisterType::Control);
		ADD_CHILD_REG(CR4, CR4_EQ, 1, 2, platform::CPURegisterType::Control);
		ADD_CHILD_REG(CR4, CR4_SO, 1, 3, platform::CPURegisterType::Control);
		ADD_CHILD_REG(CR5, CR5_LT, 1, 0, platform::CPURegisterType::Control);
		ADD_CHILD_REG(CR5, CR5_GT, 1, 1, platform::CPURegisterType::Control);
		ADD_CHILD_REG(CR5, CR5_EQ, 1, 2, platform::CPURegisterType::Control);
		ADD_CHILD_REG(CR5, CR5_SO, 1, 3, platform::CPURegisterType::Control);
		ADD_CHILD_REG(CR6, CR6_LT, 1, 0, platform::CPURegisterType::Control);
		ADD_CHILD_REG(CR6, CR6_GT, 1, 1, platform::CPURegisterType::Control);
		ADD_CHILD_REG(CR6, CR6_EQ, 1, 2, platform::CPURegisterType::Control);
		ADD_CHILD_REG(CR6, CR6_SO, 1, 3, platform::CPURegisterType::Control);
		ADD_CHILD_REG(CR7, CR7_LT, 1, 0, platform::CPURegisterType::Control);
		ADD_CHILD_REG(CR7, CR7_GT, 1, 1, platform::CPURegisterType::Control);
		ADD_CHILD_REG(CR7, CR7_EQ, 1, 2, platform::CPURegisterType::Control);
		ADD_CHILD_REG(CR7, CR7_SO, 1, 3, platform::CPURegisterType::Control);

		ADD_REG( XER, 64, platform::CPURegisterType::Control );
		ADD_CHILD_REG( XER, XER_CA, 1, 0, platform::CPURegisterType::Control );
		ADD_CHILD_REG( XER, XER_OV, 1, 1, platform::CPURegisterType::Control );
		ADD_CHILD_REG( XER, XER_SO, 1, 2, platform::CPURegisterType::Control );

		ADD_REG( FPSCR, 32, platform::CPURegisterType::Control );

		ADD_CHILD_REG( FPSCR, FPSCR0, 4, 0, platform::CPURegisterType::Control );
		ADD_CHILD_REG( FPSCR, FPSCR1, 4, 1, platform::CPURegisterType::Control );
		ADD_CHILD_REG( FPSCR, FPSCR2, 4, 2, platform::CPURegisterType::Control );
		ADD_CHILD_REG( FPSCR, FPSCR3, 4, 3, platform::CPURegisterType::Control );
		ADD_CHILD_REG( FPSCR, FPSCR4, 4, 4, platform::CPURegisterType::Control );
		ADD_CHILD_REG( FPSCR, FPSCR5, 4, 5, platform::CPURegisterType::Control );
		ADD_CHILD_REG( FPSCR, FPSCR6, 4, 6, platform::CPURegisterType::Control );
		ADD_CHILD_REG( FPSCR, FPSCR7, 4, 7, platform::CPURegisterType::Control );

		ADD_CHILD_REG(FPSCR0, FPSCR0_FX, 1, 0, platform::CPURegisterType::Control);
		ADD_CHILD_REG(FPSCR0, FPSCR0_FEX, 1, 1, platform::CPURegisterType::Control);
		ADD_CHILD_REG(FPSCR0, FPSCR0_VX, 1, 2, platform::CPURegisterType::Control);
		ADD_CHILD_REG(FPSCR0, FPSCR0_OX, 1, 3, platform::CPURegisterType::Control);
		ADD_CHILD_REG(FPSCR1, FPSCR1_UX, 1, 0, platform::CPURegisterType::Control);
		ADD_CHILD_REG(FPSCR1, FPSCR1_ZX, 1, 1, platform::CPURegisterType::Control);
		ADD_CHILD_REG(FPSCR1, FPSCR1_XX, 1, 2, platform::CPURegisterType::Control);
		ADD_CHILD_REG(FPSCR1, FPSCR1_VXSNAN, 1, 3, platform::CPURegisterType::Control);
		ADD_CHILD_REG(FPSCR2, FPSCR2_VXISI, 1, 0, platform::CPURegisterType::Control);
		ADD_CHILD_REG(FPSCR2, FPSCR2_VXIDI, 1, 1, platform::CPURegisterType::Control);
		ADD_CHILD_REG(FPSCR2, FPSCR2_VXZDZ, 1, 2, platform::CPURegisterType::Control);
		ADD_CHILD_REG(FPSCR2, FPSCR2_VXIMZ, 1, 3, platform::CPURegisterType::Control);
		ADD_CHILD_REG(FPSCR3, FPSCR3_VXVC, 1, 0, platform::CPURegisterType::Control);
		ADD_CHILD_REG(FPSCR3, FPSCR3_FR, 1, 1, platform::CPURegisterType::Control);
		ADD_CHILD_REG(FPSCR3, FPSCR3_FI, 1, 2, platform::CPURegisterType::Control);
		ADD_CHILD_REG(FPSCR3, FPSCR3_C, 1, 3, platform::CPURegisterType::Control);
		ADD_CHILD_REG(FPSCR4, FPSCR4_FL, 1, 0, platform::CPURegisterType::Control);
		ADD_CHILD_REG(FPSCR4, FPSCR4_FG, 1, 1, platform::CPURegisterType::Control);
		ADD_CHILD_REG(FPSCR4, FPSCR4_FE, 1, 2, platform::CPURegisterType::Control);
		ADD_CHILD_REG(FPSCR4, FPSCR4_FU, 1, 3, platform::CPURegisterType::Control);
		ADD_CHILD_REG(FPSCR5, FPSCR5_RES, 1, 0, platform::CPURegisterType::Control);
		ADD_CHILD_REG(FPSCR5, FPSCR5_VXSOFT, 1, 1, platform::CPURegisterType::Control);
		ADD_CHILD_REG(FPSCR5, FPSCR5_VXSQRT, 1, 2, platform::CPURegisterType::Control);
		ADD_CHILD_REG(FPSCR5, FPSCR5_VXCVI, 1, 3, platform::CPURegisterType::Control);
		ADD_CHILD_REG(FPSCR6, FPSCR6_VE, 1, 0, platform::CPURegisterType::Control);
		ADD_CHILD_REG(FPSCR6, FPSCR6_VO, 1, 1, platform::CPURegisterType::Control);
		ADD_CHILD_REG(FPSCR6, FPSCR6_UE, 1, 2, platform::CPURegisterType::Control);
		ADD_CHILD_REG(FPSCR6, FPSCR6_ZE, 1, 3, platform::CPURegisterType::Control);
		ADD_CHILD_REG(FPSCR7, FPSCR7_XE, 1, 0, platform::CPURegisterType::Control);
		ADD_CHILD_REG(FPSCR7, FPSCR7_NI, 1, 1, platform::CPURegisterType::Control);
		ADD_CHILD_REG(FPSCR7, FPSCR7_RN0, 1, 2, platform::CPURegisterType::Control);
		ADD_CHILD_REG(FPSCR7, FPSCR7_RN1, 1, 3, platform::CPURegisterType::Control);

		ADD_REG( R0, 64, platform::CPURegisterType::Generic );
		ADD_REG( R1, 64, platform::CPURegisterType::Generic );
		ADD_REG( R2, 64, platform::CPURegisterType::Generic );
		ADD_REG( R3, 64, platform::CPURegisterType::Generic );
		ADD_REG( R4, 64, platform::CPURegisterType::Generic );
		ADD_REG( R5, 64, platform::CPURegisterType::Generic );
		ADD_REG( R6, 64, platform::CPURegisterType::Generic );
		ADD_REG( R7, 64, platform::CPURegisterType::Generic );
		ADD_REG( R8, 64, platform::CPURegisterType::Generic );
		ADD_REG( R9, 64, platform::CPURegisterType::Generic );
		ADD_REG( R10, 64, platform::CPURegisterType::Generic );
		ADD_REG( R11, 64, platform::CPURegisterType::Generic );
		ADD_REG( R12, 64, platform::CPURegisterType::Generic );
		ADD_REG( R13, 64, platform::CPURegisterType::Generic );
		ADD_REG( R14, 64, platform::CPURegisterType::Generic );
		ADD_REG( R15, 64, platform::CPURegisterType::Generic );
		ADD_REG( R16, 64, platform::CPURegisterType::Generic );
		ADD_REG( R17, 64, platform::CPURegisterType::Generic );
		ADD_REG( R18, 64, platform::CPURegisterType::Generic );
		ADD_REG( R19, 64, platform::CPURegisterType::Generic );
		ADD_REG( R20, 64, platform::CPURegisterType::Generic );
		ADD_REG( R21, 64, platform::CPURegisterType::Generic );
		ADD_REG( R22, 64, platform::CPURegisterType::Generic );
		ADD_REG( R23, 64, platform::CPURegisterType::Generic );
		ADD_REG( R24, 64, platform::CPURegisterType::Generic );
		ADD_REG( R25, 64, platform::CPURegisterType::Generic );
		ADD_REG( R26, 64, platform::CPURegisterType::Generic );
		ADD_REG( R27, 64, platform::CPURegisterType::Generic );
		ADD_REG( R28, 64, platform::CPURegisterType::Generic );
		ADD_REG( R29, 64, platform::CPURegisterType::Generic );
		ADD_REG( R30, 64, platform::CPURegisterType::Generic );
		ADD_REG( R31, 64, platform::CPURegisterType::Generic );

		ADD_REG( FR0, 64, platform::CPURegisterType::FloatingPoint );
		ADD_REG( FR1, 64, platform::CPURegisterType::FloatingPoint );
		ADD_REG( FR2, 64, platform::CPURegisterType::FloatingPoint );
		ADD_REG( FR3, 64, platform::CPURegisterType::FloatingPoint );
		ADD_REG( FR4, 64, platform::CPURegisterType::FloatingPoint );
		ADD_REG( FR5, 64, platform::CPURegisterType::FloatingPoint );
		ADD_REG( FR6, 64, platform::CPURegisterType::FloatingPoint );
		ADD_REG( FR7, 64, platform::CPURegisterType::FloatingPoint );
		ADD_REG( FR8, 64, platform::CPURegisterType::FloatingPoint );
		ADD_REG( FR9, 64, platform::CPURegisterType::FloatingPoint );
		ADD_REG( FR10, 64, platform::CPURegisterType::FloatingPoint );
		ADD_REG( FR11, 64, platform::CPURegisterType::FloatingPoint );
		ADD_REG( FR12, 64, platform::CPURegisterType::FloatingPoint );
		ADD_REG( FR13, 64, platform::CPURegisterType::FloatingPoint );
		ADD_REG( FR14, 64, platform::CPURegisterType::FloatingPoint );
		ADD_REG( FR15, 64, platform::CPURegisterType::FloatingPoint );
		ADD_REG( FR16, 64, platform::CPURegisterType::FloatingPoint );
		ADD_REG( FR17, 64, platform::CPURegisterType::FloatingPoint );
		ADD_REG( FR18, 64, platform::CPURegisterType::FloatingPoint );
		ADD_REG( FR19, 64, platform::CPURegisterType::FloatingPoint );
		ADD_REG( FR20, 64, platform::CPURegisterType::FloatingPoint );
		ADD_REG( FR21, 64, platform::CPURegisterType::FloatingPoint );
		ADD_REG( FR22, 64, platform::CPURegisterType::FloatingPoint );
		ADD_REG( FR23, 64, platform::CPURegisterType::FloatingPoint );
		ADD_REG( FR24, 64, platform::CPURegisterType::FloatingPoint );
		ADD_REG( FR25, 64, platform::CPURegisterType::FloatingPoint );
		ADD_REG( FR26, 64, platform::CPURegisterType::FloatingPoint );
		ADD_REG( FR27, 64, platform::CPURegisterType::FloatingPoint );
		ADD_REG( FR28, 64, platform::CPURegisterType::FloatingPoint );
		ADD_REG( FR29, 64, platform::CPURegisterType::FloatingPoint );
		ADD_REG( FR30, 64, platform::CPURegisterType::FloatingPoint );
		ADD_REG( FR31, 64, platform::CPURegisterType::FloatingPoint );

		ADD_REG( VR0, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR1, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR2, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR3, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR4, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR5, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR6, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR7, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR8, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR9, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR10, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR11, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR12, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR13, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR14, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR15, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR16, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR17, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR18, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR19, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR20, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR21, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR22, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR23, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR24, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR25, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR26, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR27, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR28, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR29, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR30, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR31, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR32, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR33, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR34, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR35, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR36, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR37, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR38, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR39, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR40, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR41, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR42, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR43, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR44, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR45, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR46, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR47, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR48, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR49, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR50, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR51, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR52, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR53, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR54, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR55, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR56, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR57, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR58, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR59, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR60, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR61, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR62, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR63, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR64, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR65, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR66, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR67, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR68, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR69, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR70, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR71, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR72, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR73, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR74, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR75, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR76, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR77, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR78, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR79, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR80, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR81, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR82, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR83, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR84, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR85, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR86, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR87, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR88, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR89, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR90, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR91, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR92, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR93, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR94, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR95, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR96, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR97, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR98, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR99, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR100, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR101, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR102, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR103, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR104, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR105, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR106, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR107, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR108, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR109, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR110, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR111, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR112, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR113, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR114, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR115, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR116, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR117, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR118, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR119, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR120, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR121, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR122, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR123, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR124, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR125, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR126, 128, platform::CPURegisterType::Wide );
		ADD_REG( VR127, 128, platform::CPURegisterType::Wide );

		// create special registers for seeing the content of the Wide registers
		{
			uint32_t regId = eRegister_MAX;
			for (uint32_t reg = (uint32_t)eRegister_VR0; reg <= (uint32_t)eRegister_VR127; ++reg)
			{
				const auto* vreg = m_regMap[reg];

				// quad parts
				for (uint32_t i = 0; i < 2; ++i, ++regId)
				{
					char name[16];
					sprintf_s(name, "%s_QWORD[%u]", vreg->GetName(), i);
					auto* childReg = AddChildRegister(vreg->GetName(), name, -1, 64, i * 64, platform::CPURegisterType::Wide);
				}

				// dword parts
				for (uint32_t i = 0; i < 4; ++i, ++regId)
				{
					char name[16];
					sprintf_s(name, "%s_DWORD[%u]", vreg->GetName(), i);
					auto* childReg = AddChildRegister(vreg->GetName(), name, -1, 32, i * 32, platform::CPURegisterType::Wide);
				}

				// uint16 parts
				for (uint32_t i = 0; i < 8; ++i, ++regId)
				{
					char name[16];
					sprintf_s(name, "%s_WORD[%u]", vreg->GetName(), i);
					auto* childReg = AddChildRegister(vreg->GetName(), name, -1, 16, (i^2) * 16, platform::CPURegisterType::Wide);
				}

				// uint18 parts
				for (uint32_t i = 0; i < 16; ++i, ++regId)
				{
					char name[16];
					sprintf_s(name, "%s_BYTE[%u]", vreg->GetName(), i);
					auto* childReg = AddChildRegister(vreg->GetName(), name, -1, 8, (i^3) * 8, platform::CPURegisterType::Wide);
				}
			}
		}

		#undef ADD_REG
		#define CR0		0x08
		#define CR1		0x10
	}

	// cache the LR & CTR registers
	RegLR = m_regMap[eRegister_LR];
	RegCTR = m_regMap[eRegister_CTR];
	RegCR0 = m_regMap[eRegister_CR0];
	RegCR1 = m_regMap[eRegister_CR1];
	RegCR6 = m_regMap[eRegister_CR6];
	RegXER_CA = m_regMap[eRegister_XER_CA];
	RegXER_OV = m_regMap[eRegister_XER_OV];
	RegXER_SO = m_regMap[eRegister_XER_SO];

	// create instruction decoders
	{
		#define MOUNT__( name, ... ) m_opMap[eInstruction_##name] = AddInstruction( #name, new __VA_ARGS__ );
		#define MOUNTRC( name, ... ) m_opMap[eInstruction_##name##RC] = AddInstruction( #name".", new __VA_ARGS__ );

		// nops
		MOUNT__( nop, CInstructiondDecoderXenon_NOP() );

		// register move & status words
		MOUNT__( mr, CInstructiondDecoderXenon_MR() );
		MOUNT__( mfspr, CInstructiondDecoderXenon_MR() );
		MOUNT__( mtspr, CInstructiondDecoderXenon_MR() );
		MOUNT__( mfcr, CInstructiondDecoderXenon_GENERIC<0,REG>() );
		MOUNT__( mtcrf, CInstructiondDecoderXenon_GENERIC<0,IMM,REG>() );
		MOUNT__( mfocrf, CInstructiondDecoderXenon_GENERIC<0,IMM,REG>() );
		MOUNT__( mtocrf, CInstructiondDecoderXenon_GENERIC<0,IMM,REG>() );
		MOUNT__( mfmsr, CInstructiondDecoderXenon_MR() );
		MOUNT__( mtmsrd, CInstructiondDecoderXenon_MR() );
		MOUNT__( mtmsree, CInstructiondDecoderXenon_GENERIC<0,REG,REG>() );
		MOUNT__( mftb, CInstructiondDecoderXenon_GENERIC<0,REG,IMM,IMM>() );

		// floating point control registers
		MOUNT__( mffs, CInstructiondDecoderXenon_GENERIC<0,REG>() );
		MOUNTRC( mffs, CInstructiondDecoderXenon_GENERIC<CR1,REG>() );
		MOUNT__( mcrfs, CInstructiondDecoderXenon_GENERIC<0,REG,REG>() ); // CREG, CFREG
		MOUNT__( mtfsfi, CInstructiondDecoderXenon_GENERIC<0,REG,IMM>() ); // CFREG, IMM
		MOUNTRC( mtfsfi, CInstructiondDecoderXenon_GENERIC<0,REG,IMM>() ); // CFREG, IMM
		MOUNT__( mtfsf, CInstructiondDecoderXenon_GENERIC<0,IMM,REG>() ); // CFREG, IMM
		MOUNTRC( mtfsf, CInstructiondDecoderXenon_GENERIC<0,IMM,REG>() ); // CFREG, IMM
		MOUNT__( mtfsb0, CInstructiondDecoderXenon_GENERIC<0,REG>() ); // CFREG, IMM
		MOUNTRC( mtfsb0, CInstructiondDecoderXenon_GENERIC<0,REG>() ); // CFREG, IMM
		MOUNT__( mtfsb1, CInstructiondDecoderXenon_GENERIC<0,REG>() ); // CFREG, IMM
		MOUNTRC( mtfsb1, CInstructiondDecoderXenon_GENERIC<0,REG>() ); // CFREG, IMM

		// load memory
		MOUNT__( lbz, CInstructiondDecoderXenon_MEM_LOAD<8,0,eMemoryType_Logical>() );
		MOUNT__( lhz, CInstructiondDecoderXenon_MEM_LOAD<16,0,eMemoryType_Logical>() );
		MOUNT__( lwz, CInstructiondDecoderXenon_MEM_LOAD<32,0,eMemoryType_Logical>() );
		MOUNT__( lbzu, CInstructiondDecoderXenon_MEM_LOAD<8,1,eMemoryType_Logical>() );
		MOUNT__( lhzu, CInstructiondDecoderXenon_MEM_LOAD<16,1,eMemoryType_Logical>() );
		MOUNT__( lwzu, CInstructiondDecoderXenon_MEM_LOAD<32,1,eMemoryType_Logical>() );
		MOUNT__( lba, CInstructiondDecoderXenon_MEM_LOAD<8,0,eMemoryType_Algebraic>() );
		MOUNT__( lha, CInstructiondDecoderXenon_MEM_LOAD<16,0,eMemoryType_Algebraic>() );
		MOUNT__( lwa, CInstructiondDecoderXenon_MEM_LOAD<32,0,eMemoryType_Algebraic>() );
		MOUNT__( lbau, CInstructiondDecoderXenon_MEM_LOAD<8,1,eMemoryType_Algebraic>() );
		MOUNT__( lhau, CInstructiondDecoderXenon_MEM_LOAD<16,1,eMemoryType_Algebraic>() );
		MOUNT__( lwau, CInstructiondDecoderXenon_MEM_LOAD<32,1,eMemoryType_Algebraic>() );
		MOUNT__( ld, CInstructiondDecoderXenon_MEM_LOAD<64,0,eMemoryType_Logical>() );
		MOUNT__( ldu, CInstructiondDecoderXenon_MEM_LOAD<64,1,eMemoryType_Logical>() );

		// load memory indexed
		MOUNT__( lbzx, CInstructiondDecoderXenon_MEM_LOAD<8,0,eMemoryType_Logical>() );
		MOUNT__( lhzx, CInstructiondDecoderXenon_MEM_LOAD<16,0,eMemoryType_Logical>() );
		MOUNT__( lwzx, CInstructiondDecoderXenon_MEM_LOAD<32,0,eMemoryType_Logical>() );
		MOUNT__( lbzux, CInstructiondDecoderXenon_MEM_LOAD<8,1,eMemoryType_Logical>() );
		MOUNT__( lhzux, CInstructiondDecoderXenon_MEM_LOAD<16,1,eMemoryType_Logical>() );
		MOUNT__( lwzux, CInstructiondDecoderXenon_MEM_LOAD<32,1,eMemoryType_Logical>() );
		MOUNT__( lbax, CInstructiondDecoderXenon_MEM_LOAD<8,0,eMemoryType_Algebraic>() );
		MOUNT__( lhax, CInstructiondDecoderXenon_MEM_LOAD<16,0,eMemoryType_Algebraic>() );
		MOUNT__( lwax, CInstructiondDecoderXenon_MEM_LOAD<32,0,eMemoryType_Algebraic>() );
		MOUNT__( lbaux, CInstructiondDecoderXenon_MEM_LOAD<8,1,eMemoryType_Algebraic>() );
		MOUNT__( lhaux, CInstructiondDecoderXenon_MEM_LOAD<16,1,eMemoryType_Algebraic>() );
		MOUNT__( lwaux, CInstructiondDecoderXenon_MEM_LOAD<32,1,eMemoryType_Algebraic>() );
		MOUNT__( ldx, CInstructiondDecoderXenon_MEM_LOAD<64,0,eMemoryType_Logical>() );
		MOUNT__( ldux, CInstructiondDecoderXenon_MEM_LOAD<64,0,eMemoryType_Logical>() );

		// store memory
		MOUNT__( stb, CInstructiondDecoderXenon_MEM_STORE<8,0,eMemoryType_Logical>() );
		MOUNT__( sth, CInstructiondDecoderXenon_MEM_STORE<16,0,eMemoryType_Logical>() );
		MOUNT__( stw, CInstructiondDecoderXenon_MEM_STORE<32,0,eMemoryType_Logical>() );
		MOUNT__( std, CInstructiondDecoderXenon_MEM_STORE<64,0,eMemoryType_Logical>() );
		MOUNT__( stbu, CInstructiondDecoderXenon_MEM_STORE<8,1,eMemoryType_Logical>() );
		MOUNT__( sthu, CInstructiondDecoderXenon_MEM_STORE<16,1,eMemoryType_Logical>() );
		MOUNT__( stwu, CInstructiondDecoderXenon_MEM_STORE<32,1,eMemoryType_Logical>() );
		MOUNT__( stdu, CInstructiondDecoderXenon_MEM_STORE<64,1,eMemoryType_Logical>() );

		// store memory indexed
		MOUNT__( stbx, CInstructiondDecoderXenon_MEM_STORE<8,0,eMemoryType_Logical>() );
		MOUNT__( sthx, CInstructiondDecoderXenon_MEM_STORE<16,0,eMemoryType_Logical>() );
		MOUNT__( stwx, CInstructiondDecoderXenon_MEM_STORE<32,0,eMemoryType_Logical>() );
		MOUNT__( stdx, CInstructiondDecoderXenon_MEM_STORE<64,0,eMemoryType_Logical>() );
		MOUNT__( stbux, CInstructiondDecoderXenon_MEM_STORE<8,1,eMemoryType_Logical>() );
		MOUNT__( sthux, CInstructiondDecoderXenon_MEM_STORE<16,1,eMemoryType_Logical>() );
		MOUNT__( stwux, CInstructiondDecoderXenon_MEM_STORE<32,1,eMemoryType_Logical>() );
		MOUNT__( stdux, CInstructiondDecoderXenon_MEM_STORE<64,1,eMemoryType_Logical>() );

		// branch instructions
		MOUNT__( b, CInstructiondDecoderXenon_B<0>() );
		MOUNT__( bl, CInstructiondDecoderXenon_BL<0>() );
		MOUNT__( ba, CInstructiondDecoderXenon_B<1>() );
		MOUNT__( bla, CInstructiondDecoderXenon_BL<1>() );
		MOUNT__( bc, CInstructiondDecoderXenon_BC<0,0>() );
		MOUNT__( bcl, CInstructiondDecoderXenon_BC<1,0>() );
		MOUNT__( bca, CInstructiondDecoderXenon_BC<0,1>() );
		MOUNT__( bcla, CInstructiondDecoderXenon_BC<1,1>() );
		MOUNT__( bclr, CInstructiondDecoderXenon_BCLR<0>() );
		MOUNT__( bclrl, CInstructiondDecoderXenon_BCLR<1>() );
		MOUNT__( bcctr, CInstructiondDecoderXenon_BCCTRR<0>() );
		MOUNT__( bcctrl, CInstructiondDecoderXenon_BCCTRR<1>() );
	
		// lwarx/stwcx
		MOUNT__( lwarx, CInstructiondDecoderXenon_MEM_LOAD<32,0,eMemoryType_Atomic>() );
		MOUNT__( ldarx, CInstructiondDecoderXenon_MEM_LOAD<64,0,eMemoryType_Atomic>() );
		MOUNTRC( stwcx, CInstructiondDecoderXenon_MEM_STORE<32,0,eMemoryType_Atomic>() );
		MOUNTRC( stdcx, CInstructiondDecoderXenon_MEM_STORE<64,0,eMemoryType_Atomic>() );

		// load/store byte reversed
		MOUNT__( lhbrx, CInstructiondDecoderXenon_MEM_LOAD<16,0,eMemoryType_Reversed>());
		MOUNT__( lwbrx, CInstructiondDecoderXenon_MEM_LOAD<32,0,eMemoryType_Reversed>());
		MOUNT__( sthbrx, CInstructiondDecoderXenon_MEM_STORE<16,0,eMemoryType_Reversed>());
		MOUNT__( stwbrx, CInstructiondDecoderXenon_MEM_STORE<32,0,eMemoryType_Reversed>());

		// load/store string
		MOUNT__( lswi, CInstructiondDecoderXenon_GENERIC_CIMM<0,REG,REG,IMM>() );
		MOUNT__( lswx, CInstructiondDecoderXenon_GENERIC<0,REG,REG,REG>() );
		MOUNT__( stswi, CInstructiondDecoderXenon_GENERIC_CIMM<0,REG,REG,IMM>() );
		MOUNT__( stswx, CInstructiondDecoderXenon_GENERIC<0,REG,REG,REG>() );

		// memory bariers
		MOUNT__( sync, CInstructiondDecoderXenon_GENERIC<0>() );
		MOUNT__( lwsync, CInstructiondDecoderXenon_GENERIC<0>() );
		MOUNT__( ptesync, CInstructiondDecoderXenon_GENERIC<0>() );
		MOUNT__( eieio, CInstructiondDecoderXenon_GENERIC<0>() );

		// custom
		MOUNT__( fcmpu, CInstructiondDecoderXenon_FCMP() );
		MOUNT__( fcmpo, CInstructiondDecoderXenon_FCMP() );
		MOUNT__( twi, CInstructiondDecoderXenon_TRAP<32>() );
		MOUNT__( tdi, CInstructiondDecoderXenon_TRAP<64>() );
		MOUNT__( tw, CInstructiondDecoderXenon_TRAP_REG<32>() );
		MOUNT__( td, CInstructiondDecoderXenon_TRAP_REG<64>() );

		// generic instructions
		MOUNT__( sc, CInstructiondDecoderXenon_GENERIC<0,IMM>() );

		// condition register operators
		MOUNT__( crand, CInstructiondDecoderXenon_GENERIC<0,REG,REG,REG>() );
		MOUNT__( cror, CInstructiondDecoderXenon_GENERIC<0,REG,REG,REG>() );
		MOUNT__( crxor, CInstructiondDecoderXenon_GENERIC<0,REG,REG,REG>() );
		MOUNT__( crnand, CInstructiondDecoderXenon_GENERIC<0,REG,REG,REG>() );
		MOUNT__( crnor, CInstructiondDecoderXenon_GENERIC<0,REG,REG,REG>() );
		MOUNT__( creqv, CInstructiondDecoderXenon_GENERIC<0,REG,REG,REG>() );
		MOUNT__( crandc, CInstructiondDecoderXenon_GENERIC<0,REG,REG,REG>() );
		MOUNT__( crorc, CInstructiondDecoderXenon_GENERIC<0,REG,REG,REG>() );
		MOUNT__( mcrf, CInstructiondDecoderXenon_GENERIC<0,REG,REG>() );

		// load immediate
		MOUNT__( li, CInstructiondDecoderXenon_GENERIC<0,REG,IMM>() );
		MOUNT__( lis, CInstructiondDecoderXenon_GENERIC<0,REG,IMM>() );
	
		// math with immediate value
		MOUNT__( addic, CInstructiondDecoderXenon_GENERIC<CA_OUT, REG,REG,IMM>() );
		MOUNTRC( addic, CInstructiondDecoderXenon_GENERIC<CA_OUT|CR0, REG,REG,IMM>() );
		MOUNT__( addi, CInstructiondDecoderXenon_GENERIC<0,REG,REG,IMM>() );
		MOUNT__( addis, CInstructiondDecoderXenon_GENERIC<0,REG,REG,IMM>() );
		MOUNT__( subfic, CInstructiondDecoderXenon_GENERIC<CA_OUT,REG,REG,IMM>() );
		MOUNT__( mulli, CInstructiondDecoderXenon_GENERIC<0,REG,REG,IMM>() );

		// add XO-form
		MOUNT__( add, CInstructiondDecoderXenon_GENERIC<0, REG,REG,REG>() );
		MOUNTRC( add, CInstructiondDecoderXenon_GENERIC<CR0, REG,REG,REG>() );
		MOUNT__( addo, CInstructiondDecoderXenon_GENERIC<OV, REG,REG,REG>() );
		MOUNTRC( addo, CInstructiondDecoderXenon_GENERIC<OV|CR0, REG,REG,REG>() );

		// subtract grom XO-form
		MOUNT__( subf, CInstructiondDecoderXenon_GENERIC<0, REG,REG,REG>() );
		MOUNTRC( subf, CInstructiondDecoderXenon_GENERIC<CR0, REG,REG,REG>() );
		MOUNT__( subfo, CInstructiondDecoderXenon_GENERIC<OV, REG,REG,REG>() );
		MOUNTRC( subfo, CInstructiondDecoderXenon_GENERIC<OV|CR0, REG,REG,REG>() );

		// add carrying XO-form
		MOUNT__( addc, CInstructiondDecoderXenon_GENERIC<CA_OUT, REG,REG,REG>() );
		MOUNTRC( addc, CInstructiondDecoderXenon_GENERIC<CA_OUT|CR0, REG,REG,REG>() );
		MOUNT__( addco, CInstructiondDecoderXenon_GENERIC<CA_OUT|OV, REG,REG,REG>() );
		MOUNTRC( addco, CInstructiondDecoderXenon_GENERIC<CA_OUT|OV|CR0, REG,REG,REG>() );

		// subtract From carrying XO-form
		MOUNT__( subfc, CInstructiondDecoderXenon_GENERIC<CA_OUT, REG,REG,REG>() );
		MOUNTRC( subfc, CInstructiondDecoderXenon_GENERIC<CA_OUT|CR0, REG,REG,REG>() );
		MOUNT__( subfco, CInstructiondDecoderXenon_GENERIC<CA_OUT|OV, REG,REG,REG>() );
		MOUNTRC( subfco, CInstructiondDecoderXenon_GENERIC<CA_OUT|OV|CR0, REG,REG,REG>() );

		// add extended XO-form
		MOUNT__( adde, CInstructiondDecoderXenon_GENERIC<CA_OUT|CA_IN, REG,REG,REG>() );
		MOUNTRC( adde, CInstructiondDecoderXenon_GENERIC<CA_OUT|CA_IN|CR0, REG,REG,REG>() );
		MOUNT__( addeo, CInstructiondDecoderXenon_GENERIC<CA_OUT|CA_IN|OV, REG,REG,REG>() );
		MOUNTRC( addeo, CInstructiondDecoderXenon_GENERIC<CA_OUT|CA_IN|OV|CR0, REG,REG,REG>() );

		// subtract from extended XO-form
		MOUNT__( subfe, CInstructiondDecoderXenon_GENERIC<CA_OUT|CA_IN, REG,REG,REG>() );
		MOUNTRC( subfe, CInstructiondDecoderXenon_GENERIC<CA_OUT|CA_IN|CR0, REG,REG,REG>() );
		MOUNT__( subfeo, CInstructiondDecoderXenon_GENERIC<CA_OUT|CA_IN|OV, REG,REG,REG>() );
		MOUNTRC( subfeo, CInstructiondDecoderXenon_GENERIC<CA_OUT|CA_IN|OV|CR0, REG,REG,REG>() );

		// add to minus one extended XO-form
		MOUNT__( addme, CInstructiondDecoderXenon_GENERIC<CA_OUT|CA_IN, REG,REG>() );
		MOUNTRC( addme, CInstructiondDecoderXenon_GENERIC<CA_OUT|CA_IN|CR0, REG,REG>() );
		MOUNT__( addmeo, CInstructiondDecoderXenon_GENERIC<CA_OUT|CA_IN|OV, REG,REG>() );
		MOUNTRC( addmeo, CInstructiondDecoderXenon_GENERIC<CA_OUT|CA_IN|OV|CR0, REG,REG>() );

		// subtract from minus one extended XO-form
		MOUNT__( subfme, CInstructiondDecoderXenon_GENERIC<CA_OUT|CA_IN, REG,REG>() );
		MOUNTRC( subfme, CInstructiondDecoderXenon_GENERIC<CA_OUT|CA_IN|CR0, REG,REG>() );
		MOUNT__( subfmeo, CInstructiondDecoderXenon_GENERIC<CA_OUT|CA_IN|OV, REG,REG>() );
		MOUNTRC( subfmeo, CInstructiondDecoderXenon_GENERIC<CA_OUT|CA_IN|OV|CR0, REG,REG>() );
	
		// add to zero extended XO-form
		MOUNT__( addze, CInstructiondDecoderXenon_GENERIC<CA_OUT|CA_IN, REG,REG>() );
		MOUNTRC( addze, CInstructiondDecoderXenon_GENERIC<CA_OUT|CA_IN|CR0, REG,REG>() );
		MOUNT__( addzeo, CInstructiondDecoderXenon_GENERIC<CA_OUT|CA_IN|OV, REG,REG>() );
		MOUNTRC( addzeo, CInstructiondDecoderXenon_GENERIC<CA_OUT|CA_IN|OV|CR0, REG,REG>() );

		// subtract from zero extended XO-form
		MOUNT__( subfze, CInstructiondDecoderXenon_GENERIC<CA_OUT|CA_IN, REG,REG>() );
		MOUNTRC( subfze, CInstructiondDecoderXenon_GENERIC<CA_OUT|CA_IN|CR0, REG,REG>() );
		MOUNT__( subfzeo, CInstructiondDecoderXenon_GENERIC<CA_OUT|CA_IN|OV, REG,REG>() );
		MOUNTRC( subfzeo, CInstructiondDecoderXenon_GENERIC<CA_OUT|CA_IN|OV|CR0, REG,REG>() );

		// negate
		MOUNT__( neg, CInstructiondDecoderXenon_GENERIC<0, REG,REG>() );
		MOUNTRC( neg, CInstructiondDecoderXenon_GENERIC<CR0, REG,REG>() );
		MOUNT__( nego, CInstructiondDecoderXenon_GENERIC<OV, REG,REG>() );
		MOUNTRC( nego, CInstructiondDecoderXenon_GENERIC<OV|CR0, REG,REG>() );

		// multiply low doubleword XO-form
		MOUNT__( mulld, CInstructiondDecoderXenon_GENERIC<0, REG,REG,REG>() );
		MOUNTRC( mulld, CInstructiondDecoderXenon_GENERIC<CR0, REG,REG,REG>() );
		MOUNT__( mulldo, CInstructiondDecoderXenon_GENERIC<OV, REG,REG,REG>() );
		MOUNTRC( mulldo, CInstructiondDecoderXenon_GENERIC<OV|CR0, REG,REG,REG>() );
	
		// multiply low word XO-form
		MOUNT__( mullw, CInstructiondDecoderXenon_GENERIC<0, REG,REG,REG>() );
		MOUNTRC( mullw, CInstructiondDecoderXenon_GENERIC<CR0, REG,REG,REG>() );
		MOUNT__( mullwo, CInstructiondDecoderXenon_GENERIC<OV, REG,REG,REG>() );
		MOUNTRC( mullwo, CInstructiondDecoderXenon_GENERIC<OV|CR0, REG,REG,REG>() );

		// multiply high doubleword XO-form
		MOUNT__( mullhd, CInstructiondDecoderXenon_GENERIC<0, REG,REG,REG>() );
		MOUNTRC( mullhd, CInstructiondDecoderXenon_GENERIC<CR0, REG,REG,REG>() );

		// multiply High Word XO-form
		MOUNT__( mullhw, CInstructiondDecoderXenon_GENERIC<0, REG,REG,REG>() );
		MOUNTRC( mullhw, CInstructiondDecoderXenon_GENERIC<CR0, REG,REG,REG>() );

		// multiply high doubleword unsigned XO-form
		MOUNT__( mulhdu, CInstructiondDecoderXenon_GENERIC<0, REG,REG,REG>() );
		MOUNTRC( mulhdu, CInstructiondDecoderXenon_GENERIC<CR0, REG,REG,REG>() );
	
		// multiply High Word Unsigned XO-form
		MOUNT__( mulhwu, CInstructiondDecoderXenon_GENERIC<0, REG,REG,REG>() );
		MOUNTRC( mulhwu, CInstructiondDecoderXenon_GENERIC<CR0, REG,REG,REG>() );

		// divide doubleword XO-form
		MOUNT__( divd, CInstructiondDecoderXenon_GENERIC<0, REG,REG,REG>() );
		MOUNTRC( divd, CInstructiondDecoderXenon_GENERIC<CR0, REG,REG,REG>() );
		MOUNT__( divdo, CInstructiondDecoderXenon_GENERIC<OV, REG,REG,REG>() );
		MOUNTRC( divdo, CInstructiondDecoderXenon_GENERIC<OV|CR0, REG,REG,REG>() );

		// divide word XO-form
		MOUNT__( divw, CInstructiondDecoderXenon_GENERIC<0, REG,REG,REG>() );
		MOUNTRC( divw, CInstructiondDecoderXenon_GENERIC<CR0, REG,REG,REG>() );
		MOUNT__( divwo, CInstructiondDecoderXenon_GENERIC<OV, REG,REG,REG>() );
		MOUNTRC( divwo, CInstructiondDecoderXenon_GENERIC<OV|CR0, REG,REG,REG>() );

		// divide doubleword unsigned XO-form
		MOUNT__( divdu, CInstructiondDecoderXenon_GENERIC<0, REG,REG,REG>() );
		MOUNTRC( divdu, CInstructiondDecoderXenon_GENERIC<CR0, REG,REG,REG>() );
		MOUNT__( divduo, CInstructiondDecoderXenon_GENERIC<OV, REG,REG,REG>() );
		MOUNTRC( divduo, CInstructiondDecoderXenon_GENERIC<OV|CR0, REG,REG,REG>() );

		// divide word unsigned XO-form
		MOUNT__( divwu, CInstructiondDecoderXenon_GENERIC<0, REG,REG,REG>() );
		MOUNTRC( divwu, CInstructiondDecoderXenon_GENERIC<CR0, REG,REG,REG>() );
		MOUNT__( divwuo, CInstructiondDecoderXenon_GENERIC<OV, REG,REG,REG>() );
		MOUNTRC( divwuo, CInstructiondDecoderXenon_GENERIC<OV|CR0, REG,REG,REG>() );
	
		// compare
		MOUNT__( cmpwi, CInstructiondDecoderXenon_CMP() );
		MOUNT__( cmpdi, CInstructiondDecoderXenon_CMP() );
		MOUNT__( cmplwi, CInstructiondDecoderXenon_CMP() );
		MOUNT__( cmpldi, CInstructiondDecoderXenon_CMP() );
		MOUNT__( cmpw, CInstructiondDecoderXenon_CMP() );
		MOUNT__( cmpd, CInstructiondDecoderXenon_CMP() );
		MOUNT__( cmplw, CInstructiondDecoderXenon_CMP() );
		MOUNT__( cmpld, CInstructiondDecoderXenon_CMP() );

		// logical instructions with immediate values
		MOUNTRC( andi, CInstructiondDecoderXenon_GENERIC<CR0, REG,REG,IMM>() );
		MOUNTRC( andis, CInstructiondDecoderXenon_GENERIC<CR0, REG,REG,IMM>() );
		MOUNT__( ori, CInstructiondDecoderXenon_GENERIC<0, REG,REG,IMM>() );
		MOUNT__( oris, CInstructiondDecoderXenon_GENERIC<0, REG,REG,IMM>() );
		MOUNT__( xori, CInstructiondDecoderXenon_GENERIC<0, REG,REG,IMM>() );
		MOUNT__( xoris, CInstructiondDecoderXenon_GENERIC<0, REG,REG,IMM>() );

		// logical instructions
		MOUNT__( and, CInstructiondDecoderXenon_GENERIC<0,REG,REG,REG>() );
		MOUNTRC( and, CInstructiondDecoderXenon_GENERIC<CR0,REG,REG,REG>() );
		MOUNT__( or, CInstructiondDecoderXenon_GENERIC<0, REG,REG,REG>() );
		MOUNTRC( or, CInstructiondDecoderXenon_GENERIC<CR0, REG,REG,REG>() );
		MOUNT__( xor, CInstructiondDecoderXenon_GENERIC<0, REG,REG,REG>() );
		MOUNTRC( xor, CInstructiondDecoderXenon_GENERIC<CR0, REG,REG,REG>() );
		MOUNT__( nand, CInstructiondDecoderXenon_GENERIC<0, REG,REG,REG>() );
		MOUNTRC( nand, CInstructiondDecoderXenon_GENERIC<CR0, REG,REG,REG>() );
		MOUNT__( nor, CInstructiondDecoderXenon_GENERIC<0, REG,REG,REG>() );
		MOUNTRC( nor, CInstructiondDecoderXenon_GENERIC<CR0, REG,REG,REG>() );
		MOUNT__( eqv, CInstructiondDecoderXenon_GENERIC<0, REG,REG,REG>() );
		MOUNTRC( eqv, CInstructiondDecoderXenon_GENERIC<CR0, REG,REG,REG>() );
		MOUNT__( andc, CInstructiondDecoderXenon_GENERIC<0, REG,REG,REG>() );
		MOUNTRC( andc, CInstructiondDecoderXenon_GENERIC<CR0, REG,REG,REG>() );
		MOUNT__( orc, CInstructiondDecoderXenon_GENERIC<0, REG,REG,REG>() );
		MOUNTRC( orc, CInstructiondDecoderXenon_GENERIC<CR0, REG,REG,REG>() );

		// byte extend
		MOUNT__( extsb, CInstructiondDecoderXenon_GENERIC<0, REG,REG>() );
		MOUNTRC( extsb, CInstructiondDecoderXenon_GENERIC<CR0, REG,REG>() );
		MOUNT__( extsh, CInstructiondDecoderXenon_GENERIC<0, REG,REG>() );
		MOUNTRC( extsh, CInstructiondDecoderXenon_GENERIC<CR0, REG,REG>() );
		MOUNT__( extsw, CInstructiondDecoderXenon_GENERIC<0, REG,REG>() );
		MOUNTRC( extsw, CInstructiondDecoderXenon_GENERIC<CR0, REG,REG>() );

		// population count
		MOUNT__( popcntb, CInstructiondDecoderXenon_GENERIC<0, REG,REG>() );
		MOUNT__( cntlzw, CInstructiondDecoderXenon_GENERIC<0, REG,REG>() );
		MOUNTRC( cntlzw, CInstructiondDecoderXenon_GENERIC<CR0, REG,REG>() );
		MOUNT__( cntlzd, CInstructiondDecoderXenon_GENERIC<0, REG,REG>() );
		MOUNTRC( cntlzd, CInstructiondDecoderXenon_GENERIC<CR0, REG,REG>() );

		// Shift instructions with immediate params -->
		// REG,REG,IMM,IMM
		MOUNT__( rldicl, CInstructiondDecoderXenon_RLI<64,0>() );
		MOUNTRC( rldicl, CInstructiondDecoderXenon_RLI<64,CR0>() );
		MOUNT__( rldicr, CInstructiondDecoderXenon_RLI<64,0>() );
		MOUNTRC( rldicr, CInstructiondDecoderXenon_RLI<64,CR0>() );
		MOUNT__( rldic, CInstructiondDecoderXenon_RLI<64,0>() );
		MOUNTRC( rldic, CInstructiondDecoderXenon_RLI<64,CR0>() );
		MOUNT__( rldimi, CInstructiondDecoderXenon_RLI<64,0>() );
		MOUNTRC( rldimi, CInstructiondDecoderXenon_RLI<64,CR0>() );

		// REG,REG,IMM,IMM,IMM
		MOUNT__( rlwinm, CInstructiondDecoderXenon_RLIM<32,0>() );
		MOUNTRC( rlwinm, CInstructiondDecoderXenon_RLIM<32,CR0>() );
		MOUNT__( rlwimi, CInstructiondDecoderXenon_RLIM<32,0>() );
		MOUNTRC( rlwimi, CInstructiondDecoderXenon_RLIM<32,CR0>() );

		// REG,REG,REG,IMM
		MOUNT__( rldcl, CInstructiondDecoderXenon_RL<64,0>() );
		MOUNTRC( rldcl, CInstructiondDecoderXenon_RL<64,CR0>() );
		MOUNT__( rldcr, CInstructiondDecoderXenon_RL<64,0>() );
		MOUNTRC( rldcr, CInstructiondDecoderXenon_RL<64,CR0>() );

		// REG,REG,REG,IMM,IMM
		MOUNT__( rlwnm, CInstructiondDecoderXenon_GENERIC_CIMM<0,REG,REG,REG,IMM,IMM>() );
		MOUNTRC( rlwnm, CInstructiondDecoderXenon_GENERIC_CIMM<CR0,REG,REG,REG,IMM,IMM>() );

		// Extended shifts
		MOUNT__( sld, CInstructiondDecoderXenon_GENERIC<0, REG,REG,REG>() );
		MOUNTRC( sld, CInstructiondDecoderXenon_GENERIC<CR0, REG,REG,REG>() );
		MOUNT__( slw, CInstructiondDecoderXenon_GENERIC<0,REG,REG,REG>() );
		MOUNTRC( slw, CInstructiondDecoderXenon_GENERIC<CR0, REG,REG,REG>() );
		MOUNT__( srd, CInstructiondDecoderXenon_GENERIC<0,REG,REG,REG>() );
		MOUNTRC( srd, CInstructiondDecoderXenon_GENERIC<CR0, REG,REG,REG>() );
		MOUNT__( srw, CInstructiondDecoderXenon_GENERIC<0,REG,REG,REG>() );
		MOUNTRC( srw, CInstructiondDecoderXenon_GENERIC<CR0, REG,REG,REG>() );
		MOUNT__( srad, CInstructiondDecoderXenon_GENERIC<0,REG,REG,REG>() );
		MOUNTRC( srad, CInstructiondDecoderXenon_GENERIC<CR0, REG,REG,REG>() );
		MOUNT__( sraw, CInstructiondDecoderXenon_GENERIC<0,REG,REG,REG>() );
		MOUNTRC( sraw, CInstructiondDecoderXenon_GENERIC<CR0, REG,REG,REG>() );

		// Extended shifts with immediate... geez..
		MOUNT__( srawi, CInstructiondDecoderXenon_SRAI<32,0>() );
		MOUNTRC( srawi, CInstructiondDecoderXenon_SRAI<32,CR0>() );
		MOUNT__( sradi, CInstructiondDecoderXenon_SRAI<64,0>() );
		MOUNTRC( sradi, CInstructiondDecoderXenon_SRAI<64,CR0>() );

		// Cache operation instructions
		MOUNT__( dcbf, CInstructiondDecoderXenon_GENERIC<0, REG,REG>() );
		MOUNT__( dcbst, CInstructiondDecoderXenon_GENERIC<0, REG,REG>() );
		MOUNT__( dcbt, CInstructiondDecoderXenon_GENERIC<0, REG,REG>() );
		MOUNT__( dcbtst, CInstructiondDecoderXenon_GENERIC<0, REG,REG>() );
		MOUNT__( dcbz, CInstructiondDecoderXenon_DCBZ() );

		//  Floating point load instructions
		MOUNT__( lfs, CInstructiondDecoderXenon_MEM_LOAD<32,0,eMemoryType_Float>() );
		MOUNT__( lfsu, CInstructiondDecoderXenon_MEM_LOAD<32,1,eMemoryType_Float>() );
		MOUNT__( lfsx, CInstructiondDecoderXenon_MEM_LOAD<32,0,eMemoryType_Float>() );
		MOUNT__( lfsux, CInstructiondDecoderXenon_MEM_LOAD<32,1,eMemoryType_Float>() );
		MOUNT__( lfd, CInstructiondDecoderXenon_MEM_LOAD<64,0,eMemoryType_Float>() );
		MOUNT__( lfdu, CInstructiondDecoderXenon_MEM_LOAD<64,1,eMemoryType_Float>() );
		MOUNT__( lfdx, CInstructiondDecoderXenon_MEM_LOAD<64,0,eMemoryType_Float>() );
		MOUNT__( lfdux, CInstructiondDecoderXenon_MEM_LOAD<64,1,eMemoryType_Float>() );

		//  Floating point store instructions
		MOUNT__( stfs, CInstructiondDecoderXenon_MEM_STORE<32,0,eMemoryType_Float>() );
		MOUNT__( stfsu, CInstructiondDecoderXenon_MEM_STORE<32,1,eMemoryType_Float>() );
		MOUNT__( stfsx, CInstructiondDecoderXenon_MEM_STORE<32,0,eMemoryType_Float>() );
		MOUNT__( stfsux, CInstructiondDecoderXenon_MEM_STORE<32,1,eMemoryType_Float>() );
		MOUNT__( stfd, CInstructiondDecoderXenon_MEM_STORE<64,0,eMemoryType_Float>() );
		MOUNT__( stfdu, CInstructiondDecoderXenon_MEM_STORE<64,1,eMemoryType_Float>() );
		MOUNT__( stfdx, CInstructiondDecoderXenon_MEM_STORE<64,0,eMemoryType_Float>() );
		MOUNT__( stfdux, CInstructiondDecoderXenon_MEM_STORE<64,1,eMemoryType_Float>() );

		// Store float point as integer word indexed
		MOUNT__( stfiwx, CInstructiondDecoderXenon_MEM_STORE<32,0,eMemoryType_Logical>() );

		// Floating point move and single op instructions
		MOUNT__( fmr, CInstructiondDecoderXenon_GENERIC<0, REG,REG>() );
		MOUNTRC( fmr, CInstructiondDecoderXenon_GENERIC<CR1, REG,REG>() );
		MOUNT__( fneg, CInstructiondDecoderXenon_GENERIC<0, REG,REG>() );
		MOUNTRC( fneg, CInstructiondDecoderXenon_GENERIC<CR1, REG,REG>() );
		MOUNT__( fabs, CInstructiondDecoderXenon_GENERIC<0, REG,REG>() );
		MOUNTRC( fabs, CInstructiondDecoderXenon_GENERIC<CR1, REG,REG>() );
		MOUNT__( fnabs, CInstructiondDecoderXenon_GENERIC<0, REG,REG>() );
		MOUNTRC( fnabs, CInstructiondDecoderXenon_GENERIC<CR1, REG,REG>() );

		// Floating point arithmetic instructions
		MOUNT__( fadd, CInstructiondDecoderXenon_GENERIC<0, REG,REG,REG>() );
		MOUNTRC( fadd, CInstructiondDecoderXenon_GENERIC<CR1, REG,REG,REG>() );
		MOUNT__( fadds, CInstructiondDecoderXenon_GENERIC<0, REG,REG,REG>() );
		MOUNTRC( fadds, CInstructiondDecoderXenon_GENERIC<CR1, REG,REG,REG>() );
		MOUNT__( fsub, CInstructiondDecoderXenon_GENERIC<0, REG,REG,REG>() );
		MOUNTRC( fsub, CInstructiondDecoderXenon_GENERIC<CR1, REG,REG,REG>() );
		MOUNT__( fsubs, CInstructiondDecoderXenon_GENERIC<0, REG,REG,REG>() );
		MOUNTRC( fsubs, CInstructiondDecoderXenon_GENERIC<CR1, REG,REG,REG>() );
		MOUNT__( fmul, CInstructiondDecoderXenon_GENERIC<0, REG,REG,REG>() );
		MOUNTRC( fmul, CInstructiondDecoderXenon_GENERIC<CR1, REG,REG,REG>() );
		MOUNT__( fmuls, CInstructiondDecoderXenon_GENERIC<0, REG,REG,REG>() );
		MOUNTRC( fmuls, CInstructiondDecoderXenon_GENERIC<CR1, REG,REG,REG>() );
		MOUNT__( fdiv, CInstructiondDecoderXenon_GENERIC<0, REG,REG,REG>() );
		MOUNTRC( fdiv, CInstructiondDecoderXenon_GENERIC<CR1, REG,REG,REG>() );
		MOUNT__( fdivs, CInstructiondDecoderXenon_GENERIC<0, REG,REG,REG>() );
		MOUNTRC( fdivs, CInstructiondDecoderXenon_GENERIC<CR1, REG,REG,REG>() );
		MOUNT__( fsqrt, CInstructiondDecoderXenon_GENERIC<0, REG,REG>() );
		MOUNTRC( fsqrt, CInstructiondDecoderXenon_GENERIC<CR1, REG,REG>() );
		MOUNT__( frsqrtx, CInstructiondDecoderXenon_GENERIC<0, REG, REG>());
		MOUNTRC( frsqrtx, CInstructiondDecoderXenon_GENERIC<CR1, REG, REG>());
		MOUNT__( fre, CInstructiondDecoderXenon_GENERIC<0, REG,REG>() );
		MOUNTRC( fre, CInstructiondDecoderXenon_GENERIC<CR1, REG,REG>() );

		// Floating point mad instructions
		MOUNT__( fmadd, CInstructiondDecoderXenon_GENERIC<0, REG,REG,REG,REG>() );
		MOUNTRC( fmadd, CInstructiondDecoderXenon_GENERIC<CR1, REG,REG,REG,REG>() );
		MOUNT__( fmadds, CInstructiondDecoderXenon_GENERIC<0, REG,REG,REG,REG>() );
		MOUNTRC( fmadds, CInstructiondDecoderXenon_GENERIC<CR1, REG,REG,REG,REG>() );
		MOUNT__( fmsub, CInstructiondDecoderXenon_GENERIC<0, REG,REG,REG,REG>() );
		MOUNTRC( fmsub, CInstructiondDecoderXenon_GENERIC<CR1, REG,REG,REG,REG>() );
		MOUNT__( fmsubs, CInstructiondDecoderXenon_GENERIC<0, REG,REG,REG,REG>() );
		MOUNTRC( fmsubs, CInstructiondDecoderXenon_GENERIC<CR1, REG,REG,REG,REG>() );
		MOUNT__( fnmadd, CInstructiondDecoderXenon_GENERIC<0, REG,REG,REG,REG>() );
		MOUNTRC( fnmadd, CInstructiondDecoderXenon_GENERIC<CR1, REG,REG,REG,REG>() );
		MOUNT__( fnmadds, CInstructiondDecoderXenon_GENERIC<0, REG,REG,REG,REG>() );
		MOUNTRC( fnmadds, CInstructiondDecoderXenon_GENERIC<CR1, REG,REG,REG,REG>() );
		MOUNT__( fnmsub, CInstructiondDecoderXenon_GENERIC<0, REG,REG,REG,REG>() );
		MOUNTRC( fnmsub, CInstructiondDecoderXenon_GENERIC<CR1, REG,REG,REG,REG>() );
		MOUNT__( fnmsubs, CInstructiondDecoderXenon_GENERIC<0, REG,REG,REG,REG>() );
		MOUNTRC( fnmsubs, CInstructiondDecoderXenon_GENERIC<CR1, REG,REG,REG,REG>() );

		// Floating point rounding and conversion
		MOUNT__( frsp, CInstructiondDecoderXenon_GENERIC<0, REG,REG>() );
		MOUNTRC( frsp, CInstructiondDecoderXenon_GENERIC<CR1, REG,REG>() );
		MOUNT__( fctid, CInstructiondDecoderXenon_GENERIC<0, REG,REG>() );
		MOUNTRC( fctid, CInstructiondDecoderXenon_GENERIC<CR1, REG,REG>() );
		MOUNT__( fctidz, CInstructiondDecoderXenon_GENERIC<0, REG,REG>() );
		MOUNTRC( fctidz, CInstructiondDecoderXenon_GENERIC<CR1, REG,REG>() );
		MOUNT__( fctiw, CInstructiondDecoderXenon_GENERIC<0, REG,REG>() );
		MOUNTRC( fctiw, CInstructiondDecoderXenon_GENERIC<CR1, REG,REG>() );
		MOUNT__( fctiwz, CInstructiondDecoderXenon_GENERIC<0, REG,REG>() );
		MOUNTRC( fctiwz, CInstructiondDecoderXenon_GENERIC<CR1, REG,REG>() );
		MOUNT__( fcfid, CInstructiondDecoderXenon_GENERIC<0, REG,REG>() );
		MOUNTRC( fcfid, CInstructiondDecoderXenon_GENERIC<CR1, REG,REG>() );

		// Floating point select
		MOUNT__( fsel, CInstructiondDecoderXenon_GENERIC<0, REG,REG,REG,REG>() );
		MOUNTRC( fsel, CInstructiondDecoderXenon_GENERIC<CR1, REG,REG,REG,REG>() );

		// VMX indexed load
		MOUNT__(lvebx, CInstructiondDecoderXenon_MEM_LOAD<8, 0, eMemoryType_VMXIndexed>());
		MOUNT__(lvehx, CInstructiondDecoderXenon_MEM_LOAD<16, 0, eMemoryType_VMXIndexed>());
		MOUNT__(lvewx, CInstructiondDecoderXenon_MEM_LOAD<32, 0, eMemoryType_VMXIndexed>());
		MOUNT__(stvebx, CInstructiondDecoderXenon_MEM_STORE<8, 0, eMemoryType_VMXIndexed>());
		MOUNT__(stvehx, CInstructiondDecoderXenon_MEM_STORE<16, 0, eMemoryType_VMXIndexed>());
		MOUNT__(stvewx, CInstructiondDecoderXenon_MEM_STORE<32, 0, eMemoryType_VMXIndexed>());

		// VMX load
		MOUNT__( lvlx, CInstructiondDecoderXenon_MEM_LOAD<128, 0, eMemoryType_VMLX>() );
		MOUNT__( lvlxl, CInstructiondDecoderXenon_MEM_LOAD<128, 0, eMemoryType_VMLX>() );
		MOUNT__( lvrx, CInstructiondDecoderXenon_MEM_LOAD<128, 0, eMemoryType_VMRX>() );
		MOUNT__( lvrxl, CInstructiondDecoderXenon_MEM_LOAD<128, 0, eMemoryType_VMRX>() );
		MOUNT__( lvx, CInstructiondDecoderXenon_MEM_LOAD<128, 0, eMemoryType_VMX>() );
		MOUNT__( lvxl, CInstructiondDecoderXenon_MEM_LOAD<128, 0, eMemoryType_VMX>() );

		// VMX store
		MOUNT__( stvlx, CInstructiondDecoderXenon_MEM_STORE<128, 0, eMemoryType_VMLX>() );
		MOUNT__( stvlxl, CInstructiondDecoderXenon_MEM_STORE<128, 0, eMemoryType_VMLX>() );
		MOUNT__( stvrx, CInstructiondDecoderXenon_MEM_STORE<128, 0, eMemoryType_VMRX>() );
		MOUNT__( stvrxl, CInstructiondDecoderXenon_MEM_STORE<128, 0, eMemoryType_VMRX>() );
		MOUNT__( stvx, CInstructiondDecoderXenon_MEM_STORE<128, 0, eMemoryType_VMX>() );
		MOUNT__( stvxl, CInstructiondDecoderXenon_MEM_STORE<128, 0, eMemoryType_VMX>() );

		// VMX mask gen
		MOUNT__( lvsl, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( lvsr, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );

		// VMX move to/from ctrl reg
		MOUNT__( mfvscr, CInstructiondDecoderXenon_GENERIC<0, REG>() );
		MOUNT__( mtvscr, CInstructiondDecoderXenon_GENERIC<0, REG>() );

		// VMX compare
		MOUNT__( vcmpbfp, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNTRC( vcmpbfp, CInstructiondDecoderXenon_GENERIC<CR6, REG, REG, REG>() );
		MOUNT__( vcmpeqfp, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNTRC( vcmpeqfp, CInstructiondDecoderXenon_GENERIC<CR6, REG, REG, REG>() );
		MOUNT__( vcmpequb, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNTRC( vcmpequb, CInstructiondDecoderXenon_GENERIC<CR6, REG, REG, REG>() );
		MOUNT__( vcmpequh, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNTRC( vcmpequh, CInstructiondDecoderXenon_GENERIC<CR6, REG, REG, REG>() );
		MOUNT__( vcmpequw, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNTRC( vcmpequw, CInstructiondDecoderXenon_GENERIC<CR6, REG, REG, REG>() );
		MOUNT__( vcmpgefp, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNTRC( vcmpgefp, CInstructiondDecoderXenon_GENERIC<CR6, REG, REG, REG>() );
		MOUNT__( vcmpgtfp, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNTRC( vcmpgtfp, CInstructiondDecoderXenon_GENERIC<CR6, REG, REG, REG>() );
		MOUNT__( vcmpgtsb, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNTRC( vcmpgtsb, CInstructiondDecoderXenon_GENERIC<CR6, REG, REG, REG>() );
		MOUNT__( vcmpgtsh, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNTRC( vcmpgtsh, CInstructiondDecoderXenon_GENERIC<CR6, REG, REG, REG>() );
		MOUNT__( vcmpgtsw, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNTRC( vcmpgtsw, CInstructiondDecoderXenon_GENERIC<CR6, REG, REG, REG>() );
		MOUNT__( vcmpgtub, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNTRC( vcmpgtub, CInstructiondDecoderXenon_GENERIC<CR6, REG, REG, REG>() );
		MOUNT__( vcmpgtuh, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNTRC( vcmpgtuh, CInstructiondDecoderXenon_GENERIC<CR6, REG, REG, REG>() );
		MOUNT__( vcmpgtuw, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNTRC( vcmpgtuw, CInstructiondDecoderXenon_GENERIC<CR6, REG, REG, REG>() );

		// VMX math
		MOUNT__( vaddcuw, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vaddfp, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vaddsbs, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vaddshs, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vaddsws, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vaddubm, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vaddubs, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vadduhm, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vadduhs, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vadduwm, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vadduws, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vand, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vandc, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vavgsb, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vavgsh, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vavgsw, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vavgub, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vavguh, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vavguw, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vmaxfp, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vminfp, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vmaxsh, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vminsh, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vmaxsw, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vminsw, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vmaxsb, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vminsb, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vmaxuh, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vminuh, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vmaxuw, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vminuw, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vmaxub, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vminub, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vmrghb, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vmrghh, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vmrghw, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vmrglb, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vmrglh, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vmrglw, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vsubcuw, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vsubfp, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vsubsbs, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vsubshs, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vsubsws, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vsububm, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vsububs, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vsubuhm, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vsubuhs, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vsubuwm, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vsubuws, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vnor, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vor, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vxor, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );

		// VMX 128 only
		MOUNT__( vmulfp128, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vupkd3d128, CInstructiondDecoderXenon_GENERIC_CIMM<0, REG, REG, IMM>() ); // data type
		MOUNT__( vpkd3d128, CInstructiondDecoderXenon_GENERIC_CIMM<0, REG, REG, IMM, IMM, IMM>() ); // data type, mask, shift

		// VMX pack
		MOUNT__( vpkpx, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vpkshss, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vpkshus, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vpkswss, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vpkswus, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vpkuhum, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vpkuhus, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vpkuwum, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vpkuwus, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );

		// VMX unpack
		MOUNT__( vupkhpx, CInstructiondDecoderXenon_GENERIC<0, REG, REG>() );
		MOUNT__( vupkhsb, CInstructiondDecoderXenon_GENERIC<0, REG, REG>() );
		MOUNT__( vupkhsh, CInstructiondDecoderXenon_GENERIC<0, REG, REG>() );
		MOUNT__( vupklpx, CInstructiondDecoderXenon_GENERIC<0, REG, REG>() );
		MOUNT__( vupklsb, CInstructiondDecoderXenon_GENERIC<0, REG, REG>() );
		MOUNT__( vupklsh, CInstructiondDecoderXenon_GENERIC<0, REG, REG>() );

		// VMX crap
		MOUNT__( vctsxs, CInstructiondDecoderXenon_GENERIC_CIMM<0, REG, REG, IMM>() );
		MOUNT__( vctuxs, CInstructiondDecoderXenon_GENERIC_CIMM<0, REG, REG, IMM>() );
		MOUNT__( vcfsx, CInstructiondDecoderXenon_GENERIC_CIMM<0, REG, REG, IMM>() );
		MOUNT__( vcfux, CInstructiondDecoderXenon_GENERIC_CIMM<0, REG, REG, IMM>() );
		MOUNT__( vcfpsxws, CInstructiondDecoderXenon_GENERIC_CIMM<0, REG, REG, IMM>() );
		MOUNT__( vcfpuxws, CInstructiondDecoderXenon_GENERIC_CIMM<0, REG, REG, IMM>() );
		MOUNT__( vcuxwfp, CInstructiondDecoderXenon_GENERIC_CIMM<0, REG, REG, IMM>() );
		MOUNT__( vcsxwfp, CInstructiondDecoderXenon_GENERIC_CIMM<0, REG, REG, IMM>() );

		// VMX scalar math
		MOUNT__( vexptefp, CInstructiondDecoderXenon_GENERIC<0, REG, REG>() );
		MOUNT__( vlogefp, CInstructiondDecoderXenon_GENERIC<0, REG, REG>() );

		// VMX splats
		MOUNT__( vspltb, CInstructiondDecoderXenon_GENERIC_CIMM<0, REG, REG, IMM>() );
		MOUNT__( vsplth, CInstructiondDecoderXenon_GENERIC_CIMM<0, REG, REG, IMM>() );
		MOUNT__( vspltw, CInstructiondDecoderXenon_GENERIC_CIMM<0, REG, REG, IMM>() );
		MOUNT__( vspltisb, CInstructiondDecoderXenon_GENERIC_CIMM<0, REG, IMM>() );
		MOUNT__( vspltish, CInstructiondDecoderXenon_GENERIC_CIMM<0, REG, IMM>() );
		MOUNT__( vspltisw, CInstructiondDecoderXenon_GENERIC_CIMM<0, REG, IMM>() );

		// VMX 4 arg
		MOUNT__( vsel, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG, REG>() );
		MOUNT__( vrlimi128, CInstructiondDecoderXenon_GENERIC_CIMM<0, REG, REG, IMM, IMM>() );
		MOUNT__( vperm, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG, REG>() );
		MOUNT__( vpermwi128, CInstructiondDecoderXenon_GENERIC_CIMM<0, REG, REG, IMM>() );
		MOUNT__( vmaddcfp128, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG, REG>() );
		MOUNT__( vnmsubfp, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG, REG>() );
		MOUNT__( vmaddfp, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG, REG>() );

		// VMX dots
		MOUNT__( vdot3fp, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vdot4fp, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );

		// VMX scalars
		MOUNT__( vrefp, CInstructiondDecoderXenon_GENERIC<0, REG, REG>() );
		MOUNT__( vrsqrtefp, CInstructiondDecoderXenon_GENERIC<0, REG, REG>() );
		MOUNT__( vrfim, CInstructiondDecoderXenon_GENERIC<0, REG, REG>() );
		MOUNT__( vrfin, CInstructiondDecoderXenon_GENERIC<0, REG, REG>() );
		MOUNT__( vrfip, CInstructiondDecoderXenon_GENERIC<0, REG, REG>() );
		MOUNT__( vrfiz, CInstructiondDecoderXenon_GENERIC<0, REG, REG>() );

		// VMX shifts
		MOUNT__( vsl, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vslb, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vslh, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vslo, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vslw, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vsldoi, CInstructiondDecoderXenon_GENERIC_CIMM<0, REG, REG, REG, IMM>() ); //shift
		MOUNT__( vsr, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vsrab, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vsrah, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vsraw, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vsrb, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vsrh, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vsro, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vsrw, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vrlb, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vrlh, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );
		MOUNT__( vrlw, CInstructiondDecoderXenon_GENERIC<0, REG, REG, REG>() );

		#undef MOUNT__
		#undef MOUNTRC
	}

	// check that all instructions are mounted
	/*for (uint32 i=0; i<eInstruction_MAX; ++i)
	{
		EInstruction op = (EInstruction)i;
		if (!m_opMap[op])
		{
			char s[64];
			sprintf_s(s, "Missing op at slot %d\n", op );
			OutputDebugStringA(s);
		}
	}*/
}

CPU_XenonPPC::~CPU_XenonPPC()
{}

CPU_XenonPPC& CPU_XenonPPC::GetInstance()
{
	static CPU_XenonPPC theInstance;
	return theInstance;
}

uint32 CPU_XenonPPC::ValidateInstruction(const uint8* inputStream) const
{
	decoding::Instruction temp;
	return DecodeInstruction(inputStream, temp);
}

// ----------------------------------------------------------------------------------------

