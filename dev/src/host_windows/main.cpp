#include "build.h"

#include "../host_core/launcherCommandline.h"
#include "../host_core/runtimeEnvironment.h"

#include "winKernel.h"
#include "winMemory.h"
#include "winFileSystem.h"

int main(int argc, const char** argv)
{
	// create win systems
	win::FileSystem winFileSystem;
	win::Kernel winKernel;
	win::Memory* winMemory = new win::Memory();

	// native systems
	native::Systems nativeSystems;
	nativeSystems.m_fileSystem = &winFileSystem;
	nativeSystems.m_kernel = &winKernel;
	nativeSystems.m_memory = winMemory;

	// parse commandline
	launcher::Commandline commandline(argv, argc);

	// initialize the environment
	runtime::Environment env(nativeSystems);
	if (!env.Initialize(commandline))
		return -2;

	// start the app
	return env.Run();
}