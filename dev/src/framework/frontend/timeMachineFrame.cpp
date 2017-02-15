#include "build.h"
#include "timeMachineView.h"
#include "timeMachineFrame.h"
#include "project.h"
#include "projectTraceData.h"
#include "frame.h"

//---------------------------------------------------------------------------

BEGIN_EVENT_TABLE( CTimeMachineFrame, wxFrame )
	EVT_CLOSE( OnClose )
END_EVENT_TABLE()

//---------------------------------------------------------------------------

CTimeMachineFrame::CTimeMachineFrame( wxWindow* parent )
{
	wxXmlResource::Get()->LoadFrame( this, parent, wxT("TimeMachine") );

	// create notebook
	{
		wxPanel* tabPanel = XRCCTRL( *this, "TabsPanel", wxPanel );

		m_tabs = new wxAuiNotebook( tabPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_DEFAULT_STYLE & ~wxAUI_NB_CLOSE_BUTTON & ~wxAUI_NB_CLOSE_ON_ACTIVE_TAB );

		wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
		sizer->Add( m_tabs, 1, wxALL | wxEXPAND, 0 );
		tabPanel->SetSizer( sizer );
		tabPanel->Layout();
	}

	// refresh layout
	Layout();
	Show();
}

CTimeMachineFrame::~CTimeMachineFrame()
{
}

void CTimeMachineFrame::CloseAllTabs()
{
}

void CTimeMachineFrame::CreateNewTrace( const uint32 traceFrame )
{
	// look for existing entry
	const uint32 numTabs = m_tabs->GetPageCount();
	for ( uint32 i=0; i<numTabs; ++i )
	{
		const CTimeMachineView* view = static_cast< const CTimeMachineView* >( m_tabs->GetPage(i) );
		if ( view && view->GetRootTraceIndex() == traceFrame )
		{
			m_tabs->SetSelection( i );
			return;
		}
	}

	// no trace loaded
	if ( !wxTheFrame->GetProject() || !wxTheFrame->GetProject()->GetTrace() )
		return;

	// create time machine trace
	timemachine::Trace* trace = wxTheFrame->GetProject()->GetTrace()->CreateTimeMachineTrace( traceFrame );
	if ( !trace )
		return;

	// create trace entry
	CTimeMachineView* traceView = new CTimeMachineView( m_tabs, trace );
	m_tabs->AddPage( traceView, wxString::Format( "Trace #%05d", traceFrame ), true );
	
	// show the window
	Layout();
	Show();
}

void CTimeMachineFrame::OnClose( wxCloseEvent& event )
{
	event.Veto();
	Hide();
}

//---------------------------------------------------------------------------
