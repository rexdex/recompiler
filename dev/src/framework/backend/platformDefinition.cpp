#include "build.h"
#include "xmlReader.h"
#include "internalUtils.h"
#include "internalFile.h"
#include "platformDefinition.h"
#include "platformLauncher.h"
#include "platformDecompilation.h"
#include "platformExports.h"
#include "platformCPU.h"

namespace platform
{

	Definition::Definition()
		: m_interface( nullptr )
		, m_hLibraryModule( NULL )
		, m_exports( nullptr )
	{
	}

	Definition::~Definition()
	{
		if ( m_interface )
		{
			m_interface->Release();
			m_interface = nullptr;
		}

		if (m_exports)
		{
			delete m_exports;
			m_exports = nullptr;
		}

		if ( m_hLibraryModule )
		{
			FreeLibrary( (HMODULE) m_hLibraryModule );
			m_hLibraryModule = NULL;
		}
	}

	std::wstring Definition::GetIncludeDirectory() const
	{
		return m_interface->GetIncludeDirectory();
	}

	const CPU* Definition::FindCPU( const char* name ) const
	{
		for ( const auto* ptr : m_cpu )
			if ( ptr->GetName() == name )
				return ptr;

		return nullptr;
	}

	void Definition::EnumerateImageFormats( std::vector<FileFormat>& outFormats ) const
	{
		const uint32 numFormats = m_interface->GetNumExtensions();
		for ( uint32 i=0; i<numFormats; ++i )
		{
			FileFormat format;
			format.m_extension = m_interface->GetExtension(i);
			format.m_name = m_interface->GetExtensionName(i);

			if ( !format.m_name.empty() && !format.m_extension.empty() )
			{
				outFormats.push_back( format );
			}
		}
	}

	Definition* Definition::Load( ILogOutput& log, const std::wstring& baseDirectory, const std::wstring& platformDLLName )
	{
		// load the DLL
		HMODULE hLib = LoadLibraryW( platformDLLName.c_str() );
		if ( hLib == NULL )
		{
			log.Error( "Platform: Failed to load image loader from DLL '%ls'", platformDLLName.c_str() );
			return nullptr;
		}

		// get the function
		TCreatePlatformInterface func = (TCreatePlatformInterface)GetProcAddress( hLib, "CreateInterface" );
		if ( func == NULL )
		{
			log.Error( "Platform: DLL '%ls' does not contain compatible platform interface", platformDLLName.c_str() );
			FreeLibrary( hLib );
			return nullptr;
		}	

		// create the platform interface
		DecompilationInterface* platformInterface = (*func)(1);
		if ( platformInterface == nullptr )
		{
			log.Error( "Platform: Failed to create platform interface from DLL '%ls'", platformDLLName.c_str() );
			FreeLibrary( hLib );
			return nullptr;
		}

		// get platform name
		std::string platformName = platformInterface->GetName();

		// add the CPUs to local list
		std::vector< const CPU* > cpuInfos;
		const uint32 numCPUs = platformInterface->GetNumCPU();
		for (uint32 i=0; i<numCPUs; ++i)
		{
			const CPU* cpuInfo = platformInterface->GetCPU(i);
			if (cpuInfo)
			{
				log.Log( "Platform: Found definition of cpu '%hs' in platform '%hs' (%u instructions, %u registers)",
					cpuInfo->GetName().c_str(), platformName.c_str(),
					cpuInfo->GetNumInstructions(), cpuInfo->GetNumRegisters() );
				cpuInfos.push_back(cpuInfo);
			}
		}

		// make sure we have CPU definitions
		if (cpuInfos.empty())
		{
			log.Error( "Platform: Platform interface DLL '%ls' does not define any CPUs.", platformDLLName.c_str() );
			FreeLibrary( hLib );
			return nullptr;
		}

		// load export library
		ExportLibrary* exports = nullptr;
		std::string exportLibraryName = platformInterface->GetExportLibrary();
		if (!exportLibraryName.empty())
		{
			const std::wstring exportLibraryPath = baseDirectory + std::wstring(exportLibraryName.begin(), exportLibraryName.end());
			exports = ExportLibrary::LoadFromXML(log, exportLibraryPath);

			// if the export library was declared it must be loaded
			if (!exports)
			{
				log.Error("Platform: Failed to load platform export library from '%ls'.", exportLibraryPath.c_str());
				FreeLibrary(hLib);
				return nullptr;
			}
		}

		// create the platform description
		Definition* platform = new Definition();
		platform->m_name = platformName;
		platform->m_interface = platformInterface;
		platform->m_cpu = cpuInfos;
		platform->m_hLibraryModule = hLib;		
		platform->m_exports = exports;
		log.Log( "Platform: Platform '%hs' loaded", platformName.c_str() );
		return platform;
	}

	const image::Binary* Definition::LoadImageFromFile( ILogOutput& log, const std::wstring& imagePath ) const
	{
		return m_interface->LoadImageFromFile( log, this, imagePath.c_str() );
	}

	const bool Definition::DecodeImage(ILogOutput& log, decoding::Context& decodingContext) const
	{
		if (!m_interface)
			return false;

		return m_interface->DecodeImage(log, decodingContext);
	}

	const bool Definition::ExportCode(ILogOutput& log, decoding::Context& decodingContext, class code::Generator& generator) const
	{
		if (!m_interface)
			return false;

		return m_interface->ExportCode(log, decodingContext, generator);
	}

} // platform