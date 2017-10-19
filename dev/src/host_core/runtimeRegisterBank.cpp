#include "build.h"
#include "runtimeRegisterBank.h"

namespace runtime
{

	RegisterBank::RegisterBank(const RegisterBankInfo* desc)
		: m_desc(desc)
	{}

	RegisterBank::~RegisterBank()
	{
	}

} // runtime
