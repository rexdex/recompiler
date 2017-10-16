#include "build.h"
#include "fileDialog.h"

namespace tools
{

	FileDialog::FileDialog(const wxString& configTag)
		: m_configTag(configTag)
	{
		// Setup dialog parameters
		memset(&m_settings, 0, sizeof(m_settings));
		m_settings.lStructSize = sizeof(m_settings);
		m_settings.lpstrFile = new wchar_t[_MAX_FNAME];
		m_settings.nMaxFile = _MAX_FNAME;
		m_settings.lpstrInitialDir = new wchar_t[_MAX_FNAME];
		m_settings.lpstrFilter = new wchar_t[4096];
		m_settings.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_ENABLESIZING | OFN_NONETWORKBUTTON | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

		// Load directory from config file
		if (!m_configTag.empty())
		{
			ConfigSection config( wxT("FileDialog/%ls"), m_configTag.c_str() );

			// Read directory name
			const wxString dir = config.ReadString( wxT("Directory"), wxEmptyString );
			if (!dir.empty())
				SetDirectory(dir);
		}
	}

	FileDialog::~FileDialog()
	{
		delete[] m_settings.lpstrFile;
		delete[] m_settings.lpstrInitialDir;
		delete[] m_settings.lpstrFilter;
	}

	void FileDialog::ClearFormats()
	{
		m_formats.clear();
	}

	void FileDialog::AddFormat(const wxString& extension, const wxString& description)
	{
		// Make sure format is not already defined
		for (size_t i = 0; i < m_formats.size(); ++i)
		{
			const FileFormat& desc = m_formats[i];
			if (desc.GetExtension().CmpNoCase(extension) == 0)
			{
				return;
			}
		}

		// Add new format
		m_formats.push_back(FileFormat(extension, description));
	}

	void FileDialog::AddFormat(const FileFormat& format)
	{
		// Make sure format is not already defined
		for (size_t i = 0; i < m_formats.size(); ++i)
		{
			const FileFormat& desc = m_formats[i];
			if (desc.GetExtension().CmpNoCase(format.GetExtension()) == 0)
			{
				return;
			}
		}

		// Add new format
		m_formats.push_back(format);
	}

	void FileDialog::SetMultiselection(bool multiselection)
	{
		if (multiselection)
		{
			m_settings.Flags |= OFN_ALLOWMULTISELECT;
		}
		else
		{
			m_settings.Flags &= ~OFN_ALLOWMULTISELECT;
		}
	}

	void FileDialog::SetDirectory(const wxString& directory)
	{
		wcscpy_s((wchar_t*)m_settings.lpstrInitialDir, _MAX_PATH, directory.wc_str());
	}

	bool FileDialog::DoOpen(wxWindow* owner, const wxString& defaultFileName /*= wxEmptyString*/)
	{
		// Remember current directory
		wchar_t tempDir[_MAX_PATH];
		GetCurrentDirectoryW(_MAX_PATH, tempDir);

		// Update settings
		UpdateFilters(true);

		// Reset file list
		m_files.clear();
		wcscpy_s((wchar_t*)m_settings.lpstrFile, _MAX_FNAME, defaultFileName.empty() ? wxT("") : defaultFileName.wc_str());

		// Prepare dialog
		m_settings.hwndOwner = owner ? (HWND)owner->GetHWND() : NULL;
		m_settings.lpstrTitle = (m_settings.Flags & OFN_ALLOWMULTISELECT) ? TEXT("Open files") : TEXT("Open file");

		// Show dialog
		if (!GetOpenFileNameW(&m_settings))
		{
			// Canceled by user
			return false;
		}

		// Restore original directory
		GetCurrentDirectoryW(_MAX_PATH, (wchar_t*)m_settings.lpstrInitialDir);
		SetCurrentDirectoryW(tempDir);

		// Parse file names
		if (m_settings.Flags & OFN_ALLOWMULTISELECT)
		{
			wchar_t* pos = m_settings.lpstrFile;
			wxString newFile;

			// Parse file names
			for (;;)
			{
				if (!*pos)
				{
					if (newFile.length())
					{
						m_files.push_back(newFile);
						newFile = wxEmptyString;
						pos++;
					}
					else
					{
						break;
					}

					continue;
				}

				wchar_t str[2] = { *pos, 0 };
				newFile += str;
				pos++;
			}

			// More than one file then we must deal with the path given in the first entry
			if (m_files.size() > 1)
			{
				// Item 0 is a base path
				wxString mainPath = m_files[0];
				m_files.erase(m_files.begin());

				// Append file names with path
				for (size_t i = 0; i < m_files.size(); i++)
				{
					wxString fileName = m_files[i];
					m_files[i] = mainPath;
					m_files[i] += TEXT("\\");
					m_files[i] += fileName;
				}
			}
		}
		else
		{
			// Single file only
			m_files.push_back(m_settings.lpstrFile);
		}

		// Get extension index
		int index = m_settings.nFilterIndex - 1;
		if (m_formats.size() > 1)
		{
			// Account for "all formats"
			index--;
		}

		// Update selected extension
		if (index >= 0 && index < (int)m_formats.size())
		{
			m_extension = m_formats[index].GetExtension();
		}
		else
		{
			m_extension = wxEmptyString;
		}

		// Save in the config
		if (!m_configTag.empty() && m_files.size())
		{
			/*// Strip path from file name
			FFilePath fileName( m_files[0] );

			// Save path
			{
				CEdConfigSection config( wxT("FileDialog/%ls"), m_configTag.c_str() );
				config.writeString( wxT("Directory"), fileName.toPath() );
			}*/
		}

		// Done
		return true;
	}

	bool FileDialog::DoSave(wxWindow* owner, const wxString& defaultFileName /*= wxEmptyString*/)
	{
		// Remember current directory
		wchar_t tempDir[_MAX_PATH];
		GetCurrentDirectoryW(_MAX_PATH, tempDir);

		// Update settings
		UpdateFilters(false);

		// Reset files
		m_files.clear();

		// Prepare dialog
		m_settings.hwndOwner = owner ? (HWND)owner->GetHWND() : NULL;
		m_settings.lpstrTitle = TEXT("Save file");

		// Copy initial file name
		if (!defaultFileName.empty())
		{
			// File name
			wcscpy_s(m_settings.lpstrFile, _MAX_FNAME, defaultFileName.wc_str());

			// Get default extension
			if (m_extension.empty())
			{
				m_extension = m_formats[0].GetExtension();
			}

			// Append extension to file name
			if (!m_extension.empty())
			{
				wcscat_s(m_settings.lpstrFile, _MAX_FNAME, L".");
				wcscat_s(m_settings.lpstrFile, _MAX_FNAME, m_extension.wc_str());
			}
		}
		else
		{
			// No default file name given
			wcscpy_s(m_settings.lpstrFile, _MAX_FNAME, L"");
		}

		// Show dialog
		if (!GetSaveFileNameW(&m_settings))
		{
			// Canceled by user
			return false;
		}

		// Get extension
		int index = m_settings.nFilterIndex - 1;

		// Update
		if (index >= 0 && index < (int)m_formats.size())
		{
			m_extension = m_formats[index].GetExtension();
		}
		else
		{
			// Custom extension given
			m_extension = wxEmptyString;
		}

		// Get file
		wxString file(m_settings.lpstrFile);

		// Append extension if not given
		if (file.Find(L".") == -1)
		{
			file += TEXT(".");
			file += m_extension;
		}

		// Add file to list  
		m_files.push_back(file);

		// Restore directory
		GetCurrentDirectoryW(_MAX_PATH, (wchar_t*)m_settings.lpstrInitialDir);
		SetCurrentDirectoryW(tempDir);

		// Save in the config
		if (!m_configTag.empty() && m_files.size())
		{
			/*// Strip path from file name
			FFilePath fileName( m_files[0] );

			// Save path
			{
				CEdConfigSection config( wxT("FileDialog/%ls"), m_configTag.c_str() );
				config.writeString( wxT("Directory"), fileName.toPath() );
			}*/
		}

		// Done
		return true;
	}

	static void AppendStr(std::vector< wchar_t >& outString, const wchar_t* str)
	{
		const size_t length = wcslen(str);
		for (size_t i = 0; i < length; ++i)
		{
			outString.push_back(str[i]);
		}
	}

	void FileDialog::UpdateFilters(bool all)
	{
		bool addedMultiFilter = false;
		std::vector< wchar_t > filterString;

		if (!m_formats.size())
		{
			// No filters
			AppendStr(filterString, L"All files [*.*]");
			filterString.push_back(0);
			AppendStr(filterString, L"*.*");
			filterString.push_back(0);
			filterString.push_back(0);
		}
		else if (m_formats.size() == 1)
		{
			// One filter
			AppendStr(filterString, m_formats[0].GetName().c_str());
			AppendStr(filterString, L" [*.");
			AppendStr(filterString, m_formats[0].GetExtension().c_str());
			AppendStr(filterString, L"]");
			filterString.push_back(0);
			AppendStr(filterString, L"*.");
			AppendStr(filterString, m_formats[0].GetExtension().c_str());
			filterString.push_back(0);
			filterString.push_back(0);
		}
		else
		{
			// Many filters
			if (all)
			{
				addedMultiFilter = true;
				AppendStr(filterString, L"All supported formats");
				AppendStr(filterString, L" [");

				for (DWORD i = 0; i < m_formats.size(); i++)
				{
					if (i != 0)
					{
						AppendStr(filterString, L";");
					}
					AppendStr(filterString, L"*.");
					AppendStr(filterString, m_formats[i].GetExtension().c_str());
				}

				AppendStr(filterString, L"]");
				filterString.push_back(0);

				for (DWORD i = 0; i < m_formats.size(); i++)
				{
					if (i != 0)
					{
						AppendStr(filterString, L";");
					}

					AppendStr(filterString, L"*.");
					AppendStr(filterString, m_formats[i].GetExtension().c_str());
				}

				filterString.push_back(0);
			}

			for (DWORD i = 0; i < m_formats.size(); i++)
			{
				AppendStr(filterString, m_formats[i].GetName().c_str());
				AppendStr(filterString, L" [");
				AppendStr(filterString, L"*.");
				AppendStr(filterString, m_formats[i].GetExtension().c_str());
				AppendStr(filterString, L"]");
				filterString.push_back(0);
				AppendStr(filterString, L"*.");
				AppendStr(filterString, m_formats[i].GetExtension().c_str());
				filterString.push_back(0);
			}

			filterString.push_back(0);
		}

		// Update filter index
		int find = -1;

		// Find filter
		for (size_t i = 0; i < m_formats.size(); i++)
		{
			const FileFormat& format = m_formats[i];
			if (format.GetExtension() == m_extension)
			{
				find = i;
				break;
			}
		}

		// Set filter index  
		if (find == -1)
		{
			m_settings.nFilterIndex = 1;
		}
		else
		{
			if (addedMultiFilter)
			{
				m_settings.nFilterIndex = 2 + find;
			}
			else
			{
				m_settings.nFilterIndex = 1 + find;
			}
		}

		// Copy buffer
		memcpy((wchar_t*)m_settings.lpstrFilter, &filterString[0], filterString.size() * sizeof(wchar_t));
	}

}// tools