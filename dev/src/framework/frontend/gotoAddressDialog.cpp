#include "build.h"
#include "gotoAddressDialog.h"
#include "project.h"
#include "memoryView.h"
#include "widgetHelpers.h"

#include "../backend/decodingEnvironment.h"
#include "../backend/image.h"

BEGIN_EVENT_TABLE( CGotoAddressDialog, wxDialog )
	EVT_LISTBOX( XRCID("ModeList"), CGotoAddressDialog::OnReferenceChanged )
	EVT_LISTBOX_DCLICK( XRCID("ModeList"), CGotoAddressDialog::OnReferenceDblClick )
	EVT_LISTBOX_DCLICK( XRCID("RecentList"), CGotoAddressDialog::OnHistoryDblClick )
	EVT_BUTTON( XRCID("Go"), CGotoAddressDialog::OnGoTo )
	EVT_BUTTON( XRCID("Cancel"), CGotoAddressDialog::OnCancel )
	EVT_TOGGLEBUTTON( XRCID("DecHex"), CGotoAddressDialog::OnAddressTypeChanged )
	EVT_TIMER( 12345, CGotoAddressDialog::OnNavigationTimer )
END_EVENT_TABLE()

CGotoAddressDialog::CGotoAddressDialog( wxWindow* parent, class Project* project, class CMemoryView* view )
	: m_project( project )
	, m_view( view )
	, m_address( 0 )
	, m_navigateIndex( 0 )
	, m_navigateTimer( this, 12345 )
{
	// load the dialog
	wxXmlResource::Get()->LoadDialog( this, parent, wxT("GotoAddress") );

	// initialize custom helpers
	m_decHex = new CToggleButtonAuto( this, "DecHex", "Hex", "Dec", true );
	m_addressErrorHint = new CTextBoxErrorHint( this, "RelativeAddress" );

	// set the inital address from the view (view is not required!)
	if ( view != nullptr )
	{
		m_address = view->GetCurrentRVA();
	}

	// update the address list and everything
	UpdateReferenceList();
	UpdateHistoryList();
	UpdateRelativeAddress();

	// set focus to the text control
	XRCCTRL( *this, "RelativeAddress", wxTextCtrl)->SetFocus();
	XRCCTRL( *this, "RelativeAddress", wxTextCtrl)->Connect( wxEVT_KEY_DOWN, wxKeyEventHandler( CGotoAddressDialog::OnTextKeyDown ), NULL, this );

	// layout the dialog
	Layout();
}

CGotoAddressDialog::~CGotoAddressDialog()
{
}
	
const uint32 CGotoAddressDialog::GetNewAddress()
{
	if ( 0 != wxDialog::ShowModal() )
	{
		return INVALID_ADDRESS;
	}

	// return the final address
	return m_address;
}

void CGotoAddressDialog::UpdateHistoryList()
{
	CListBoxUpdate list( this, "RecentList", false, false );

	// get the address history
	std::vector< uint32 > fullAddressHistory;
	m_project->GetAddressHistory().GetFullHistory( fullAddressHistory );
	std::sort( fullAddressHistory.begin(), fullAddressHistory.end() );

	// get reference address
	const uint32 baseAddressRVA = TListBoxSelection<uint32>( this, "ModeList" ).GetSelection(0);

	// display the address list
	uint32 prevAddr = 0;
	for ( uint32 i=0; i<fullAddressHistory.size(); ++i )
	{
		const uint32 rva = fullAddressHistory[i];
		if ( rva == prevAddr )
		{
			continue;
		}

		prevAddr = rva;
		if ( baseAddressRVA == 0 )
		{
			list.AddItemWithData( new TClientData< uint32 >(rva), false, "%08Xh", rva );
		}
		else
		{
			wxLongLong_t relative = (wxLongLong_t)rva - (wxLongLong_t)baseAddressRVA;
			if ( relative >= 0 )
			{
				list.AddItemWithData( new TClientData< uint32 >(rva), false, "%08Xh (base + %04Xh)", rva, (uint32)relative );
			}
			else
			{
				const uint32 absRelative = (uint32)( -relative ); 
				list.AddItemWithData( new TClientData< uint32 >(rva), false, "%08Xh (base - %04Xh)", rva, (uint32)absRelative );
			}
		}
	}
}

void CGotoAddressDialog::UpdateReferenceList()
{
	CListBoxUpdate list( this, "ModeList" );

	// absolute memory base
	{
		const uint32 memoryBase = 0;
		list.AddItemWithData( new TClientData<uint32>(0), true, "Absolute address (%08Xh)", 0 );
	}

	// we have the image :)
	auto* image = m_project->GetEnv().GetImage();
	const uint32 imageBase = image->GetBaseAddress();
	list.AddItemWithData( new TClientData<uint32>(imageBase), false, "Base of the image (%08Xh)", imageBase );

	// if we have a memory view we have more options
	if ( m_view != nullptr )
	{
		// get the current address
		const uint32 currentRVA = m_view->GetCurrentRVA();

		// get the section base
		{
			// find base section
			const image::Section* section = nullptr;
			const uint32 numSection = image->GetNumSections();
			for ( uint32 i=0; i<numSection; ++i )
			{
				const image::Section* test = image->GetSection(i);
				const uint32 sectionStartRVA = imageBase + test->GetVirtualAddress();
				const uint32 sectionEndRVA = imageBase + test->GetVirtualAddress() + test->GetVirtualSize();

				if ( currentRVA >= sectionStartRVA && currentRVA < sectionEndRVA )
				{
					section = test;
					break;
				}
			}

			// relative to section base
			if ( nullptr != section )
			{
				const uint32 sectionBase = imageBase + section->GetVirtualAddress();
				list.AddItemWithData( new TClientData<uint32>(sectionBase), false, "Base of the section '%s' (%08Xh)", section->GetName(), sectionBase );
			}
		}

		// relative to current address
		list.AddItemWithData( new TClientData<uint32>(currentRVA), false, "Current address (%08Xh)", currentRVA );
	}
}	

void CGotoAddressDialog::UpdateRelativeAddress()
{
	// get the base address
	const uint32 baseAddress = TListBoxSelection<uint32>( this, "ModeList" ).GetSelection(0);

	// compute the relative address
	const bool hexBase = m_decHex->GetValue();
	const wxLongLong_t relativeAddress = (wxLongLong_t)m_address - (wxLongLong_t)baseAddress;
	const uint32 absRelativeAddress = relativeAddress>0 ? (uint32)relativeAddress : (uint32)-relativeAddress;
	CTextBoxUpdate( this, "RelativeAddress" ).SetText( 
		(relativeAddress >= 0) ? 
			(hexBase ? "%08X" : "%d") :
			(hexBase ? "-%08X" : "-%d"),
		absRelativeAddress );

	// reset the error hint
	m_addressErrorHint->ResetErrorHint();
}

bool CGotoAddressDialog::GrabCurrentAddress()
{
	const bool hex = m_decHex->GetValue();
	wxLongLong_t offset = 0;
	if ( CTextBoxReader( this, "RelativeAddress" ).GetInt( offset, hex ) )
	{
		const uint32 baseAddress = TListBoxSelection<uint32>( this, "ModeList" ).GetSelection(0);
		m_address = (uint32)( (wxLongLong_t)baseAddress + offset );
		return true;
	}

	// invalid address
	return false;
}

void CGotoAddressDialog::OnReferenceChanged( wxCommandEvent& event )
{
	UpdateHistoryList();
	UpdateRelativeAddress();
}

void CGotoAddressDialog::OnReferenceDblClick( wxCommandEvent& event )
{
	const uint32 address = TListBoxSelection<uint32>( this, "ModeList" ).GetSelection( m_address );
	if ( m_project->GetEnv().GetImage()->IsValidAddress( address ) )
	{
		m_address = address;
		EndDialog( 0 );
	}
}

void CGotoAddressDialog::OnHistorySelected( wxCommandEvent& event )
{
	UpdateRelativeAddress();
}

void CGotoAddressDialog::OnHistoryDblClick( wxCommandEvent& event )
{
	const uint32 address = TListBoxSelection<uint32>( this, "RecentList" ).GetSelection( m_address );
	if ( m_project->GetEnv().GetImage()->IsValidAddress( address ) )
	{
		m_address = address;
		EndDialog( 0 );
	}
}

void CGotoAddressDialog::OnGoTo( wxCommandEvent& event )
{
	if ( GrabCurrentAddress() )
	{
		// is the address valid
		if ( m_project->GetEnv().GetImage()->IsValidAddress( m_address ) )
		{
			// exit the dialog
			EndDialog( 0 );
			return;
		}
	}

	// specified address was not valid
	m_addressErrorHint->SetErrorHint( true );
}

void CGotoAddressDialog::OnCancel( wxCommandEvent& event )
{
	EndDialog( -1 );
}

void CGotoAddressDialog::OnAddressTypeChanged( wxCommandEvent& event )
{
	wxLongLong_t offset = 0;
	if ( CTextBoxReader( this, "RelativeAddress" ).GetInt( offset, !m_decHex->GetValue() ) )
	{
		const uint32 baseAddress = TListBoxSelection<uint32>( this, "ModeList" ).GetSelection(0);
		m_address = (uint32)( (wxLongLong_t)baseAddress + offset );
		UpdateRelativeAddress();
	}
}

void CGotoAddressDialog::OnTextKeyDown( wxKeyEvent& event )
{
	// fake navigation
	if ( event.GetKeyCode() == WXK_UP || event.GetKeyCode() == WXK_DOWN )
	{
		wxListBox* list = XRCCTRL(*this, "ModeList", wxListBox );
		if (list->GetCount() > 1)
		{
			int index = list->GetSelection();
			const int lastIndex = list->GetCount() - 1;
			if ( event.GetKeyCode() == WXK_UP )
			{
				m_navigateIndex = (index == 0) ? lastIndex : (index-1);
				m_navigateTimer.StartOnce( 50 );
			}
			else if ( event.GetKeyCode() == WXK_DOWN )
			{
				m_navigateIndex = (index == lastIndex) ? 0 : index + 1;
				m_navigateTimer.StartOnce( 50 );
			}
		}

		return;
	}

	// enter
	if ( event.GetKeyCode() == WXK_RETURN )
	{
		wxCommandEvent fakeEvent;
		OnGoTo(fakeEvent);

		return;
	}

	// skip rest of the chars
	event.Skip();
}

void CGotoAddressDialog::OnNavigationTimer( wxTimerEvent& event )
{
	wxListBox* list = XRCCTRL(*this, "ModeList", wxListBox );
	if ( m_navigateIndex != list->GetSelection() )
	{
		list->SetFocus();
		list->SetSelection( m_navigateIndex );

		UpdateHistoryList();
		UpdateRelativeAddress();

		XRCCTRL( *this, "RelativeAddress", wxTextCtrl)->SetFocus();
		XRCCTRL( *this, "RelativeAddress", wxTextCtrl)->SetSelection(-1,-1);
	}
}