#include "build.h"
#include "internalParser.inl"
#include "internalUtils.h"
#include "platformCPU.h"
#include "decodingInstruction.h"

namespace platform
{

	//---------------------------------------------------------------------------

	CPURegister::CPURegister(const char* name, const uint32 bitSize, const uint32 bitOffset, const EInstructionRegisterType itype, const CPURegister* parent, const int nativeIndex)
		: m_bitSize( bitSize )
		, m_bitOffset( bitOffset )
		, m_type( itype )
		, m_parentRegister( parent )
		, m_name( name )
		, m_nativeIndex( nativeIndex )
		, m_traceIndex(-1)
		, m_traceDataOffset(-1)
	{
		if ( nullptr != parent )
		{
			m_childIndex = (int)const_cast<CPURegister*>(parent)->m_children.size();
			const_cast< CPURegister* >( parent )->m_children.push_back( this );
		}
	}

	CPURegister::~CPURegister()
	{
	}

	void CPURegister::BindToTrace(int traceIndex, int traceDataOffset) const
	{
		m_traceIndex = traceIndex;
		m_traceDataOffset = traceDataOffset;
	}

	void CPURegister::Collect(std::vector<const CPURegister* >& outArray) const
	{
		outArray.push_back(this);

		for (const auto* child : m_children)
			child->Collect(outArray);
	}

	//---------------------------------------------------------------------------

	CPUInstruction::CPUInstruction(const char* name, const class CPUInstructionNativeDecompiler* decompiler)
		: m_name(name)
		, m_decompiler(decompiler)
	{}

	//---------------------------------------------------------------------------

	CPU::CPU(const char* name, const CPUInstructionNativeDecoder* decoder)
		: m_name( name )
		, m_nativeDecoder( decoder )
	{
	}

	CPU::~CPU()
	{
		// cleanup registers
		DeleteVector(m_registers);
		DeleteVector(m_instructions);
	}

	const CPURegister* CPU::FindRegister( const char* name ) const
	{
		TRegisterMap::const_iterator it = m_registerMap.find( name );
		if ( it != m_registerMap.end() )
			return it->second;

		return nullptr;
	}

	const CPUInstruction* CPU::FindInstruction( const char* name ) const
	{
		TInstructionMap::const_iterator it = m_instructionMap.find( name );
		if ( it != m_instructionMap.end() )
			return it->second;

		return nullptr;
	}

	static const unsigned char BitReverseTable256[] = 
	{
	  0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0, 
	  0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8, 
	  0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4, 
	  0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC, 
	  0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2, 
	  0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
	  0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6, 
	  0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE, 0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
	  0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
	  0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9, 
	  0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
	  0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
	  0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3, 
	  0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
	  0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7, 
	  0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
	};

	inline uint32 FullReverse32( const uint32 v )
	{
		uint32 ret;
		const uint8* p = (const uint8*)&v;
		uint8* q = (uint8*)&ret;
		q[3] = BitReverseTable256[p[0]]; 
		q[2] = BitReverseTable256[p[1]]; 
		q[1] = BitReverseTable256[p[2]]; 
		q[0] = BitReverseTable256[p[3]];
		return ret;
	}

	inline uint32 PerByteBitReverse32( const uint32 v )
	{
		uint32 ret;
		const uint8* p = (const uint8*)&v;
		uint8* q = (uint8*)&ret;
		q[0] = BitReverseTable256[p[0]]; 
		q[1] = BitReverseTable256[p[1]]; 
		q[2] = BitReverseTable256[p[2]]; 
		q[3] = BitReverseTable256[p[3]];
		return ret;
	}

	inline uint32 EndianReverse32( const uint32 v )
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

	inline uint32 FullReverse16( const uint16 v )
	{
		uint16 ret;
		const uint8* p = (const uint8*)&v;
		uint8* q = (uint8*)&ret;
		q[1] = BitReverseTable256[p[0]]; 
		q[0] = BitReverseTable256[p[1]];
		return ret;
	}

	const uint32 CPU::DecodeInstructionText(class ILogOutput& log, const uint8* stream, std::string& outText) const
	{
		return 0;
	}

	const uint32 CPU::ValidateInstruction(class ILogOutput& log, const uint8* stream) const
	{
		if ( !m_nativeDecoder )
			return 0;

		return m_nativeDecoder->ValidateInstruction(stream);
	}

	const uint32 CPU::DecodeInstruction(class ILogOutput& log, const uint8* stream, decoding::Instruction& outInstruction) const
	{
		if ( !m_nativeDecoder )
			return 0;

		return m_nativeDecoder->DecodeInstruction(stream, outInstruction);
	}

	const CPUInstruction* CPU::AddInstruction(const char* name, const class CPUInstructionNativeDecompiler* decompiler)
	{
		// invalid name
		if (!name || !name[0])
			return NULL;

		// instruction already exists ?
		if (FindInstruction(name))
			return NULL;

		// create instruction
		CPUInstruction* instr = new CPUInstruction(name, decompiler);
		m_instructions.push_back(instr);
		m_instructionMap[name] = instr;
		return instr;
	}

	const CPURegister* CPU::AddRootRegister(const char* name, const int nativeIndex, const uint32 bitSize, const EInstructionRegisterType type)
	{
		// invalid name
		if (!name || !name[0])
			return NULL;

		// register already exists ?
		if (FindRegister(name))
			return NULL;

		// create register
		CPURegister* regInfo = new CPURegister(name, bitSize, 0, type, nullptr, nativeIndex);
		m_registers.push_back(regInfo);
		m_rootRegisters.push_back(regInfo);
		m_registerMap[name] = regInfo;
		return regInfo;
	}

	const CPURegister* CPU::AddChildRegister(const char* parentName, const char* name, const int nativeIndex, const uint32 bitSize, const uint32 bitOffset, const EInstructionRegisterType type)
	{
		// invalid name
		if (!name || !name[0])
			return NULL;

		// register already exists ?
		if (FindRegister(name))
			return NULL;

		// find parent register
		const CPURegister* parentReg = FindRegister(parentName);
		if (!parentReg)
			return NULL;

		// create register
		CPURegister* regInfo = new CPURegister(name, bitSize, bitOffset, type, parentReg, nativeIndex);
		m_registers.push_back(regInfo);
		m_registerMap[name] = regInfo;
		return regInfo;
	}

} // platform