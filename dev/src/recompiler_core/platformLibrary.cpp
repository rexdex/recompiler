#include "build.h"
#include "platformLibrary.h"
#include "platformDefinition.h"
#include "internalUtils.h"
#include "xmlReader.h"

namespace platform
{

	Library::Library()
	{
	}

	Library::~Library()
	{
	}

	Library& Library::GetInstance()
	{
		static Library theInstance;
		return theInstance;
	}

	const Definition* Library::FindPlatform(const char* name) const
	{
		// linear search
		for (auto it : m_platformLibrary)
			if (it->GetName() == name)
				return it;

		// not found
		return nullptr;
	}

	const std::vector< Definition* > Library::FindCompatiblePlatforms(const std::wstring& filePath) const
	{
		std::vector< Definition* > ret;

		const wchar_t* fileExt = wcsrchr(filePath.c_str(), '.');
		if (nullptr != fileExt)
		{
			fileExt += 1; // skip the '.'

			const std::wstring unicodeExt(fileExt);
			const std::string ansiExt(unicodeExt.begin(), unicodeExt.end());

			// check supported extensions
			for (auto ptr : m_platformLibrary)
			{
				std::vector< FileFormat > fileFormats;
				ptr->EnumerateImageFormats(fileFormats);

				bool isExtensionSupported = false;
				for (const auto jt : fileFormats)
				{
					if (jt.m_extension == ansiExt)
					{
						isExtensionSupported = true;
						break;
					}
				}

				// TODO: add more tests (like the actual header read, etc)
				if (isExtensionSupported)
					ret.push_back(ptr);
			}
		}

		return ret;
	}

	void Library::EnumerateImageFormats(std::vector<FileFormat>& outFormats) const
	{
		for (auto it = m_platformLibrary.begin(); it != m_platformLibrary.end(); ++it)
		{
			std::vector< FileFormat > fileFormats;
			(*it)->EnumerateImageFormats(fileFormats);

			for (auto jt : fileFormats)
			{
				// already on list?
				bool alreadyDefined = false;
				for (auto& kt : outFormats)
				{
					if (kt.m_extension == jt.m_extension)
					{
						alreadyDefined = true;
						break;
					}
				}

				// new format
				if (!alreadyDefined)
					outFormats.push_back(jt);
			}
		}
	}

	void Library::Close()
	{
		DeleteVector(m_platformLibrary);
	}

	bool Library::Initialize(class ILogOutput& log)
	{
		// load the platform list
		const auto appPath = GetAppDirectoryPath();
		const auto platformListPath = appPath + L"platforms.xml";
		auto platformList = xml::Reader::LoadXML(platformListPath);
		if (!platformList)
		{
			log.Error("Libary: Missing platforms.xml in application directory.");
			return false;
		}

		// load platforms
		if (platformList->Open("library"))
		{
			while (platformList->Iterate("platform"))
			{
				const auto platformLibraryName = platformList->GetAttribute("name");
				if (!platformLibraryName.empty())
				{
					auto platform = Definition::Load(log, appPath, platformLibraryName);
					if (platform)
					{
						log.Log("Library: Registered platform '%hs' loaded from '%hs'", platform->GetName().c_str(), platformLibraryName.c_str());
						m_platformLibrary.push_back(platform);
					}
				}
			}
		}

		// no platforms found
		if (m_platformLibrary.empty())
		{
			log.Error("Library: No decoding platforms found. Tool is unusable.");
			return false;
		}

		// initialized
		return true;
	}

	//---------------------------------------------------------------------------

} // platform