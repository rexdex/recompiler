#include "build.h"

#include "..\backend\launcherCommandline.h"
#include "..\backend\runtimeEnvironment.h"

#include "winKernel.h"
#include "winMemory.h"
#include "winFileSystem.h"

int wmain( int argc, const wchar_t** argv )
{
	// image specified ?
	if ( argc < 2 )
	{
		fprintf( stdout, "Recompiler Runtime\n" );
		fprintf( stdout, "(C) 2013-2017 by Rex Dex\n" );
		fprintf( stdout, "\n" );
		fprintf( stdout, "Usage:\n" );
		fprintf( stdout, "  launcher -image=<imagepath> -platform=<platformdll> [-trace] [-verbose] [-tty]\n");
		fprintf( stdout, "\n" );
		fprintf( stdout, "Generic options:\n" );
		fprintf( stdout, "  -verbose\n" );
		fprintf( stdout, "  -trace\n" );
		fprintf( stdout, "  -tty\n");
		return -1;
	}

	// create win systems
	win::FileSystem winFileSystem;
	win::Kernel winKernel;
	win::Memory winMemory;

	// native systems
	native::Systems nativeSystems;
	nativeSystems.m_fileSystem = &winFileSystem;
	nativeSystems.m_kernel = &winKernel;
	nativeSystems.m_memory = &winMemory;

	// parse commandline
	launcher::Commandline commandline(argv, argc);	

	// initialize the environment
	runtime::Environment env(nativeSystems);
	if (!env.Initialize(commandline))
		return -2;

	// start the app
	return env.Run();
}