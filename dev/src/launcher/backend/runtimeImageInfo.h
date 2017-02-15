#pragma once

#include "launcherBase.h"

namespace runtime
{
	class RegisterBank;

	typedef void(*TGlobalMemReadFunc)(uint64 address, const uint32 size, void* outPtr);
	typedef void(*TGlobalMemWriteFunc)(uint64 address, const uint32 size, const void* inPtr);
	typedef void(*TGlobalPortReadFunc)(uint16 portIndex, const uint32 size, void* outPtr);
	typedef void(*TGlobalPortWriteFunc)(uint16 portIndex, const uint32 size, const void* inPtr);
	typedef void(*TInterruptFunc)(uint64 ip, RegisterBank& regs);

	typedef uint64 (__fastcall *TBlockFunc)(uint64 ip, RegisterBank& regs);

	struct IOBank
	{
		TGlobalMemReadFunc		m_memReadPtr;
		TGlobalMemWriteFunc		m_memWritePtr;
		TGlobalPortReadFunc		m_portReadPtr;
		TGlobalPortWriteFunc	m_portWritePtr;
	};

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

		IOBank*				m_ioBank;

		uint32				m_numInterrupts;
		InterruptCall*		m_interrupts;
	};	

	typedef ImageInfo* (__stdcall *TGetImageInfo)();
};

