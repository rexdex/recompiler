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
		virtual xnative::XResult GetCapabilities(const uint32 user, const uint32 flags, xnative::X_INPUT_CAPABILITIES* outCaps) = 0;
		virtual xnative::XResult GetState(const uint32 user, xnative::X_INPUT_STATE* outState) = 0;
		virtual xnative::XResult SetState(const uint32 user, const xnative::X_INPUT_VIBRATION* state) = 0;
		virtual xnative::XResult GetKeystroke(const uint32 user, const uint32 flags, xnative::X_INPUT_KEYSTROKE* outKey) = 0;
	};

	/// input system
	class InputSystem
	{
	public:
		InputSystem(Kernel* kernel, const launcher::Commandline& commandline);
		virtual ~InputSystem();

		// get captured input state
		const xnative::XResult GetState(const uint32 user, xnative::X_INPUT_STATE* outState);

		// get capabilities
		const xnative::XResult GetCapabilities(const uint32 user, const uint32 flags, xnative::X_INPUT_CAPABILITIES* outCaps);

		// set device vibration
		const xnative::XResult SetState(const uint32 user, const xnative::X_INPUT_VIBRATION* state);

		// get keystroke event
		const xnative::XResult GetKeystroke(const uint32 user, const uint32 flags, xnative::X_INPUT_KEYSTROKE* outKey);

	private:
		typedef std::vector<IInputDeviceHandler*> THandlers;
		THandlers m_handlers;
	};

	///----

} // xenon