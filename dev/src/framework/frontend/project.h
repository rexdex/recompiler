#pragma once

#include "projectAddressHistory.h"
#include "projectTraceData.h"
#include "projectConfiguration.h"

/// Editable project
class Project
{
public:
	inline decoding::Environment& GetEnv() const { return *m_env; }

	inline ProjectTraceData* GetTrace() const { return m_traceData; }

	inline ProjectAdddrsesHistory& GetAddressHistory() { return m_addressHistory; }
	inline const ProjectAdddrsesHistory& GetAddressHistory() const { return m_addressHistory; }

	inline ProjectConfig& GetConfig() { return m_config; }
	inline const ProjectConfig& GetConfig() const { return m_config; }

	//---

	virtual ~Project();

	// build histogram data
	void BuildHistogram();

	// clear cached data (decoded instructions, blocks, etc)
	void ClearCachedData();

	// save project file
	bool Save( ILogOutput& log );

	// load trace data
	bool LoadTrace( ILogOutput& log, const std::wstring& traceFilePath = std::wstring() );

	// do we have breakpoint at given address ?
	bool HasBreakpoint( const uint32 codeAddress ) const;

	// toggle breakpoint at address
	bool SetBreakpoint( const uint32 codeAddress, const bool flag );

	// clear all brekapoints in code
	bool ClearAllBreakpoints();

	// get histogram value for given memory address
	const uint32 GetHistogramValue(const uint32 memoryAddrses) const;

	//! Is the project modified ?
	bool IsModified() const;

	// build memory map
	void BuildMemoryMap();

	//! Decode instruction in manually selected range of memory 
	bool DecodeInstructionsFromMemory( ILogOutput& log, const uint32 startAddress, const uint32 endAddress );

	//---

	// create new project from executable
	static Project* LoadImageFile( ILogOutput& log, const std::wstring& imagePath, const std::wstring& projectPath );

	// load existing project
	static Project* LoadProject( ILogOutput& log, const std::wstring& projectPath );

private:
	Project();

	// address history
	ProjectAdddrsesHistory		m_addressHistory;

	// configuration
	ProjectConfig				m_config;

	// decoding environment (backend root object)
	decoding::Environment*		m_env;

	// local modification flag
	bool						m_isModified;

	// current trace
	ProjectTraceData*			m_traceData;

	// data access histogram
	typedef std::vector< uint32 > TAccessHistogram;
	uint32						m_histogramBase;
	TAccessHistogram			m_histogram;

};

