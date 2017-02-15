#include "build.h"
#include "launcherCommandline.h"
#include "launcherUtils.h"

namespace launcher
{

	Commandline::Commandline()
	{
	}

	Commandline::Commandline(const wchar_t** argv, const int argc)
	{
		for (int i = 1; i < argc; ++i)
		{
			const auto* pos = argv[i];
			if (pos[0] != '-')
				continue;

			// get option name
			pos += 1;
			const auto* start = pos;
			while (*pos && *pos != '=')
				pos += 1;

			// extract into string
			const std::string optionName = UnicodeToAnsi(std::wstring(start, pos - start));

			// option value ?
			if (*pos == '=')
			{
				pos += 1;

				if (*pos == '\"')
				{
					start = pos;
					while (*pos && *pos != '\"')
						pos += 1;

					const std::wstring optionValue(start, pos - start);
					AddOption(optionName, optionValue);
				}
				else
				{
					const std::wstring optionValue(pos);
					AddOption(optionName, optionValue);
				}
			}
			else
			{
				AddOption(optionName, std::wstring());
			}
		}
	}

	const bool Commandline::HasOption(const char* name) const
	{
		for (const auto& option : m_options)
			if (option.m_name == name)
				return true;

		return false;
	}

	const std::wstring Commandline::GetOptionValueW(const char* name) const
	{
		for (const auto& option : m_options)
			if (option.m_name == name)
				return option.m_values[0];

		return std::wstring();
	}

	const std::string Commandline::GetOptionValueA(const char* name) const
	{
		for (const auto& option : m_options)
			if (option.m_name == name)
				return UnicodeToAnsi(option.m_values[0]);

		return std::string();
	}

	void Commandline::AddOption(const std::string& name, const std::wstring& value)
	{
		for (auto& option : m_options)
		{
			if (option.m_name == name)
			{
				if ( !value.empty() )
					option.m_values.push_back(value);

				return;
			}
		}

		Option info;
		info.m_name = name;
		info.m_values.push_back(value);
		m_options.push_back(std::move(info));
	}

} // launcher
