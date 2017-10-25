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

		uint64 __fastcall Xbox_XamResetInactivity(uint64 ip, cpu::CpuRegs& regs)
		{
			//const auto val = regs.R3;
			RETURN_ARG(0);
		}

		uint64 __fastcall Xbox_XamEnableInactivityProcessing(uint64 ip, cpu::CpuRegs& regs)
		{
			//const auto val = regs.R3;
			//const auto val2 = regs.R4;
			RETURN_ARG(0);
		}

		uint64 __fastcall Xbox_XamUserGetDeviceContext(uint64 ip, cpu::CpuRegs& regs)
		{
			auto user = (uint32)regs.R3;
			auto unk = regs.R4;
			auto outPtr = Pointer<uint32>(regs.R5);

			*outPtr = 0;

			if (!user || (user & 0xFF) == 0xFF)
			{
				RETURN_ARG(0);
			}
			else
			{
				RETURN_ARG(-1);
			}
		}

		uint64 __fastcall Xbox_XamInputGetState(uint64 ip, cpu::CpuRegs& regs)
		{
			auto user = (uint32)(regs.R3);
			auto flags = (uint32)(regs.R4);
			auto statePtr = Pointer<X_INPUT_STATE>(regs.R5);

			if ((flags & 0xFF) && (flags & XINPUT_FLAG_GAMEPAD) == 0)
				RETURN_ARG(X_ERROR_DEVICE_NOT_CONNECTED);

			if ((user & 0xFF) == 0xFF || (flags & XINPUT_FLAG_ANY_USER))
				user = 0;

			auto ret = GPlatform.GetInputSystem().GetState(user, statePtr.GetNativePointer());
			RETURN_ARG(ret);
		}

		uint64 __fastcall Xbox_XamInputGetCapabilities(uint64 ip, cpu::CpuRegs& regs)
		{
			auto user = (uint32)regs.R3;
			auto flags = (uint32)regs.R4;
			auto capsPtr = Pointer<X_INPUT_CAPABILITIES>(regs.R5);

			if (!capsPtr.IsValid())
				RETURN_ARG(X_ERROR_BAD_ARGUMENTS);

			if ((flags & 0xFF) && (flags & XINPUT_FLAG_GAMEPAD) == 0)
				RETURN_ARG(X_ERROR_DEVICE_NOT_CONNECTED);

			if ((user & 0xFF) == 0xFF || (flags & XINPUT_FLAG_ANY_USER))
				user = 0;

			const auto ret = GPlatform.GetInputSystem().GetCapabilities(user, flags, capsPtr.GetNativePointer());
			RETURN_ARG(ret);
		}

		uint64 __fastcall Xbox_XamInputGetCapabilitiesEx(uint64 ip, cpu::CpuRegs& regs)
		{
			auto tmp = (uint32)regs.R3;
			auto user = (uint32)regs.R4;
			auto flags = (uint32)regs.R5;
			auto capsPtr = Pointer<X_INPUT_CAPABILITIES>(regs.R6);

			if (!capsPtr.IsValid())
				RETURN_ARG(X_ERROR_BAD_ARGUMENTS);

			if ((flags & 0xFF) && (flags & XINPUT_FLAG_GAMEPAD) == 0)
				RETURN_ARG(X_ERROR_DEVICE_NOT_CONNECTED);

			if ((user & 0xFF) == 0xFF || (flags & XINPUT_FLAG_ANY_USER))
				user = 0;

			const auto ret = GPlatform.GetInputSystem().GetCapabilities(user, flags, capsPtr.GetNativePointer());
			RETURN_ARG(ret);
		}

		uint64 __fastcall Xbox_XamInputSetState(uint64 ip, cpu::CpuRegs& regs)
		{
			auto user = (uint32)regs.R3;
			auto unk = (uint32)regs.R4;
			auto ptr = Pointer<X_INPUT_VIBRATION>(regs.R5);

			if (!ptr.IsValid())
				RETURN_ARG(X_ERROR_BAD_ARGUMENTS);

			if ((user & 0xFF) == 0xFF)
				user = 0;

			auto ret = GPlatform.GetInputSystem().SetState(user, ptr.GetNativePointer());
			RETURN_ARG(ret);
		}

		// http://ffplay360.googlecode.com/svn/Test/Common/AtgXime.cpp
		uint64 __fastcall Xbox_XamInputGetKeystroke(uint64 ip, cpu::CpuRegs& regs)
		{
			auto user = (uint32)regs.R3;
			auto flags = (uint32)regs.R4;
			auto ptr = Pointer<X_INPUT_KEYSTROKE>(regs.R5);

			if (!ptr.IsValid())
				RETURN_ARG(X_ERROR_BAD_ARGUMENTS);

			if ((flags & 0xFF) && (flags & XINPUT_FLAG_GAMEPAD) == 0)
				RETURN_ARG(X_ERROR_DEVICE_NOT_CONNECTED);

			if ((user & 0xFF) == 0xFF || (flags & XINPUT_FLAG_ANY_USER))
				user = 0;

			auto ret = GPlatform.GetInputSystem().GetKeystroke(user, flags, ptr.GetNativePointer());
			RETURN_ARG(ret);
		}

		// http://ffplay360.googlecode.com/svn/Test/Common/AtgXime.cpp
		uint64 __fastcall Xbox_XamInputGetKeystrokeEx(uint64 ip, cpu::CpuRegs& regs)
		{
			auto user = cpu::mem::loadAddr<uint32>((uint32)regs.R3);
			auto flags = (uint32)regs.R4;
			auto ptr = Pointer<X_INPUT_KEYSTROKE>(regs.R5);

			if (!ptr.IsValid())
				RETURN_ARG(X_ERROR_BAD_ARGUMENTS);

			if ((flags & 0xFF) && (flags & XINPUT_FLAG_GAMEPAD) == 0)
				RETURN_ARG(X_ERROR_DEVICE_NOT_CONNECTED);

			if ((user & 0xFF) == 0xFF || (flags & XINPUT_FLAG_ANY_USER))
				user = 0;

			auto ret = GPlatform.GetInputSystem().GetKeystroke(user, flags, ptr.GetNativePointer());
			RETURN_ARG(ret);
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