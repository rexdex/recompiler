#pragma once

/// dynamic data for given address
struct ProjectDynamicTraceDataInfo
{
	uint32					m_address;			// just for reference

	uint32					m_hasOtherEntries:1;
	uint32					m_hasBranchesTaken:1;
	uint32					m_hasMemoryData:1;

	std::vector<uint32>		m_otherEntries;		// other entries that visited the same location
	std::vector<uint32>		m_branchesTaken;	// in case of a branch this lists other entries in which the branch was taken

	struct MemInfo
	{
		uint32		m_address;	// target address of memory
		uint8		m_size;		// size of the data read/written
		uint32		m_mask;		// which bytes are affected
	};

	std::vector<MemInfo>	m_memoryAddressWrites;
	std::vector<MemInfo>	m_memoryAddressReads;

	ProjectDynamicTraceDataInfo();
};

/// dynamic data extracted from trace data
class ProjectDynamicTraceData
{
private:
	// container with dynamic data
	typedef std::map< uint32, ProjectDynamicTraceDataInfo* >		TDynamicDataMap;
	TDynamicDataMap					m_dynamicData;

	// source static trace data
	const class ProjectTraceData*	m_data;

public:
	ProjectDynamicTraceData(const class ProjectTraceData* data);
	~ProjectDynamicTraceData();

	// get dynamic data info for address, can be empty
	const ProjectDynamicTraceData* GetDynamicTraceData(const uint32 entryIndex);
};