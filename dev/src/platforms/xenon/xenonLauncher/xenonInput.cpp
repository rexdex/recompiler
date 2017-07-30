#include "build.h"
#include "xenonKernel.h"
#include "xenonInput.h"

#include <Xinput.h>

#pragma comment (lib, "xinput.lib")

namespace xenon
{

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

		virtual xnative::XResult GetCapabilities(const uint32 user, const uint32 flags, xnative::X_INPUT_CAPABILITIES* outCaps) override final
		{
			XINPUT_CAPABILITIES caps;
			memset(&caps, 0, sizeof(caps));
			auto ret = XInputGetCapabilities(user, flags, &caps);
			outCaps->flags.Set(caps.Flags);
			outCaps->gamepad.buttons.Set(caps.Gamepad.wButtons);
			outCaps->gamepad.left_trigger.Set(caps.Gamepad.bLeftTrigger);
			outCaps->gamepad.right_trigger.Set(caps.Gamepad.bRightTrigger);
			outCaps->gamepad.thumb_lx.Set(caps.Gamepad.sThumbLX);
			outCaps->gamepad.thumb_ly.Set(caps.Gamepad.sThumbLY);
			outCaps->gamepad.thumb_rx.Set(caps.Gamepad.sThumbRX);
			outCaps->gamepad.thumb_ry.Set(caps.Gamepad.sThumbRY);
			outCaps->sub_type.Set(caps.SubType);
			outCaps->type.Set(caps.Type);
			outCaps->vibration.left_motor_speed.Set(caps.Vibration.wLeftMotorSpeed);
			outCaps->vibration.right_motor_speed.Set(caps.Vibration.wRightMotorSpeed);
			return (xnative::XResult)ret;
		}

		virtual xnative::XResult GetState(const uint32 user, xnative::X_INPUT_STATE* outState) override final
		{
			XINPUT_STATE state;
			memset(&state, 0, sizeof(state));
			auto ret = XInputGetState(user, &state);
			outState->gamepad.buttons.Set(state.Gamepad.wButtons);
			outState->gamepad.left_trigger.Set(state.Gamepad.bLeftTrigger);
			outState->gamepad.right_trigger.Set(state.Gamepad.bRightTrigger);
			outState->gamepad.thumb_lx.Set(state.Gamepad.sThumbLX);
			outState->gamepad.thumb_ly.Set(state.Gamepad.sThumbLY);
			outState->gamepad.thumb_rx.Set(state.Gamepad.sThumbRX);
			outState->gamepad.thumb_ry.Set(state.Gamepad.sThumbRY);
			outState->packet_number.Set(state.dwPacketNumber);
			return (xnative::XResult)ret;
		}

		virtual xnative::XResult SetState(const uint32 user, const xnative::X_INPUT_VIBRATION* state) override final
		{
			XINPUT_VIBRATION vib;
			vib.wLeftMotorSpeed = state->left_motor_speed.Get();
			vib.wRightMotorSpeed = state->right_motor_speed.Get();
			auto ret = XInputSetState(user, &vib);
			return (xnative::XResult)ret;
		}

		virtual xnative::XResult GetKeystroke(const uint32 user, const uint32 flags, xnative::X_INPUT_KEYSTROKE* outKey) override final
		{
			XINPUT_KEYSTROKE stroke;
			auto ret = XInputGetKeystroke(user, flags, &stroke);
			outKey->flags.Set(stroke.Flags);
			outKey->hid_code.Set(stroke.HidCode);
			outKey->unicode.Set(stroke.Unicode);
			outKey->user_index.Set(stroke.UserIndex);
			outKey->virtual_key.Set(stroke.VirtualKey);
			return (xnative::XResult)ret;
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

	const xnative::XResult InputSystem::GetState(const uint32 user, xnative::X_INPUT_STATE* outState)
	{
		for (auto* drv : m_handlers)
		{
			auto ret = drv->GetState(user, outState);
			if (ret != xnative::X_ERROR_DEVICE_NOT_CONNECTED)
				return ret;
		}

		return xnative::X_ERROR_DEVICE_NOT_CONNECTED;
	}

	const xnative::XResult InputSystem::GetCapabilities(const uint32 user, const uint32 flags, xnative::X_INPUT_CAPABILITIES* outCaps)
	{
		for (auto* drv : m_handlers)
		{
			auto ret = drv->GetCapabilities(user, flags, outCaps);
			if (ret != xnative::X_ERROR_DEVICE_NOT_CONNECTED)
				return ret;
		}

		return xnative::X_ERROR_DEVICE_NOT_CONNECTED;
	}

	const xnative::XResult InputSystem::SetState(const uint32 user, const xnative::X_INPUT_VIBRATION* state)
	{
		for (auto* drv : m_handlers)
		{
			auto ret = drv->SetState(user, state);
			if (ret != xnative::X_ERROR_DEVICE_NOT_CONNECTED)
				return ret;
		}

		return xnative::X_ERROR_DEVICE_NOT_CONNECTED;
	}

	const xnative::XResult InputSystem::GetKeystroke(const uint32 user, const uint32 flags, xnative::X_INPUT_KEYSTROKE* outKey)
	{
		for (auto* drv : m_handlers)
		{
			auto ret = drv->GetKeystroke(user, flags, outKey);
			if (ret != xnative::X_ERROR_DEVICE_NOT_CONNECTED)
				return ret;
		}

		return xnative::X_ERROR_DEVICE_NOT_CONNECTED;
	}

} // xenon
