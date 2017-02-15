#include "build.h"
#include "progressDialog.h"
#include "callstackFrame.h"
#include "callstackFunctionTree.h"
#include "project.h"
#include "frame.h"

#include "..\backend\traceCalltree.h"
#include "..\backend\decodingNameMap.h"
#include "..\backend\decodingEnvironment.h"
#include "..\backend\decodingContext.h"

BEGIN_EVENT_TABLE( CCallstackFrame, wxFrame )
	EVT_TOOL( XRCID("refreshCallstack"), CCallstackFrame::OnRefreshData )
	EVT_TOOL( XRCID("exportCallstack"), CCallstackFrame::OnExportData )
	EVT_TOOL( XRCID("showFunctionNames"), CCallstackFrame::OnToggleFunctionNames )	
	EVT_CLOSE( CCallstackFrame::OnClose )
END_EVENT_TABLE();

CCallstackFrame::CCallstackFrame( wxWindow* parent )
	: m_tree( nullptr )
{
	wxXmlResource::Get()->LoadFrame( this, parent, wxT("Callstack") );

	// create tree view
	{
		wxPanel* panel = XRCCTRL( *this, "TreeTab", wxPanel );
		m_tree = new CCallstackFunctionTree( panel );
		wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
		sizer->Add( m_tree, 1, wxALL | wxEXPAND, 0 );
		panel->SetSizer( sizer );
		panel->Layout();
		m_tree->Enable( false );
	}		

	// rebuild views
	BuildNormalTree();

	// register event listeren
	wxTheEvents->RegisterListener( this );

	// refresh layout
	Layout();
	Show();
}

CCallstackFrame::~CCallstackFrame()
{
	wxTheEvents->UnregisterListener( this );
}

void CCallstackFrame::Refresh()
{
	// no project
	if ( !wxTheFrame->GetProject() )
	{
		wxMessageBox( wxT("No loaded project, unable to build callstack data"), wxT("Error"), wxICON_ERROR | wxOK, this );
		return;
	}

	// no trace data
	if ( !wxTheFrame->GetProject()->GetTrace() )
	{
		wxMessageBox( wxT("No loaded trace data, unable to build callstack data"), wxT("Error"), wxICON_ERROR | wxOK, this );
		return;
	}

	// we alredy have a callstack
	if ( wxTheFrame->GetProject()->GetTrace()->GetCallstack() )
	{
		if ( wxNO == wxMessageBox( wxT("Callstack data already extracted, recompute ?"), wxT("Data override"), wxICON_QUESTION | wxYES_NO, this ) )
			return;
	}

	// update data
	if ( !wxTheFrame->GetProject()->GetTrace()->BuildCallstack( *wxTheFrame->GetLogView() ) )
	{
		wxMessageBox( wxT("Failed to build callstack data"), wxT("Error"), wxICON_ERROR | wxOK, this );
	}

	BuildNormalTree();
}

void CCallstackFrame::BuildNormalTree()
{
	const bool showFunctionNames = XRCCTRL( *this, "Toolbar", wxToolBar )->GetToolState( XRCID("showFunctionNames" ) );
	const uint32 maxDepth = 100;

	if ( wxTheFrame->GetProject()->GetTrace() )
	{
		m_tree->Rebuild( wxTheFrame->GetProject()->GetTrace()->GetCallstack(), maxDepth, showFunctionNames );
	}
}

void CCallstackFrame::OnRefreshData( wxCommandEvent& event )
{
	Refresh();
}

void CCallstackFrame::OnToggleFunctionNames( wxCommandEvent& event )
{
	BuildNormalTree();
}

void CCallstackFrame::OnExportData( wxCommandEvent& event )
{
}

void CCallstackFrame::OnClose( wxCloseEvent& event )
{
	event.Veto();
	Hide();
}

void CCallstackFrame::OnTraceOpened()
{
	BuildNormalTree();
}

void CCallstackFrame::OnTraceClosed()
{
	BuildNormalTree();
}

void CCallstackFrame::OnTraceIndexChanged( const uint32 currentEntry )
{
	m_tree->Highlight( currentEntry );
}
