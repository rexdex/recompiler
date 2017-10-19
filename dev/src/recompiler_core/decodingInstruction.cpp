#include "build.h"
#include "internalUtils.h"
#include "internalParser.inl"
#include "platformDefinition.h"
#include "platformCPU.h"
#include "decodingInstruction.h"
#include "decodingInstructionInfo.h"

namespace decoding
{

	Instruction::Instruction()
		: m_codeSize( 0 )
		, m_dataSize( 32 )
		, m_addrSize( 32 )
		, m_opcode( nullptr )
	{
	}

	Instruction::~Instruction()
	{
	}

	const char* Instruction::GetName() const
	{
		return m_opcode ? m_opcode->GetName() : "NULL";
	}

	void Instruction::ConcatPrint( char*& outStream, const char* maxPos, const char* txt )
	{
		const char* curStream = txt;
		while ( outStream < maxPos && *curStream )
		{
			*outStream++ = *curStream++;
		}
	}

	void Instruction::ConcatPrintf( char*& outStream, const char* maxPos, const char* txt, ... )
	{
		char buf[128];
		va_list args;

		va_start( args, txt );
		vsprintf_s( buf, sizeof(buf), txt, args );
		va_end( args );

		const char* curStream = buf;
		while ( outStream < maxPos && *curStream )
		{
			*outStream++ = *curStream++;
		}
	}

	bool Instruction::GenerateOperandText( const Operand& op, const bool validFlag, char*& outStream, const char* maxPos )
	{
		// this operand is invalid
		if ( op.m_type == eType_None )
		{
			return validFlag;
		}

		// append the separator from the previous valid operand
		if ( validFlag )
		{
			ConcatPrint( outStream, maxPos, ", " );
		}

		// append shit
		switch ( op.m_type )
		{
			case eType_Imm:
			{
				ConcatPrintf( outStream, maxPos, "%d", op.m_imm );
				break;
			}

			case eType_Reg:
			{
				ConcatPrint( outStream, maxPos, op.m_reg->GetName() );
				break;
			}

			case eType_Mem:
			{
				const int imm = (int&)op.m_imm;

				ConcatPrintf( outStream, maxPos, "<#[");

				bool space = false;

				if ( op.m_reg )
				{
					ConcatPrintf( outStream, maxPos, "%s", op.m_reg->GetName() );
					space = true;
				}

				if ( op.m_index )
				{
					if ( op.m_scale > 1 )
					{
						ConcatPrintf( outStream, maxPos, "%s%s*%s", 
							space ? " + " : "", 
							op.m_index->GetName(), 
							op.m_scale );
					}
					else
					{
						ConcatPrintf( outStream, maxPos, "%s%s", 
							space ? " + " : "", 
							op.m_index->GetName() );
					}

					space = true;
				}

				if ( op.m_imm != 0 )
				{
					ConcatPrintf( outStream, maxPos, "%s%d", 
						space 
							? ((imm < 0) ? " - " : " + ")
							: ((imm < 0) ? "-" : ""),
						abs(imm) );
				}

				ConcatPrintf( outStream, maxPos, "]>");
				break;
			}

			default:
			{
				ConcatPrint( outStream, maxPos, "<invalid>" );
				break;
			}
		}

		// valid from now on
		return true;
	}

	void Instruction::CheckOperandFlags( const Operand& op, bool& outDataFlagsNeeded, bool& outAddressFlagsNeeded )
	{
		if ( op.m_type == eType_Mem )
		{
			outAddressFlagsNeeded = true;
			outDataFlagsNeeded = true;
		}
	}

	void Instruction::GenerateSimpleText(const uint64_t codeAddress, char*& outStream, const char* maxPos) const
	{
		// invalid instruction
		if ( !m_opcode )
		{
			ConcatPrint( outStream, maxPos, "<invalid>" );
			*outStream = 0;
			return;
		}

		// do we require data size flags ? (only if we are doing memory access)
		// do we require addres size flags ? (only if we are doing address computation on x86)
		bool needsDataFlags = false;
		bool needsAddressFlags = false;

		// start with instruction name
		ConcatPrint( outStream, maxPos, m_opcode->GetName() );

		// separator
		if ( m_arg0.m_type != eType_None )
		{
			ConcatPrint( outStream, maxPos, " " );
		}
		
		// append operands
		bool validFlag = false;
		validFlag = GenerateOperandText( m_arg0, validFlag, outStream, maxPos );
		validFlag = GenerateOperandText( m_arg1, validFlag, outStream, maxPos );
		validFlag = GenerateOperandText( m_arg2, validFlag, outStream, maxPos );
		validFlag = GenerateOperandText( m_arg3, validFlag, outStream, maxPos );
		validFlag = GenerateOperandText( m_arg4, validFlag, outStream, maxPos );
		validFlag = GenerateOperandText( m_arg5, validFlag, outStream, maxPos );

		// end
		*outStream = 0;
	}

	const bool Instruction::GenerateComment(const uint64_t codeAddress, char*& outStream, const char* maxPos) const
	{
		// invalid instruction
		if ( !m_opcode )
			return false;

		// generate native description
		const platform::CPUInstructionNativeDecompiler* dc = m_opcode->GetDecompiler();
		if (dc && dc->GetCommentText(*this, codeAddress, outStream, (uint32)(maxPos - outStream)))
			return true;

		// not generated
		return false;
	}

	void Instruction::GenerateText(const uint64_t codeAddress, char*& outStream, const char* maxPos) const
	{
		// invalid instruction
		if ( !m_opcode )
		{
			ConcatPrint( outStream, maxPos, "<invalid>" );
			*outStream = 0;
			return;
		}

		// generate native description
		const platform::CPUInstructionNativeDecompiler* dc = m_opcode->GetDecompiler();
		if (dc && dc->GetExtendedText(*this, codeAddress, outStream, (uint32)(maxPos - outStream)))
			return;

		// fallback
		GenerateSimpleText(codeAddress, outStream, maxPos);
	}

	const bool Instruction::GetExtendedInfo(const uint64_t codeAddress, class decoding::Context& context, InstructionExtendedInfo& outInfo) const
	{
		// invalid instruction
		if ( !m_opcode )
			return false;

		// use the decoder
		const platform::CPUInstructionNativeDecompiler* dc = m_opcode->GetDecompiler();
		if (dc && dc->GetExtendedInfo(*this, codeAddress, context, outInfo))
			return true;

		// nothing generated
		return false;
	}

	void Instruction::Setup(const uint32 codeSize, const platform::CPUInstruction* opcode, const Operand& arg0/*= Operand()*/, const Operand& arg1 /*= Operand()*/, const Operand& arg2 /*= Operand()*/, const Operand& arg3 /*= Operand()*/, const Operand& arg4 /*= Operand()*/, const Operand& arg5 /*= Operand()*/)
	{
		m_codeSize = codeSize;
		m_opcode = opcode;
		m_arg0 = arg0;
		m_arg1 = arg1;
		m_arg2 = arg2;
		m_arg3 = arg3;
		m_arg4 = arg4;
		m_arg5 = arg5;
	}

} // decoding