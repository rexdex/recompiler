#include "build.h"
#include "codeGeneratorMSVC.h"
#include "codePrinter.h"
#include "internalUtils.h"
#include "zlib\zlib.h"
#include "decodingEnvironment.h"

#if defined(_WIN64) || defined(_WIN32)

#include <Windows.h>

namespace code
{
	namespace msvc
	{

		//----------------------------------------------------------------------

		Generator::File::File(const wchar_t* fileName)
			: m_numBlocks(0)
			, m_numInstructions(0)
			, m_codePrinter(new Printer())
		{
			wcscpy_s(m_fileName, fileName);
		}

		Generator::File::~File()
		{
			delete m_codePrinter;
		}

		//----------------------------------------------------------------------

		Generator::Generator(class ILogOutput& log, const Commandline& params)
			: m_currentFile(NULL)
			, m_logOutput(&log)
			, m_blockBaseAddress(0)
			, m_totalNumBlocks(0)
			, m_totalNumInstructions(0)
			, m_isBlockMultiAddress(false)
			, m_inBlock(false)
			, m_hasImage(false)
			, m_imageBaseAddress(0)
			, m_imageEntryAdrdress(0)
			, m_imageUncompressedSize(0)
			, m_imageCompressedSize(0)
			, m_forceMultiAddressBlocks(false)
			, m_instructionsPerFile(32 * 1024)
			, m_runtimePlatform("win64")
			, m_compilationPlatform("release")
		{
			// get the optimization settings
			if (params.HasOption("debug"))
			{
				m_compilationPlatform = "debug";
				m_forceMultiAddressBlocks = true;
			}
		}

		Generator::~Generator()
		{
			DeleteVector(m_files);
		}

		void Generator::CloseFile()
		{
			CloseBlock();

			if (m_currentFile)
			{
				// stats
				m_logOutput->Log("CodeGen: Generated file '%ls', %u blocks, %u instructions",
					m_currentFile->m_fileName,
					m_currentFile->m_numBlocks,
					m_currentFile->m_numInstructions);
				m_currentFile = NULL;
			}
		}

		void Generator::StartFile(const char* customFileName, const bool addIncludes /*= true*/)
		{
			CloseFile();

			if (customFileName && customFileName[0])
			{
				const std::wstring tempFile(customFileName, customFileName + strlen(customFileName));
				m_currentFile = new File(tempFile.c_str());
			}
			else
			{
				wchar_t fileName[32];
				swprintf_s(fileName, ARRAYSIZE(fileName), L"autocode_%d.cpp", (int)m_files.size());

				m_currentFile = new File(fileName);
			}

			m_files.push_back(m_currentFile);

			if (addIncludes && !m_includes.empty())
			{
				for (uint32 i = 0; i < m_includes.size(); ++i)
				{
					m_currentFile->m_codePrinter->Printf("#include \"%ls\"\n", m_includes[i].c_str());
				}

				m_currentFile->m_codePrinter->Printf("\n");
			}

			m_inBlock = false;
		}

		void Generator::CloseBlock()
		{
			// function tail
			if (m_inBlock)
			{
				if (m_isBlockMultiAddress)
				{
					m_currentFile->m_codePrinter->Indent(-1);
					m_currentFile->m_codePrinter->Print("}\n");
				}

				m_currentFile->m_codePrinter->Printf("return 0x%08X;\n", m_blockReturnAddress);
				m_currentFile->m_codePrinter->Indent(-1);
				m_currentFile->m_codePrinter->Printf("} // Block from %06Xh-%06Xh (%d instructions)\n",
					m_blockBaseAddress,
					m_blockReturnAddress,
					(m_blockReturnAddress - m_blockBaseAddress) / 4);
				m_currentFile->m_codePrinter->Printf("\n");

				// setup the return address (final address of the block)
				if (!m_exportedBlocks.empty() && m_exportedBlocks.back().m_addressEnd == 0)
				{
					m_exportedBlocks.back().m_addressEnd = m_blockReturnAddress;
				}

				m_blockReturnAddress = 0;
				m_blockBaseAddress = 0;
				m_isBlockMultiAddress = false;
				m_inBlock = false;
			}
		}

		void Generator::SetLoadAddress(const uint64 loadAddress)
		{
			m_logOutput->Log("CodeGen: Image load address set to 0x%08llx", loadAddress);
			m_imageBaseAddress = loadAddress;
		}

		void Generator::SetEntryAddress(const uint64 entryAddress)
		{
			m_logOutput->Log("CodeGen: Image entry address set to 0x%08llx", entryAddress);
			m_imageEntryAdrdress = entryAddress;
		}

		void Generator::AddPlatformInclude(const std::wstring& path)
		{
			TIncludes::const_iterator it = std::find(m_includes.begin(), m_includes.end(), path);
			if (it == m_includes.end())
				m_includes.push_back(path);
		}

		void Generator::AddImportSymbol(const char* name, const uint64 tableAddress, const uint64 entryAddress)
		{
			ImportInfo info;
			strcpy_s(info.m_importName, name);
			info.m_tableAddress = tableAddress;
			info.m_entryAddress = entryAddress;
			m_exportedSymbols.push_back(info);
		}

		void Generator::AddImageData(ILogOutput& log, const void* imageData, const uint32 imageSize)
		{
			// already have an image
			if (m_hasImage)
			{
				m_logOutput->Warn("CodeGen: Image data already added");
				return;
			}

			// generate compressed image data
			log.SetTaskName("Compressing image... ");
			std::vector<uint8> compresedImageData;
			if (!CompressData(imageData, imageSize, compresedImageData))
			{
				m_logOutput->Error("CodeGen: Unable to compress image data");
				return;
			}

			// stats
			log.Log("Decompile: Image compressed %u->%u bytes", (uint32)imageSize, compresedImageData.size());

			// setup data
			m_hasImage = true;
			m_imageCompressedSize = (uint32)compresedImageData.size();
			m_imageUncompressedSize = imageSize;

			// create new file (image data)
			StartFile("image.cpp");

			// Header
			m_currentFile->m_codePrinter->Printf("// This file contains the compressed copy of original image file to be decompressed and loaded by the runtime\n");
			m_currentFile->m_codePrinter->Printf("// Compressed size = %d\n", m_imageCompressedSize);
			m_currentFile->m_codePrinter->Printf("// Uncompressed size = %d\n", m_imageUncompressedSize);
			m_currentFile->m_codePrinter->Print("\n");

			// Generate image data
			m_currentFile->m_codePrinter->Printf("const unsigned char CompressedImageData[%d] = {", m_imageCompressedSize);
			log.SetTaskName("Exporting image data... ");
			for (uint32 i = 0; i < m_imageCompressedSize; ++i)
			{
				log.SetTaskProgress(i, m_imageCompressedSize);

				if ((i & 31) == 0)
					m_currentFile->m_codePrinter->Printf("\n\t");

				const uint8 val = compresedImageData[i];
				m_currentFile->m_codePrinter->Printf("%d,", val);
			}

			m_currentFile->m_codePrinter->Print("\n}; // CompressedImageData\n\n");
			m_currentFile->m_codePrinter->Print("const unsigned char* CompressedImageDataStart = &CompressedImageData[0];\n\n");

			// end file
			CloseFile();
		}

		void Generator::AddGlueFile()
		{
			// create new file (glue logic file)
			StartFile("main.cpp");

			// Windows include
			if (m_runtimePlatform == "win64" || m_runtimePlatform == "win32")
			{
				m_currentFile->m_codePrinter->Printf("#include <Windows.h>\n");
				m_currentFile->m_codePrinter->Printf("\n");
			}

			// block exports
			if (!m_exportedBlocks.empty())
			{
				m_currentFile->m_codePrinter->Printf("// %d exported blocks\n", m_exportedBlocks.size());
				for (uint32 i = 0; i < m_exportedBlocks.size(); ++i)
				{
					const char* blockSymbolName = m_exportedBlocks[i].m_name;
					m_currentFile->m_codePrinter->Printf("extern uint64 __fastcall %s( uint64 ip, cpu::CpuRegs& regs ); // 0x%08X - 0x%08X\n",
						blockSymbolName,
						m_exportedBlocks[i].m_addressStart,
						m_exportedBlocks[i].m_addressEnd);
				}
				m_currentFile->m_codePrinter->Printf("\n");
			}

			// interrupts
			if (!m_exportedInterrupts.empty())
			{
				m_currentFile->m_codePrinter->Printf("// %d exported interrupt calls\n", m_exportedInterrupts.size());
				m_currentFile->m_codePrinter->Printf("runtime::InterruptCall ExportedInterrupts[%d] = {\n", m_exportedInterrupts.size());
				for (uint32 i = 0; i < m_exportedInterrupts.size(); ++i)
				{
					const InterruptInfo& info = m_exportedInterrupts[i];
					m_currentFile->m_codePrinter->Printf("{ %d, %d, (runtime::TInterruptFunc) &runtime::UnhandledInterruptCall }, // used %d times \n",
						info.m_type,
						info.m_index,
						info.m_useCount);
				}
				m_currentFile->m_codePrinter->Print("};\n");
				m_currentFile->m_codePrinter->Print("\n");

				// calling interface
				m_currentFile->m_codePrinter->Print("// interrupt handlers\n");
				for (uint32 i = 0; i < m_exportedInterrupts.size(); ++i)
				{
					const InterruptInfo& info = m_exportedInterrupts[i];
					m_currentFile->m_codePrinter->Printf("void %s(uint64 ip, cpu::CpuRegs& regs) { (*ExportedInterrupts[%d].m_functionPtr)(ip, regs); }\n",
						info.m_name, i);
				}
				m_currentFile->m_codePrinter->Print("\n");
			}

			// block exports
			if (!m_exportedBlocks.empty())
			{
				m_currentFile->m_codePrinter->Printf("runtime::BlockInfo ExportedBlocks[%d] = {\n", m_exportedBlocks.size());

				for (uint32 i = 0; i < m_exportedBlocks.size(); ++i)
				{
					const BlockInfo& info = m_exportedBlocks[i];
					m_currentFile->m_codePrinter->Printf("\t{ 0x%08X, 0x%08X, (runtime::TBlockFunc) &%s }, \n",
						info.m_addressStart,
						(info.m_multiAddress) ? (info.m_addressEnd - info.m_addressStart) : 1,
						info.m_name);
				}

				m_currentFile->m_codePrinter->Print("};\n");
				m_currentFile->m_codePrinter->Print("\n");
			}

			// function exports
			if (!m_exportedSymbols.empty())
			{
				m_currentFile->m_codePrinter->Printf("runtime::ImportInfo ExportedImports[%d] = {\n", m_exportedSymbols.size());

				for (uint32 i = 0; i < m_exportedSymbols.size(); ++i)
				{
					const ImportInfo& info = m_exportedSymbols[i];

					m_currentFile->m_codePrinter->Printf("\t{ \"%s\", %d, 0x%08X }, \n",
						info.m_importName,
						(info.m_entryAddress == 0) ? 0 : 1,
						(info.m_entryAddress == 0) ? info.m_tableAddress : info.m_entryAddress);
				}

				m_currentFile->m_codePrinter->Print("};\n");
				m_currentFile->m_codePrinter->Print("\n");
			}

			// image info
			m_currentFile->m_codePrinter->Print("runtime::ImageInfo ExportImageInfo;\n\n");

			// image data import
			if (m_hasImage)
			{
				m_currentFile->m_codePrinter->Print("// compressed image data buffer\n");
				m_currentFile->m_codePrinter->Print("extern const unsigned char* CompressedImageDataStart;\n");
				m_currentFile->m_codePrinter->Print("\n");
			}

			// windows only shit
			if (m_runtimePlatform == "win64" || m_runtimePlatform == "win32") 
			{
				m_currentFile->m_codePrinter->Print("BOOL WINAPI DllMain( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved ) { return TRUE; }\n");
			}

			// get the image info
			m_currentFile->m_codePrinter->Print("extern \"C\" __declspec(dllexport) void* GetImageInfo()\n");
			m_currentFile->m_codePrinter->Print("{\n");
			
			// initialization
			{
				m_currentFile->m_codePrinter->Print("\tmemset( &ExportImageInfo, 0, sizeof(ExportImageInfo) );\n");
				if (m_hasImage)
				{
					m_currentFile->m_codePrinter->Printf("\tExportImageInfo.m_imageLoadAddress = 0x%llX;\n", m_imageBaseAddress);
					m_currentFile->m_codePrinter->Printf("\tExportImageInfo.m_imageCompressedSize = %u;\n", m_imageCompressedSize);
					m_currentFile->m_codePrinter->Printf("\tExportImageInfo.m_imageUncompressedSize = %u;\n", m_imageUncompressedSize);
					m_currentFile->m_codePrinter->Print("\tExportImageInfo.m_imageCompresedData = CompressedImageDataStart;\n");
				}

				m_currentFile->m_codePrinter->Printf("\tExportImageInfo.m_entryAddress = 0x%llx;\n", m_imageEntryAdrdress);

				// interrupts
				m_currentFile->m_codePrinter->Printf("\tExportImageInfo.m_numInterrupts = %d;\n", m_exportedInterrupts.size());
				m_currentFile->m_codePrinter->Printf("\tExportImageInfo.m_interrupts = %s;\n", m_exportedInterrupts.size() ? "&ExportedInterrupts[0]" : "NULL");

				// blocks
				m_currentFile->m_codePrinter->Printf("\tExportImageInfo.m_numBlocks = %d;\n", m_exportedBlocks.size());
				m_currentFile->m_codePrinter->Printf("\tExportImageInfo.m_blocks = %s;\n", m_exportedBlocks.size() ? "&ExportedBlocks[0]" : "NULL");

				// imports
				m_currentFile->m_codePrinter->Printf("\tExportImageInfo.m_numImports = %d;\n", m_exportedSymbols.size());
				m_currentFile->m_codePrinter->Printf("\tExportImageInfo.m_imports = %s;\n", m_exportedSymbols.size() ? "&ExportedImports[0]" : "NULL");
			}

			m_currentFile->m_codePrinter->Print("\treturn &ExportImageInfo;\n");
			m_currentFile->m_codePrinter->Print("}\n");
			m_currentFile->m_codePrinter->Print("\n");

			CloseFile();
		}
		/*
		const bool Generator::AddInterruptCall(const uint32 type, const uint32 index, std::string& outSymbolName, std::string& outSignature)
		{
			// already defined ?
			for (uint32 i = 0; i < m_exportedInterrupts.size(); ++i)
			{
				InterruptInfo& info = m_exportedInterrupts[i];
				if (info.m_type == type && info.m_index == index)
				{
					info.m_useCount += 1;
					outSymbolName = info.m_name;
					outSignature = info.m_signature;
					return true;
				}
			}

			// format symbol name
			char symbolName[256];
			sprintf_s(symbolName, "__interrupt_%u_%u", type, index);

			// format function call signature
			char symbolSignature[256];
			sprintf_s(symbolSignature, "extern void %s(const uint64 ip, cpu::CpuRegs& regs)", symbolName);

			// create information
			InterruptInfo info;
			strcpy_s(info.m_name, symbolName);
			strcpy_s(info.m_signature, symbolSignature);
			info.m_type = type;
			info.m_index = index;
			info.m_useCount = 1;
			m_exportedInterrupts.push_back(info);

			// use the generated symbol
			outSymbolName = symbolName;
			outSignature = symbolSignature;
			return true;
		}*/

		void Generator::StartBlock(const uint64 addr, const bool multiAddress, const char* optionalFunctionName)
		{
			// start new file
			if (!m_currentFile || m_currentFile->m_numInstructions > m_instructionsPerFile)
				StartFile(NULL);

			// close previous block
			CloseBlock();

			// block header
			m_currentFile->m_codePrinter->Printf("//////////////////////////////////////////////////////\n");
			m_currentFile->m_codePrinter->Printf("// Block at %06Xh\n", addr);
			if (optionalFunctionName)
				m_currentFile->m_codePrinter->Printf("// Function '%s'\n", optionalFunctionName);
			m_currentFile->m_codePrinter->Printf("//////////////////////////////////////////////////////\n");

			// start block
			m_blockBaseAddress = addr;
			m_blockReturnAddress = addr; // nothing
			m_isBlockMultiAddress = multiAddress || m_forceMultiAddressBlocks;
			m_lastMultiAddressSwitch = 0xFFFFFFFF;
			m_inBlock = true;

			// format block symbol
			char blockSymbolName[128];
			sprintf_s(blockSymbolName, "_code__block%08llX", addr);

			// finish current block
			if (!m_exportedBlocks.empty() && m_exportedBlocks.back().m_addressEnd == 0)
			{
				m_exportedBlocks.back().m_addressEnd = addr;
			}

			// add new block
			BlockInfo info;
			strcpy_s(info.m_name, blockSymbolName);
			info.m_addressStart = addr;
			info.m_addressEnd = 0;
			info.m_multiAddress = multiAddress;
			m_exportedBlocks.push_back(info);

			// block function header
			m_currentFile->m_codePrinter->Printf("uint64 __fastcall %s( uint64 ip, cpu::CpuRegs& regs )\n", blockSymbolName);
			m_currentFile->m_codePrinter->Print("{\n");
			m_currentFile->m_codePrinter->Indent(1);

			// block addr switch
			if (m_isBlockMultiAddress)
			{
				m_currentFile->m_codePrinter->Printf("const uint32 local_instr = (uint32)(ip - 0x%08llX) / 4;\n", addr);
				m_currentFile->m_codePrinter->Print("switch ( local_instr )\n");
				m_currentFile->m_codePrinter->Print("{\n");
				m_currentFile->m_codePrinter->Indent(1);

				// todo: optional multi address protection
				m_currentFile->m_codePrinter->Printf("default:\truntime::InvalidAddress(ip, 0x%08llX);\n", m_blockBaseAddress);
			}

			// stats
			m_totalNumBlocks += 1;
			m_currentFile->m_numBlocks += 1;
		}

		void Generator::AddCodef(const uint64 addr, const char* txt, ...)
		{
			char buffer[8192];
			va_list args;

			va_start(args, txt);
			vsprintf_s(buffer, sizeof(buffer), txt, args);
			va_end(args);

			AddCode(addr, buffer);
		}

		void Generator::AddCode(const uint64 addr, const char* code)
		{
			// WTF ?
			if (!m_currentFile)
			{
				m_logOutput->Error("CodeGen: Trying to add code outside file");
				return;
			}

			// no block ?
			if (!m_inBlock)
			{
				m_logOutput->Error("CodeGen: Trying to add code outside block");
				return;
			}

			// block addr switch
			if (m_isBlockMultiAddress)
			{
				const auto switchIndex = (addr - m_blockBaseAddress) / 4;
				if (switchIndex != m_lastMultiAddressSwitch)
				{
					m_lastMultiAddressSwitch = switchIndex;
					m_currentFile->m_codePrinter->Printf("  /* %08Xh */ case %4d:  \t\t", addr, switchIndex, addr);
				}
				else
				{
					m_currentFile->m_codePrinter->Printf("/* %08Xh case %4d:*/\t\t", addr, switchIndex, addr);
				}
			}

			// add code line
			m_currentFile->m_codePrinter->Print(code);

			// advance the address pointer
			m_lastGeneratedCodeAddress = addr;
			m_blockReturnAddress = addr + 4; // addr + instr size, HACK

			// stats
			m_totalNumInstructions += 1;
			m_currentFile->m_numInstructions += 1;
		}

		void Generator::AddMakeFile(const std::wstring& tempPath, const std::wstring& outFilePath)
		{
			// create new file (project file)
			StartFile("autocode.vcxproj", false);

			// Platform name config name
			const std::string configName = m_forceMultiAddressBlocks ? "debug " : "release";
			const std::string platformName = "x64";

			// FAKE XML 
			m_currentFile->m_codePrinter->Print("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
			m_currentFile->m_codePrinter->Print("  <Project DefaultTargets=\"Build\" ToolsVersion=\"4.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n");
			m_currentFile->m_codePrinter->Print("    <ItemGroup Label=\"ProjectConfigurations\">\n");
			m_currentFile->m_codePrinter->Print("      <ProjectConfiguration Include=\"Build|x64\">\n");
			m_currentFile->m_codePrinter->Print("        <Configuration>Build</Configuration>\n");
			m_currentFile->m_codePrinter->Print("        <Platform>x64</Platform>\n");
			m_currentFile->m_codePrinter->Print("      </ProjectConfiguration>\n");
			m_currentFile->m_codePrinter->Print("    </ItemGroup>\n");
			m_currentFile->m_codePrinter->Print("  <PropertyGroup Label=\"Globals\">\n");
			m_currentFile->m_codePrinter->Print("    <ProjectGuid>{D9FC1B76-CB60-481B-B288-6EF63DE94065}</ProjectGuid>\n");
			m_currentFile->m_codePrinter->Print("    <Keyword>Win32Proj</Keyword>\n");
			m_currentFile->m_codePrinter->Print("    <RootNamespace>autocode</RootNamespace>\n");
			m_currentFile->m_codePrinter->Print("  </PropertyGroup>\n");
			m_currentFile->m_codePrinter->Print("  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.Default.props\" />\n");
			m_currentFile->m_codePrinter->Print("  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Build|x64'\" Label=\"Configuration\">\n");
			m_currentFile->m_codePrinter->Print("    <ConfigurationType>DynamicLibrary</ConfigurationType>\n");
			m_currentFile->m_codePrinter->Print("    <UseDebugLibraries>false</UseDebugLibraries>\n");
			m_currentFile->m_codePrinter->Printf("    <WholeProgramOptimization>false</WholeProgramOptimization>\n");
			m_currentFile->m_codePrinter->Print("    <CharacterSet>Unicode</CharacterSet>\n");
			m_currentFile->m_codePrinter->Print("    <PlatformToolset>v140</PlatformToolset>\n");
			m_currentFile->m_codePrinter->Print("  </PropertyGroup>\n");
			m_currentFile->m_codePrinter->Print("  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.props\" />\n");
			m_currentFile->m_codePrinter->Print("  <ImportGroup Label=\"ExtensionSettings\">\n");
			m_currentFile->m_codePrinter->Print("  </ImportGroup>\n");
			m_currentFile->m_codePrinter->Print("  <ImportGroup Condition=\"'$(Configuration)|$(Platform)'=='Build|x64'\" Label=\"PropertySheets\">\n");
			m_currentFile->m_codePrinter->Print("    <Import Project=\"$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\" Condition=\"exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')\" Label=\"LocalAppDataPlatform\" />\n");
			m_currentFile->m_codePrinter->Print("  </ImportGroup>\n");
			m_currentFile->m_codePrinter->Print("  <PropertyGroup Label=\"UserMacros\" />\n");
			m_currentFile->m_codePrinter->Print("  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Build|x64'\">\n");
			m_currentFile->m_codePrinter->Print("    <LinkIncremental>false</LinkIncremental>\n");
			m_currentFile->m_codePrinter->Print("    <OutDir>..\\build\\</OutDir>\n");
			m_currentFile->m_codePrinter->Print("    <IntDir>..\\build\\</IntDir>\n");
			m_currentFile->m_codePrinter->Print("  </PropertyGroup>\n");
			m_currentFile->m_codePrinter->Print("  <ItemDefinitionGroup Condition=\"'$(Configuration)|$(Platform)'=='Build|x64'\">\n");
			m_currentFile->m_codePrinter->Print("    <ClCompile>\n");
			m_currentFile->m_codePrinter->Print("      <WarningLevel>Level3</WarningLevel>\n");
			m_currentFile->m_codePrinter->Print("      <PrecompiledHeader>\n");
			m_currentFile->m_codePrinter->Print("      </PrecompiledHeader>\n");
			m_currentFile->m_codePrinter->Print("      <Optimization>MaxSpeed</Optimization>\n");
			m_currentFile->m_codePrinter->Print("      <FunctionLevelLinking>true</FunctionLevelLinking>\n");
			m_currentFile->m_codePrinter->Print("      <IntrinsicFunctions>true</IntrinsicFunctions>\n");
			m_currentFile->m_codePrinter->Print("      <PreprocessorDefinitions>WIN32;NDEBUG;_WINDOWS;_USRDLL;AUTOCODE_EXPORTS;%(PreprocessorDefinitions)</PreprocessorDefinitions>\n");
			m_currentFile->m_codePrinter->Print("      <MultiProcessorCompilation>true</MultiProcessorCompilation>\n");
			m_currentFile->m_codePrinter->Print("    </ClCompile>\n");
			m_currentFile->m_codePrinter->Print("    <Link>\n");
			m_currentFile->m_codePrinter->Print("      <SubSystem>Windows</SubSystem>\n");
			m_currentFile->m_codePrinter->Print("      <GenerateDebugInformation>true</GenerateDebugInformation>\n");
			m_currentFile->m_codePrinter->Printf("    <EnableCOMDATFolding>false</EnableCOMDATFolding>\n");
			m_currentFile->m_codePrinter->Print("      <OptimizeReferences>true</OptimizeReferences>\n");
			m_currentFile->m_codePrinter->Printf("      <OutputFile>%ls</OutputFile>\n", outFilePath.c_str());
			m_currentFile->m_codePrinter->Print("    </Link>\n");
			m_currentFile->m_codePrinter->Print("  </ItemDefinitionGroup>\n");
			m_currentFile->m_codePrinter->Print("  <ItemGroup>\n");
			for (uint32 i = 0; i < m_files.size() - 1; ++i)
			{
				std::wstring fullPath = tempPath + L"code/";
				fullPath += m_files[i]->m_fileName;

				m_currentFile->m_codePrinter->Printf("    <ClCompile Include=\"%ls\" />\n", fullPath.c_str());
			}
			m_currentFile->m_codePrinter->Print("  </ItemGroup>\n");
			m_currentFile->m_codePrinter->Print("  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.targets\" />\n");
			m_currentFile->m_codePrinter->Print("  <ImportGroup Label=\"ExtensionTargets\">\n");
			m_currentFile->m_codePrinter->Print("  </ImportGroup>\n");
			m_currentFile->m_codePrinter->Print("</Project>\n");

			// save
			CloseFile();
		}

		static bool GetKeyData(HKEY hRootKey, const wchar_t *subKey, const wchar_t* value, void* data, DWORD cbData)
		{
			HKEY hKey;
			if (ERROR_SUCCESS != RegOpenKeyExW(hRootKey, subKey, 0, KEY_QUERY_VALUE, &hKey))
				return false;

			if (ERROR_SUCCESS != RegQueryValueExW(hKey, value, NULL, NULL, (LPBYTE)data, &cbData))
			{
				RegCloseKey(hKey);
				return false;
			}

			RegCloseKey(hKey);
			return true;
		}

		static std::wstring GetMSBuildDir()
		{
			wchar_t msBuildPath[512];

			if (GetKeyData(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\MSBuild\\ToolsVersions\\14.0", L"MSBuildToolsPath", msBuildPath, sizeof(msBuildPath)))
				return msBuildPath;

			if (GetKeyData(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\MSBuild\\ToolsVersions\\4.0", L"MSBuildToolsPath", msBuildPath, sizeof(msBuildPath)))
				return msBuildPath;

			if (GetKeyData(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\MSBuild\\ToolsVersions\\3.5", L"MSBuildToolsPath", msBuildPath, sizeof(msBuildPath)))
				return msBuildPath;

			if (GetKeyData(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\MSBuild\\ToolsVersions\\2.0", L"MSBuildToolsPath", msBuildPath, sizeof(msBuildPath)))
				return msBuildPath;

			return std::wstring();
		}

		static std::wstring GetMSBuildPath()
		{
			std::wstring path = GetMSBuildDir();
			if (!path.empty())
				path += L"msbuild.exe";

			return path;
		}

		const bool Generator::CompileModule(IGeneratorRemoteExecutor& executor, const std::wstring& tempPath, const std::wstring& outputFilePath)
		{
			// append some more stuff to the temp path based on the compilation platform
			std::wstring fullTempPath = tempPath;

			// generate glue file and the make file (vcproj)
			AddGlueFile();
			AddMakeFile(fullTempPath, outputFilePath);

			// return the solution file path
			std::wstring makeFilePath = fullTempPath;
			makeFilePath += L"code/autocode.vcxproj";

			// find the ms build
			const auto msBuildPath = GetMSBuildPath();
			if (msBuildPath.empty())
			{
				m_logOutput->Error("CodeGen: MSBuild toolchain not found");
				return false;
			}

			// save files
			for (uint32 i=0; i<m_files.size(); ++i)
			{
				const auto* file = m_files[i];
				m_logOutput->SetTaskProgress(i, (int)m_files.size());
				m_logOutput->SetTaskName("Saving file '%ls'...", file->m_fileName);

				std::wstring fullFilePath = fullTempPath + L"code/";
				fullFilePath += file->m_fileName;

				if (!file->m_codePrinter->Save(fullFilePath.c_str()))
				{
					m_logOutput->Error("CodeGen: Failed to save content to '%ls'", file->m_fileName);
					return false;
				}
			}

			// compile solution
			std::wstring commandLine;
			commandLine = L" \"";
			commandLine += makeFilePath;
			commandLine += L"\" ";
			commandLine += L"/ds /t:build /verbosity:minimal /nologo /p:Platform=x64 /p:Configuration=Build";

			// run the task
			const auto retCode = executor.RunExecutable(*m_logOutput, msBuildPath, commandLine);
			if (retCode != 0)
			{
				m_logOutput->Error("CodeGen: There were compilation errors");
				return false;
			}

			// we are done
			m_logOutput->Log("CodeGen: Stuff generated");
			return true;
		}

	} // msvc
} // code

#endif