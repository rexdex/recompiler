#pragma once

namespace tools
{

	//---------------------------------------------------------------------------

	/// Configuration element
	class IConfiguration
	{
	public:
		IConfiguration();
		virtual ~IConfiguration();

		//! Save/Load configuration
		virtual void OnLoadConfiguration() = 0;
		virtual void OnSaveConfiguration() = 0;
	};

	//---------------------------------------------------------------------------

	/// Section in configuration editor
	class ConfigSection
	{
	public:
		//! Get the section name
		inline const wxString& GetSectionName() const { return m_sectionName; }

		//---

		explicit ConfigSection(const wxChar* sectionName, ...);
		explicit ConfigSection(const ConfigSection& base, const wxChar* sectionName, ...);
		~ConfigSection();

		//---

		//! Read string value
		bool ReadString(const wxChar* key, wxString* outValue) const;

		//! Read string value
		bool ReadString(const wxChar* key, std::string* outValue) const;

		//! Read string value
		bool ReadString(const wxChar* key, std::wstring* outValue) const;

		//! Read string array
		void ReadStringArray(const wxChar* key, wxArrayString* outValue) const;

		//! Read integer value
		bool ReadInteger(const wxChar* key, int* outValue) const;

		//! Read float value
		bool ReadFloat(const wxChar* key, float* outValue) const;

		//! Read boolean value
		bool ReadBool(const wxChar* key, bool* outValue) const;

		//---

		//! Read string value
		wxString ReadString(const wxChar* key, const wxString& defaultValue) const;

		//! Read string value
		std::string ReadStringStd(const wxChar* key, const std::string& defaultValue) const;

		//! Read string value
		std::wstring ReadStringStd(const wxChar* key, const std::wstring& defaultValue) const;

		//! Read integer value
		int ReadInteger(const wxChar* key, int defaultValue) const;

		//! Read float value
		float ReadFloat(const wxChar* key, float defaultValue) const;

		//! Read boolean value
		bool ReadBool(const wxChar* key, bool defaultValue) const;

		//---

		//! Write string value
		void WriteString(const wxChar* key, const wxString& val);

		//! Write array of strings
		void WriteStringArray(const wxChar* key, const wxArrayString& val);

		//! Write integer value
		void WriteInteger(const wxChar* key, int val);

		//! Write float value
		void WriteFloat(const wxChar* key, float val);

		//! Write boolean value
		void WriteBool(const wxChar* key, bool val);

		//---

		//! Restore setting of given control from config
		void RestoreControl(wxControl* control, const wxChar* nameOverride = nullptr) const;

		//! Save settings of given control to config
		void SaveControl(const wxControl* control, const wxChar* nameOverride = nullptr);

	private:
		wxConfigBase*	m_config;		//!< The config file
		wxString		m_sectionName;	//!< Config section name
	};

	//---------------------------------------------------------------------------

	// Configuration class that saves and loads itself from the config
	class AutoConfig : public IConfiguration
	{
	public:
		AutoConfig(const char* varSectionName);
		~AutoConfig();

		// Refresh the values from config
		void Load();

		// Save the configuration
		void Save();

	protected:
		void AddConfigValue(int& var, const char* name);
		void AddConfigValue(float& var, const char* name);
		void AddConfigValue(wxString& var, const char* name);
		void AddConfigValue(wxArrayString& var, const char* name);
		void AddConfigValue(bool& var, const char* name);

	private:
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
		TConfigVars m_vars;

		ConfigSection m_varSection;

		///--

		void AddConfigValueRaw(void* ptr, const char* name, EConfigVarType type);

		virtual void OnLoadConfiguration();
		virtual void OnSaveConfiguration();
	};

	//---------------------------------------------------------------------------

} // tools