#include "build.h"
#include "xenonMemory.h"

namespace xenon
{
	Memory::Memory(native::IMemory& nativeMemory)
		: m_virtualMemoryBase(NULL)
		, m_virtualMemorySize(0)
		, m_virtualMemoryPagesTotal(0)
		, m_virtualMemoryPagesAllocated(0)
		, m_virtualMemoryPagesComitted(0)
		, m_physicalMemoryBase(NULL)
		, m_physicalMemorySize(0)
		, m_physicalMemoryPagesTotal(0)
		, m_physicalMemoryPagesAllocated(0)
		, m_tempShitPhysical(0)
		, m_nativeMemory(nativeMemory)
	{}

	const bool Memory::InitializeVirtualMemory(const uint32 totalVirtualMemoryAvaiable)
	{
		std::lock_guard<std::mutex> lock(m_lock);

		// already initialized
		if (NULL != m_virtualMemoryBase)
		{
			GLog.Err("Mem: Virtual memory already initialized");
			return false;
		}

		// allocate the virtual address space range that will be used as virtual memory
		m_virtualMemoryBase = m_nativeMemory.AllocVirtualMemory(VIRTUAL_MEMORY_BASE, totalVirtualMemoryAvaiable);
		if (NULL == m_virtualMemoryBase)
		{
			GLog.Warn("Mem: Failed to initialize virtual memory range at preferred address %06Xh", VIRTUAL_MEMORY_BASE);

			// try any address
			m_virtualMemoryBase = m_nativeMemory.AllocVirtualMemory(0, totalVirtualMemoryAvaiable);
			if (NULL == m_virtualMemoryBase)
			{
				GLog.Err("Mem: Failed to initialize virtual of size %d (%1.2f MB). Error = %08Xh",
					totalVirtualMemoryAvaiable,
					(float)totalVirtualMemoryAvaiable / (1024.0f*1024.0f),
					GetLastError());

				return false;
			}
		}

		// setup virtual memory data
		m_virtualMemorySize = totalVirtualMemoryAvaiable;
		m_virtualMemoryPagesTotal = totalVirtualMemoryAvaiable / VIRTUAL_PAGE_SIZE;
		m_virtualMemoryPagesAllocated = 0;
		m_virtualMemoryPagesComitted = 0;

		// clear memory flags
		memset(&m_virtualMemoryFlags, 0, sizeof(m_virtualMemoryFlags));

		// setup block allocator
		m_virtualMemoryAllocator.Initialize(m_virtualMemoryPagesTotal);

		// stats
		GLog.Log("Mem: Initialized virtual memory range at %06Xh, size %1.2f MB (%d pages)",
			(uint32)m_virtualMemoryBase,
			(float)totalVirtualMemoryAvaiable / (1024.0f*1024.0f),
			m_virtualMemoryPagesTotal);

		return true;
	}

	const bool Memory::InitializePhysicalMemory(const uint32 totalPhysicalMemoryAvaiable)
	{
		std::lock_guard<std::mutex> lock(m_lock);

		// already initialized
		if (NULL != m_physicalMemoryBase)
		{
			GLog.Err("Mem: Physical memory already initialized");
			return false;
		}

		// allocate the virtual address space range that will be used as virtual memory
		m_physicalMemoryBase = m_nativeMemory.AllocVirtualMemory(PHYSICAL_MEMORY_LOW, totalPhysicalMemoryAvaiable);
		if (NULL == m_physicalMemoryBase)
		{
			GLog.Warn("Mem: Failed to initialize physical memory range at preferred address %06Xh", PHYSICAL_MEMORY_LOW);

			// try any address
			m_physicalMemoryBase = m_nativeMemory.AllocVirtualMemory(0, totalPhysicalMemoryAvaiable);
			if (NULL == m_virtualMemoryBase)
			{
				GLog.Err("Mem: Failed to initialize physical memory of size %d (%1.2f MB)",
					totalPhysicalMemoryAvaiable,
					(float)totalPhysicalMemoryAvaiable / (1024.0f*1024.0f));

				return false;
			}
		}

		// cleanup
		memset(m_physicalMemoryBase, 0xCC, totalPhysicalMemoryAvaiable);

		// setup physical memory data
		m_physicalMemorySize = totalPhysicalMemoryAvaiable;
		m_physicalMemoryPagesTotal = totalPhysicalMemoryAvaiable / PHYSICAL_PAGE_SIZE;
		m_physicalMemoryPagesAllocated = 0;

		// initialize page allocator
		m_physicalMemoryAllocator.Initialize(m_physicalMemoryPagesTotal);

		// stats
		GLog.Log("Mem: Initialized physical memory range at %06Xh, size %1.2f MB (%d pages)",
			(uint32)m_physicalMemoryBase,
			(float)totalPhysicalMemoryAvaiable / (1024.0f*1024.0f),
			m_physicalMemoryPagesTotal);

		return true;
	}

	const bool Memory::IsVirtualMemory(void* base, const uint32 size) const
	{
		if (((uint8*)base < (uint8*)m_virtualMemoryBase) ||
			(((uint8*)base + size) > ((uint8*)m_virtualMemoryBase + m_virtualMemorySize)))
		{
			return false;
		}

		return true;
	}

	const bool Memory::IsVirtualMemoryReserved(void* base, const uint32 size) const
	{
		if (!IsVirtualMemory(base, size))
			return false;

		const uint32 firstPage = (uint32)(((uint8*)base - (uint8*)m_virtualMemoryBase) / VIRTUAL_PAGE_SIZE);
		const uint32 numPages = (size + (VIRTUAL_PAGE_SIZE - 1)) / VIRTUAL_PAGE_SIZE;
		return m_virtualMemoryAllocator.IsAllocated(firstPage, numPages);
	}

	const uint64 Memory::GetVirtualMemoryFlags(void* base, const uint32 size) const
	{
		if (!IsVirtualMemory(base, size))
			return false;

		const uint32 firstPage = PageFromAddress(base);
		return m_virtualMemoryFlags[firstPage];
	}

	const void Memory::SetVirtualMemoryFlags(void* base, const uint32 size, const uint64 flags)
	{
		DEBUG_CHECK(flags <= 0xFFFF);

		if (!IsVirtualMemory(base, size))
			return;

		const uint32 firstPage = PageFromAddress(base);
		m_virtualMemoryFlags[firstPage] = (uint16)flags;
	}

	const uint32 Memory::GetVirtualMemoryAllocationSize(void* base) const
	{
		if (!IsVirtualMemory(base, 1))
			return 0;

		const uint32 firstPage = PageFromAddress(base);
		if (!m_virtualMemoryAllocator.IsAllocated(firstPage, 1))
			return 0;

		uint32 numPages = 0;
		m_virtualMemoryAllocator.GetNumAllocatedBlocks(firstPage, numPages);
		return (VIRTUAL_PAGE_SIZE * numPages);
	}

	void* Memory::VirtualAlloc(void* base, const uint32 size, const uint64 allocFlags, const uint64 protectFlags)
	{
		std::lock_guard<std::mutex> lock(m_lock);

		// allocation
		if (allocFlags & xnative::XMEM_RESERVE || (base == NULL))
		{
			// auto alloc
			if (!(allocFlags & xnative::XMEM_RESERVE))
			{
				GLog.Spam("VMEM: Auto allocating (size=%d)", size);
			}

			// try direct allocation
			if (base != NULL)
			{
				GLog.Spam("VMEM: Trying to allocate at non-zero base %06Xh (size=%d). Not supported.", (uint32)base, size);
				return NULL;
			}

			// page count
			uint32 firstPageIndex = 0;
			const bool top = 0 != (allocFlags & xnative::XMEM_TOP_DOWN);
			const uint32 numPages = PagesFromSize(size);
			if (!m_virtualMemoryAllocator.AllocateBlock(numPages, top, firstPageIndex))
			{
				GLog.Err("VMEM: Virtual memory allocation of %d (%1.2fKB) failed",
					size, (float)size / 1024.0f);

				return NULL;
			}

			// setup the base pointer
			base = AddressFromPage(firstPageIndex);
			GLog.Spam("VMEM: Virtual memory allocated (%1.2fKB, %d pages) at %06Xh",
				size / 1024.0f, numPages, (uint32)base);

			// stats
			m_virtualMemoryPagesAllocated += numPages;
		}

		// should we commit the memory ?
		if (allocFlags & xnative::XMEM_COMMIT)
		{
			// invalid memory
			if (!base)
			{
				GLog.Err("VMEM: Virtual memory commit called with NULL base");
				return NULL;
			}

			// check the range
			if (!IsVirtualMemory(base, size))
			{
				GLog.Err("VMEM: Trying to commit memory range %06Xh, size %d which is not valid virtual memory range", (uint32)base, size);
				return NULL;
			}

			// commit only the uncommitted pages
			uint32 numPagesComitted = 0;
			const uint32 firstPage = PageFromAddress(base);
			const uint32 numPages = PagesFromSize(size);
			for (uint32 i = 0; i < numPages; ++i)
			{
				const uint32 pageIndex = firstPage + i;
				if (!m_virtualMemoryCommitted.IsSet(pageIndex))
				{
					// do the work
					uint8* pageBase = (uint8*)m_virtualMemoryBase + pageIndex * VIRTUAL_PAGE_SIZE;
					if (NULL == ::VirtualAlloc(pageBase, VIRTUAL_PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE))
					{
						GLog.Err("VMEM: OS commit of page %d failed", pageIndex);
						return NULL;
					}

					// fill with pattern
					//memset( pageBase, 0xFE, VIRTUAL_PAGE_SIZE );

					// mark as committed
					m_virtualMemoryCommitted.Set(pageIndex);
					numPagesComitted += 1;
				}
			}

			// new pages committed ?
			if (numPagesComitted > 0)
			{
				GLog.Spam("VMEM: Virtual memory committed (%1.2fKB, %d pages, %d new) at %06Xh",
					size / 1024.0f, numPages, numPagesComitted, (uint32)base);

				m_virtualMemoryPagesComitted += numPagesComitted;
			}
		}

		// set default protection flags
		SetVirtualMemoryFlags(base, size, protectFlags);

		// return base
		return base;
	}

	const bool Memory::VirtualFree(void* base, const uint32 initSize, const uint64 allocFlags, uint32& outFreedSize)
	{
		std::lock_guard<std::mutex> lock(m_lock);

		// invalid memory
		if (!base)
		{
			GLog.Err("VMEM: Virtual memory free called with NULL base");
			return false;
		}

		// size not specified
		uint32 size = initSize;
		if (!size)
		{
			// get size of the allocated memory block
			const uint32 firstPage = PageFromAddress(base);
			uint32 numBlocks = 0;
			if (!m_virtualMemoryAllocator.GetNumAllocatedBlocks(firstPage, numBlocks))
			{
				GLog.Err("VMEM: Unable to determine number of pages allocated for address %08Xh (page %d)", (uint32)base, firstPage);
			}

			// update the allocation size
			size = numBlocks * VIRTUAL_PAGE_SIZE;
			GLog.Spam("VMEM: Determined size of allocation at %08Xh to be %d bytes (%d pages)", (uint32)base, size, numBlocks);
		}

		// should we decommit the memory ? always decommit on release
		if (allocFlags & (xnative::XMEM_DECOMMIT | xnative::XMEM_RELEASE))
		{
			// check the range
			if (!IsVirtualMemory(base, size))
			{
				GLog.Err("VMEM: Trying to decommit memory range %06Xh, size %d which is not valid virtual memory range", (uint32)base, size);
				return false;
			}

			// decommit only the committed pages
			uint32 numPagesDecomitted = 0;
			const uint32 firstPage = PageFromAddress(base);
			const uint32 numPages = PagesFromSize(size);
			for (uint32 i = 0; i < numPages; ++i)
			{
				const uint32 pageIndex = firstPage + i;
				if (m_virtualMemoryCommitted.IsSet(pageIndex))
				{
					// do the work
					uint8* pageBase = (uint8*)m_virtualMemoryBase + pageIndex * VIRTUAL_PAGE_SIZE;
					if (!::VirtualFree(pageBase, VIRTUAL_PAGE_SIZE, MEM_DECOMMIT))
					{
						GLog.Err("VMEM: OS decommit of page %d failed", pageIndex);
						return NULL;
					}

					// mark as committed
					m_virtualMemoryCommitted.Clear(pageIndex);
					numPagesDecomitted += 1;
				}
			}

			// new pages committed ?
			if (numPagesDecomitted > 0)
			{
				GLog.Spam("VMEM: Virtual memory decommitted (%1.2fKB, %d pages, %d new) at %06Xh",
					size / 1024.0f, numPages, numPagesDecomitted, (uint32)base);

				m_virtualMemoryPagesComitted -= numPagesDecomitted;
			}

			outFreedSize = size;
		}

		// release
		if (allocFlags & xnative::XMEM_RELEASE)
		{
			const uint32 firstPage = PageFromAddress(base);
			const uint32 numPages = PagesFromSize(size);
			if (!m_virtualMemoryAllocator.FreeBlock(firstPage, numPages))
			{
				GLog.Err("VMEM: Failed to release memory at %06Xh", (uint32)base);
				return false;
			}

			// stats
			m_virtualMemoryPagesAllocated -= numPages;
			outFreedSize = size;
		}

		// done
		return true;
	}

	void* Memory::PhysicalAlloc(const uint32 size, const uint32 alignment, const uint32 protectFlags)
	{
		std::lock_guard<std::mutex> lock(m_lock);

		GLog.Log("PMEM: Allocating physical memory (size=%d, alignment=%d,flags=%d)", size, alignment, protectFlags);

		// tempshit allocation
		const uint32 addr = (m_tempShitPhysical + (alignment - 1)) & (~(alignment - 1));
		m_tempShitPhysical += size;
		void* ptr = (char*)m_physicalMemoryBase + addr;//m_physicalMemorySize - addr - size;
		return ptr;
	}

	void Memory::PhysicalFree(void* base, const uint32 size)
	{
		// TODO!
	}

	const uint32 Memory::TranslatePhysicalAddress(const uint32 localAddress) const
	{
		const uint32 physicalMemoryAddrMask = 0x1FFFFFFF;
		return (const uint32)m_physicalMemoryBase + (localAddress & physicalMemoryAddrMask);
	}

	//--

	void* Memory::AllocateSmallBlock(const uint32 size)
	{
		// i'm burning in hell for this
		// TODO: proper allocator for small blocks (HeapAlloc)
		return VirtualAlloc(0, size, xnative::XMEM_RESERVE | xnative::XMEM_COMMIT, xnative::XPAGE_READWRITE);
	}

	void Memory::FreeSmallBlock(void* memory)
	{
		uint32 freedSize = 0;
		VirtualFree(memory, 0, xnative::XMEM_FREE | xnative::XMEM_DECOMMIT | xnative::XMEM_RELEASE, freedSize);
	}

	//---

} // xenon