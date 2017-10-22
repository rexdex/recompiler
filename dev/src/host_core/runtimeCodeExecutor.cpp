#include "build.h"
#include "runtimeCodeTable.h"
#include "runtimeCodeExecutor.h"
#include "runtimeRegisterBank.h"
#include "runtimeTraceWriter.h"
#include "runtimeTraceFile.h"

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
		auto register shiftedCodeTable = m_code->GetCodeTable();
		auto register ip = m_ip;
		auto register counter = 1000;

		while (ip && counter--)
		{
			const auto func = shiftedCodeTable[ip - m_code->GetCodeStartAddress()];
			ip = func(ip, *m_regs);
		}

		m_ip = ip;
		return (m_ip != 0);
	}

#if defined(_WIN32) || defined(_WIN64)
	static int ExceptionFilter(unsigned int code, struct _EXCEPTION_POINTERS *ep)
	{
		// print info about AV
		if (code == EXCEPTION_ACCESS_VIOLATION)
		{
			const auto* mode = (ep->ExceptionRecord->ExceptionInformation[0] == 1) ? "writing" : "reading";
			const auto address = ep->ExceptionRecord->ExceptionInformation[1];
			GLog.Err("Runtime: Access violation %s location 0x%08llX. Stopping bad thread.", mode, address);
		}

		// we claim to handle everything - we will be closing the thread any way
		return EXCEPTION_EXECUTE_HANDLER;
	}
#endif

	bool CodeExecutor::RunTraced(TraceWriter& trace)
	{
		// on windows add some extra security - in case an AV happens we will be able to flush the trace
#if defined(_WIN32) || defined(_WIN64)
		__try
#endif
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
		}
#if defined(_WIN32) || defined(_WIN64)
		__except(ExceptionFilter(GetExceptionCode(), GetExceptionInformation()))
		{
			if (trace.GetParentFile())
				trace.GetParentFile()->Flush();
			m_ip = 0;
		}
#endif

		return (m_ip != 0);
	}

} // runtime
