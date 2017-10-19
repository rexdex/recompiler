#include "build.h"
#include "xenonMemory.h"

namespace xenon
{
	Memory::Memory(native::IMemory& nativeMemory)
		: m_nativeMemory(nativeMemory)
	{}

	//--

	const bool Memory::MemoryClass::Initialize(native::IMemory& memory, const uint32 prefferedBase, const uint32 totalVirtualMemoryAvaiable)
	{
		// allocate the virtual address space range that will be used as virtual memory
		m_base = memory.AllocVirtualMemory(prefferedBase, totalVirtualMemoryAvaiable);
		if (NULL == m_base)
		{
			GLog.Warn("Mem: Failed to initialize memory range at preferred address %06Xh", prefferedBase);

			// try any address
			m_base = memory.AllocVirtualMemory(0, totalVirtualMemoryAvaiable);
			if (NULL == m_base)
			{
				GLog.Err("Mem: Failed to initialize memory of size %d (%1.2f MB)",
					totalVirtualMemoryAvaiable,
					(float)totalVirtualMemoryAvaiable / (1024.0f*1024.0f));
				return false;
			}
		}

		// setup virtual memory data
		m_totalSize = totalVirtualMemoryAvaiable;
		m_totalPages = totalVirtualMemoryAvaiable / PAGE_SIZE;
		m_allocatedPages  = 0;

		// setup block allocator
		m_allocator.Initialize(m_totalPages);

		// stats
		GLog.Log("Mem: Initialized memory range at %06Xh, size %1.2f MB (%d pages)",
			(uint32)m_base,
			(float)totalVirtualMemoryAvaiable / (1024.0f*1024.0f),
			m_totalPages);

		return true;
	}

	void* Memory::MemoryClass::Allocate(const uint32 size, const bool top, const uint32 flags)
	{
		std::lock_guard<std::mutex> lock(m_lock);

		// page count
		uint32 firstPageIndex = 0;
		const uint32 numPages = PagesFromSize(size);
		if (!m_allocator.AllocateBlock(numPages, top, flags, firstPageIndex))
		{
			GLog.Err("VMEM: Memory allocation of %d (%1.2fKB) failed", size, (float)size / 1024.0f);
			return nullptr;
		}

		// setup the base pointer
		m_allocatedPages += numPages;
		auto* base = AddressFromPage(firstPageIndex);

		// stats
		GLog.Spam("VMEM: Memory allocated (%1.2fKB, %d pages) at %06Xh, Total pages: %u", size / 1024.0f, numPages, (uint32)base, m_allocatedPages);

		// return base
		return base;
	}

	void Memory::MemoryClass::Free(void* memory, uint32& outNumPages)
	{
		std::lock_guard<std::mutex> lock(m_lock);

		const uint32 page = PageFromAddress(memory);

		// get size of freed memory
		uint32 numAllocatedPages = 0;
		if (!m_allocator.GetNumAllocatedPages(page, numAllocatedPages))
			return;

		// free block
		m_allocator.FreeBlock(page);
		m_allocatedPages -= numAllocatedPages;
		outNumPages = numAllocatedPages;

		// stats
		GLog.Spam("VMEM: Memory free (%d pages) at %06Xh, Total pages: %u", numAllocatedPages, (uint32)memory, m_allocatedPages);
	}

	//--

	const bool Memory::InitializeVirtualMemory(const uint32 prefferedBase, const uint32 totalPhysicalMemoryAvaiable)
	{
		return m_virtual.Initialize(m_nativeMemory, prefferedBase, totalPhysicalMemoryAvaiable);
	}

	const bool Memory::InitializePhysicalMemory(const uint32 prefferedBase, const uint32 totalPhysicalMemoryAvaiable)
	{
		return m_physical.Initialize(m_nativeMemory, prefferedBase, totalPhysicalMemoryAvaiable);
	}

	const bool Memory::IsVirtualMemory(void* base, const uint32 size) const
	{
		return m_virtual.Contains(base, size);
	}

	const bool Memory::IsVirtualMemoryReserved(void* base, const uint32 size) const
	{
		if (!IsVirtualMemory(base, size))
			return false;

		const uint32 firstPage = m_virtual.PageFromAddress(base);
		const uint32 numPages = m_virtual.PagesFromSize(size);
		return m_virtual.m_allocator.IsAllocated(firstPage, numPages);
	}

	const uint64 Memory::GetVirtualMemoryFlags(void* base, const uint32 size) const
	{
		if (!IsVirtualMemory(base, size))
			return false;

		const uint32 firstPage = m_virtual.PageFromAddress(base);

		uint32 flags = 0;
		m_virtual.m_allocator.GetPageFlags(firstPage, flags);
		return flags;
	}

	const void Memory::SetVirtualMemoryFlags(void* base, const uint32 size, const uint64 flags)
	{
		DEBUG_CHECK(flags <= 0xFFFFFFFF);

		if (!IsVirtualMemory(base, size))
			return;

		const uint32 firstPage = m_virtual.PageFromAddress(base);
		m_virtual.m_allocator.SetPageFlags(firstPage, (uint16)flags);
	}

	const uint32 Memory::GetVirtualMemoryAllocationSize(void* base) const
	{
		if (!IsVirtualMemory(base, 1))
			return 0;

		const uint32 firstPage = m_virtual.PageFromAddress(base);
		if (!m_virtual.m_allocator.IsAllocated(firstPage, 1))
			return 0;

		uint32 numPages = 0;
		m_virtual.m_allocator.GetNumAllocatedPages(firstPage, numPages);
		return PAGE_SIZE * numPages;
	}

	void* Memory::VirtualAlloc(void* base, const uint32 size, const uint64 allocFlags, const uint64 protectFlags)
	{
		// allocation
		if ((allocFlags & xnative::XMEM_RESERVE) || (base == nullptr))
		{
			DEBUG_CHECK(base == nullptr);
			DEBUG_CHECK(protectFlags <= 0xFFFF);

			const bool topDown = (allocFlags & xnative::XMEM_TOP_DOWN) != 0;
			base = m_virtual.Allocate(size, topDown, (uint16)protectFlags);
		}

		// return base
		return base;
	}

	const bool Memory::VirtualFree(void* base, const uint32 initSize, const uint64 allocFlags, uint32& outFreedSize)
	{
		if (allocFlags & xnative::XMEM_RELEASE)
		{
			uint32 numAllocatedPages = 0;

			m_virtual.Free(base, numAllocatedPages);
			outFreedSize = numAllocatedPages * PAGE_SIZE;
		}

		// done
		return true;
	}

	void* Memory::PhysicalAlloc(const uint32 size, const uint32 alignment, const uint32 protectFlags)
	{
		//DEBUG_CHECK(alignment <= 4096);
		auto* base = m_physical.Allocate(size, false, (uint16)protectFlags);

		return base;
	}

	void Memory::PhysicalFree(void* base, const uint32 size)
	{
		uint32 numFreedPages = 0;
		m_physical.Free(base, numFreedPages);
	}

	const uint32 Memory::TranslatePhysicalAddress(const uint32 localAddress) const
	{
		const uint32 physicalMemoryAddrMask = 0x1FFFFFFF;
		return (const uint32)m_physical.m_base + (localAddress & physicalMemoryAddrMask);
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