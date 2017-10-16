#pragma once

#include "launcherBase.h"
#include "runtimeSymbols.h"
#include "native.h"

namespace launcher
{
	class Commandline;
}

namespace runtime
{
	class IPlatform;
	class Image;

	/// runtime environment - image + platform
	class LAUNCHER_API Environment
	{
	public:
		Environment(const native::Systems& nativeSystems);
		~Environment();

		/// initialize environment
		bool Initialize(const launcher::Commandline& commandline);

		/// run until done, returns application exit code
		int Run();

	private:
		IPlatform*	m_platform;
		Image*		m_image;

		Symbols*	m_symbols;

		native::Systems	m_systems;
	};

} // runtime