#include "build.h"
#include "xenonKernel.h"
#include "xenonInput.h"

#include <Xinput.h>

#pragma comment (lib, "xinput.lib")

namespace xenon
{
#pragma warning (disable : 4995)
	//---

	class XInputHandler : public IInputDeviceHandler
	{
	public:
		XInputHandler()
		{}

		virtual bool Initialize() override final
		{
			XInputEnable(TRUE);
			return true;
		}

		virtual lib::XResult GetCapabilities(const uint32 user, const uint32 flags, lib::X_INPUT_CAPABILITIES* outCaps) override final
		{
			XINPUT_CAPABILITIES caps;
			memset(&caps, 0, sizeof(caps));
			auto ret = XInputGetCapabilities(user, flags, &caps);
			outCaps->flags = caps.Flags;
			outCaps->gamepad.buttons = caps.Gamepad.wButtons;
			outCaps->gamepad.left_trigger = caps.Gamepad.bLeftTrigger;
			outCaps->gamepad.right_trigger = caps.Gamepad.bRightTrigger;
			outCaps->gamepad.thumb_lx = caps.Gamepad.sThumbLX;
			outCaps->gamepad.thumb_ly = caps.Gamepad.sThumbLY;
			outCaps->gamepad.thumb_rx = caps.Gamepad.sThumbRX;
			outCaps->gamepad.thumb_ry = caps.Gamepad.sThumbRY;
			outCaps->sub_type = caps.SubType;
			outCaps->type = caps.Type;
			outCaps->vibration.left_motor_speed = caps.Vibration.wLeftMotorSpeed;
			outCaps->vibration.right_motor_speed = caps.Vibration.wRightMotorSpeed;
			return (lib::XResult)ret;
		}

		virtual lib::XResult GetState(const uint32 user, lib::X_INPUT_STATE* outState) override final
		{
			XINPUT_STATE state;
			memset(&state, 0, sizeof(state));
			auto ret = XInputGetState(user, &state);
			outState->gamepad.buttons = state.Gamepad.wButtons;
			outState->gamepad.left_trigger = state.Gamepad.bLeftTrigger;
			outState->gamepad.right_trigger = state.Gamepad.bRightTrigger;
			outState->gamepad.thumb_lx = state.Gamepad.sThumbLX;
			outState->gamepad.thumb_ly = state.Gamepad.sThumbLY;
			outState->gamepad.thumb_rx = state.Gamepad.sThumbRX;
			outState->gamepad.thumb_ry = state.Gamepad.sThumbRY;
			outState->packet_number = state.dwPacketNumber;
			return (lib::XResult)ret;
		}

		virtual lib::XResult SetState(const uint32 user, const lib::X_INPUT_VIBRATION* state) override final
		{
			XINPUT_VIBRATION vib;
			vib.wLeftMotorSpeed = state->left_motor_speed;
			vib.wRightMotorSpeed = state->right_motor_speed;
			auto ret = XInputSetState(user, &vib);
			return (lib::XResult)ret;
		}

		virtual lib::XResult GetKeystroke(const uint32 user, const uint32 flags, lib::X_INPUT_KEYSTROKE* outKey) override final
		{
			XINPUT_KEYSTROKE stroke;
			auto ret = XInputGetKeystroke(user, flags, &stroke);
			outKey->flags = stroke.Flags;
			outKey->hid_code = stroke.HidCode;
			outKey->unicode = stroke.Unicode;
			outKey->user_index = stroke.UserIndex;
			outKey->virtual_key = stroke.VirtualKey;
			return (lib::XResult)ret;
		}
	};

	//--

	InputSystem::InputSystem(Kernel* kernel, const launcher::Commandline& commandline)
	{
		{
			auto dev = new XInputHandler();
			if (dev->Initialize())
				m_handlers.push_back(dev);
		}
	}

	InputSystem::~InputSystem()
	{
		for (auto* dev : m_handlers)
			delete dev;
	}

	const lib::XResult InputSystem::GetState(const uint32 user, lib::X_INPUT_STATE* outState)
	{
		for (auto* drv : m_handlers)
		{
			auto ret = drv->GetState(user, outState);
			if (ret != lib::X_ERROR_DEVICE_NOT_CONNECTED)
				return ret;
		}

		return lib::X_ERROR_DEVICE_NOT_CONNECTED;
	}

	const lib::XResult InputSystem::GetCapabilities(const uint32 user, const uint32 flags, lib::X_INPUT_CAPABILITIES* outCaps)
	{
		for (auto* drv : m_handlers)
		{
			auto ret = drv->GetCapabilities(user, flags, outCaps);
			if (ret != lib::X_ERROR_DEVICE_NOT_CONNECTED)
				return ret;
		}

		return lib::X_ERROR_DEVICE_NOT_CONNECTED;
	}

	const lib::XResult InputSystem::SetState(const uint32 user, const lib::X_INPUT_VIBRATION* state)
	{
		for (auto* drv : m_handlers)
		{
			auto ret = drv->SetState(user, state);
			if (ret != lib::X_ERROR_DEVICE_NOT_CONNECTED)
				return ret;
		}

		return lib::X_ERROR_DEVICE_NOT_CONNECTED;
	}

	const lib::XResult InputSystem::GetKeystroke(const uint32 user, const uint32 flags, lib::X_INPUT_KEYSTROKE* outKey)
	{
		for (auto* drv : m_handlers)
		{
			auto ret = drv->GetKeystroke(user, flags, outKey);
			if (ret != lib::X_ERROR_DEVICE_NOT_CONNECTED)
				return ret;
		}

		return lib::X_ERROR_DEVICE_NOT_CONNECTED;
	}

} // xenon
