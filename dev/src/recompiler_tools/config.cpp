#include "build.h"
#include "config.h"
#include "configManager.h"

namespace tools
{

	//---------------------------------------------------------------------------

	IConfiguration::IConfiguration()
	{
		ConfigurationManager::GetInstance().RegisterConfigurationEntry(this);
	}

	IConfiguration::~IConfiguration()
	{
		ConfigurationManager::GetInstance().UnregisterConfigurationEntry(this);
	}

	//---------------------------------------------------------------------------

	ConfigSection::ConfigSection(const wxChar* sectionName, ...)
	{
		// Format section name
		wxChar formatedName[512];
		va_list args;
		va_start(args, sectionName);
		vswprintf_s(formatedName, ARRAYSIZE(formatedName), sectionName, args);
		va_end(args);

		// Format section path
		m_sectionName = wxT("/");
		m_sectionName += formatedName;
		m_sectionName += wxT("/");

		// Get the config file
		m_config = ConfigurationManager::GetInstance().GetConfig();
	}

	ConfigSection::ConfigSection(const ConfigSection& base, const wxChar* sectionName, ...)
	{
		// Format section name
		wxChar formatedName[512];
		va_list args;
		va_start(args, sectionName);
		vswprintf_s(formatedName, ARRAYSIZE(formatedName), sectionName, args);
		va_end(args);

		// Format section path
		m_sectionName = base.m_sectionName;
		m_sectionName += formatedName;
		m_sectionName += wxT("/");

		// Get the config file
		m_config = base.m_config;
	}

	ConfigSection::~ConfigSection()
	{
		// Flush changes
		m_config->Flush();
	}

	bool ConfigSection::ReadString(const wxChar* key, wxString* outValue) const
	{
		const wxString fullKeyName = m_sectionName + key;
		return m_config->Read(fullKeyName, outValue);
	}

	bool ConfigSection::ReadString(const wxChar* key, std::string* outValue) const
	{
		const wxString fullKeyName = m_sectionName + key;
		wxString strValue;
		if (!m_config->Read(fullKeyName, &strValue))
			return false;

		*outValue = strValue.c_str().AsChar();
		return true;
	}

	bool ConfigSection::ReadString(const wxChar* key, std::wstring* outValue) const
	{
		const wxString fullKeyName = m_sectionName + key;
		wxString strValue;
		if (!m_config->Read(fullKeyName, &strValue))
			return false;

		*outValue = strValue.wc_str();
		return true;
	}

	void ConfigSection::ReadStringArray(const wxChar* key, wxArrayString* outValue) const
	{
		// Load items, max 1000
		for (uint32 i = 0; i < 1000; ++i)
		{
			// Format array key name
			wxString fullKeyName = m_sectionName;
			fullKeyName += key;
			fullKeyName += wxString::Format(wxT("/Elem%i"), i);

			// If valid element was read add it to table
			wxString outString;
			if (m_config->Read(fullKeyName, &outString))
			{
				outValue->push_back(outString);
			}
			else
			{
				// End of list
				break;
			}
		}
	}

	bool ConfigSection::ReadInteger(const wxChar* key, int* outValue) const
	{
		const wxString fullKeyName = m_sectionName + key;
		return m_config->Read(fullKeyName, outValue);
	}

	bool ConfigSection::ReadFloat(const wxChar* key, float* outValue) const
	{
		const wxString fullKeyName = m_sectionName + key;
		return m_config->Read(fullKeyName, outValue);
	}

	bool ConfigSection::ReadBool(const wxChar* key, bool* outValue) const
	{
		const wxString fullKeyName = m_sectionName + key;
		return m_config->Read(fullKeyName, outValue);
	}

	wxString ConfigSection::ReadString(const wxChar* key, const wxString& defaultValue) const
	{
		wxString outValue = defaultValue;
		const wxString fullKeyName = m_sectionName + key;
		m_config->Read(fullKeyName, &outValue);
		return outValue;
	}

	std::string ConfigSection::ReadStringStd(const wxChar* key, const std::string& defaultValue) const
	{
		wxString outValue;
		const wxString fullKeyName = m_sectionName + key;
		if (!m_config->Read(fullKeyName, &outValue))
			return defaultValue;

		return outValue.c_str().AsChar();
	}

	std::wstring ConfigSection::ReadStringStd(const wxChar* key, const std::wstring& defaultValue) const
	{
		wxString outValue;
		const wxString fullKeyName = m_sectionName + key;
		if (!m_config->Read(fullKeyName, &outValue))
			return defaultValue;

		return outValue.wc_str();
	}

	int ConfigSection::ReadInteger(const wxChar* key, int defaultValue) const
	{
		int outValue = defaultValue;
		const wxString fullKeyName = m_sectionName + key;
		m_config->Read(fullKeyName, &outValue);
		return outValue;
	}

	float ConfigSection::ReadFloat(const wxChar* key, float defaultValue) const
	{
		float outValue = defaultValue;
		const wxString fullKeyName = m_sectionName + key;
		m_config->Read(fullKeyName, &outValue);
		return outValue;
	}

	bool ConfigSection::ReadBool(const wxChar* key, bool defaultValue) const
	{
		bool outValue = defaultValue;
		const wxString fullKeyName = m_sectionName + key;
		m_config->Read(fullKeyName, &outValue);
		return outValue;
	}

	void ConfigSection::WriteString(const wxChar* key, const wxString& val)
	{
		const wxString fullKeyName = m_sectionName + key;
		m_config->Write(fullKeyName, val);
	}

	void ConfigSection::WriteStringArray(const wxChar* key, const wxArrayString& val)
	{
		// Delete group
		const wxString groupName = m_sectionName + key;
		m_config->DeleteGroup(groupName);

		// Save items
		for (uint32 i = 0; i < val.size(); ++i)
		{
			// Format array key name
			wxString fullKeyName = m_sectionName;
			fullKeyName += key;
			fullKeyName += wxString::Format(wxT("/Elem%i"), i);

			// Write element
			m_config->Write(fullKeyName, val[i]);
		}
	}

	void ConfigSection::WriteInteger(const wxChar* key, int val)
	{
		const wxString fullKeyName = m_sectionName + key;
		m_config->Write(fullKeyName, val);
	}

	void ConfigSection::WriteFloat(const wxChar* key, float val)
	{
		const wxString fullKeyName = m_sectionName + key;
		m_config->Write(fullKeyName, val);
	}

	void ConfigSection::WriteBool(const wxChar* key, bool val)
	{
		const wxString fullKeyName = m_sectionName + key;
		m_config->Write(fullKeyName, val);
	}

	void ConfigSection::RestoreControl(wxControl* control, const wxChar* nameOverride /*= nullptr*/) const
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

			for (uint32 i = 0; ; ++i)
			{
				// Format array key name
				wxString fullKeyName = m_sectionName;
				fullKeyName += key;
				fullKeyName += wxString::Format(wxT("/Elem%i"), i);

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

	void ConfigSection::SaveControl(const wxControl* control, const wxChar* nameOverride /*= nullptr*/)
	{
		const wxString key = (nameOverride) ? nameOverride : control->GetName();

		if (control->GetClassInfo()->IsKindOf(&wxComboBox::ms_classInfo))
		{
			const wxComboBox* comboBox = static_cast<const wxComboBox*>(control);

			// Delete group
			const wxString groupName = m_sectionName + key;
			m_config->DeleteGroup(groupName);

			// Save items
			const uint32 numElems = comboBox->GetCount();
			for (uint32 i = 0; i < numElems; ++i)
			{
				// Format array key name
				wxString fullKeyName = m_sectionName;
				fullKeyName += key;
				fullKeyName += wxString::Format(wxT("/Elem%i"), i);

				// Write element
				const wxString val = comboBox->GetString(i);
				m_config->Write(fullKeyName, val);
			}

			// Selected
			{

				wxString fullKeyName = m_sectionName;
				fullKeyName += key;
				fullKeyName += wxT("/Selected");

				const int selectedIndex = comboBox->GetSelection();
				m_config->Write(fullKeyName, selectedIndex);
			}
		}
		else if (control->GetClassInfo()->IsKindOf(&wxTextCtrl::ms_classInfo))
		{
			const wxTextCtrl* textCtrl = static_cast<const wxTextCtrl*>(control);
			m_config->Write(key, textCtrl->GetValue());
		}
		else if (control->GetClassInfo()->IsKindOf(&wxCheckBox::ms_classInfo))
		{
			const wxCheckBox* checkBox = static_cast<const wxCheckBox*>(control);
			m_config->Write(key, checkBox->GetValue());
		}
	}

	//---------------------------------------------------------------------------

	AutoConfig::AutoConfig(const char* varSectionName)
		: m_varSection(L"Vars/%hs", varSectionName)
	{
	}

	AutoConfig::~AutoConfig()
	{
	}

	void AutoConfig::Load()
	{
		OnLoadConfiguration();
	}

	void AutoConfig::Save()
	{
		OnSaveConfiguration();
	}

	void AutoConfig::OnLoadConfiguration()
	{
		for (uint32 i = 0; i < m_vars.size(); ++i)
		{
			const Var& v = m_vars[i];

			if (v.m_type == eConfigVarType_Int)
			{
				m_varSection.ReadInteger(v.m_name.wc_str(), (int*)v.m_ptr);
			}
			else if (v.m_type == eConfigVarType_Float)
			{
				m_varSection.ReadFloat(v.m_name.wc_str(), (float*)v.m_ptr);
			}
			else if (v.m_type == eConfigVarType_Bool)
			{
				m_varSection.ReadBool(v.m_name.wc_str(), (bool*)v.m_ptr);
			}
			else if (v.m_type == eConfigVarType_String)
			{
				m_varSection.ReadString(v.m_name.wc_str(), (wxString*)v.m_ptr);
			}
			else if (v.m_type == eConfigVarType_StringArray)
			{
				m_varSection.ReadStringArray(v.m_name.wc_str(), (wxArrayString*)v.m_ptr);
			}
		}		
	}

	void AutoConfig::OnSaveConfiguration()
	{
		for (uint32 i = 0; i < m_vars.size(); ++i)
		{
			const Var& v = m_vars[i];

			if (v.m_type == eConfigVarType_Int)
			{
				m_varSection.WriteInteger(v.m_name.wc_str(), *(int*)v.m_ptr);
			}
			else if (v.m_type == eConfigVarType_Float)
			{
				m_varSection.WriteFloat(v.m_name.wc_str(), *(float*)v.m_ptr);
			}
			else if (v.m_type == eConfigVarType_Bool)
			{
				m_varSection.WriteBool(v.m_name.wc_str(), *(bool*)v.m_ptr);
			}
			else if (v.m_type == eConfigVarType_String)
			{
				m_varSection.WriteString(v.m_name.wc_str(), *(wxString*)v.m_ptr);
			}
			else if (v.m_type == eConfigVarType_StringArray)
			{
				m_varSection.WriteStringArray(v.m_name.wc_str(), *(wxArrayString*)v.m_ptr);
			}
		}
	}

	void AutoConfig::AddConfigValueRaw(void* ptr, const char* name, EConfigVarType type)
	{
		for (uint32 i = 0; i < m_vars.size(); ++i)
		{
			if (m_vars[i].m_name == name)
			{
				return;
			}
		}

		Var info;
		info.m_name = name;
		info.m_type = type;
		info.m_ptr = ptr;
		m_vars.push_back(info);
	}

	void AutoConfig::AddConfigValue(int& var, const char* name)
	{
		AddConfigValueRaw(&var, name, eConfigVarType_Int);
	}

	void AutoConfig::AddConfigValue(float& var, const char* name)
	{
		AddConfigValueRaw(&var, name, eConfigVarType_Float);
	}

	void AutoConfig::AddConfigValue(wxString& var, const char* name)
	{
		AddConfigValueRaw(&var, name, eConfigVarType_String);
	}

	void AutoConfig::AddConfigValue(wxArrayString& var, const char* name)
	{
		AddConfigValueRaw(&var, name, eConfigVarType_StringArray);
	}

	void AutoConfig::AddConfigValue(bool& var, const char* name)
	{
		AddConfigValueRaw(&var, name, eConfigVarType_Bool);
	}

	//---------------------------------------------------------------------------

} // tools