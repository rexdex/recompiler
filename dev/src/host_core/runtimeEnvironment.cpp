#include "build.h"
#include "runtimeEnvironment.h"
#include "runtimePlatform.h"
#include "runtimeImage.h"
#include "runtimeSymbols.h"
#include "runtimeDevice.h"

#include "launcherCommandline.h"

namespace runtime
{

	Environment::Environment(const native::Systems& nativeSystems)
		: m_platform(nullptr)
		, m_image(nullptr)
		, m_systems(nativeSystems)
	{
		m_symbols = new Symbols();
	}

	Environment::~Environment()
	{
		if (m_platform)
		{
			m_platform->Shutdown();
			m_platform = nullptr;
		}

		delete m_symbols;
		m_symbols = nullptr;
	}

	extern IPlatform* GetSimulatedPlatform(); // implemented by the platform launcher

	bool Environment::Initialize(const launcher::Commandline& commandline)
	{
		// no arguments
		if (!commandline.HasOption("image"))
		{
			fprintf(stdout, "Recompiler Runtime\n");
			fprintf(stdout, "(C) 2013-2017 by Rex Dex\n");
			fprintf(stdout, "\n");
			fprintf(stdout, "Usage:\n");
			fprintf(stdout, "  launcher -fsroot=<path> -image=<imagepath> [-debug|-trace] [-verbose] [-tty]\n");
			fprintf(stdout, "\n");
			fprintf(stdout, "Required option:\n");
			fprintf(stdout, "  -fsroot=<path> - path to the root file system for the application\n");
			fprintf(stdout, "  -image=<imagepath> - path to recompiled image to run\n");
			fprintf(stdout, "\n");
			fprintf(stdout, "Generic options:\n");
			fprintf(stdout, "  -verbose - display all logging (not only errors/warnings)\n");
			fprintf(stdout, "  -debug - allow debugging via the network\n");
			fprintf(stdout, "  -trace - trace all executed instructions\n");
			fprintf(stdout, "  -tty - create additional terminal window\n");
			return false;
		}

		// verbose output
		if (commandline.HasOption("verbose"))
			GLog.SetVerboseMode(true);

		// get image path
		auto imagePath = commandline.GetOptionValueW("image");
		if (imagePath.empty())
		{
			GLog.Err("Env: Image path not specified, use the -image= parameter");
			return false;
		}

		//---

		// load the platform
		m_platform = GetSimulatedPlatform();

		// initialize the platform, this will create any additional devices
		if (!m_platform->Initialize(m_systems, commandline, *m_symbols))
		{
			GLog.Err("Env: Failed to initialize platform '%hs'", m_platform->GetName());
			return false;
		}

		// list platform devices
		const auto numDevices = m_platform->GetNumDevices();
		GLog.Log("Env: Platform '%hs' initialized, %u devices", m_platform->GetName(), numDevices);
		for (uint32 i = 0; i < numDevices; ++i)
		{
			auto* devicePtr = m_platform->GetDevice(i);
			GLog.Log("Env: Device[%u]: %%hs", i, devicePtr->GetName());
		}

		//-----

		// load and bind the image
		auto image = new Image();
		if (!image->Load(imagePath))
		{
			GLog.Err("Env: Failed to load image from '%ls'", imagePath.c_str());
			delete image;
			return false;
		}

		// bind the image
		if (!image->Bind(*m_symbols))
		{
			GLog.Err("Env: Failed to bind image '%ls' with the platform. Image will not function properly.", imagePath.c_str());
			delete image;
			return false;
		}

		//---

		// ready
		m_image = image;
		return true;
	}

	int Environment::Run()
	{
		return m_platform->RunImage(*m_image);
	}

} // runtime