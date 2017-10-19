#pragma once

#include "launcherBase.h"

namespace runtime
{
	class RegisterBank;

	typedef void(*TGlobalMemReadFunc)(const uint64_t ip, const uint64_t address, const uint64_t size, void* outPtr);
	typedef void(*TGlobalMemWriteFunc)(const uint64_t ip, const uint64_t address, const uint64_t size, const void* inPtr);
	typedef void(*TGlobalPortReadFunc)(const uint64_t ip, const uint16_t portIndex, const uint64_t size, void* outPtr);
	typedef void(*TGlobalPortWriteFunc)(const uint64_t ip, const uint16_t portIndex, const uint64_t size, const void* inPtr);
	typedef void(*TInterruptFunc)(const uint64_t ip, const uint32_t index, const RegisterBank& regs);

	typedef uint64 (__fastcall *TBlockFunc)(const uint64_t ip, RegisterBank& regs);

	struct InterruptCall
	{
		uint32				m_type;
		uint32				m_index;
		TInterruptFunc		m_functionPtr;
	};

	struct BlockInfo
	{
		uint64		m_codeAddress;
		uint32		m_codeSize;
		void*		m_functionPtr;
	};

	struct ImportInfo
	{
		const char*	m_name;
		uint8		m_type; // 0-var, 1-function
		uint32		m_address; // 0-var placement address (in process memory) 1-function entry point
	};

	struct ImageInfo
	{
		uint64				m_imageLoadAddress;
		uint32				m_imageCompressedSize;
		uint32				m_imageUncompressedSize;
		const uint8*		m_imageCompresedData;

		uint64				m_entryAddress;

		uint32				m_initialHeapReservedSize;
		uint32				m_initialHeapCommitedSize;
		uint64				m_initialHeapAddress;

		uint32				m_initialStackSize;
		uint64				m_initialStackAddress;

		uint32				m_numBlocks;
		BlockInfo*			m_blocks;

		uint32				m_numImports;
		ImportInfo*			m_imports;

		uint32				m_numInterrupts;
		InterruptCall*		m_interrupts;
	};	

	typedef ImageInfo* (__stdcall *TGetImageInfo)();
};

