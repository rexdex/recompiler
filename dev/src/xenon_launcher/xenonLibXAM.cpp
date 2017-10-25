#include "build.h"
#include "xenonLib.h"  
#include "xenonLibNatives.h"
#include "xenonPlatform.h"
#include "xenonGraphics.h"
#include "xenonUserManager.h"
#include "xenonBindings.h"

namespace xenon
{
	namespace lib
	{

		//---------------------------------------------------------------------------

		X_STATUS Xbox_XamEnumerate()
		{
			return X_STATUS_UNSUCCESSFUL;
		}

		X_STATUS Xbox_XamContentCreateEx()
		{
			return X_STATUS_UNSUCCESSFUL;
		}

		X_STATUS Xbox_XamContentSetThumbnail()
		{
			return X_STATUS_UNSUCCESSFUL;
		}

		X_STATUS Xbox_XamContentClose()
		{
			return X_STATUS_UNSUCCESSFUL;
		}

		X_STATUS Xbox_XamContentCreateEnumerator()
		{
			return X_STATUS_UNSUCCESSFUL;
		}

		X_STATUS Xbox_XamShowSigninUI()
		{
			// HACK: when requested to do that it means that we want to sign in some users, for now, sign-in the default one
			if (nullptr == GPlatform.GetUserProfileManager().GetUser(0))
			{
				GLog.Log("XAM: Detected request to show sign-in window, signin in default user");
				GPlatform.GetUserProfileManager().SignInUser(0, xenon::UserProfileManager::DEFAULT_USER_UID);
			}

			// notify about the UI hiding
			GPlatform.GetKernel().PostEventNotification(XN_SYS_UI.GetCode(), false);

			// no errors here
			return X_STATUS_SUCCESS;
		}

		X_STATUS Xbox_XamShowKeyboardUI()
		{
			GPlatform.GetKernel().PostEventNotification(XN_SYS_UI.GetCode(), false);
			return X_STATUS_SUCCESS;
		}

		X_STATUS Xbox_XamShowDeviceSelectorUI()
		{
			GPlatform.GetKernel().PostEventNotification(XN_SYS_UI.GetCode(), false);
			return X_STATUS_SUCCESS;
		}

		X_STATUS Xbox_XamShowMessageBoxUI()
		{
			GPlatform.GetKernel().PostEventNotification(XN_SYS_UI.GetCode(), false);
			return X_STATUS_SUCCESS;
		}

		X_STATUS Xbox_XamLoaderTerminateTitle()
		{
			GLog.Log("XamLoaderTerminateTitle: called");
			throw runtime::TerminateProcessException(0, 0);
		}

		X_STATUS Xbox_XamShowMessageBoxUIEx()
		{
			GPlatform.GetKernel().PostEventNotification(XN_SYS_UI.GetCode(), false);
			return X_STATUS_SUCCESS;
		}

		X_STATUS Xbox_XamLoaderLaunchTitle()
		{
			return X_STATUS_UNSUCCESSFUL;
		}

		X_STATUS Xbox_XamTaskShouldExit()
		{
			return X_STATUS_UNSUCCESSFUL;
		}

		X_STATUS Xbox_XamTaskCloseHandle()
		{
			return X_STATUS_UNSUCCESSFUL;
		}

		X_STATUS Xbox_XamTaskSchedule()
		{
			return X_STATUS_UNSUCCESSFUL;
		}

		uint32 Xbox_XamNotifyCreateListener()
		{
			auto listener = GPlatform.GetKernel().CreateEventNotifier();
			return listener->GetHandle();
		}

		X_STATUS Xbox_XamUserGetXUID(uint32 index, Pointer<uint64> outIDPtr)
		{
			if (index >= xenon::UserProfileManager::MAX_USERS)
				return X_ERROR_NO_SUCH_USER;

			if (!outIDPtr.IsValid())
				return X_ERROR_BAD_ARGUMENTS;

			auto* user = GPlatform.GetUserProfileManager().GetUser(index);
			if (user != nullptr)
				*outIDPtr = user->GetID();

			return X_STATUS_SUCCESS;
		}

		X_STATUS Xbox_XamUserGetSigninState(uint32 index)
		{
			if (index >= xenon::UserProfileManager::MAX_USERS)
				return X_ERROR_NO_SUCH_USER;

			// XUserSigninState_NotSignedIn User is not signed in. 
			// XUserSigninState_SignedInLocally User is signed in on the local Xbox 360 console, but not signed in to Xbox LIVE.
			// XUserSigninState_SignedInToLive User is signed in to Xbox LIVE.

			auto* user = GPlatform.GetUserProfileManager().GetUser(index);
			if (user == nullptr)
				return 0;

			return 1; // signed locally
		}

		uint32 Xbox_XNotifyGetNext(uint32 handle, uint32 id, Pointer<uint32> outEventId, Pointer<uint32> outEventData)
		{
			// resolve handle to object
			const auto eventListener = static_cast<xenon::KernelEventNotifier*>(GPlatform.GetKernel().ResolveHandle(handle, xenon::KernelObjectType::EventNotifier));
			if (!eventListener)
				return 0;

			// asked for specific notification ?
			if (id != 0)
			{
				uint32 data = 0;
				if (eventListener->PopSpecificNotification(id, data))
				{
					*outEventId = id;
					*outEventData = data;
					return 1;
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
					return 1;
				}
			}

			return 0;
		}

		//---------------------------------------------------------------------------

		void RegisterXboxXAM(runtime::Symbols& symbols)
		{
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
			REGISTER(XNotifyGetNext);
			REGISTER(XamLoaderTerminateTitle);
			REGISTER(XamShowMessageBoxUIEx);
		}

	} // lib
} // xenon