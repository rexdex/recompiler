#include "build.h"
#include "traceMemorySlice.h"

namespace trace
{

	//--

	MemoryCell::MemoryCell()
		: m_value(0)
		, m_history(nullptr)
		, m_numHistoryEntries(0)
		, m_curHistoryEntry(0)
		, m_curSeq(INVALID_TRACE_FRAME_ID)
		, m_nextSeq(INVALID_TRACE_FRAME_ID)
	{}

	MemoryCell::MemoryCell(const MemoryCellHistoryEntry* history, const uint32_t numHistoryEntries)
		: m_value(0)
		, m_history(history)
		, m_numHistoryEntries(numHistoryEntries)
		, m_curHistoryEntry(-1) // before first write
		, m_curSeq(INVALID_TRACE_FRAME_ID)
		, m_nextSeq(INVALID_TRACE_FRAME_ID)
	{}

	const TraceFrameID MemoryCell::GetEntrySeq(int entry) const
	{
		if (entry < 0)
			return 0;
		else if (entry >= m_numHistoryEntries)
			return INVALID_TRACE_FRAME_ID;
		else
			return m_history[entry].m_seq;
	}

	const uint8_t MemoryCell::GetEntryValue(int entry) const
	{
		if (entry < 0)
			return 0;
		else if (entry >= m_numHistoryEntries)
			return 0xCD;
		else
			return m_history[entry].m_value;
	}

	const int32_t MemoryCell::FindEntryIndexForSequencePoint(const int32_t curEntry, const TraceFrameID newSeq) const
	{
		// no history
		if (m_numHistoryEntries == 0)
			return -1;

		// make our job a little bit easier in one entry case
		if (m_numHistoryEntries == 1)
			return (newSeq >= m_history[0].m_seq) ? 0 : -1;

		// linear search
		int32_t newIndex = -1;
		for (int i = 0; i < m_numHistoryEntries; ++i)
		{
			const auto& entry = m_history[i];
			if (newSeq < entry.m_seq)
				break;
			newIndex = i;
		}

		/*
		// our search will go forwards or backwards depending if the next sequence point is in the future or past
		int newIndex = curEntry;
		const auto curSeq = GetEntrySeq(curEntry);
		if (newSeq > curSeq)
		{
			// go forward in the history if we can
			const auto lastValidEntryIndex = m_numHistoryEntries - 1;
			while (newIndex < lastValidEntryIndex)
			{
				const auto nextSeq = m_history[newIndex+1].m_seq; // the future write
				if (newSeq < nextSeq)
					break;
				++newIndex;
			}
		}
		else if (newSeq < curSeq)
		{
			// go back in the history looking for first write that is in the scope of given sequence point
			while (newIndex > 0)
			{
				const auto& entry = m_history[newIndex];
				if (newSeq >= entry.m_seq) // write from this entry happens before or at given time, this is a good entry to use
					break;
				newIndex -= 1;
			}
		}*/

		// return new entry index
		return newIndex;
	}

	bool MemoryCell::Rewind(const TraceFrameID newSeq)
	{
		// do nothing if the sequence ID is within the limits of current cell
		if (newSeq >= m_curSeq && newSeq < m_nextSeq)
			return false;

		// walk the history and get the new history for a given sequence point
		const int newEntryIndex = FindEntryIndexForSequencePoint(m_curHistoryEntry, newSeq);
		if (newEntryIndex == m_curHistoryEntry) // we haven't moreved far...
			return false;

		// set new value
		const auto oldValue = m_value;
		m_curHistoryEntry = newEntryIndex;
		m_value = GetEntryValue(newEntryIndex);
		m_curSeq = GetEntrySeq(newEntryIndex);
		m_nextSeq = GetEntrySeq(newEntryIndex+1);

		// the value can still be the same but we need to update the display :)
		return true;
	}

	static MemoryCell GEmptyMemoryCell;

	const MemoryCell& MemoryCell::EMPTY()
	{
		return GEmptyMemoryCell;
	}

	//--

	MemorySlice::MemorySlice(const uint64_t startAddress, const uint64_t endAddress, std::vector<MemoryCell>& cells)
		: m_startAddress(startAddress)
		, m_endAddress(endAddress)
		, m_seq(0)
		, m_cells(std::move(cells))
	{}

	MemorySlice::~MemorySlice()
	{}

	const MemoryCell& MemorySlice::GetMemoryCell(const uint64_t address) const
	{
		if (!ContainsAddress(address))
			return MemoryCell::EMPTY();

		const auto offset = address - m_startAddress;
		return m_cells[offset];
	}

	void MemorySlice::Rewind(const TraceFrameID seq)
	{
		for (auto& cell : m_cells)
			cell.Rewind(seq);
	}

	//--


} // trace
