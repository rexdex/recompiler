#pragma once

namespace code
{
	class Printer;

	/// Code generator executable runner
	class RECOMPILER_API IGeneratorRemoteExecutor
	{
	public:
		virtual const uint32 RunExecutable(ILogOutput& log, const std::wstring& executablePath, const std::wstring& executableArguments) = 0;
	};

	/// Code generator
	class RECOMPILER_API IGenerator
	{
	public:
		virtual ~IGenerator();

		// set load address for converted image
		virtual void SetLoadAddress(const uint64 loadAddress) = 0;

		// set entry address for converted image
		virtual void SetEntryAddress(const uint64 entryAddress) = 0;

		// add global include (added to all files)
		virtual void AddPlatformInclude(const std::wstring& path) = 0;

		// add import symbol
		virtual void AddImportSymbol(const char* name, const uint64 tableAddress, const uint64 entryAddress) = 0;

		// add image file
		virtual void AddImageData(ILogOutput& log, const void* imageData, const uint32 imageSize) = 0;

		// print instruction
		virtual void AddCode(const uint64 addr, const char* code) = 0;

		// print instruction, formated
		virtual void AddCodef(const uint64 addr, const char* code, ...) = 0;

		// start new block
		virtual void StartBlock(const uint64 addr, const bool multiAddress, const char* optionalFunctionName) = 0;

		// close current block
		virtual void CloseBlock() = 0;

		// generate the final stuff
		virtual const bool CompileModule(IGeneratorRemoteExecutor& executor, const std::wstring& tempPath, const std::wstring& outputFilePath) = 0;

		///--

		/// list code generators
		static void ListGenerators(std::vector<std::string>& outCodeGenerators);

		/// create code generator
		static std::shared_ptr<IGenerator> CreateGenerator(ILogOutput& log, const Commandline& parameters);
	};

} // code