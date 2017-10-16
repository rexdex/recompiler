#pragma once

namespace tools
{

	class FileFormat
	{
	public:
		FileFormat()
		{}

		FileFormat(const wxString& name, const wxString& ext)
			: m_name(name)
			, m_extension(ext)
		{}

		inline const wxString& GetName() const { return m_name; }
		inline const wxString& GetExtension() const { return m_extension; }

	private:
		wxString		m_name;
		wxString		m_extension;
	};

	class FileDialog
	{
	public:
		typedef std::vector< wxString >		TFiles;
		inline const TFiles& GetFiles() const { return m_files; }
		inline const wxString GetFile() const { return m_files.empty() ? wxEmptyString : m_files[0]; }

		//---

		FileDialog(const wxString& configTag);
		~FileDialog();

		void ClearFormats();
		void AddFormat(const wxString& extension, const wxString& description);
		void AddFormat(const FileFormat& format);

		void SetMultiselection(bool multiselection);
		void SetDirectory(const wxString& directory);

		bool DoOpen(wxWindow* owner, const wxString& defaultFileName = wxEmptyString);
		bool DoSave(wxWindow* owner, const wxString& defaultFileName = wxEmptyString);

	private:
		void UpdateFilters(bool all);

		typedef std::vector< FileFormat >	TFormats;

		OPENFILENAMEW				m_settings;
		wxString					m_extension;
		wxString					m_configTag;

		TFormats					m_formats;
		TFiles						m_files;
	};

} // tools