#pragma once

namespace platform
{
	class CodeProfile;
}

namespace code
{
	class Printer;

	/// Code task
	struct RECOMPILER_API Task
	{
	public:
		std::string		m_name;
		std::wstring	m_command;
		std::wstring	m_arguments;
	};

	/// Code generator
	class RECOMPILER_API Generator
	{
	public:
		Generator(class ILogOutput& log, const std::wstring& includeDirectory, const std::wstring& projectPath, const std::wstring& outPath, const decoding::CompilationMode mode, const decoding::CompilationPlatform platform, const uint64 entryAddress);
		~Generator();

		// get the compilation mode for this generator
		inline const decoding::CompilationMode GetCompilationMode() const { return m_compilationMode; }

		// get the compilation platform for this generator
		inline const decoding::CompilationPlatform GetCompilationPlatform() const { return m_compilationPlatform; }

		// add global include (added to all files)
		const bool AddInclude( const std::wstring& path, const bool resetPrevious = false );

		// add import symbol
		const bool AddImportSymbol( const char* name, const uint32 tableAddress, const uint32 entryAddress );

		// add image file
		const bool AddImageData( ILogOutput& log, const void* imageData, const uint32 imageSize, const uint32 imageBaseAddress );

		// emit int/trap/sc call
		const bool AddInterruptCall( const uint32 type, const uint32 index, std::string& outSymbolName, std::string& outSignature );

		// print instruction
		const bool AddCode( const uint32 addr, const char* code );

		// print instruction, formated
		const bool AddCodef( const uint32 addr, const char* code, ... );

		// start new block
		const bool StartBlock( const uint32 addr, const bool multiAddress, const char* optionalFunctionName );

		// close current block
		const bool CloseBlock();

		// finish code generation, returns build tasks
		void FinishGeneration( std::vector< Task >& outBuildTasks );

	private:
		// add the image glue
		const bool AddGlueFile();

		// generate manual project/solution/makefile
		void AddVSMakeFile();

		// close current file
		const bool CloseFile();

		// force start a new file
		const bool StartFile( const char* customFileName, const bool addIncludes = true );

		class File
		{
		public:
			wchar_t		m_fileName[64];
			uint32		m_numBlocks;
			uint32		m_numInstructions;

			File(const wchar_t* fileName);
			~File();
		};

		// generated blocks
		struct BlockInfo
		{
			char			m_name[32];
			bool			m_multiAddress;
			uint32			m_addressStart;
			uint32			m_addressEnd;
		};

		// trap/int/sc info
		struct InterruptInfo
		{
			char			m_name[64];			// function name
			char			m_signature[128];	// full function call signature
			uint32			m_type;				// internal interrupt type (0=trap, 1=sc, etc)
			uint32			m_index;			// interrupt index (int 3, int 20, etc)
			uint32			m_useCount;			// number of times global was sued
		};

		// import info
		struct ImportInfo
		{
			char			m_importName[200];
			uint32			m_tableAddress;		 // functions & vars (hold pointer to location in which the resolved symbol address is written)
			uint32			m_entryAddress;		 // functions only, 0=var
		};

		decoding::CompilationMode		m_compilationMode;
		decoding::CompilationPlatform	m_compilationPlatform;

		typedef std::vector< BlockInfo > TBlockList;
		TBlockList m_exportedBlocks;

		typedef std::vector< InterruptInfo > TInterruptInfo;
		TInterruptInfo	m_exportedInterrupts;

		typedef std::vector< ImportInfo > TFunctionList;
		TFunctionList m_exportedSymbols;

		typedef std::vector< std::wstring > TIncludes;
		TIncludes	m_includes;

		std::wstring	m_projectPath;
		std::wstring	m_includeDirectory;
		std::wstring	m_outputCodePath;

		typedef std::vector< File* > TFiles;
		TFiles			m_files;
		File*			m_currentFile;

		Printer*		m_codePrinter;

		uint32			m_totalNumBlocks;
		uint32			m_totalNumInstructions;

		uint32			m_blockBaseAddress;
		uint32			m_blockReturnAddress;

		bool			m_inBlock;
		bool			m_isBlockMultiAddress;
		uint32			m_lastMultiAddressSwitch;
		uint32			m_lastGeneratedCodeAddress;

		bool			m_hasImage;
		uint32			m_imageBaseAddress;
		uint32			m_imageUncompressedSize;
		uint32			m_imageCompressedSize;
		uint64			m_imageEntryAdrdress;

		ILogOutput*		m_logOutput;
	};

} // code