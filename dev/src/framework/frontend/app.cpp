#include "build.h"
#include "events.h"
#include "frame.h"

#include "../backend/platformLibrary.h"

IMPLEMENT_APP( CToolsApp );

//---------------------------------------------------------------------------

CGlobalConfig::CGlobalConfig()
	: CAutoConfig( "GlobalConfig" )
	, m_undecorateSymbols( false )
	, m_fullModulePath( false )
{
	AddConfigValue( m_undecorateSymbols, "UndecorateSymbols" );
	AddConfigValue( m_fullModulePath, "FullModulePath" );
}

//---------------------------------------------------------------------------

ISyncUpdate::ISyncUpdate()
{
	if ( wxTheApp != nullptr )
	{
		wxTheApp->RegisterSyncUpdate( this );
	}
}

ISyncUpdate::~ISyncUpdate()
{
	if ( wxTheApp != nullptr )
	{
		wxTheApp->UnregisterSyncUpdate( this );
	}
}

//---------------------------------------------------------------------------

CToolsApp::CToolsApp()
	: m_events( nullptr )
{
	// setup debugging symbol options
	SymSetOptions( SYMOPT_UNDNAME );

	// get executable path and build the system paths
	{
		// get executable path
		wchar_t tempPath[ _MAX_PATH ];
		GetModuleFileNameW( GetModuleHandle( NULL ), tempPath, _MAX_PATH );

		// remove the ending file name
		wchar_t* separator = wcsrchr( tempPath, L'\\' );
		if ( !separator ) separator = wcsrchr( tempPath, L'/' );
		*separator = 0;

		// binary path
		m_binaryPath = tempPath;
		m_binaryPath += L"\\";

		// remove the "Config.Platform" directory
		separator = wcsrchr( tempPath, L'\\' );
		if ( !separator ) separator = wcsrchr( tempPath, L'/' );
		*separator = 0;

		// remove the "bin" directory
		separator = wcsrchr( tempPath, L'\\' );
		if ( !separator ) separator = wcsrchr( tempPath, L'/' );
		*separator = 0;

		// create the editor path
		m_editorPath = tempPath;
		m_editorPath += "\\data\\ui\\";
	}

	// create event system
	m_events = new CEventDispatcher();
}

CToolsApp::~CToolsApp()
{
	// cleanup event system
	if ( m_events )
	{
		delete m_events;
		m_events = nullptr;
	}
}

void CToolsApp::RegisterSyncUpdate( ISyncUpdate* syncUpdate )
{
	m_syncUpdate.push_back( syncUpdate );
}

void CToolsApp::UnregisterSyncUpdate( ISyncUpdate* syncUpdate )
{
	TSyncUpdates::iterator it = std::find( m_syncUpdate.begin(), m_syncUpdate.end(), syncUpdate );
	if ( it != m_syncUpdate.end() )
	{
		m_syncUpdate.erase( it );
	}
}

/*bool CToolsApp::OnInitGui()
{
	return true;
}*/

void CToolsApp::OnIdle(wxIdleEvent& event)
{
	bool updateNeeded = false;

	// Task system
	{
		// update and redraw the log
		updateNeeded |= CLogWindow::RefreshWindows();

		// update application
		wxApp::OnIdle(event);
	}

	// Sync updates
	{
		TSyncUpdates copy = m_syncUpdate;
		for ( uint32 i=0; i<copy.size(); ++i )
		{
			copy[i]->OnSyncUpdate();
		}
 	}

	// Naviation
	if (wxTheFrame)
	{
		wxTheFrame->ProcessPendingNavigation();
	}

	// request more events if we still have something to process
	if ( updateNeeded )
	{
		event.RequestMore();
	}
}

bool CToolsApp::OnInit()
{
	//	LoadLibraryA( "comctl32.dll" );
	//InitCommonControls();

	// app name
	SetVendorName( wxT( "Recompiler" ) );
	SetAppName( wxT( "Recompiler v1.0" ) ); 

	/// initialize GDI+
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	Gdiplus::GdiplusStartup( &m_gdiplusToken, &gdiplusStartupInput, NULL);

	// initialize XML resources
	wxInitAllImageHandlers();
	wxXmlResource::Get()->InitAllHandlers();

	// Initialize XRC resources
	wxXmlResource::Get()->LoadAllFiles( m_editorPath );

	// Create main frame
	wxTheFrame = new CMainFrame();
	wxTheFrame->Hide();
	SetTopWindow( wxTheFrame );

	// Intialize GUI
	//wxAppBase::OnInitGui();

	// Set icon
	wxTheFrame->SetIcon( wxICON( amain ) );
	wxTheFrame->Maximize();
	wxTheFrame->CreateLayout();
	wxTheFrame->Show();

	// initialize library
	if ( !platform::Library::GetInstance().Initialize( *wxTheFrame->GetLogView(), m_binaryPath.wc_str() ) )
	{
		wxMessageBox( wxT("Failed to load data library"), wxT("Error initializing application"), wxOK | wxICON_ERROR );
		return false;	
	}

	// load all config sections registered so far
	m_config.LoadConfig();

	// update main window
	wxTheFrame->PostLibraryInit();

	// connect the OnIdle
	Connect( wxEVT_IDLE, wxIdleEventHandler( CToolsApp::OnIdle ), NULL, this );

	// Initialized
	return true;
}

int CToolsApp::OnExit()
{
	// close GDI+
	Gdiplus::GdiplusShutdown( m_gdiplusToken );

	// defered cleanup
	DeletePendingObjects();

	// close library
	platform::Library::GetInstance().Close();

	// close application
	return wxApp::OnExit();
}

void CToolsApp::NavigateToAddress( const uint32 address )
{
	wxTheFrame->NavigateToAddress(address);
}