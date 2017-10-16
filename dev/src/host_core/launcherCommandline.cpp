#include "build.h"
#include "launcherCommandline.h"
#include "launcherUtils.h"

namespace launcher
{

	Commandline::Commandline()
	{
	}

	Commandline::Commandline(const wchar_t* cmd)
	{
		Parse(cmd);
	}

	Commandline::Commandline(const wchar_t** argv, const int argc)
	{
		std::wstring fullCommandLine;

		for (int i = 1; i < argc; ++i)
		{
			fullCommandLine += argv[i];
			fullCommandLine += L" ";
		}

		Parse(fullCommandLine.c_str());
	}

	Commandline::Commandline(const char** argv, const int argc)
	{
		std::string fullCommandLine;

		for (int i = 1; i < argc; ++i)
		{
			fullCommandLine += argv[i];
			fullCommandLine += " ";
		}

		Parse(AnsiToUnicode(fullCommandLine).c_str());
	}

	void Commandline::Parse(const wchar_t* cmdLine)
	{
		const auto* pos = cmdLine;
		while (*pos)
		{
			const auto ch = *pos++;

			// skip white spaces
			if (ch <= 32)
				continue;

			// extract values
			if (ch == '-')
			{
				// extract the key
				const auto* start = pos;
				while (*pos > 32 && *pos != '=')
					pos += 1;

				// extract into string
				const std::string optionName = UnicodeToAnsi(std::wstring(start, pos - start));

				// option value ?
				if (*pos == '=')
				{
					pos += 1; // skip the "="

					// extract option from quotes
					if (*pos == '\"')
					{
						// skip till past the quotes
						start = pos;
						while (*pos && *pos != '\"')
							pos += 1;

						// get the full value
						const std::wstring optionValue(start, pos - start);
						AddOption(optionName, optionValue);
					}
					else
					{
						// skip till a while space is found
						start = pos;
						while (*pos > 32)
							pos += 1;

						// get the full value
						const std::wstring optionValue(start, pos - start);
						AddOption(optionName, optionValue);
					}
				}
				else
				{
					// option with no value
					AddOption(optionName, std::wstring());
				}
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
			if (option.m_name == name && !option.m_values.empty())
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
				if (!value.empty())
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
