#pragma once

namespace platform
{
	class Definition;
	class CompilerDefinition;
}

namespace image
{
	class Binary;
}

namespace decoding
{
	class Context;

	/// Global decoding environment
	class RECOMPILER_API Environment
	{
	public:
		~Environment();

		//----

		// Get the project path (can be empty until saved)
		inline const std::wstring& GetProjectPath() const { return m_projectPath; }

		// Get current image being decoded
		inline const std::shared_ptr<image::Binary>& GetImage() const { return m_image; }

		// Get the platform used for decompilation of this image
		inline const platform::Definition* GetPlatform() const { return m_platform; }

		// Get current decoding context
		inline Context* GetDecodingContext() const { return m_decodingContext; }

		//----

		//! Has the project data been modified ?
		bool IsModified() const;

		//! Save current environment to file
		bool Save(class ILogOutput& log);

		//----

		//! Create a decoding environment from a loaded image for given platform
		//! Creates the initial project structure
		static std::shared_ptr<Environment> Create(class ILogOutput& log, const platform::Definition* platform, const std::shared_ptr<image::Binary>& loadedBinary, const std::wstring& projectFile);

		//! Load existing environment from file
		static std::shared_ptr<Environment> Load(class ILogOutput& log, const std::wstring& projectFile);

	private:
		// source path for the project file
		std::wstring m_projectPath; // project root file

		// decoding context for function decoding
		Context* m_decodingContext;

		// image loaded and the image that is loaded
		std::shared_ptr<image::Binary> m_image;

		// decoding platform
		const platform::Definition*	m_platform;
		std::string m_platformName;

		Environment();
	};

} // decoding