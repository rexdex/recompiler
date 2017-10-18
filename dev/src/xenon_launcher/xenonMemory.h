#pragma once
#include "../host_core/blockAllocator.h"
#include "../host_core/bitMask.h"

namespace xenon
{

	/// xenon memory allocator
	class Memory
	{
	public:
		Memory(native::IMemory& nativeMemory);

		// initialize virtual memory range
		const bool InitializeVirtualMemory(const uint32 totalVirtualMemoryAvaiable);

		// initialize physical memory range
		const bool InitializePhysicalMemory(const uint32 totalPhysicalMemoryAvaiable);

		//---

		// is this a valid virtual memory range ?
		const bool IsVirtualMemory(void* base, const uint32 size) const;

		// is this virtual memory reserved ?
		const bool IsVirtualMemoryReserved(void* base, const uint32 size) const;

		// get size of virtual memory allocation, zero if not allocated
		const uint32 GetVirtualMemoryAllocationSize(void* base) const;

		// get user flags for the memory (protection flags mostly)
		const uint64 GetVirtualMemoryFlags(void* base, const uint32 size) const;
				
		// set protection flag for range
		const void SetVirtualMemoryFlags(void* base, const uint32 size, const uint64 flags);

		// allocate virtual memory range
		void* VirtualAlloc(void* base, const uint32 size, const uint64 allocFlags, const uint64 protectFlags);

		// release virtual memory range (partial regions are supported, committed memory will be freed)
		const bool VirtualFree(void* base, const uint32 size, const uint64 allocFlags, uint32& outFreedSize);

		//--

		// allocate physical memory range
		void* PhysicalAlloc(const uint32 size, const uint32 alignment, const uint32 protectFlags);

		// free physical memory range
		void PhysicalFree(void* base, const uint32 size);

		// translate local physical memory address to absolute address
		const uint32 TranslatePhysicalAddress(const uint32 localAddress) const;

		//--

		// allocate small memory block that will be accessible by the simulated Xenon platform
		void* AllocateSmallBlock(const uint32 size);

		// free small memory block
		void FreeSmallBlock(void* memory);

	private:
		static const uint32 VIRTUAL_PAGE_SIZE = 4096;
		static const uint32 VIRTUAL_MEMORY_BASE = 0x40000000;
		static const uint32 VIRTUAL_MEMORY_MAX = 512 << 20;
		static const uint32 VIRTUAL_PAGES_MAX = VIRTUAL_MEMORY_MAX / VIRTUAL_PAGE_SIZE;

		static const uint32 PHYSICAL_PAGE_SIZE = 4096;
		static const uint32 PHYSICAL_MEMORY_LOW = 0xC0000000;
		static const uint32 PHYSICAL_MEMORY_HIGH = 0xE0000000;

		// native memory system
		native::IMemory& m_nativeMemory;

		// get memory page from memory address	
		const uint32 PageFromAddress(const void* base) const
		{
			return  (uint32)(((uint8*)base - (uint8*)m_virtualMemoryBase) / VIRTUAL_PAGE_SIZE);
		}

		// get base address for memory page
		void* AddressFromPage(const uint32 page) const
		{
			return (uint8*)m_virtualMemoryBase + (page * VIRTUAL_PAGE_SIZE);
		}

		// get number of required pages for given size
		const uint32 PagesFromSize(const uint32 size) const
		{
			return (size + (VIRTUAL_PAGE_SIZE - 1)) / VIRTUAL_PAGE_SIZE;
		}

		typedef utils::BlockAllocator< VIRTUAL_MEMORY_MAX / VIRTUAL_PAGE_SIZE >	TVirtualBlocks;
		typedef utils::BitMask< VIRTUAL_PAGES_MAX >	TVirtualCommit;
		typedef utils::BlockAllocator< (PHYSICAL_MEMORY_HIGH - PHYSICAL_MEMORY_LOW) / PHYSICAL_PAGE_SIZE >	TPhysicalBlocks;

		void*			m_virtualMemoryBase;			// allocated base of the memory range
		uint32			m_virtualMemorySize;			// size of the allocated memory range
		uint32			m_virtualMemoryPagesTotal;		// total number of memory pages
		uint32			m_virtualMemoryPagesAllocated;	// allocated number of memory pages
		uint32			m_virtualMemoryPagesComitted;	// number of committed virtual memory pages
		TVirtualBlocks	m_virtualMemoryAllocator;		// page allocator
		uint16			m_virtualMemoryFlags[VIRTUAL_PAGES_MAX]; // page flags
		TVirtualCommit	m_virtualMemoryCommitted;		// commit flags for page

		void*			m_physicalMemoryBase;			// allocated base of the physical memory range
		uint32			m_physicalMemorySize;			// size of the allocated physical memory range
		uint32			m_physicalMemoryPagesTotal;		// total number of physical memory pages
		uint32			m_physicalMemoryPagesAllocated;	// allocated number of physical memory pages
		TPhysicalBlocks	m_physicalMemoryAllocator;		// page allocator	

		uint32			m_tempShitPhysical;

		std::mutex		m_lock;						// lock for memory allocator
	};
} // xenon