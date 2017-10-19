#pragma once

namespace image
{
	class Binary;
}

namespace decoding
{
	class Context;
}

namespace code
{
	class IGenerator;
}

namespace platform
{
	class CPU;
	class DecompilationInterface;

	struct RECOMPILER_API FileFormat
	{
		std::string		m_name;
		std::string		m_extension;
	};

	/// Platform definition
	class RECOMPILER_API Definition
	{
	public:
		// get name & description of the platform
		inline const std::string& GetName() const { return m_name; }

		// get the low-level decompilation interface
		inline DecompilationInterface* GetDecompilationInterface() const { return m_interface; }

		// get platform export library, may be NULL
		inline const std::shared_ptr<ExportLibrary>& GetExportLibrary() const { return m_exports; }

		// get the CPUs defined in the platform
		inline const uint32 GetNumCPUs() const { return (uint32)m_cpu.size(); }
		inline const CPU* GetCPU(const uint32 index) const { return m_cpu[index]; }

	public:
		~Definition();

		// find CPU by unique template name (not the display name)
		const CPU* FindCPU(const char* name) const;

		// enumerate supported formats
		void EnumerateImageFormats(std::vector<platform::FileFormat>& outFormats) const;

		// load executable image created for this platform
		std::shared_ptr<image::Binary> LoadImageFromFile(ILogOutput& log, const std::wstring& imagePath) const;

		//! Decode image instructions, resolve jumps and initial block locations, usually done only once, returns false on critical errors
		const bool DecodeImage(ILogOutput& log, decoding::Context& decodingContext) const;

		//! Export code from image
		const bool ExportCode(ILogOutput& log, decoding::Context& decodingContext, const Commandline& settings, code::IGenerator& generator) const;

		//---

		// load platform definition (will load the dependent data)
		static Definition* Load(ILogOutput& log, const std::wstring& baseDirectory, const std::string& platformDLLName);

	private:
		Definition();

		// platform decoding interface
		DecompilationInterface* m_interface;

		// module handle
		void* m_hLibraryModule;

		// cached name of the platform
		std::string m_name;

		// cpu definitions in this platform (usually one)
		typedef std::vector< const CPU* >	TCPUDefinitions;
		TCPUDefinitions m_cpu;

		// export library
		std::shared_ptr<ExportLibrary> m_exports;
	};

	//---------------------------------------------------------------------------

} // platform