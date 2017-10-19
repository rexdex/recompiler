#pragma once

namespace tools
{
	struct BuildTask;

	// project type
	enum class ProjectImageType : uint8
	{
		Application,
		DynamicLibrary,
	};

	// project recompilation quality
	enum class ProjectRecompilationMode : uint8
	{
		Debug, // allows debugging and traces
		SafeRelease, // normal mode
		FastRelease, // fastest possible mode
	};

	// project settings for image decompilation
	struct ProjectImageSettings
	{
		// type of the image
		ProjectImageType m_imageType;

		// is this project enabled for recompilation
		bool m_recompilationEnabled;

		// is this project enabled for enumeration (loaded by the loader)
		bool m_emulationEnabled;

		// decompilation tool
		std::string m_recompilationTool;

		// recompilation mode
		ProjectRecompilationMode m_recompilationMode;

		// override image base address
		uint64 m_customLoadAddress;

		// override image entry address
		uint64 m_customEntryAddress;

		// when moving image to different base address ignore the relocation data
		bool m_ignoreRelocations;

		//--

		ProjectImageSettings();

		const uint64 CalcHash() const;
	};

	// imported image information
	class ProjectImage
	{
	public:
		ProjectImage(Project* project);

		// get project settings
		inline const ProjectImageSettings& GetSettings() const { return m_settings; }

		// get decoding environment
		inline const decoding::Environment& GetEnvironment() const { return *m_env; }

		// get the address history for image navigation
		inline const ProjectAdddrsesHistory& GetAddressHistory() const { return m_addressHistory; }
		inline ProjectAdddrsesHistory& GetAddressHistory() { return m_addressHistory; }

		// get project load path, path relative to the project
		inline const std::wstring& GetLoadPath() const { return m_loadPath; }

		// get the image short display name
		inline const std::string& GetDisplayName() const { return m_displayName; }

		// get the path to the compiled binary project
		// NOTE: this depends on the settings applied to the image
		const wxString GetFullPathToCompiledDir() const;

		// get the path to the compiled binary project
		// NOTE: this depends on the settings applied to the image
		const wxString GetFullPathToCompiledBinary() const;

		// get full path to the project file
		const wxString GetFullPathToImage() const;

		// set new settings, if changed project will be marked ad modified
		void SetSettings(const ProjectImageSettings& newSettings);

		// mark after changes as modified so we will prompt to save the changes
		void MarkAsModified();

		// check if the project image requires recompilation
		const bool RequiresRecompilation() const;

		// get the compilation tasks
		void GetCompilationTasks(std::vector<BuildTask>& outTasks) const;

		// can we compile this project?
		const bool CanCompile() const;

		// can we run this project
		const bool CanRun() const;

		//--

		// load from stream
		bool Load(ILogOutput& log, IBinaryFileReader& reader);

		// save to stream
		bool Save(ILogOutput& log, IBinaryFileWriter& writer) const;

		//--

		// create image
		static std::shared_ptr<ProjectImage> Create(ILogOutput& log, Project* project, const std::wstring& imageImportPath);

	public:
		/// owning project
		Project* m_owner;

		/// decoded image file (with respect to project directory)
		std::wstring m_loadPath;

		/// image short name (user32.dll, etc)
		std::string m_displayName;

		/// compiled directory (.temp/name_crc/)
		std::wstring m_compiledDirectory;

		/// decoded image
		std::shared_ptr<decoding::Environment> m_env;

		/// project settings
		ProjectImageSettings m_settings;

		/// history of visited addresses (for project navigation)
		ProjectAdddrsesHistory m_addressHistory;

		//---

		void PostLoad();
	};

} // tools