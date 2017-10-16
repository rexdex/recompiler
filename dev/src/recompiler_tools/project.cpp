#include "build.h"
#include "project.h"
#include "projectImage.h"

#include "../recompiler_core/platformDefinition.h"
#include "../recompiler_core/platformLibrary.h"
#include "../recompiler_core/internalFile.h"

namespace tools
{

	//---------------------------------------------------------------------------

	ProjectSettings::ProjectSettings()
	{}

	static inline IBinaryFileWriter &operator<<(IBinaryFileWriter& file, const ProjectSettings& s)
	{
		file << s.m_customRoot;
		return file;
	}

	static inline IBinaryFileReader &operator >> (IBinaryFileReader& file, ProjectSettings& s)
	{
		file >> s.m_customRoot;
		return file;
	}

	//---------------------------------------------------------------------------

	Project::Project(const platform::Definition* platform, const wxString& projectFilePath)
		: m_platform(platform)
		, m_projectPath(projectFilePath)
		, m_projectDir(wxFileName(projectFilePath).GetPath())
		, m_isModified(false)
	{
		m_projectDir += wxFileName::GetPathSeparator();
	}

	Project::~Project()
	{}

	bool Project::IsModified() const
	{
		return m_isModified;
	}

	void Project::MarkAsModified()
	{
		m_isModified = true;
	}

	void Project::AddImage(const std::shared_ptr<ProjectImage>& image)
	{
		if (image)
		{
			m_images.push_back(image);
			MarkAsModified();
		}
	}

	void Project::RemoveImage(const std::shared_ptr<ProjectImage>& image)
	{
		std::remove(m_images.begin(), m_images.end(), image);
		MarkAsModified();
	}

	bool Project::Save(ILogOutput& log)
	{
		// open file
		auto writer = CreateFileWriter(m_projectPath, true);
		if (!writer.get())
			return false;

		FileChunk chunk(*writer, "Project");

		// save platform name
		*writer << m_platform->GetName();

		// save project settings
		{
			FileChunk chunk(*writer, "Settings");
			*writer << m_settings;
		}

		// save image references
		{
			FileChunk chunk(*writer, "Images");

			*writer << (uint32)m_images.size();
			for (const auto& img : m_images)
			{
				if (!img->Save(log, *writer))
				{
					log.Error("Project: Failed to save setup for project image");
					return false;
				}
			}
		}

		// saved
		log.Log("Project: Saved project '%ls'", m_projectPath.c_str());
		m_isModified = false;
		return true;
	}

	/*std::shared_ptr<Project> Project::LoadImageFile(ILogOutput& log, const std::wstring& imagePath, const std::wstring& projectPath)
	{
		// get the best platform for given image extension
		auto platforms = platform::Library::GetInstance().FindCompatiblePlatforms(imagePath);
		if (platforms.empty())
		{
			log.Error("Project: No decoding platform found that can load image '%ls'", imagePath.c_str());
			return nullptr;
		}

		// Use the first platform, TODO: selection
		const auto* platform = platforms[0];

		// importing image
		CScopedTask task(log, "Importing image");

		// load the image
		const auto mainImage = platform->LoadImageFromFile(log, imagePath);
		if (!mainImage)
		{
			log.Error("Project: Failed to load image from '%ls', platform '%hs'", imagePath.c_str(), platform->GetName().c_str());
			return nullptr;
		}

		// create decoding context
		auto decodingContext = decoding::Context::Create(log, mainImage, platform);
		if (!decodingContext)
		{
			log.Error("Project: Failed to create decoding context for image '%ls', platform '%hs'", imagePath.c_str(), platform->GetName().c_str());
			return nullptr;
		}

		// create environment
		auto env = decoding::Environment::Create(log, platform, mainImage, projectPath);
		if (!env)
		{
			log.Error("Project: Failed to create decoding environment for image '%ls', platform '%hs'", imagePath.c_str(), platform->GetName().c_str());
			return nullptr;
		}

		// cache the image and decoding context
		std::shared_ptr<Project> project(new Project());
		project->m_env = env;

		// initialize address history
		const uint32 entryAddress = mainImage->GetEntryAddress();
		project->m_addressHistory.Reset(entryAddress);

		// return created project
		return project;
	}*/

	std::shared_ptr<Project> Project::LoadProject(ILogOutput& log, const std::wstring& projectPath)
	{
		// open the file
		auto reader = CreateFileReader(projectPath.c_str());
		if (!reader)
		{
			log.Error("Project: Failed to open project file '%ls'", projectPath.c_str());
			return nullptr;
		}

		// start the project chunk
		FileChunk chunk(*reader, "Project");
		if (!chunk)
		{
			log.Error("Project: Failed to load project file '%ls'. File is corrupted.", projectPath.c_str());
			return nullptr;
		}

		// load platform name
		std::string platformName;
		*reader >> platformName;
		log.Log("Project: Using platform '%hs'", platformName.c_str());

		// find the platform definition
		auto* platformDefinition = platform::Library::GetInstance().FindPlatform(platformName.c_str());
		if (!platformDefinition)
		{
			log.Error("Project: Platform '%hs' is not avaiable, unable to load project", platformName.c_str());
			return nullptr;
		}

		// create output project
		std::shared_ptr<Project> ret(new Project(platformDefinition, projectPath));

		// load project settings
		{
			FileChunk chunk(*reader, "Settings");
			if (chunk)
				*reader >> ret->m_settings;
		}

		// load images
		{
			FileChunk chunk(*reader, "Images");
			if (chunk)
			{
				uint32 numImages = 0;
				*reader >> numImages;
				for (uint32 i = 0; i < numImages; ++i)
				{
					// load the image
					std::shared_ptr<ProjectImage> img(new ProjectImage(ret.get()));
					if (img->Load(log, *reader))
					{
						ret->m_images.push_back(img);
					}
					else
					{
						log.Warn("Project: Failed to load an image referenced by the project. It may not run.");
					}
				}
			}
		}

		// return created project
		return ret;
	}

	std::shared_ptr<Project> Project::CreateProject(ILogOutput& log, const std::wstring& projectPath, const platform::Definition* platformDefinition)
	{
		// invalid platform
		if (!platformDefinition)
		{
			log.Error("Project: Invalid platform");
			return nullptr;
		}

		// create the project file
		std::shared_ptr<Project> ret(new Project(platformDefinition, projectPath));
		if (!ret->Save(log))
		{
			log.Error("Project: Failed to save initial project file. Check disk for write permissions.");
			return nullptr;
		}

		// return the created project
		return ret;
	}

} // tools