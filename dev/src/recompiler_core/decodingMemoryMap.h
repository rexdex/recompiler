#pragma once

class IBinaryFileReader;
class IBinaryFileWriter;

namespace image
{
	class Binary;
}

namespace decoding
{
	//---------------------------------------------------------------------------

	// Generic type of memory location - as read from the section
	enum class MemoryFlag : uint16
	{
		// valid memory (comes from section)
		Valid = 1 << 0,

		// flag set only for the first bytes of multi byte entries
		FirstByte = 1 << 1,

		// read only data
		ReadOnly = 1 << 2,

		// executable data (but may be not a valid instruction, beware, check the instruction flags to be sure)
		Executable = 1 << 4,

		// generic data (may be not used by code)
		GenericData = 1 << 5,

		// confirmed data (address taken in the code, found by static analysys or visitedby in a memory trace)
		ReferencedData = 1 << 6,

		// section start (get's special treatment)
		SectionStart = 1 << 7,

		// do we have optional comment for this line ?
		HasComment = 1 << 8,

		// do we have problem reported on this line ?
		HasProblem = 1 << 9,

		// breakpoint on execution/data access
		HasBreakpoint = 1 << 10,

		// this memory location was visited at least once (data from trace)
		WasVisited = 1 << 11,
	};

	//---------------------------------------------------------------------------

	// Decoding data flags (may be empty)
	enum class DataFlag : uint16
	{
		// data appears to be signed (user flag)
		Signed = 1 << 1,

		// data appears to be floating point (float,double or vec4 depends on the size)
		Float = 1 << 2,

		// data appears to be address in code (jump/vtable)
		CodePtr = 1 << 3,

		// data appears to be address in data (pointer)
		DataPtr = 1 << 4,

		// data is references by code
		HasCodeRef = 1 << 5,

		// data is references by other data
		HasDataRef = 1 << 6,

		// data was referenced in a indirect call (vtable ?) - trace information only
		IndirectCallPtr = 1 << 7,

		// data was referenced in a indirect jump - trace information only
		IndirectJumpPtr = 1 << 8,
	};

	//---------------------------------------------------------------------------

	// Decoding instruction flags (valid only for memory marked with "executable" flag, never 0)
	enum class InstructionFlag : uint32
	{
		// valid instruction
		Valid = 1 << 0,

		// instruction first byte
		FirstByte = 1 << 1,

		// this is a call instruction
		Call = 1 << 2,

		// this is a jump instruction
		Jump = 1 << 3,

		// this is a return from call instruction
		Ret = 1 << 4,

		// this is a conditional instruction
		Conditional = 1 << 5,

		// this is a memory access instruction
		Memory = 1 << 6,

		// this is an indirect jump/call instruction (address not know)
		Indirect = 1 << 7,

		// this is a privledged instruction
		Privledged = 1 << 8,

		// this instruction is static jump target
		StaticJumpTarget = 1 << 9,

		// this instruction is dynamic jump target (indirect address)
		DynamicJumpTarget = 1 << 10,

		// this instruction is static call target
		StaticCallTarget = 1 << 11,

		// this instruction is dynamic call target
		DynamicCallTarget = 1 << 12,

		// this is the stack of instruction block
		BlockStart = 1 << 13,

		// this is the start of function (also a block)
		FunctionStart = 1 << 14,

		// this instruction has valid branch target
		HasBranchTarget = 1 << 15,

		// this is an import function stub
		ImportFunction = 1 << 16,

		// code is not directly executed (stub)
		Thunk = 1 << 17,

		// this code adderss was found in data sections
		ReferencedInData = 1 << 18,

		// this code references image data location (resolved)
		HasDataRef = 1 << 19,

		// this instuction requres the memory store/load to be mapped (via read/write function)
		MappedMemory = 1 << 20,

		// this is the entry point
		EntryPoint = 1 << 30,
	};

	//---------------------------------------------------------------------------

	// Decoding instruction flags
	class RECOMPILER_API InstructionFlags
	{
	public:
		// get instruction flags
		const inline bool IsValid() const { return 0 != (m_flags & (uint32)InstructionFlag::Valid); }
		const inline bool IsFirstByte() const { return 0 != (m_flags & (uint32)InstructionFlag::FirstByte); }
		const inline bool IsCall() const { return 0 != (m_flags & (uint32)InstructionFlag::Call); }
		const inline bool IsJump() const { return 0 != (m_flags & (uint32)InstructionFlag::Jump); }
		const inline bool IsConditional() const { return 0 != (m_flags & (uint32)InstructionFlag::Conditional); }
		const inline bool IsRet() const { return 0 != (m_flags & (uint32)InstructionFlag::Ret); }
		const inline bool IsMemory() const { return 0 != (m_flags & (uint32)InstructionFlag::Memory); }
		const inline bool IsIndirect() const { return 0 != (m_flags & (uint32)InstructionFlag::Indirect); }
		const inline bool IsPrivledged() const { return 0 != (m_flags & (uint32)InstructionFlag::Privledged); }
		const inline bool IsStaticJumpTarget() const { return 0 != (m_flags & (uint32)InstructionFlag::StaticJumpTarget); }
		const inline bool IsDynamicJumpTarget() const { return 0 != (m_flags & (uint32)InstructionFlag::DynamicJumpTarget); }
		const inline bool IsStaticCallTarget() const { return 0 != (m_flags & (uint32)InstructionFlag::StaticCallTarget); }
		const inline bool IsDynamicCallTarget() const { return 0 != (m_flags & (uint32)InstructionFlag::DynamicCallTarget); }
		const inline bool IsEntryPoint() const { return 0 != (m_flags & (uint32)InstructionFlag::EntryPoint); }
		const inline bool IsBlockStart() const { return 0 != (m_flags & (uint32)InstructionFlag::BlockStart); }
		const inline bool IsFunctionStart() const { return 0 != (m_flags & (uint32)InstructionFlag::FunctionStart); }
		const inline bool IsBranchTarget() const { return 0 != (m_flags & (uint32)InstructionFlag::HasBranchTarget); }
		const inline bool IsDataRef() const { return 0 != (m_flags & (uint32)InstructionFlag::HasDataRef); }
		const inline bool IsImportFunction() const { return 0 != (m_flags & (uint32)InstructionFlag::ImportFunction); }
		const inline bool IsThunk() const { return 0 != (m_flags & (uint32)InstructionFlag::Thunk); }
		const inline bool IsReferencedInData() const { return 0 != (m_flags & (uint32)InstructionFlag::ReferencedInData); }
		const inline bool IsMappedMemory() const { return 0 != (m_flags & (uint32)InstructionFlag::MappedMemory); }

	public:
		inline InstructionFlags()
			: m_flags(0)
		{};

		inline InstructionFlags(const uint32 flags)
			: m_flags(flags)
		{}

		inline InstructionFlags& SetFlag(const InstructionFlag flag)
		{
			m_flags |= (uint32)flag;
			return *this;
		}

		inline InstructionFlags& ClearFlag(const InstructionFlag flag)
		{
			m_flags &= ~(uint32)flag;
			return *this;
		}

	private:
		uint32		m_flags;
	};

	//---------------------------------------------------------------------------

	// Decoding data flags
	class RECOMPILER_API DataFlags
	{
	public:
		// get data flags
		const inline bool IsSigned() const { return 0 != (m_flags & (uint32)DataFlag::Signed); }
		const inline bool IsFloat() const { return 0 != (m_flags & (uint32)DataFlag::Float); }
		const inline bool IsCodePtr() const { return 0 != (m_flags & (uint32)DataFlag::CodePtr); }
		const inline bool IsDataPtr() const { return 0 != (m_flags & (uint32)DataFlag::DataPtr); }
		const inline bool IsIndirectCodePtr() const { return 0 != (m_flags & (uint32)DataFlag::IndirectCallPtr); }
		const inline bool IsIndirectJumpPtr() const { return 0 != (m_flags & (uint32)DataFlag::IndirectJumpPtr); }
		const inline bool IsCodeRef() const { return 0 != (m_flags & (uint32)DataFlag::HasCodeRef); }
		const inline bool IsDataRef() const { return 0 != (m_flags & (uint32)DataFlag::HasDataRef); }

	public:
		inline DataFlags()
			: m_flags(0)
		{};

		inline DataFlags(const uint32 flags)
			: m_flags(flags)
		{}

		inline DataFlags& SetFlag(const DataFlag flag)
		{
			m_flags |= (uint32)flag;
			return *this;
		}

		inline DataFlags& ClearFlag(const DataFlag flag)
		{
			m_flags &= ~(uint32)flag;
			return *this;
		}

	private:
		uint32		m_flags;
	};

	//---------------------------------------------------------------------------

	/// Decoding memory flag
	class RECOMPILER_API MemoryFlags
	{
	public:
		inline MemoryFlags(const uint64 rva, const uint64 size, const uint32 memoryFlags, const uint32 specialFlags)
			: m_rva(rva)
			, m_size(size)
			, m_memoryFlags(memoryFlags)
			, m_specialFlags(specialFlags)
		{}

		// get the memory location info
		const inline bool IsValid() const { return 0 != (m_memoryFlags & (uint32)MemoryFlag::Valid); }
		inline const bool IsFirstByte() const { return 0 != (m_memoryFlags & (uint32)MemoryFlag::FirstByte); }
		inline const bool IsReadOnly() const { return 0 != (m_memoryFlags & (uint32)MemoryFlag::ReadOnly); }
		inline const bool IsExecutable() const { return 0 != (m_memoryFlags & (uint32)MemoryFlag::Executable); }
		inline const bool IsGenericData() const { return 0 != (m_memoryFlags & (uint32)MemoryFlag::GenericData); }
		inline const bool IsReferencedData() const { return 0 != (m_memoryFlags & (uint32)MemoryFlag::ReferencedData); }
		inline const bool IsSectionStart() const { return 0 != (m_memoryFlags & (uint32)MemoryFlag::SectionStart); }
		inline const bool HasComment() const { return 0 != (m_memoryFlags & (uint32)MemoryFlag::HasComment); }
		inline const bool HasProblem() const { return 0 != (m_memoryFlags & (uint32)MemoryFlag::HasProblem); }
		inline const bool HasBreakpoint() const { return 0 != (m_memoryFlags & (uint32)MemoryFlag::HasBreakpoint); }
		inline const bool WasVisited() const { return 0 != (m_memoryFlags & (uint32)MemoryFlag::WasVisited); }

		// get data size
		inline const uint64 GetSize() const
		{
			return m_size;
		}

		// get the instruction only flags
		inline const InstructionFlags GetInstructionFlags() const
		{
			return InstructionFlags(m_specialFlags);
		}

		// get the data only falgs
		inline const DataFlags GetDataFlags() const
		{
			return DataFlags(m_specialFlags);
		}

	private:
		uint64		m_rva; // not saved here, just for reference
		uint64		m_size; // not saved here, just for reference
		uint32		m_memoryFlags;
		uint32		m_specialFlags;
	};

	//---------------------------------------------------------------------------

	/// Decoding memory map - map of the whole executable image
	class RECOMPILER_API MemoryMap
	{
	public:
		// Is this valid memory address ?
		inline const bool IsValidAddress(const uint64 addr) const { return (addr >= m_baseAddress) && (addr < (m_baseAddress + m_memorySize)); }

		// Is the data modified ?
		inline const bool IsModified() const { return m_isModified; }

	public:
		MemoryMap();
		~MemoryMap();

		// Save the memory map
		void Save(ILogOutput& log, IBinaryFileWriter& writer) const;

		// Load the memory map
		bool Load(ILogOutput& log, IBinaryFileReader& reader);

		// Initialize from image
		bool Initialize(ILogOutput& log, const image::Binary& baseImage);

		// validate the dirty ranges
		void Validate();

		// invalid custom dirty range
		void InvalidateRange(const uint64 rva, const uint64 size);

		// get dirty range, returns false if there's no dirty range
		bool GetDirtyRange(uint64& outFirstDirtyAddressRVA, uint64& outLastDirtyAddressRVA) const;

		// Get memory address description, uses the RVA addressing
		const MemoryFlags GetMemoryInfo(const uint64 rva) const;

		// Set length of block at given address
		bool SetMemoryBlockLength(ILogOutput& log, const uint64 rva, const uint64 size);

		// Set general block type of the block at specified location
		bool SetMemoryBlockType(ILogOutput& log, const uint64 rva, const uint32 setFlags, const uint32 clearFlags);

		// Set specialized block type of the block at specified location
		bool SetMemoryBlockSubType(ILogOutput& log, const uint64 rva, const uint32 setFlags, const uint32 clearFlags);

		//--

		// all the addresses with the block flag
		// changed when block flag is set/removed
		typedef std::unordered_set<uint64> TBlockRoots;
		inline const TBlockRoots& GetBlockRoots() const { return m_blockRoots; }

		// all the addresses with the function flag
		// changed when block flag is set/removed
		typedef std::unordered_set<uint64> TFunctionRoots;
		inline const TFunctionRoots& GetFunctionRoots() const { return m_functionRoots; }

	private:
		// descriptive entry
		struct Entry
		{
			uint32 m_flags : 16;
			uint32 m_size : 8;
			uint32 m_reserved : 8;
			uint32 m_specyficFlags;

			inline const bool HasFlag(const MemoryFlag flag) const
			{
				return 0 != (m_flags & (uint32)flag);
			}

			inline void SetFlag(const MemoryFlag flag)
			{
				m_flags |= (uint32)flag;
			}

			inline void ClearFlag(const MemoryFlag flag)
			{
				m_flags &= ~(uint32)flag;
			}
		};

		// base address
		uint64 m_baseAddress;

		// memory map (huge, sizeof the executable*8)
		Entry* m_memoryMap;
		uint32 m_memorySize;

		// dirty range accumulator (naive)
		uint64 m_firstDirtyAddress;
		uint64 m_lastDirtyAddress;

		// maps
		TBlockRoots m_blockRoots;
		TFunctionRoots m_functionRoots;

		// internal data was modified
		mutable bool m_isModified;
	};

} // decoding