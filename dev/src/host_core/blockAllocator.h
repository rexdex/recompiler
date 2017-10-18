#pragma once

#include "bitMask.h"

namespace utils
{
	template< uint32 MAX_PAGES >
	class BlockAllocator
	{
	private:
		struct BlockInfo
		{
			uint32			m_firstPage;
			uint32			m_numPages;
			BlockInfo*		m_next;
		};

		BitMask<MAX_PAGES> m_pageBits;
		uint32			   m_allocSize[MAX_PAGES]; // tempshit

		BitMask<MAX_PAGES> m_protectBits;

		BlockInfo*		m_freeBlocks;
		uint32			m_numFreePages;

		BlockInfo		m_blockTable[MAX_PAGES / 2]; // maximum fragmentation
		BlockInfo		*m_blockPool;

	public:
		BlockAllocator()
			: m_blockPool(NULL)
			, m_numFreePages(0)
			, m_freeBlocks(NULL)
		{
			// create block pool
			for (uint32 i = 0; i < (MAX_PAGES / 2); ++i)
			{
				BlockInfo* block = &m_blockTable[i];
				block->m_firstPage = 0;
				block->m_numPages = 0;
				block->m_next = m_blockPool;
				m_blockPool = block;
			}

			// reset page flags
			memset(&m_pageBits, 0, sizeof(m_pageBits));
			memset(&m_allocSize, 0, sizeof(m_allocSize));
			memset(&m_protectBits, 0, sizeof(m_protectBits));
		}

		// initialize memory range
		void Initialize(const uint32 numPages)
		{
			// reset
			m_numFreePages = numPages;

			// create first free page
			m_freeBlocks = m_blockPool;
			m_freeBlocks->m_firstPage = 0;
			m_freeBlocks->m_numPages = numPages;
			m_blockPool = m_blockPool->m_next;
			m_freeBlocks->m_next = nullptr;
		}

		// is the given memory range allocated ?
		bool IsAllocated(const uint32 firstPage, const uint32 numPages) const
		{
			for (uint32 i = 0; i < numPages; ++i)
			{
				if (m_pageBits.IsSet(firstPage + i))
					return true;
			}

			return false;
		}

		// is given page protected?
		bool IsProtected(const uint32 page) const
		{
			return m_pageBits.IsSet(page);
		}

		// get number of allocated blocks for given page
		bool GetNumAllocatedBlocks(const uint32 basePage, uint32& outNumAllocatedBlocks) const
		{
			// not allocated at all
			if (!m_pageBits.IsSet(basePage))
				return false;

			// allocated ?
			if (!m_allocSize[basePage])
				return false;

			outNumAllocatedBlocks = m_allocSize[basePage];
			return true;
		}

		// allocate memory of given size
		bool AllocateBlock(const uint32 numPages, const bool top, uint32& outAllocatedFirstPage)
		{
			// to small
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
					// free space is left at the begining of the block
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
				m_pageBits.Set(pageIndex);
			}

			// remember allocation size
			DEBUG_CHECK(m_allocSize[outAllocatedFirstPage] == 0);
			m_allocSize[outAllocatedFirstPage] = numPages;

			// allocated
			return true;
		}

		// free pages back to pool
		bool FreeBlock(const uint32 firstPage, const uint32 numPages)
		{
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
				added = true;
			}

			DEBUG_CHECK(added);
			MergeFreeBlocks();

			// mark memory as not allocated
			DEBUG_CHECK(m_allocSize[firstPage] == numPages);
			m_allocSize[firstPage] = 0;

			return true;
		}

		void MergeFreeBlocks()
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
	};

} // win