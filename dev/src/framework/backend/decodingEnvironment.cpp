#include "build.h"
#include "internalUtils.h"
#include "internalFile.h"
#include "xmlReader.h"

#include "image.h"

#include "platformLibrary.h"
#include "platformDefinition.h"

#include "decodingEnvironment.h"
#include "decodingContext.h"
#include "decodingMemoryMap.h"
#include "decodingCommentMap.h"
#include "decodingAddressMap.h"
#include "decodingNameMap.h"
#include "codeGenerator.h"
#include "platformRunner.h"

namespace decoding
{

	//----------------------------------------------------------------------

	const char* GetCompilationModeName(const CompilationMode mode)
	{
		switch (mode)
		{
		case CompilationMode::Debug: return "Debug";
		case CompilationMode::Release: return "Release";
		case CompilationMode::Final: return "Final";
		}

		return "Unknown";
	}

	const char* GetCompilationPlatformName(const CompilationPlatform platform)
	{
		switch (platform)
		{
			case CompilationPlatform::VS2015: return "VS2015";
			case CompilationPlatform::XCode: return "XCode";
		}

		return "Unknown";
	}

	//----

	Environment::Environment()
		: m_image( nullptr )
		, m_decodingContext( nullptr )
		, m_platform( nullptr )
		, m_compilationMode(CompilationMode::Debug)
		, m_compilationPlatform(CompilationPlatform::VS2015)
	{
	}

	Environment::~Environment()
	{
		delete m_decodingContext;
		m_decodingContext = nullptr;

		delete m_image;
		m_image = nullptr;
	}

	bool Environment::IsModified() const
	{
		// get merged flag from decoding context data
		if ( m_decodingContext->GetMemoryMap().IsModified() ) 
			return true;

		// not modified
		return false;
	}

	void Environment::SetCompilationMode(CompilationMode mode)
	{
		m_compilationMode = mode;
	}

	void Environment::SetCompilationPlatform(CompilationPlatform platform)
	{
		m_compilationPlatform = platform;
	}

	//----

	Environment* Environment::Create( class ILogOutput& log, const platform::Definition* platform, const image::Binary* loadedBinary, const std::wstring& projectFile )
	{	
		std::auto_ptr<Environment> ret( new Environment() );

		// create the decoding context
		ret->m_image = loadedBinary;
		ret->m_platform = platform;
		ret->m_platformName = platform->GetName();

		// extract the project file path and project directory
		FixFilePathW( projectFile.c_str(), L"px", ret->m_projectDir, ret->m_projectPath );

		// create the decoding context
		ret->m_decodingContext = Context::Create( log, ret->m_image, ret->m_platform );
		if ( !ret->m_decodingContext )
		{
			log.Error( "Env: Failed to create decoding context for platform '%hs'", ret->m_platform->GetName().c_str() );
			return nullptr;
		}
		
		// initial decoding pass, can fail
		if ( !platform->DecodeImage( log, *ret->m_decodingContext ) )
		{
			log.Error( "Env: Fatal error while decoding image for platform '%hs'", ret->m_platform->GetName().c_str() );
			return nullptr;
		}

		// save the project
		if ( !ret->Save( log, true ) )
		{
			log.Error( "Env: Failed to save project at '%ls'", projectFile.c_str() );
			return nullptr;
		}

		// saved!
		return ret.release();
	}

	static std::wstring AssemblePath( const std::wstring& basePath, const std::string& localPath )
	{
		std::wstring wLocalPath( localPath.begin(), localPath.end() );
		std::wstring ret( basePath );
		ret += L".";
		ret += wLocalPath;
		return ret;
	}

	static std::shared_ptr<IBinaryFileReader> GetElementReader( const std::wstring& basePath, const char* name )
	{
		std::wstring fullPath = AssemblePath( basePath, name );
		if ( FileExists( fullPath.c_str() ) )
			return std::shared_ptr<IBinaryFileReader>( CreateFileReader( fullPath.c_str() ) );

		return nullptr;
	}

	Environment* Environment::Load( class ILogOutput& log, const std::wstring& projectPath )
	{
		// project file ends with "xml"
		// project dir is the directory with the "xml" file
		// prepare saving context
		std::wstring projectDir;
		std::wstring projectFilePath;
		FixFilePathW( projectPath.c_str(), L"px", projectDir, projectFilePath );

		// load the project XML
		std::auto_ptr< xml::Reader > xml( xml::Reader::LoadXML( projectPath ) );
		if ( !xml.get() || !xml->GetError().empty() )
		{
			log.Error( "Env: Failed to load project file '%ls'.", projectPath.c_str() );
			return nullptr;
		}

		// open the autostart block
		if ( !xml->Open( "project" ) )
		{
			log.Error( "Env: Missing project block in project file '%ls'.", projectPath.c_str() );
			return nullptr;
		}

		// get the platform name
		std::string platformName = xml->GetAttribute( "platform" );
		if ( platformName.empty() )
		{
			log.Error( "Env: Missing name of the platform in project '%ls'.", projectPath.c_str() );
			return nullptr;
		}

		// find the image loader
		const platform::Definition* platform = platform::Library::GetInstance().FindPlatform( platformName.c_str() );
		if ( !platform )
		{
			log.Error( "Env: Unable to find platform '%s' used in project '%ls'.", platformName.c_str(), projectPath.c_str() );
			return nullptr;
		}

		// setup 
		std::auto_ptr<Environment> ret( new Environment() );
		ret->m_platform = platform;
		ret->m_platformName = platform->GetName();
		ret->m_projectPath = projectFilePath;
		ret->m_projectDir = projectDir;

		// load image
		{
			auto imagePtr = new image::Binary();
			ret->m_image = imagePtr;

			auto loader = GetElementReader( projectFilePath, "image" );
			if ( !loader )
			{
				log.Error( "Env: Missing the .image data. Project '%ls' files are corrupted", projectPath.c_str() );
				return nullptr;
			}

			if ( !imagePtr->Load( log, *loader ) )
			{
				log.Error( "Env: Failed to load .image data. Project '%ls' files are corrupted", projectPath.c_str() );
				return nullptr;
			}
		}

		// create decoding context
		ret->m_decodingContext = Context::Create( log, ret->m_image, ret->m_platform );
		if ( !ret->m_decodingContext )
		{
			log.Error( "Env: Failed to create decoding context for platform '%hs'", ret->m_platform->GetName().c_str() );
			return nullptr;
		}

		// load memory map
		{
			auto loader = GetElementReader( projectFilePath, "mem" );
			if ( !loader )
			{
				log.Error( "Env: Missing the .mem data. Project '%ls' files are corrupted", projectPath.c_str() );
				return nullptr;
			}

			if ( !ret->m_decodingContext->GetMemoryMap().Load( log, *loader ) )
			{
				log.Error( "Env: Failed to load .mem data. Project '%ls' files are corrupted", projectPath.c_str() );
				return nullptr;
			}
		}

		// load comment map
		{
			auto loader = GetElementReader( projectFilePath, "comment" );
			if ( !loader )
			{
				log.Error( "Env: Missing the .comment data. Project '%ls' files are corrupted", projectPath.c_str() );
				return nullptr;
			}

			if ( !ret->m_decodingContext->GetCommentMap().Load( log, *loader ) )
			{
				log.Error( "Env: Failed to load .comment data. Project '%ls' files are corrupted", projectPath.c_str() );
				return nullptr;
			}
		}

		// load address map
		{
			auto loader = GetElementReader( projectFilePath, "address" );
			if ( !loader )
			{
				log.Error( "Env: Missing the .address data. Project '%ls' files are corrupted", projectPath.c_str() );
				return nullptr;
			}

			if ( !ret->m_decodingContext->GetAddressMap().Load( log, *loader ) )
			{
				log.Error( "Env: Failed to load .address data. Project '%ls' files are corrupted", projectPath.c_str() );
				return nullptr;
			}
		}

		// load name map
		{
			auto loader = GetElementReader( projectFilePath, "name" );
			if ( !loader )
			{
				log.Error( "Env: Missing the .name data. Project '%ls' files are corrupted", projectPath.c_str() );
				return nullptr;
			}

			if ( !ret->m_decodingContext->GetNameMap().Load( log, *loader ) )
			{
				log.Error( "Env: Failed to load .name data. Project '%ls' files are corrupted", projectPath.c_str() );
				return nullptr;
			}
		}

		// finished
		log.Log( "Image: Loaded project from '%ls' for decoding", projectFilePath.c_str() );
		return ret.release();
	}

	bool Environment::Save( class ILogOutput& log, const bool forceAll /*= false*/ )
	{
		// memory counter
		uint64 totalSavedMem = 0;

		// save image data, only once
		{
			const std::wstring path = AssemblePath( m_projectPath, "image" );
			if ( !FileExists( path.c_str() ) || forceAll )
			{
				log.Log( "Env: Saving image data" );

				std::shared_ptr< IBinaryFileWriter > writer( CreateFileWriter( path.c_str(), true ) );
				if ( !writer )
				{
					log.Error( "Env: Failed to create output file '%ls'", path.c_str() );
					return false;
				}

				if ( !m_image->Save( log, *writer ) )
					return false;

				totalSavedMem += writer->GetSize();
			}
		}

		// save the memory map
		{
			const std::wstring path = AssemblePath( m_projectPath, "mem" );
			if ( !FileExists( path.c_str() ) || m_decodingContext->GetMemoryMap().IsModified() || forceAll )
			{
				log.Log( "Env: Saving memory map" );

				std::shared_ptr< IBinaryFileWriter > writer( CreateFileWriter( path.c_str(), true ) );
				if ( !writer )
				{
					log.Error( "Env: Failed to create output file '%ls'", path.c_str() );
					return false;
				}

				if ( !m_decodingContext->GetMemoryMap().Save( log, *writer ) )
					return false;

				totalSavedMem += writer->GetSize();
			}
		}

		// save the comment map
		{
			const std::wstring path = AssemblePath( m_projectPath, "comment" );
			if ( !FileExists( path.c_str() ) || m_decodingContext->GetCommentMap().IsModified() || forceAll )
			{
				log.Log( "Env: Saving comment map" );

				std::shared_ptr< IBinaryFileWriter > writer( CreateFileWriter( path.c_str(), true ) );
				if ( !writer )
				{
					log.Error( "Env: Failed to create output file '%ls'", path.c_str() );
					return false;
				}

				if ( !m_decodingContext->GetCommentMap().Save( log, *writer ) )
					return false;

				totalSavedMem += writer->GetSize();
			}
		}

		// save the name map
		{
			const std::wstring path = AssemblePath( m_projectPath, "name" );
			if ( !FileExists( path.c_str() ) || m_decodingContext->GetNameMap().IsModified() || forceAll )
			{
				log.Log( "Env: Saving name map" );

				std::shared_ptr< IBinaryFileWriter > writer( CreateFileWriter( path.c_str(), true ) );
				if ( !writer )
				{
					log.Error( "Env: Failed to create output file '%ls'", path.c_str() );
					return false;
				}

				if ( !m_decodingContext->GetNameMap().Save( log, *writer ) )
					return false;

				totalSavedMem += writer->GetSize();
			}
		}

		// save the address map
		{
			const std::wstring path = AssemblePath( m_projectPath, "address" );
			if ( !FileExists( path.c_str() ) || m_decodingContext->GetAddressMap().IsModified() || forceAll )
			{
				log.Log( "Env: Saving address map" );

				std::shared_ptr< IBinaryFileWriter > writer( CreateFileWriter( path.c_str(), true ) );
				if ( !writer )
				{
					log.Error( "Env: Failed to create output file '%ls'", path.c_str() );
					return false;
				}

				if ( !m_decodingContext->GetAddressMap().Save( log, *writer ) )
					return false;

				totalSavedMem += writer->GetSize();
			}
		}

		// create the output XML
		{
			std::string outXML = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
			outXML += "<project platform=\"";
			outXML += m_platformName;
			outXML += "\">\n";
			outXML += "</project>\n";

			// save the project file
			if ( !SaveStringToFileA( m_projectPath.c_str(), outXML ) )
			{
				log.Error( "Env: Failed to save project XML" );
				return false;
			}
		}

		// env saved
		log.Log( "Env: Project data saved (%1.2f MB total)", (totalSavedMem / (1024.0f*1024.0f)) );
		return true;
	}

	bool Environment::CompileCode(class ILogOutput& log) const
	{
		const std::string profileName = GetCompilationModeName(m_compilationMode);
		const std::string platformName = GetCompilationPlatformName(m_compilationPlatform);

		// get the code output path
		std::wstring codeDir = GetProjectDir();
		codeDir += L"code\\";
		codeDir += std::wstring(profileName.begin(), profileName.end());
		codeDir += L".";
		codeDir += std::wstring(platformName.begin(), platformName.end());
		codeDir += L"\\";

		CScopedTask task( log, "Compiling project" );
		const auto includeDirectory = m_platform->GetIncludeDirectory();
		code::Generator generator( log, includeDirectory, m_projectPath, codeDir, m_compilationMode, m_compilationPlatform, m_image->GetEntryAddress() );

		// generate code files
		if ( !m_platform->ExportCode( log, *m_decodingContext, generator ) )
		{
			log.Error("Compile: Failed to generate code for selected code profile and platform");
			return false;
		}

		// finish code generation
		std::vector< code::Task > tasks;
		generator.FinishGeneration(tasks);
		if (tasks.empty())
		{
			log.Log("Compile: Nothing to compile!");
			return true;
		}

		// compile the code
		for ( const auto& task : tasks )
		{

			log.SetTaskName("%hs...", task.m_name.c_str());
			log.Log( "Starting '%hs'...", task.m_name.c_str());

			platform::ExternalAppEnvironment runner( log, "Compile", true );
			const int ret = runner.Execute(task.m_command, task.m_arguments);

			if (ret != 0)
				break;
		}

		// done
		log.Flush();
		log.Log("Compile: Compilation finished!");
		return true;
	}

#if 0
	bool Environment::RunCode( class ILogOutput& log, class ILogOutput& appLog, const platform::CodeProfile* codeProfile, const platform::CompilerDefinition* compiler, const EnvironmentRunContext& context ) const
	{
#if 0
		// profile must exit
		if ( !codeProfile )
		{
			log.Error( "Compile: Cannot run code without compilation profile" );
			return false;
		}

		// compile definition must exist
		if ( !compiler )
		{
			log.Error( "Compile: Cannot run code without compiler definition" );
			return false;
		}

		// get the code output path
		const std::string profileName = codeProfile->GetName();
		std::wstring appModule = Environment::GetInstance().GetProjectDir();
		appModule += L"bin\\";
		appModule += std::wstring( profileName.begin(), profileName.end() );
		appModule += L".dll";

		// get runner
		std::wstring appRunner = GetAppDirectoryPath();
		const std::string appLauncherName = codeProfile->GetLauncherApp();
		appRunner += std::wstring( appLauncherName.begin(), appLauncherName.end() );

		// format arguments
		std::wstring args;
		if ( context.m_traceInstructions )
		{
			log.Log( "Run: Instruction trace is enabled" );
			args += L"-trace ";

			if ( !context.m_traceDirectory.empty() )
			{
				args += L"-tracepath \"";
				args += context.m_traceDirectory;
				args += L"\" ";
			}
		}

		// verbose log
		if ( context.m_verboseLog )
		{
			log.Log( "Run: Verbose logging is enabled" );
			args += L"-verbose ";
		}

		// TODO: specify trace output file

		// devkit directory (HACKED)
		if ( !context.m_devkitDirectory.empty() )
		{
			args += L"-devkit ";
			args += L"\"";
			args += context.m_devkitDirectory.c_str();
			args += L"\" ";
		}

		// dvd directory (HACKED)
		if ( !context.m_dvdDirectory.empty() )
		{
			args += L"-dvd ";
			args += L"\"";
			args += context.m_dvdDirectory.c_str();
			args += L"\" ";
		}

		// application to run
		args += appModule;

		// prepare env
		{		
			CScopedTask task( log, "Running project" );
			log.SetTaskName("Running application...");
			platform::ExternalAppEnvironment appEnv( appLog, "Run", false );
			if ( 0 != appEnv.Execute( appRunner, args ) )
			{
				log.Error( "Run: Application execution finished with errors" );
				return false;
			}
			else
			{
				log.Log( "Run: Application finished without errors" );
			}
		}

		// done
		return true;
#else
		return false;
#endif
	}
#endif

} // decoding