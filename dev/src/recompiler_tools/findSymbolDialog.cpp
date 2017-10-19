#include "build.h"
#include "findSymbolDialog.h"

#include "../recompiler_core/decodingNameMap.h"
#include "../recompiler_core/decodingEnvironment.h"
#include "../recompiler_core/decodingContext.h"
#include "../recompiler_core/decodingMemoryMap.h"

namespace tools
{
	BEGIN_EVENT_TABLE(FindSymbolDialog, wxDialog)
		EVT_BUTTON(XRCID("OK"), FindSymbolDialog::OnOK)
		EVT_BUTTON(XRCID("Cancel"), FindSymbolDialog::OnCancel)
		EVT_TEXT(XRCID("SymbolName"), FindSymbolDialog::OnTextChanged)
		EVT_TEXT_ENTER(XRCID("SymbolName"), FindSymbolDialog::OnTextEnter)
		EVT_LIST_ITEM_SELECTED(XRCID("SymbolList"), FindSymbolDialog::OnListSelected)
		EVT_LIST_ITEM_ACTIVATED(XRCID("SymbolList"), FindSymbolDialog::OnListActivated)
		EVT_TIMER(12345, FindSymbolDialog::OnTimer)
		END_EVENT_TABLE()

		wxString CFindSymbolDialogFilter;

	FindSymbolDialog::FindSymbolDialog(decoding::Environment& env, wxWindow* parent)
		: m_updateTimer(this, 12345)
		, m_env(&env)
	{
		// load the dialog
		wxXmlResource::Get()->LoadDialog(this, parent, wxT("FindSymbol"));

		// create columns
		m_symbolList = XRCCTRL(*this, "SymbolList", wxListCtrl);
		m_symbolList->AppendColumn("Address", wxLIST_FORMAT_CENTER, 80);
		m_symbolList->AppendColumn("Symbol name", wxLIST_FORMAT_LEFT, 400);
		m_symbolList->AppendColumn("Symbol flags", wxLIST_FORMAT_LEFT, 200);

		// enable text edition
		m_symbolName = XRCCTRL(*this, "SymbolName", wxTextCtrl);
		m_symbolName->SetFocus();
		m_symbolName->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(FindSymbolDialog::OnTextKeyEvent), NULL, this);
		m_symbolName->SetValue(CFindSymbolDialogFilter);
		m_symbolName->SelectAll();

		// refresh list
		UpdateSymbolList();
	}

	FindSymbolDialog::~FindSymbolDialog()
	{
	}

	void FindSymbolDialog::OnTimer(wxTimerEvent& event)
	{
		UpdateSymbolList();
	}

	void FindSymbolDialog::OnOK(wxCommandEvent& event)
	{
		EndDialog(0);
	}

	void FindSymbolDialog::OnCancel(wxCommandEvent& event)
	{
		EndDialog(-1);
	}

	void FindSymbolDialog::OnTextChanged(wxCommandEvent& event)
	{
		m_updateTimer.Stop();
		m_updateTimer.Start(200, true);
	}

	void FindSymbolDialog::OnListSelected(wxListEvent& event)
	{
		m_selectedSymbolAddress = 0;
		m_selectedSymbolName = wxEmptyString;

		int focusedIndex = m_symbolList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (focusedIndex != -1)
		{
			uint32 addr = 0;
			wxString addrString = m_symbolList->GetItemText(focusedIndex, 0);
			if (1 == sscanf_s(addrString.c_str().AsChar(), "%08Xh", &addr))
			{
				m_selectedSymbolAddress = addr;
				m_selectedSymbolName = m_symbolList->GetItemText(focusedIndex, 1);
			}
		}
	}

	void FindSymbolDialog::OnListActivated(wxListEvent& event)
	{
		if (!m_selectedSymbolName.empty())
		{
			EndDialog(0);
		}
	}

	void FindSymbolDialog::OnTextKeyEvent(wxKeyEvent& event)
	{
		if (m_symbolList->GetItemCount() > 0)
		{
			if (event.GetKeyCode() == WXK_UP)
			{
				m_symbolList->Freeze();

				int focusedIndex = m_symbolList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
				int newFocusedIndex = focusedIndex;
				if (focusedIndex > 0)
					newFocusedIndex = focusedIndex - 1;
				else
					newFocusedIndex = m_symbolList->GetItemCount() - 1;

				if (newFocusedIndex != focusedIndex)
				{
					m_symbolList->SetItemState(focusedIndex, 0, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
					m_symbolList->SetItemState(newFocusedIndex, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
					m_symbolList->EnsureVisible(newFocusedIndex);
				}

				m_symbolList->Thaw();
				m_symbolList->Refresh(false);
			}
			else if (event.GetKeyCode() == WXK_DOWN)
			{
				m_symbolList->Freeze();

				int focusedIndex = m_symbolList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
				int newFocusedIndex = focusedIndex;
				if (focusedIndex < (m_symbolList->GetItemCount() - 1))
					newFocusedIndex = focusedIndex + 1;
				else
					newFocusedIndex = 0;

				if (newFocusedIndex != focusedIndex)
				{
					m_symbolList->SetItemState(focusedIndex, 0, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
					m_symbolList->SetItemState(newFocusedIndex, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
					m_symbolList->EnsureVisible(newFocusedIndex);
				}

				m_symbolList->Thaw();
				m_symbolList->Refresh(false);
			}
		}

		event.Skip();
	}

	void FindSymbolDialog::OnTextEnter(wxCommandEvent& event)
	{
		if (!m_selectedSymbolName.empty())
		{
			EndDialog(0);
		}
	}

	class SymbolAdder : public decoding::NameMap::ISymbolConsumer
	{
	public:
		wxListCtrl*					m_list;
		const decoding::MemoryMap*	m_memoryMap;


		SymbolAdder(wxListCtrl* list, const decoding::MemoryMap* memoryMap)
			: m_list(list)
			, m_memoryMap(memoryMap)
		{
		}

		virtual void ProcessSymbols(const decoding::NameMap::SymbolInfo* symbols, const uint32 numSymbols) override
		{
			for (uint32 i = 0; i < numSymbols; ++i)
			{
				const uint32 fullAddress = symbols[i].m_address;
				char addressText[16];
				sprintf_s(addressText, "%08Xh", fullAddress);

				int index = m_list->InsertItem(m_list->GetItemCount(), addressText, -1);
				m_list->SetItem(index, 1, symbols->m_name);

				wxString flagsStr;
				const decoding::MemoryFlags flags = m_memoryMap->GetMemoryInfo(fullAddress);
				if (flags.IsExecutable())
				{
					if (flags.GetInstructionFlags().IsFunctionStart()) flagsStr += "Function ";
					if (flags.GetInstructionFlags().IsEntryPoint()) flagsStr += "EntryPoint ";
					if (flags.GetInstructionFlags().IsImportFunction()) flagsStr += "Import ";
				}

				m_list->SetItem(index, 2, flagsStr);
			}
		}
	};

	void FindSymbolDialog::UpdateSymbolList()
	{
		// begin update
		m_symbolList->Freeze();
		m_symbolList->DeleteAllItems();

		// get the filter text
		CFindSymbolDialogFilter = m_symbolName->GetValue();
		std::string filterText = CFindSymbolDialogFilter.c_str().AsChar();

		// add symbols
		SymbolAdder adder(m_symbolList, &m_env->GetDecodingContext()->GetMemoryMap());
		m_env->GetDecodingContext()->GetNameMap().EnumerateSymbols(filterText.c_str(), adder);

		// select something
		if (m_symbolList->GetItemCount() > 0)
		{
		}

		// end update
		m_symbolList->Thaw();
		m_symbolList->Refresh(false);
	}
} // tools