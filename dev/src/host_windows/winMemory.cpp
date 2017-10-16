#include "build.h"
#include "winMemory.h"

#pragma warning( disable: 4312 )
#pragma warning( disable: 4311 )
#pragma warning( disable: 4302 )

namespace win
{

	Memory::Memory()
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
	{
	}

	Memory::~Memory()
	{
	}

	bool Memory::InitializeVirtualMemory(const uint32 totalVirtualMemoryAvaiable)
	{
		std::lock_guard<std::mutex> lock(m_lock);

		// already initialized
		if (NULL != m_virtualMemoryBase)
		{
			GLog.Err("Mem: Virtual memory already initialized");
			return false;
		}

		// allocate the virtual address space range that will be used as virtual memory
		m_virtualMemoryBase = ::VirtualAlloc((void*)VIRTUAL_MEMORY_BASE, totalVirtualMemoryAvaiable, MEM_RESERVE, PAGE_READWRITE);
		if (NULL == m_virtualMemoryBase)
		{
			GLog.Warn("Mem: Failed to initialize virtual memory range at preferred address %06Xh", VIRTUAL_MEMORY_BASE);

			// try any address
			m_virtualMemoryBase = ::VirtualAlloc(NULL, totalVirtualMemoryAvaiable, MEM_RESERVE, PAGE_READWRITE);
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

		// setup block allocator
		m_virtualMemoryAllocator.Initialize(m_virtualMemoryPagesTotal);

		// stats
		GLog.Log("Mem: Initialized virtual memory range at %06Xh, size %1.2f MB (%d pages)",
			(uint32)m_virtualMemoryBase,
			(float)totalVirtualMemoryAvaiable / (1024.0f*1024.0f),
			m_virtualMemoryPagesTotal);

		return true;
	}

	bool Memory::InitializePhysicalMemory(const uint32 totalPhysicalMemoryAvaiable)
	{
		std::lock_guard<std::mutex> lock(m_lock);

		// already initialized
		if (NULL != m_physicalMemoryBase)
		{
			GLog.Err("Mem: Physical memory already initialized");
			return false;
		}

		// allocate the virtual address space range that will be used as virtual memory
		m_physicalMemoryBase = ::VirtualAlloc((void*)PHYSICAL_MEMORY_LOW, totalPhysicalMemoryAvaiable, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		if (NULL == m_physicalMemoryBase)
		{
			GLog.Warn("Mem: Failed to initialize physical memory range at preferred address %06Xh", PHYSICAL_MEMORY_LOW);

			// try any address
			m_physicalMemoryBase = ::VirtualAlloc(NULL, totalPhysicalMemoryAvaiable, MEM_RESERVE, PAGE_READWRITE);
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

	bool Memory::IsVirtualMemory(void* base, const uint32 size) const
	{
		if (((uint8*)base < (uint8*)m_virtualMemoryBase) ||
			(((uint8*)base + size) >((uint8*)m_virtualMemoryBase + m_virtualMemorySize)))
		{
			return false;
		}

		return true;
	}

	bool Memory::IsVirtualMemoryReserved(void* base, const uint32 size) const
	{
		if (!IsVirtualMemory(base, size))
			return false;

		const uint32 firstPage = (uint32)(((uint8*)base - (uint8*)m_virtualMemoryBase) / VIRTUAL_PAGE_SIZE);
		const uint32 numPages = (size + (VIRTUAL_PAGE_SIZE - 1)) / VIRTUAL_PAGE_SIZE;
		return m_virtualMemoryAllocator.IsAllocated(firstPage, numPages);
	}

	bool Memory::IsVirtualMemoryComitted(void* base, const uint32 size) const
	{
		if (!IsVirtualMemory(base, size))
			return false;

		const uint32 firstPage = (uint32)(((uint8*)base - (uint8*)m_virtualMemoryBase) / VIRTUAL_PAGE_SIZE);
		const uint32 numPages = (size + (VIRTUAL_PAGE_SIZE - 1)) / VIRTUAL_PAGE_SIZE;
		for (uint32 i = 0; i < numPages; ++i)
		{
			if (m_virtualMemoryCommitedPages.IsSet(firstPage + i))
				return true;
		}

		return false;
	}

	uint32 Memory::GetVirtualMemoryProtectFlags(void* base) const
	{
		if (!IsVirtualMemory(base, 1))
			return false;

		const uint32 pageIndex = (uint32)(((uint8*)base - (uint8*)m_virtualMemoryBase) / VIRTUAL_PAGE_SIZE);
		uint32 flags = 0;
		flags |= m_virtualMemoryProtectedPages.IsSet(pageIndex * 4 + 0) << 0;
		flags |= m_virtualMemoryProtectedPages.IsSet(pageIndex * 4 + 1) << 1;
		flags |= m_virtualMemoryProtectedPages.IsSet(pageIndex * 4 + 2) << 2;
		flags |= m_virtualMemoryProtectedPages.IsSet(pageIndex * 4 + 3) << 3;
		return flags;
	}

	void Memory::VirtualProtect(void* base, const uint32 size, const uint32 flags)
	{
		if (!IsVirtualMemory(base, size))
			return;

		const uint32 firstPage = (uint32)(((uint8*)base - (uint8*)m_virtualMemoryBase) / VIRTUAL_PAGE_SIZE);
		const uint32 numPages = (size + (VIRTUAL_PAGE_SIZE - 1)) / VIRTUAL_PAGE_SIZE;
		for (uint32 i = 0; i < numPages; ++i)
		{
			const uint32 pageFlagIndex = (firstPage + i);
			m_virtualMemoryProtectedPages.Toggle(pageFlagIndex + 0, 0 != (flags & 1));
			m_virtualMemoryProtectedPages.Toggle(pageFlagIndex + 1, 0 != (flags & 2));
			m_virtualMemoryProtectedPages.Toggle(pageFlagIndex + 2, 0 != (flags & 4));
			m_virtualMemoryProtectedPages.Toggle(pageFlagIndex + 3, 0 != (flags & 8));
		}
	}

	uint32 Memory::GetVirtualMemoryAllocationSize(void* base) const
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

	void* Memory::VirtualAlloc(void* base, const uint32 size, const uint32 allocFlags, const uint32 protectFlags)
	{
		std::lock_guard<std::mutex> lock(m_lock);

		// allocation
		if (allocFlags & eAlloc_Reserve || (base == NULL))
		{
			// auto alloc
			if (!(allocFlags & eAlloc_Reserve))
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
			const bool top = 0 != (allocFlags & eAlloc_Top);
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
		if (allocFlags & eAlloc_Commit)
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

			// comitt only the uncomitted pages
			uint32 numPagesComitted = 0;
			const uint32 firstPage = PageFromAddress(base);
			const uint32 numPages = PagesFromSize(size);
			for (uint32 i = 0; i < numPages; ++i)
			{
				const uint32 pageIndex = firstPage + i;
				if (!m_virtualMemoryCommitedPages.IsSet(pageIndex))
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

					// mark as comitted
					m_virtualMemoryCommitedPages.Set(pageIndex);
					numPagesComitted += 1;
				}
			}

			// new pages comitted ?
			if (numPagesComitted > 0)
			{
				GLog.Spam("VMEM: Virtual memory comitted (%1.2fKB, %d pages, %d new) at %06Xh",
					size / 1024.0f, numPages, numPagesComitted, (uint32)base);

				m_virtualMemoryPagesComitted += numPagesComitted;
			}
		}

		// set default protection flags
		VirtualProtect(base, size, protectFlags);

		// return base
		return base;
	}

	bool Memory::VirtualFree(void* base, const uint32 initSize, const uint32 allocFlags, uint32& outFreedSize)
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
		if (allocFlags & (eAlloc_Decomit | eAlloc_Release))
		{
			// check the range
			if (!IsVirtualMemory(base, size))
			{
				GLog.Err("VMEM: Trying to decommit memory range %06Xh, size %d which is not valid virtual memory range", (uint32)base, size);
				return false;
			}

			// decomitt only the comitted pages
			uint32 numPagesDecomitted = 0;
			const uint32 firstPage = PageFromAddress(base);
			const uint32 numPages = PagesFromSize(size);
			for (uint32 i = 0; i < numPages; ++i)
			{
				const uint32 pageIndex = firstPage + i;
				if (m_virtualMemoryCommitedPages.IsSet(pageIndex))
				{
					// do the work
					uint8* pageBase = (uint8*)m_virtualMemoryBase + pageIndex * VIRTUAL_PAGE_SIZE;
					if (!::VirtualFree(pageBase, VIRTUAL_PAGE_SIZE, MEM_DECOMMIT))
					{
						GLog.Err("VMEM: OS decommit of page %d failed", pageIndex);
						return NULL;
					}

					// mark as comitted
					m_virtualMemoryCommitedPages.Clear(pageIndex);
					numPagesDecomitted += 1;
				}
			}

			// new pages comitted ?
			if (numPagesDecomitted > 0)
			{
				GLog.Spam("VMEM: Virtual memory decomitted (%1.2fKB, %d pages, %d new) at %06Xh",
					size / 1024.0f, numPages, numPagesDecomitted, (uint32)base);

				m_virtualMemoryPagesComitted -= numPagesDecomitted;
			}

			outFreedSize = size;
		}

		// release
		if (allocFlags & eAlloc_Release)
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
	}

	uint32 Memory::TranslatePhysicalAddress(const uint32 localAddress) const
	{
		const uint32 physicalMemoryAddrMask = 0x1FFFFFFF;
		return (const uint32)m_physicalMemoryBase + (localAddress & physicalMemoryAddrMask);
	}

} // win