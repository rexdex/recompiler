#include "build.h"
#include "blockAllocator.h"

//#pragma optimize("",off)

namespace utils
{

	BlockAllocator::BlockAllocator()
		: m_blockPool(NULL)
		, m_numFreePages(0)
		, m_freeBlocks(NULL)
	{}

	void BlockAllocator::Initialize(const uint32 numPages)
	{
		// create page tables
		m_blockTable.resize(numPages*2 - 1);
		m_pages.resize(numPages);

		// create block pool
		for (size_t i = 0; i<m_blockTable.size(); ++i)
		{
			BlockInfo* block = &m_blockTable[i];
			block->m_firstPage = 0;
			block->m_numPages = 0;
			block->m_next = m_blockPool;
			m_blockPool = block;
		}

		// reset
		m_numFreePages = numPages;

		// create first free page
		m_freeBlocks = m_blockPool;
		m_freeBlocks->m_firstPage = 0;
		m_freeBlocks->m_numPages = numPages;
		m_blockPool = m_blockPool->m_next;
		m_freeBlocks->m_next = nullptr;
	}

	const bool BlockAllocator::IsAllocated(const uint32 firstPage, const uint32 numPages) const
	{
		const auto maxPage = m_pages.size();

		for (uint32 i = 0; i < numPages; ++i)
		{
			auto pageIndex = firstPage + i;
			if (pageIndex >= maxPage)
				break;

			const auto& info = m_pages[pageIndex];
			if (info.IsAllocated())
				return true;
		}

		return false;
	}

	const bool BlockAllocator::SetPageFlags(const uint32 page, const uint32 flags)
	{
		const auto maxPage = m_pages.size();
		if (page >= maxPage)
			return false;

		auto& info = m_pages[page];
		if (!info.IsAllocated())
			return false;

		info.m_flags = flags;
		return true;
	}

	const bool BlockAllocator::GetPageFlags(const uint32 page, uint32& outFlags) const
	{
		const auto maxPage = m_pages.size();
		if (page >= maxPage)
			return false;

		auto& info = m_pages[page];
		if (!info.IsAllocated())
			return false;

		outFlags = info.m_flags;
		return true;
	}

	const bool BlockAllocator::GetBasePage(const uint32 page, uint32& outBasePage) const
	{
		const auto maxPage = m_pages.size();
		if (page >= maxPage)
			return false;

		auto& info = m_pages[page];
		if (!info.IsAllocated())
			return false;

		outBasePage = info.m_basePage;
		DEBUG_CHECK(m_pages[outBasePage].IsAllocated());
		return true;
	}

	const bool BlockAllocator::GetNumAllocatedPages(const uint32 page, uint32& outNumAllocatedPages) const
	{
		const auto maxPage = m_pages.size();
		if (page >= maxPage)
			return false;

		auto& info = m_pages[page];
		if (!info.IsAllocated())
			return false;

		outNumAllocatedPages = info.m_numPages;
		return true;
	}

	const uint32 BlockAllocator::GetTotalAllocatedPages() const
	{
		return (uint32)(m_pages.size() - m_numFreePages);
	}

	void BlockAllocator::GetAllocationBitMask(std::vector<bool>& outAllocatedPages) const
	{
		outAllocatedPages.resize(m_pages.size());
		for (uint32 i = 0; i < m_pages.size(); ++i)
			outAllocatedPages[i] = m_pages[i].IsAllocated();
	}

	const bool BlockAllocator::AllocateBlock(const uint32 numPages, const bool top, const uint32 initialFlags, uint32& outAllocatedFirstPage)
	{
		// to small to ever fit
		if (numPages > m_numFreePages)
			return false;

		// find first fitting block
		BlockInfo** prevFreeBlock = NULL;
		BlockInfo* freeBlock = NULL;
		{
			BlockInfo** prevBlock = &m_freeBlocks;
			BlockInfo* curBlock = m_freeBlocks;
			while (curBlock != NULL)
			{
				if (curBlock->m_numPages >= numPages)
				{
					freeBlock = curBlock;
					prevFreeBlock = prevBlock;
					if (!top) break;
				}

				prevBlock = &curBlock->m_next;
				curBlock = curBlock->m_next;
			}
		}

		// nothing
		if (!freeBlock)
			return false;

		// get the first allocated page
		if (top)
			outAllocatedFirstPage = freeBlock->m_firstPage + freeBlock->m_numPages - numPages;
		else
			outAllocatedFirstPage = freeBlock->m_firstPage;

		// split block
		const uint32 pagesLeft = freeBlock->m_numPages - numPages;
		if (pagesLeft)
		{
			// adjust free block
			if (top)
			{
				// free space is left at the beginning of the block
				freeBlock->m_numPages -= numPages;
			}
			else
			{
				// free space is left at the end of the block
				freeBlock->m_firstPage += numPages;
				freeBlock->m_numPages -= numPages;
			}
		}
		else
		{
			// unlink
			DEBUG_CHECK(*prevFreeBlock == freeBlock);
			*prevFreeBlock = freeBlock->m_next;

			// add block to free pool
			freeBlock->m_numPages = 0;
			freeBlock->m_firstPage = 0;
			freeBlock->m_next = m_blockPool;
			m_blockPool = freeBlock;
		}

		// mark memory as allocated
		for (uint32 i = 0; i < numPages; ++i)
		{
			const uint32 pageIndex = outAllocatedFirstPage + i;
			auto& pageInfo = m_pages[pageIndex];
			pageInfo.m_numPages = numPages;
			pageInfo.m_basePage = outAllocatedFirstPage;
			pageInfo.m_flags = initialFlags;
		}

		// allocated
		m_numFreePages -= numPages;
		return true;
	}

	const bool BlockAllocator::FreeBlock(const uint32 page)
	{
		const auto maxPage = m_pages.size();
		if (page >= maxPage)
			return false;
		
		// check that the memory is allocated
		const auto& pageInfo = m_pages[page];
		if (!pageInfo.IsAllocated())
			return false;

		// get the range of pages to free
		const auto firstPage = pageInfo.m_basePage;
		const auto numPages = pageInfo.m_numPages;

		// mark all pages as free
		for (uint32 i = 0; i < numPages; ++i)
		{
			auto& pageInfo = m_pages[i];
			pageInfo.m_flags = 0;
			pageInfo.m_basePage = 0;
			pageInfo.m_numPages = 0;
		}

		// allocate block
		BlockInfo* freeBlock = m_blockPool;
		DEBUG_CHECK(freeBlock != nullptr);
		m_blockPool = freeBlock->m_next;
		freeBlock->m_next = nullptr;
		freeBlock->m_numPages = numPages;
		freeBlock->m_firstPage = firstPage;

		// page indices
		const uint32 lastPageUsed = firstPage + numPages - 1;
		const uint32 endPage = firstPage + numPages;

		uint32 lastPageOfPrevBlock = 0;

		// add into the free list
		bool added = false;
		BlockInfo** prevBlock = &m_freeBlocks;
		BlockInfo* lastBlock = m_freeBlocks;
		while (*prevBlock)
		{
			if (firstPage >= lastPageOfPrevBlock && lastPageUsed < (*prevBlock)->m_firstPage)
			{
				// link in list
				freeBlock->m_next = *prevBlock;
				*prevBlock = freeBlock;

				added = true;
				break;
			}

			lastPageOfPrevBlock = (*prevBlock)->m_firstPage + (*prevBlock)->m_numPages;
			lastBlock = *prevBlock;
			prevBlock = &(*prevBlock)->m_next;
		}

		// not added anywhere, add at the end
		if (!added)
		{
			DEBUG_CHECK(lastBlock != nullptr);
			DEBUG_CHECK(*prevBlock == nullptr);
			DEBUG_CHECK(lastBlock != freeBlock);
			DEBUG_CHECK(firstPage >= lastBlock->m_firstPage);
			DEBUG_CHECK(lastBlock->m_next == nullptr);

			lastBlock->m_next = freeBlock;
			freeBlock->m_next = nullptr;
		}

		// update stats
		m_numFreePages += numPages;

		// merge free blocks that are close to each other
		MergeFreeBlocks();
		return true;
	}

	void BlockAllocator::MergeFreeBlocks()
	{
		BlockInfo* cur = m_freeBlocks;
		while (cur)
		{
			BlockInfo* next = cur->m_next;

			// try merge
			if (next && (next->m_firstPage == (cur->m_firstPage + cur->m_numPages)))
			{
				// merge memory range
				cur->m_numPages += next->m_numPages;
				cur->m_next = next->m_next;

				// return free block structure to pool
				next->m_next = m_blockPool;
				next->m_firstPage = 0;
				next->m_numPages = 0;
				m_blockPool = next;

				continue;
			}

			// continue
			cur = next;
		}
	}

} // utils