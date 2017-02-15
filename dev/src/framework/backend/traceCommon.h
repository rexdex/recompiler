#pragma once

namespace common
{

	struct TraceFileHeader
	{
		static const uint32 MAGIC = 'TRAC';

		uint32		m_magic;

		char		m_platformName[16];
		char		m_cpuName[16];

		int			m_numFramesPerPage; // amount of delta frames per each full page
		int			m_numRegisers; // number of registers captured
	};

	struct TraceRegiserInfo
	{
		char		m_name[16];
		int			m_bitSize;		
	};

	struct TraceIndexFrame
	{
		static const uint32 MAGIC = 0x1111B00B; // IIIIndex Boob

		uint32		m_magic; // identifier
		uint64		m_nextIndexFrame; // offset to next index frame, zero if this is the last one
		uint64		m_fullFrameOffsets[1]; // offsets to full frames
	};

	struct TraceFullFrame
	{
		static const uint32 MAGIC = 0xFFFFB00B; // FFFFull Boob

		uint32	m_magic; // identifier
		uint32	m_nextOffset; // offset to next full frame, 0 if this is the last one, measured from the start of structure
	};

	struct TraceDeltaFrame
	{
		static const uint32 MAGIC = 0xDDDDB00B; // DDDDelta Boob

		uint32	m_magic; // identifier
	};

	struct TraceDataHeader
	{
		static const uint32 MAX_REGS = 512; // maximum supported registers

		uint32	m_ip; // instruction pointer at the capture
		uint64	m_clock; // physical clock value
		uint64	m_mask[MAX_REGS / 64]; // mask of stored register values
		uint32	m_dataSize; // size of the following data block (in bytes)

		const bool HasDataForReg(const uint32 regIndex) const
		{
			const auto arrayIndex = regIndex / 64;
			const auto arrayMask = 1ULL << (regIndex & 63);
			return 0 != (m_mask[arrayIndex] & arrayMask);
		}
	};

} // common