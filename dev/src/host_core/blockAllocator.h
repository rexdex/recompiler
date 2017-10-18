#pragma once

#include "bitset.h"

namespace utils
{
	class LAUNCHER_API BlockAllocator
	{	
	public:
		BlockAllocator();

		// initialize memory range
		void Initialize(const uint32 numPages);

		// is the given memory range allocated ?
		const bool IsAllocated(const uint32 firstPage, const uint32 numPages) const;

		// get user flags for a memory page
		const bool SetPageFlags(const uint32 page, const uint32 flags);

		// get user flags for a memory page
		const bool GetPageFlags(const uint32 page, uint32& outFlags) const;

		// get base allocated page
		const bool GetBasePage(const uint32 page, uint32& outBasePage) const;

		// get number of allocated blocks for given page
		const bool GetNumAllocatedPages(const uint32 page, uint32& outNumAllocatedPages) const;

		// get total number of allocated pages
		const uint32 GetTotalAllocatedPages() const;

		// get bitmask of allocatedpages
		void GetAllocationBitMask(std::vector<bool>& outAllocatedPages) const;

		// allocate memory of given size
		const bool AllocateBlock(const uint32 numPages, const bool top, const uint32 initialFlags, uint32& outAllocatedFirstPage);

		// free pages back to pool, the whole block touching page is freed
		const bool FreeBlock(const uint32 page);

	private:
		void MergeFreeBlocks();

		struct BlockInfo
		{
			uint32 m_firstPage;
			uint32 m_numPages;
			BlockInfo* m_next;

			inline BlockInfo()
				: m_firstPage(0)
				, m_numPages(0)
				, m_next(nullptr)
			{}
		};

		struct PageInfo
		{
			uint32 m_basePage; // first page that is allocated in this block
			uint32 m_numPages; // size of the allocation (number of pages)
			uint32 m_flags; // user defined flags

			inline PageInfo()
				: m_basePage(0)
				, m_numPages(0)
				, m_flags(0)
			{}

			inline const bool IsAllocated() const
			{
				return (m_numPages > 0);
			}
		};

		std::vector<PageInfo> m_pages; // page informations

		BlockInfo* m_freeBlocks; // allocated/free blocks
		uint32 m_numFreePages; // number of free pages

		std::vector<BlockInfo> m_blockTable; // reserved blocks for maximum fragmentation case (allocated-free-allocated-free)
		BlockInfo* m_blockPool; // pool of blocks
	};

} // utils