#pragma once

namespace trace
{
	class DataFrame;
	class Registers;
	class PagedFileReader;
}

namespace decoding
{
	class Context;
	class Instruction;
	class InstructionExtendedInfo;
}

namespace platform
{

	//---------------------------------------------------------------------------

	/// Native instruction decoder (faster than script)
	class RECOMPILER_API CPUInstructionNativeDecoder
	{
	public:
		virtual ~CPUInstructionNativeDecoder() {};

		// validate instruction from code stream, returns instruction size in bytes or 0 if instruction is not valid
		virtual uint32 ValidateInstruction(const uint8* inputStream) const = 0;

		// decode instruction from code stream, returns instruction size in bytes or 0 if instruction is not valid
		virtual uint32 DecodeInstruction(const uint8* inputStream, class decoding::Instruction& outDecodedInstruction) const = 0;
	};

	/// Runtime CPU instruction decompiler
	class RECOMPILER_API CPUInstructionNativeDecompiler
	{
	public:
		virtual ~CPUInstructionNativeDecompiler() {};

		//! Generate human readable instruction comment
		virtual bool GetCommentText(const class decoding::Instruction& instr, const uint64_t codeAddress, char* outText, const uint32 outTextSize) const = 0;

		//! Generate human readable instruction text
		virtual bool GetExtendedText(const class decoding::Instruction& instr, const uint64_t codeAddress, char* outText, const uint32 outTextSize) const = 0;

		//! Generate extended info for instruction
		virtual bool GetExtendedInfo(const class decoding::Instruction& instr, const uint64_t codeAddress, const decoding::Context& context, decoding::InstructionExtendedInfo& outInfo) const = 0;
	};

	//---------------------------------------------------------------------------

	/// Type of register value
	enum class EInstructionRegisterType : char
	{
		Flags = 0, // register contains bits that are interpreted as flags
		Integer = 1, // register contains numerical value
		FloatingPoint = 2, // register contains floating point value 
		Wide = 3, // multi component SSE/MMX type registers
	};

	/// Register referenced by instruction (global object shared by instructions)
	class RECOMPILER_API CPURegister
	{
	private:
		// register type
		EInstructionRegisterType		m_type;

		// register size (in bits)
		uint32							m_bitSize;

		// register offset (in bits) in the parent register
		uint32							m_bitOffset;

		// parent register (e.g eax for ax)
		const CPURegister*				m_parentRegister;
		int32							m_childIndex; // index in parent, -1 if has no parent

		// register descriptive name
		std::string						m_name;

		// first level children registers (e.g. ax for eax, ah and al for ax)
		typedef std::vector< const CPURegister* > TChildRegisters;
		TChildRegisters					m_children;

		// native index
		int								m_nativeIndex;

		// trace support
		mutable int						m_traceIndex; // index in the trace register mapping
		mutable int						m_traceDataOffset; // offset in the trace frame to the data for this register, NOTE: only parent registers are in trace files

	public:
		CPURegister(const char* name, const uint32 bitSize, const uint32 bitOffset, const EInstructionRegisterType itype, const CPURegister* parent, const int nativeIndex);
		virtual ~CPURegister();

		inline const char* GetName() const { return m_name.c_str(); }
		inline const uint32 GetBitSize() const { return m_bitSize; }
		inline const uint32 GetBitOffset() const { return m_bitOffset; }
		inline const int GetNativeIndex() const { return m_nativeIndex; }
		inline const EInstructionRegisterType GetType() const { return m_type; }

		inline const CPURegister* GetParent() const { return m_parentRegister; }
		inline const int GetChildIndex() const { return m_childIndex;  }

		inline const uint32 GetNumChildRegisters() const { return (uint32)m_children.size(); }
		inline const CPURegister* GetChildRegister( const size_t index ) const { return m_children[ index ]; }

		inline const int GetTraceIndex() const { return m_traceIndex;  }
		inline const int GetTraceDataOffset() const { return m_traceDataOffset; }

		// bind register to the trace system
		void BindToTrace(int traceIndex, int traceDataOffset) const;

		// collect register and sub registers
		void Collect(std::vector<const CPURegister* >& outArray) const;
	};

	//---------------------------------------------------------------------------

	/// instruction base definition
	class RECOMPILER_API CPUInstruction
	{
	private:
		// instruction name
		std::string										m_name;

		// runtime user data block (for speeding up decompilation)
		const CPUInstructionNativeDecompiler*	m_decompiler;

	public:
		CPUInstruction(const char* name, const class CPUInstructionNativeDecompiler* decompiler);

		inline const char* GetName() const { return m_name.c_str(); }
		inline const CPUInstructionNativeDecompiler* GetDecompiler() const { return m_decompiler; }
	};

	//---------------------------------------------------------------------------

	/// instruction CPU definition
	class RECOMPILER_API CPU
	{
	public:
		inline const std::string& GetName() const { return m_name; }

		inline const uint32 GetNumRegisters() const { return (uint32)m_registers.size(); }
		inline const CPURegister* GetRegister( const uint32 index ) const { return m_registers[ index ]; }

		inline const uint32 GetNumRootRegisters() const { return (uint32)m_rootRegisters.size(); }
		inline const CPURegister* GetRootRegister(const uint32 index) const { return m_rootRegisters[index]; }

		inline const uint32 GetNumInstructions() const { return (uint32)m_instructions.size(); }
		inline const CPUInstruction* GetInstruction( const uint32 index ) const { return m_instructions[ index ]; }

	public:
		CPU(const char* name, const CPUInstructionNativeDecoder* decoder);
		virtual ~CPU();

		// find register by the instruction set and
		const CPURegister* FindRegister(const char* name) const;

		// find instruction definition by it's internal name
		const CPUInstruction* FindInstruction(const char* name) const;

		// validate CPU instruction using provided input stream, returns the size of the instruction in bytes or 0 if the decoding failed
		const uint32 ValidateInstruction(class ILogOutput& log, const uint8* stream) const;

		// decode CPU instruction using provided input stream, returns NULL if the decoding failed, returns empty instruction if failed
		const uint32 DecodeInstruction(class ILogOutput& log, const uint8* stream, decoding::Instruction& outInstruction) const;

		// add instruction to CPU
		const CPUInstruction* AddInstruction(const char* name, const class CPUInstructionNativeDecompiler* decompiler);

		// add root register to CPU
		const CPURegister* AddRootRegister(const char* name, const int nativeIndex, const uint32 bitSize, const EInstructionRegisterType type);

		// add child register to CPU
		const CPURegister* AddChildRegister(const char* parentName, const char* name, const int nativeIndex, const uint32 bitSize, const uint32 bitOffset, const EInstructionRegisterType type);

	private:
		// fallback: decode CPU instruction using provided input stream, retruns decoded string (or empty string if nothing found)
		const uint32 DecodeInstructionText(class ILogOutput& log, const uint8* stream, std::string& outText) const;

		// cpu name
		std::string						m_name;

		// registers used by CPU
		typedef std::vector< CPURegister* > TRegisters;
		typedef std::map< std::string, CPURegister* > TRegisterMap;
		TRegisters						m_registers;
		TRegisterMap					m_registerMap;

		// root registers
		TRegisters						m_rootRegisters;

		// instruction sets used by CPU
		typedef std::vector< CPUInstruction* > TInstructions;
		typedef std::map< std::string, CPUInstruction* > TInstructionMap;
		TInstructions					m_instructions;
		TInstructionMap					m_instructionMap;

		// native decoders
		typedef const CPUInstructionNativeDecoder* TNativeDecoder;
		TNativeDecoder					m_nativeDecoder;
	};

} // platform
