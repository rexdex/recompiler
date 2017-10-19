#pragma once

#include "launcherBase.h"
#include "runtimeImageInfo.h"

namespace runtime
{
	class RegisterBank;

	/// Executable code
	class CodeTable
	{
	public:
		inline const uint64 GetCodeStartAddress() const { return m_startCodeAddress; }
		inline const uint64	GetCodeEndAddress() const { return m_endCodeAddress; }

		inline const TBlockFunc* GetCodeTable() const { return m_codeTable; }
		inline const uint32 GetCodeTableLength() const { return m_codeTableLength; }

		CodeTable(const uint64 startCodeAddress, const uint64 endCodeAddress);
		~CodeTable();

		// mount code block at given addresses
		bool MountBlock(const uint64 startCodeAddress, const uint32 length, TBlockFunc func, const bool forceOverride = false);

	private:
		// invalid code entry
		static uint64 __fastcall InvalidCodeBlock(uint64 ip, RegisterBank& regs);

		uint64				m_startCodeAddress;
		uint64				m_endCodeAddress;

		TBlockFunc*			m_codeTable;
		uint32				m_codeTableLength;
	};

} // runtime