#pragma once

namespace trace
{
	
#pragma pack(push)
#pragma pack(1)
	struct MemoryCellHistoryEntry
	{
		TraceFrameID m_seq;
		uint8_t m_value;
	};
#pragma pack(pop)

	/// cell (single byte) of memory
	/// has the features to be traced back in the history
	class RECOMPILER_API MemoryCell
	{
	public:
		MemoryCell();
		MemoryCell(const MemoryCell& other) = default;
		MemoryCell& operator=(const MemoryCell& other) = default;

		// initialize the memory cell, usually done from the trace file
		MemoryCell(const MemoryCellHistoryEntry* history, const uint32_t numHistoryEntries);

		// do we have any history data for this memory cell ?
		inline const bool HasHistory() const { return m_numHistoryEntries > 0; }

		// get cell history
		inline const MemoryCellHistoryEntry* GetHistoryEntries() const { return m_history; }

		// get number of cell's history entries
		inline const uint32 GetHistoryCount() const { return m_numHistoryEntries; }

		// get current cell value
		inline const uint8_t GetValue() const { return m_value; }

		// get the trace sequence frame for which the memory value was captured
		inline const TraceFrameID GetFrameID() const { return m_curSeq; }

		//--

		// get the "EMPTY" memory cell, allows us to allways pass references to the MemoryCell objects
		static const MemoryCell& EMPTY();

		//--

		// rewind frame to new time point, returns true if the value of the cell has changed
		bool Rewind(const TraceFrameID newSeq);

	private:
		uint8_t m_value; // captured value

		const MemoryCellHistoryEntry* m_history; // whole table
		int32_t m_numHistoryEntries; // total number of entries in the history
		int32_t m_curHistoryEntry; // entry we have applied

		TraceFrameID m_curSeq; // current sequence point
		TraceFrameID m_nextSeq; // next sequence point when we need to recompute stuff

		const TraceFrameID GetEntrySeq(int entry) const;
		const uint8_t GetEntryValue(int entry) const;

		const int32_t FindEntryIndexForSequencePoint(const int32_t curEntry, const TraceFrameID seq) const;
	};

	/// captures slice state of memory at given sequence point
	/// can be travesed in time but not in address range (a new slice must be obtained)
	class RECOMPILER_API MemorySlice
	{
	public:
		MemorySlice(const uint64_t startAddress, const uint64_t endAddress, std::vector<MemoryCell>& cells);
		~MemorySlice();

		/// get start (base) address of the memory slice
		inline const uint64_t GetStartAddress() const { return m_startAddress; }

		/// get end address of the memory slice
		/// NOTE: the end address is not in the slice
		inline const uint64_t GetEndAddress() const { return m_endAddress; }

		/// check if this memory slice contains given memory range
		inline const bool ContainsAddress(const uint64_t address) const { return (address >= m_startAddress) && (address < m_endAddress); }

		/// get sequence point for which the slice was captured
		inline const TraceFrameID GetFrameID() const { return m_seq; }

		//---

		/// get memory cell for given address, note: returned cell may not have history if it's outside the memory range
		const MemoryCell& GetMemoryCell(const uint64_t address) const;

		/// rewind memory slice to given sequence point
		void Rewind(const TraceFrameID seq);

	private:
		uint64 m_startAddress; // start address of the whole slice
		uint64 m_endAddress; // end address of the whole slice

		TraceFrameID m_seq; // current sequence point

		std::vector<MemoryCell> m_cells; // captured memory cells
	};

} // trace