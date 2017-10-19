#include "xenonImageLoader.h"
#include "xenonPlatform.h"
#include "xenonCPU.h"

#include "../recompiler_core/decodingContext.h"
#include "../recompiler_core/image.h"
#include "../recompiler_core/decodingAddressMap.h"
#include "../recompiler_core/decodingMemoryMap.h"
#include "../recompiler_core/decodingNameMap.h"
#include "../recompiler_core/decodingCommentMap.h"

extern "C" __declspec(dllimport) void __stdcall OutputDebugStringA(const char* lpOutputString);

//---------------------------------------------------------------------------

////#pragma optimize("",off)

class CAddressCalculatorXenon
{
private:
	decoding::Context*					m_context;

	uint32								m_numRegs;
	uint32								m_numRegMask;
	uint64*								m_regValues;
	uint64*								m_regMask;

	const platform::CPUInstruction*		m_opLi;
	const platform::CPUInstruction*		m_opLis;
	const platform::CPUInstruction*		m_opAddi;
	const platform::CPUInstruction*		m_opAddis;
	const platform::CPUInstruction*		m_opMr;
	const platform::CPUInstruction*		m_opMtspr;
	const platform::CPUInstruction*		m_opMfspr;

	const static int REG_MASK = 63;
	const static int REG_SHIFT = 6;

public:
	CAddressCalculatorXenon(decoding::Context& context)
		: m_context(&context)
		, m_numRegs(CPU_XenonPPC::GetInstance().GetNumRegisters())
		, m_numRegMask((m_numRegs + 63) / 64)
	{
		// create register mapping
		m_regValues = new uint64[m_numRegs];
		memset(m_regValues, 0, sizeof(uint64) * m_numRegs);
		m_regMask = new uint64[m_numRegMask];
		memset(m_regMask, 0, sizeof(uint64) * m_numRegMask);

		// cache important instructions
		m_opLi = CPU_XenonPPC::GetInstance().FindInstruction("li");
		m_opLis = CPU_XenonPPC::GetInstance().FindInstruction("lis");
		m_opAddi = CPU_XenonPPC::GetInstance().FindInstruction("addi");
		m_opAddis = CPU_XenonPPC::GetInstance().FindInstruction("addis");
		m_opMr = CPU_XenonPPC::GetInstance().FindInstruction("mr");
		m_opMtspr = CPU_XenonPPC::GetInstance().FindInstruction("mtspr");
		m_opMfspr = CPU_XenonPPC::GetInstance().FindInstruction("mfspr");
	}

	~CAddressCalculatorXenon()
	{
		delete[] m_regValues;
		delete[] m_regMask;
	}

	// sign extend for 32 bits
	static inline uint64 EXTS(const uint32 s)
	{
		union
		{
			uint32 u;
			int32 i;
		} w;

		union
		{
			uint64 u;
			int64 i;
		} d;

		w.u = s;
		d.i = w.i;
		return d.u;
	}

	void Reset()
	{
		memset(m_regMask, 0, sizeof(uint64) * m_numRegMask);
	}

	void SetReg(const platform::CPURegister* reg, const uint64 value)
	{
		const int regIndex = reg->GetNativeIndex();
		if (regIndex >= 0 && regIndex < (int)m_numRegs)
		{
			const uint64 mask = (1ULL << (regIndex & REG_MASK));
			m_regMask[regIndex >> REG_SHIFT] |= mask;
			m_regValues[regIndex] = value;
		}
	}

	void ClearReg(const platform::CPURegister* reg)
	{
		const int regIndex = reg->GetNativeIndex();
		if (regIndex >= 0 && regIndex < (int)m_numRegs)
		{
			const uint64 mask = (1ULL << (regIndex & REG_MASK));
			m_regMask[regIndex >> REG_SHIFT] &= ~mask;
			m_regValues[regIndex] = 0;
		}
	}

	const bool IsValidReg(const platform::CPURegister* reg)
	{
		const int regIndex = reg->GetNativeIndex();
		if (regIndex >= 0 && regIndex < (int)m_numRegs)
		{
			const uint64 mask = (1ULL << (regIndex & REG_MASK));
			if (m_regMask[regIndex >> REG_SHIFT] & mask)
				return true;
		}

		return false;
	}

	const bool MoveValue(const platform::CPURegister* dest, const platform::CPURegister* src)
	{
		if (IsValidReg(src))
		{
			const uint64 val = m_regValues[src->GetNativeIndex()];
			SetReg(dest, val);
			return true;
		}
		else
		{
			ClearReg(dest);
			return false;
		}
	}

	const bool OpLi(const platform::CPURegister* dest, const uint32 imm)
	{
		const uint64 extsw = EXTS(imm);
		SetReg(dest, extsw);
		return true;
	}

	const bool OpLis(const platform::CPURegister* dest, const uint32 imm)
	{
		const uint64 extsw = EXTS(imm << 16);
		SetReg(dest, extsw);
		return true;
	}

	const bool OpAddi(const platform::CPURegister* dest, const platform::CPURegister* src, const uint32 imm)
	{
		if (!IsValidReg(src))
		{
			ClearReg(dest);
			return false;
		}

		const uint64 val = m_regValues[src->GetNativeIndex()];
		const uint64 extsw = EXTS(imm);
		SetReg(dest, extsw + val);
		return true;
	}

	const bool OpAddis(const platform::CPURegister* dest, const platform::CPURegister* src, const uint32 imm)
	{
		if (!IsValidReg(src))
		{
			ClearReg(dest);
			return false;
		}

		const uint64 val = m_regValues[src->GetNativeIndex()];
		const uint64 extsw = EXTS(imm << 16);
		SetReg(dest, extsw + val);
		return true;
	}

	const image::Section* GetSectionForAddress(const uint64 targetAddress) const
	{
		const auto baseAddress = m_context->GetImage()->GetBaseAddress();
		const auto numSections = m_context->GetImage()->GetNumSections();
		for (uint32 i = 0; i < numSections; ++i)
		{
			const image::Section* section = m_context->GetImage()->GetSection(i);
			const auto sectionStartAddress = section->GetVirtualOffset() + baseAddress;
			const auto sectionEndAddress = sectionStartAddress + section->GetVirtualSize();

			if (targetAddress >= sectionStartAddress && targetAddress < sectionEndAddress)
				return section;
		}

		return NULL;
	}

	const bool IsCodeAddrses(const uint32 targetAddress) const
	{
		const image::Section* section = GetSectionForAddress(targetAddress);
		return section && section->CanExecute();
	}

	const bool IsDataAddress(const uint32 targetAddress) const
	{
		const image::Section* section = GetSectionForAddress(targetAddress);
		return section && !section->CanExecute();
	}

	const bool HandleDataAddress(ILogOutput& log, const decoding::Instruction& instr, const uint64 codeAddress, const uint32 targetAddress, const decoding::InstructionExtendedInfo& info)
	{
		//log.Log("Decode: decoded runtime data address at %06Xh", codeAddress);
		const image::Section* section = GetSectionForAddress(targetAddress);
		if (!section)
		{
			// set the memory reference any way - always useful
			m_context->GetAddressMap().SetReferencedAddress(codeAddress, targetAddress);

			// gpu range ?
			if (targetAddress >= 0x7FC8000 && targetAddress <= 0x80000000)
			{
				m_context->GetMemoryMap().SetMemoryBlockSubType(log, codeAddress, (uint32)decoding::InstructionFlag::MappedMemory, 0);
				return true;
			}

			log.Warn("Decode: decoded address value is outside image at %06Xh", codeAddress);
			return false;
		}
		else if (section->CanExecute())
		{
			log.Warn("Decode: reading/writing from code section at %06Xh", codeAddress);
			return false;
		}

		m_context->GetMemoryMap().SetMemoryBlockLength(log, targetAddress, info.m_memorySize);
		m_context->GetMemoryMap().SetMemoryBlockType(log, targetAddress, (uint32)decoding::MemoryFlag::ReferencedData, 0);
		m_context->GetMemoryMap().SetMemoryBlockType(log, targetAddress, (uint32)decoding::MemoryFlag::GenericData, 0);

		m_context->GetMemoryMap().SetMemoryBlockSubType(log, targetAddress, (uint32)decoding::DataFlag::HasCodeRef, 0);
		if (info.m_memoryFlags & decoding::InstructionExtendedInfo::eMemoryFlags_Float)
			m_context->GetMemoryMap().SetMemoryBlockSubType(log, targetAddress, (uint32)decoding::DataFlag::Float, 0);

		m_context->GetAddressMap().SetReferencedAddress(codeAddress, targetAddress);
		return true;
	}

	const bool HandleCodeAddress(ILogOutput& log, const decoding::Instruction& instr, const uint64 codeAddress, const uint32 targetAddress, const decoding::InstructionExtendedInfo& info)
	{
		//log.Log("Decode: decoded runtime code address at %06Xh", codeAddress);

		const image::Section* section = GetSectionForAddress(targetAddress);
		if (!section)
		{
			log.Warn("Decode: decoded address value is outside image at %06Xh", codeAddress);
			return false;
		}
		else if (!section->CanExecute())
		{
			log.Warn("Decode: call/jump to a non-executable address at %06Xh", codeAddress);
			return false;
		}

		m_context->GetMemoryMap().SetMemoryBlockSubType(log, targetAddress, (uint32)decoding::InstructionFlag::BlockStart, 0);

		if (info.m_codeFlags & (decoding::InstructionExtendedInfo::eInstructionFlag_Call))
			m_context->GetMemoryMap().SetMemoryBlockSubType(log, targetAddress, (uint32)decoding::InstructionFlag::DynamicCallTarget, 0);
		if (info.m_codeFlags & (decoding::InstructionExtendedInfo::eInstructionFlag_Jump))
			m_context->GetMemoryMap().SetMemoryBlockSubType(log, targetAddress, (uint32)decoding::InstructionFlag::DynamicJumpTarget, 0);

		m_context->GetAddressMap().SetReferencedAddress(codeAddress, targetAddress);
		return true;
	}

	void Process(ILogOutput& log, const decoding::Instruction& instr, const uint64 codeAddress, const decoding::InstructionExtendedInfo& info)
	{
		// block start, reset
		const decoding::MemoryFlags flags = m_context->GetMemoryMap().GetMemoryInfo(codeAddress);
		if (flags.GetInstructionFlags().IsBlockStart())
			Reset();

		// function call
		if (flags.GetInstructionFlags().IsCall())
			Reset();

		// special instructions
		if (instr.GetOpcode() == m_opLi)
			OpLi(instr.GetArg0().m_reg, instr.GetArg1().m_imm);
		else if (instr.GetOpcode() == m_opLis)
			OpLis(instr.GetArg0().m_reg, instr.GetArg1().m_imm);
		else if (instr.GetOpcode() == m_opAddi)
			OpAddi(instr.GetArg0().m_reg, instr.GetArg1().m_reg, instr.GetArg2().m_imm);
		else if (instr.GetOpcode() == m_opAddis)
			OpAddis(instr.GetArg0().m_reg, instr.GetArg1().m_reg, instr.GetArg2().m_imm);
		else if (instr.GetOpcode() == m_opMr)
			MoveValue(instr.GetArg0().m_reg, instr.GetArg1().m_reg);
		else if (instr.GetOpcode() == m_opMtspr)
			MoveValue(instr.GetArg0().m_reg, instr.GetArg1().m_reg);
		else if (instr.GetOpcode() == m_opMfspr)
			MoveValue(instr.GetArg0().m_reg, instr.GetArg1().m_reg);

		// addressable
		else
		{
			// second op
			if (info.m_memoryFlags != 0 && info.m_memorySize)
			{
				if (instr.GetArg1().m_type == decoding::Instruction::eType_Mem)
				{
					bool isValid = true;
					if (instr.GetArg1().m_reg)
						isValid &= IsValidReg(instr.GetArg1().m_reg);
					if (instr.GetArg1().m_index)
						isValid &= IsValidReg(instr.GetArg1().m_index);


					if (isValid)
					{
						uint64 targetAddress = instr.GetArg1().m_imm;

						if (instr.GetArg1().m_reg)
						{
							const uint32 regIndex = instr.GetArg1().m_reg->GetNativeIndex();
							targetAddress += m_regValues[regIndex];
						}

						if (instr.GetArg1().m_index)
						{
							const uint32 regIndex = instr.GetArg1().m_index->GetNativeIndex();
							targetAddress += m_regValues[regIndex] * instr.GetArg1().m_scale;
						}

						HandleDataAddress(log, instr, codeAddress, (uint32)targetAddress, info);
					}

				}
			}
			else if (info.m_codeFlags & (decoding::InstructionExtendedInfo::eInstructionFlag_Jump | decoding::InstructionExtendedInfo::eInstructionFlag_Call))
			{
				// based on reg ?
				if (info.m_branchTargetReg)
				{
					if (IsValidReg(info.m_branchTargetReg))
					{
						const uint32 index = info.m_branchTargetReg->GetNativeIndex();
						const uint32 targetAddress = (const uint32)(m_regValues[index]);
						HandleCodeAddress(log, instr, codeAddress, targetAddress, info);
					}
				}
			}

			// destroy written regs
			for (uint32 i = 0; i < info.m_registersModifiedCount; ++i)
			{
				const platform::CPURegister* modReg = info.m_registersModified[i];
				ClearReg(modReg);
			}
		}
	}
};

//---------------------------------------------------------------------------

class CMemoryMappingDetector
{
private:
	decoding::Context*					m_context;

	const platform::CPUInstruction*		m_opEIEIO;
	const platform::CPUInstruction*		m_opSync;

public:
	CMemoryMappingDetector(decoding::Context& context)
		: m_context(&context)
	{
		// cache important instructions
		m_opEIEIO = CPU_XenonPPC::GetInstance().FindInstruction("eieio");
		m_opSync = CPU_XenonPPC::GetInstance().FindInstruction("sync");
	}

	void Process(ILogOutput& log, const decoding::Instruction& instr, const uint64 codeAddress, const decoding::InstructionExtendedInfo& info)
	{
		if (instr.GetOpcode() == m_opEIEIO)
		{
			// is the previous instruction a store ?
			decoding::Instruction prevInstr;
			const uint32 instructionSize = m_context->DecodeInstruction(log, codeAddress - 4, prevInstr, false);
			if (instructionSize)
			{
				// get extended instruction information (for branch target)
				decoding::InstructionExtendedInfo prevInfo;
				if (prevInstr.GetExtendedInfo(codeAddress - 4, *m_context, prevInfo))
				{
					// is this a memory write ?
					if (prevInfo.m_memoryFlags | decoding::InstructionExtendedInfo::eMemoryFlags_Write)
					{
						log.Log("Found memory mapped write at %08Xh", codeAddress - 4);
						m_context->GetMemoryMap().SetMemoryBlockSubType(log, codeAddress - 4, (uint32)decoding::InstructionFlag::MappedMemory, 0);
					}
				}
			}
		}
		else if (instr.GetOpcode() == m_opSync)
		{
			//log.Log( "Found sync at %08Xh", codeAddress );
		}
	}
};

//---------------------------------------------------------------------------

bool DecompilationXenon::DecodeImage(ILogOutput& log, class decoding::Context& context) const
{
	const auto image = context.GetImage();

	// apply known symbols
	{
		log.SetTaskName("Applying symbols...");
		uint32 numFunctionSymbols = 0;
		const uint32 numSymbols = image->GetNumSymbols();
		for (uint32 i = 0; i < numSymbols; ++i)
		{
			const image::Symbol* symbol = image->GetSymbol(i);
			const uint32 address = symbol->GetAddress();
			log.SetTaskProgress(i, numSymbols);

			// only functions
			if (symbol->GetType() == image::Symbol::eSymbolType_Function)
			{
				// mark as function call
				const uint32 flags = (uint32)decoding::InstructionFlag::StaticCallTarget | (uint32)decoding::InstructionFlag::BlockStart | (uint32)decoding::InstructionFlag::FunctionStart;
				context.GetMemoryMap().SetMemoryBlockSubType(log, address, flags, 0);

				// set name
				context.GetNameMap().SetName(address, symbol->GetName());

				// count
				numFunctionSymbols += 1;
			}
			else
			{
				// generic comment
				context.GetCommentMap().SetComment(address, symbol->GetName());
			}
		}

		log.Log("Decode: Applied %d known symbol informations", numFunctionSymbols);
	}

	// apply known symbols
	{
		log.SetTaskName("Creating imports...");
		const uint32 numImports = image->GetNumImports();
		for (uint32 i = 0; i < numImports; ++i)
		{
			const image::Import* import = image->GetImport(i);

			// function
			if (import->GetType() == image::Import::eImportType_Function)
			{
				const auto entryAddress = import->GetEntryAddress();

				// mark the header as function call
				const uint32 flags = (uint32)decoding::InstructionFlag::StaticCallTarget |
					(uint32)decoding::InstructionFlag::BlockStart |
					(uint32)decoding::InstructionFlag::FunctionStart |
					(uint32)decoding::InstructionFlag::ImportFunction;
				context.GetMemoryMap().SetMemoryBlockSubType(log, entryAddress, flags, 0);

				// map the first two words as import thunk
				context.GetMemoryMap().SetMemoryBlockSubType(log, entryAddress + 0, (uint32)decoding::InstructionFlag::Thunk, 0);
				context.GetMemoryMap().SetMemoryBlockSubType(log, entryAddress + 4, (uint32)decoding::InstructionFlag::Thunk, 0);
				//context.GetMemoryMap().SetMemoryBlockLength( log, address+0, 8 );								

				// set name if known
				if (import->GetExportName()[0])
				{
					context.GetNameMap().SetName(entryAddress, import->GetExportName());
				}
			}
			// data
			else if (import->GetType() == image::Import::eImportType_Data)
			{
				const auto tableAddress = import->GetTableAddress();

				context.GetMemoryMap().SetMemoryBlockLength(log, tableAddress + 0, 4);

				std::string comment = "Data import from '";
				comment += import->GetExportName();
				comment += "'";

				context.GetCommentMap().SetComment(tableAddress + 0, comment.c_str());

				// set name if known
				if (import->GetExportName()[0])
				{
					context.GetNameMap().SetName(tableAddress, import->GetExportName());
				}
			}
		}

		// stats
		log.Log("Decode: Applied %d image imports", numImports);
	}

	// main decompilation
	bool returnStatus = true;
	{
		// decode all executable sections
		const uint32 numSections = image->GetNumSections();
		for (uint32 i = 0; i < numSections; ++i)
		{
			const image::Section* section = image->GetSection(i);
			if (!section->CanExecute())
				continue;

			// section stats
			uint32 numDecodedInstructions = 0;
			uint32 numSkippedInstructions = 0;
			uint32 numFailedInstructions = 0;

			// start decoding phase
			log.SetTaskName("Decoding section '%hs'...", section->GetName().c_str());

			// code section ranges
			log.Log("Decode: Code section name: '%hs'", section->GetName().c_str());
			log.Log("Decode: Code section CPU: '%hs'", section->GetCPUName().c_str());
			log.Log("Decode: Code section start: %06Xh", section->GetVirtualOffset());
			log.Log("Decode: Code section size: %06Xh", section->GetVirtualSize());

			// compute code range
			const auto baseAddress = image->GetBaseAddress();
			const auto sectionBaseAddress = baseAddress + section->GetVirtualOffset();
			const auto endAddress = baseAddress + section->GetVirtualOffset() + section->GetVirtualSize();

			// process all instructions in the code range
			auto address = sectionBaseAddress;
			bool forceBlockStart = false;
			int lastCount = -1;
			while (address < endAddress)
			{
				// update progress
				log.SetTaskProgress(address - sectionBaseAddress, endAddress - sectionBaseAddress);

				// skip the "thunk" bytes
				decoding::MemoryFlags flags = context.GetMemoryMap().GetMemoryInfo(address);
				if (flags.GetInstructionFlags().IsThunk())
				{
					numSkippedInstructions += 1;
					address += (uint32)flags.GetSize();
					continue;
				}

				// not a first byte - fatal parsing error
				if (!flags.IsFirstByte())
				{
					returnStatus = false;
					log.Error("Decode: Code misalignment at address %06Xh", address);
					break;
				}

				// parse till the first invalid instruction is encountered
				decoding::Instruction instr;
				const auto instructionSize = context.DecodeInstruction(log, address, instr, false);
				if (!instructionSize)
				{
					log.Error("Decode: Instruction decoding failed at address %06Xh", address);
					////returnStatus = false;
					//numFailedInstructions += 1;
					//address += 4;
					returnStatus = false;
					//continue;
					break;
				}

				// get extended instruction information (for branch target)
				decoding::InstructionExtendedInfo info;
				if (!instr.GetExtendedInfo(address, context, info))
				{
					log.Error("Decode: Failed to get extended information for instruction at address %06Xh (%s)", address, instr.GetName());
					returnStatus = false;
					break;
					//address += 4;
					//continue;
					//break;
				}

				// force a start of block
				if (forceBlockStart)
				{
					context.GetMemoryMap().SetMemoryBlockSubType(log, address, (uint32)decoding::InstructionFlag::BlockStart, 0);
					forceBlockStart = false;
				}

				// mark absolute jumps/calls
				if (info.m_branchTargetAddress && info.m_codeFlags & decoding::InstructionExtendedInfo::eInstructionFlag_Jump)
				{
					if (0 != (info.m_branchTargetAddress & 3))
						log.Error("Decode: Misalligned jump address at %06Xh", address);

					// start block at target
					context.GetMemoryMap().SetMemoryBlockSubType(log, info.m_branchTargetAddress, (uint32)decoding::InstructionFlag::StaticJumpTarget, 0);
					context.GetMemoryMap().SetMemoryBlockSubType(log, info.m_branchTargetAddress, (uint32)decoding::InstructionFlag::BlockStart, 0);

					// conditional
					context.GetMemoryMap().SetMemoryBlockSubType(log, address, (uint32)decoding::InstructionFlag::Jump, 0);
					if (info.m_codeFlags & decoding::InstructionExtendedInfo::eInstructionFlag_Conditional)
					{
						context.GetMemoryMap().SetMemoryBlockSubType(log, address, (uint32)decoding::InstructionFlag::Conditional, 0);
					}

					// bind target link
					context.GetAddressMap().SetReferencedAddress(address, info.m_branchTargetAddress);
				}
				else if (info.m_branchTargetAddress && info.m_codeFlags & decoding::InstructionExtendedInfo::eInstructionFlag_Call)
				{
					if (0 != (info.m_branchTargetAddress & 3))
						log.Error("Decode: Misalligned call address at %06Xh", address);

					// start block at target
					context.GetMemoryMap().SetMemoryBlockSubType(log, info.m_branchTargetAddress, (uint32)decoding::InstructionFlag::StaticCallTarget, 0);
					context.GetMemoryMap().SetMemoryBlockSubType(log, info.m_branchTargetAddress, (uint32)decoding::InstructionFlag::BlockStart, 0);
					context.GetMemoryMap().SetMemoryBlockSubType(log, info.m_branchTargetAddress, (uint32)decoding::InstructionFlag::FunctionStart, 0); // call is always to a function

																																						// conditional
					context.GetMemoryMap().SetMemoryBlockSubType(log, address, (uint32)decoding::InstructionFlag::Call, 0);
					if (info.m_codeFlags & decoding::InstructionExtendedInfo::eInstructionFlag_Conditional)
					{
						context.GetMemoryMap().SetMemoryBlockSubType(log, address, (uint32)decoding::InstructionFlag::Conditional, 0);
					}

					// bind target link
					context.GetAddressMap().SetReferencedAddress(address, info.m_branchTargetAddress);
				}
				else if (!info.m_branchTargetAddress && info.m_branchTargetReg &&
					(info.m_codeFlags & decoding::InstructionExtendedInfo::eInstructionFlag_Return) && !(info.m_codeAddress & decoding::InstructionExtendedInfo::eInstructionFlag_Conditional))
				{
					// return from function call
					context.GetMemoryMap().SetMemoryBlockSubType(log, address, (uint32)decoding::InstructionFlag::Ret, 0);
					forceBlockStart = true;
				}

				// advance
				address += instructionSize;
				numDecodedInstructions += 1;
			}

			// stats
			if (numFailedInstructions)
			{
				log.Error("Decode: Decoded %d instructions (%d skipped, %d failed)", numDecodedInstructions, numSkippedInstructions, numFailedInstructions);
			}
			else
			{
				log.Log("Decode: Decoded %d instructions (%d skipped)", numDecodedInstructions, numSkippedInstructions, numFailedInstructions);
			}
		}
	}

	// do not do rest of the instuction decoding has failed
	if (returnStatus)
	{
		// decode indirect addresses
		{
			// start decoding phase
			log.SetTaskName("Decoding addresses...");

			// decode all executable sections
			const uint32 numSections = image->GetNumSections();
			for (uint32 i = 0; i < numSections; ++i)
			{
				const image::Section* section = image->GetSection(i);
				if (!section->CanExecute())
					continue;

				// start decoding phase
				log.SetTaskName("Resolving addresses in section '%s'...", section->GetName().c_str());

				// compute code range
				const uint32 baseAddress = image->GetBaseAddress();
				const uint32 sectionBaseAddress = baseAddress + section->GetVirtualOffset();
				const uint32 endAddress = baseAddress + section->GetVirtualOffset() + section->GetVirtualSize();

				// create address evaluator
				CAddressCalculatorXenon addressEvaluator(context);

				// process all instructions in the code range
				uint32 address = sectionBaseAddress;
				while (address < endAddress)
				{
					// update progress
					log.SetTaskProgress(address - sectionBaseAddress, endAddress - sectionBaseAddress);

					// parse till the first invalid instruction is encountered
					decoding::Instruction instr;
					const uint32 instructionSize = context.DecodeInstruction(log, address, instr, false);
					if (!instructionSize)
						break;

					// get extended instruction information (for branch target)
					decoding::InstructionExtendedInfo info;
					if (!instr.GetExtendedInfo(address, context, info))
						break;

					// update address evaluator
					addressEvaluator.Process(log, instr, address, info);

					// advance
					address += instructionSize;
				}
			}
		}

		// pointed code blocks
		{
			log.SetTaskName("Resolving code pointers...");
			uint32 numCodePointers = 0;
			uint32 numDataPointers = 0;
			const uint32 numSections = image->GetNumSections();
			for (uint32 i = 0; i < numSections; ++i)
			{
				const image::Section* section = image->GetSection(i);

				// start decoding phase
				log.SetTaskName("Resolving code pointers in section '%hs'...", section->GetName().c_str());

				// skip executable sections
				if (section->CanExecute())
					continue;

				// skip sections with no physical data
				if (!section->GetPhysicalSize())
					continue;

				// skip section that are not placed in virtual memory
				if (!section->GetVirtualSize())
					continue;

				// compute code range
				const uint32 baseAddress = image->GetBaseAddress();
				const uint32 sectionBaseAddress = baseAddress + section->GetVirtualOffset();
				const uint32 endAddress = baseAddress + section->GetVirtualOffset() + section->GetVirtualSize();

				// process all instructions in the code range
				uint32 address = sectionBaseAddress;
				bool returnStatus = true;
				bool forceBlockStart = false;
				while (address < endAddress)
				{
					// update progress
					log.SetTaskProgress(address - sectionBaseAddress, endAddress - sectionBaseAddress);

					// get the stored word
					const uint32 offset = address - baseAddress;
					const uint32 targetAddress = _byteswap_ulong(*(const uint32*)(image->GetMemory() + offset));

					// is this an address into the image data ?
					if (targetAddress >= image->GetBaseAddress() && targetAddress < (image->GetBaseAddress() + image->GetMemorySize()))
					{
						// is this an address from code section?					
						const image::Section* targetSection = NULL;
						for (uint32 j = 0; j < numSections; ++j)
						{
							const image::Section* testSection = image->GetSection(j);
							if (targetAddress >= (baseAddress + testSection->GetVirtualOffset()) &&
								targetAddress < (baseAddress + testSection->GetVirtualOffset() + testSection->GetVirtualSize()))
							{
								targetSection = testSection;
								break;
							}
						}

						// we require valid code section
						if (targetSection)
						{
							// executable section
							if (targetSection->CanExecute())
							{
								// is this 4 aligned address ?
								if (targetAddress & 3)
								{
									log.Log("Decode: Found non aligned reference to code at address %08Xh, section '%s'", address, section->GetName().c_str());
								}
								else
								{
									// well, mark as reference to code
									context.GetMemoryMap().SetMemoryBlockLength(log, address, 4); // address size
									context.GetMemoryMap().SetMemoryBlockType(log, address, (uint32)decoding::MemoryFlag::GenericData, 0);
									context.GetMemoryMap().SetMemoryBlockSubType(log, address, (uint32)decoding::DataFlag::CodePtr, 0);
									context.GetAddressMap().SetReferencedAddress(address, targetAddress);

									// ignore import thunks
									if (!context.GetMemoryMap().GetMemoryInfo(targetAddress).GetInstructionFlags().IsImportFunction())
									{
										// set block start at target address (does not need to be a function)
										context.GetMemoryMap().SetMemoryBlockSubType(log, targetAddress, (uint32)decoding::InstructionFlag::BlockStart, 0);
										context.GetMemoryMap().SetMemoryBlockSubType(log, targetAddress, (uint32)decoding::InstructionFlag::ReferencedInData, 0);

										//	log.Log( "Decode: Found reference to code at address %08Xh, section '%s'", address, section->GetName() );
										//log.Log( "Decode: Referenced address: %08Xh, section '%s'", targetAddress, targetCodeSection->GetName() );
										numCodePointers += 1;
									}
								}
							}
							else
							{
								// well, mark as reference to code
								context.GetMemoryMap().SetMemoryBlockLength(log, address, 4); // address size
								context.GetMemoryMap().SetMemoryBlockType(log, address, (uint32)decoding::MemoryFlag::GenericData, 0);
								context.GetMemoryMap().SetMemoryBlockSubType(log, address, (uint32)decoding::DataFlag::DataPtr, 0);
								context.GetAddressMap().SetReferencedAddress(address, targetAddress); // sets CodePtr style

																									  // set block start at target address (does not need to be a function)
								context.GetMemoryMap().SetMemoryBlockSubType(log, targetAddress, (uint32)decoding::DataFlag::HasDataRef, 0);

								//log.Log( "Decode: Found reference to data at address %08Xh, section '%s'", address, section->GetName().c_str() );
								numDataPointers += 1;
							}
						}
					}

					// advance
					address += 4;
				}
			}

			// stats
			log.Log("Decode: Found %d code pointers and %d data pointers", numCodePointers, numDataPointers);
		}
	}

	// scanning for mapped memory access
	if (returnStatus)
	{
		// start decoding phase
		log.SetTaskName("Looking for eieio...");

		// decode all executable sections
		const uint32 numSections = image->GetNumSections();
		for (uint32 i = 0; i < numSections; ++i)
		{
			const image::Section* section = image->GetSection(i);
			if (!section->CanExecute())
				continue;

			// start decoding phase
			log.SetTaskName("Resolving mapped memory in section '%s'...", section->GetName().c_str());

			// compute code range
			const uint32 baseAddress = image->GetBaseAddress();
			const uint32 sectionBaseAddress = baseAddress + section->GetVirtualOffset();
			const uint32 endAddress = baseAddress + section->GetVirtualOffset() + section->GetVirtualSize();

			// create address evaluator
			CMemoryMappingDetector detector(context);

			// process all instructions in the code range
			uint32 address = sectionBaseAddress;
			while (address < endAddress)
			{ 
				// update progress
				log.SetTaskProgress(address - sectionBaseAddress, endAddress - sectionBaseAddress);

				// parse till the first invalid instruction is encountered
				decoding::Instruction instr;
				const uint32 instructionSize = context.DecodeInstruction(log, address, instr, false);
				if (!instructionSize)
					break;

				// get extended instruction information (for branch target)
				decoding::InstructionExtendedInfo info;
				if (!instr.GetExtendedInfo(address, context, info))
					break;

				// update address evaluator
				detector.Process(log, instr, address, info);

				// advance
				address += instructionSize;
			}
		}
	}

	// done
	return true;
}

//---------------------------------------------------------------------------

