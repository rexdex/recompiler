#include "build.h"
#include "projectConfiguration.h"
#include "utils.h"

ProjectConfig::ProjectConfig()
	: m_trace( false )
	, m_showLog( true )
	, m_verboseLog( false )
{	
}

void ProjectConfig::Load()
{
	// TODO
}

void ProjectConfig::Save()
{
	// TODO
}

void ProjectConfig::Reset( const std::wstring& projectFilePath )
{
	m_trace = false;
	m_showLog = true;
	m_verboseLog = false;

	std::wstring baseDir;
	ExtractDirW( projectFilePath.c_str(), baseDir );

	m_devkitDir = baseDir + L"devkit";
	m_dvdDir = baseDir + L"dvd";
}
