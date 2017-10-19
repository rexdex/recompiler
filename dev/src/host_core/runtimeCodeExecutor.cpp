#include "build.h"
#include "runtimeCodeTable.h"
#include "runtimeCodeExecutor.h"
#include "runtimeRegisterBank.h"
#include "runtimeTraceWriter.h"

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

	bool CodeExecutor::RunPure()
	{
		auto register shiftedCodeTable = m_code->GetCodeTable() - m_code->GetCodeStartAddress();
		auto register ip = m_ip;
		auto register counter = 1000;

		while (ip && counter--)
		{
			const auto func = shiftedCodeTable[ip];
			ip = func(ip, *m_regs);
		}

		m_ip = ip;
		return (m_ip != 0);
	}

	bool CodeExecutor::RunTraced(TraceWriter& trace)
	{
		auto register shiftedCodeTable = m_code->GetCodeTable() - m_code->GetCodeStartAddress();
		auto register ip = m_ip;
		auto register counter = 1000;

		while (ip && counter--)
		{
			const auto func = shiftedCodeTable[ip];
			ip = func(ip, *m_regs);
			trace.AddFrame(ip, *m_regs);
		}

		m_ip = ip;
		return (m_ip != 0);
	}

} // runtime
