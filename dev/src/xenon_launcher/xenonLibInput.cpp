#include "build.h"
#include "xenonLib.h"  
#include "xenonLibNatives.h"
#include "xenonThread.h"
#include "xenonPlatform.h"
#include "xenonInput.h"
#include "xenonBindings.h"

namespace xenon
{
	namespace lib
	{

		//---------------------------------------------------------------------------

		static const uint32 XINPUT_FLAG_GAMEPAD = 0x01;
		static const uint32 XINPUT_FLAG_ANY_USER = 1 << 30;

		X_STATUS Xbox_XamResetInactivity()
		{
			return X_STATUS_SUCCESS;
		}

		X_STATUS Xbox_XamEnableInactivityProcessing()
		{
			return X_STATUS_SUCCESS;
		}

		X_STATUS Xbox_XamUserGetDeviceContext(uint32 userIndex, uint32, Pointer<uint32> outPtr)
		{
			*outPtr = 0;

			if (!userIndex || (userIndex & 0xFF) == 0xFF)
				return 0;
			else
				return -1;
		}

		X_STATUS Xbox_XamInputGetState(uint32 userIndex, uint32 flags, Pointer<X_INPUT_STATE> outStatePtr)
		{
			if ((flags & 0xFF) && (flags & XINPUT_FLAG_GAMEPAD) == 0)
				return X_ERROR_DEVICE_NOT_CONNECTED;

			if ((userIndex & 0xFF) == 0xFF || (flags & XINPUT_FLAG_ANY_USER))
				userIndex = 0;

			return GPlatform.GetInputSystem().GetState(userIndex, outStatePtr.GetNativePointer());
		}

		X_STATUS Xbox_XamInputGetCapabilities(uint32 userIndex, uint32 flags, Pointer<X_INPUT_CAPABILITIES> outCapsPtr)
		{
			if (!outCapsPtr.IsValid())
				return X_ERROR_BAD_ARGUMENTS;

			if ((flags & 0xFF) && (flags & XINPUT_FLAG_GAMEPAD) == 0)
				return X_ERROR_DEVICE_NOT_CONNECTED;

			if ((userIndex & 0xFF) == 0xFF || (flags & XINPUT_FLAG_ANY_USER))
				userIndex = 0;

			return GPlatform.GetInputSystem().GetCapabilities(userIndex, flags, outCapsPtr.GetNativePointer());
		}

		X_STATUS Xbox_XamInputGetCapabilitiesEx(uint32 tempIndex, uint32 userIndex, uint32 flags, Pointer<X_INPUT_CAPABILITIES> outCapsPtr)
		{
			if (!outCapsPtr.IsValid())
				return X_ERROR_BAD_ARGUMENTS;

			if ((flags & 0xFF) && (flags & XINPUT_FLAG_GAMEPAD) == 0)
				return X_ERROR_DEVICE_NOT_CONNECTED;

			if ((userIndex & 0xFF) == 0xFF || (flags & XINPUT_FLAG_ANY_USER))
				userIndex = 0;

			return GPlatform.GetInputSystem().GetCapabilities(userIndex, flags, outCapsPtr.GetNativePointer());
		}

		X_STATUS Xbox_XamInputSetState(uint32 userIndex, uint32, Pointer<X_INPUT_VIBRATION> statePtr)
		{
			if (!statePtr.IsValid())
				return X_ERROR_BAD_ARGUMENTS;

			if ((userIndex & 0xFF) == 0xFF)
				userIndex = 0;

			return GPlatform.GetInputSystem().SetState(userIndex, statePtr.GetNativePointer());
		}

		// http://ffplay360.googlecode.com/svn/Test/Common/AtgXime.cpp
		X_STATUS Xbox_XamInputGetKeystroke(uint32 userIndex, uint32 flags, Pointer<X_INPUT_KEYSTROKE> outKeystrokePtr)
		{
			if (!outKeystrokePtr.IsValid())
				return X_ERROR_BAD_ARGUMENTS;

			if ((flags & 0xFF) && (flags & XINPUT_FLAG_GAMEPAD) == 0)
				return X_ERROR_DEVICE_NOT_CONNECTED;

			if ((userIndex & 0xFF) == 0xFF || (flags & XINPUT_FLAG_ANY_USER))
				userIndex = 0;

			return GPlatform.GetInputSystem().GetKeystroke(userIndex, flags, outKeystrokePtr.GetNativePointer());
		}

		// http://ffplay360.googlecode.com/svn/Test/Common/AtgXime.cpp
		X_STATUS Xbox_XamInputGetKeystrokeEx(Pointer<uint32> userIndexPointer, uint32 flags, Pointer<X_INPUT_KEYSTROKE> outKeystrokePtr)
		{
			if (!userIndexPointer.IsValid())
				return X_ERROR_BAD_ARGUMENTS;

			auto userIndex = *userIndexPointer;
			return Xbox_XamInputGetKeystroke(userIndex, flags, outKeystrokePtr);
		}
			
		//---------------------------------------------------------------------------

		void RegisterXboxInput(runtime::Symbols& symbols)
		{
			REGISTER(XamUserGetDeviceContext);
			REGISTER(XamInputGetState);
			REGISTER(XamInputSetState);
			REGISTER(XamInputGetCapabilities);
			REGISTER(XamInputGetCapabilitiesEx);
			REGISTER(XamResetInactivity);
			REGISTER(XamEnableInactivityProcessing);
			REGISTER(XamInputGetKeystroke);
			REGISTER(XamInputGetKeystrokeEx);
		}

	} // lib
} // xenon