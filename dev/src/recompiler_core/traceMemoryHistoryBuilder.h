#pragma once

#include "traceMemoryHistory.h"

namespace image
{
	class Binary;
}

namespace trace
{
	class Data;

	/// build of the memory trace file
	class RECOMPILER_API MemoryHistoryBuilder
	{
	public:
		struct Page
		{
			uint32		m_id;
			uint64		m_address;
			uint32		m_entriesHead[MemoryHistory::ENTRIES_PER_PAGE];
			uint32		m_entriesTail[MemoryHistory::ENTRIES_PER_PAGE];

			Page(const uint64 adddres)
				: m_id(0)
				, m_address(adddres)
			{
				memset(&m_entriesHead[0],0,sizeof(m_entriesHead));
				memset(&m_entriesTail[0],0,sizeof(m_entriesTail));
			}
		};

		struct PageRange
		{
			uint64		m_address;
			Page*		m_pages[MemoryHistory::PAGES_PER_RANGE];

			PageRange(const uint64 address)
				: m_address(address)
			{
				memset(&m_pages[0], 0, sizeof(m_pages));
			}

			~PageRange()
			{
				for (uint32 i=0; i<MemoryHistory::PAGES_PER_RANGE; ++i)
				{
					delete m_pages[i];
				}
			}
		};

		uint32	m_nextEntryId;
		uint64	m_entryDataStart;
		FILE*	m_outputFile;

		PageRange* m_ranges[ MemoryHistory::MAX_RANGES ];

	public:
		MemoryHistoryBuilder();
		~MemoryHistoryBuilder();

		// build a memory trace file from normal data trace
		bool BuildFile( class ILogOutput& log, decoding::Context& decodingContext, const Data& traceData, const wchar_t* outputFile);

	private:
		// add memory operation to range of addresses
		const bool AddMemoryOperation(const uint32 traceEntryIndex, const uint64 memoryAddress, const uint8 type, const uint32 size, const void* data);

		// add memory operation to given address
		const bool AddMemoryOperationAt(const uint32 traceEntryIndex, const uint64 memoryAddress, const uint8 type, const uint8 data);

		// get page range at given address base
		PageRange* GetPageRange(const uint64 address);

		// get page at given address base
		Page* GetPage(const uint64 address);

		// get file position
		const uint64 GetFilePos() const;

		// seek to file position
		void SetFilePos(const uint64 pos);

		// write to file
		void WriteToFile(const void* data, const uint32 size);

		// read from file
		void ReadFromFile(void* data, const uint32 size);
	};

} // trace