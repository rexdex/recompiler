#pragma once

class CFileFormat
{
private:
	wxString		m_name;
	wxString		m_extension;

public:
	CFileFormat()
	{}

	CFileFormat( const wxString& name, const wxString& ext )
		: m_name( name )
		, m_extension( ext )
	{}

	inline const wxString& GetName() const { return m_name; }
	inline const wxString& GetExtension() const { return m_extension; }
};

class CFileDialog
{
private:
	typedef std::vector< wxString >		TFiles;
	typedef std::vector< CFileFormat >	TFormats;

	OPENFILENAMEW				m_settings;
	wxString					m_extension;
	wxString					m_configTag;

	TFormats					m_formats;
	TFiles						m_files;

public:
	inline const TFiles& GetFiles() const { return m_files; }
	inline const wxString GetFile() const { return m_files.empty() ? wxEmptyString : m_files[0]; }

public:
	CFileDialog( const wxString& configTag );
	~CFileDialog();

	void ClearFormats();
	void AddFormat( const wxString& extension, const wxString& description );
	void AddFormat( const CFileFormat& format );

	void SetMultiselection( bool multiselection );
	void SetDirectory( const wxString& directory );

	bool DoOpen( wxWindow* owner, const wxString& defaultFileName = wxEmptyString );
	bool DoSave( wxWindow* owner, const wxString& defaultFileName = wxEmptyString );

private:
	void UpdateFilters( bool all );
};
