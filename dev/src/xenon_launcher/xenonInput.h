#pragma once

#include "xenonKernel.h"
#include "xenonLibNatives.h"

namespace xenon
{

	///----

	/// internal device handler
	class IInputDeviceHandler
	{
	public:
		virtual ~IInputDeviceHandler() {};

		virtual bool Initialize() = 0;
		virtual lib::XResult GetCapabilities(const uint32 user, const uint32 flags, lib::X_INPUT_CAPABILITIES* outCaps) = 0;
		virtual lib::XResult GetState(const uint32 user, lib::X_INPUT_STATE* outState) = 0;
		virtual lib::XResult SetState(const uint32 user, const lib::X_INPUT_VIBRATION* state) = 0;
		virtual lib::XResult GetKeystroke(const uint32 user, const uint32 flags, lib::X_INPUT_KEYSTROKE* outKey) = 0;
	};

	/// input system
	class InputSystem
	{
	public:
		InputSystem(Kernel* kernel, const launcher::Commandline& commandline);
		virtual ~InputSystem();

		// get captured input state
		const lib::XResult GetState(const uint32 user, lib::X_INPUT_STATE* outState);

		// get capabilities
		const lib::XResult GetCapabilities(const uint32 user, const uint32 flags, lib::X_INPUT_CAPABILITIES* outCaps);

		// set device vibration
		const lib::XResult SetState(const uint32 user, const lib::X_INPUT_VIBRATION* state);

		// get keystroke event
		const lib::XResult GetKeystroke(const uint32 user, const uint32 flags, lib::X_INPUT_KEYSTROKE* outKey);

	private:
		typedef std::vector<IInputDeviceHandler*> THandlers;
		THandlers m_handlers;
	};

	///----

} // xenon