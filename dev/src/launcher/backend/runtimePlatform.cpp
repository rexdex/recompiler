#include "build.h"
#include "runtimePlatform.h"

namespace runtime
{
	IPlatform::IPlatform()
	{
	}

	IPlatform::~IPlatform()
	{
	}

	IPlatform* IPlatform::CreatePlatform(const std::wstring& platformDLLPath)
	{
		typedef IPlatform* (__stdcall *TCreatePlatformInterface)(const int callerInterfaceVersion);

		// load the DLL
		HMODULE hLib = LoadLibraryW(platformDLLPath.c_str());
		if (hLib == NULL)
			return nullptr;

		// get the function
		TCreatePlatformInterface func = (TCreatePlatformInterface)GetProcAddress(hLib, "CreateInterface");
		if (func == NULL)
		{
			FreeLibrary(hLib);
			return nullptr;
		}

		// create the platform interface
		auto* platformInterface = (*func)(1);
		if (platformInterface == nullptr)
		{
			FreeLibrary(hLib);
			return nullptr;
		}

		// return platform interface
		return platformInterface;
	}

} // runtime

