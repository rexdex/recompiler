#include "xenonCodeGeneration.h"
#include "xenonPlatform.h"
#include "xenonCPU.h"

#include "../recompiler_core/decodingContext.h"
#include "../recompiler_core/image.h"
#include "../recompiler_core/decodingMemoryMap.h"
#include "../recompiler_core/decodingNameMap.h"
#include "../recompiler_core/codeGenerator.h"
#include "../recompiler_core/internalUtils.h"

//---------------------------------------------------------------------------

CodeGeneratorOptionsXenon::CodeGeneratorOptionsXenon(const bool allowDebugging)
	: m_allowBlockMerging(!allowDebugging)
	, m_allowLocalLabels(!allowDebugging)
	, m_allowCallInlinling(!allowDebugging)
{
	m_forceMultiAddressBlocks = allowDebugging;
	m_debugTrace = allowDebugging;
}

CodeGeneratorOptionsXenon::~CodeGeneratorOptionsXenon()
{
}

//---------------------------------------------------------------------------

CCodeSegmentsXenon::CCodeSegmentsXenon()
{}

CCodeSegmentsXenon::~CCodeSegmentsXenon()
{}

const bool CCodeSegmentsXenon::CreateSegments(ILogOutput& log, const decoding::Context& decodingContext, const CodeGeneratorOptionsXenon& options)
{
	// scan the code sections
	const auto image = decodingContext.GetImage();
	const uint32 numSections = image->GetNumSections();
	for (uint32 i = 0; i<numSections; ++i)
	{
		const image::Section* section = image->GetSection(i);
		if (!section->CanExecute())
			continue;

		// get section range
		const auto start = section->GetVirtualOffset() + image->GetBaseAddress();
		const auto end = start + section->GetVirtualSize();
		log.SetTaskName("Creating code segments in '%s'...", section->GetName().c_str());

		// current block
		BlockInfo* currentBlock = NULL;

		// create the functions and blocks
		auto cur = start;
		while (cur < end)
		{
			// update task progress
			log.SetTaskProgress((int)(cur - start), (int)(end - start));

			// not executable data
			const decoding::MemoryFlags flags = decodingContext.GetMemoryMap().GetMemoryInfo(cur);
			if (!flags.IsExecutable())
			{
				if (currentBlock)
				{
					currentBlock->m_endAddress = cur;
					currentBlock = NULL;
				}

				cur += 4;
				continue;
			}

			// import crap
			if (flags.GetInstructionFlags().IsImportFunction())
			{
				if (currentBlock)
				{
					currentBlock->m_endAddress = cur;
					currentBlock = NULL;
				}

				cur += 16;
				continue;
			}

			// start of block
			if (flags.GetInstructionFlags().IsBlockStart() || flags.GetInstructionFlags().IsStaticCallTarget() ||
				flags.GetInstructionFlags().IsDynamicCallTarget())
			{
				if (currentBlock)
				{
					currentBlock->m_endAddress = cur;
					currentBlock = NULL;
				}

				// add new block
				BlockInfo newBlock;
				newBlock.m_startAddrses = cur;
				newBlock.m_endAddress = 0;
				m_blocks.push_back(newBlock);
				currentBlock = &m_blocks.back();
			}

			// advance
			cur += 4;
		}

		// end block before current section ends
		if (currentBlock)
		{
			currentBlock->m_endAddress = cur;
			currentBlock = NULL;
		}
	}

	// stats
	log.Log("Decompile: Found %d code blocks", m_blocks.size());
	return true;
}

//---------------------------------------------------------------------------

CodeGeneratorXenon::Instruction::Instruction(const uint32 address, const decoding::Instruction& op)
	: m_address(address)
	, m_targetAddress(0)
	, m_op(op)
	, m_flagLocalLabel(0)
	, m_flagMerged(0)
	, m_target(NULL)
{}

CodeGeneratorXenon::Instruction::~Instruction()
{
	Unlink();

	for (uint32 i = 0; i<m_links.size(); ++i)
	{
		m_links[i]->m_target = NULL;
	}
}

void CodeGeneratorXenon::Instruction::Unlink()
{
	if (m_target)
	{
		m_target->m_links.erase(
			std::find(m_target->m_links.begin(), m_target->m_links.end(), this));

		m_target = NULL;
	}
}

void CodeGeneratorXenon::Instruction::LinkTo(Instruction* target)
{
	if (!m_target && target)
	{
		m_target = target;
		m_target->m_links.push_back(this);
	}
}

CodeGeneratorXenon::Block::Block()
	: m_multiAddress(0)
	, m_functionStart(0)
{
}

CodeGeneratorXenon::Block::~Block()
{
	for (uint32 i = 0; i<m_instructions.size(); ++i)
	{
		delete m_instructions[i];
	}
}

CodeGeneratorXenon::CodeGeneratorXenon(const decoding::Context& context, const CodeGeneratorOptionsXenon& options)
	: m_options(&options)
	, m_image(context.GetImage().get())
	, m_context(&context)
{
}

CodeGeneratorXenon::~CodeGeneratorXenon()
{
	for (uint32 i = 0; i<m_blocks.size(); ++i)
	{
		delete m_blocks[i];
	}
}

////#pragma optimize("",off)

const bool CodeGeneratorXenon::AddBlock(class ILogOutput& log, const uint32 start, const uint32 end, class code::IGenerator& codeGen)
{
	// start block
	m_blocks.push_back(new Block());
	Block* block = m_blocks.back();

	// get memory info at block start
	const decoding::MemoryFlags flags = m_context->GetMemoryMap().GetMemoryInfo(start);
	if (flags.IsExecutable() && flags.GetInstructionFlags().IsFunctionStart())
	{
		block->m_functionStart = 1;
		m_isInSwitch = false;
	}

	// force the multiaddress block if needed
	if (m_options->m_forceMultiAddressBlocks || m_options->m_debugTrace)
		block->m_multiAddress = true;

	// we are inside the switch :(
	if (m_isInSwitch)
		block->m_multiAddress = true;

	// add instructions
	uint32 codeAddress = start;
	while (codeAddress < end)
	{
		// decode instruction at address
		decoding::Instruction op;
		const uint32 size = const_cast<decoding::Context*>(m_context)->DecodeInstruction(log, codeAddress, op, false);
		if (!size)
		{
			codeAddress += 4;
			continue;
			//return false;
		}

		// extract additional info
		decoding::InstructionExtendedInfo info;
		if (!op.GetExtendedInfo(codeAddress, *const_cast<decoding::Context*>(m_context), info))
			return false;

		// if we have a call instruction that stores stores the next instruction address in the LR reigister we need to use multiblock stuff (unless the call is the last instruction)
		if (info.m_codeFlags & (decoding::InstructionExtendedInfo::eInstructionFlag_Call))
		{
			if (codeAddress + 4 < end)
			{
				block->m_multiAddress = true;
			}
		}

		// switch (bctr)
		if (info.m_codeFlags & (decoding::InstructionExtendedInfo::eInstructionFlag_Jump))
		{
			if (info.m_branchTargetReg)
			{
				block->m_multiAddress = true; // reeanble
				m_isInSwitch = true;
			}
		}

		// add instruction to list
		Instruction* instr = new Instruction(codeAddress, op);
		instr->m_info = info;

		// generate default output code
		const platform::CPUInstruction* cpuInstr = op.GetOpcode();
		const IInstructiondDecompilerXenon* decoder = static_cast< const IInstructiondDecompilerXenon* >(cpuInstr->GetDecompiler());
		if (!decoder->Decompile(log, codeAddress, op, info, codeGen, instr->m_rawCode))
		{
			log.Error("Decompile: Decompilation for instruction '%s' at address %06Xh failed", op.GetName(), codeAddress);
			if (!decoder->Decompile(log, codeAddress, op, info, codeGen, instr->m_rawCode))
			{
				return false;
			}
		}

		// advance
		block->m_instructions.push_back(instr);
		codeAddress += size;
	}

	// block decoded
	return true;
}

////#pragma optimize("",on)

const bool CodeGeneratorXenon::CanGlue() const
{
	// code gluing not allowed in options
	if (!m_options->m_allowBlockMerging)
		return false;

	// not allowed yet
	return false;
}

const bool CodeGeneratorXenon::Optimize(class ILogOutput& log)
{
	return true;
}

const bool CodeGeneratorXenon::Emit(class ILogOutput& log, class code::IGenerator& codeGen) const
{
	// empty block
	if (m_blocks.empty() || m_blocks[0]->m_instructions.empty())
	{
		//log.Error( "CodeGen: Trying to emit code from empty blocks" );
		//return false;
		return true;
	}

	// get flags of the block root
	const uint32 startAddress = m_blocks[0]->m_instructions[0]->m_address;
	const decoding::MemoryFlags startFlags = m_context->GetMemoryMap().GetMemoryInfo(startAddress);

	// function?
	const bool isFunction = startFlags.IsExecutable() && startFlags.GetInstructionFlags().IsFunctionStart();
	const char* functionName = NULL;
	if (isFunction)
	{
		functionName = m_context->GetNameMap().GetName(startAddress);
	}

	// determine if we need the multiaddress stuff
	bool multiAddress = false;
	for (uint32 i = 0; i<m_blocks.size(); ++i)
	{
		const Block* block = m_blocks[i];
		if (block->m_multiAddress)
		{
			multiAddress = true;
			break;
		}
	}

	// emit the final code
	codeGen.StartBlock(startAddress, true/*multiAddress*/, functionName);
	for (uint32 i = 0; i<m_blocks.size(); ++i)
	{
		const Block* block = m_blocks[i];
		for (uint32 j = 0; j<block->m_instructions.size(); ++j)
		{
			const Instruction* instr = block->m_instructions[j];
			const uint32 codeAddress = instr->m_address;

			// emit original code
			{
				char originalCode[256];
				char* stream = originalCode;
				instr->m_op.GenerateSimpleText(codeAddress, stream, originalCode + sizeof(originalCode));
				codeGen.AddCodef(codeAddress, "/* %s */\n", originalCode);
			}

			// optimized away
			if (instr->m_flagMerged)
			{
				codeGen.AddCodef(codeAddress, "/* OPTIMIZED AWAY */\n");
				continue;
			}

			// emit code (prefer final code)
			if (!instr->m_finalCode.empty())
			{
				codeGen.AddCodef(codeAddress, "%s\n", instr->m_finalCode.c_str());
			}
			else if (!instr->m_rawCode.empty())
			{
				codeGen.AddCodef(codeAddress, "%s\n", instr->m_rawCode.c_str());
			}
			else if (!instr->m_rawCode.empty())
			{
				codeGen.AddCodef(codeAddress, "/* NO CODE */\n");
			}

			// trace support
			if (m_options->m_debugTrace)
			{
				codeGen.AddCodef(codeAddress, "return 0x%08X;\n", codeAddress + 4);
			}
		}
	}

	// end block
	codeGen.CloseBlock();

	// emitted
	return true;
}

//---------------------------------------------------------------------------

bool DecompilationXenon::ExportCode(ILogOutput& log, decoding::Context& decodingContext, const Commandline& settings, class code::IGenerator& codeGen) const
{
	// parse options
	const bool withDebugging = settings.HasOption("O0") || settings.HasOption("debug");
	CodeGeneratorOptionsXenon options(withDebugging);

	// emit the image
	codeGen.AddImageData(log, decodingContext.GetImage()->GetMemory(), decodingContext.GetImage()->GetMemorySize());

	// set image stuff
	codeGen.SetEntryAddress(decodingContext.GetEntryPointRVA());
	codeGen.SetLoadAddress(decodingContext.GetBaseOffsetRVA());

	// find functions and blocks
	CCodeSegmentsXenon blocks;
	if (!blocks.CreateSegments(log, decodingContext, options))
		return false;

	// build full path to the xenon CPU definition file
	const auto cpuFilePath = GetAppDirectoryPath() + L"../../dev/src/xenon_launcher/xenonCPU.h";
	codeGen.AddPlatformInclude(cpuFilePath);

	// export symbols
	{
		log.SetTaskName("Exporting symbols... ");

		const auto image = decodingContext.GetImage();
		const uint32 numImports = image->GetNumImports();
		for (uint32 i = 0; i<numImports; ++i)
		{
			const image::Import* import = image->GetImport(i);
			log.SetTaskProgress(i, numImports);

			if (import->GetType() == image::Import::eImportType_Function && import->GetEntryAddress() != 0)
			{
				codeGen.AddImportSymbol(import->GetExportName(), import->GetTableAddress(), import->GetEntryAddress());
			}
			else if (import->GetType() == image::Import::eImportType_Data)
			{
				codeGen.AddImportSymbol(import->GetExportName(), import->GetTableAddress(), 0);
			}
		}
	}

	// stats
	log.SetTaskName("Generating code... ");

	// process each block
	uint32 currentBlockIndex = 0;
	uint32 startBlockIndex = 0;
	uint32 numBlockBlobs = 0;
	uint32 numBlockInstructions = 0;
	while (currentBlockIndex < blocks.m_blocks.size())
	{
		CodeGeneratorXenon blob(decodingContext, options);

		// stats
		log.SetTaskProgress(currentBlockIndex, (int)blocks.m_blocks.size());

		// update stats
		numBlockBlobs += 1;
		startBlockIndex = currentBlockIndex;

		// add current block
		const CCodeSegmentsXenon::BlockInfo& block = blocks.m_blocks[currentBlockIndex];
		currentBlockIndex += 1;
		if (!blob.AddBlock(log, block.m_startAddrses, block.m_endAddress, codeGen))
		{
			log.Error("Decompile: Failed to decompile block starting at %06Xh", block.m_startAddrses);
			return false;
		}

		// try glue following blocks
		while (blob.CanGlue() && currentBlockIndex < blocks.m_blocks.size())
		{
			const CCodeSegmentsXenon::BlockInfo& block = blocks.m_blocks[currentBlockIndex];
			currentBlockIndex += 1;
			if (!blob.AddBlock(log, block.m_startAddrses, block.m_endAddress, codeGen))
			{
				log.Error("Decompile: Failed to decompile block starting at %06Xh", block.m_startAddrses);
				return false;
			}
		}

		// optimize blob code
		if (!blob.Optimize(log))
		{
			log.Error("Decompile: Failed to optimize code blob");
			return false;
		}

		// output the blob code to code output
		if (!blob.Emit(log, codeGen))
		{
			log.Error("Decompile: Failed to emit code blob");
			return false;
		}
	}

	// done
	log.Log("Compile: %d blocks processed, %d block groups, %d instructions",
		blocks.m_blocks.size(), numBlockBlobs, numBlockInstructions);

	// done
	return true;
}

//---------------------------------------------------------------------------

#if 0

bool decoding::InterfaceXenon::GenerateBranchCode(ILogOutput& log, const decoding::CodeExportOptions& options, class code::IGenerator& codeGen, const uint32 address, const uint32 branchType, const char* ctrRegName, const char* branchCode, const bool simple)
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
		codeGen.AddCode(address, "regs.CTR -= 1;\n");

	// branch code
	if (ctrTest && flagTest)
		codeGen.AddCodef(address, "if ( regs.CTR %s 0 && %sregs.%s ) { %s }\n", ctrCmp, flagCmp, ctrRegName, branchCode);
	else if (ctrTest && !flagTest)
		codeGen.AddCodef(address, "if ( regs.CTR %s 0 ) { %s }\n", ctrCmp, branchCode);
	else if (!ctrTest && flagTest)
		codeGen.AddCodef(address, "if ( %sregs.%s ) { %s }\n", flagCmp, ctrRegName, branchCode);
	else if (!ctrTest && !flagTest)
		codeGen.AddCodef(address, "if ( 1 ) { %s }\n", branchCode);

	return true;
}

bool decoding::InterfaceXenon::ExportCodeInstructions(ILogOutput& log, const decoding::CodeExportOptions& options, class code::IGenerator& codeGen)
{
	log.SetTaskName("Generating code...");

	// setup generic includes
	codeGen.ResetIncludes();

	// build full path to the xenon base definition file
	{
		std::wstring baseIncludeFile = decoding::Environment::GetInstance().GetDataPath();
		baseIncludeFile += L"..\\src\\platforms\\xenon\\launcher64\\xenonBase.h";
		codeGen.AddInclude(baseIncludeFile);
	}

	// build full path to the xenon CPU definition file
	{
		std::wstring cpuIncludeFile = decoding::Environment::GetInstance().GetDataPath();
		cpuIncludeFile += L"..\\src\\platforms\\xenon\\launcher64\\xenonCPU.h";
		codeGen.AddInclude(cpuIncludeFile);
	}

	// export symbols
	{
		const image::Binary* image = m_context->GetImage();
		const uint32 numImports = image->GetNumImports();
		for (uint32 i = 0; i<numImports; ++i)
		{
			const image::Import* import = image->GetImport(i);

			if (import->GetType() == image::Import::eImportType_Function && import->GetEntryAddress() != 0)
			{
				codeGen.AddImportSymbol(import->GetExportName(), import->GetTableAddress(), import->GetEntryAddress());
			}
			else if (import->GetType() == image::Import::eImportType_Data)
			{
				codeGen.AddImportSymbol(import->GetExportName(), import->GetTableAddress(), 0);
			}
		}
	}

	// process all instructions in the code range
	uint32 address = m_codeStartAddress;
	uint32 blockAddress = m_codeStartAddress;
	bool hadValidCode = false;
	while (address < m_codeEndAddress)
	{
		log.SetTaskProgress(address - m_codeStartAddress, m_codeEndAddress - m_codeStartAddress);

		// get flags
		const decoding::MemoryFlags flags = m_context->GetMemoryMap().GetMemoryInfo(address);

		// make sure memory is executable
		if (!flags.IsExecutable())
		{
			if (hadValidCode)
			{
				log.Error("CodeGen: Encountered not executable memory at %06Xh.", address);
				codeGen.CloseFunction();
			}

			hadValidCode = false;
			address += 4;
			continue;
		}

		// valid code
		hadValidCode = true;

		// function start
		if (flags.GetInstructionFlags().IsFunctionStart())
		{
			// is this imported function ?
			const decoding::MemoryFlags flags = m_context->GetMemoryMap().GetMemoryInfo(address);
			const bool isImportFunction = flags.GetInstructionFlags().IsImportFunction();

			// start function
			const char* name = m_context->GetNameMap().GetName(address);
			codeGen.StartFunction(address, name, isImportFunction);

			// special case for import thunks, ehh
			if (flags.GetInstructionFlags().IsImportFunction() || flags.GetInstructionFlags().IsThunk())
			{
				{
					codeGen.StartBlock(address, false);
					codeGen.AddCodef(address, "cpu::unimplemented_import_function( 0x%08X, \"%s\" );\n", address, name);
					codeGen.CloseBlock();
				}

				address += 16; // full import!
				codeGen.CloseFunction();
				continue;
			}
		}

		// block start
		if (flags.GetInstructionFlags().IsBlockStart())
		{
			bool isMultiAddressBlock = false; // TODO
			if (options.m_forceMultiAddressBlocks) isMultiAddressBlock = true;
			if (options.m_forceNoMultiAddressBlocks) isMultiAddressBlock = false;
			codeGen.StartBlock(address, isMultiAddressBlock);
			blockAddress = address;
		}

		// get instruction
		const decoding::Instruction op = m_context->DecodeInstruction(log, address);
		if (!op.IsValid())
		{
			log.Error("CodeGen: Encountered invalid instruction at %06Xh.", address);
			codeGen.AddCodef(address, "/* unknown instruction */\n", address);
			codeGen.AddCodef(address, "cpu::invalid_instruction( 0x%08X, 0x%08X );\n", address, blockAddress);
			address += 4;
			continue;
		}

		// generate code for CPU instruction
		if (!ExportSingleInstruction(log, options, codeGen, address, op))
		{
			return false;
		}

		// advance
		address += op.GetCodeSize();
	}

	// prepare the glue info
	CodeGeneratorGlueData glueInfo;
	glueInfo.m_imageLoadAddress = m_context->GetImage()->GetBaseAddress();
	glueInfo.m_imageSize = m_context->GetImage()->GetMemorySize();
	glueInfo.m_entryPointAddrses = m_context->GetImage()->GetEntryAddress();
	glueInfo.m_codeConfigName = options.m_codeConfigName;
	glueInfo.m_enableCodeTrace = options.m_debugEnableCodeTrace;
	glueInfo.m_enableMemoryTrace = options.m_debugEnableMemoryTrace;

	// end
	codeGen.Finish(glueInfo);

	// done
	return true;
}

#define INVALID_CASE else { log.Error( "CodeGen: Unhandled instruction type of '%s' at %06Xh", op.GetOpcode()->GetName(), address ); return false; }

#define UPDATE_ARG1_IMM { codeGen.AddCodef( address, "regs.%s = (uint32)(regs.%s + 0x%08X);\n", op.GetArg1().m_reg->GetName(), op.GetArg1().m_reg->GetName(), op.GetArg1().m_imm ); }
#define UPDATE_ARG1_REG2 { codeGen.AddCodef( address, "regs.%s = (uint32)(regs.%s + regs.%s);\n", op.GetArg1().m_reg->GetName(), op.GetArg1().m_reg->GetName(), op.GetArg2().m_reg->GetName() ); }

static inline bool CheckOperand(const decoding::Instruction::Operand& op)
{
	if (op.m_type == decoding::Instruction::eType_None) return true;
	if (op.m_type == decoding::Instruction::eType_Reg) return true;
	if (op.m_type == decoding::Instruction::eType_Imm) return true;
	return false;
}

static inline bool CheckOperandDest(const decoding::Instruction::Operand& op)
{
	if (op.m_type == decoding::Instruction::eType_Reg) return true;
	return false;
}

static inline void ExportOperand(std::string& txt, const decoding::Instruction::Operand& op)
{
	if (op.m_type != decoding::Instruction::eType_None)
	{
		txt += ",";

		if (op.m_type == decoding::Instruction::eType_Reg)
		{
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
}

bool decoding::InterfaceXenon::ExportSingleInstruction(ILogOutput& log, const decoding::CodeExportOptions& options, class code::IGenerator& codeGen, const uint32 address, const decoding::Instruction& op)
{
	//  emit raw instruction
	if (options.m_emitOriginalCode)
	{
		char buffer[256];

		if (!DecompilationXenon::GenerateInstructionTextImpl(op, buffer, 256))
		{
			char* printStream = buffer;
			op.GenerateText(printStream, printStream + 256);
		}

		codeGen.AddCodef(address, "/* %08Xh\t\t %s */\n", address, buffer);
	}

	// detect last "." (control flag)
	char opcodeName[32];
	strcpy_s(opcodeName, op.GetOpcode()->GetName());
	const size_t opcodeNameLen = strlen(opcodeName);
	const bool controlFlags = (opcodeName[opcodeNameLen - 1] == '.');
	if (controlFlags)
	{
		opcodeName[opcodeNameLen - 1] = 0;
	}

	// do we exit with branch ?
	bool exitWithUnconditionalBranch = false;

	// per-instruction logic
	if (op == "nop" || op == "nop_zero")
	{
		codeGen.AddCode(address, "cpu::op::nop();\n");
	}
	else if (op == "bl")
	{
		if (op.MatchOperands(decoding::Instruction::eType_Imm))
		{
			const uint32 targetAddr = address + op.GetArg0().m_imm;
			codeGen.AddCodef(address, "regs.LR = 0x%08X; return 0x%08X;\n", address + 4, targetAddr);
			exitWithUnconditionalBranch = true;
		}
		INVALID_CASE;
	}
	else if (op == "b")
	{
		if (op.MatchOperands(decoding::Instruction::eType_Imm))
		{
			const uint32 targetAddr = address + op.GetArg0().m_imm;
			codeGen.AddCodef(address, "return 0x%08X;\n", targetAddr);
			exitWithUnconditionalBranch = true;
		}
		INVALID_CASE;
	}
	else if (op == "mfspr" || op == "mtspr" || op == "mr" || op == "mfmsr")
	{
		if (op.MatchOperands(decoding::Instruction::eType_Reg, decoding::Instruction::eType_Reg))
		{
			codeGen.AddCodef(address, "regs.%s = regs.%s;\n", op.GetArg0().m_reg->GetName(), op.GetArg1().m_reg->GetName());
		}
		INVALID_CASE;
	}
	// time base
	else if (op == "mftb")
	{
		if (op.MatchOperands(decoding::Instruction::eType_Reg, decoding::Instruction::eType_Imm, decoding::Instruction::eType_Imm))
		{
			if (op.GetArg1().m_imm == 12 && op.GetArg2().m_imm == 8)
			{
				// QueryPerformanceCounter
				codeGen.AddCodef(address, "regs.%s = cpu::op::rtc();\n", op.GetArg0().m_reg->GetName());
			}
			INVALID_CASE;
		}
		INVALID_CASE;
	}
	// simple stores (std,stdu,stw,stwu,stb,stbu)
	else if (op.Match("st#") || op.Match("st#u"))
	{
		if (op.MatchOperands(decoding::Instruction::eType_Reg, decoding::Instruction::eType_MemReg))
		{
			// data size
			uint32 size = 0;
			if (op[2] == 'b') size = 8;
			else if (op[2] == 'h') size = 16;
			else if (op[2] == 'w') size = 32;
			else if (op[2] == 'd') size = 64;
			INVALID_CASE;

			// general store
			codeGen.AddCodef(address, "cpu::mem::store%d( (uint%d)regs.%s, regs.%s + 0x%08X );\n",
				size,
				size,
				op.GetArg0().m_reg->GetName(),
				op.GetArg1().m_reg->GetName(), op.GetArg1().m_imm);

			// address update
			if (op[3] == 'u')
			{
				UPDATE_ARG1_IMM;
			}
		}
		INVALID_CASE;
	}
	// indexed stores (stxd,stxdu,stw,stwu,stb,stbu)
	else if (op.Match("st#x") || op.Match("st#ux"))
	{
		if (op.MatchOperands(decoding::Instruction::eType_Reg, decoding::Instruction::eType_Reg, decoding::Instruction::eType_Reg))
		{
			// data size
			uint32 size = 0;
			if (op[2] == 'b') size = 8;
			else if (op[2] == 'h') size = 16;
			else if (op[2] == 'w') size = 32;
			else if (op[2] == 'd') size = 64;
			INVALID_CASE;

			// general store
			codeGen.AddCodef(address, "cpu::mem::store%d( (uint%d)regs.%s, regs.%s + regs.%s );\n",
				size,
				size,
				op.GetArg0().m_reg->GetName(),
				op.GetArg1().m_reg->GetName(), op.GetArg2().m_reg->GetName());

			// address update
			if (op[3] == 'u')
			{
				UPDATE_ARG1_REG2;
			}
		}
		INVALID_CASE;
	}
	// simple zero fill loads
	else if (op.Match("l#z") || op.Match("l#zu"))
	{
		if (op.MatchOperands(decoding::Instruction::eType_Reg, decoding::Instruction::eType_MemReg))
		{
			// data size
			uint32 size = 0;
			if (op[1] == 'b') size = 8;
			else if (op[1] == 'h') size = 16;
			else if (op[1] == 'w') size = 32;
			else if (op[1] == 'd') size = 64;
			INVALID_CASE;

			// general load
			codeGen.AddCodef(address, "cpu::mem::load%dz( &regs.%s, regs.%s + 0x%08X );\n",
				size,
				op.GetArg0().m_reg->GetName(),
				op.GetArg1().m_reg->GetName(), op.GetArg1().m_imm);

			// address update
			if (op[3] == 'u')
			{
				UPDATE_ARG1_IMM;
			}
		}
		else if (op.MatchOperands(decoding::Instruction::eType_Reg, decoding::Instruction::eType_MemImm))
		{
			// data size
			uint32 size = 0;
			if (op[1] == 'b') size = 8;
			else if (op[1] == 'h') size = 16;
			else if (op[1] == 'w') size = 32;
			else if (op[1] == 'd') size = 64;
			INVALID_CASE;

			// export global address for reading
			std::string symbolName;
			codeGen.AddGlobal(op.GetArg1().m_imm, size, NULL, false, symbolName);

			// general load
			codeGen.AddCodef(address, "extern uint%d %s;\n",
				size, symbolName.c_str());
			codeGen.AddCodef(address, "regs.%s = %s;\n",
				op.GetArg0().m_reg->GetName(),
				symbolName.c_str());
		}
		INVALID_CASE;
	}
	// indexed zero fill loads
	else if (op.Match("l#zx") || op.Match("l#zux"))
	{
		// format dest = mem[src0 + src1]
		if (op.MatchOperands(decoding::Instruction::eType_Reg, decoding::Instruction::eType_Reg, decoding::Instruction::eType_Reg))
		{
			// data size
			uint32 size = 0;
			if (op[1] == 'b') size = 8;
			else if (op[1] == 'h') size = 16;
			else if (op[1] == 'w') size = 32;
			INVALID_CASE;

			// general load
			codeGen.AddCodef(address, "cpu::mem::load%dz( &regs.%s, regs.%s + regs.%s );\n",
				size,
				op.GetArg0().m_reg->GetName(),
				op.GetArg1().m_reg->GetName(), op.GetArg2().m_reg->GetName());

			// address update
			if (op[4] == 'u')
			{
				UPDATE_ARG1_REG2;
			}
		}
		INVALID_CASE;
	}
	// simple algebraic fill loads
	else if (op.Match("l#a") || op.Match("l#au"))
	{
		if (op.MatchOperands(decoding::Instruction::eType_Reg, decoding::Instruction::eType_MemReg))
		{
			// data size
			uint32 size = 0;
			if (op[1] == 'b') size = 8;
			else if (op[1] == 'h') size = 16;
			else if (op[1] == 'w') size = 32;
			INVALID_CASE;

			// general load
			codeGen.AddCodef(address, "cpu::mem::load%da( &regs.%s, regs.%s + 0x%08X );\n",
				size,
				op.GetArg0().m_reg->GetName(),
				op.GetArg1().m_reg->GetName(), op.GetArg1().m_imm);

			// address update
			if (op[3] == 'u')
			{
				UPDATE_ARG1_IMM;
			}
		}
		INVALID_CASE;
	}
	// indexed zero fill loads
	else if (op.Match("l#ax") || op.Match("l#aux"))
	{
		// format dest = mem[src0 + src1]
		if (op.MatchOperands(decoding::Instruction::eType_Reg, decoding::Instruction::eType_Reg, decoding::Instruction::eType_Reg))
		{
			// data size
			uint32 size = 0;
			if (op[1] == 'b') size = 8;
			else if (op[1] == 'h') size = 16;
			else if (op[1] == 'w') size = 32;
			INVALID_CASE;

			// general load
			codeGen.AddCodef(address, "cpu::mem::load%da( &regs.%s, regs.%s + regs.%s );\n",
				size,
				op.GetArg0().m_reg->GetName(),
				op.GetArg1().m_reg->GetName(), op.GetArg2().m_reg->GetName());

			// address update
			if (op[3] == 'u')
			{
				UPDATE_ARG1_REG2;
			}
		}
		INVALID_CASE;
	}
	// 64-bit load
	else if (op == "ld" || op == "ldu")
	{
		// format dest = mem[src0 + ofs]
		if (op.MatchOperands(decoding::Instruction::eType_Reg, decoding::Instruction::eType_MemReg))
		{
			// general load
			codeGen.AddCodef(address, "cpu::mem::load64( &regs.%s, regs.%s + 0x%08X );\n",
				op.GetArg0().m_reg->GetName(),
				op.GetArg1().m_reg->GetName(), op.GetArg1().m_imm);

			// address update
			if (op[2] == 'u')
			{
				UPDATE_ARG1_IMM;
			}
		}
		INVALID_CASE;
	}
	// 64-bit indexed load
	else if (op == "ldx" || op == "ldux")
	{
		// format dest = mem[src0 + src1]
		if (op.MatchOperands(decoding::Instruction::eType_Reg, decoding::Instruction::eType_Reg, decoding::Instruction::eType_Reg))
		{
			// general load
			codeGen.AddCodef(address, "cpu::mem::load64( &regs.%s, regs.%s + regs.%s );\n",
				op.GetArg0().m_reg->GetName(),
				op.GetArg1().m_reg->GetName(), op.GetArg2().m_reg->GetName());

			// address update
			if (op[2] == 'u')
			{
				UPDATE_ARG1_REG2;
			}
		}
		INVALID_CASE;
	}
	// 32-bit float load
	else if (op == "lfs" || op == "lfsu")
	{
		// format dest = mem[src0 + ofs]
		if (op.MatchOperands(decoding::Instruction::eType_Reg, decoding::Instruction::eType_MemReg))
		{
			// general load
			codeGen.AddCodef(address, "cpu::mem::load32f( &regs.%s, regs.%s + 0x%08X );\n",
				op.GetArg0().m_reg->GetName(),
				op.GetArg1().m_reg->GetName(), op.GetArg1().m_imm);

			// address update
			if (op[3] == 'u')
			{
				UPDATE_ARG1_IMM;
			}
		}
		INVALID_CASE;
	}
	// 32-bit float load
	else if (op == "lfsx" || op == "lfsux")
	{
		// format dest = mem[src0 + src1]
		if (op.MatchOperands(decoding::Instruction::eType_Reg, decoding::Instruction::eType_Reg, decoding::Instruction::eType_Reg))
		{
			// general load
			codeGen.AddCodef(address, "cpu::mem::load32f( &regs.%s, regs.%s + regs.%s );\n",
				op.GetArg0().m_reg->GetName(),
				op.GetArg1().m_reg->GetName(), op.GetArg2().m_reg->GetName());

			// address update
			if (op[3] == 'u')
			{
				UPDATE_ARG1_REG2;
			}
		}
		INVALID_CASE;
	}
	// 64-bit float load
	else if (op == "lfd" || op == "lfdu")
	{
		// format dest = mem[src0 + ofs]
		if (op.MatchOperands(decoding::Instruction::eType_Reg, decoding::Instruction::eType_MemReg))
		{
			// general load
			codeGen.AddCodef(address, "cpu::mem::load64f( &regs.%s, regs.%s + 0x%08X );\n",
				op.GetArg0().m_reg->GetName(),
				op.GetArg1().m_reg->GetName(), op.GetArg1().m_imm);

			// address update
			if (op[3] == 'u')
			{
				UPDATE_ARG1_IMM;
			}
		}
		INVALID_CASE;
	}
	// 64-bit float load
	else if (op == "lfdx" || op == "lfdux")
	{
		// format dest = mem[src0 + src1]
		if (op.MatchOperands(decoding::Instruction::eType_Reg, decoding::Instruction::eType_Reg, decoding::Instruction::eType_Reg))
		{
			// general load
			codeGen.AddCodef(address, "cpu::mem::load64f( &regs.%s, regs.%s + regs.%s );\n",
				op.GetArg0().m_reg->GetName(),
				op.GetArg1().m_reg->GetName(), op.GetArg2().m_reg->GetName());

			// address update
			if (op[3] == 'u')
			{
				UPDATE_ARG1_REG2;
			}
		}
		INVALID_CASE;
	}
	// 32-bit float store
	else if (op.Match("stfs") || op.Match("stfsu"))
	{
		if (op.MatchOperands(decoding::Instruction::eType_Reg, decoding::Instruction::eType_MemReg))
		{
			// general store
			codeGen.AddCodef(address, "cpu::mem::store32f( regs.%s, regs.%s + 0x%08X );\n",
				op.GetArg0().m_reg->GetName(),
				op.GetArg1().m_reg->GetName(), op.GetArg1().m_imm);

			// address update
			if (op[4] == 'u')
			{
				UPDATE_ARG1_IMM;
			}
		}
		INVALID_CASE;
	}
	// 64-bit float store
	else if (op.Match("stfd") || op.Match("stfdu"))
	{
		if (op.MatchOperands(decoding::Instruction::eType_Reg, decoding::Instruction::eType_MemReg))
		{
			// general store
			codeGen.AddCodef(address, "cpu::mem::store64f( regs.%s, regs.%s + 0x%08X );\n",
				op.GetArg0().m_reg->GetName(),
				op.GetArg1().m_reg->GetName(), op.GetArg1().m_imm);

			// address update
			if (op[4] == 'u')
			{
				UPDATE_ARG1_IMM;
			}
		}
		INVALID_CASE;
	}
	// 32-bit indexed store for floating point
	else if (op.Match("stfsx") || op.Match("stfsux"))
	{
		if (op.MatchOperands(decoding::Instruction::eType_Reg, decoding::Instruction::eType_Reg, decoding::Instruction::eType_Reg))
		{
			// general store
			codeGen.AddCodef(address, "cpu::mem::store32f( regs.%s, regs.%s + regs.%s );\n",
				op.GetArg0().m_reg->GetName(),
				op.GetArg1().m_reg->GetName(), op.GetArg2().m_reg->GetName());

			// address update
			if (op[4] == 'u')
			{
				UPDATE_ARG1_REG2;
			}
		}
		INVALID_CASE;
	}
	// 64-bit indexed store for floating point
	else if (op.Match("stfdx") || op.Match("stfdux"))
	{
		if (op.MatchOperands(decoding::Instruction::eType_Reg, decoding::Instruction::eType_Reg, decoding::Instruction::eType_Reg))
		{
			// general store
			codeGen.AddCodef(address, "cpu::mem::store64f( regs.%s, regs.%s + regs.%s );\n",
				op.GetArg0().m_reg->GetName(),
				op.GetArg1().m_reg->GetName(), op.GetArg2().m_reg->GetName());

			// address update
			if (op[4] == 'u')
			{
				UPDATE_ARG1_REG2;
			}
		}
		INVALID_CASE;
	}
	// compare
	else if (op.MatchN<3>("cmp"))
	{
		// with immediate
		if (op.MatchOperands(decoding::Instruction::eType_Reg, decoding::Instruction::eType_Reg, decoding::Instruction::eType_Imm))
		{
			const int crIndex = atoi(op.GetArg0().m_reg->GetName() + 2);
			codeGen.AddCodef(address, "cpu::op::%s<%d>(regs,regs.%s,0x%08X);\n",
				opcodeName, crIndex,
				op.GetArg1().m_reg->GetName(),
				op.GetArg2().m_imm);
		}
		// with reg
		else if (op.MatchOperands(decoding::Instruction::eType_Reg, decoding::Instruction::eType_Reg, decoding::Instruction::eType_Reg))
		{
			const int crIndex = atoi(op.GetArg0().m_reg->GetName() + 2);
			codeGen.AddCodef(address, "cpu::op::%s<%d>(regs,regs.%s,regs.%s);\n",
				opcodeName, crIndex,
				op.GetArg1().m_reg->GetName(),
				op.GetArg2().m_reg->GetName());
		}
		INVALID_CASE;
	}
	// float compare
	else if (op.MatchN<5>("fcmpu") || op.MatchN<5>("fcmpo"))
	{
		// with immediate
		if (op.MatchOperands(decoding::Instruction::eType_Reg, decoding::Instruction::eType_Reg, decoding::Instruction::eType_Reg))
		{
			const int crIndex = atoi(op.GetArg0().m_reg->GetName() + 2);
			codeGen.AddCodef(address, "cpu::op::%s<%d>(regs,regs.%s,regs.%s);\n",
				opcodeName, crIndex,
				op.GetArg1().m_reg->GetName(),
				op.GetArg1().m_reg->GetName());
		}
		INVALID_CASE;
	}
	// shifts
	else if (op.MatchN<6>("rldicl") || op.MatchN<6>("rldicr") || op.MatchN<5>("rldic") || op.MatchN<6>("rldimi"))
	{
		if (op.MatchOperands(decoding::Instruction::eType_Reg, decoding::Instruction::eType_Reg, decoding::Instruction::eType_Imm, decoding::Instruction::eType_Imm))
		{
			codeGen.AddCodef(address, "cpu::op::%s<%d,%d,%d>(regs,&regs.%s,regs.%s);\n",
				opcodeName,
				controlFlags ? 1 : 0,
				op.GetArg2().m_imm, // shift val
				op.GetArg3().m_imm, // mask
				op.GetArg0().m_reg->GetName(),
				op.GetArg1().m_reg->GetName());
		}
		INVALID_CASE;
	}
	// shifts with reg param 
	else if (op.MatchN<5>("rldcl") || op.MatchN<5>("rldcr"))
	{
		if (op.MatchOperands(decoding::Instruction::eType_Reg, decoding::Instruction::eType_Reg, decoding::Instruction::eType_Reg, decoding::Instruction::eType_Imm))
		{
			codeGen.AddCodef(address, "cpu::op::%s<%d,%d>(regs,&regs.%s,regs.%s,regs.%s);\n",
				opcodeName,
				controlFlags ? 1 : 0,
				op.GetArg3().m_imm,
				op.GetArg0().m_reg->GetName(),
				op.GetArg1().m_reg->GetName(),
				op.GetArg2().m_reg->GetName());
		}
		INVALID_CASE;
	}
	// shifts with immediate params with mask 
	else if (op.MatchN<6>("rlwimi") || op.MatchN<6>("rlwinm") || op.MatchN<5>("rlwnm"))
	{
		if (op.MatchOperands(decoding::Instruction::eType_Reg, decoding::Instruction::eType_Reg, decoding::Instruction::eType_Imm, decoding::Instruction::eType_Imm, decoding::Instruction::eType_Imm))
		{
			codeGen.AddCodef(address, "cpu::op::%s<%d,%d,%d,%d>(regs,&regs.%s,regs.%s);\n",
				opcodeName,
				controlFlags ? 1 : 0,
				op.GetArg2().m_imm, // shift val
				op.GetArg3().m_imm, // mask B
				op.GetArg4().m_imm, // mask M
				op.GetArg0().m_reg->GetName(),
				op.GetArg1().m_reg->GetName());
		}
		INVALID_CASE;
	}
	// algebraic immediate shifts
	else if (op.MatchN<5>("sradi") || op.MatchN<5>("srawi"))
	{
		if (op.MatchOperands(decoding::Instruction::eType_Reg, decoding::Instruction::eType_Reg, decoding::Instruction::eType_Imm))
		{
			codeGen.AddCodef(address, "cpu::op::%s<%d,%d>(regs,&regs.%s,regs.%s);\n",
				opcodeName,
				controlFlags ? 1 : 0,
				op.GetArg2().m_imm, // shift val
				op.GetArg0().m_reg->GetName(),
				op.GetArg1().m_reg->GetName());
		}
		INVALID_CASE;
	}
	// conditional branch to link register
	else if (op == "bclr" || op == "bclrl")
	{
		if (op.MatchOperands(decoding::Instruction::eType_Imm, decoding::Instruction::eType_Reg) && op.GetArg1().m_reg->GetParent())
		{
			const int branchType = op.GetArg0().m_imm;

			// format the full test register name
			char ctrRegName[16];
			{
				const int ctrReg = op.GetArg1().m_reg->GetParent()->GetValue();
				const int ctrTypeReg = op.GetArg1().m_reg->GetValue();
				const char* ctrFlags[4] = { ".lt", ".gt", ".eq", ".so" };
				sprintf_s(ctrRegName, sizeof(ctrRegName), "CR[%d]%s", ctrReg, ctrFlags[ctrTypeReg]);
			}

			// format the full jump code
			char branchCode[128];
			{
				if (op == "bclrl")
				{
					sprintf_s(branchCode, sizeof(branchCode), "auto tempLR = regs.LR; regs.LR = 0x%08X; return (uint32)tempLR;",
						address + 4);
					//exitWithUnconditionalBranch = true;
				}
				else
				{
					sprintf_s(branchCode, sizeof(branchCode), "return (uint32)regs.LR;");
					//exitWithUnconditionalBranch = true;
				}
			}

			// create branch code
			GenerateBranchCode(log, options, codeGen, address, branchType, ctrRegName, branchCode, false);
		}
		INVALID_CASE;
	}
	// conditional branch to count register
	else if (op == "bcctr" || op == "bcctrl")
	{
		if (op.MatchOperands(decoding::Instruction::eType_Imm, decoding::Instruction::eType_Reg) && op.GetArg1().m_reg->GetParent())
		{
			const int branchType = op.GetArg0().m_imm;

			// format the full test register name
			char ctrRegName[16];
			{
				const int ctrReg = op.GetArg1().m_reg->GetParent()->GetValue();
				const int ctrTypeReg = op.GetArg1().m_reg->GetValue();
				const char* ctrFlags[4] = { ".lt", ".gt", ".eq", ".so" };
				sprintf_s(ctrRegName, sizeof(ctrRegName), "CR[%d]%s", ctrReg, ctrFlags[ctrTypeReg]);
			}

			// format the full jump code
			char branchCode[128];
			{
				if (op == "bcctrl")
				{
					sprintf_s(branchCode, sizeof(branchCode), "regs.LR = 0x%08X; return (uint32)regs.CTR;",
						address + 4);
					//exitWithBranch = true;
				}
				else
				{
					sprintf_s(branchCode, sizeof(branchCode), "return (uint32)regs.CTR;");
					//exitWithBranch = true;
				}
			}

			// create branch code
			GenerateBranchCode(log, options, codeGen, address, branchType, ctrRegName, branchCode, true);
		}
		INVALID_CASE;
	}
	// conditional branch
	else if (op == "bc" || op == "bcl" || op == "bca" || op == "bcla")
	{
		if (op.MatchOperands(decoding::Instruction::eType_Imm, decoding::Instruction::eType_Reg, decoding::Instruction::eType_Imm) && op.GetArg1().m_reg->GetParent())
		{
			const int branchType = op.GetArg0().m_imm;
			const bool branchAbsolute = (op == "bca" || op == "bcla");
			const uint32 branchTarget = branchAbsolute ? (op.GetArg2().m_imm) : (op.GetArg2().m_imm + address);

			// format the full test register name
			char ctrRegName[16];
			{
				const int ctrReg = op.GetArg1().m_reg->GetParent()->GetValue();
				const int ctrTypeReg = op.GetArg1().m_reg->GetValue();
				const char* ctrFlags[4] = { ".lt", ".gt", ".eq", ".so" };
				sprintf_s(ctrRegName, sizeof(ctrRegName), "CR[%d]%s", ctrReg, ctrFlags[ctrTypeReg]);
			}

			// format the full jump code
			char branchCode[128];
			{
				if (op == "bcl" || op == "bcla")
				{
					sprintf_s(branchCode, sizeof(branchCode), "regs.LR = 0x%08X; return 0x%08X; %s",
						address + 4,
						branchTarget,
						branchAbsolute ? "/*ABSOLUTE*/" : "");
					//exitWithBranch = true;
				}
				else
				{
					sprintf_s(branchCode, sizeof(branchCode), "return 0x%08X; %s",
						branchTarget,
						branchAbsolute ? "/*ABSOLUTE*/" : "");
					//exitWithBranch = true;
				}
			}

			// Resolve jump code
			GenerateBranchCode(log, options, codeGen, address, branchType, ctrRegName, branchCode, false);
		}
		INVALID_CASE;
	}
	// trap
	else if (op == "twi" || op == "tdi")
	{
		if (op.MatchOperands(decoding::Instruction::eType_Imm, decoding::Instruction::eType_Reg, decoding::Instruction::eType_Imm))
		{
			const uint32 flags = op.GetArg0().m_imm & 0x1F;

			if (flags == 0x1F)
			{
				// unconditonal trap
				codeGen.AddCodef(address, "cpu::op::trap(0x%08X, 0x%08X, regs); // unconditional trap\n", address, op.GetArg2().m_imm);
			}
			else
			{
				// conditonal trap
				codeGen.AddCodef(address, "cpu::op::%s<%d>(0x%08X, regs.%s, 0x%08X);\n",
					opcodeName,
					flags,
					address,
					op.GetArg1().m_reg->GetName(),
					op.GetArg2().m_imm);
			}
		}
		INVALID_CASE;

	}
	// generic
	else
	{
		// in order to auto generate the call we need to have very special parameters :)
		bool argsOK = CheckOperand(op.GetArg0());
		argsOK &= CheckOperand(op.GetArg1());
		argsOK &= CheckOperand(op.GetArg2());
		argsOK &= CheckOperand(op.GetArg3());
		argsOK &= CheckOperand(op.GetArg4());
		argsOK &= CheckOperand(op.GetArg5());

		// valid ?
		if (argsOK && op.GetArg0().m_type == decoding::Instruction::eType_Reg)
		{
			// auto format the instruction call
			std::string code = "cpu::op::";

			// notes: ALL immediate values are extended to unsigned 32 bits
			code += opcodeName;
			code += controlFlags ? "<1>" : "<0>";

			// parameters
			{
				code += "(regs,&regs.";
				code += op.GetArg0().m_reg->GetName();

				(code, op.GetArg1());
				ExportOperand(code, op.GetArg2());
				ExportOperand(code, op.GetArg3());
				ExportOperand(code, op.GetArg4());
				ExportOperand(code, op.GetArg5());
				code += ");";
			}

			// end
			code += "\n";

			// export
			codeGen.AddCode(address, code.c_str());
		}
		else
		{
			log.Error("CodeGen: failed to generate code at address %06Xh', instruction = '%s'", address, opcodeName);
		}
	}

	// we are using the "trace" option - we exit after each instruction to enable the trace stuff
	if (options.m_debugEnableCodeTrace && !exitWithUnconditionalBranch)
	{
		codeGen.AddCodef(address, "return 0x%08X;\n", address + 4);
	}

	// done
	return true;
}

#endif

//---------------------------------------------------------------------------
