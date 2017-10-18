#include "build.h"
#include "xenonLibUtils.h" 
#include "xenonLibNatives.h"
#include "xenonPlatform.h"
#include "xenonGraphics.h"

//---------------------------------------------------------------------------

uint64 __fastcall XboxXam_XamEnumerate(uint64 ip, cpu::CpuRegs& regs)
{
	RETURN_DEFAULT();
}

uint64 __fastcall XboxXam_XamContentCreateEx(uint64 ip, cpu::CpuRegs& regs)
{
	RETURN_DEFAULT();
}

uint64 __fastcall XboxXam_XamContentSetThumbnail(uint64 ip, cpu::CpuRegs& regs)
{
	RETURN_DEFAULT();
}

uint64 __fastcall XboxXam_XamContentClose(uint64 ip, cpu::CpuRegs& regs)
{
	RETURN_DEFAULT();
}

uint64 __fastcall XboxXam_XamContentCreateEnumerator(uint64 ip, cpu::CpuRegs& regs)
{
	RETURN_DEFAULT();
}

uint64 __fastcall XboxXam_XamShowSigninUI(uint64 ip, cpu::CpuRegs& regs)
{
	RETURN_DEFAULT();
}

uint64 __fastcall XboxXam_XamShowKeyboardUI(uint64 ip, cpu::CpuRegs& regs)
{
	RETURN_DEFAULT();
}

uint64 __fastcall XboxXam_XamShowDeviceSelectorUI(uint64 ip, cpu::CpuRegs& regs)
{
	RETURN_DEFAULT();
}

uint64 __fastcall XboxXam_XamShowMessageBoxUI(uint64 ip, cpu::CpuRegs& regs)
{
	RETURN_DEFAULT();
}

uint64 __fastcall XboxXam_XamLoaderLaunchTitle(uint64 ip, cpu::CpuRegs& regs)
{
	RETURN_DEFAULT();
}

uint64 __fastcall XboxXam_XamTaskShouldExit(uint64 ip, cpu::CpuRegs& regs)
{
	RETURN_DEFAULT();
}

uint64 __fastcall XboxXam_XamTaskCloseHandle(uint64 ip, cpu::CpuRegs& regs)
{
	RETURN_DEFAULT();
}

uint64 __fastcall XboxXam_XamTaskSchedule(uint64 ip, cpu::CpuRegs& regs)
{
	RETURN_DEFAULT();
}

uint64 __fastcall XboxXam_XamNotifyCreateListener(uint64 ip, cpu::CpuRegs& regs)
{
	auto listener = GPlatform.GetKernel().CreateEventNotifier();
	const auto handle = listener->GetHandle();
	RETURN_ARG(handle);
}

uint64 __fastcall XboxXam_XamUserGetSigninState(uint64 ip, cpu::CpuRegs& regs)
{
	RETURN_DEFAULT();
}

//---------------------------------------------------------------------------

void RegisterXboxXAM(runtime::Symbols& symbols)
{
#define REGISTER(x) symbols.RegisterFunction(#x, (runtime::TBlockFunc) &XboxXam_##x);
	REGISTER(XamEnumerate);
	REGISTER(XamContentCreateEx);
	REGISTER(XamContentClose);
	REGISTER(XamContentSetThumbnail);
	REGISTER(XamContentCreateEnumerator);
	REGISTER(XamShowSigninUI);
	REGISTER(XamShowKeyboardUI);
	REGISTER(XamShowDeviceSelectorUI);
	REGISTER(XamShowMessageBoxUI);
	REGISTER(XamLoaderLaunchTitle);
	REGISTER(XamTaskShouldExit);
	REGISTER(XamTaskCloseHandle);
	REGISTER(XamTaskSchedule);
	REGISTER(XamNotifyCreateListener);
	REGISTER(XamUserGetSigninState);
#undef REGISTER
}
