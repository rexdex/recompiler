#pragma once

namespace platform
{
	class TaskIOReader;

	/// Environment for running external stuff
	class ExternalAppEnvironment
	{
	public:
		ExternalAppEnvironment( class ILogOutput& log, const char* prefix, const bool stripErrors );
		~ExternalAppEnvironment();

		/// Set execution directory
		void SetDirectory( const std::wstring& dir );

		/// Execute command, returns error code
		int Execute( const std::wstring& command, const std::wstring& args );

	private:
		class ILogOutput*		m_log;
		class TaskIOReader*		m_output;
		const char*				m_prefix;
	};

} // platform