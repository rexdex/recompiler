#include "build.h"
#include "traceDataFile.h"
#include "platformCPU.h"
#include "traceRawReader.h"
#include "traceDataBuilder.h"

namespace trace
{

	//---

	DataFrame::DataFrame(const LocationInfo& locInfo, const FrameType& type, const NavigationData& navi, std::vector<uint8>& data)
		: m_location(locInfo)
		, m_type(type)
		, m_navigation(navi)
		, m_data(std::move(data))
	{}

	const uint8* DataFrame::GetRegData(const platform::CPURegister* reg) const
	{
		if (m_type != FrameType::CpuInstruction)
			return nullptr;

		const auto dataOffset = reg ? reg->GetTraceDataOffset() : -1;
		return (dataOffset != -1) ? (m_data.data() + dataOffset) : nullptr;
	}

	//---

	DataFile::DataFile(const platform::CPU* cpuInfo)
		: m_cpu(cpuInfo)
	{
		std::vector<uint8> data;
		m_emptyFrame = new DataFrame(LocationInfo(), FrameType::Invalid, NavigationData(), data);
	}

	DataFile::~DataFile()
	{
		delete m_emptyFrame;
		PurgeCache();
	}

	const DataFrame& DataFile::GetFrame(const TraceFrameID id)
	{
		// invalid frame ?
		if (id >= m_entries.size())
			return *m_emptyFrame;

		// get frame from cache
		const auto it = m_cache.find(id);
		if (it != m_cache.end())
		{
			auto* frame = it->second;
			return frame ? *frame : *m_emptyFrame;
		}

		// add to cache
		auto* frame = CompileFrame(id);
		m_cache[id] = frame;

		// return frame data
		return frame ? *frame : *m_emptyFrame;
	}

	DataFrame* DataFile::CompileFrame(const TraceFrameID id)
	{
		// get the entry info
		const auto& entryInfo = m_entries[id];

		// unpack data
		const auto& blobInfo = *(const BlobInfo*)(m_dataBlob.data() + entryInfo.m_offset);

		// setup the location info
		LocationInfo info;
		info.m_ip = blobInfo.m_ip;
		info.m_seq = id;
		info.m_time = blobInfo.m_time;
		info.m_contextId = entryInfo.m_context;
		info.m_contextSeq = blobInfo.m_localSeq;

		// setup navigation info
		NavigationData navi;
		navi.m_context = entryInfo.m_context;
		navi.m_prevInContext = entryInfo.m_prevThread;
		navi.m_nextInContext = entryInfo.m_nextThread;

		// prepare data buffer
		std::vector<uint8> data;
		data.resize(m_dataFrameSize, 0);

		// get the base frame
		if (entryInfo.m_base != INVALID_TRACE_FRAME_ID)
		{
			const auto& baseFrame = GetFrame(entryInfo.m_base);
			memcpy(data.data(), baseFrame.GetRawData(), m_dataFrameSize);
		}

		// unpack the packed data
		{
			const auto* readPtr = (const uint8_t*)&blobInfo + sizeof(blobInfo);

			// read number of registers and the data for the registers
			const auto numRegs = *readPtr++;
			for (uint32 i = 0; i < numRegs; ++i)
			{
				// read register id
				const auto regIndex = *readPtr++;

				// get the register
				const auto* reg = m_registers[regIndex];
				const auto dataOffset = reg->GetTraceDataOffset();

				// load data for the register
				memcpy(data.data() + dataOffset, readPtr, reg->GetBitSize() / 8);
				readPtr += reg->GetBitSize() / 8;
			}
		}

		// prepare the frame
		const auto type = (FrameType)entryInfo.m_type;
		return new DataFrame(info, type, navi, data);
	}

	template< typename T >
	static const bool WriteDataChunk(ILogOutput& log, std::ofstream& f, const std::vector<T>& data, uint64& outPos, uint64& outSize)
	{
		const auto* readPtr = (const char*)data.data();
		const auto dataSize = data.size() * sizeof(T);
		const auto dataBlockSize = 64 * 1024;

		const auto startPos = (uint64)f.tellp();

		// TODO: add compression

		uint64_t pos = 0;
		while (pos < dataSize)
		{
			// update the visual progress
			log.SetTaskProgress((uint32)(pos / dataBlockSize), (uint32)(dataSize / dataBlockSize));

			// write the data block
			auto sizeToWrite = (pos + dataBlockSize <= dataSize) ? dataBlockSize : (dataSize - pos);
			f.write(readPtr, sizeToWrite);
			pos += sizeToWrite;
			readPtr += sizeToWrite;

			// error ?
			if (f.fail())
			{
				log.Error("Trace: Failed to save data to file. Check for disk space.");
				return false;
			}
		}

		// update chunk data
		outPos = startPos;
		outSize = (uint64)f.tellp() - startPos;

		// data saved
		return true;
	}

	const bool DataFile::Save(ILogOutput& log, const std::wstring& filePath) const
	{
		std::ofstream file(filePath, std::ios::out | std::ios::binary);

		// write file header
		FileHeader header;
		memset(&header, 0, sizeof(header));
		header.m_magic = 0;
		header.m_numRegisters = (uint32)m_registers.size();
		file.write((char*)&header, sizeof(header));

		// write the registers
		for (const auto* reg : m_registers)
		{
			FileRegister regInfo;
			strcpy_s(regInfo.m_name, reg->GetName());
			regInfo.m_dataSize = reg->GetBitSize() / 8;
			file.write((char*)&regInfo, sizeof(regInfo));
		}

		// write the chunks
		{
			log.SetTaskName("Saving context table...");
			auto& info = header.m_chunks[CHUNK_CONTEXTS];
			if (!WriteDataChunk(log, file, m_contexts, info.m_dataOffset, info.m_dataSize))
				return false;
		}
		{
			log.SetTaskName("Saving entry offset table...");
			auto& info = header.m_chunks[CHUNK_ENTRIES];
			if (!WriteDataChunk(log, file, m_entries, info.m_dataOffset, info.m_dataSize))
				return false;
		}
		{
			log.SetTaskName("Saving data blob...");
			auto& info = header.m_chunks[CHUNK_DATA_BLOB];
			if (!WriteDataChunk(log, file, m_dataBlob, info.m_dataOffset, info.m_dataSize))
				return false;
		}

		// patch the header
		const auto fileDataSize = file.tellp();
		file.seekp(0);
		header.m_magic = MAGIC; // allows us to read the file
		file.write((char*)&header, sizeof(header));

		// saved
		log.Log("Trace: Saved %1.2fMB of data", (double)(uint64)file.tellp() / (1024.0*1024.0));
		return true;
	}

	template< typename T >
	static const bool ReadDataChunk(ILogOutput& log, std::ifstream& f, std::vector<T>& data, const uint64 chunkPos, const uint64 chunkSize)
	{
		// move to position in file
		f.seekg(chunkPos);

		// create entries
		{
			const auto numEntries = chunkSize / sizeof(T);
			data.resize(numEntries);
		}

		// load data
		auto writePtr = (char*)data.data();
		const auto dataBlockSize = 64 * 1024;

		// TODO: add compression

		uint64_t pos = 0;
		while (pos < chunkSize)
		{
			// update the visual progress
			log.SetTaskProgress((uint32)(pos / dataBlockSize), (uint32)(chunkSize / dataBlockSize));

			// write the data block
			auto sizeToRead = (pos + dataBlockSize <= chunkSize) ? dataBlockSize : (chunkSize - pos);
			f.read(writePtr, sizeToRead);
			pos += sizeToRead;
			writePtr += sizeToRead;

			// error ?
			if (f.fail())
			{
				log.Error("Trace: Failed to load data from file");
				return false;
			}
		}
		
		// data saved
		return true;
	}

	std::unique_ptr<DataFile> DataFile::Load(ILogOutput& log, const platform::CPU& cpuInfo, const std::wstring& filePath)
	{
		std::ifstream file(filePath, std::ios::in | std::ios::binary);
		if (file.fail())
		{
			log.Error("Trace: Unable to open file '%ls'", filePath.c_str());
			return nullptr;
		}

		// load the file header
		FileHeader header;
		memset(&header, 0, sizeof(header));
		file.read((char*)&header, sizeof(header));

		// check that the file is valid
		if (header.m_magic != MAGIC)
		{
			log.Error("Trace: File '%ls' does not have a valid header", filePath.c_str());
			return nullptr;
		}

		// load the registers
		uint32_t traceDataOffsetPos = 0;
		std::unique_ptr<DataFile> ret(new DataFile(&cpuInfo));
		for (uint32 i = 0; i < header.m_numRegisters; ++i)
		{
			// load register information
			FileRegister regInfo;
			memset(&regInfo, 0, sizeof(regInfo));
			file.read((char*)&regInfo, sizeof(regInfo));

			// lookup register in the cpu
			const auto* cpuReg = cpuInfo.FindRegister(regInfo.m_name);
			if (!cpuReg)
			{
				log.Error("Trace: File '%ls' uses unknown register '%hs'", filePath.c_str(), regInfo.m_name);
				return nullptr;
			}

			// check data size
			const auto cpuRegSize = cpuReg->GetBitSize() / 8;
			if (cpuRegSize != regInfo.m_dataSize)
			{
				log.Error("Trace: File '%ls' uses register '%hs' with different size (%u) that currently present (%u)", filePath.c_str(), regInfo.m_name, regInfo.m_dataSize, cpuRegSize);
				return nullptr;
			}

			// assign data offset
			// TODO: make it less hacky
			cpuReg->BindToTrace(i, traceDataOffsetPos);
			traceDataOffsetPos += regInfo.m_dataSize;

			// add to register table
			ret->m_registers.push_back(cpuReg);
		}

		// remember how much memory we need for one frame worth of data
		log.Log("Trace: Register data consumes %u bytes per frame", traceDataOffsetPos);
		ret->m_dataFrameSize = traceDataOffsetPos;

		// load the chunks
		{
			log.SetTaskName("Loading context table...");
			auto& info = header.m_chunks[CHUNK_CONTEXTS];
			if (!ReadDataChunk(log, file, ret->m_contexts, info.m_dataOffset, info.m_dataSize))
				return false;
		}
		{
			log.SetTaskName("Loading entry offset table...");
			auto& info = header.m_chunks[CHUNK_ENTRIES];
			if (!ReadDataChunk(log, file, ret->m_entries, info.m_dataOffset, info.m_dataSize))
				return false;
		}
		{
			log.SetTaskName("Loading data blob...");
			auto& info = header.m_chunks[CHUNK_DATA_BLOB];
			if (!ReadDataChunk(log, file, ret->m_dataBlob, info.m_dataOffset, info.m_dataSize))
				return false;
		}

		// loaded
		ret->PostLoad();
		return ret;
	}

	std::unique_ptr<DataFile> DataFile::Build(ILogOutput& log, const platform::CPU& cpuInfo, const RawTraceReader& rawTrace)
	{
		std::unique_ptr<DataFile> ret(new DataFile(&cpuInfo));

		// create registers
		uint32_t traceDataOffsetPos = 0;
		const auto numTraceRegisters = rawTrace.GetRegisters().size();
		for (uint32_t i=0; i<numTraceRegisters; ++i)
		{
			const auto& regInfo = rawTrace.GetRegisters()[i];

			// lookup register in the cpu
			const auto* cpuReg = cpuInfo.FindRegister(regInfo.m_name.c_str());
			if (!cpuReg)
			{
				log.Error("Trace: Trace uses unknown register '%hs'", regInfo.m_name.c_str());
				return nullptr;
			}

			// check data size
			const auto cpuRegSize = cpuReg->GetBitSize() / 8;
			if (cpuRegSize != regInfo.m_dataSize)
			{
				log.Error("Trace: Trace uses register '%hs' with different size (%u) that currently present (%u)", regInfo.m_name.c_str(), regInfo.m_dataSize, cpuRegSize);
				return nullptr;
			}

			// assign data offset
			// TODO: make it less hacky
			cpuReg->BindToTrace(i, traceDataOffsetPos);
			traceDataOffsetPos += regInfo.m_dataSize;

			// add to register table
			ret->m_registers.push_back(cpuReg);
		}

		// remember how much memory we need for one frame worth of data
		log.Log("Trace: Register data consumes %u bytes per frame", traceDataOffsetPos);
		ret->m_dataFrameSize = traceDataOffsetPos;

		// build the data
		DataBuilder builder(rawTrace);
		rawTrace.Scan(log, builder);

		// extract data
		builder.m_blob.exportToVector(ret->m_dataBlob);
		builder.m_entries.exportToVector(ret->m_entries);
		builder.m_contexts.exportToVector(ret->m_contexts);

		// done
		return ret;
	}

	void DataFile::PurgeCache()
	{
		for (auto it : m_cache)
			delete it.second;
		m_cache.clear();
	}

	void DataFile::ManageCache()
	{
		static const uint32 MAX_FRAMES = 1 << 20;

		if (m_cache.size() > MAX_FRAMES)
			PurgeCache();
	}

	void DataFile::PostLoad()
	{
	}

} // trace

