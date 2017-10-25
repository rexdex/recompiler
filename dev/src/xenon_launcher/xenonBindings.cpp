#include "build.h"
#include "xenonLib.h"
#include "xenonBindings.h"

namespace xenon
{
	namespace lib
	{
		namespace binding
		{

			//---------------------------------------------------------------------------

			FunctionInterface::FunctionInterface(const uint64 ip, runtime::RegisterBank& regs)
				: m_ip(ip)
				, m_regs((cpu::CpuRegs&)regs)
				, m_nextGenericArgument(0)
				, m_nextFloatingArgument(0)
			{}

			//---------------------------------------------------------------------------



		} // binding
	} // lib
} // xenon
