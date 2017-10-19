#pragma once

#include "launcherBase.h"

namespace runtime
{
	class RegisterBankInfo;

	/// bank of registers, used by CPU emulation
	/// note, the actual size depends on the cpu, always allocated by the platform
	class LAUNCHER_API RegisterBank
	{
	public:
		RegisterBank(const RegisterBankInfo* desc);
		virtual ~RegisterBank();

		/// get the function return address, used in error handling situations
		virtual uint64 ReturnFromFunction() = 0;

		/// get description of this register bank
		inline const RegisterBankInfo* GetDesc() const { return m_desc; }

	protected:
		const RegisterBankInfo*	m_desc;
	};

} // runtime
		