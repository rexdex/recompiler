#pragma once

namespace launcher
{
	// commandline wrapper
	class LAUNCHER_API Commandline
	{
	public:
		Commandline();
		Commandline(const char** argv, const int argc);
		Commandline(const wchar_t** argv, const int argc);
		Commandline(const wchar_t* cmd);

		/// do we have an option specified ?
		const bool HasOption(const char* name) const;

		/// get first value for given option
		const std::wstring GetOptionValueW(const char* name) const;

		/// get first value for given option
		const std::string GetOptionValueA(const char* name) const;

	private:
		struct Option
		{
			std::string m_name;
			std::vector<std::wstring> m_values;
		};

		typedef std::vector< Option > TOptions;
		TOptions	m_options;		

		void AddOption(const std::string& name, const std::wstring& value);
		void Parse(const wchar_t* cmdLine);
	};

} // launcher