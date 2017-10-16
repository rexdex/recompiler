#include "build.h"
#include "traceData.h"
#include "tracePagedFile.h"
#include "traceCommon.h"
#include "platformCPU.h"
#include "platformDefinition.h"
#include "platformLibrary.h"
#include <algorithm>
#include <assert.h>

#include <Windows.h>

namespace trace
{

	//---------------------------------------------------------------------------

	Registers::Registers()
	{
	}

	void Registers::Build(const std::vector< const platform::CPURegister* >& traceRegisters)
	{
		m_traceRegisters = traceRegisters;

		m_traceRegistersSizes.resize(traceRegisters.size());

		uint32 dataPos = 0;

		for (uint32 i = 0; i < m_traceRegisters.size(); ++i )
		{
			const auto* reg = m_traceRegisters[i];

			const auto bitSize = reg->GetBitSize();
			assert(bitSize >= 8);
			assert((bitSize&7) == 0);

			const auto byteSize = bitSize / 8;
			m_traceRegistersSizes[i] = byteSize;

			reg->BindToTrace(i, dataPos);
			dataPos += byteSize;
		}

		// TODO: reverse mapping ?
	}

	int Registers::FindTraceRegisterIndex(const platform::CPURegister* reg) const
	{
		for (uint32 i=0; i<m_traceRegisters.size(); ++i)
			if (m_traceRegisters[i] == reg)
				return i;

		return -1;
	}

	//---------------------------------------------------------------------------

	DataFrame::DataFrame()
		: m_address(0)
		, m_index(0)
		, m_dataSize(0)
		, m_data(nullptr)
	{
	}

	DataFrame::~DataFrame()
	{
	}

	void DataFrame::BindBuffer(uint8* data, const uint32 dataSize)
	{
		m_data = data;
		m_dataSize = dataSize;
	}

	void DataFrame::CopyValues(const DataFrame& frame)
	{
		memcpy(m_data, frame.m_data, m_dataSize);
	}

	DataFrame& DataFrame::EMPTY()
	{
		static DataFrame theEmptyFrame;
		static uint8 theEmptyFrameData[16 * 512];
		if (theEmptyFrame.m_data == nullptr )
		{ 
			theEmptyFrame.m_dataSize = sizeof(theEmptyFrameData);
			theEmptyFrame.m_data = theEmptyFrameData;
			theEmptyFrame.m_clock = 0;
			theEmptyFrame.m_address = 0x5550DEAD;
			theEmptyFrame.m_index = 0;
		}
		return theEmptyFrame;
	}

	const uint8* DataFrame::GetRegData(const platform::CPURegister* reg) const
	{
		const auto dataOffset = reg ? reg->GetTraceDataOffset() : -1;
		return (dataOffset != -1) ? (m_data + dataOffset) : nullptr;
	}

	//---------------------------------------------------------------------------

	DataFramePage::DataFramePage(Data* owner, const uint32 maxFrames, const uint32 dataSizePerFrame)
		: m_firstFrame( 0 )
		, m_refCount(1)
		, m_owner(owner)
		, m_lruCacheToken(0)
	{
		m_maxFrames = maxFrames;
		m_frames = (DataFrame*)malloc(sizeof(DataFrame) * maxFrames);
		memset(m_frames, 0, sizeof(DataFrame) * maxFrames);

		m_maxData = maxFrames * dataSizePerFrame;
		m_data = (uint8*)malloc(m_maxData);
		memset(m_data, 0, m_maxData);

		auto* curValue = m_data;
		for (uint32 i = 0; i < maxFrames; ++i )
		{
			m_frames[i].BindBuffer(curValue, dataSizePerFrame);
			curValue += dataSizePerFrame;
		}
	}

	DataFramePage::~DataFramePage()
	{
		memset(m_frames, 0xDD, m_maxFrames * sizeof(DataFrame));
		free(m_frames);
		m_frames = nullptr;

		memset(m_data, 0xDE, m_maxData);
		free(m_data);
		m_data = nullptr;
	}

	const DataFrame& DataFramePage::GetFrame(const uint32 frameIndex) const
	{
		assert(frameIndex >= m_firstFrame);
		assert(frameIndex < (m_firstFrame + m_numFrames));
		return m_frames[frameIndex - m_firstFrame];
	}

	bool DataFramePage::ContainsFrame(const uint32 frameIndex) const
	{
		if (frameIndex >= m_firstFrame && frameIndex < (m_firstFrame + m_numFrames))
			return true;

		return false;
	}

	void DataFramePage::Release()
	{
		assert(m_refCount >= 0);
		m_owner->ReleasePageRef(this);
	}

	void DataFramePage::AddRef()
	{
		m_refCount += 1;
	}

	//---------------------------------------------------------------------------

	Data::Data()
		: m_cpu(nullptr)
		, m_file(nullptr)
		, m_fileReader(nullptr)
		, m_numFramesPerPage(0)
		, m_freePagesCacheIndex(1)
		, m_numFrames(0)
	{
	}

	Data::~Data()
	{
		delete m_fileReader;
		m_fileReader = nullptr;

		delete m_file;
		m_file = nullptr;
	}	

	static void ScanTraceForFullFrames(ILogOutput& log, PagedFileReader& reader, std::vector<uint64>& outFullFrameOffsets, uint32& outNumExtraFrames)
	{
		// skim through all data
		// file is generated in append mode only so there's no big table with everything
		// so we basically need to rebuild the data from the file
		for (;;)
		{
			// add to map
			const uint64 currentFileOffset = reader.GetOffset();
			outFullFrameOffsets.push_back(currentFileOffset);
			log.SetTaskProgress((int)currentFileOffset, (int)reader.GetSize());

			// read header
			common::TraceFullFrame info;
			memset(&info, 0, sizeof(info));
			reader.Read(&info, sizeof(info));

			// not a full frame trace info
			if (info.m_magic != common::TraceFullFrame::MAGIC)
			{
				const uint64 offset = reader.GetOffset() - 4;
				outFullFrameOffsets.pop_back();
				reader.Seek(currentFileOffset);
				break;
			}

			// last frame
			if (info.m_nextOffset == 0)
			{
				// skip of the full data
				common::TraceDataHeader dataInfo;
				memset(&dataInfo, 0, sizeof(dataInfo));
				reader.Read(&dataInfo, sizeof(dataInfo));
				reader.Skip(dataInfo.m_dataSize);
				break;
			}

			// move to next full frame
			const uint64 nextFrameOffset = currentFileOffset + info.m_nextOffset;
			reader.Seek(nextFrameOffset);
		}

		// parse the rest of frames - they should be delta frames only
		for (;;)
		{
			// read header
			common::TraceDeltaFrame info;
			memset(&info, 0, sizeof(info));
			reader.Read(&info, sizeof(info));

			// not a delta frame trace info
			// incomplete data
			if (info.m_magic != common::TraceDeltaFrame::MAGIC)
				break;

			// read data block
			common::TraceDataHeader dataInfo;
			memset(&dataInfo, 0, sizeof(dataInfo));
			reader.Read(&dataInfo, sizeof(dataInfo));

			// skip the data
			const auto localSkipOffset = reader.GetOffset() + dataInfo.m_dataSize;
			reader.Seek(localSkipOffset);

			// count the outstanding frames
			outNumExtraFrames += 1;
		}
	}

	Data* Data::OpenTraceFile( class ILogOutput& log, const std::wstring& filePath )
	{
		// stats
		log.SetTaskName("Scanning trace file...");

		// open the page file
		std::auto_ptr<PagedFile> file(PagedFile::OpenFile(filePath));
		if (!file.get())
		{
			log.Error("Trace: Failed to open file '%ls'", filePath.c_str());
			return nullptr;
		}

		// create local reader
		std::auto_ptr<PagedFileReader> reader(file->CreateReader());
		if (!reader.get())
			return nullptr;

		// read stream header
		common::TraceFileHeader header;
		memset(&header, 0, sizeof(header));
		if (!reader->Read(&header, sizeof(header)))
		{
			log.Error("Trace: Error reading file '%ls'", filePath.c_str());
			return nullptr;
		}

		// check header
		if (header.m_magic != common::TraceFileHeader::MAGIC)
		{
			log.Error("Trace: File '%ls' is not a valid trace file for this application", filePath.c_str());
			return nullptr;
		}

		// find the platform used for tracing
		const platform::Definition* platform = platform::Library::GetInstance().FindPlatform(header.m_platformName);
		if ( !platform )
		{
			log.Error("Trace: Trace file '%ls' uses unknown platform '%s", filePath.c_str(), header.m_platformName);
			return nullptr;
		}

		// find the CPU used for tracing
		const platform::CPU* cpu = platform->FindCPU(header.m_cpuName);
		if (!platform)
		{
			log.Error("Trace: Trace file '%ls' uses unknown cpu '%s' (platform '%hs')", filePath.c_str(), header.m_cpuName, header.m_platformName);
			return nullptr;
		}

		// unbind all trace data from CPU registers
		for (uint32 i = 0; i <cpu->GetNumRegisters(); ++i)
		{ 
			const auto* cpuReg = cpu->GetRegister(i);
			cpuReg->BindToTrace(-1, -1);
		}

		// load register table
		std::vector<common::TraceRegiserInfo> regInfos;
		regInfos.resize(header.m_numRegisers);
		reader->Read(regInfos.data(), sizeof(common::TraceRegiserInfo) * header.m_numRegisers);

		// build registry mapping
		uint32 maxDataPerFrame = 0;
		std::vector< const platform::CPURegister* > traceRegisters;
		for (const auto& regInfo : regInfos)
		{
			// validate that register exists
			const auto* platformReg = cpu->FindRegister(regInfo.m_name);
			if (!platformReg)
			{
				log.Error("Trace: Register '%hs' used in trace not found in cpu '%hs'", regInfo.m_name, header.m_cpuName);
				continue;
			}

			// cannot use non-root registers in the trace
			if (nullptr != platformReg->GetParent())
			{
				log.Error("Trace: Register '%hs' used in trace is not a root regsiter of cpu '%hs' (parent: '%hs')", regInfo.m_name, header.m_cpuName, platformReg->GetParent()->GetName());
				continue;
			}

			// validate that the data size will match
			if (platformReg->GetBitSize() != regInfo.m_bitSize)
			{
				log.Error("Trace: Register '%hs' used in trace has different size (%d) than in the cpu '%hs' (%d)", regInfo.m_name, regInfo.m_bitSize, header.m_cpuName, platformReg->GetBitSize());
				continue;
			}

			// save in mapping
			traceRegisters.push_back(platformReg);
			maxDataPerFrame += (platformReg->GetBitSize() / 8);
		}

		// we need valid mapping to continue (all registers restored)
		if (traceRegisters.size() != regInfos.size())
		{
			log.Error("Trace: Could not restore register mapping");
			return nullptr;
		}

		// load the offset table for the full frames
		std::vector< uint64 > fullFrameOffsets;
		uint32 numFrames = 0;
		ScanTraceForFullFrames(log, *reader, fullFrameOffsets, numFrames);

		// count frames accessible via the full frames, note: the last block is "loose frames" and should not be counted twice
		if (fullFrameOffsets.size() > 1)
			numFrames += (uint32)((fullFrameOffsets.size()-1) * header.m_numFramesPerPage);

		// stats
		log.Log("Trace: Found %u frames in trace file (%u full frames)", numFrames, (uint32)fullFrameOffsets.size());
		log.Log("Trace: Max data per frame: %d bytes (%1.2f KB in page)", maxDataPerFrame, (float)(maxDataPerFrame * header.m_numFramesPerPage) / 1024.0f);		

		// return data
		std::auto_ptr<Data> ret( new Data() );
		ret->m_platform = platform;
		ret->m_cpu = cpu;
		ret->m_file = file.release();
		ret->m_fileReader = reader.release();
		ret->m_registers.Build(traceRegisters);
		ret->m_numFrames = numFrames;
		ret->m_numFramesPerPage = header.m_numFramesPerPage;
		ret->m_pageOffsets = std::move(fullFrameOffsets);
		ret->m_maxDataPerFrame = maxDataPerFrame;

		// finalize
		return ret.release();
	}

	DataFramePage* Data::GetPage(const uint32 pageIndex)
	{
		std::lock_guard< std::mutex > lock(m_lock);

		// already created and in use ? just add another reference
		{
			auto it = m_activePages.find(pageIndex);
			if (it != m_activePages.end())
			{
				auto* ret = it->second;
				assert(ret->GetPageIndex() == pageIndex);

				ret->m_lruCacheToken = m_freePagesCacheIndex;
				m_freePagesCacheIndex += 1;

				ret->AddRef();
				return ret;
			}
		}

		// try to resurrect from the dead (free list)
		{
			auto it = m_freePages.find(pageIndex);
			if (it != m_freePages.end())
			{
				auto* ret = it->second;
				assert(ret->GetPageIndex() == pageIndex);

				ret->m_lruCacheToken = m_freePagesCacheIndex;
				m_freePagesCacheIndex += 1;

				m_freePages.erase(it);

				m_activePages[pageIndex] = ret;
				assert(m_activePages[pageIndex] == ret);

 				ret->AddRef(); // 0->1
				return ret;
			}
		}

		// create new page
		auto* page = PreparePage_NoLock(pageIndex);
		if (!page)
			return nullptr;

		// put in cache
		m_activePages[pageIndex] = page;
		assert(m_activePages[pageIndex] == page);
		return page;
	}

	void Data::ReleasePageRef(DataFramePage* page)
	{
		std::lock_guard< std::mutex > lock(m_lock);

		if (0 == --page->m_refCount)
		{
			// to many pages in the free list, remove the oldest
			if ( m_freePages.size() > MAX_FREE_PAGES )
			{ 
				uint32 oldestPageCacheToken = m_freePagesCacheIndex+1;
				uint32 oldestPageKey = 0;
				for (const auto it : m_freePages)
				{
					if (it.second->m_lruCacheToken < oldestPageCacheToken)
					{
						oldestPageKey = it.first;
						oldestPageCacheToken = it.second->m_lruCacheToken;
					}
				}

				auto it = m_freePages.find(oldestPageKey);
				delete it->second;
				m_freePages.erase(it);
			}

			// put in the free list
			m_freePages[page->GetPageIndex()] = page;

			// remove from active list
			auto it = m_activePages.find(page->GetPageIndex());
			assert(it != m_activePages.end());
			assert(it->second == page);
			m_activePages.erase(it);
		}
	}

	DataFramePage* Data::AllocPage_NoLock()
	{
		// get the oldest from the free pages
		DataFramePage* oldestPage = nullptr;
		uint32 oldestPageCacheToken = 0;
		uint32 oldestPageKey = 0;
		for (const auto it : m_freePages)
		{
			if (!oldestPage || it.second->m_lruCacheToken < oldestPageCacheToken )
			{
				oldestPageKey = it.first;
				oldestPage = it.second;
				oldestPageCacheToken = it.second->m_lruCacheToken;
			}
		}

		if (oldestPage)
		{
			// remove from list of free pages
			auto it = m_freePages.find(oldestPageKey);
			m_freePages.erase(it);

			oldestPage->AddRef(); // 0->1
			return oldestPage;
		}

		// create a new page
		auto* newPage = new DataFramePage(this, m_numFramesPerPage, m_maxDataPerFrame);
		return newPage;
	}

	static void LoadFrameData( PagedFileReader& reader, uint32 frameIndex, DataFrame& data, const Registers& regMapping )
	{
		// load header
		common::TraceDataHeader info;
		memset(&info, 0, sizeof(info));
		reader.Read(&info, sizeof(info));

		// generic info
		data.m_address = info.m_ip;
		data.m_clock = info.m_clock;
		data.m_index = frameIndex;

		// preload all values into one big buffer, simpler and faster
		static uint8 TempBuffer[16 * 512];
		reader.Read( &TempBuffer, info.m_dataSize );

		// read/write logic
		const uint8* readPtr = TempBuffer;
		uint8* writePtr = data.GetData();

		// load all registers that have data
		const auto numRegistrs = regMapping.GetNumTraceRegisters();
		for (uint32 i = 0; i < numRegistrs; ++i)
		{
			const auto regSize = regMapping.GetTraceRegisterSizes()[i];
			const auto* reg = regMapping.GetTraceRegisters()[i];

			// read only if we have data
			if (info.HasDataForReg(i))
			{
				memcpy(writePtr, readPtr, regSize);
				readPtr += regSize;
			}

			writePtr += regSize;
		}
	}

	DataFramePage* Data::PreparePage_NoLock(const uint32 pageIndex)
	{
		assert(pageIndex < m_pageOffsets.size());

		// go to proper place in the file
		const auto fileOffset = m_pageOffsets[pageIndex];
		m_fileReader->Seek(fileOffset);

		// load the full frame info, this checks that the data is valid
		common::TraceFullFrame info;
		memset(&info, 0, sizeof(info));
		m_fileReader->Read(&info, sizeof(info));
		if (info.m_magic != common::TraceFullFrame::MAGIC)
			return nullptr;

		// allocate empty page entry
		auto* page = AllocPage_NoLock();
		assert(page->m_refCount == 1); // fresh, not used elsewhere
		page->m_pageIndex = pageIndex;
		page->m_firstFrame = pageIndex * m_numFramesPerPage;
		page->m_numFrames = std::min<uint32>(m_numFrames, page->m_firstFrame + m_numFramesPerPage) - page->m_firstFrame;

		// read full data into the first 
		uint32 frameIndex = (pageIndex * m_numFramesPerPage);
		LoadFrameData(*m_fileReader, frameIndex, page->m_frames[0], m_registers);

		// read rest of the frames as delta frames
		for (uint32 i = 1; i < page->m_numFrames; ++i)
		{
			// make sure it's a delta frame
			common::TraceDeltaFrame info;
			memset(&info, 0, sizeof(info));
			m_fileReader->Read(&info, sizeof(info));
			if (info.m_magic != common::TraceDeltaFrame::MAGIC)
				break;

			// copy previous values from
			page->m_frames[i].CopyValues(page->m_frames[i-1]);
			LoadFrameData(*m_fileReader, frameIndex + i, page->m_frames[i], m_registers);
		}

		// return created frame
		return page;
	}

	void Data::BatchVisit(const uint32 firstFrame, const uint32 lastFrame, IBatchVisitor& visitor)
	{
		std::auto_ptr<DataReader> reader(CreateReader());

		const int32 delta = (lastFrame >= firstFrame) ? 1 : -1;
		uint32 frame = firstFrame;
		do 
		{
			const auto& data = reader->GetFrame(frame);
			if (!visitor.ProcessFrame(data))
				break;

			if (frame != lastFrame)
				frame += delta;
		} while (frame != lastFrame);
	}

	DataReader* Data::CreateReader() const
	{
		return new DataReader((Data*)this);
	}

	//---------------------------------------------------------------------------

	DataReader::DataReader(Data* data)
		: m_data(data)
	{
	}

	DataReader::~DataReader()
	{
		// release pages back to the system
		for (auto* page : m_pages)
			page->Release();
	}

	const uint32 DataReader::GetNumFrames() const
	{
		return m_data->m_numFrames;
	}

	const class Data& DataReader::GetData() const
	{
		return *m_data;
	}

	DataFramePage* DataReader::RequestPage(const uint32 frameIndex)
	{
		assert(frameIndex < GetNumFrames());

		// use any existing
		for (uint32 i = 0; i < m_pages.size(); ++i)
		{
			auto* page = m_pages[i];
			if (page->ContainsFrame(frameIndex))
			{
				// move to front
				if (i != 0)
				{
					// TODO: more optimal?
					m_pages.erase(m_pages.begin() + i);
					m_pages.insert(m_pages.begin(), page);
				}

				assert(page->ContainsFrame(frameIndex));
				return page;
			}
		}

		// remove last frame
		if (m_pages.size() >= MAX_CACHED_PAGES)
		{
			auto* lastPage = m_pages.back();
			lastPage->Release(); // may delete the page

			m_pages.pop_back();
		}

		// request new page from the system
		const auto pageIndex = frameIndex / m_data->GetNumDataFramesPerPage();
		auto* page = m_data->GetPage(pageIndex);
		assert(page->ContainsFrame(frameIndex));
		assert(page != nullptr);

		// put in cache
		m_pages.insert(m_pages.begin(), page);
		return page;
	}

	const DataFrame& DataReader::GetFrame(const uint32 frameIndex)
	{
		const auto maxFrameIndex = GetNumFrames() - 1;
		const uint32 clampedFrameIndex = std::min<uint32>(frameIndex, maxFrameIndex);

		const auto* page = RequestPage(clampedFrameIndex);
		assert(page != nullptr);

		return page->GetFrame(clampedFrameIndex);
	}

} // trace