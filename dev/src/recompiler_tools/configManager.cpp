#include "build.h"
#include "config.h"
#include "configManager.h"

namespace tools
{

	ConfigurationManager::ConfigurationManager()
		: m_sessionConfig(wxT("UniversalCompiler"), wxT("RexDex"), wxT("UniversalCompiler.ini"), wxEmptyString, wxCONFIG_USE_LOCAL_FILE)
	{}

	ConfigurationManager::~ConfigurationManager()
	{}

	ConfigurationManager& ConfigurationManager::GetInstance()
	{
		static ConfigurationManager theInstance;
		return theInstance;
	}

	void ConfigurationManager::SaveConfig()
	{
		std::lock_guard<std::mutex> lock(m_configEntriesLock);

		// Dump all configuration entries
		for (auto* entry : m_configEntries)
			entry->OnSaveConfiguration();

		// Flush the changes to the file
		m_sessionConfig.Flush();
	}

	void ConfigurationManager::LoadConfig()
	{
		std::lock_guard<std::mutex> lock(m_configEntriesLock);

		// Load all configuration entries
		for (auto* entry : m_configEntries)
			entry->OnLoadConfiguration();
	}

	void ConfigurationManager::RegisterConfigurationEntry(IConfiguration* entry)
	{
		std::lock_guard<std::mutex> lock(m_configEntriesLock);
		m_configEntries.push_back(entry);
	}

	void ConfigurationManager::UnregisterConfigurationEntry(IConfiguration* entry)
	{
		std::lock_guard<std::mutex> lock(m_configEntriesLock);
		std::remove(m_configEntries.begin(), m_configEntries.end(), entry);
	}

} // tools