#include "build.h"

#include "decodingContext.h"
#include "decodingInstruction.h"
#include "decodingInstructionInfo.h"

#include "tracePagedFile.h"
#include "traceData.h"
#include "traceMemoryHistory.h"
#include "traceMemoryHistoryBuilder.h"
#include "platformCPU.h"
#include "traceUtils.h"

namespace trace
{

	MemoryHistoryBuilder::MemoryHistoryBuilder()
		: m_nextEntryId(1)
		, m_outputFile(NULL)
	{
		memset( &m_ranges[0], 0, sizeof(m_ranges));
	}

	MemoryHistoryBuilder::~MemoryHistoryBuilder()
	{
		// close file
		if ( m_outputFile )
		{
			fclose(m_outputFile);
			m_outputFile = NULL;
		}

		// cleanup
		for (uint32 i=0; i<MemoryHistory::MAX_RANGES; ++i)
		{
			delete m_ranges[i];
		}
	}

	bool MemoryHistoryBuilder::BuildFile(class ILogOutput& log, decoding::Context& decodingContext, const Data& data, const wchar_t* outputFile)
	{
		// get the target CPU
		const platform::CPU* dataCpu = data.GetCPU();
		if (!dataCpu)
			return false;

		// open output file
		_wfopen_s(&m_outputFile, outputFile, L"wb+");
		if (!m_outputFile)
			return false;

		// prepare header
		MemoryHistory::Header header;
		memset(&header, 0, sizeof(header));
		header.m_magic = MemoryHistory::MAGIC;

		// write header
		SetFilePos(0);
		WriteToFile(&header, sizeof(header));

		// entry data start
		header.m_entriesOffset = GetFilePos();
		m_entryDataStart = GetFilePos();

		// reset counters
		m_nextEntryId = 1;

		// create trace data reader
		std::auto_ptr<DataReader> reader(data.CreateReader());

		// stats
		log.SetTaskName("Building memory trace...");

		// process the entries
		uint32 numWriteInstructions = 0;
		uint32 numWriteBytes = 0;
		uint32 numReadInstructions = 0;
		uint32 numReadBytes = 0;
		const uint32 numFrames = data.GetNumDataFrames();
		for (uint32 i=0; i<numFrames; ++i)
		{
			// stats
			log.SetTaskProgress(i,numFrames);

			// read trace data
			const DataFrame& frame = reader->GetFrame(i);
			const DataFrame& nextFrame = reader->GetFrame(i+1); // so we know what has been read
			const uint32 codeAddress = frame.GetAddress();
			if ( !codeAddress )
				continue;

			// decode instruction
			decoding::Instruction op;
			if ( !decodingContext.DecodeInstruction( log, codeAddress, op ) )
			{
				log.Error( "MemTrace: Unknown instruction used at address %08Xh, the trace may be incomplete", codeAddress );
				continue;
			}

			// get extra instruction info
			decoding::InstructionExtendedInfo info;
			if (!op.GetExtendedInfo(codeAddress, decodingContext, info))
				continue;

			// not a memory access instruction ?
			if (0 == (info.m_memoryFlags & (decoding::InstructionExtendedInfo::eMemoryFlags_Write | decoding::InstructionExtendedInfo::eMemoryFlags_Read)))
				continue;

			// calculate target address (using trace data)
			uint64 memoryAdddress;
			if (!info.ComputeMemoryAddress(frame, memoryAdddress))
				continue;

			// get the register
			const platform::CPURegister* reg0 = op.GetArg0().m_reg;

			// write memory operation
			uint8 opType = 1; // write
			uint64 value = 0;
			if (info.m_memoryFlags & decoding::InstructionExtendedInfo::eMemoryFlags_Write)
			{
				numWriteBytes += info.m_memorySize;
				numWriteInstructions += 1;
				value = trace::GetRegisterValueInteger(reg0, frame, false);
				opType = 1;
			}
			else if (info.m_memoryFlags & decoding::InstructionExtendedInfo::eMemoryFlags_Read)
			{
				numReadBytes += info.m_memorySize;
				numReadInstructions += 1;

				value = trace::GetRegisterValueInteger(reg0, nextFrame, false); // what was the value that was read ?
				opType = 0;
			}

			// add op
			if (info.m_memorySize == 1)
			{
				const uint8 valueToSave = (uint8)value;
				AddMemoryOperation(i, memoryAdddress, opType, info.m_memorySize, &valueToSave);
			}
			else if (info.m_memorySize == 2)
			{
				const uint16 valueToSave = _byteswap_ushort( (uint16)value);
				AddMemoryOperation(i, memoryAdddress, opType, info.m_memorySize, &valueToSave);
			}
			else if (info.m_memorySize == 4)
			{
				const uint32 valueToSave = _byteswap_ulong( (uint32)value );
				AddMemoryOperation(i, memoryAdddress, opType, info.m_memorySize, &valueToSave);
			}
			else if (info.m_memorySize == 8)
			{
				const uint64 valueToSave = _byteswap_uint64(value);
				AddMemoryOperation(i, memoryAdddress, opType, info.m_memorySize, &valueToSave);
			}
			else
			{
				log.Error( "MemTrace: Unsupported memory access of size %u used for accessing the memory at %06Xh at entry %u, IP=%06Xh. Entry will not be saved!", 
					info.m_memorySize, memoryAdddress, i, codeAddress);
				continue;
			}
		}

		// final stats
		log.Log( "MemTrace: Read %u bytes in %u instructions", numReadBytes, numReadInstructions );
		log.Log( "MemTrace: Written %u bytes in %u instructions", numWriteBytes, numWriteInstructions );

		// update header
		header.m_numEntries = m_nextEntryId-1;

		// store the entries IDs from the valid pages, enumerate pages
		{
			header.m_pagesOffset = GetFilePos();
			header.m_numPages = 0;

			for (uint32 i=0; i<MemoryHistory::MAX_RANGES; ++i)
			{
				const PageRange* pr = m_ranges[i];
				if (!pr)
					continue;

				for (uint32 j=0; j<MemoryHistory::PAGES_PER_RANGE; ++j)
				{
					Page* page = pr->m_pages[j];
					if (!page)
						continue;

					// set page id
					page->m_id = header.m_numPages+1;
					header.m_numPages += 1;

					// write the top list IDs to the file
					WriteToFile(&page->m_address, sizeof(page->m_address));
					WriteToFile(&page->m_entriesHead,sizeof(page->m_entriesHead));
				}
			}

			log.Log( "MemTrace: %u pages written", header.m_numPages );
		}

		// store the page ranges
		{
			header.m_pageRangesOffset = GetFilePos();
			header.m_numPageRanges = 0;

			for (uint32 i=0; i<MemoryHistory::MAX_RANGES; ++i)
			{
				const PageRange* pr = m_ranges[i];
				if (!pr)
					continue;

				// count non zero pages
				uint16 numNonZeroPages = 0;
				for (uint32 j=0; j<MemoryHistory::PAGES_PER_RANGE; ++j)
				{
					Page* page = pr->m_pages[j];
					if (page)
						numNonZeroPages += 1;
				}

				// save the page rande address
				if (!numNonZeroPages) 
					continue;

				// store the page range data
				WriteToFile(&pr->m_address, sizeof(pr->m_address));
				for (uint32 j=0; j<MemoryHistory::PAGES_PER_RANGE; ++j)
				{
					Page* page = pr->m_pages[j];
					uint32 pageId = page ? page->m_id : 0;
					WriteToFile(&pageId,sizeof(page->m_id));
				}

				// count the saved page ranges
				header.m_numPageRanges += 1;
			}

			log.Log( "MemTrace: %u page ranges", header.m_numPageRanges );
		}

		// update the file header
		SetFilePos(0);
		WriteToFile(&header, sizeof(header));

		// saved
		fclose(m_outputFile);
		return true;
	}

	MemoryHistoryBuilder::PageRange* MemoryHistoryBuilder::GetPageRange(const uint64 address)
	{
		// get page index
		const uint64 rangeIndex = (address >> MemoryHistory::RANGE_SHIFT);
		if (!m_ranges[rangeIndex])
		{
			const uint64 baseAddress = rangeIndex << MemoryHistory::RANGE_SHIFT;
			m_ranges[rangeIndex] = new PageRange(baseAddress);
		}

		// get the page
		return m_ranges[rangeIndex];
	}

	MemoryHistoryBuilder::Page* MemoryHistoryBuilder::GetPage(const uint64 address)
	{
		// get matching page range
		PageRange* range = GetPageRange(address);
		if (!range)
			return NULL;

		// no page entry, create it
		const uint64 pageIndex = (address >> MemoryHistory::PAGE_SHIFT) & MemoryHistory::PAGE_MASK;
		if (!range->m_pages[pageIndex])
		{
			const uint64 baseAddress = (address >> MemoryHistory::PAGE_SHIFT) << MemoryHistory::PAGE_SHIFT;
			range->m_pages[pageIndex] = new Page(baseAddress);
		}

		// get the page
		return range->m_pages[pageIndex];
	}

	const bool MemoryHistoryBuilder::AddMemoryOperation(const uint32 traceEntryIndex, const uint64 memoryAddress, const uint8 type, const uint32 size, const void* data)
	{
		// add memory operations to the whole range
		const uint8* dataPtr = (const uint8*)data;
		for (uint32 i=0; i<size; ++i)
		{
			if (!AddMemoryOperationAt(traceEntryIndex, memoryAddress+i, type, dataPtr[i]))
				return false;
		}

		return true;
	}

	const bool MemoryHistoryBuilder::AddMemoryOperationAt(const uint32 traceEntryIndex, const uint64 memoryAddress, const uint8 type, const uint8 data)
	{
		// get target page
		MemoryHistoryBuilder::Page* page = GetPage(memoryAddress);
		if (!page)
			return false;

		// allocate new entry id
		const uint32 writtenEntryId = m_nextEntryId++;

		// get entry linked list for this address
		uint32& entryListHead = page->m_entriesHead[memoryAddress & MemoryHistory::PAGE_MASK];
		uint32& entryListTail = page->m_entriesTail[memoryAddress & MemoryHistory::PAGE_MASK];
		if (!entryListHead)
		{
			entryListHead = writtenEntryId;
			entryListTail = writtenEntryId;
		}
		else
		{
			const uint64 currentEndPos = GetFilePos();

			// update tail entry
			const uint64 tailEntryPos = m_entryDataStart + (entryListTail-1)*sizeof(MemoryHistory::Entry);
			SetFilePos(tailEntryPos);

			// read the entry data
			MemoryHistory::Entry entryInfo;
			ReadFromFile( &entryInfo, sizeof(entryInfo));
			entryInfo.m_next = writtenEntryId;
			WriteToFile( &entryInfo, sizeof(entryInfo));

			// restore file pos
			SetFilePos(currentEndPos);

			// link
			entryListTail = writtenEntryId;
		}

		// write entry on disk
		MemoryHistory::Entry entryInfo;
		entryInfo.m_data = data;
		entryInfo.m_entry = traceEntryIndex;
		entryInfo.m_next = 0; // nothing yet
		entryInfo.m_op = type;

		// save the entry
		WriteToFile( &entryInfo, sizeof(entryInfo) );
		return true;
	}

	const uint64 MemoryHistoryBuilder::GetFilePos() const
	{
		fpos_t pos = 0;
		fgetpos(m_outputFile, &pos);
		return pos;
	}

	void MemoryHistoryBuilder::SetFilePos(const uint64 pos_)
	{
		fpos_t pos = (fpos_t)pos_;
		fsetpos(m_outputFile, &pos);
	}

	void MemoryHistoryBuilder::WriteToFile(const void* data, const uint32 size)
	{
		fwrite(data,1,size,m_outputFile);
	}

	void MemoryHistoryBuilder::ReadFromFile(void* data, const uint32 size)
	{
		fread(data,1,size,m_outputFile);
	}

	//---------------------------------------------------------------------------

} // trace