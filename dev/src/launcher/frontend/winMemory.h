#pragma once

#include "..\backend\nativeMemory.h"

#include "winMemoryBlockAllocator.h"
#include <mutex>

namespace win
{
	/// memory handler
	class Memory : public native::IMemory
	{
	public:
		Memory();
		~Memory();

		virtual bool InitializeVirtualMemory(const uint32 totalVirtualMemoryAvaiable) override final;
		virtual bool InitializePhysicalMemory(const uint32 totalPhysicalMemoryAvaiable) override final;

		virtual bool IsVirtualMemory(void* base, const uint32 size) const override final;
		virtual bool IsVirtualMemoryReserved(void* base, const uint32 size) const override final;
		virtual bool IsVirtualMemoryComitted(void* base, const uint32 size) const override final;

		virtual uint32 GetVirtualMemoryProtectFlags(void* base) const override final;
		virtual uint32 GetVirtualMemoryAllocationSize(void* base) const override final;

		virtual void VirtualProtect(void* base, const uint32 size, const uint32 flags) override final;
		virtual void* VirtualAlloc(void* base, const uint32 size, const uint32 allocFlags, const uint32 protectFlags) override final;
		virtual bool VirtualFree(void* base, const uint32 size, const uint32 allocFlags, uint32& outFreedSize) override final;

		virtual void* PhysicalAlloc(const uint32 size, const uint32 alignment, const uint32 protectFlags) override final;
		virtual void PhysicalFree(void* base, const uint32 size) override final;
		virtual uint32 TranslatePhysicalAddress(const uint32 localAddress) const override final;

	private:
		static const uint32 VIRTUAL_PAGE_SIZE = 4096;
		static const uint32 VIRTUAL_MEMORY_BASE = 0x40000000;
		static const uint32 VIRTUAL_MEMORY_MAX = 512 << 20;

		static const uint32 PHYSICAL_PAGE_SIZE = 4096;
		static const uint32 PHYSICAL_MEMORY_LOW = 0xC0000000;
		static const uint32 PHYSICAL_MEMORY_HIGH = 0xE0000000;

		const uint32 PageFromAddress(const void* base) const
		{
			return  (uint32)(((uint8*)base - (uint8*)m_virtualMemoryBase) / VIRTUAL_PAGE_SIZE);
		}

		void* AddressFromPage(const uint32 page) const
		{
			return (uint8*)m_virtualMemoryBase + (page * VIRTUAL_PAGE_SIZE);
		}

		const uint32 PagesFromSize(const uint32 size) const
		{
			return (size + (VIRTUAL_PAGE_SIZE - 1)) / VIRTUAL_PAGE_SIZE;
		}

		typedef BlockAllocator< VIRTUAL_MEMORY_MAX / VIRTUAL_PAGE_SIZE >	TVirtualBlocks;
		typedef BitMask< VIRTUAL_MEMORY_MAX / VIRTUAL_PAGE_SIZE >			TVirtualComit;
		typedef BitMask< 4 * (VIRTUAL_MEMORY_MAX / VIRTUAL_PAGE_SIZE) >		TVirtualProtect;
		typedef BlockAllocator< (PHYSICAL_MEMORY_HIGH - PHYSICAL_MEMORY_LOW) / PHYSICAL_PAGE_SIZE >	TPhysicalBlocks;

		void*			m_virtualMemoryBase;			// allocated base of the memory range
		uint32			m_virtualMemorySize;			// size of the allocated memory range
		uint32			m_virtualMemoryPagesTotal;		// total number of memory pages
		uint32			m_virtualMemoryPagesAllocated;	// allocated number of memory pages
		uint32			m_virtualMemoryPagesComitted;	// number of commited virtual memory pages
		TVirtualBlocks	m_virtualMemoryAllocator;		// page allocator
		TVirtualComit	m_virtualMemoryCommitedPages;	// virtual pages that are comitted to physical memory
		TVirtualComit	m_virtualMemoryProtectedPages;	// virtual pages that are comitted to physical memory

		void*			m_physicalMemoryBase;			// allocated base of the physical memory range
		uint32			m_physicalMemorySize;			// size of the allocated physical memory range
		uint32			m_physicalMemoryPagesTotal;		// total number of physical memory pages
		uint32			m_physicalMemoryPagesAllocated;	// allocated number of physical memory pages
		TPhysicalBlocks	m_physicalMemoryAllocator;		// page allocator	

		uint32			m_tempShitPhysical;

		std::mutex		m_lock;						// lock for memory allocator
	};

} // win