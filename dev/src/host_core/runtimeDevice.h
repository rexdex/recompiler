#pragma once

#include "launcherBase.h"

namespace runtime
{
	enum class DeviceType
	{
		None = 0,
		CPU = 1,
		GPU = 5,
	};

	/// basic hardware device
	class LAUNCHER_API IDevice
	{
	public:
		/// get device type
		virtual DeviceType GetType() const = 0;

		/// get device name
		virtual const char* GetName() const = 0;

	protected:
		IDevice();
		virtual ~IDevice();
	};

} // runtime