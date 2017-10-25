#include "build.h"
#include "xenonLib.h"  

namespace xenon
{
	//---------------------------------------------------------------------------

	FunctionInterface::FunctionInterface(const uint64 ip, const cpu::CpuRegs& regs)
		: m_ip(ip)
		, m_regs(regs)
	{}

	//---------------------------------------------------------------------------
	
}
