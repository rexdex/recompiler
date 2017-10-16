#pragma once

#include "launcherBase.h"

namespace launcher
{
	class Commandline;
}

namespace native
{
	struct Systems;
}

namespace runtime
{
	class Symbols;
	class Image;

	/// devices
	class IDevice;
	class IDeviceCPU;

	/// execution platform interface
	class LAUNCHER_API IPlatform
	{
	public:
		IPlatform();

		/// get human readable name of the platform
		virtual const char* GetName() const = 0;

		/// get number of CPUs in platform
		virtual uint32 GetNumCPUs() const = 0;

		/// get CPU interface
		virtual IDeviceCPU* GetCPU(const uint32 index) const = 0;

		/// get number of devices in platform (note: CPUs are also reported)
		virtual uint32 GetNumDevices() const = 0;

		/// get CPU interface
		virtual IDevice* GetDevice(const uint32 index) const = 0;

		/// initialize platform 
		virtual bool Initialize(const native::Systems& nativeSystems, const launcher::Commandline& commandline, Symbols& symbols) = 0;

		/// shutdown platform
		virtual void Shutdown() = 0;

		/// run image, returns exit code
		virtual int RunImage(const Image& image) = 0;

	protected:
		virtual ~IPlatform();
	};

} // runtime
