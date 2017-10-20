#pragma once

namespace tools
{
	/// Dialog used to search for symbols in project
	class FindSymbolDialog : public wxDialog
	{
		DECLARE_EVENT_TABLE();

	public:
		FindSymbolDialog(const decoding::Environment& env, wxWindow* parent);
		~FindSymbolDialog();

		inline const char* GetSelectedSymbolName() const { return m_selectedSymbolName.c_str().AsChar(); }
		inline const uint64 GetSelectedSymbolAddress() const { return m_selectedSymbolAddress; }

	private:
		void OnTimer(wxTimerEvent& event);
		void OnOK(wxCommandEvent& event);
		void OnCancel(wxCommandEvent& event);
		void OnTextChanged(wxCommandEvent& event);
		void OnListSelected(wxListEvent& event);
		void OnListActivated(wxListEvent& event);
		void OnTextKeyEvent(wxKeyEvent& event);
		void OnTextEnter(wxCommandEvent& event);

		void UpdateSymbolList();

		struct SymbolInfo
		{
			const char*		m_name;
			uint32			m_address;
		};

		// known symbols
		typedef std::vector< SymbolInfo >	TSymbols;
		TSymbols			m_symbols;

		// update timer
		wxTimer				m_updateTimer;

		// ctrls
		wxListCtrl*			m_symbolList;
		wxTextCtrl*			m_symbolName;

		// selected symbol
		wxString			m_selectedSymbolName;
		uint64				m_selectedSymbolAddress;

		const decoding::Environment*	m_env;
	};

} // tools