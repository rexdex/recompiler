#include "build.h"
#include "xenonLibUtils.h" 
#include "xenonLibNatives.h"
#include "xenonPlatform.h"
#include "xenonGraphics.h"
#include "xenonUserManager.h"

//---------------------------------------------------------------------------

uint64 __fastcall XboxXam_XamEnumerate(uint64 ip, cpu::CpuRegs& regs)
{
	RETURN_ARG(xnative::X_STATUS_UNSUCCESSFUL);
//	RETURN_DEFAULT();
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
	RETURN_ARG(xnative::X_STATUS_UNSUCCESSFUL);
	//	RETURN_DEFAULT();
}

uint64 __fastcall XboxXam_XamShowSigninUI(uint64 ip, cpu::CpuRegs& regs)
{
	// HACK: when requested to do that it means that we want to sign in some users, for now, sign-in the default one
	if (nullptr == GPlatform.GetUserProfileManager().GetUser(0))
	{
		GLog.Log("XAM: Detected request to show sign-in window, signin in default user");
		GPlatform.GetUserProfileManager().SignInUser(0, xenon::UserProfileManager::DEFAULT_USER_UID);
	}

	// notify about the UI hiding
	GPlatform.GetKernel().PostEventNotification(xnative::XN_SYS_UI.GetCode(), false);

	// no errors here
	RETURN_ARG(0);
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

uint64 __fastcall XboxXam_XamUserGetXUID(uint64 ip, cpu::CpuRegs& regs)
{
	const auto index = (uint32)(regs.R3);
	auto ptr = utils::MemPtr64<uint64>(regs.R5);

	if (index >= xenon::UserProfileManager::MAX_USERS)
		RETURN_ARG(xnative::X_ERROR_NO_SUCH_USER);

	auto* user = GPlatform.GetUserProfileManager().GetUser(index);
	if (user != nullptr)
		ptr.Set(user->GetID());

	RETURN_ARG(0);
}

uint64 __fastcall XboxXam_XamUserGetSigninState(uint64 ip, cpu::CpuRegs& regs)
{
	const auto index = (uint32)(regs.R3);

	if (index >= xenon::UserProfileManager::MAX_USERS)
		RETURN_ARG(xnative::X_ERROR_NO_SUCH_USER);

	// XUserSigninState_NotSignedIn User is not signed in. 
	// XUserSigninState_SignedInLocally User is signed in on the local Xbox 360 console, but not signed in to Xbox LIVE.
	// XUserSigninState_SignedInToLive User is signed in to Xbox LIVE.

	auto* user = GPlatform.GetUserProfileManager().GetUser(index);
	if (user == nullptr)
		RETURN_ARG(0);

	RETURN_ARG(1); // signed locally
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
	REGISTER(XamUserGetXUID);
#undef REGISTER
}