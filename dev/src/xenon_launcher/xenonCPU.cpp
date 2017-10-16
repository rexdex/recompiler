#include "build.h"
#include "xenonCPU.h"
#include "xenonCPUDevice.h"

namespace cpu
{
	CpuRegs::CpuRegs()
		: RegisterBank( &xenon::CPU_RegisterBankInfo::GetInstance() )
		, RES( nullptr )
		, INT(nullptr)
	{
		const auto size = (ptrdiff_t)((char*)&RES - (char*)&LR);
		memset(&LR, 0, size);
	}

	uint64 CpuRegs::ReturnFromFunction()
	{
		return LR;
	}

} // cpu