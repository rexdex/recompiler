#include "build.h"
#include "tracePagedFile.h"
#include "traceData.h"
#include "traceMemoryHistory.h"

namespace trace 
{
	MemoryHistory::MemoryHistory()
		: m_pageRanges(NULL)
		, m_entryFile(NULL)
		, m_numPageRanges(0)
		, m_pages(NULL)
		, m_numPages(0)
	{
	}

	MemoryHistory::~MemoryHistory()
	{
		if (m_pages)
		{
			free((void*)m_pages);
			m_pages = NULL;
		}

		if (m_pageRanges)
		{
			free((void*)m_pageRanges);
			m_pageRanges = NULL;
		}

		if (m_entryFile)
		{
			delete m_entryFile;
			m_entryFile = NULL;
		}
	}

	class MemoryHistoryReader* MemoryHistory::CreateReader() const
	{
		return new MemoryHistoryReader(this, m_entryFile->CreateReader());
	}

	MemoryHistory* MemoryHistory::OpenFile(class ILogOutput& log, const wchar_t* filePath)
	{
		// open file
		std::auto_ptr<PagedFile> file(PagedFile::OpenFile(filePath));
		if (!file.get())
		{
			log.Error( "MemoryTrace: Failed to open file '%ls'", filePath);
			return NULL;
		}

		// create data reader
		std::auto_ptr<PagedFileReader> reader(file->CreateReader());

		// read the header
		Header header;
		memset(&header,0,sizeof(header));
		if (!reader->Read(&header, sizeof(header)))
		{
			log.Error( "MemoryTrace: Error reading from '%ls'", filePath );
			return NULL;
		}

		// valid file ?
		if (header.m_magic != MAGIC)
		{
			log.Error( "MemoryTrace: Invalid format of '%ls' (%08X!=%08X)", filePath, header.m_magic, MAGIC );
			return NULL;
		}

		// create output object
		std::auto_ptr<MemoryHistory> ret(new MemoryHistory());
		ret->m_numPageRanges = header.m_numPageRanges;
		ret->m_numPages = header.m_numPages;
		ret->m_numEntries = header.m_numEntries;

		// Load pages
		ret->m_pages = (const Page*) malloc(header.m_numPages*sizeof(Page));
		reader->Seek( header.m_pagesOffset );
		if (!ret->m_pages || !reader->Read((void*)ret->m_pages, header.m_numPages*sizeof(Page)))
		{
			log.Error( "MemoryTrace: Error loading pages from '%ls'", filePath);
			return NULL;
		}

		// Create page ranges
		ret->m_pageRanges = (const PageRange*) malloc(header.m_numPageRanges*sizeof(PageRange));
		reader->Seek(header.m_pageRangesOffset);
		if (!ret->m_pageRanges || !reader->Read((void*)ret->m_pageRanges, header.m_numPageRanges*sizeof(PageRange)))
		{
			log.Error( "MemoryTrace: Error loading pages from '%ls'", filePath);
			return NULL;
		}

		// Setup page mapping
		memset(&ret->m_addressSpaceMap[0], 0, sizeof(ret->m_addressSpaceMap));
		for (uint32 i=0; i<header.m_numPageRanges; ++i)
		{
			const PageRange* pr = &ret->m_pageRanges[i];
			const uint32 prIndex = pr->m_address >> (RANGE_SHIFT);
			if (ret->m_addressSpaceMap[prIndex])
			{
				log.Log( "MemoryTrace: PageRange collision at entry %u", prIndex);
				return NULL;
			}

			ret->m_addressSpaceMap[prIndex] = pr;
		}

		// stats
		log.Log( "MemoryTrace: Loaded memory trace, %u entries in %u pages, %u page ranges", header.m_numEntries, header.m_numPages, header.m_numPageRanges );

		// set file
		ret->m_entryFile = file.release();
		ret->m_entriesOffset = header.m_entriesOffset;

		// done
		return ret.release();
	}

	const uint32 MemoryHistory::GetEntryHeadForAddress(const uint32 memoryAddress) const
	{
		// get page from range
		const uint32 prIndex = memoryAddress >> RANGE_SHIFT;
		const PageRange* pr = m_addressSpaceMap[ prIndex ];
		if ( !pr )
			return 0;

		// get page in page range
		const uint32 pageNr = (memoryAddress >> PAGE_SHIFT) & PAGE_MASK;
		const uint32 pageIndex = pr->m_pages[ pageNr ];
		if ( !pageIndex )
			return 0;

		// get entry index
		const Page* page = m_pages + (pageIndex-1);
		const uint32 entryList = page->m_entries[ memoryAddress & PAGE_MASK ];
		return entryList;
	}

	//---------------------------------------------------------------------------

	MemoryHistoryReader::MemoryHistoryReader(const MemoryHistory* data, PagedFileReader* fileReader)
		: m_data(data)
		, m_reader(fileReader)
	{
	}

	MemoryHistoryReader::~MemoryHistoryReader()
	{
		if (m_reader)
		{
			delete m_reader;
			m_reader = NULL;
		}
	}

	const bool MemoryHistoryReader::GetHistory(const uint32 address, const uint32 minTraceEntry, const uint32 maxTraceEntry, std::vector<History>& outHistory) const
	{
		// get the list head
		const uint32 entryList = m_data->GetEntryHeadForAddress(address);
		if ( !entryList )
			return false;

		// build the list
		uint32 entryIndex = entryList;
		while (entryIndex)
		{
			// invalid entry ?
			if (entryIndex >= m_data->m_numEntries)
				break;

			// read entry		
			MemoryHistory::Entry entry;
			m_reader->Seek(m_data->m_entriesOffset + (entryIndex-1)*sizeof(entry));
			if (!m_reader->Read(&entry, sizeof(entry)))
				break;

			// out of range
			if (entry.m_entry > maxTraceEntry)
				break;

			// not yet in range
			if (entry.m_entry >= minTraceEntry)
			{
				// create propper history entry
				History info;
				info.m_entry = entry.m_entry;
				info.m_op = entry.m_op;
				info.m_data = entry.m_data;
				outHistory.push_back(info);
			}

			// go to the next entry
			entryIndex = entry.m_next;
		}

		// done
		return true;
	}

} // trace