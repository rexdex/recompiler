#pragma once

//---------------------------------------------------------------------------

namespace platform
{
	class CPURegister;
	class CPUInstruction;
}

//---------------------------------------------------------------------------

namespace trace
{
	class Registers;
	class DataFrame;
}

//---------------------------------------------------------------------------

namespace decoding
{

	// Extended instruction information
	// The actual workhorse of the whole thing
	class RECOMPILER_API InstructionExtendedInfo
	{
	public:
		static const uint32 MAX_REGISTERS = 8;

		enum EMemoryFlags
		{
			eMemoryFlags_Read		= 1 << 0, // memory is being read by the instruction
			eMemoryFlags_Write		= 1 << 1, // memory is being written by the instruction
			eMemoryFlags_Touch		= 1 << 2, // memory is being touched but not read/write (cache)
			eMemoryFlags_Depends	= 1 << 3, // instruction depends on the memory content in some obscure way
			eMemoryFlags_Aligned	= 1 << 4, // instruction expects memory address to be aligned
			eMemoryFlags_Float		= 1 << 5, // memory is expected to be loading point value
			eMemoryFlags_DirectMap  = 1 << 6, // memory access is directly mapped (write/read instructions)
		};

		enum EInstructionFlags
		{
			eInstructionFlag_Nop			= 1 << 0, // instruction can be removed
			eInstructionFlag_Jump			= 1 << 1, // instruction represents a simple jump
			eInstructionFlag_Call			= 1 << 2, // instruction represents a function call
			eInstructionFlag_Return			= 1 << 3, // instruction represents a return from function
			eInstructionFlag_Diverge		= 1 << 4, // instruction can diverge a flow of code (conditional branch)
			eInstructionFlag_Conditional	= 1 << 5, // instruction is conditional (uses condition registers or flags)
			eInstructionFlag_ModifyFlags	= 1 << 6, // instruction is modifying the flag registers
			eInstructionFlag_Static			= 1 << 7, // instruction branch target is static
		};

		enum ERegDependencyMode
		{
			eReg_None = 0,
			eReg_Dependency = 1,
			eReg_Output = 2,
			eReg_Both = 3,
		};

		// instruction address and flags
		uint32		m_codeAddress;
		uint32		m_codeFlags;

		// speculated branch target (if known)
		uint64							m_branchTargetAddress;
		const platform::CPURegister*	m_branchTargetReg;

		// memory access info (if any)
		uint64							m_memoryAddressOffset;
		const platform::CPURegister*	m_memoryAddressBase;
		const platform::CPURegister*	m_memoryAddressIndex;
		uint32							m_memoryAddressScale;
		uint32							m_memoryAddressMask;
		uint32							m_memoryFlags;
		uint32							m_memorySize;

		// registers read (a fully modified register is NOT a dependency)
		const platform::CPURegister*	m_registersDependencies[ MAX_REGISTERS ];
		uint32							m_registersDependenciesCount;

		// registers modified
		const platform::CPURegister*	m_registersModified[ MAX_REGISTERS ];
		uint32							m_registersModifiedCount;

		//---

		InstructionExtendedInfo();

		bool AddRegister(const platform::CPURegister* reg, const ERegDependencyMode mode);
		bool AddRegisterDependency(const platform::CPURegister* reg);
		bool AddRegisterOutput(const platform::CPURegister* reg, const bool modifiesWholeRegister=true);

		// compute memory address used by this instruction
		bool ComputeMemoryAddress(const trace::DataFrame& data, uint64& outAddress) const;

		// compute branch target address
		bool ComputeBranchTargetAddress(const trace::DataFrame& data, uint64& outAddress) const;
	};

} // decoding

//---------------------------------------------------------------------------
