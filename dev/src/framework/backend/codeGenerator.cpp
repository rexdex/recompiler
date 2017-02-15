#include "build.h"
#include "codeGenerator.h"
#include "codePrinter.h"
#include "internalUtils.h"
#include "zlib\zlib.h"
#include "decodingEnvironment.h"

namespace code
{

	//----------------------------------------------------------------------

	Generator::File::File(const wchar_t* fileName)
		: m_numBlocks( 0 )
		, m_numInstructions( 0 )
	{
		wcscpy_s( m_fileName, fileName );
	}

	Generator::File::~File()
	{
	}

	//----------------------------------------------------------------------

	Generator::Generator(class ILogOutput& log, const std::wstring& includeDirectory, const std::wstring& projectPath, const std::wstring& outPath, const decoding::CompilationMode mode, const decoding::CompilationPlatform platform, const uint64 entryAddress)
		: m_currentFile( NULL )
		, m_logOutput( &log )
		, m_blockBaseAddress( 0 )
		, m_totalNumBlocks( 0 )
		, m_totalNumInstructions( 0 )
		, m_isBlockMultiAddress( false )
		, m_inBlock( false )
		, m_hasImage( false )
		, m_imageBaseAddress( 0 )
		, m_imageUncompressedSize( 0 )
		, m_imageCompressedSize( 0 )
		, m_outputCodePath(outPath)
		, m_compilationMode(mode)
		, m_compilationPlatform(platform)
		, m_imageEntryAdrdress(entryAddress)
		, m_includeDirectory(includeDirectory)
	{
		m_codePrinter = new Printer();
		m_projectPath = projectPath;
	}

	Generator::~Generator()
	{
		DeleteVector( m_files );

		delete m_codePrinter;
		m_codePrinter = nullptr;
	}

	const bool Generator::CloseFile()
	{
		if (!CloseBlock())
			return false;

		if (m_currentFile)
		{
			std::wstring savePath = m_outputCodePath;
			savePath += m_currentFile->m_fileName;

			if (!m_codePrinter->Save(savePath.c_str()))
			{
				m_logOutput->Error( "CodeGen: Failed to save file '%ls'", savePath.c_str() );
				return false;
			}

			// stats
			m_logOutput->Log( "CodeGen: Saved file '%ls', %u blocks, %u instructions", 
				m_currentFile->m_fileName,
				m_currentFile->m_numBlocks,
				m_currentFile->m_numInstructions );

			m_currentFile = NULL;
		}

		m_codePrinter->Reset();
		return true;
	}

	const bool Generator::StartFile( const char* customFileName, const bool addIncludes /*= true*/ )
	{
		if (!CloseFile())
			return false;

		if ( customFileName && customFileName[0] )
		{
			const std::wstring tempFile(customFileName, customFileName+strlen(customFileName));
			m_currentFile = new File(tempFile.c_str());
		}
		else
		{
			wchar_t fileName[ 32 ];
			swprintf_s(fileName, ARRAYSIZE(fileName), L"autocode_%d.cpp", (int)m_files.size() );

			m_currentFile = new File(fileName);
		}

		m_files.push_back( m_currentFile );

		if (addIncludes && !m_includes.empty())
		{
			for (uint32 i = 0; i < m_includes.size(); ++i)
			{
				m_codePrinter->Printf("#include \"%ls\"\n", m_includes[i].c_str());
			}

			m_codePrinter->Printf("\n");

			m_codePrinter->Printf("extern runtime::IOBank ExportedIOBank;\n");
			m_codePrinter->Printf("\n");
		}

		m_inBlock = false;
		return true;
	}

	const bool Generator::CloseBlock()
	{
		// function tail
		if ( m_inBlock )
		{
			if ( m_isBlockMultiAddress )
			{
				m_codePrinter->Indent(-1);
				m_codePrinter->Print( "}\n");
			}

			m_codePrinter->Printf( "return 0x%08X;\n", m_blockReturnAddress );
			m_codePrinter->Indent(-1);
			m_codePrinter->Printf( "} // Block from %06Xh-%06Xh (%d instructions)\n", 
				m_blockBaseAddress,
				m_blockReturnAddress,
				(m_blockReturnAddress-m_blockBaseAddress)/4 );
			m_codePrinter->Printf( "\n" );

			// setup the return address (final address of the block)
			if ( !m_exportedBlocks.empty() && m_exportedBlocks.back().m_addressEnd == 0 )
			{
				m_exportedBlocks.back().m_addressEnd = m_blockReturnAddress;
			}

			m_blockReturnAddress = 0;
			m_blockBaseAddress = 0;
			m_isBlockMultiAddress = false;
			m_inBlock = false;
		}

		// block closed
		return true;
	}

	const bool Generator::AddInclude( const std::wstring& path, const bool discardPrevious )
	{
		if (discardPrevious)
			m_includes.clear();

		TIncludes::const_iterator it = std::find( m_includes.begin(), m_includes.end(), path );
		if ( it == m_includes.end() )
		{
			m_includes.push_back( path );
		}

		return true;
	}

	const bool Generator::AddImportSymbol( const char* name, const uint32 tableAddress, const uint32 entryAddress )
	{
		ImportInfo info;
		strcpy_s(info.m_importName, name);
		info.m_tableAddress = tableAddress;
		info.m_entryAddress = entryAddress;
		m_exportedSymbols.push_back(info);
		return true;
	}

	const bool Generator::AddImageData( ILogOutput& log, const void* imageData, const uint32 imageSize, const uint32 imageBaseAddress )
	{
		// already have an image
		if (m_hasImage)
			return true;

		// generate compressed image data
		log.SetTaskName( "Compressing image... ");
		CBinaryBigBuffer compresedImageData;
		if (!CompressLargeData( log, imageData, imageSize, compresedImageData))
			return false;

		// stats
		log.Log( "Decompile: Image compressed %u->%u bytes", 
			(uint32)imageSize, (uint32)compresedImageData.GetSize() );

		// setup data
		m_hasImage = true;
		m_imageBaseAddress = imageBaseAddress;
		m_imageCompressedSize = (uint32)compresedImageData.GetSize();
		m_imageUncompressedSize = imageSize;

		// create new file (image data)
		if ( !StartFile( "image.cpp" ) )
			return false;

		// Header
		m_codePrinter->Printf( "// This file contains the compressed copy of original image file to be decompressed and loaded by the runtime\n" );
		m_codePrinter->Printf( "// Compressed size = %d\n", m_imageCompressedSize );
		m_codePrinter->Printf( "// Uncompressed size = %d\n", m_imageUncompressedSize );
		m_codePrinter->Printf( "// Load address = %08Xh\n", m_imageBaseAddress );
		m_codePrinter->Print( "\n" );

		// Generate image data
		m_codePrinter->Printf( "const unsigned char CompressedImageData[%d] = {", m_imageCompressedSize );
		log.SetTaskName( "Exporting image data... ");
		for (uint32 i=0; i<m_imageCompressedSize; ++i)
		{
			log.SetTaskProgress(i, m_imageCompressedSize );

			if ((i & 31) == 0)
				m_codePrinter->Printf( "\n\t" );

			const uint8 val = compresedImageData.GetByte(i);
			m_codePrinter->Printf( "%d,", val );
		}

		m_codePrinter->Print( "\n}; // CompressedImageData\n\n");
		m_codePrinter->Print( "const unsigned char* CompressedImageDataStart = &CompressedImageData[0];\n\n" );

		// end file
		CloseFile();
		return true;
	}

	const bool Generator::AddGlueFile()
	{
		// create new file (glue logic file)
		if ( !StartFile( "main.cpp" ) )
			return false;

		// Windows include
		if (m_compilationPlatform == decoding::CompilationPlatform::VS2015)
		{
			m_codePrinter->Printf("#include <Windows.h>\n");
			m_codePrinter->Printf("\n");
		}

		// block exports
		if ( !m_exportedBlocks.empty() )
		{
			m_codePrinter->Printf( "// %d exported blocks\n", m_exportedBlocks.size() );
			for ( uint32 i=0; i<m_exportedBlocks.size(); ++i )
			{
				const char* blockSymbolName = m_exportedBlocks[i].m_name;
				m_codePrinter->Printf( "extern uint64 __fastcall %s( uint64 ip, cpu::CpuRegs& regs ); // 0x%08X - 0x%08X\n", 
					blockSymbolName,
					m_exportedBlocks[i].m_addressStart,
					m_exportedBlocks[i].m_addressEnd );
			}
			m_codePrinter->Printf( "\n" );
		}

		// io bank
		{
			m_codePrinter->Print("// input output bank\n");
			m_codePrinter->Print("runtime::IOBank ExportedIOBank = {\n");
			m_codePrinter->Print("   (runtime::TGlobalMemReadFunc) &cpu::UnhandledGlobalRead,\n ");
			m_codePrinter->Print("   (runtime::TGlobalMemWriteFunc) &cpu::UnhandledGlobalWrite,\n ");
			m_codePrinter->Print("   (runtime::TGlobalPortReadFunc) &cpu::UnhandledPortRead,\n ");
			m_codePrinter->Print("   (runtime::TGlobalPortWriteFunc) &cpu::UnhandledPortWrite,\n ");
			m_codePrinter->Print("};\n");
			m_codePrinter->Print("\n");
		}

		// interrupts
		if ( !m_exportedInterrupts.empty() )
		{		
			m_codePrinter->Printf( "// %d exported interrupt calls\n", m_exportedInterrupts.size() );
			m_codePrinter->Printf( "runtime::InterruptCall ExportedInterrupts[%d] = {\n", m_exportedInterrupts.size() ); 
			for ( uint32 i=0; i<m_exportedInterrupts.size(); ++i )
			{
				const InterruptInfo& info = m_exportedInterrupts[i];
				m_codePrinter->Printf( "{ %d, %d, (runtime::TInterruptFunc) &cpu::UnhandledInterruptCall }, // used %d times \n", 
					info.m_type,
					info.m_index,
					info.m_useCount );
			}
			m_codePrinter->Print( "};\n" );
			m_codePrinter->Print( "\n" );	

			// calling interface
			m_codePrinter->Print( "// interrupt handlers\n" );
			for ( uint32 i=0; i<m_exportedInterrupts.size(); ++i )
			{
				const InterruptInfo& info = m_exportedInterrupts[i];
				m_codePrinter->Printf( "void %s(uint64 ip, cpu::CpuRegs& regs) { (*ExportedInterrupts[%d].m_functionPtr)(ip, regs); }\n",
					info.m_name, i );
			}
			m_codePrinter->Print( "\n" );
		}

		// block exports
		if ( !m_exportedBlocks.empty() )
		{
			m_codePrinter->Printf( "runtime::BlockInfo ExportedBlocks[%d] = {\n", m_exportedBlocks.size() );

			for ( uint32 i=0; i<m_exportedBlocks.size(); ++i )
			{
				const BlockInfo& info = m_exportedBlocks[i];
				m_codePrinter->Printf( "\t{ 0x%08X, 0x%08X, (runtime::TBlockFunc) &%s }, \n",
					info.m_addressStart,
					(info.m_multiAddress) ? (info.m_addressEnd - info.m_addressStart) : 1,
					info.m_name );
			}

			m_codePrinter->Print( "};\n" );
			m_codePrinter->Print( "\n" );
		}	

		// function exports
		if ( !m_exportedSymbols.empty() )
		{
			m_codePrinter->Printf( "runtime::ImportInfo ExportedImports[%d] = {\n", m_exportedSymbols.size() );

			for ( uint32 i=0; i<m_exportedSymbols.size(); ++i )
			{
				const ImportInfo& info = m_exportedSymbols[i];

				m_codePrinter->Printf( "\t{ \"%s\", %d, 0x%08X }, \n",
					info.m_importName,
					(info.m_entryAddress==0) ? 0 : 1,
					(info.m_entryAddress==0) ? info.m_tableAddress : info.m_entryAddress );
			}

			m_codePrinter->Print( "};\n" );
			m_codePrinter->Print( "\n" );
		}

		// image info
		m_codePrinter->Print( "runtime::ImageInfo ExportImageInfo;\n\n" );

		// get the image info
		m_codePrinter->Print( "extern \"C\" __declspec(dllexport) void* GetImageInfo()\n" );
		m_codePrinter->Print( "{\n" );
		m_codePrinter->Print( "\treturn &ExportImageInfo;\n" );
		m_codePrinter->Print( "}\n" );
		m_codePrinter->Print( "\n" );

		// image data import
		if (m_hasImage)
		{
			m_codePrinter->Print( "// compressed image data buffer\n" );
			m_codePrinter->Print( "extern const unsigned char* CompressedImageDataStart;\n" );
			m_codePrinter->Print( "\n" );
		}

		// dll main - final glue code
		if (m_compilationPlatform == decoding::CompilationPlatform::VS2015)
		{
			m_codePrinter->Print("BOOL WINAPI DllMain( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved )\n");
			m_codePrinter->Print("{\n");
			m_codePrinter->Indent(1);
			m_codePrinter->Print("if (fdwReason != DLL_PROCESS_ATTACH) return TRUE;\n");
			m_codePrinter->Print("\n");
		}
		else
		{ 
			m_codePrinter->Print("INT InitializeCode()\n");
			m_codePrinter->Print("{\n");
			m_codePrinter->Indent(1);
		}

		// initialization
		{
			m_codePrinter->Print( "memset( &ExportImageInfo, 0, sizeof(ExportImageInfo) );\n" );
			if (m_hasImage)
			{
				m_codePrinter->Printf( "ExportImageInfo.m_imageLoadAddress = 0x%08X;\n", m_imageBaseAddress );
				m_codePrinter->Printf( "ExportImageInfo.m_imageCompressedSize = 0x%08X;\n", m_imageCompressedSize );
				m_codePrinter->Printf( "ExportImageInfo.m_imageUncompressedSize = 0x%08X;\n", m_imageUncompressedSize );
				m_codePrinter->Print( "ExportImageInfo.m_imageCompresedData = CompressedImageDataStart;\n" );
			}

			m_codePrinter->Printf( "ExportImageInfo.m_entryAddress = 0x%08X;\n", m_imageEntryAdrdress );

			// io bank
			m_codePrinter->Printf("ExportImageInfo.m_ioBank = &ExportedIOBank;\n", m_exportedInterrupts.size());

			// interrupts
			m_codePrinter->Printf( "ExportImageInfo.m_numInterrupts = %d;\n", m_exportedInterrupts.size() );
			m_codePrinter->Printf( "ExportImageInfo.m_interrupts = %s;\n", m_exportedInterrupts.size() ? "&ExportedInterrupts[0]" : "NULL" );

			// blocks
			m_codePrinter->Printf( "ExportImageInfo.m_numBlocks = %d;\n", m_exportedBlocks.size() );
			m_codePrinter->Printf( "ExportImageInfo.m_blocks = %s;\n", m_exportedBlocks.size() ? "&ExportedBlocks[0]" : "NULL" );

			// imports
			m_codePrinter->Printf( "ExportImageInfo.m_numImports = %d;\n", m_exportedSymbols.size() );
			m_codePrinter->Printf( "ExportImageInfo.m_imports = %s;\n", m_exportedSymbols.size() ? "&ExportedImports[0]" : "NULL" );

		}

		m_codePrinter->Print( "return 1;\n" );

		m_codePrinter->Indent( -1 );
		m_codePrinter->Print( "}\n" );	

		CloseFile();
		return true;
	}

	const bool Generator::AddInterruptCall( const uint32 type, const uint32 index, std::string& outSymbolName, std::string& outSignature )
	{
		// already defined ?
		for ( uint32 i=0; i<m_exportedInterrupts.size(); ++i )
		{
			InterruptInfo& info = m_exportedInterrupts[i];
			if ( info.m_type == type && info.m_index == index )
			{
				info.m_useCount += 1;
				outSymbolName = info.m_name;
				outSignature = info.m_signature;
				return true;
			}
		}

		// format symbol name
		char symbolName[256];
		sprintf_s( symbolName, "__interrupt_%u_%u", type, index );

		// format function call signature
		char symbolSignature[256];
		sprintf_s( symbolSignature, "extern void %s(const uint64 ip, cpu::CpuRegs& regs)", symbolName );

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
	}

	const bool Generator::StartBlock( const uint32 addr, const bool multiAddress, const char* optionalFunctionName )
	{
		// start new file
		if ( !m_currentFile || m_currentFile->m_numInstructions > 64000 )
		{
			if (!StartFile(NULL))
				return false;
		}

		// close previous block
		CloseBlock();

		// block header
		m_codePrinter->Printf( "//////////////////////////////////////////////////////\n" );
		m_codePrinter->Printf( "// Block at %06Xh\n", addr );
		if (optionalFunctionName)
			m_codePrinter->Printf( "// Function '%s'\n", optionalFunctionName );
		m_codePrinter->Printf( "//////////////////////////////////////////////////////\n" );

		// start block
		m_blockBaseAddress = addr;
		m_blockReturnAddress = addr; // nothing
		m_isBlockMultiAddress = multiAddress || (m_compilationMode == decoding::CompilationMode::Debug);
		m_lastMultiAddressSwitch = 0xFFFFFFFF;
		m_inBlock = true;

		// format block symbol
		char blockSymbolName[ 128 ];
		sprintf_s( blockSymbolName, "_code__block%08X", addr );

		// finish current block
		if ( !m_exportedBlocks.empty() && m_exportedBlocks.back().m_addressEnd == 0 )
		{
			m_exportedBlocks.back().m_addressEnd = addr;
		}

		// add new block
		BlockInfo info;
		strcpy_s(info.m_name, blockSymbolName);
		info.m_addressStart = addr;
		info.m_addressEnd = 0;
		info.m_multiAddress = multiAddress;
		m_exportedBlocks.push_back( info );

		// block function header
		m_codePrinter->Printf( "uint64 __fastcall %s( uint64 ip, cpu::CpuRegs& regs )\n", blockSymbolName );
		m_codePrinter->Print( "{\n" );
		m_codePrinter->Indent( 1 );

		// block addr switch
		if ( m_isBlockMultiAddress )
		{
			m_codePrinter->Printf( "const uint32 local_instr = (uint32)(ip - 0x%08X) / 4;\n", addr );
			m_codePrinter->Print( "switch ( local_instr )\n" );
			m_codePrinter->Print( "{\n" );
			m_codePrinter->Indent( 1 );

			// todo: optional multi address protection
			m_codePrinter->Printf( "default:\tcpu::invalid_address( ip, 0x%08X );\n", m_blockBaseAddress );
		}

		// stats
		m_totalNumBlocks += 1;
		m_currentFile->m_numBlocks += 1;
		return true;
	}

	const bool Generator::AddCodef( const uint32 addr, const char* txt, ... )
	{
		char buffer[ 8192 ];
		va_list args;

		va_start( args, txt );
		vsprintf_s( buffer, sizeof(buffer), txt, args );
		va_end( args );

		return AddCode( addr, buffer );
	}

	const bool Generator::AddCode( const uint32 addr, const char* code )
	{
		// WTF ?
		if ( !m_currentFile )
			return false;

		// no block ?
		if ( !m_inBlock )
			return false;

		// block addr switch
		if ( m_isBlockMultiAddress )
		{
			const uint32 switchIndex = (addr-m_blockBaseAddress) / 4;
			if ( switchIndex != m_lastMultiAddressSwitch )
			{
				m_lastMultiAddressSwitch = switchIndex;
				m_codePrinter->Printf( "  /* %08Xh */ case %4d:  \t\t", addr, switchIndex, addr );
			}
			else
			{
				m_codePrinter->Printf( "/* %08Xh case %4d:*/\t\t", addr, switchIndex, addr );
			}
		}

		// add code line
		m_codePrinter->Print( code );

		// advance the address pointer
		m_lastGeneratedCodeAddress = addr;
		m_blockReturnAddress = addr + 4; // addr + instr size, HACK

		// stats
		m_totalNumInstructions += 1;
		m_currentFile->m_numInstructions += 1;
		return true;
	}

	void Generator::AddVSMakeFile()
	{
		// create new file (project file)
		StartFile("autocode.vcxproj", false);

		// Platform name config name
		const std::string configName = decoding::GetCompilationModeName(m_compilationMode);
		const std::string platformName = decoding::GetCompilationPlatformName(m_compilationPlatform);

		// Executable name
		std::wstring imageFile = m_projectPath;
		imageFile += L".";
		imageFile += std::wstring(configName.begin(), configName.end());
		imageFile += L".";
		imageFile += std::wstring(platformName.begin(), platformName.end());
		imageFile += L".dll";

		// FAKE XML 
		m_codePrinter->Print("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
		m_codePrinter->Print("  <Project DefaultTargets=\"Build\" ToolsVersion=\"4.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n");
		m_codePrinter->Print("    <ItemGroup Label=\"ProjectConfigurations\">\n");
		m_codePrinter->Print("      <ProjectConfiguration Include=\"Build|x64\">\n");
		m_codePrinter->Print("        <Configuration>Build</Configuration>\n");
		m_codePrinter->Print("        <Platform>x64</Platform>\n");
		m_codePrinter->Print("      </ProjectConfiguration>\n");
		m_codePrinter->Print("    </ItemGroup>\n");
		m_codePrinter->Print("  <PropertyGroup Label=\"Globals\">\n");
		m_codePrinter->Print("    <ProjectGuid>{D9FC1B76-CB60-481B-B288-6EF63DE94065}</ProjectGuid>\n");
		m_codePrinter->Print("    <Keyword>Win32Proj</Keyword>\n");
		m_codePrinter->Print("    <RootNamespace>autocode</RootNamespace>\n");
		m_codePrinter->Print("  </PropertyGroup>\n");
		m_codePrinter->Print("  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.Default.props\" />\n");
		m_codePrinter->Print("  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Build|x64'\" Label=\"Configuration\">\n");
		m_codePrinter->Print("    <ConfigurationType>DynamicLibrary</ConfigurationType>\n");
		m_codePrinter->Print("    <UseDebugLibraries>false</UseDebugLibraries>\n");
		m_codePrinter->Printf("    <WholeProgramOptimization>false</WholeProgramOptimization>\n");
		m_codePrinter->Print("    <CharacterSet>Unicode</CharacterSet>\n");
		m_codePrinter->Print("    <PlatformToolset>v140</PlatformToolset>\n");
		m_codePrinter->Print("  </PropertyGroup>\n");
		m_codePrinter->Print("  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.props\" />\n");
		m_codePrinter->Print("  <ImportGroup Label=\"ExtensionSettings\">\n");
		m_codePrinter->Print("  </ImportGroup>\n");
		m_codePrinter->Print("  <ImportGroup Condition=\"'$(Configuration)|$(Platform)'=='Build|x64'\" Label=\"PropertySheets\">\n");
		m_codePrinter->Print("    <Import Project=\"$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\" Condition=\"exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')\" Label=\"LocalAppDataPlatform\" />\n");
		m_codePrinter->Print("  </ImportGroup>\n");
		m_codePrinter->Print("  <PropertyGroup Label=\"UserMacros\" />\n");
		m_codePrinter->Print("  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Build|x64'\">\n");
		m_codePrinter->Print("    <LinkIncremental>false</LinkIncremental>\n");
		m_codePrinter->Print("    <OutDir>..\\..\\temp\\$(Platform)\\$(Configuration)\\</OutDir>\n");
		m_codePrinter->Print("    <IntDir>..\\..\\temp\\$(Platform)\\$(Configuration)\\</IntDir>\n");
		m_codePrinter->Print("  </PropertyGroup>\n");
		m_codePrinter->Print("  <ItemDefinitionGroup Condition=\"'$(Configuration)|$(Platform)'=='Build|x64'\">\n");
		m_codePrinter->Print("    <ClCompile>\n");
		m_codePrinter->Print("      <WarningLevel>Level3</WarningLevel>\n");
		m_codePrinter->Print("      <PrecompiledHeader>\n");
		m_codePrinter->Print("      </PrecompiledHeader>\n");
		m_codePrinter->Print("      <Optimization>MaxSpeed</Optimization>\n");
		m_codePrinter->Print("      <FunctionLevelLinking>true</FunctionLevelLinking>\n");
		m_codePrinter->Print("      <IntrinsicFunctions>true</IntrinsicFunctions>\n");
		m_codePrinter->Print("      <PreprocessorDefinitions>WIN32;NDEBUG;_WINDOWS;_USRDLL;AUTOCODE_EXPORTS;%(PreprocessorDefinitions)</PreprocessorDefinitions>\n");
		m_codePrinter->Print("      <MultiProcessorCompilation>true</MultiProcessorCompilation>\n");
		m_codePrinter->Printf("      <AdditionalIncludeDirectories>%ls;%%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>\n", m_includeDirectory.c_str() );
		m_codePrinter->Print("    </ClCompile>\n");
		m_codePrinter->Print("    <Link>\n");
		m_codePrinter->Print("      <SubSystem>Windows</SubSystem>\n");
		m_codePrinter->Print("      <GenerateDebugInformation>true</GenerateDebugInformation>\n");
		m_codePrinter->Printf("    <EnableCOMDATFolding>false</EnableCOMDATFolding>\n");
		m_codePrinter->Print("      <OptimizeReferences>true</OptimizeReferences>\n");
		m_codePrinter->Printf("      <OutputFile>%ls</OutputFile>\n", imageFile.c_str());
		m_codePrinter->Print("    </Link>\n");
		m_codePrinter->Print("  </ItemDefinitionGroup>\n");
		m_codePrinter->Print("  <ItemGroup>\n");
		for (uint32 i = 0; i < m_files.size() - 1; ++i)
		{
			const wchar_t* fileName = m_files[i]->m_fileName;
			const std::string tempFile(fileName, fileName + wcslen(fileName));

			m_codePrinter->Printf("    <ClCompile Include=\"%s\" />\n",
				tempFile.c_str());
		}
		m_codePrinter->Print("  </ItemGroup>\n");
		m_codePrinter->Print("  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.targets\" />\n");
		m_codePrinter->Print("  <ImportGroup Label=\"ExtensionTargets\">\n");
		m_codePrinter->Print("  </ImportGroup>\n");
		m_codePrinter->Print("</Project>\n");

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

		if (GetKeyData(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\MSBuild\\ToolsVersions\\2.0", L"MSBuildToolsPath", msBuildPath, sizeof(msBuildPath)) )
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

	void Generator::FinishGeneration(std::vector< Task >& outBuildTasks)
	{
		// generate glue file and the make file (vcproj)
		CloseFile();
		AddGlueFile();

		// visual studio compilation
		if (m_compilationPlatform == decoding::CompilationPlatform::VS2015)
		{
			AddVSMakeFile();

			// return the solution file path
			std::wstring makeFilePath = m_outputCodePath;
			makeFilePath += L"autocode.vcxproj";

			// lookup the MSBUILD and create the task
			Task info;
			info.m_name = "Build";
			info.m_command = GetMSBuildPath();
			if (!info.m_command.empty())
			{
				info.m_arguments = L" \"";
				info.m_arguments += makeFilePath;
				info.m_arguments += L"\" ";
				info.m_arguments += L"/ds /t:build /verbosity:minimal /nologo /p:Platform=x64 /p:Configuration=Build";

				// add the task
				outBuildTasks.push_back(info);
			}
		}
	}

} // code