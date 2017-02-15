#include "build.h"
#include "runtimeCodeTable.h"
#include "runtimeCodeExecutor.h"
#include "runtimeRegisterBank.h"

namespace runtime
{

	CodeExecutor::CodeExecutor(RegisterBank* regs, const CodeTable* code, const uint64 ip)
		: m_regs(regs)
		, m_code(code)
		, m_ip(ip)
	{
	}

	CodeExecutor::~CodeExecutor()
	{
	}

	bool CodeExecutor::Step()
	{
		const auto relAddress = m_ip - m_code->GetCodeStartAddress();
		const auto func = m_code->GetCodeTable()[relAddress];
		m_ip = func(m_ip, *m_regs);
		return (m_ip != 0);
	}

} // runtime
