#include "build.h"
#include "frame.h"
#include "nameUndecorator.h"
#include "memoryView.h"
#include "logWindow.h"
#include "project.h"
#include "progressDialog.h"
#include "projectMemoryView.h"
#include "widgetHelpers.h"
#include "registerView.h"
#include "traceInfoView.h"
#include "progressDialog.h"
#include "../backend/platformDefinition.h"
#include "../backend/platformLibrary.h"
#include "../backend/decodingEnvironment.h"
#include "../backend/image.h"

//#include "task_LoadImage.h"

CMainFrame* wxTheFrame = nullptr;

wxIMPLEMENT_CLASS( CMainFrame, wxFrame );

/// Help screen
class HelpImageMemoryViewer : public IMemoryDataView
{
private:
	uint32		m_length;
	uint8*		m_bytes;
	uint8*		m_size;
	uint8*		m_flags; // 1- label, 2 - align coment

public:
	HelpImageMemoryViewer( const uint32 length = 1 )//1024*1024 )
		: m_length( length )
	{
		m_bytes = new uint8[ length ];
		m_flags = new uint8[ length ];
		m_size = new uint8[ length ];

		memset( m_bytes, 0, length );
		memset( m_flags, 0, length );
		memset( m_flags, 0, length );

		for ( uint32 i=0; i<length; ++i )
		{
			m_bytes[i] = rand() & 0xFF;
		}

		uint32 offset = 0;
		while ( offset < length )
		{
			uint32 cnt = rand()  % 100;

			if ( cnt < 33 )
			{
				m_size[offset] = 1;
			}
			else if ( cnt < 64 )
			{
				m_size[offset] = 2;
			}
			else
			{
				m_size[offset] = 4;
			}

			if ( offset == 0 )
			{
				m_size[0] = 1;
			}

			const uint32 left = length - offset;
			if ( m_size[offset] >left )
			{
				m_size[offset] = 1;
			}

			cnt = rand()  % 100;

			if ( cnt < 10 )
			{
				m_flags[ offset ] |= 1; // align
			}

			cnt = rand()  % 100;

			if ( cnt < 10 )
			{
				m_flags[ offset ] |= 2; // label
			}

			offset += m_size[ offset ];
		}
	}

	virtual uint32 GetLength() const override { return m_length; }
	virtual uint32 GetBaseAddress() const override { return 0; }

	virtual uint32 GetRawBytes( const uint32 startOffset, const uint32 endOffset, const uint32 bufferSize, uint8* outBuffer ) const
	{
		for ( uint32 i=startOffset; i<endOffset; ++i )
		{
			outBuffer[i-startOffset] = m_bytes[i];
		}

		return 0;
	}

	virtual uint32 GetAddressInfo( const uint32 address, uint32& outNumLines, uint32& outNumBytes ) const
	{
		if ( address == 0 )
		{
			outNumLines = 7;
			outNumBytes = 1;
			return 0;
		}

		// align
		uint32 realAddress = address;
		while ( m_size[ realAddress ] == 0 && address > 0 )
		{
			realAddress -= 1;
		}

		outNumBytes = m_size[ realAddress ];
		outNumLines = 1;
		if ( m_flags[ realAddress ] & 1 ) outNumLines += 1;
		if ( m_flags[ realAddress ] & 2 ) outNumLines += 1;

		return realAddress;
	}

	virtual bool GetAddressText( const uint32 address, IMemoryLinePrinter& printer ) const
	{
		if ( address == 0 )
		{
			printer.Print( ";--------------------------------------------------------------" );
			printer.Print( "; Welcome to the ConsoleGameConverter v0.2");
			printer.Print( "; (C) 2013-2014 by Tomasz \"RexDex\" Jonarski");
			printer.Print( ";--------------------------------------------------------------" );
			printer.Print( ";" );
			printer.Print( "; 1) Load game executable, or" );
			printer.Print( "; 2) Load existing project" );
			return true;
		}

		if ( m_flags[address] & 2 )
		{
			printer.Print( ";--------------------------------------------------------------" );
		}

		if ( m_flags[address] & 1 )
		{
			printer.Print( ":BLOCK_%06X", address );
		}

		if ( m_size[address] == 1 )
		{
			printer.Print( "db %02Xh", *(const uint8*) ( m_bytes + address ) );
		}
		else if ( m_size[address] == 2 )
		{
			printer.Print( "dw %04Xh", *(const uint16*) ( m_bytes + address ) );
		}
		else if ( m_size[address] == 4 )
		{
			printer.Print( "dd %08Xh", *(const uint32*) ( m_bytes + address ) );
		}

		return false;
	}
};

CMainFrame::CMainFrame( wxWindow *parent /*= NULL*/, int id /*= -1*/ )
	: m_projectFiles( "MainFiles" )
	, m_imageFiles( "ImageFiles" )
	, m_traceFiles( "TraceFiles" )
	, m_libraryInitialized( false )
	, m_updatingSectionList( false )
	, m_memoryView( nullptr )	
	, m_logView( nullptr )
	, m_project( nullptr )
	, m_tabs( nullptr )
	, m_callstack( nullptr )
	, m_memory( nullptr )
	, m_timemachine( nullptr )
{
	wxTheFrame = this;
	wxXmlResource::Get()->LoadFrame( this, parent, wxT("MainFrame") );

	// initialize the progress bar window
	CProgressDialog::Initialize( (HWND)GetHWND() );

	// prepare layout
	m_layout.SetManagedWindow( this );

	// setup file formats for projects
	m_projectFiles.AddFormat( CFileFormat( "Project File", "px" ) );
	m_projectFiles.SetMultiselection( false );

	// setup file formats for trace
	m_traceFiles.AddFormat( CFileFormat( "Trace File", "trace" ) );
	m_traceFiles.SetMultiselection( false );

	// Organize
	m_layout.Update();
}

void CMainFrame::CreateLayout()
{
	// calculate 1/3rd of the client size
	const int paneSize = 2*(GetClientSize().x / 5);

	// Center pane ( content )
	{
		// create document tabs
		m_tabs = new wxAuiNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_DEFAULT_STYLE & ~wxAUI_NB_CLOSE_BUTTON & ~wxAUI_NB_CLOSE_ON_ACTIVE_TAB );
		m_layout.AddPane( m_tabs, wxAuiPaneInfo().Name(wxT("$Content")).Caption(wxT("Documents")).CenterPane().PaneBorder(false).CloseButton(false)  );
	}

	// Right top tabs (utils/log)
	{
		// create document tabs
		m_topTabs = new wxAuiNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_DEFAULT_STYLE & ~wxAUI_NB_CLOSE_BUTTON & ~wxAUI_NB_CLOSE_ON_ACTIVE_TAB );
		m_layout.AddPane( m_topTabs, wxAuiPaneInfo().Name(wxT("RightPaneTop")).Caption(wxT("Tools")).PaneBorder(false).Dockable(false).CloseButton(false).Right().MinSize(paneSize,400) );
	}

	// Right bottom tabs (utils/log)
	{
		// create document tabs
		m_bottomTabs = new wxAuiNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_DEFAULT_STYLE & ~wxAUI_NB_CLOSE_BUTTON & ~wxAUI_NB_CLOSE_ON_ACTIVE_TAB | wxAUI_NB_BOTTOM );
		m_layout.AddPane( m_bottomTabs, wxAuiPaneInfo().Name(wxT("RightPaneBottom")).Caption(wxT("Log & stuff")).PaneBorder(false).Dockable(false).CloseButton(false).Right().MinSize(paneSize,400) );
	}	

	// create the memory tab
	{
		m_memoryView = new CMemoryView( m_tabs );
		m_tabs->AddPage( m_memoryView, wxT("Image") );
	}	

	// Create the log window
	{
		m_logView = new CLogWindow( m_bottomTabs );
		m_bottomTabs->AddPage( m_logView, wxT("Log") );
	}

	// Create the register view window
	{
		m_regView = new CRegisterView( m_bottomTabs );
		m_topTabs->AddPage( m_regView, wxT("Registers") );
	}

	// Create the trace view window
	{
		m_traceView = new CTraceInfoView( m_bottomTabs );
		m_topTabs->AddPage( m_traceView, wxT("Trace") );
	}

	// update initial format
	UpdateUI();
	UpdateCompilationPlatformList();
	UpdateCompilationModeList();

	// update the layout once again
	m_layout.Update();
}

CMainFrame::~CMainFrame()
{
	m_layout.UnInit();

	/*if ( wxTheApp != nullptr )
	{
		wxTheApp->GetTaskSystem().RemoveEventListener( m_taskView );
	}*/

	wxTheFrame = nullptr;
}

void CMainFrame::PostLibraryInit()
{
	// enable some options that depend on the library loading
	m_libraryInitialized = true;

	// enumerate known file formats
	std::vector< platform::FileFormat > executableFormats;
	platform::Library::GetInstance().EnumerateImageFormats( executableFormats );

	// setup file formats for images
	for ( uint32 i=0; i<executableFormats.size(); ++i )
	{
		const platform::FileFormat& f = executableFormats[i];
		m_imageFiles.AddFormat( CFileFormat( f.m_name, f.m_extension ) );
	}
	m_imageFiles.SetMultiselection( false );

	// show some help screen
	m_memoryView->SetDataView( new HelpImageMemoryViewer() );
}

void CMainFrame::ToggleCommand( const unsigned int id, const bool state )
{
	wxToolBar* toolbar = XRCCTRL( *this, "ToolBar", wxToolBar );
	if ( toolbar != nullptr )
	{
		toolbar->ToggleTool( id, state );
	}

	wxMenuBar* menu = GetMenuBar();
	if ( menu != nullptr )
	{
		if ( menu->FindItem( id ) != nullptr )
		{
			menu->Check( id, state );
		}
	}
}

void CMainFrame::EnableCommand( const unsigned int id, const bool state )
{
	wxToolBar* toolbar = XRCCTRL( *this, "ToolBar", wxToolBar );
	if ( toolbar != nullptr )
	{
		toolbar->EnableTool( id, state );
	}

	wxMenuBar* menu = GetMenuBar();
	if ( menu != nullptr )
	{
		if ( menu->FindItem( id ) != nullptr )
		{
			menu->Enable( id, state );
		}
	}
}

void CMainFrame::UpdateUI()
{
	EnableCommand( XRCID("fileSave"), (m_project != nullptr) );
	EnableCommand( XRCID("fileDecompileAgain"), (m_project != nullptr) );
	EnableCommand( XRCID("editUndecorateSymbols"), CFunctionNameUndecorator::GetInstance().IsValid() );
	ToggleCommand( XRCID("editUndecorateSymbols"), wxTheApp->GetGlobalConfig().m_undecorateSymbols );
	ToggleCommand( XRCID("editShowFullPath"), wxTheApp->GetGlobalConfig().m_fullModulePath );

	EnableCommand( XRCID("codeBuild"), (m_project != nullptr) );
	EnableCommand( XRCID("codeRun"), (m_project != nullptr) );

	UpdateNavigationUI();
	UpdateTraceUI();
}

void CMainFrame::UpdateNavigationUI()
{
	EnableCommand( XRCID("navigatePrevious"), (m_project != nullptr) && m_project->GetAddressHistory().CanNavigateBack() );
	EnableCommand( XRCID("navigateNext"), (m_project != nullptr) && m_project->GetAddressHistory().CanNavigateForward() );
	EnableCommand( XRCID("navigateGoTo"), (m_project != nullptr) );
	EnableCommand( XRCID("navigateFind"), (m_project != nullptr) );
	EnableCommand( XRCID("navigateToReferenced"), (m_project != nullptr) );
}

void CMainFrame::UpdateTraceUI()
{
	const bool hasTrace = (m_project && m_project->GetTrace());
	EnableCommand( XRCID("traceRun"), hasTrace );
	EnableCommand( XRCID("traceStepBack"), hasTrace );
	EnableCommand( XRCID("traceStepOut"), hasTrace );
	EnableCommand( XRCID("traceStepIn"), hasTrace );
	EnableCommand( XRCID("traceStepOver"), hasTrace );
	EnableCommand( XRCID("traceReset"), hasTrace );
}

void CMainFrame::UpdateSectionList()
{
	wxChoice* ctrl = XRCCTRL( *this, "sectionList", wxChoice );

	// start update
	m_updatingSectionList = true;

	// begin update
	ctrl->Freeze();
	ctrl->Clear();

	// only if we have the module
	int newSectionIndex = -1;
	if ( nullptr != m_project )
	{
		// active offset
		const uint32 activeAddress = m_project->GetAddressHistory().GetCurrentAddress();

		// add sections
		const image::Binary* mainImage = m_project->GetEnv().GetImage();
		const uint32 numSection = mainImage->GetNumSections();
		for ( uint32 i=0; i<numSection; ++i )
		{
			const image::Section* section = mainImage->GetSection( i );

			// mode
			const char* mode = "";
			if ( section->CanExecute() )
			{
				mode = "+x";
			}
			else if ( section->CanWrite() )
			{
				mode = "+w";
			}

			// format section string
			char buffer[128];
			sprintf_s( buffer, sizeof(buffer), "%hs %hs (0x%X - 0x%X) %1.2fKB", 
				section->GetName().c_str(), mode,
				section->GetVirtualAddress() + mainImage->GetBaseAddress(), 
				section->GetVirtualAddress() + section->GetVirtualSize()-1 + mainImage->GetBaseAddress(),
				section->GetVirtualSize() / 1024.0f );
			ctrl->AppendString( buffer );

			// valid section for our new address ?
			if ( section->IsValidOffset( activeAddress ) )
			{
				newSectionIndex = i;
			}
		}
	}

	// no sections
	if ( ctrl->GetCount() == 0 ) 
	{
		ctrl->Enable( false );
		ctrl->AppendString( "(No sections)" );
		ctrl->SetSelection( 0 );
	}
	else
	{
		ctrl->SetSelection( newSectionIndex >= 0 ? newSectionIndex : 0 );
		ctrl->Enable( true );
	}

	// refresh
	ctrl->Thaw();
	ctrl->Refresh();

	// end update
	m_updatingSectionList = false;
}

void CMainFrame::UpdateCompilationPlatformList()
{
	wxChoice* ctrl = XRCCTRL(*this, "buildConfig", wxChoice);

	if (m_project)
	{
		ctrl->Enable();
		ctrl->Clear();

		for (int i = 0; i < (int)decoding::CompilationMode::MAX; ++i)
		{
			const auto name = decoding::GetCompilationModeName((decoding::CompilationMode)i);
			ctrl->Append(name);
		}

		ctrl->SetSelection((int)m_project->GetEnv().GetCompilationMode());
	}
	else
	{
		ctrl->Disable();
		ctrl->Clear();
		ctrl->Append("(No project)");
		ctrl->SetSelection(0);
	}
}

void CMainFrame::UpdateCompilationModeList()
{
	wxChoice* ctrl = XRCCTRL(*this, "buildCompiler", wxChoice);

	if (m_project)
	{
		ctrl->Enable();
		ctrl->Clear();

		for (int i = 0; i < (int)decoding::CompilationPlatform::MAX; ++i)
		{
			const auto name = decoding::GetCompilationPlatformName((decoding::CompilationPlatform)i);
			ctrl->Append(name);
		}

		ctrl->SetSelection((int)m_project->GetEnv().GetCompilationPlatform());
	}
	else
	{
		ctrl->Disable();
		ctrl->Clear();
		ctrl->Append("(No project)");
		ctrl->SetSelection(0);
	}
}

bool CMainFrame::EnsureSaved( bool silent, bool forceSave )
{
	if ( !m_project )
	{
		return true;
	}

	if ( forceSave || m_project->IsModified() )
	{
		// ask the user
		int ret = wxYES;
		if ( !silent )
		{
			ret = wxMessageBox( "Project is modified. Save?", "Modification detected", wxICON_QUESTION | wxYES_NO | wxCANCEL | wxYES_DEFAULT );
			if ( ret == wxCANCEL )
			{
				return false;
			}
		}

		// project not yet saved, ask for the file name
		if ( ret == wxYES )
		{
			if ( !m_project->Save( *m_logView ) )
			{
				return false;
			}
		}
	}

	// saved
	return true;
}

void CMainFrame::SetProject( Project* newProject )
{
	// set new project
	delete m_project;
	m_project = newProject;

	// refresh the UI
	UpdateUI();
	UpdateCompilationPlatformList();
	UpdateCompilationModeList();

	// create the view
	if ( newProject )
	{
		// show data
		m_memoryView->SetDataView( new CProjectMemoryView( newProject ) );

		// go to last position in the project
		const uint32 currentAddress = newProject->GetAddressHistory().GetCurrentAddress();
		m_memoryView->SetActiveOffset( currentAddress - newProject->GetEnv().GetImage()->GetBaseAddress() );
	}	
	else
	{
		m_memoryView->SetDataView( nullptr );
	}

	// reset list UI
	UpdateSectionList();
}

void CMainFrame::AddHTMLPanel( const wxString& txt, const wxString& title, const bool canClose /*= true*/ )
{
	// find existing report tab
	const size_t numPages = m_tabs->GetPageCount();
	for ( size_t i=0; i<numPages; ++i )
	{
		if ( m_tabs->GetPageText(i) == title )
		{
			wxHtmlWindow* html = static_cast< wxHtmlWindow* >( m_tabs->GetPage(i) );
			html->SetPage( txt );
			m_tabs->ChangeSelection(i);
			return;
		}
	}

	// create new report tab
	{
		wxHtmlWindow* html = new wxHtmlWindow( m_tabs );
		html->SetPage( txt );
		m_tabs->AddPage( html, title, true );
	}
}