#include "build.h"
#include "xenonLibUtils.h" 
#include "xenonLibNatives.h"
#include "xenonThread.h"
#include <mutex>
#include "xenonPlatform.h"
#include "xenonInput.h"

//---------------------------------------------------------------------------

static const uint32 XINPUT_FLAG_GAMEPAD = 0x01;
static const uint32 XINPUT_FLAG_ANY_USER = 1 << 30;

uint64 __fastcall XboxInput_XamResetInactivity(uint64 ip, cpu::CpuRegs& regs)
{
	const auto val = regs.R3;
	GLog.Log("XamResetInactivity(%d)", val);
	RETURN_ARG(0);
}

uint64 __fastcall XboxInput_XamEnableInactivityProcessing(uint64 ip, cpu::CpuRegs& regs)
{
	const auto val = regs.R3;
	const auto val2 = regs.R4;
	GLog.Log("XamEnableInactivityProcessing(%d, %d)", val, val2);
	RETURN_ARG(0);
}

uint64 __fastcall XboxInput_XamUserGetDeviceContext(uint64 ip, cpu::CpuRegs& regs)
{
	auto user = (uint32)regs.R3;
	auto unk = regs.R4;
	void* outPtr = (void*)regs.R5;

	GLog.Log("XamUserGetDeviceContext(%d, %d, %.8X)", user, unk, outPtr);

	cpu::mem::store<uint32>(outPtr, 0);

	if (!user || (user & 0xFF) == 0xFF)
	{
		RETURN_ARG(0);
	}
	else
	{
		RETURN_ARG(-1);
	}
}

uint64 __fastcall XboxInput_XamInputGetState(uint64 ip, cpu::CpuRegs& regs)
{
	auto user = (uint32)(regs.R3);
	auto flags = (uint32)(regs.R4);
	auto* statePtr = (xnative::X_INPUT_STATE*)(uint32)regs.R5;

	GLog.Log("XamInputGetState(%d, %.8X, %.8X)", user, flags, statePtr);

	if ((flags & 0xFF) && (flags & XINPUT_FLAG_GAMEPAD) == 0)
		RETURN_ARG(xnative::X_ERROR_DEVICE_NOT_CONNECTED);

	if ((user & 0xFF) == 0xFF || (flags & XINPUT_FLAG_ANY_USER))
		user = 0;

	auto ret = GPlatform.GetInputSystem().GetState(user, statePtr);
	RETURN_ARG(ret);
}

uint64 __fastcall XboxInput_XamInputGetCapabilities(uint64 ip, cpu::CpuRegs& regs)
{
	auto user = (uint32)regs.R3;
	auto flags = (uint32)regs.R4;
	auto capsPtr = (void*)(uint32)regs.R5;

	GLog.Log("XamInputGetCapabilities(%d, %.8X, %.8X)", user, flags, capsPtr);

	if (!capsPtr)
		RETURN_ARG(xnative::X_ERROR_BAD_ARGUMENTS);

	if ((flags & 0xFF) && (flags & XINPUT_FLAG_GAMEPAD) == 0)
		RETURN_ARG(xnative::X_ERROR_DEVICE_NOT_CONNECTED);

	if ((user & 0xFF) == 0xFF || (flags & XINPUT_FLAG_ANY_USER))
		user = 0;

	auto* caps = (xnative::X_INPUT_CAPABILITIES*)capsPtr;
	const auto ret = GPlatform.GetInputSystem().GetCapabilities(user, flags, caps);
	RETURN_ARG(ret);
}

uint64 __fastcall XboxInput_XamInputGetCapabilitiesEx(uint64 ip, cpu::CpuRegs& regs)
{
	auto tmp = (uint32)regs.R3;
	auto user = (uint32)regs.R4;
	auto flags = (uint32)regs.R5;
	auto capsPtr = (void*)(uint32)regs.R6;

	GLog.Log("XamInputGetCapabilitiesEx(%d, %d, %.8X, %.8X)", tmp, user, flags, capsPtr);

	if (!capsPtr)
		RETURN_ARG(xnative::X_ERROR_BAD_ARGUMENTS);

	if ((flags & 0xFF) && (flags & XINPUT_FLAG_GAMEPAD) == 0)
		RETURN_ARG(xnative::X_ERROR_DEVICE_NOT_CONNECTED);

	if ((user & 0xFF) == 0xFF || (flags & XINPUT_FLAG_ANY_USER))
		user = 0;

	auto* caps = (xnative::X_INPUT_CAPABILITIES*)capsPtr;
	const auto ret = GPlatform.GetInputSystem().GetCapabilities(user, flags, caps);
	RETURN_ARG(ret);
}

uint64 __fastcall XboxInput_XamInputSetState(uint64 ip, cpu::CpuRegs& regs)
{
	auto user = (uint32)regs.R3;
	auto unk = (uint32)regs.R4;
	auto* ptr = (xnative::X_INPUT_VIBRATION*)(uint32)regs.R5;

	GLog.Log("XamInputSetState(%d, %.8X, %.8X)", user, unk, ptr);

	if (!ptr)
		RETURN_ARG(xnative::X_ERROR_BAD_ARGUMENTS);

	if ((user & 0xFF) == 0xFF)
		user = 0;

	auto ret = GPlatform.GetInputSystem().SetState(user, ptr);
	RETURN_ARG(ret);
}

// http://ffplay360.googlecode.com/svn/Test/Common/AtgXime.cpp
uint64 __fastcall XboxInput_XamInputGetKeystroke(uint64 ip, cpu::CpuRegs& regs)
{
	auto user = (uint32)regs.R3;
	auto flags = (uint32)regs.R4;
	auto* ptr = (xnative::X_INPUT_KEYSTROKE*)(uint32)regs.R5;

	GLog.Log("XamInputGetKeystroke(%d, %.8X, %.8X)", user, flags, ptr);

	if (!ptr)
		RETURN_ARG(xnative::X_ERROR_BAD_ARGUMENTS);

	if ((flags & 0xFF) && (flags & XINPUT_FLAG_GAMEPAD) == 0)
		RETURN_ARG(xnative::X_ERROR_DEVICE_NOT_CONNECTED);

	if ((user & 0xFF) == 0xFF || (flags & XINPUT_FLAG_ANY_USER))
		user = 0;

	auto ret = GPlatform.GetInputSystem().GetKeystroke(user, flags, ptr);
	RETURN_ARG(ret);
}

// http://ffplay360.googlecode.com/svn/Test/Common/AtgXime.cpp
uint64 __fastcall XboxInput_XamInputGetKeystrokeEx(uint64 ip, cpu::CpuRegs& regs)
{
	auto user = cpu::mem::loadAddr<uint32>((uint32)regs.R3);
	auto flags = (uint32)regs.R4;
	auto* ptr = (xnative::X_INPUT_KEYSTROKE*)regs.R5;

	GLog.Log("XamInputGetKeystroke(%d, %.8X, %.8X)", user, flags, ptr);

	if (!ptr)
		RETURN_ARG(xnative::X_ERROR_BAD_ARGUMENTS);

	if ((flags & 0xFF) && (flags & XINPUT_FLAG_GAMEPAD) == 0)
		RETURN_ARG(xnative::X_ERROR_DEVICE_NOT_CONNECTED);

	if ((user & 0xFF) == 0xFF || (flags & XINPUT_FLAG_ANY_USER))
		user = 0;

	auto ret = GPlatform.GetInputSystem().GetKeystroke(user, flags, ptr);
	RETURN_ARG(ret);
}

//---------------------------------------------------------------------------

void RegisterXboxInput(runtime::Symbols& symbols)
{
#define REGISTER(x) symbols.RegisterFunction(#x, (runtime::TBlockFunc) &XboxInput_##x);
	REGISTER(XamUserGetDeviceContext);
	REGISTER(XamInputGetState);
	REGISTER(XamInputSetState);
	REGISTER(XamInputGetCapabilities);
	REGISTER(XamInputGetCapabilitiesEx);
	REGISTER(XamResetInactivity);
	REGISTER(XamEnableInactivityProcessing);
	REGISTER(XamInputGetKeystroke);
	REGISTER(XamInputGetKeystrokeEx);
#undef REGISTER
}