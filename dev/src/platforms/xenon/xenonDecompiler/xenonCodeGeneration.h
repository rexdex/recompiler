#pragma once

//---------------------------------------------------------------------------

#include "../../../framework/backend/build.h"
#include "../../../framework/backend/decodingInstruction.h"
#include "../../../framework/backend/decodingInstructionInfo.h"

//---------------------------------------------------------------------------

class CodeGeneratorOptionsXenon
{
public:
	bool		m_allowBlockMerging;
	bool		m_allowLocalLabels;
	bool		m_allowCallInlinling;

	bool		m_forceMultiAddressBlocks;

	bool		m_debugTrace;

public:
	CodeGeneratorOptionsXenon( const bool allowDebugging );
	~CodeGeneratorOptionsXenon();
};

//---------------------------------------------------------------------------

class CCodeSegmentsXenon
{
public:
	struct BlockInfo
	{
		uint32			m_startAddrses;
		uint32			m_endAddress; // not included
	};

	typedef std::vector<BlockInfo> TBlocks;
	TBlocks				m_blocks;

	CCodeSegmentsXenon();
	~CCodeSegmentsXenon();

	const bool CreateSegments(ILogOutput& log, const decoding::Context& decodingContext, const CodeGeneratorOptionsXenon& options);
};

//---------------------------------------------------------------------------

class CodeGeneratorXenon
{
public:
	struct Instruction
	{
		uint32						m_address;
		uint32						m_targetAddress; // local jumps

		decoding::Instruction		m_op;		// original instruction
		decoding::InstructionExtendedInfo	m_info;		// extended instruction info

		uint32						m_flagLocalLabel:1;	// add a local label here
		uint32						m_flagMerged:1;		// we merged the code out of this instruction into some other instruction

		Instruction*				m_target;			// local link
		std::vector<Instruction*>	m_links;			// other instructions linking here

		std::string					m_rawCode;			// raw decompled code
		std::string					m_finalCode;		// final code to emit

		Instruction(const uint32 address, const decoding::Instruction& op);
		~Instruction();

		void Unlink();
		void LinkTo(Instruction* target);
	};

	struct Block
	{
		typedef std::vector<Instruction*>		TInstructions;

		TInstructions		m_instructions;
		uint32				m_multiAddress:1;
		uint32				m_functionStart:1;

		Block();
		~Block();
	};

	typedef std::vector<Block*>		TBlocks;
	TBlocks			m_blocks;

	bool			m_isInSwitch;

	const CodeGeneratorOptionsXenon*	m_options;
	const image::Binary*					m_image;
	const decoding::Context*				m_context;

public:
	CodeGeneratorXenon(const decoding::Context& context, const CodeGeneratorOptionsXenon& options);
	~CodeGeneratorXenon();

	// add block to code decoder
	const bool AddBlock(class ILogOutput& log, const uint32 start, const uint32 end, class code::Generator& codeGen);

	// should we try to merge the following block ?
	const bool CanGlue() const;

	// optimize code
	const bool Optimize(class ILogOutput& log);

	// emit code
	const bool Emit(class ILogOutput& log, class code::Generator& codeGen) const;
};

//---------------------------------------------------------------------------
