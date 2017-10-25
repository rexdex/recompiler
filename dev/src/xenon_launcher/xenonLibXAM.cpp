#include "build.h"
#include "xenonLib.h"  
#include "xenonLibNatives.h"
#include "xenonPlatform.h"
#include "xenonGraphics.h"
#include "xenonUserManager.h"

namespace xenon
{
	namespace lib
	{

		//---------------------------------------------------------------------------

		uint64 __fastcall Xbox_XamEnumerate(uint64 ip, cpu::CpuRegs& regs)
		{
			RETURN_ARG(xnative::X_STATUS_UNSUCCESSFUL);
			//	RETURN_DEFAULT();
		}

		uint64 __fastcall Xbox_XamContentCreateEx(uint64 ip, cpu::CpuRegs& regs)
		{
			RETURN_DEFAULT();
		}

		uint64 __fastcall Xbox_XamContentSetThumbnail(uint64 ip, cpu::CpuRegs& regs)
		{
			RETURN_DEFAULT();
		}

		uint64 __fastcall Xbox_XamContentClose(uint64 ip, cpu::CpuRegs& regs)
		{
			RETURN_DEFAULT();
		}

		uint64 __fastcall Xbox_XamContentCreateEnumerator(uint64 ip, cpu::CpuRegs& regs)
		{
			RETURN_ARG(xnative::X_STATUS_UNSUCCESSFUL);
			//	RETURN_DEFAULT();
		}

		uint64 __fastcall Xbox_XamShowSigninUI(uint64 ip, cpu::CpuRegs& regs)
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

		uint64 __fastcall Xbox_XamShowKeyboardUI(uint64 ip, cpu::CpuRegs& regs)
		{
			RETURN_DEFAULT();
		}

		uint64 __fastcall Xbox_XamShowDeviceSelectorUI(uint64 ip, cpu::CpuRegs& regs)
		{
			RETURN_DEFAULT();
		}

		uint64 __fastcall Xbox_XamShowMessageBoxUI(uint64 ip, cpu::CpuRegs& regs)
		{
			RETURN_DEFAULT();
		}

		uint64 __fastcall Xbox_XamLoaderLaunchTitle(uint64 ip, cpu::CpuRegs& regs)
		{
			RETURN_DEFAULT();
		}

		uint64 __fastcall Xbox_XamTaskShouldExit(uint64 ip, cpu::CpuRegs& regs)
		{
			RETURN_DEFAULT();
		}

		uint64 __fastcall Xbox_XamTaskCloseHandle(uint64 ip, cpu::CpuRegs& regs)
		{
			RETURN_DEFAULT();
		}

		uint64 __fastcall Xbox_XamTaskSchedule(uint64 ip, cpu::CpuRegs& regs)
		{
			RETURN_DEFAULT();
		}

		uint64 __fastcall Xbox_XamNotifyCreateListener(uint64 ip, cpu::CpuRegs& regs)
		{
			auto listener = GPlatform.GetKernel().CreateEventNotifier();
			const auto handle = listener->GetHandle();
			RETURN_ARG(handle);
		}

		uint64 __fastcall Xbox_XamUserGetXUID(uint64 ip, cpu::CpuRegs& regs)
		{
			const auto index = (uint32)(regs.R3);
			auto ptr = Pointer<uint64>(regs.R5);

			if (index >= xenon::UserProfileManager::MAX_USERS)
				RETURN_ARG(xnative::X_ERROR_NO_SUCH_USER);

			auto* user = GPlatform.GetUserProfileManager().GetUser(index);
			if (user != nullptr)
				*ptr = user->GetID();

			RETURN_ARG(0);
		}

		uint64 __fastcall Xbox_XamUserGetSigninState(uint64 ip, cpu::CpuRegs& regs)
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

		uint64 __fastcall Xbox_XNotifyGetNext(uint64 ip, cpu::CpuRegs& regs)
		{
			const auto handle = (const uint32)regs.R3;
			const auto id = (const uint32)regs.R4;
			auto outEventId = Pointer<uint32>(regs.R5);
			auto outEventData = Pointer<uint32>(regs.R6);

			// resolve handle to object
			const auto eventListener = static_cast<xenon::KernelEventNotifier*>(GPlatform.GetKernel().ResolveHandle(handle, xenon::KernelObjectType::EventNotifier));
			if (!eventListener)
			{
				RETURN_ARG(0);
			}

			// asked for specific notification ?
			if (id != 0)
			{
				uint32 data = 0;
				if (eventListener->PopSpecificNotification(id, data))
				{
					*outEventId = id;
					*outEventData = data;
					RETURN_ARG(1);
				}
			}
			else
			{
				uint32 id = 0;
				uint32 data = 0;
				if (eventListener->PopNotification(id, data))
				{
					*outEventId = id;
					*outEventData = data;
					RETURN_ARG(1);
				}
			}

			RETURN_ARG(0);
		}

		//---------------------------------------------------------------------------

		void RegisterXboxXAM(runtime::Symbols& symbols)
		{
			REGISTER_RAW(XamEnumerate);
			REGISTER_RAW(XamContentCreateEx);
			REGISTER_RAW(XamContentClose);
			REGISTER_RAW(XamContentSetThumbnail);
			REGISTER_RAW(XamContentCreateEnumerator);
			REGISTER_RAW(XamShowSigninUI);
			REGISTER_RAW(XamShowKeyboardUI);
			REGISTER_RAW(XamShowDeviceSelectorUI);
			REGISTER_RAW(XamShowMessageBoxUI);
			REGISTER_RAW(XamLoaderLaunchTitle);
			REGISTER_RAW(XamTaskShouldExit);
			REGISTER_RAW(XamTaskCloseHandle);
			REGISTER_RAW(XamTaskSchedule);
			REGISTER_RAW(XamNotifyCreateListener);
			REGISTER_RAW(XamUserGetSigninState);
			REGISTER_RAW(XamUserGetXUID);
			REGISTER_RAW(XNotifyGetNext);
		}

	} // lib
} // xenon