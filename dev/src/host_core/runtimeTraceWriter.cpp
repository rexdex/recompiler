#include "build.h"
#include "runtimeRegisterBank.h"
#include "runtimeTraceWriter.h"
#include "runtimeCPU.h"

#include "../recompiler_core/traceCommon.h"

namespace runtime
{

	TraceWriter::TraceWriter(const runtime::RegisterBankInfo& bankInfo, HANDLE hFileHandle)
		: m_outputFile(hFileHandle)
		, m_frameIndex(0)
		, m_prevBlockSkipOffset(0)
		, m_numFramesPerPage(1024)
		, m_numRegsToWrite(0)
		, m_fullWriteSize(0)
	{
		// write header
		common::TraceFileHeader header;
		header.m_magic = common::TraceFileHeader::MAGIC;
		header.m_numFramesPerPage = m_numFramesPerPage;
		header.m_numRegisers = bankInfo.GetNumRootRegisters();
		strcpy_s(header.m_cpuName, bankInfo.GetCPUName().c_str());
		strcpy_s(header.m_platformName, bankInfo.GetPlatformName().c_str());
		Write(&header, sizeof(header));

		// bit masks
		memset(&m_deltaWwriteRegMask, 0, sizeof(m_deltaWwriteRegMask));
		memset(&m_fullWriteRegMask, 0, sizeof(m_fullWriteRegMask));

		// prepare register write list, also, write the register information
		for (uint32 i = 0; i < bankInfo.GetNumRootRegisters(); ++i)
		{
			const auto* reg = bankInfo.GetRootRegister(i);

			common::TraceRegiserInfo regInfo;
			regInfo.m_bitSize = reg->GetBitCount();
			strcpy_s(regInfo.m_name, reg->GetName());
			Write(&regInfo, sizeof(regInfo));

			RegToWrite& writeInfo = m_registersToWrite[m_numRegsToWrite++];
			writeInfo.m_dataOffset = reg->GetDataOffset();
			writeInfo.m_size = reg->GetBitCount() / 8;
			m_fullWriteSize += writeInfo.m_size;

			m_fullWriteRegMask[i / 64] |= (1ULL << (i & 63));
		}

		GLog.Log("Trace: Trace contains %d registers to write (%d bytes in full frame)", m_numRegsToWrite, m_fullWriteSize);
	}

	TraceWriter::~TraceWriter()
	{
		if (m_outputFile != INVALID_HANDLE_VALUE)
		{
			GLog.Log("Trace: Written %d frames, %llu bytes of data", m_frameIndex, GetOffset());

			CloseHandle(m_outputFile);
			m_outputFile = INVALID_HANDLE_VALUE;
		}
	}

	void TraceWriter::Flush()
	{
	}

	void TraceWriter::CapureData(const runtime::RegisterBank& regs)
	{
		const auto* info = m_registersToWrite;
		uint8* writePtr = m_curData;
		for (uint32 i = 0; i < m_numRegsToWrite; ++i, ++info)
		{
			const uint8* readPtr = (const uint8*)&regs + info->m_dataOffset;
			const uint8* endReadPtr = readPtr + info->m_size;
			while (readPtr < endReadPtr)
				*writePtr++ = *readPtr++;
		}
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

	void TraceWriter::BuildDeltaData()
	{
		m_deltaWriteSize = 0;

		const auto* info = m_registersToWrite;

		for (uint32 i = 0; i < ARRAYSIZE(m_deltaWwriteRegMask); ++i)
			m_deltaWwriteRegMask[i] = 0;

		uint8* writePtr = m_deltaWriteBuffer;
		const auto* curPtr = m_curData;
		const auto* prevPtr = m_prevData;
		for (uint32 i = 0; i < m_numRegsToWrite; ++i, ++info)
		{
			// data different ?
			if (!ComparePtr(curPtr, prevPtr, info->m_size))
			{
				m_deltaWwriteRegMask[i / 64] |= (1ULL << (i & 63));
				CopyPtr(curPtr, writePtr, info->m_size);
			}

			curPtr += info->m_size;
			prevPtr += info->m_size;
		}

		// compute size of data to write
		m_deltaWriteSize = (uint32)(writePtr - m_deltaWriteBuffer);
	}

	void TraceWriter::AddFrame(const uint64 ip, const runtime::RegisterBank& regs)
	{
		// do no add empty frames
		if (!ip)
			return;

		// should we write a full frame
		const bool isFullFrame = (0 == (m_frameIndex % m_numFramesPerPage));
		if (isFullFrame)
		{
			const auto currentOffset = GetOffset();

			// patch the previous offset
			if (m_prevBlockSkipOffset > 0)
			{
				common::TraceFullFrame prevInfo;
				prevInfo.m_magic = common::TraceFullFrame::MAGIC;
				prevInfo.m_nextOffset = (uint32)(currentOffset - m_prevBlockSkipOffset);
				SetOffset(m_prevBlockSkipOffset);
				Write(&prevInfo, sizeof(prevInfo));
				SetOffset(currentOffset);
			}

			// make sure the block we are just about to write will get patched
			m_prevBlockSkipOffset = currentOffset;

			// write the full frame header
			common::TraceFullFrame info;
			info.m_magic = common::TraceFullFrame::MAGIC;
			info.m_nextOffset = 0; // until patched
			Write(&info, sizeof(info));
		}
		else
		{
			// write the delta frame header
			common::TraceDeltaFrame info;
			info.m_magic = common::TraceDeltaFrame::MAGIC;
			Write(&info, sizeof(info));
		}

		// capture current data
		CapureData(regs);

		// build delta frame
		if (!isFullFrame)
			BuildDeltaData();

		// setup data header
		common::TraceDataHeader info;
		info.m_ip = (uint32)ip;
		info.m_clock = __rdtsc();
		info.m_dataSize = isFullFrame ? m_fullWriteSize : m_deltaWriteSize;
		for (uint32 i = 0; i < ARRAYSIZE(info.m_mask); ++i)
			info.m_mask[i] = isFullFrame ? m_fullWriteRegMask[i] : m_deltaWwriteRegMask[i];
		Write(&info, sizeof(info));

		// write actual data
		if (isFullFrame)
			Write(m_curData, m_fullWriteSize);
		else
			Write(m_deltaWriteBuffer, m_deltaWriteSize);

		// use current data as base for next frame
		memcpy(m_prevData, m_curData, m_fullWriteSize);

		// advance
		m_frameIndex += 1;
	}

	//----

	void TraceWriter::SetOffset(const uint64 offset)
	{
		LARGE_INTEGER pos;
		pos.QuadPart = offset;
		SetFilePointerEx(m_outputFile, pos, NULL, FILE_BEGIN);
	}

	uint64 TraceWriter::GetOffset() const
	{
		LARGE_INTEGER pos;
		pos.QuadPart = 0;
		if (SetFilePointerEx(m_outputFile, pos, &pos, FILE_CURRENT))
			return pos.QuadPart;
		return 0;
	}

	void TraceWriter::Write(const void* data, const uint32 size)
	{
		DWORD numWritten = 0;
		WriteFile(m_outputFile, data, size, &numWritten, NULL);
	}

	//----

	TraceWriter* TraceWriter::CreateTrace(const runtime::RegisterBankInfo& bankInfo, const std::wstring& outputFile)
	{
		// open file
		HANDLE hFile = CreateFileW(outputFile.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			GLog.Err("TraceLog: Failed to create trace file '%ls'", outputFile.c_str());
			return nullptr;
		}

		// create writer
		return new TraceWriter(bankInfo, hFile);
	}

} // runtime