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

namespace decoding
{

	//----

	Environment::Environment()
		: m_image(nullptr)
		, m_decodingContext(nullptr)
		, m_platform(nullptr)
	{
	}

	Environment::~Environment()
	{
		delete m_decodingContext;
		m_decodingContext = nullptr;
	}

	bool Environment::IsModified() const
	{
		// get merged flag from decoding context data
		if (m_decodingContext->GetMemoryMap().IsModified())
			return true;

		// not modified
		return false;
	}

	//----

	std::shared_ptr<Environment> Environment::Create(class ILogOutput& log, const platform::Definition* platform, const std::shared_ptr<image::Binary>& loadedBinary, const std::wstring& projectFile)
	{
		std::shared_ptr<Environment> ret(new Environment());

		// create the decoding context
		ret->m_image = loadedBinary;
		ret->m_platform = platform;
		ret->m_platformName = platform->GetName();
		ret->m_projectPath = projectFile;

		// create the decoding context
		ret->m_decodingContext = Context::Create(log, ret->m_image, ret->m_platform);
		if (!ret->m_decodingContext)
		{
			log.Error("Env: Failed to create decoding context for platform '%hs'", ret->m_platform->GetName().c_str());
			return nullptr;
		}

		// initial decoding pass, can fail
		if (!platform->DecodeImage(log, *ret->m_decodingContext))
		{
			log.Error("Env: Fatal error while decoding image for platform '%hs'", ret->m_platform->GetName().c_str());
			return nullptr;
		}

		// save the project
		ret->Save(log);

		// saved!
		return ret;
	}

	std::shared_ptr<Environment> Environment::Load(class ILogOutput& log, const std::wstring& projectPath)
	{
		// open the project file
		const auto loader = CreateFileReader(projectPath.c_str());
		if (!loader)
		{
			log.Error("Env: Unable to open file '%ls'", projectPath.c_str());
			return nullptr;
		}

		// project chunk is required
		FileChunk chunk(*loader, "Project");
		if (!chunk)
		{
			log.Error("Env: Project file '%ls' is invalid", projectPath.c_str());
			return nullptr;
		}

		// load the platform name
		std::string platformName;
		*loader >> platformName;

		// find the image loader
		const platform::Definition* platform = platform::Library::GetInstance().FindPlatform(platformName.c_str());
		if (!platform)
		{
			log.Error("Env: Unable to find platform '%s' used in project '%ls'.", platformName.c_str(), projectPath.c_str());
			return nullptr;
		}

		// setup 
		std::shared_ptr<Environment> ret(new Environment());
		ret->m_platform = platform;
		ret->m_platformName = platform->GetName();
		ret->m_projectPath = projectPath;

		// load image
		{
			ret->m_image.reset(new image::Binary());
			if (!ret->m_image->Load(log, *loader))
			{
				log.Error("Env: Failed to load .image data. Project '%ls' files are corrupted", projectPath.c_str());
				return nullptr;
			}
		}

		// create decoding context
		ret->m_decodingContext = Context::Create(log, ret->m_image, ret->m_platform);
		if (!ret->m_decodingContext)
		{
			log.Error("Env: Failed to create decoding context for platform '%hs'", ret->m_platform->GetName().c_str());
			return nullptr;
		}

		// load memory map
		if (!ret->m_decodingContext->GetMemoryMap().Load(log, *loader))
		{
			log.Error("Env: Failed to load .mem data. Project '%ls' files are corrupted", projectPath.c_str());
			return nullptr;
		}

		// load comment map
		if (!ret->m_decodingContext->GetCommentMap().Load(log, *loader))
		{
			log.Error("Env: Failed to load .comment data. Project '%ls' files are corrupted", projectPath.c_str());
			return nullptr;
		}

		// load name map
		if (!ret->m_decodingContext->GetNameMap().Load(log, *loader))
		{
			log.Error("Env: Failed to load .name data. Project '%ls' files are corrupted", projectPath.c_str());
			return nullptr;
		}

		// load address map
		if (!ret->m_decodingContext->GetAddressMap().Load(log, *loader))
		{
			log.Error("Env: Failed to load .address data. Project '%ls' files are corrupted", projectPath.c_str());
			return nullptr;
		}

		// finished
		log.Log("Image: Loaded project from '%ls'", projectPath.c_str());
		return ret;
	}

	bool Environment::Save(class ILogOutput& log)
	{
		// create the file writer
		std::shared_ptr< IBinaryFileWriter > writer(CreateFileWriter(m_projectPath.c_str(), true));
		if (!writer)
		{
			log.Error("Env: Failed to create output file '%ls'", m_projectPath.c_str());
			return false;
		}

		// save the project chunk is required
		FileChunk chunk(*writer, "Project");
		*writer << m_platformName;

		// save image data, only once
		m_image->Save(log, *writer);

		// save the memory map
		m_decodingContext->GetMemoryMap().Save(log, *writer);

		// save the comment map
		m_decodingContext->GetCommentMap().Save(log, *writer);

		// save the name map
		m_decodingContext->GetNameMap().Save(log, *writer);

		// save the address map
		m_decodingContext->GetAddressMap().Save(log, *writer);

		// env saved
		const auto totalSaved = writer->GetSize();
		log.Log("Env: Project data saved (%1.2f MB total)", (totalSaved / (1024.0f*1024.0f)));
		return true;
	}

} // decoding