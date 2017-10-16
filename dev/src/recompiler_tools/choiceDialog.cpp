#include "build.h"
#include "choiceDialog.h"

namespace tools
{

	BEGIN_EVENT_TABLE(ChoiceDialog, wxDialog)
		EVT_BUTTON(XRCID("OK"), ChoiceDialog::OnOK)
		EVT_BUTTON(XRCID("Cancel"), ChoiceDialog::OnCancel)
		END_EVENT_TABLE();

	ChoiceDialog::ChoiceDialog(wxWindow* parent)
	{
		// load the dialog
		wxXmlResource::Get()->LoadDialog(this, parent, wxT("SelectChoice"));
	}

	ChoiceDialog::~ChoiceDialog()
	{
	}

	bool ChoiceDialog::ShowDialog(const std::string& title, const std::string& message, const std::vector< std::string >& options, std::string& selected)
	{
		wxChoice* list = XRCCTRL(*this, "Options", wxChoice);

		// add choices
		{
			list->Freeze();
			list->Clear();

			int selectedIndex = -1;
			for (size_t i = 0; i < options.size(); ++i)
			{
				list->AppendString(options[i]);

				if (options[i] == selected)
				{
					selectedIndex = i;
				}
			}

			if (selectedIndex == -1)
				selectedIndex = 0;

			if (selectedIndex >= 0 && selectedIndex < (int)list->GetCount())
			{
				list->SetSelection(selectedIndex);
			}

			list->Thaw();
			list->Refresh();
		}

		// set dialog message and title
		SetTitle(title.c_str());
		XRCCTRL(*this, "txtLabel", wxStaticText)->SetLabel(message.c_str());

		// show
		if (0 == wxDialog::ShowModal())
		{
			const int selectedIndex = list->GetCurrentSelection();
			if (selectedIndex >= 0 && selectedIndex < (int)list->GetCount())
			{
				selected = list->GetString(selectedIndex).c_str().AsChar();
				return true;
			}
		}

		// invalid
		return false;
	}

	void ChoiceDialog::OnOK(wxCommandEvent& event)
	{
		EndDialog(0);
	}

	void ChoiceDialog::OnCancel(wxCommandEvent& event)
	{
		EndDialog(-1);
	}

} // tools