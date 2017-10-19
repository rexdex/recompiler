#include "build.h"
#include "runtimeCodeTable.h"
#include "runtimeRegisterBank.h"

namespace runtime
{

	CodeTable::CodeTable(const uint64 startCodeAddress, const uint64 endCodeAddress)
		: m_startCodeAddress(startCodeAddress)
		, m_endCodeAddress(endCodeAddress)
	{
		// create code table
		m_codeTableLength = (uint32)(endCodeAddress - startCodeAddress);
		m_codeTable = (runtime::TBlockFunc*) malloc(sizeof(runtime::TBlockFunc) * m_codeTableLength);

		// all addreses are invalid until mounted with code
		for (uint32 i = 0; i < m_codeTableLength; ++i)
			m_codeTable[i] = &InvalidCodeBlock;
	}

	CodeTable::~CodeTable()
	{
		free(m_codeTable);
		m_codeTable = NULL;
	}

	bool CodeTable::MountBlock(const uint64 startCodeAddress, const uint32 length, TBlockFunc func, const bool forceOverride /*= false*/)
	{
		// invalid code range :(
		if (startCodeAddress < m_startCodeAddress || (startCodeAddress + length > m_endCodeAddress))
		{
			GLog.Err("Code: Code mount range 0x%08X-0x%08X lies outside image code range 0x%08X-0x%08X",
				startCodeAddress, startCodeAddress + length,
				m_startCodeAddress, m_endCodeAddress);

			return false;
		}

		// empty range
		const uint32 numTableEntries = length;
		if (!numTableEntries)
		{
			GLog.Err("Code: Code mount range 0x%08X-0x%08X is empty",
				startCodeAddress, startCodeAddress + length);
			return false;
		}

		// calcualte the range of code entries
		const uint32 firstTableEntry = (uint32)(startCodeAddress - m_startCodeAddress);
		for (uint32 i = 0; i < numTableEntries; ++i)
		{
			if (!forceOverride)
			{
				if (m_codeTable[firstTableEntry + i] != &InvalidCodeBlock)
				{
					GLog.Err("Code: Address %06Xh is already occupied by code", i + m_startCodeAddress);
					return false;
				}
			}

			m_codeTable[firstTableEntry + i] = func;
		}

		// mounted
		return true;
	}

	uint64 __fastcall CodeTable::InvalidCodeBlock(uint64 ip, RegisterBank& regs)
	{
		GLog.Err("Code: Invalid IP=%08Xh reached", ip);
		return regs.ReturnFromFunction();
	}

} // runtime