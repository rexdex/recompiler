#pragma once

#include "codeGenerator.h"

namespace code
{
	namespace msvc
	{
		/// Code generator for MSVC tool chain
		class Generator : public IGenerator
		{
		public:
			Generator(class ILogOutput& log, const Commandline& params);
			virtual ~Generator();

		protected:
			// IGenerator
			virtual void SetLoadAddress(const uint64 loadAddress) override final;
			virtual void SetEntryAddress(const uint64 entryAddress) override final;
			virtual void AddPlatformInclude(const std::wstring& path) override final;
			virtual void AddImportSymbol(const char* name, const uint64 tableAddress, const uint64 entryAddress) override final;
			virtual void AddImageData(ILogOutput& log, const void* imageData, const uint32 imageSize) override final;
			virtual void AddCode(const uint64 addr, const char* code) override final;
			virtual void AddCodef(const uint64 addr, const char* code, ...) override final;
			virtual void StartBlock(const uint64 addr, const bool multiAddress, const char* optionalFunctionName) override final;
			virtual void CloseBlock() override final;
			virtual const bool CompileModule(IGeneratorRemoteExecutor& executor, const std::wstring& tempPath, const std::wstring& outputFilePath) override final;

		private:
			// add the image glue
			void AddGlueFile();

			// generate manual project/solution/makefile
			void AddMakeFile(const std::wstring& tempPath, const std::wstring& outFilePath);

			// close current file
			void CloseFile();

			// force start a new file
			void StartFile(const char* customFileName, const bool addIncludes = true);

			class File
			{
			public:
				wchar_t	m_fileName[64];
				uint32 m_numBlocks;
				uint32 m_numInstructions;
				Printer* m_codePrinter;

				File(const wchar_t* fileName);
				~File();
			};

			// generated blocks
			struct BlockInfo
			{
				char m_name[32];
				bool m_multiAddress;
				uint64 m_addressStart;
				uint64 m_addressEnd;
			};

			// trap/int/sc info
			struct InterruptInfo
			{
				char m_name[64]; // function name
				char m_signature[128]; // full function call signature
				uint32 m_type; // internal interrupt type (0=trap, 1=sc, etc)
				uint32 m_index; // interrupt index (int 3, int 20, etc)
				uint32 m_useCount; // number of times global was sued
			};

			// import info
			struct ImportInfo
			{
				char m_importName[200];
				uint64 m_tableAddress;		 // functions & vars (hold pointer to location in which the resolved symbol address is written)
				uint64 m_entryAddress;		 // functions only, 0=var
			};

			typedef std::vector< BlockInfo > TBlockList;
			TBlockList m_exportedBlocks;

			typedef std::vector< InterruptInfo > TInterruptInfo;
			TInterruptInfo	m_exportedInterrupts;

			typedef std::vector< ImportInfo > TFunctionList;
			TFunctionList m_exportedSymbols;

			typedef std::vector< std::wstring > TIncludes;
			TIncludes	m_includes;

			typedef std::vector< File* > TFiles;
			TFiles			m_files;
			File*			m_currentFile;

			uint32			m_totalNumBlocks;
			uint32			m_totalNumInstructions;

			uint64			m_blockBaseAddress;
			uint64			m_blockReturnAddress;

			bool			m_inBlock;
			bool			m_isBlockMultiAddress;
			uint64			m_lastMultiAddressSwitch;
			uint64			m_lastGeneratedCodeAddress;

			bool			m_hasImage;
			uint64			m_imageBaseAddress;
			uint32			m_imageUncompressedSize;
			uint32			m_imageCompressedSize;
			uint64			m_imageEntryAdrdress;

			uint32			m_instructionsPerFile;
			bool			m_forceMultiAddressBlocks;
			std::string		m_runtimePlatform;
			std::string		m_compilationPlatform;

			ILogOutput*		m_logOutput;
		};

	} // 
} // 