#pragma once

#include "../host_core/blockAllocator.h"
#include "../host_core/bitset.h"

namespace xenon
{
	/// xenon memory allocator
	class Memory
	{
	public:
		Memory(native::IMemory& nativeMemory);

		// initialize virtual memory range
		const bool InitializeVirtualMemory(const uint32 prefferedBase, const uint32 totalVirtualMemoryAvaiable);

		// initialize physical memory range
		const bool InitializePhysicalMemory(const uint32 prefferedBase, const uint32 totalPhysicalMemoryAvaiable);

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
		static const uint32 PAGE_SIZE = 4096;
		static const uint32 MAX_MEMORY_SIZE = 0x40000000;
		static const uint32 MAX_PAGES = MAX_MEMORY_SIZE / PAGE_SIZE;

		// native memory system
		native::IMemory& m_nativeMemory;

		struct MemoryClass
		{
			std::mutex m_lock;

			void*					m_base;				// allocated base of the memory range
			uint32					m_totalSize;		// size of the allocated memory range
			uint32					m_totalPages;		// total number of memory pages
			uint32					m_allocatedPages;	// allocated number of memory pages
			utils::BlockAllocator	m_allocator;		// page allocator

			// get memory page from memory address	
			inline const uint32 PageFromAddress(const void* base) const
			{
				return  (uint32)(((uint8*)base - (uint8*)m_base) / PAGE_SIZE);
			}

			// get base address for memory page
			inline void* AddressFromPage(const uint32 page) const
			{
				return (uint8*)m_base + (page * PAGE_SIZE);
			}

			// get number of required pages for given size
			inline const uint32 PagesFromSize(const uint32 size) const
			{
				return (size + (PAGE_SIZE - 1)) / PAGE_SIZE;
			}

			// check if the pointer is contained in this memory class range
			inline const bool Contains(void* base, const uint32 size) const
			{
				if (((uint8*)base < (uint8*)m_base) || (((uint8*)base + size) > ((uint8*)m_base + m_totalSize)))
					return false;
				return true;
			}

			// initialize the memory range
			const bool Initialize(native::IMemory& memory, const uint32 prefferedBase, const uint32 totalVirtualMemoryAvaiable);

			// allocate memory
			void* Allocate(const uint32 size, const bool top, const uint32 flags);

			// free memory
			void Free(void* memory, uint32& outNumPages);
		};

		MemoryClass m_virtual;
		MemoryClass m_physical;
	};
} // xenon