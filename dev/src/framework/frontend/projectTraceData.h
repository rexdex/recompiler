#pragma once

#include "../backend/traceData.h"

class Project;

namespace trace
{
	class CallTree;
	class MemoryHistory;
}

namespace platform
{
	class CPURegister;
	class CPU;
}

struct ProjectTraceMemoryHistoryInfo
{
	static const uint32 MAX_SIZE = 16; // vec4

	uint32		m_entry;		// trace location
	uint16		m_validMask;	// bytes affected
	uint16		m_opMask;		// 1-write 0-read for affected bytes
	uint8		m_data[16];		// affected data
};

struct ProjectTraceMemoryAccessEntry
{
	uint32		m_address;		// code address
	uint32		m_trace;		// trace location
	uint8		m_type;			// memory access type (1-read, 2-write)
	uint8		m_size;			// memory access size
	std::string m_data;		// data written/read
};

struct ProjectTraceRegisterHistoryInfo
{
	uint32		m_traceEntry;
	uint32		m_op; // 1-write 0-read
};

struct ProjectTraceCallHistory
{
	std::string		m_functionName;
	uint32			m_functionStart;
	uint32			m_traceEnter;
	uint32			m_traceLeave; // 0 if no leave

	typedef std::vector<ProjectTraceCallHistory> TCalls;
	TCalls			m_childCalls;
	
	ProjectTraceCallHistory()
		: m_functionStart(0)
		, m_traceEnter(0)
		, m_traceLeave(0)
	{}
};

struct ProjectTraceAddressHistoryInfo
{
	uint32							m_traceEntry;
	const platform::CPURegister*	m_register;
	std::string						m_value;

	ProjectTraceAddressHistoryInfo()
		: m_traceEntry(0)
		, m_register(nullptr)
	{}
};

/// Browsable trace data
class ProjectTraceData
{
public:
	ProjectTraceData();
	~ProjectTraceData();

	// get owning project
	inline Project& GetProject() const { return *m_project; } 
 
	// get current trace address
	const uint32 GetCurrentAddress() const;

	// ger current entry index
	const uint32 GetCurrentEntryIndex() const;

	// get number of frames
	const uint32 GetNumEntries() const;

	// get current register frames
	const trace::DataFrame& GetCurrentFrame() const;

	// get next register frame
	const trace::DataFrame& GetNextFrame() const;

	// get register list
	const trace::Registers& GetRegisters() const;

	// get callstack data, can be NULL
	const trace::CallTree* GetCallstack() const;

	// create time machine trace
	class timemachine::Trace* CreateTimeMachineTrace( const uint32 traceFrameIndex );

	// reset
	bool Reset();

	// move to entry
	bool MoveToEntry(const uint32 entryIndex);

	// get next addrses with breakpoint
	int GetNextBreakEntry() const;

	// step back to the previoud address (if possible)
	int GetStepBackEntry() const;

	// step into current instruction (if possible)
	int GetStepIntoEntry() const;

	// step over current instruction (if possible)
	int GetStepOverEntry() const;

	// step out from current block (function) if possible
	int GetStepOutEntry() const;

	// visit range of trace frames
	void BatchVisit( const uint32 firstEntry, const uint32 lastEntry, trace::IBatchVisitor& dataInterface ) const;

	// visit all trace frames
	void BatchVisitAll(trace::IBatchVisitor& dataInterface) const;

	// build memory map
	bool BuildMemoryMap();

	// build callstack
	bool BuildCallstack(ILogOutput& log);

	// get trace entries that modified given memory range
	bool GetMemoryHistory(const uint32 address, const uint32 size, std::vector<ProjectTraceMemoryHistoryInfo>& outHistory) const;

	// get history of register values at given code address
	bool GetRegisterHistory(const uint32 codeAddress, const char* regName, const int startTrace, const int endTrace, std::vector< ProjectTraceAddressHistoryInfo >& outHistory ) const;

	// find first memory access 
	bool FindFirstMemoryAccess(const uint32 startEntry, const uint32 memoryAddress, const uint32 size, const uint8 actionMask, uint32& outEntryIndex, uint32& outCodeAddress) const;

	// find all memory access
	bool FindAllMemoryAccess(const uint32 memoryAddress, const uint32 size, const uint8 actionMask, std::vector< ProjectTraceMemoryAccessEntry >& outEntries ) const;

	// find forward address that is reading/writing from given memory locations
	bool FindForwardMemoryAccess(const uint32 startEntry, const uint32 memoryAddress, const uint32 size, const uint8 actionMask, uint32& outEntryIndex, uint32& outCodeAddress) const;

	// find backward address that is reading/writing from given memory locations
	bool FindBackwardMemoryAccess(const uint32 startEntry, const uint32 memoryAddress, const uint32 size, const uint8 actionMask, uint32& outEntryIndex, uint32& outCodeAddress) const;

	// find forward register access
	bool FindForwardRegisterAccess(const uint32 startEntry, const char* regName, const uint8 actionMask, uint32& outEntryIndex, uint32& outCodeAddress) const;

	// find backward register access
	bool FindBackwardRegisterAccess(const uint32 startEntry, const char* regName, const uint8 actionMask, uint32& outEntryIndex, uint32& outCodeAddress) const;

	//---

	// load from file
	static ProjectTraceData* LoadFromFile(ILogOutput& log, Project* project, const std::wstring& filePath );

private:
	std::wstring			m_filePath;

	Project*				m_project;

	trace::Data*			m_data;			// native data

	// memory map
	trace::MemoryHistory*	m_memory;		// native memory capture

	// callstack
	trace::CallTree*		m_callstack;	// native callstack capture

	// default reader
	trace::DataReader*		m_reader;
	trace::MemoryHistoryReader*	m_memReader;

	// current trace entry
	uint32					m_currentEntry;
	uint64					m_currentEntryAddress;
};