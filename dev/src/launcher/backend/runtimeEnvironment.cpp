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
	}

	Environment::~Environment()
	{
		if (m_platform)
		{
			m_platform->Shutdown();
			m_platform = nullptr;
		}
	}

	bool Environment::Initialize(const launcher::Commandline& commandline)
	{
		// verbose output
		if (commandline.HasOption("verbose"))
			GLog.SetVerboseMode(true);

		// get platform path
		const auto platformPath = commandline.GetOptionValueW("platform");
		if (platformPath.empty())
		{
			GLog.Err("Env: Platform not specified, use the -platform= parameter");
			return false;
		}

		// get image path
		auto imagePath = commandline.GetOptionValueW("image");
		if (imagePath.empty())
		{
			GLog.Err("Env: Image path not specified, use the -image= parameter");
			return false;
		}

		//---

		// load the platform
		m_platform = IPlatform::CreatePlatform(platformPath);
		if ( !m_platform )
		{ 
			GLog.Err("Env: Failed to create platform library from '%ls'", platformPath.c_str());
			return false;
		}

		// initialize the platform, this will create any additional devices
		if ( !m_platform->Initialize( m_systems, commandline, m_symbols ) )
		{ 
			GLog.Err("Env: Failed to initialize platform '%hs'", m_platform->GetName());
			return false;
		}

		// list platform devices
		const auto numDevices = m_platform->GetNumDevices();
		GLog.Log("Env: Platform '%hs' initialized, %u devices", m_platform->GetName(), numDevices);
		for (uint32 i = 0; i < numDevices; ++i )
		{
			auto* devicePtr = m_platform->GetDevice(i);
			GLog.Log("Env: Device[%u]: %%hs", i, devicePtr->GetName());
		}

		//-----

		// load and bind the image
		auto image = new Image();
		if ( !image->Load( imagePath ) )
		{
			GLog.Err("Env: Failed to load image from '%ls'", imagePath.c_str());
			delete image;
			return false;
		}

		// bind the image
		if (!image->Bind(m_symbols))
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