#pragma once

namespace platform
{
	class Definition;
	class CodeProfile;
	class CompilerDefinition;
}

namespace image
{
	class Binary;
}

namespace decoding
{  
	class Context;

	extern RECOMPILER_API const char* GetCompilationModeName(const CompilationMode mode);

	extern RECOMPILER_API const char* GetCompilationPlatformName(const CompilationPlatform platform);

	/// Global decoding environment
	class RECOMPILER_API Environment
	{
	public:
		~Environment();

		//----

		// Get the project path (can be empty until saved)
		inline const std::wstring& GetProjectPath() const { return m_projectPath; }

		// Get the project directory (can be empty until saved)
		inline const std::wstring& GetProjectDir() const { return m_projectDir; }

		// Get current image being decoded
		inline const image::Binary* GetImage() const { return m_image; }

		// Get the platform used for decompilation of this image
		inline const platform::Definition* GetPlatform() const { return m_platform; }

		// Get active code profile
		inline const CompilationMode GetCompilationMode() const { return m_compilationMode; }

		// Get active compiler
		inline const CompilationPlatform GetCompilationPlatform() const { return m_compilationPlatform; }

		// Get current decoding context
		inline Context* GetDecodingContext() const { return m_decodingContext; }

		//----

		//! Has the project data been modified ?
		bool IsModified() const;

		//! Set compilation mode
		void SetCompilationMode(CompilationMode mode);

		//! Set compilation platform
		void SetCompilationPlatform(CompilationPlatform platform);

		//! Save current environment to file
		bool Save( class ILogOutput& log, const bool forceAll = false );

		//! Export image for execution
		bool CompileCode( class ILogOutput& log ) const;

		//----

		//! Create a decoding environment from a loaded image for given platform
		//! Creates the initial project structure
		static Environment* Create( class ILogOutput& log, const platform::Definition* platform, const image::Binary* loadedBinary, const std::wstring& projectFile );

		//! Load existing environment from file
		static Environment* Load( class ILogOutput& log, const std::wstring& projectFile );

	private:
		// source path for the project file
		std::wstring				m_projectPath; // project root file
		std::wstring				m_projectDir; // project directory

		// decoding context for function decoding
		Context*					m_decodingContext;

		// image loaded and the image that is loaded
		const image::Binary*		m_image;

		// decoding platform
		const platform::Definition*	m_platform;
		std::string					m_platformName;

		// compilation settings
		CompilationMode				m_compilationMode;
		CompilationPlatform			m_compilationPlatform;

		Environment();
	};

} // decoding