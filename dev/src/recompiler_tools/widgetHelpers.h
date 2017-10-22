#pragma once

namespace tools
{

	//---------------------------------------------------------------------------

	/// Specialized client data
	template< typename T >
	class TClientData : public wxClientData
	{
	private:
		T		m_data;

	public:
		TClientData(const T& data)
			: m_data(data)
		{}

		inline const T& GetData()
		{
			return m_data;
		}
	};

	//---------------------------------------------------------------------------

	/// Specialized client data with auto delete
	template< typename T >
	class TClientDataPtr : public wxClientData
	{
	private:
		T*		m_data;

	public:
		TClientDataPtr(T* data)
			: m_data(data)
		{}

		virtual ~TClientDataPtr()
		{
			delete m_data;
			m_data = nullptr;
		}

		inline T* GetData()
		{
			return m_data;
		}
	};

	//---------------------------------------------------------------------------

	/// Specialized wxObject with reference counted pointer
	template< typename T >
	class TWXObjectRefPtr : public wxObject
	{
	private:
		T*		m_data;

	public:
		TWXObjectRefPtr(const T * data)
			: m_data(const_cast<T*>(data))
		{
			if (m_data)
			{
				m_data->AddRef();
			}
		}

		virtual ~TWXObjectRefPtr()
		{
			if (m_data)
			{
				m_data->Release();
			}
		}

		inline const T* GetObject()
		{
			return m_data;
		}
	};

	//---------------------------------------------------------------------------

	/// Specialized wxObject with reference counted pointer
	template< typename T >
	class TWXObjectPtr : public wxObject
	{
	private:
		T*		m_data;

	public:
		TWXObjectPtr(const T * data)
			: m_data(const_cast<T*>(data))
		{
		}

		virtual ~TWXObjectPtr()
		{
		}

		inline const T* GetObject()
		{
			return m_data;
		}
	};

	//---------------------------------------------------------------------------

	/// List box update helper
	class ListBoxUpdate : public wxEvtHandler
	{
	private:
		class wxWindow*		m_parent;
		class wxListBox*	m_list;
		bool				m_allowEmpty;
		bool				m_issueEvent;
		int					m_selectedIndex;

	public:
		ListBoxUpdate(class wxWindow* parent, const char* name, bool allowEmpty = false, bool issueEvent = true);
		~ListBoxUpdate();

		// add the item to the list
		void AddItem(bool isSelected, const char* txt, ...);

		// add item with user data to the list
		void AddItemWithData(wxClientData* data, bool isSelected, const char* txt, ...);

	private:
		// fake event that eats the selection change
		void OnSelectionChanged(wxCommandEvent& event);
	};

	//---------------------------------------------------------------------------

	/// List box selection value extractor
	template< typename T >
	class TListBoxSelection
	{
	private:
		class wxListBox*	m_list;

	public:
		TListBoxSelection(wxWindow* parent, const char* name)
		{
			wxWindow* window = parent->FindWindow(XRCID(name));
			m_list = wxDynamicCast(window, wxListBox);
		}

		inline T GetSelection(const T& defaultValue = T(0)) const
		{
			// extract selected value from the list box
			if (nullptr != m_list)
			{
				const int selectedIndex = m_list->GetSelection();
				if (selectedIndex != -1)
				{
					if (m_list->HasClientObjectData())
					{
						TClientData<T>* data = static_cast<TClientData<T>*>(m_list->GetClientObject(selectedIndex));
						if (nullptr != data)
						{
							return data->GetData();
						}
					}
				}
			}

			// use default value
			return defaultValue;
		}
	};

	//---------------------------------------------------------------------------

	/// Text window updater
	class TextBoxUpdate : public wxEvtHandler
	{
	private:
		class wxTextCtrl*	m_textBox;
		bool				m_changed;

	public:
		TextBoxUpdate(wxWindow* parent, const char* name);
		~TextBoxUpdate();

		// set the text value
		TextBoxUpdate& SetText(const char* txt, ...);

		// select all of the test
		TextBoxUpdate& SelectText();

	private:
		// fake event
		void OnTextChanged(wxCommandEvent& event);
	};

	//---------------------------------------------------------------------------

	/// Text window value reader
	class TextBoxReader
	{
	private:
		class wxTextCtrl*	m_textBox;
		bool				m_changed;

	public:
		TextBoxReader(wxWindow* parent, const char* name);
		~TextBoxReader();

		// get string value
		const wxString GetString(const char* defaultText = "") const;

		// get numerical value
		const bool GetInt(wxLongLong_t& outValue, const bool hex = false) const;
	};

	//---------------------------------------------------------------------------

	/// Highlights the error state in the text box
	class TextBoxErrorHint : public wxEvtHandler
	{
	private:
		class wxTextCtrl*	m_textBox;
		bool				m_hasError;
		wxColour			m_normalColor;
		wxColour			m_errorColor;

	public:
		TextBoxErrorHint(wxWindow* parent, const char* name);
		~TextBoxErrorHint();

		// set the error hint
		void SetErrorHint(const bool hasError = true);

		// reset the error hint
		void ResetErrorHint();

	private:
		void OnTextChanged(wxCommandEvent& event);
	};

	//---------------------------------------------------------------------------

	/// Toggle button helper
	class ToggleButtonAuto : public wxEvtHandler
	{
	private:
		class wxToggleButton*	m_toggleButton;
		wxString				m_onString;
		wxString				m_offString;
		bool					m_value;

	public:
		ToggleButtonAuto(wxWindow* parent, const char* name, const char* customOnText = nullptr, const char* customOffText = nullptr, const bool defaultValue = false);
		~ToggleButtonAuto();

		// get button value
		const bool GetValue() const;

		// set button value
		void SetValue(const bool isToggled);

	private:
		void OnButtonToggled(wxCommandEvent& event);
	};

	//---------------------------------------------------------------------------

	/// One pointer
	template< class T >
	class THelperPtr
	{
	private:
		T*		m_ptr;

	public:
		inline THelperPtr()
			: m_ptr(nullptr)
		{}

		inline ~THelperPtr()
		{
			if (m_ptr != nullptr)
			{
				delete m_ptr;
				m_ptr = nullptr;
			}
		}

		inline void operator=(T* ptr)
		{
			delete m_ptr;
			m_ptr = ptr;
		}

		inline T* operator->() const
		{
			return m_ptr;
		}

	private:
		inline THelperPtr<T>(const THelperPtr<T>&) {};
		THelperPtr<T>& operator==(const THelperPtr<T>&) { return *this; };
	};

	//---------------------------------------------------------------------------

	template < class T >
	static inline void ReleaseVector(std::vector<T*>& table)
	{
		for (std::vector<T*>::const_iterator it = table.begin();
			it != table.end(); ++it)
		{
			(*it)->Release();
		}

		table.clear();
	}

	template < class T >
	static inline void ReleaseVectorConst(std::vector<const T*>& table)
	{
		for (std::vector<const T*>::const_iterator it = table.begin();
			it != table.end(); ++it)
		{
			const_cast<T*>((*it))->Release();
		}

		table.clear();
	}

	template < typename V, typename K >
	static inline void ReleaseMap(std::map<K, V*>& table)
	{
		for (std::map<K, V*>::const_iterator it = table.begin();
			it != table.end(); ++it)
		{
			(*it).second->Release();
		}

		table.clear();
	}

	template < class T >
	static inline void DeleteVector(std::vector<T*>& table)
	{
		for (std::vector<T*>::const_iterator it = table.begin();
			it != table.end(); ++it)
		{
			delete (*it);
		}

		table.clear();
	}

	template < typename V, typename K >
	static inline void DeleteMap(std::map<K, V*>& table)
	{
		for (std::map<K, V*>::const_iterator it = table.begin();
			it != table.end(); ++it)
		{
			delete (*it).second;
		}

		table.clear();
	}

	//---------------------------------------------------------------------------

	extern bool ParseHexValue64(const char* start, uint64& outValue);
	extern bool ParseAddress(const wxString& str, uint64& outValue);

	//---------------------------------------------------------------------------

} // tools