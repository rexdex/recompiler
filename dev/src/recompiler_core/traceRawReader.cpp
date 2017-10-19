#include "build.h"
#include "traceRawReader.h"
#include "traceCommon.h"

//#pragma optimize("",off)

namespace trace
{

	RawTraceReader::RawTraceReader(std::unique_ptr<std::ifstream>& f)
		: m_file(std::move(f))
	{}

	RawTraceReader::~RawTraceReader()
	{}

	void RawTraceReader::Read(void* data, const uint32_t size) const
	{
		m_file->read((char*)data, size);
	}

	void RawTraceReader::DecodeFrameData(const uint8* mask, uint8* refData, uint8* curData) const
	{
		const auto REGS_PER_WORD = 8;

		// process registers
		const auto numRegs = m_registers.size();
		const auto numMaskWords = (numRegs + (REGS_PER_WORD-1)) / REGS_PER_WORD;
		uint32 regIndex = 0;
		for (uint32_t i = 0; i < numMaskWords; ++i)
		{
			// skip groups
			if (!mask[i])
			{
				regIndex += REGS_PER_WORD;
				continue;
			}

			// process registers
			for (uint32_t j = 0; j < REGS_PER_WORD && regIndex < numRegs; ++j, ++regIndex)
			{
				// skip registers that are not set
				if (0 == (mask[i] & (1 << j)))
					continue;

				// load data for this register
				const auto& regInfo = m_registers[regIndex];
				auto* targetData = refData + regInfo.m_dataOffset;
				Read(targetData, regInfo.m_dataSize);
			}
		}

		// snapshot the data after decoding the frame
		memcpy(curData, refData, m_frameSize);
	}

	void RawTraceReader::Scan(ILogOutput& log, IRawTraceVisitor& vistor) const
	{
		// reset the file position
		m_file->seekg(m_postHeaderOffset);

		// loading context for each found block
		std::vector<Context*> contextTables;

		// the raw frame
		RawTraceFrame rawFrame;
		rawFrame.m_data.resize(m_frameSize, 0);

		// load all data until end of file has been reached
		while ((uint64)m_file->tellg() < m_fileSize)
		{
			// update the file position
			log.SetTaskProgress((uint32)((uint64)m_file->tellg() / 1024), (uint32)(m_fileSize / 1024));

			// load the block header
			common::TraceBlockHeader header;
			Read(&header, sizeof(header));

			// not a valid header
			if (header.m_magic != common::TraceBlockHeader::MAGIC)
			{
				log.Warn("Trace: Invalid block header at %llu (%1.2f%% of file). Stopping trace loading.",
					m_file->tellg(), ((double)m_file->tellg() / (double)m_fileSize) * 100.0);
				break;
			}

			// do we have enough data ?
			/*if ((uint64)m_file->tellg() + header.m_size > m_fileSize)
			{
				log.Warn("Trace: Last trace block was not written fully. It wont be considered.");
				break;
			}*/

			// get/allocate context table
			const auto writerID = header.m_writerId;
			if (writerID >= contextTables.size())
				contextTables.resize((writerID & ~63) + 64, nullptr);

			auto* context = contextTables[writerID];
			if (!context)
			{
				context = new Context(header.m_writerId, header.m_threadId, m_frameSize);
				contextTables[header.m_writerId] = context;
			}

			// load frame
			for (uint32 i = 0; i < header.m_numEntries; ++i)
			{
				// load frame header
				common::TraceFrame frame;
				Read(&frame, sizeof(frame));

				// invalid frame ?
				if (frame.m_magic != common::TraceFrame::MAGIC)
				{
					log.Warn("Trace: Invalid frame header at %llu (%1.2f%% of file). Stopping trace loading.",
						m_file->tellg(), ((double)m_file->tellg() / (double)m_fileSize) * 100.0);
					break;
				}

				// report start of the block
				if (context->m_numEntries == 0)
					vistor.StartContext(context->m_writerId, context->m_threadId, frame.m_ip, frame.m_seq, "");

				// decode frame data
				DecodeFrameData(frame.m_mask, context->m_refData.data(), rawFrame.m_data.data());

				// setup rest 
				rawFrame.m_seq = frame.m_seq;
				rawFrame.m_ip = frame.m_ip;
				rawFrame.m_threadId = context->m_threadId;
				rawFrame.m_writerId = context->m_writerId;
				rawFrame.m_timeStamp = frame.m_clock;
				vistor.ConsumeFrame(context->m_writerId, frame.m_seq, rawFrame);

				// remember last sequence number of a context, this can be used to close it
				context->m_lastSeq = frame.m_seq;
				context->m_lastIp = frame.m_ip;
				context->m_numEntries += 1;
			}
		}

		// end all active contexts
		for (auto* ctx : contextTables)
		{
			if (ctx)
			{
				vistor.EndContext(ctx->m_writerId, ctx->m_lastIp, ctx->m_lastSeq, ctx->m_numEntries);
			}
		}
	}

	std::unique_ptr<RawTraceReader> RawTraceReader::Load(ILogOutput& log, const std::wstring& rawTraceFilePath)
	{
		// open file
		auto file = std::make_unique<std::ifstream>(rawTraceFilePath, std::ios::binary | std::ios::in);
		if (!file || file->fail())
		{
			log.Error("Trace: Unable to open file '%ls'", rawTraceFilePath.c_str());
			return nullptr;
		}

		// load file header
		common::TraceFileHeader header;
		file->read((char*)&header, sizeof(header));
		if (header.m_magic != common::TraceFileHeader::MAGIC)
		{
			log.Error("Trace: Opened file is not a trace file");
			return nullptr;
		}

		// create the basic reader
		std::unique_ptr<RawTraceReader> ret(new RawTraceReader(file));
		ret->m_cpuName = header.m_cpuName;
		ret->m_platformName = header.m_platformName;
		log.Log("Trace: Loaded trace for platform '%hs', cpu '%hs'", ret->m_platformName.c_str(), ret->m_cpuName.c_str());

		// map registers
		uint32_t dataOffset = 0;
		ret->m_registers.reserve(header.m_numRegisers);
		for (uint32 i = 0; i < header.m_numRegisers; ++i)
		{
			common::TraceRegiserInfo regInfo;
			ret->m_file->read((char*)&regInfo, sizeof(regInfo));

			RegInfo info;
			info.m_name = regInfo.m_name;
			info.m_dataSize = regInfo.m_bitSize / 8;
			info.m_dataOffset = dataOffset;
			dataOffset += info.m_dataSize;

			log.Log("Trace: Found register '%hs', data offset %u, data size %u", info.m_name.c_str(), info.m_dataOffset, info.m_dataSize);
			ret->m_registers.push_back(info);
		}

		// each raw trace frame is as big as all the register data (huge)
		log.Log("Trace: Trace contains %u registers, %u bytes of data in full frame", header.m_numRegisers, dataOffset);
		ret->m_frameSize = dataOffset;
		ret->m_postHeaderOffset = ret->m_file->tellg();

		// measure file size
		ret->m_file->seekg(0, std::ios::end);
		ret->m_fileSize = ret->m_file->tellg();

		return ret;
	}

} // trace