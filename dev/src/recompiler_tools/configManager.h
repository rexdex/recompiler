#pragma once

namespace tools
{

	/// Editor configuration manager
	class ConfigurationManager
	{
	public:
		//! Get config section
		inline wxConfigBase* GetConfig() { return &m_sessionConfig; }

	public:
		//! Flush and save editor configuration file
		void SaveConfig();

		//! Load configuration from config file
		void LoadConfig();

		//! Register configuration entry
		void RegisterConfigurationEntry(IConfiguration* entry);

		//! Unregister configuration entry
		void UnregisterConfigurationEntry(IConfiguration* entry);

		///---

		//! Get the config system instance
		static ConfigurationManager& GetInstance();

	private:
		ConfigurationManager();
		~ConfigurationManager();

		std::mutex m_configEntriesLock;
		std::vector< IConfiguration* > m_configEntries;

		wxFileConfig m_sessionConfig;
	};

} // tools