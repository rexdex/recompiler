#pragma once

/// generic configuration for project
class ProjectConfig
{
public:
	ProjectConfig();

	void Reset( const std::wstring& projectFilePath );

	void Load();
	void Save();

public:
	bool				m_trace;
	bool				m_showLog;
	bool				m_verboseLog;

	std::wstring		m_configToRun;

	std::wstring		m_devkitDir;
	std::wstring		m_dvdDir;
};