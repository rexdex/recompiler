#pragma once

namespace trace
{

	//---------------------------------------------------------------------------

	class PagedFile;
	class PagedFileReader;
	class MemoryHistoryReader;
	class MemoryHistoryBuilder;

	//---------------------------------------------------------------------------

	/// Trace memory history
	class RECOMPILER_API MemoryHistory
	{
	public:
		MemoryHistory();
		~MemoryHistory();

		// create file reader
		class MemoryHistoryReader* CreateReader() const;

		// load trace memory file
		static MemoryHistory* OpenFile( class ILogOutput& log, const wchar_t* filePath );

	private:
		static const uint32 MAGIC				= 'MEMT';
		static const uint32 ENTRIES_PER_PAGE	= 1024;
		static const uint32 PAGES_PER_RANGE		= 1024;
		static const uint32 MAX_RANGES			= 4096;

		static const uint32 RANGE_SHIFT			= 20; // 12 bits left
		static const uint32 PAGE_SHIFT			= 10; // 22 bits left, select 10
		static const uint32 PAGE_MASK			= 1023; // select 10 bits

		#pragma pack(push)
		#pragma pack(4)
		struct Header
		{
			uint32		m_magic;

			uint32		m_numEntries;
			uint64		m_entriesOffset;

			uint32		m_numPages;
			uint64		m_pagesOffset;

			uint32		m_numPageRanges;
			uint64		m_pageRangesOffset;

		};
		#pragma pack(pop)

		struct Entry
		{
			uint32		m_next:24;		// next entry
			uint32		m_data:8;		// memory data
			uint32		m_entry:31;		// index of entry affecting this location
			uint32		m_op:1;			// read:0 write:1
		};

		// ~1KB access page
		struct Page
		{
			uint32		m_address;	// base address (22 upper bits)
			uint32		m_entries[ENTRIES_PER_PAGE]; // entries heads
		};

		// 1024 pages (1MB of memory - 20 bites)
		struct PageRange
		{
			uint32		m_address;	// base address (12 upper bits)
			uint32		m_pages[PAGES_PER_RANGE]; // IDs
		};

		// address space mapping for higher (32-20 = 12 bits = 4096 entries)
		const PageRange*		m_pageRanges;
		uint32					m_numPageRanges;
		uint32					m_numPageIds;

		// page ranges (valid ones)
		const Page*				m_pages;
		uint32					m_numPages;

		// file with memory access entries
		PagedFile*				m_entryFile;
		uint64					m_entriesOffset;
		uint32					m_numEntries;

		// addres space map
		const PageRange*		m_addressSpaceMap[MAX_RANGES];

		// get the head list entry for given address
		const uint32 GetEntryHeadForAddress(const uint32 memoryAddress) const;

		friend class MemoryHistoryBuilder;
		friend class MemoryHistoryReader;
	};

	//---------------------------------------------------------------------------

	/// memory trace data reader
	class RECOMPILER_API MemoryHistoryReader
	{
	public:	
		struct History
		{
			uint32	m_entry;
			uint8	m_data; 
			uint8	m_op;  //1-write, 0-read
		};

		MemoryHistoryReader( const MemoryHistory* data, PagedFileReader* fileReader);
		~MemoryHistoryReader();

		// get memory history for given byte
		const bool GetHistory( const uint32 address, const uint32 minTraceEntry, const uint32 maxTraceEntry, std::vector<History>& outHistory ) const;

	private:
		// data (address space mapping)
		const MemoryHistory*		m_data;

		// file reader
		PagedFileReader*		m_reader;
	};
	
	//---------------------------------------------------------------------------

} // trace