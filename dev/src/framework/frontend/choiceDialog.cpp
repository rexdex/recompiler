#include "build.h"
#include "choiceDialog.h"

BEGIN_EVENT_TABLE( CChoiceDialog, wxDialog )
	EVT_BUTTON( XRCID("OK"), CChoiceDialog::OnOK )
	EVT_BUTTON( XRCID("Cancel"), CChoiceDialog::OnCancel )
END_EVENT_TABLE();

CChoiceDialog::CChoiceDialog( wxWindow* parent )
{
	// load the dialog
	wxXmlResource::Get()->LoadDialog( this, parent, wxT("SelectChoice") );
}

CChoiceDialog::~CChoiceDialog()
{
}

bool CChoiceDialog::ShowDialog( const std::string& title, const std::string& message, const std::vector< std::string >& options, std::string& selected )
{
	wxChoice* list = XRCCTRL( *this, "Options", wxChoice );

	// add choices
	{
		list->Freeze();
		list->Clear();

		int selectedIndex = -1;
		for ( size_t i=0; i<options.size(); ++i )
		{
			list->AppendString( options[i] );			

			if ( options[i] == selected )
			{
				selectedIndex = i;
			}
		}

		if ( selectedIndex == -1 )
			selectedIndex = 0;

		if ( selectedIndex >= 0 && selectedIndex < (int)list->GetCount() )
		{
			list->SetSelection( selectedIndex );
		}

		list->Thaw();
		list->Refresh();
	}

	// set dialog message and title
	SetTitle( title.c_str() );
	XRCCTRL( *this, "txtLabel", wxStaticText )->SetLabel( message.c_str() );

	// show
	if ( 0 == wxDialog::ShowModal() )
	{
		const int selectedIndex = list->GetCurrentSelection();
		if ( selectedIndex >= 0 && selectedIndex < (int)list->GetCount() )
		{
			selected = list->GetString(selectedIndex).c_str().AsChar();
			return true;
		}
	}

	// invalid
	return false;
}

void CChoiceDialog::OnOK( wxCommandEvent& event )
{
	EndDialog(0);
}

void CChoiceDialog::OnCancel( wxCommandEvent& event )
{
	EndDialog(-1);
}

