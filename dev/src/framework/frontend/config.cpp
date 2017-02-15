#include "build.h"

//---------------------------------------------------------------------------

IConfiguration::IConfiguration()
{
	if ( wxTheApp != nullptr )
	{
		wxTheApp->GetConfig().RegisterConfigurationEntry( this );
	}
}

IConfiguration::~IConfiguration()
{
	if ( wxTheApp != nullptr )
	{
		wxTheApp->GetConfig().UnregisterConfigurationEntry( this );
	}
}

//---------------------------------------------------------------------------

CConfigurationManager::CConfigurationManager()
	: m_sessionConfig( wxT("ProTools"), wxT("DexTech"), wxT("ProTools.ini"), wxEmptyString, wxCONFIG_USE_LOCAL_FILE )
{
}

CConfigurationManager::~CConfigurationManager()
{
}

void CConfigurationManager::SaveConfig()
{
	// Dump all configuration entries
	for ( DWORD i=0; i<m_configEntries.size(); i++ )
	{
		IConfiguration* entry = m_configEntries[i];
		entry->OnSaveConfiguration();
	}

	// Flush the changes to the file
	m_sessionConfig.Flush();
}

void CConfigurationManager::LoadConfig()
{
	// Dump all configuration entries
	for ( DWORD i=0; i<m_configEntries.size(); i++ )
	{
		IConfiguration* entry = m_configEntries[i];
		entry->OnLoadConfiguration();
	}

	// Flush the changes to the file
	m_sessionConfig.Flush();
}

void CConfigurationManager::RegisterConfigurationEntry( IConfiguration* entry )
{
	m_configEntries.push_back( entry );
}

void CConfigurationManager::UnregisterConfigurationEntry( IConfiguration* entry )
{
	m_configEntries.erase( std::find( m_configEntries.begin(), m_configEntries.end(), entry ) );
}

//---------------------------------------------------------------------------

CConfigSection::CConfigSection( const wxChar* sectionName, ... )
{
	// Format section name
	wxChar formatedName[ 512 ];
	va_list args;
	va_start( args, sectionName );
	vswprintf_s( formatedName, ARRAYSIZE(formatedName), sectionName, args );
	va_end( args );

	// Format section path
	m_sectionName = wxT("/");
	m_sectionName += formatedName;
	m_sectionName += wxT("/");

	// Get the config file
	m_config = wxTheApp->GetConfig().GetConfig();
}

CConfigSection::CConfigSection( const CConfigSection& base, const wxChar* sectionName, ... )
{
	// Format section name
	wxChar formatedName[ 512 ];
	va_list args;
	va_start( args, sectionName );
	vswprintf_s( formatedName, ARRAYSIZE(formatedName), sectionName, args );
	va_end( args );

	// Format section path
	m_sectionName = base.m_sectionName;
	m_sectionName += formatedName;
	m_sectionName += wxT("/");

	// Get the config file
	m_config = base.m_config;
}

CConfigSection::~CConfigSection()
{
	// Flush changes
	m_config->Flush();
}

bool CConfigSection::ReadString( const wxChar* key, wxString* outValue ) const
{
	const wxString fullKeyName = m_sectionName + key;
	return m_config->Read( fullKeyName, outValue );
}

void CConfigSection::ReadStringArray( const wxChar* key, wxArrayString* outValue ) const
{
	// Load items, max 1000
	for ( uint32 i=0; i<1000; ++i )
	{
		// Format array key name
		wxString fullKeyName = m_sectionName;
		fullKeyName += key;
		fullKeyName += wxString::Format( wxT("/Elem%i"), i );

		// If valid element was read add it to table
		wxString outString;
		if ( m_config->Read( fullKeyName, &outString ) )
		{
			outValue->push_back( outString );
		}
		else
		{
			// End of list
			break;
		}
	}
}

bool CConfigSection::ReadInteger( const wxChar* key, int* outValue ) const
{
	const wxString fullKeyName = m_sectionName + key;
	return m_config->Read( fullKeyName, outValue );
}

bool CConfigSection::ReadFloat( const wxChar* key, float* outValue ) const
{
	const wxString fullKeyName = m_sectionName + key;
	return m_config->Read( fullKeyName, outValue );
}

bool CConfigSection::ReadBool( const wxChar* key, bool* outValue ) const
{
	const wxString fullKeyName = m_sectionName + key;
	return m_config->Read( fullKeyName, outValue );
}

wxString CConfigSection::ReadString( const wxChar* key, const wxString& defaultValue ) const
{
	wxString outValue = defaultValue;
	const wxString fullKeyName = m_sectionName + key;
	m_config->Read( fullKeyName, &outValue );
	return outValue;
}

int CConfigSection::ReadInteger( const wxChar* key, int defaultValue ) const
{
	int outValue = defaultValue;
	const wxString fullKeyName = m_sectionName + key;
	m_config->Read( fullKeyName, &outValue );
	return outValue;
}

float CConfigSection::ReadFloat( const wxChar* key, float defaultValue ) const
{
	float outValue = defaultValue;
	const wxString fullKeyName = m_sectionName + key;
	m_config->Read( fullKeyName, &outValue );
	return outValue;
}

bool CConfigSection::ReadBool( const wxChar* key, bool defaultValue ) const
{
	bool outValue = defaultValue;
	const wxString fullKeyName = m_sectionName + key;
	m_config->Read( fullKeyName, &outValue );
	return outValue;
}

void CConfigSection::WriteString( const wxChar* key, const wxString& val )
{
	const wxString fullKeyName = m_sectionName + key;
	m_config->Write( fullKeyName, val );
}

void CConfigSection::WriteStringArray( const wxChar* key, const wxArrayString& val )
{
	// Delete group
	const wxString groupName = m_sectionName + key;
	m_config->DeleteGroup( groupName );

	// Save items
	for ( uint32 i=0; i<val.size(); ++i )
	{
		// Format array key name
		wxString fullKeyName = m_sectionName;
		fullKeyName += key;
		fullKeyName += wxString::Format( wxT("/Elem%i"), i );

		// Write element
		m_config->Write( fullKeyName, val[i] );
	}
}

void CConfigSection::WriteInteger( const wxChar* key, int val )
{
	const wxString fullKeyName = m_sectionName + key;
	m_config->Write( fullKeyName, val );
}

void CConfigSection::WriteFloat( const wxChar* key, float val )
{
	const wxString fullKeyName = m_sectionName + key;
	m_config->Write( fullKeyName, val );
}

void CConfigSection::WriteBool( const wxChar* key, bool val )
{
	const wxString fullKeyName = m_sectionName + key;
	m_config->Write( fullKeyName, val );
}

void CConfigSection::RestoreControl( wxControl* control, const wxChar* nameOverride /*= nullptr*/ ) const
{
	const wxString key = (nameOverride) ? nameOverride : control->GetName();
	bool bValueSet = false;

	// begin update
	control->Freeze();

	// load values
	if (control->GetClassInfo()->IsKindOf(&wxComboBox::ms_classInfo))
	{
		wxComboBox* comboBox = static_cast<wxComboBox*>(control);
		
		// Load items
		int selected = -1;
		wxArrayString items;

		for ( uint32 i=0; ; ++i )
		{
			// Format array key name
			wxString fullKeyName = m_sectionName;
			fullKeyName += key;
			fullKeyName += wxString::Format( wxT("/Elem%i"), i );

			// Write element
			wxString itemValue;
			if (m_config->Read(fullKeyName, &itemValue))
			{
				items.push_back(itemValue);
			}
			else
			{
				break;
			}
		}

		// Selected
		{
			wxString fullKeyName = m_sectionName;
			fullKeyName += key;
			fullKeyName += wxT("/Selected");
			m_config->Read(fullKeyName, &selected);
		}

		// update
		comboBox->Clear();
		comboBox->Append(items);
		comboBox->SetSelection(selected);
		bValueSet = true;
	}
	else if (control->GetClassInfo()->IsKindOf(&wxTextCtrl::ms_classInfo))
	{
		wxTextCtrl* textCtrl = static_cast<wxTextCtrl*>(control);
		wxString newValue;
		if (m_config->Read(key, &newValue))
		{
			textCtrl->SetValue(newValue);
			bValueSet = true;
		}
	}
	else if (control->GetClassInfo()->IsKindOf(&wxCheckBox::ms_classInfo))
	{
		wxCheckBox* checkBox = static_cast<wxCheckBox*>(control);
		bool newValue = false;
		if (m_config->Read(key, &newValue))
		{
			checkBox->SetValue(newValue);
			bValueSet = true;
		}
	}

	// end update
	control->Thaw();

	// refresh only when realy updated
	if (bValueSet)
	{
		control->Refresh();
	}
}

void CConfigSection::SaveControl( const wxControl* control, const wxChar* nameOverride /*= nullptr*/ )
{
	const wxString key = (nameOverride) ? nameOverride : control->GetName();

	if (control->GetClassInfo()->IsKindOf(&wxComboBox::ms_classInfo))
	{
		const wxComboBox* comboBox = static_cast<const wxComboBox*>(control);

		// Delete group
		const wxString groupName = m_sectionName + key;
		m_config->DeleteGroup( groupName );

		// Save items
		const uint32 numElems = comboBox->GetCount();
		for ( uint32 i=0; i<numElems; ++i )
		{
			// Format array key name
			wxString fullKeyName = m_sectionName;
			fullKeyName += key;
			fullKeyName += wxString::Format( wxT("/Elem%i"), i );

			// Write element
			const wxString val = comboBox->GetString(i);
			m_config->Write( fullKeyName, val );
		}

		// Selected
		{
		
			wxString fullKeyName = m_sectionName;
			fullKeyName += key;
			fullKeyName += wxT("/Selected");

			const int selectedIndex = comboBox->GetSelection();
			m_config->Write( fullKeyName, selectedIndex );
		}
	}
	else if (control->GetClassInfo()->IsKindOf(&wxTextCtrl::ms_classInfo))
	{
		const wxTextCtrl* textCtrl = static_cast<const wxTextCtrl*>(control);
		m_config->Write( key, textCtrl->GetValue() );
	}
	else if (control->GetClassInfo()->IsKindOf(&wxCheckBox::ms_classInfo))
	{
		const wxCheckBox* checkBox = static_cast<const wxCheckBox*>(control);
		m_config->Write( key, checkBox->GetValue() );
	}
}

//---------------------------------------------------------------------------

CAutoConfig::CAutoConfig( const char* varSectionName )
	: m_varSection( L"Vars/%s", varSectionName )
{
}

CAutoConfig::~CAutoConfig()
{
}

void CAutoConfig::Load()
{
	OnLoadConfiguration();
}

void CAutoConfig::Save()
{
	OnSaveConfiguration();
}

void CAutoConfig::OnLoadConfiguration()
{
	for ( uint32 i=0; i<m_vars.size(); ++i )
	{
		const Var& v = m_vars[i];

		if ( v.m_type == eConfigVarType_Int )
		{
			m_varSection.ReadInteger( v.m_name.wc_str(), (int*)v.m_ptr );
		}
		else if ( v.m_type == eConfigVarType_Float )
		{
			m_varSection.ReadFloat( v.m_name.wc_str(), (float*)v.m_ptr );
		}
		else if ( v.m_type == eConfigVarType_Bool )
		{
			m_varSection.ReadBool( v.m_name.wc_str(), (bool*)v.m_ptr );
		}
		else if ( v.m_type == eConfigVarType_String )
		{
			m_varSection.ReadString( v.m_name.wc_str(), (wxString*)v.m_ptr );
		}
		else if ( v.m_type == eConfigVarType_StringArray )
		{
			m_varSection.ReadStringArray( v.m_name.wc_str(), (wxArrayString*)v.m_ptr );
		}
	}
}

void CAutoConfig::OnSaveConfiguration()
{
	for ( uint32 i=0; i<m_vars.size(); ++i )
	{
		const Var& v = m_vars[i];

		if ( v.m_type == eConfigVarType_Int )
		{
			m_varSection.ReadInteger( v.m_name.wc_str(), (int*)v.m_ptr );
		}
		else if ( v.m_type == eConfigVarType_Float )
		{
			m_varSection.ReadFloat( v.m_name.wc_str(), (float*)v.m_ptr );
		}
		else if ( v.m_type == eConfigVarType_Bool )
		{
			m_varSection.ReadBool( v.m_name.wc_str(), (bool*)v.m_ptr );
		}
		else if ( v.m_type == eConfigVarType_String )
		{
			m_varSection.ReadString( v.m_name.wc_str(), (wxString*)v.m_ptr );
		}
		else if ( v.m_type == eConfigVarType_StringArray )
		{
			m_varSection.ReadStringArray( v.m_name.wc_str(), (wxArrayString*)v.m_ptr );
		}
	}
}

void CAutoConfig::AddConfigValueRaw( void* ptr, const char* name, EConfigVarType type )
{
	for ( uint32 i=0; i<m_vars.size(); ++i )
	{
		if ( m_vars[i].m_name == name )
		{
			return;
		}
	}

	Var info;
	info.m_name = name;
	info.m_type = type;
	info.m_ptr = ptr;
	m_vars.push_back( info );
}

void CAutoConfig::AddConfigValue( int& var, const char* name )
{
	AddConfigValueRaw( &var, name, eConfigVarType_Int );
}

void CAutoConfig::AddConfigValue( float& var, const char* name )
{
	AddConfigValueRaw( &var, name, eConfigVarType_Float );
}

void CAutoConfig::AddConfigValue( wxString& var, const char* name )
{
	AddConfigValueRaw( &var, name, eConfigVarType_String );
}

void CAutoConfig::AddConfigValue( wxArrayString& var, const char* name )
{
	AddConfigValueRaw( &var, name, eConfigVarType_StringArray );
}

void CAutoConfig::AddConfigValue( bool& var, const char* name )
{
	AddConfigValueRaw( &var, name, eConfigVarType_Bool );
}

//---------------------------------------------------------------------------
