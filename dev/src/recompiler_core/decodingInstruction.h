#pragma once

//---------------------------------------------------------------------------

namespace platform
{
	class CPURegister;
	class CPUInstruction;
}

//---------------------------------------------------------------------------

namespace decoding
{
	class Context;
	class InstructionExtendedInfo;
	
	/// Generic decoded instruction
	class RECOMPILER_API Instruction
	{
	public:
		// regiser reference (use the CPU definition directly)
		typedef const platform::CPURegister* TReg;

		// opcode reference
		typedef const class platform::CPUInstruction* TOpcode;

		// operand type
		enum Type
		{
			eType_None,				// invalid (not defined)
			eType_Reg,				// register only, "r"
			eType_Imm,				// immediate only "i"
			eType_Mem,				// dereferenced register "[r] or [r+d+idx*scale]"
		};

		// instruction operand 
		struct Operand
		{
			Type		m_type;    // common
			uint8		m_scale;   // x86 only
			TReg		m_reg;     // common
			uint32		m_imm;     // immediate value, always SIGN EXTENDED if needed
			TReg		m_index;   // x86 only
			TReg		m_segment; // x86 only

			inline Operand()
				: m_type( eType_None )
				, m_reg( nullptr )
				, m_scale( 1 )
				, m_imm( 0 )
				, m_index( nullptr )
				, m_segment( nullptr )
			{}
		};

	protected:
		// size of the instruction (in bytes)
		uint8			m_codeSize;

		// size of used data in the instruction
		uint8			m_dataSize;

		// size of address calculation registers
		uint8			m_addrSize;

		// instruction opcode
		TOpcode			m_opcode;

		// operands (not all instructions use all)
		Operand			m_arg0;
		Operand			m_arg1;
		Operand			m_arg2;
		Operand			m_arg3;
		Operand			m_arg4;
		Operand			m_arg5;

	public:
		// is this instruction valid
		inline const bool IsValid() const { return (m_opcode != nullptr); }

		// instruction placement
		inline const uint8 GetCodeSize() const { return m_codeSize; }

		// data/address sizes
		inline const uint8 GetDataSize() const { return m_dataSize; }
		inline const uint8 GetAddrSize() const { return m_addrSize; }

		// instruction opcode
		inline TOpcode GetOpcode() const { return m_opcode; }

		// get operands (not all instructions use all)
		inline const Operand& GetArg0() const { return m_arg0; }
		inline const Operand& GetArg1() const { return m_arg1; }
		inline const Operand& GetArg2() const { return m_arg2; }
		inline const Operand& GetArg3() const { return m_arg3; }
		inline const Operand& GetArg4() const { return m_arg4; }
		inline const Operand& GetArg5() const { return m_arg5; }

		// get opcode name
		const char* GetName() const;

		// compare full instruction name
		inline bool operator==( const char* txt ) const
		{
			return (0 == strcmp( GetName(), txt ) );
		}

		// compare partial instruction
		template< uint32 N >
		inline bool MatchN( const char* txt ) const
		{
			return 0 == strncmp( GetName(), txt, N );
		}

		// compare partial instruction
		inline bool Match( const char* txt ) const
		{
			uint32 i = 0;
			const char* opName = GetName();
			while ( txt[i] != 0 && opName[i] != 0 )
			{
				bool match = (txt[i] == '?');
				match |= (opName[i] == txt[i]);
				match |= (txt[i] == '#') && (opName[i] == 'w' || opName[i] == 'b' || opName[i] == 'd' || opName[i] == 'h');
				if ( !match ) return false;

				i += 1;
			}

			return (txt[i] == 0) && (opName[i] == 0);
		}

		// match the operand types
		inline bool MatchOperands( const Instruction::Type dest ) const
		{
			if (m_arg0.m_type != dest) return false;
			if (m_arg1.m_type != Instruction::eType_None) return false;
			if (m_arg2.m_type != Instruction::eType_None) return false;
			if (m_arg3.m_type != Instruction::eType_None) return false;
			if (m_arg4.m_type != Instruction::eType_None) return false;
			if (m_arg5.m_type != Instruction::eType_None) return false;
			return true;
		}

		// match the operand types
		inline bool MatchOperands( const Instruction::Type dest, const Instruction::Type src0 ) const
		{
			if (m_arg0.m_type != dest) return false;
			if (m_arg1.m_type != src0) return false;
			if (m_arg2.m_type != Instruction::eType_None) return false;
			if (m_arg3.m_type != Instruction::eType_None) return false;
			if (m_arg4.m_type != Instruction::eType_None) return false;
			if (m_arg5.m_type != Instruction::eType_None) return false;
			return true;
		}

		// match the operand types
		inline bool MatchOperands( const Instruction::Type dest, const Instruction::Type src0, const Instruction::Type src1 ) const
		{
			if (m_arg0.m_type != dest) return false;
			if (m_arg1.m_type != src0) return false;
			if (m_arg2.m_type != src1) return false;
			if (m_arg3.m_type != Instruction::eType_None) return false;
			if (m_arg4.m_type != Instruction::eType_None) return false;
			if (m_arg5.m_type != Instruction::eType_None) return false;
			return true;
		}

		// match the operand types
		inline bool MatchOperands( const Instruction::Type dest, const Instruction::Type src0, const Instruction::Type src1, const Instruction::Type src2 ) const
		{
			if (m_arg0.m_type != dest) return false;
			if (m_arg1.m_type != src0) return false;
			if (m_arg2.m_type != src1) return false;
			if (m_arg3.m_type != src2) return false;
			if (m_arg4.m_type != Instruction::eType_None) return false;
			if (m_arg5.m_type != Instruction::eType_None) return false;
			return true;
		}

		// match the operand types
		inline bool MatchOperands( const Instruction::Type dest, const Instruction::Type src0, const Instruction::Type src1, const Instruction::Type src2, const Instruction::Type src3 ) const
		{
			if (m_arg0.m_type != dest) return false;
			if (m_arg1.m_type != src0) return false;
			if (m_arg2.m_type != src1) return false;
			if (m_arg3.m_type != src2) return false;
			if (m_arg4.m_type != src3) return false;
			if (m_arg5.m_type != Instruction::eType_None) return false;
			return true;
		}

		// char indexed
		inline char operator[]( const int index ) const
		{
			return GetName()[index];
		}

	public:
		Instruction(); // creates invalid instruction
		~Instruction();

		// setup instruction from parts
		void Setup(const uint32 codeSize, const platform::CPUInstruction* opcode, const Operand& arg0 = Operand(), const Operand& arg1 = Operand(), const Operand& arg2 = Operand(), const Operand& arg3 = Operand(), const Operand& arg4 = Operand(), const Operand& arg5 = Operand());

		// get simple text representation (fast implementation)
		void GenerateSimpleText(const uint64_t codeAddress, char*& outStream, const char* maxPos) const;

		// get full text representation (requires instruction decoder)
		void GenerateText(const uint64_t codeAddress, char*& outStream, const char* maxPos) const;

		// generate comment text
		const bool GenerateComment(const uint64_t codeAddress, char*& outStream, const char* maxPos) const;

		// get extra instruction information 
		const bool GetExtendedInfo(const uint64_t codeAddress, decoding::Context& context, InstructionExtendedInfo& outInfo) const;

	private:
		// get text representation (fast implementation)
		static bool GenerateOperandText(const Operand& op, const bool validFlag, char*& outStream, const char* maxPos);

		// string concatenation
		static void ConcatPrintf(char*& outStream, const char* maxPos, const char* txt, ...);
		static void ConcatPrint(char*& outStream, const char* maxPos, const char* txt);

		// check operand for the need of data/addrses flags
		static void CheckOperandFlags(const Operand& op, bool& outDataFlagsNeeded, bool& outAddressFlagsNeeded);
	};

	//---------------------------------------------------------------------------

} // decoding