// XOpPrint.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <string>
#include <vector>
#include <map>

typedef unsigned int uint32;

class BitMask
{
public:
	uint32		m_bits;
	uint32		m_mask;

public:
	BitMask()
		: m_mask(0)
		, m_bits(0)
	{}

	BitMask(const uint32 opcode)
		: m_mask(0xFFFFFFFF)
		, m_bits(opcode)
	{}

	void Set(const uint32 value)
	{
		m_bits = value;
		m_mask = 0xFFFFFFFF;
	}

	void Add(const uint32 value)
	{
		const uint32 diff = m_bits ^ value;
		m_mask &= ~diff; // mask off different bits
	}

	const bool IsBitValid(const int index) const
	{
		const uint32 mask = 1 << index;
		return 0 != (m_mask & mask);
	}

	void Dump(FILE* f) const
	{
		// general bit pattern
		fprintf(f," Mask: '");
		int j=0;
		for (int i=31; i>=0; --i, ++j)
		//for (int i=0; i<=31; ++i)
		{
			if (j && ((j&7) == 0))
				fprintf(f, " " );

			const uint32 mask = 1 << i;
			const char* ch = (m_mask & mask) ? ((m_bits & mask) ? "1" : "0") : "*";
			fprintf(f,ch);
		}
		fprintf(f,"'\n");

		// find fields
		int numFields = 0;
		for (int i=0; i<31; ++i)
		{
			if (!IsBitValid(i))
				continue;

			int end=i;
			while (end <= 31)
			{
				if (!IsBitValid(end))
					break;
				++end;
			}


			const uint32 maskedVal = (m_bits & m_mask);
			const int len = (end - i);
			const int val = (maskedVal >> i) & ((1<<len)-1);

			fprintf(f, "  Field[%d]: %d:%d (%d)\n",
				numFields++, 32-end, end-i, val );

			i += len;
		}
	}

	void DumpMasked(FILE* f, const BitMask& parentMask) const
	{
		// general bit pattern
		fprintf(f," ");
		int j=0;
		for (int i=31; i>=0; --i, ++j)
		//for (int i=0; i<=31; ++i)
		{
			if (j && ((j&7) == 0))
				fprintf(f, " " );

			const uint32 mask = 1 << i;
			const char* ch = 
				(parentMask.m_mask & mask) 
					? "-" 
					: (m_mask & mask) 
						? ((m_bits & mask) ? "1" : "0") 
						: "*";
			fprintf(f,ch);			
		}
		fprintf(f,"\n");
	}
};

struct Variant
{
	std::string		m_name;
	BitMask			m_mask;

	Variant(const std::string& name, const uint32 opcode)
		: m_name(name)
		, m_mask(opcode)
	{
	}

	void Update(const uint32 opcode)
	{
		m_mask.Add(opcode);
	}

	void Dump(FILE* f, const BitMask& op) const
	{
		fprintf(f, "        Variant %6s: ", m_name.c_str());
		m_mask.DumpMasked(f, op);
	}
};

struct VariantMap
{
	typedef std::map<std::string, Variant*>		TVariantMap;
	TVariantMap		m_map;

	void Add(const std::string& name, uint32 opcode)
	{
		TVariantMap::const_iterator it = m_map.find(name);
		if (it != m_map.end())
		{
			it->second->Update(opcode);
			return;
		}

		Variant* v = new Variant(name, opcode);
		m_map[ name ] = v;
	}

	void Dump(FILE* f, const BitMask& parentMask) const
	{		
		// print the valid mask
		{
			uint32 validMask = 0xFFFFFFFF;

			for (TVariantMap::const_iterator it = m_map.begin();
				it != m_map.end(); ++it)
			{
				validMask &= it->second->m_mask.m_mask; // only valid values
			}

			fprintf(f,"        Mask:            ");
			int j=0;
			for (int i=31; i>=0; --i, ++j)
			//for (int i=0; i<=31; ++i)
			{
				if (j && ((j&7) == 0))
					fprintf(f, " " );

				const uint32 mask = 1 << i;
				const char* ch = 
					((validMask & ~parentMask.m_mask) & mask) 
						? "*" : "-";
				fprintf(f,ch);			
			}
			fprintf(f,"\n");
		}

		for (TVariantMap::const_iterator it = m_map.begin();
			it != m_map.end(); ++it)
		{
			it->second->Dump(f, parentMask);
		}
	}
};

struct InstructionData
{
	std::string					m_name;
	std::vector<std::string>	m_params;
	uint32						m_op;
};

struct Instruction
{
	std::string		m_name;
	BitMask			m_op;

	typedef std::vector<VariantMap*>	VariantList;
	VariantList		m_variants;

	Instruction(const InstructionData& data)
		: m_name(data.m_name)
		, m_op(data.m_op)
	{
		m_variants.resize(data.m_params.size());
		for ( uint32 i=0; i<m_variants.size(); ++i)
		{
			m_variants[i] = new VariantMap();
			m_variants[i]->Add( data.m_params[i], data.m_op );
		}
	}

	bool Update(const InstructionData& data)
	{
		if (data.m_params.size() != m_variants.size())
		{
			fprintf( stderr, "Instruction '%s' parameter count mismatch (%d!=%d) !!\n", 
				data.m_name.c_str(), m_variants.size(), data.m_params.size() );
			return false;
		}

		m_op.Add(data.m_op);

		for ( uint32 i=0; i<m_variants.size(); ++i)
		{
			m_variants[i]->Add( data.m_params[i], data.m_op );
		}
	}

	void Dump(FILE* f)
	{
		fprintf(f, "Instruction '%s' (%d params)\n", m_name.c_str(), m_variants.size());
		m_op.Dump(f);

		for (uint32 i=0; i<m_variants.size(); ++i)
		{
			fprintf(f, "    Variants for param %d (%d):\n", i, m_variants[i]->m_map.size());
			m_variants[i]->Dump(f, m_op);
		}
	}
};

struct InstructionMap
{
	typedef		std::map<std::string, Instruction*>		TOpMap;
	TOpMap		m_ops;

	bool Add(const InstructionData& data)
	{
		TOpMap::const_iterator it = m_ops.find(data.m_name);
		if (it != m_ops.end())
			return it->second->Update(data);

		Instruction* instr = new Instruction(data);
		m_ops[data.m_name] = instr;
		return true;
	}

	void Dump(FILE *f)
	{
		fprintf( stdout, "Found %d unique instructions\n", m_ops.size() );

		for (TOpMap::const_iterator it = m_ops.begin();
			it != m_ops.end(); ++it)
		{
			it->second->Dump(f);
		}
	}
};

InstructionMap GInstructionMap;

void SkipWord(char*& txt)
{
	while (*txt && *txt <= ' ')
		++txt;

	while (*txt && *txt > ' ')
		++txt;
}

bool ParseHex(char*& txt, uint32& out)
{
	while (*txt && *txt <= ' ')
		++txt;

	uint32 ret = 0;

	const char* start = txt;
	while (*txt && *txt > ' ')
	{
		int num = 0;
		if ( *txt >= '0' && *txt <= '9' ) num = *txt - '0';
		else if ( *txt == 'a' ) num = 10;
		else if ( *txt == 'b' ) num = 11;
		else if ( *txt == 'c' ) num = 12;
		else if ( *txt == 'd' ) num = 13;
		else if ( *txt == 'e' ) num = 14;
		else if ( *txt == 'f' ) num = 15;
		else return false;

		ret = ret * 16;
		ret += num;
		++txt;
	}

	if (start == txt)
		return false;

	out = ret;
	return true;
}

bool IsSeparatorChar( const char ch )
{
	if (ch <= ' ') return true;
	if (ch == ',') return true;
	return false;
}

bool ParsePart(char*& txt, std::string& out)
{
	while (*txt && *txt <= ' ')
		++txt;

	if ( *txt == ';' )
		return false;

	out = "";
	std::string ret;
	while (*txt && !IsSeparatorChar(*txt))
	{
		char str[2] = {*txt, 0};
		ret += str;
		++txt;
	}

	if (*txt && IsSeparatorChar(*txt))
		++txt;

	if (ret.empty())
		return false;

	out = ret;
	return true;
}

int _tmain(int argc, _TCHAR* argv[])
{
	FILE* f = fopen("..\\xgen\\Release_LTCG\\main.cod", "r");
	if (!f)
		return -1;

	while (!feof(f))
	{
		char buf[2048];
		fgets(buf, 2048, f);

		char* line = buf;
		while (*line && *line <= ' ')
			++line;

		if (*line == ';')
			continue;

		// scan address and opcode
		uint32 address=0, opcode=0;
		if (!ParseHex(line, address))
			continue;
		if (!ParseHex(line, opcode))
			continue;

		// invert endianess
		//opcode = _byteswap_ulong(opcode);

		// stream
		char* stream = line;

		InstructionData opData;
		opData.m_op = opcode;

		// parse instruction name
		if (!ParsePart(stream, opData.m_name))
			continue;

		// parse additional data
		std::string param;
		while (ParsePart(stream, param))
		{
			opData.m_params.push_back(param);
		}

		// find instruction in map
		if (!GInstructionMap.Add(opData))
			break;
	}

	// done parsing
	fclose(f);

	// dump the master opcode map 
	FILE* fout = fopen("dump.txt", "w");
	if (!fout) fout = stdout;
	GInstructionMap.Dump( fout );
	if (fout != stdout) fclose(fout);

	// done
	return 0;
}

