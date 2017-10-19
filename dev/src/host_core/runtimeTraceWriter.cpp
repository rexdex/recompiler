#include "build.h"
#include "runtimeRegisterBank.h"
#include "runtimeTraceWriter.h"
#include "runtimeTraceFile.h"
#include "runtimeCPU.h"

#include "../recompiler_core/traceCommon.h"

namespace runtime
{

	TraceWriter::TraceWriter(TraceFile* owner, const uint32_t threadId, std::atomic<uint32_t>& sequenceNumber, const char* name)
		: m_owner(owner)
		, m_frameIndex(0)
		, m_localWriteBufferPos(0)
		, m_localWriteStartFrameIndex(0)
		, m_threadId(threadId)
		, m_sequenceNumber(&sequenceNumber)
		, m_name(name)
	{
		static std::atomic<uint32_t> WriterIDAllocator(0);
		m_writerId = WriterIDAllocator++;

		memset(&m_prevData, 0, sizeof(m_prevData));
	}

	void TraceWriter::Detach()
	{
		LocalFlush();
		m_owner = nullptr;
	}

	TraceWriter::~TraceWriter()
	{
		LocalFlush();
	}

	void TraceWriter::LocalFlush()
	{
		if (m_localWriteBufferPos > 0)
		{
			// patch data
			auto* header = (common::TraceBlockHeader*)&m_localWriteBuffer[0];
			header->m_magic = common::TraceBlockHeader::MAGIC;
			header->m_numEntries = (uint32)(m_frameIndex - m_localWriteStartFrameIndex);
			//header->m_size = m_localWriteBufferPos;

			// write the data
			if (m_owner != nullptr)
				m_owner->WriteBlockAsync(m_localWriteBuffer, m_localWriteBufferPos);
			m_localWriteBufferPos = 0;
			m_localWriteStartFrameIndex = m_frameIndex;
		}
	}

	void TraceWriter::AddFrame(const uint64 ip, const runtime::RegisterBank& regs)
	{
		// no owner
		if (m_owner == nullptr)
			return;

		// do no add empty frames
		if (!ip)
			return;

		// when starting the block, write the full frame
		if (m_localWriteBufferPos == 0)
			WriteBlockHeader();

		// compute the deltas between current and previous frames
		WriteDeltaFrame(ip, regs);

		// flush when getting close to the end of block
		if (m_localWriteBufferPos > (LOCAL_WRITE_BUFFER_SIZE - GUARD_AREA_SIZE))
			LocalFlush();
	}
	
	void TraceWriter::WriteBlockHeader()
	{
		auto* info = LocalWrite<common::TraceBlockHeader>();
		info->m_magic = 0xDEADBEAF;
		info->m_threadId = m_threadId;
		info->m_writerId = m_writerId;
	}

	static inline bool ComparePtr(const uint8* a, const uint8* b, const uint32 size)
	{
		const auto end = a + size;
		while (a < end)
			if (*a++ != *b++)
				return false;

		return true;
	}

	static inline void CopyPtr(const uint8* readPtr, uint8*& writePtr, const uint32 size)
	{
		const auto end = readPtr + size;
		while (readPtr < end)
			*writePtr++ = *readPtr++;
	}
		
	void TraceWriter::WriteDeltaFrame(const uint64_t ip, const runtime::RegisterBank& regs)
	{
		// count written frames
		++m_frameIndex;

		// write frame header (will be patched)
		auto* frame = LocalWrite<common::TraceFrame>();
		frame->m_magic = common::TraceFrame::MAGIC;
		frame->m_ip = ip;
		frame->m_clock = __rdtsc();
		frame->m_seq = (*m_sequenceNumber)++;
		memset(frame->m_mask, 0, sizeof(frame->m_mask));

		// save current position
		auto pos = m_localWriteBufferPos;
		int writtenRegs = 0;

		// start writing data
		{
			const auto numRegs = m_owner->m_numRegsToWrite;
			const auto* info = m_owner->m_registersToWrite;

			const auto* curBase = (const uint8*)&regs;
			auto* prevBase = (uint8*)m_prevData;
			for (uint32 i = 0; i < numRegs; ++i, ++info)
			{
				const auto* curPtr = curBase + info->m_dataOffset;
				auto* prevPtr = prevBase + info->m_dataOffset;

				// data different ?
				if (!ComparePtr(curPtr, prevPtr, info->m_size))
				{
					frame->m_mask[i / 8] |= (1 << (i & 7));
					LocalWrite(curPtr, info->m_size);
					CopyPtr(curPtr, prevPtr, info->m_size);
					writtenRegs += 1;
				}
			}
		}
	}

} // runtime