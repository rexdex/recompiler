#include "build.h"
#include "runtimeImageInfo.h"
#include "runtimeImage.h"
#include "runtimeCodeTable.h"
#include "runtimeSymbols.h"
#include "launcherUtils.h"

namespace runtime
{

	Image::Image()
		: m_imageData(NULL)
		, m_imageSize(0)
		, m_codeLibrary(NULL)
		, m_codeTable(NULL)
	{
	}

	Image::~Image()
	{
		delete m_codeTable;
		m_codeTable = NULL;

		FreeLibrary(m_codeLibrary);
		m_codeLibrary = NULL;
	}

	uint64 __fastcall UnimplementedFunctionBlock(uint64 ip, RegisterBank& regs)
	{
		GLog.Err("Code: Unimplemented function at IP=%06Xh reached", ip);
		DEBUG_CHECK(!"Please write the stub :)");
		return 0;
	}

	bool Image::Load(const std::wstring& imageFilePath)
	{
		// load the code
		HMODULE hCodeLibrary = LoadLibraryW(imageFilePath.c_str());
		if (hCodeLibrary == NULL)
		{
			GLog.Err("Image: Failed to load code image from '%ls'", imageFilePath.c_str());
			return false;
		}

		// query the interface function
		runtime::TGetImageInfo getImageInfoFunc = (runtime::TGetImageInfo) GetProcAddress(hCodeLibrary, "GetImageInfo");
		if (getImageInfoFunc == NULL)
		{
			FreeLibrary(hCodeLibrary);
			GLog.Err("Image: Code image from '%ls' is not valid", imageFilePath.c_str());
			return false;
		}

		// request the image information
		const runtime::ImageInfo* imageInfo = getImageInfoFunc();
		if (imageInfo == NULL)
		{
			FreeLibrary(hCodeLibrary);
			GLog.Err("Image: Code image from '%ls' does not contain embedded code", imageFilePath.c_str());
			return false;
		}

		// stats
		GLog.Log("Image: Loaded code from '%ls'", imageFilePath.c_str());
		GLog.Log("Image: Image load address = %06Xh", imageInfo->m_imageLoadAddress);
		GLog.Log("Image: Image compressed size = %d", imageInfo->m_imageCompressedSize);
		GLog.Log("Image: Image uncompressed size = %d", imageInfo->m_imageUncompressedSize);
		GLog.Log("Image: Entry point = %06Xh", imageInfo->m_entryAddress);
		GLog.Log("Image: Number of code blocks = %d", imageInfo->m_numBlocks);
		GLog.Log("Image: Number of imports = %d", imageInfo->m_numImports);
		GLog.Log("Image: Global interrupts = %d", imageInfo->m_numInterrupts);

		// allocate the image memory
		const uint32 imageSize = imageInfo->m_imageUncompressedSize;
		void* imageData = VirtualAlloc((LPVOID)imageInfo->m_imageLoadAddress, imageSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		if (NULL == imageData)
		{
			GLog.Err("Image: Could not load original image at %06Xh", imageInfo->m_imageLoadAddress);
			FreeLibrary(hCodeLibrary);
			return false;
		}

		// load data
		uint32 imageDecompressedSize = imageSize;
		if (!launcher::DecompressData(imageInfo->m_imageCompresedData, imageInfo->m_imageCompressedSize, imageData, imageDecompressedSize)
			|| (imageDecompressedSize != imageSize))
		{
			VirtualFree(imageData, imageSize, MEM_RELEASE);
			GLog.Err("Image: Data decompression failed (%d!=%d)", imageDecompressedSize, imageSize);
			FreeLibrary(hCodeLibrary);
			return false;
		}

		// determine range of valid code
		uint64 minCodeAddress = imageInfo->m_imageLoadAddress;
		uint64 maxCodeAddress = imageInfo->m_entryAddress;
		for (uint32 i = 0; i < imageInfo->m_numBlocks; ++i)
		{
			const uint64 blockMinAddress = imageInfo->m_blocks[i].m_codeAddress;
			const uint64 blockMaxAddress = blockMinAddress + imageInfo->m_blocks[i].m_codeSize;

			minCodeAddress = std::min<uint64>(minCodeAddress, blockMinAddress);
			maxCodeAddress = std::max<uint64>(maxCodeAddress, blockMaxAddress);
		}

		// max function import
		const uint32 numImports = imageInfo->m_numImports;
		for (uint32 i = 0; i < numImports; ++i)
		{
			const runtime::ImportInfo& importInfo = imageInfo->m_imports[i];
			if (importInfo.m_type == 1) // function
			{
				const uint64  addr = importInfo.m_address;
				minCodeAddress = std::min<uint64>(minCodeAddress, addr);
				maxCodeAddress = std::max<uint64>(maxCodeAddress, addr + 16);
			}
		}

		// stats
		GLog.Log("Image: Executable code range: %06llXh-%06llXh", minCodeAddress, maxCodeAddress);

		// create code table and mount blocks and mount the blocks
		CodeTable* codeTable = new CodeTable(minCodeAddress, maxCodeAddress);
		for (uint32 i = 0; i < imageInfo->m_numBlocks; ++i)
		{
			const uint64 blockAddress = imageInfo->m_blocks[i].m_codeAddress;
			const uint32 blockSize = imageInfo->m_blocks[i].m_codeSize;

			runtime::TBlockFunc blockFunc = (runtime::TBlockFunc) imageInfo->m_blocks[i].m_functionPtr;
			if (!codeTable->MountBlock(blockAddress, blockSize, blockFunc))
			{
				GLog.Err("Image: Failed to mount code block %d at %06Xh", i, blockAddress);
			}
		}

		// stats
		GLog.Log("Image: Mounted %d code blocks", imageInfo->m_numBlocks);

		// setup
		m_imageInfo = imageInfo;
		m_imageData = imageData;
		m_imageSize = imageSize;
		m_codeLibrary = hCodeLibrary;
		m_codeTable = codeTable;
		return true;
	}

	bool Image::Bind(const Symbols& symbols)
	{
		bool status = true;

		// resolve functions
		uint32 numImportedFunctions = 0;
		uint32 numImportedSymbols = 0;
		uint32 numUnknownFunctions = 0;
		uint32 numUnknownSymbols = 0;
		const uint32 numImports = m_imageInfo->m_numImports;
		for (uint32 i = 0; i < numImports; ++i)
		{
			const runtime::ImportInfo& importInfo = m_imageInfo->m_imports[i];

			// function
			if (importInfo.m_type == 1)
			{
				// find function code prototype
				runtime::TBlockFunc importCode = symbols.FindFunctionCode(importInfo.m_name);
				if (!importCode)
				{
					numUnknownFunctions += 1;
					GLog.Warn("Image: Unimplemented import function '%s' at 0x%08X. Crash possible.", importInfo.m_name, importInfo.m_address);

					// mount unimplemented functio0n
					m_codeTable->MountBlock(importInfo.m_address, 4, &UnimplementedFunctionBlock, true);
					continue;
				}

				// bind to code table
				if (m_codeTable->MountBlock(importInfo.m_address, 4, importCode, true))
				{
					numImportedFunctions += 1;
				}
				else
				{
					GLog.Err("Image: Failed to mount code block for import function '%s'.", importInfo.m_name);
					status = false;
				}
			}

			// symbol
			else if (importInfo.m_type == 0)
			{
				// find explicit symbol address
				uint64 symbolDataAddress = symbols.FindSymbolAddress(importInfo.m_name);

				// may be a function...
				if (symbolDataAddress == 0)
				{
					for (uint32 j = 0; j < numImports; ++j)
					{
						const runtime::ImportInfo& functionImportInfo = m_imageInfo->m_imports[j];
						if (functionImportInfo.m_type == 1 && (0 == strcmp(functionImportInfo.m_name, importInfo.m_name)))
						{
							symbolDataAddress = functionImportInfo.m_address;
							break;
						}
					}
				}

				// unknown symbol value :*(
				if (!symbolDataAddress)
				{
					numUnknownSymbols += 1;
					GLog.Warn("Image: Unknown address of symbol '%s'. Crash possible.", importInfo.m_name);
					continue;
				}

				// check patch range
				if (importInfo.m_address < m_imageInfo->m_imageLoadAddress || importInfo.m_address >= (m_imageInfo->m_imageLoadAddress + m_imageSize))
				{
					GLog.Err("Image: Patch address for symbol '%s' lies outside image memory (%06Xh)", importInfo.m_name, importInfo.m_address);
					continue;
				}

				// set the symbol address value directly in the image memory (patch image)
				// TODO: expose to platform
				*(uint32*)((char*)m_imageData + (importInfo.m_address - m_imageInfo->m_imageLoadAddress)) = _byteswap_ulong((uint32)symbolDataAddress); // TODO: byteswapping depends on the platform
				//GLog.Log( "Image: Patched %06Xh with %06Xh", importInfo.m_address, symbolDataAddress );
				numImportedSymbols += 1;
			}
		}

		// patch the interrupts
		for (uint32 i = 0; i < m_imageInfo->m_numInterrupts; ++i)
		{
			auto& info = m_imageInfo->m_interrupts[i];
			if ( info.m_type != 0 )
			{ 
				GLog.Err("Image: Unsupported interrupt type %u interrupt %u", info.m_type, info.m_index);
				status = false;
				continue;
			}

			// find the interrupt handler
			const auto handler = symbols.FindInterruptCallback(info.m_index);
			if (!handler)
			{
				GLog.Err("Image: Missing interrupt handler for interrupt %u", info.m_index);
				status = false;
				continue;
 			}

			// bind
			info.m_functionPtr = handler;
		}

		// stats
		GLog.Log("Image: Imported and patched %d functions and %d symbols", numImportedFunctions, numImportedSymbols);
		if (numUnknownFunctions)
			GLog.Err("Image: %d unknown functions", numUnknownFunctions);
		if (numUnknownSymbols)
			GLog.Err("Image: %d unknown symbols", numUnknownSymbols);

		return status;
	}

} // runtime