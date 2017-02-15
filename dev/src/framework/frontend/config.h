#pragma once

//---------------------------------------------------------------------------

/// Configuration element
class IConfiguration
{
public:
	IConfiguration();
	virtual ~IConfiguration();

	//! Save/Load configuration
	virtual void OnLoadConfiguration()=0;
	virtual void OnSaveConfiguration()=0;
};

/// Editor configuration manager
class CConfigurationManager
{
private:
	std::vector< IConfiguration* >		m_configEntries;
	wxFileConfig						m_sessionConfig;

public:
	//! Get config section
	inline wxConfigBase* GetConfig() { return &m_sessionConfig; }

public:
	CConfigurationManager();
	~CConfigurationManager();

	//! Flush and save editor configuration file
	void SaveConfig();

	//! Load configuration from config file
	void LoadConfig();

	//! Register configuration entry
	void RegisterConfigurationEntry( IConfiguration* entry );

	//! Unregister configuration entry
	void UnregisterConfigurationEntry( IConfiguration* entry );
};

//---------------------------------------------------------------------------

/// Section in configuration editor
class CConfigSection
{
private:
	wxConfigBase*	m_config;		//!< The config file
	wxString		m_sectionName;	//!< Config section name

public:
	//! Get the section name
	inline const wxString& getSectionName() const { return m_sectionName; }

public:
	explicit CConfigSection( const wxChar* sectionName, ... );
	explicit CConfigSection( const CConfigSection& base, const wxChar* sectionName, ... );
	~CConfigSection();

public:
	//! Read string value
	bool ReadString( const wxChar* key, wxString* outValue ) const;

	//! Read string array
	void ReadStringArray( const wxChar* key, wxArrayString* outValue ) const;

	//! Read integer value
	bool ReadInteger( const wxChar* key, int* outValue ) const;

	//! Read float value
	bool ReadFloat( const wxChar* key, float* outValue ) const;

	//! Read boolean value
	bool ReadBool( const wxChar* key, bool* outValue ) const;

public:
	//! Read string value
	wxString ReadString( const wxChar* key, const wxString& defaultValue ) const;

	//! Read integer value
	int ReadInteger( const wxChar* key, int defaultValue ) const;

	//! Read float value
	float ReadFloat( const wxChar* key, float defaultValue ) const;

	//! Read boolean value
	bool ReadBool( const wxChar* key, bool defaultValue ) const;

public:
	//! Write string value
	void WriteString( const wxChar* key, const wxString& val );

	//! Write array of strings
	void WriteStringArray( const wxChar* key, const wxArrayString& val );

	//! Write integer value
	void WriteInteger( const wxChar* key, int val );

	//! Write float value
	void WriteFloat( const wxChar* key, float val );

	//! Write boolean value
	void WriteBool( const wxChar* key, bool val );

public:
	//! Restore setting of given control from config
	void RestoreControl( wxControl* control, const wxChar* nameOverride = nullptr ) const;

	//! Save settings of given control to config
	void SaveControl( const wxControl* control, const wxChar* nameOverride = nullptr );
};

//---------------------------------------------------------------------------

// Configuration class that saves and loads itself from the config
class CAutoConfig : public IConfiguration
{
protected:
	enum EConfigVarType
	{
		eConfigVarType_None,
		eConfigVarType_Int,
		eConfigVarType_Float,
		eConfigVarType_Bool,
		eConfigVarType_String,
		eConfigVarType_StringArray,
	};

	struct Var
	{
		EConfigVarType	m_type;
		wxString		m_name;
		void*			m_ptr;
	};

	typedef std::vector< Var >	TConfigVars;
	TConfigVars					m_vars;

	CConfigSection				m_varSection;

public:
	CAutoConfig( const char* varSectionName );
	~CAutoConfig();

	// Refresh the values from config
	void Load();

	// Save the configuration
	void Save();

protected:
	//! Save/Load configuration
	virtual void OnLoadConfiguration();
	virtual void OnSaveConfiguration();

protected:
	//! Register configuration value
	void AddConfigValueRaw( void* ptr, const char* name, EConfigVarType type );

	// Templated version for AddConfigValueRaw
	void AddConfigValue( int& var, const char* name );
	void AddConfigValue( float& var, const char* name );
	void AddConfigValue( wxString& var, const char* name );
	void AddConfigValue( wxArrayString& var, const char* name );
	void AddConfigValue( bool& var, const char* name );
};

//---------------------------------------------------------------------------
