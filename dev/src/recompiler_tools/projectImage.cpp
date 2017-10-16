#include "build.h"
#include "project.h"
#include "projectImage.h"
#include "projectAddressHistory.h"

#include "../recompiler_core/image.h"
#include "../recompiler_core/decodingEnvironment.h"
#include "../recompiler_core/decodingContext.h"
#include "../recompiler_core/decodingMemoryMap.h"
#include "../recompiler_core/platformLibrary.h"
#include "../recompiler_core/platformDefinition.h"
#include "../recompiler_core/internalFile.h"
#include "../recompiler_core/internalUtils.h"
#include "buildDialog.h"
#include "../recompiler_core/codeGenerator.h"

namespace tools
{
	//--

	ProjectImageSettings::ProjectImageSettings()
		: m_imageType(ProjectImageType::Application)
		, m_recompilationEnabled(true)
		, m_emulationEnabled(true)
		, m_recompilationMode(ProjectRecompilationMode::Debug)
		, m_customLoadAddress(0)
		, m_customEntryAddress(0)
		, m_ignoreRelocations(false)
	{}

	const uint64 ProjectImageSettings::CalcHash() const
	{
		CRC64 ret;
		ret << (uint8)m_imageType;
		ret << m_recompilationEnabled;
		ret << m_emulationEnabled;
		ret << m_recompilationTool;
		ret << (uint8)m_recompilationMode;
		ret << m_customLoadAddress;
		ret << m_customEntryAddress;
		ret << m_ignoreRelocations;
		return ret.GetCRC();
	}

	static inline IBinaryFileWriter &operator<<(IBinaryFileWriter& file, const ProjectImageSettings& s)
	{
		file << (uint8)s.m_imageType;
		file << s.m_recompilationEnabled;
		file << s.m_emulationEnabled;
		file << s.m_recompilationTool;
		file << (uint8)s.m_recompilationMode;
		file << s.m_customLoadAddress;
		file << s.m_customEntryAddress;
		file << s.m_ignoreRelocations;
		return file;
	}

	static inline IBinaryFileReader &operator >> (IBinaryFileReader& file, ProjectImageSettings& s)
	{
		file >> (uint8&)s.m_imageType;
		file >> s.m_recompilationEnabled;
		file >> s.m_emulationEnabled;
		file >> s.m_recompilationTool;
		file >> (uint8&)s.m_recompilationMode;
		file >> s.m_customLoadAddress;
		file >> s.m_customEntryAddress;
		file >> s.m_ignoreRelocations;
		return file;
	}

	//--

	ProjectImage::ProjectImage(Project* project)
		: m_owner(project)
	{}

	const wxString ProjectImage::GetFullPathToImage() const
	{
		wxString ret = m_owner->GetProjectDirectory();
		ret += m_loadPath;
		return ret;
	}

	const wxString ProjectImage::GetFullPathToCompiledBinary() const
	{
		wxString path = GetFullPathToCompiledDir();
		path += "code.bin";
		return path;
	}

	const wxString ProjectImage::GetFullPathToCompiledDir() const
	{
		wxString ret = m_compiledDirectory;

		// msvc_
		ret += m_settings.m_recompilationTool;

		// mode
		if (m_settings.m_recompilationMode == tools::ProjectRecompilationMode::Debug)
			ret += "_debug";
		else if (m_settings.m_recompilationMode == tools::ProjectRecompilationMode::SafeRelease)
			ret += "_safe";
		else if (m_settings.m_recompilationMode == tools::ProjectRecompilationMode::FastRelease)
			ret += "_fast";

		// load address override
		if (m_settings.m_customLoadAddress != 0)
			ret += wxString::Format("_L%08llX", m_settings.m_customLoadAddress);

		// entry address override
		if (m_settings.m_customEntryAddress != 0)
			ret += wxString::Format("_E%08llX", m_settings.m_customEntryAddress);

		ret += wxFileName::GetPathSeparator();
		return ret;
	}

	void ProjectImage::SetSettings(const ProjectImageSettings& newSettings)
	{
		if (m_settings.CalcHash() != newSettings.CalcHash())
		{
			m_settings = newSettings;
			MarkAsModified();
		}
	}

	void ProjectImage::MarkAsModified()
	{
		m_owner->MarkAsModified();
	}

	const bool ProjectImage::RequiresRecompilation() const
	{
		// we always have to recompile if we don't have the target file
		if (!wxFileExists(GetFullPathToCompiledBinary()))
			return true;

		return false;
	}

	const bool ProjectImage::CanCompile() const
	{
		if (!m_settings.m_recompilationEnabled)
			return false;
		if (m_settings.m_recompilationTool.empty())
			return false;
		return true;
	}

	const bool ProjectImage::CanRun() const
	{
		if (!m_settings.m_emulationEnabled)
			return false;
		if (!wxFileExists(GetFullPathToCompiledBinary()))
			return false;
		return true;
	}

	static wxString EscapePath(const wxString path)
	{
		if (wxFileName::GetPathSeparator() == '\\')
		{
			wxString temp(path);
			temp.Replace("\\", "\\\\");
			return temp;
		}
		else
		{
			return path;
		}
	}

	void ProjectImage::GetCompilationTasks(std::vector<BuildTask>& outTasks) const
	{
		BuildTask task;
		task.m_displayName = "Build '" + m_displayName + "'";
		task.m_command = wxTheApp->GetCommandLineTool();

		task.m_commandLine = " -command=recompile ";

		task.m_commandLine += " -generator=";
		task.m_commandLine += m_settings.m_recompilationTool;

		task.m_commandLine += " -in=\"";
		task.m_commandLine += EscapePath(GetFullPathToImage());
		task.m_commandLine += "\"";

		task.m_commandLine += " -out=\"";
		task.m_commandLine += EscapePath(GetFullPathToCompiledDir());
		task.m_commandLine += "\"";

		if (m_settings.m_recompilationMode == ProjectRecompilationMode::Debug)
			task.m_commandLine += " -O0";
		else if (m_settings.m_recompilationMode == ProjectRecompilationMode::SafeRelease)
			task.m_commandLine += " -O1";
		else if (m_settings.m_recompilationMode == ProjectRecompilationMode::FastRelease)
			task.m_commandLine += " -O2";

		outTasks.push_back(task);		
	}

	bool ProjectImage::Save(ILogOutput& log, IBinaryFileWriter& writer) const
	{
		FileChunk chunk(writer, "ProjectImage");

		// save the image file name
		writer << m_loadPath;
		writer << m_displayName;

		// save settings
		{
			FileChunk chunk(writer, "Settings");
			writer << m_settings;
		}

		// save address history
		{
			FileChunk chunk(writer, "AddressHistory");
			m_addressHistory.Save(log, writer);
		}

		// save the decoding environment data
		if (m_env->IsModified())
			if (!m_env->Save(log))
				return false;

		// saved
		return true;
	}

	bool ProjectImage::Load(ILogOutput& log, IBinaryFileReader& reader)
	{
		FileChunk chunk(reader, "ProjectImage");
		if (!chunk)
			return false;

		// load the image name
		reader >> m_loadPath;
		reader >> m_displayName;

		// save settings
		{
			FileChunk chunk(reader, "Settings");
			if (!chunk)
				return false;
			reader >> m_settings;
		}

		// save address history
		{
			FileChunk chunk(reader, "AddressHistory");
			if (chunk)
				m_addressHistory.Load(log, reader);
		}

		// load the image's decoding environment
		wxFileName decodedImagePath(m_loadPath);
		if (!decodedImagePath.MakeAbsolute(m_owner->GetProjectDirectory()))
		{
			log.Error("Project: Reference image path '%ls' is not a valid path", m_loadPath.c_str());
			return false;
		}

		// load
		m_env = decoding::Environment::Load(log, decodedImagePath.GetFullPath().wc_str());
		if (!m_env)
			return false;

		// post process
		PostLoad();
		return true;
	}

	std::shared_ptr<ProjectImage> ProjectImage::Create(ILogOutput& log, Project* project, const std::wstring& imageImportPath)
	{
		// get the image name
		const std::string imageName = wxFileName(imageImportPath.c_str()).GetName().c_str().AsChar();
		if (imageName.empty())
		{
			log.Error("Project: Unable to import file '%ls' because the file name is invalid");
			return nullptr;
		}

		// get the path to the decoded image
		wxFileName decodedImagePath(imageImportPath);
		decodedImagePath.SetExt("pdi");

		// get the load path as relative path to the project file
		wxFileName relativePath(decodedImagePath);
		if (!relativePath.MakeRelativeTo(project->GetProjectDirectory()))
		{
			log.Error("Project: Unable to import file '%ls' because there's no relative path from project to the file");
			return nullptr;
		}

		// stats
		const std::wstring loadPath = relativePath.GetFullPath().wc_str();
		log.Log("Project: Relative image path: '%ls'", loadPath.c_str());

		// load image binary
		const auto bin = project->GetPlatform()->LoadImageFromFile(log, imageImportPath);
		if (!bin)
		{
			log.Error("Project: Unable to import file '%ls' because the image could not be loaded");
			return nullptr;
		}

		// create the decoding environment
		auto env = decoding::Environment::Create(log, project->GetPlatform(), bin, decodedImagePath.GetFullPath().wc_str());
		if (!env)
		{
			log.Error("Project: Unable to import file '%ls' because the image could not be decoded");
			return nullptr;
		}

		// create the project image
		std::shared_ptr<ProjectImage> ret(new ProjectImage(project));
		ret->m_env = env;
		ret->m_loadPath = loadPath;
		ret->m_displayName = imageName;
		ret->m_addressHistory.Reset(bin->GetEntryAddress());
		ret->PostLoad();
		return ret;
	}

	void ProjectImage::PostLoad()
	{
		// find a recompilation tool
		if (m_settings.m_recompilationTool.empty())
		{
			std::vector<std::string> generatorNames;
			code::IGenerator::ListGenerators(generatorNames);

			if (!generatorNames.empty())
				m_settings.m_recompilationTool = generatorNames[0];
		}

		// get the crc of the image data
		m_compiledDirectory = m_owner->GetProjectDirectory();
		m_compiledDirectory += wxString(".temp") + wxFileName::GetPathSeparator();
		m_compiledDirectory += AnsiToUnicode(m_displayName);
		m_compiledDirectory += wxString::Format("_0x%016llX", m_env->GetDecodingContext()->GetImage()->GetCRC());
		m_compiledDirectory += wxFileName::GetPathSeparator();
	}

	//--

} // tools