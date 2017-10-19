#pragma once

namespace common
{

	struct TraceFileHeader
	{
		static const uint32 MAGIC = 'TRAC';

		uint32		m_magic;

		char		m_platformName[16];
		char		m_cpuName[16];

		uint32_t	m_numRegisers; // number of registers captured
	};

	struct TraceRegiserInfo
	{
		char		m_name[16];
		uint32_t	m_bitSize;
	};

	struct TraceBlockHeader
	{
		static const uint32 MAGIC = 'BLCK';

		uint32 m_magic; // identifier
		uint32 m_writerId; // unique ID of the writer
		uint32 m_threadId; // id of the thread writing this frame or 0 for interrupt call
		uint32 m_numEntries; // number of entries in this block

		inline TraceBlockHeader()
			: m_magic(0)
			, m_writerId(0)
			, m_threadId(0)
			, m_numEntries(0) // filled later
		{}
	};

	struct TraceFrame
	{
		static const uint32 MAX_REGS = 512; // maximum supported registers
		static const uint32 MAGIC = 'FRAM';

		uint32 m_magic; // identifier
		uint32 m_seq; // sequence number (allows to have absolute order of operations)
		uint64 m_ip; // instruction pointer at the capture
		uint64 m_clock; // physical clock value
		uint8 m_mask[MAX_REGS / 8]; // mask of stored register values

		inline TraceFrame()
			: m_magic(0)
			, m_ip(0)
			, m_clock(0)
		{}

		const bool HasDataForReg(const uint32 regIndex) const
		{
			const auto arrayIndex = regIndex / 8;
			const auto arrayMask = 1 << (regIndex & 7);
			return 0 != (m_mask[arrayIndex] & arrayMask);
		}
	};

} // common