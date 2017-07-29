#include "build.h"
#include "runtimeCodeTable.h"
#include "runtimeCodeExecutor.h"
#include "runtimeRegisterBank.h"
#include "..\..\platforms\xenon\xenonLauncher\xenonCPU.h"

static uint32 GTrackedAddress = 0;
static uint32 GTrackedValue = 0xC0114800;

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
		if (GTrackedAddress != 0)
		{
			if (mem::loadAddr<uint32>(GTrackedAddress) != GTrackedValue)
			{
				GLog.Err("Value changed!");
				::DebugBreak();
			}
		}

		const auto relAddress = m_ip - m_code->GetCodeStartAddress();
		const auto func = m_code->GetCodeTable()[relAddress];
		m_ip = func(m_ip, *m_regs);
		return (m_ip != 0);
	}

} // runtime
