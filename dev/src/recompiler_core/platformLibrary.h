#pragma once

namespace platform
{
	class CPU;
	struct FileFormat;
	class Definition;
	class ExportLibrary;

	/// platform library - contains all of the supported platforms
	class RECOMPILER_API Library
	{
	public:
		// platform libraries
		inline const uint32 GetNumPlatforms() const { return (uint32)m_platformLibrary.size(); }
		inline const Definition* GetPlatform(const uint32 index) const { return m_platformLibrary[index]; }

		// initialize library at given data folder
		bool Initialize(class ILogOutput& log);

		// close library, release all loaded platforms
		void Close();

		// find platform library by name
		const Definition* FindPlatform(const char* name) const;

		// find platform library that can decompile given executable
		const std::vector< Definition* > FindCompatiblePlatforms(const std::wstring& filePath) const;

		// enumerate all supported executable formats for all supported platforms
		void EnumerateImageFormats(std::vector<FileFormat>& outFormats) const;

		//---

		// singleton, justified
		static Library& GetInstance();

	private:
		Library();
		~Library();

		// recognized decoding platform 
		typedef std::vector< platform::Definition* >	TPlatformLibrary;
		TPlatformLibrary		m_platformLibrary;
	};

} // platform