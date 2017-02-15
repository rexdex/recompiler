#include "build.h"
#include "frame.h"
#include "nameUndecorator.h"
#include "memoryView.h"
#include "logWindow.h"
#include "project.h"
#include "progressDialog.h"
#include "gotoAddressDialog.h"
#include "projectMemoryView.h"
#include "registerView.h"
#include "traceInfoView.h"
#include "findSymbolDialog.h"
#include "timeMachineFrame.h"
#include "callstackFrame.h"
#include "memoryFrame.h"
#include "runDialog.h"
#include "choiceDialog.h"
#include "utils.h"

#include "../backend/image.h"
#include "../backend/decodingEnvironment.h"
#include "../backend/platformDefinition.h"

//#include "task_LoadImage.h"
//#include "task_GenerateCode.h"

BEGIN_EVENT_TABLE( CMainFrame, wxFrame )
	EVT_ACTIVATE( CMainFrame::OnActivate )
	EVT_MENU( XRCID("fileOpenImage"), CMainFrame::OnFileOpenImage )
	EVT_MENU( XRCID("fileOpenProject"), CMainFrame::OnFileOpenProject )
	EVT_MENU( XRCID("fileSave"), CMainFrame::OnFileSave )
	EVT_MENU( XRCID("fileDecompileAgain"), CMainFrame::OnFileDecompile )
	EVT_MENU( XRCID("editUndecorateSymbols"), CMainFrame::OnToggleNameDecoration )
	EVT_MENU( XRCID("editShowFullPath"), CMainFrame::OnToggleFullModulePath )
	EVT_MENU( XRCID("navigateNext"), CMainFrame::OnGotoNext )
	EVT_MENU( XRCID("navigatePrevious"), CMainFrame::OnGotoPrevious )
	EVT_MENU( XRCID("navigateGoTo"), CMainFrame::OnGotoAddress )
	EVT_MENU( XRCID("navigateFind"), CMainFrame::OnFindSymbol )
	EVT_CHOICE( XRCID("sectionList"), CMainFrame::OnSectionSelected )
	EVT_MENU( XRCID("convertToCode"), CMainFrame::OnConvertToCode )
	EVT_MENU( XRCID("convertToDefault"), CMainFrame::OnConvertToDefault )
	EVT_MENU( XRCID("reloadLibrary"), CMainFrame::OnReloadLibrary )
	EVT_MENU( XRCID("codeBuild"), CMainFrame::OnCodeBuild )
	EVT_MENU( XRCID("codeRun"), CMainFrame::OnCodeRun )
	EVT_MENU( XRCID("traceLoadFile"), CMainFrame::OnTraceLoad )
	EVT_MENU( XRCID("traceRun"), CMainFrame::OnTraceRun )
	EVT_MENU( XRCID("traceReset"), CMainFrame::OnTraceReset )	
	EVT_MENU( XRCID("traceEnd"), CMainFrame::OnTraceEnd )	
	EVT_MENU( XRCID("traceStepOver"), CMainFrame::OnTraceStepOver )
	EVT_MENU( XRCID("traceStepIn"), CMainFrame::OnTraceStepIn )
	EVT_MENU( XRCID("traceStepOut"), CMainFrame::OnTraceStepOut )
	EVT_MENU( XRCID("traceStepBack"), CMainFrame::OnTraceStepBack )
	EVT_MENU( XRCID("breakpointToggle"), CMainFrame::OnToggleBreakpoint )
	EVT_MENU( XRCID("breakpointClearAll"), CMainFrame::OnClearAllBreakpoints )	
	EVT_MENU( XRCID("openCallstack"), CMainFrame::OnOpenCallstack )		
	EVT_MENU( XRCID("openTimeMachine"), CMainFrame::OnOpenTimeMachine )		
	EVT_MENU( XRCID("openMemory"), CMainFrame::OnOpenMemory )		
	EVT_CLOSE( CMainFrame::OnClose )
END_EVENT_TABLE()
	
void CMainFrame::OnActivate( wxActivateEvent& event )
{
	event.Skip();
}

void CMainFrame::OnClose( wxCloseEvent& event )
{
	if ( !EnsureSaved() )
	{
		event.Veto();
		return;
	}

	event.Skip();
}

void CMainFrame::OnFileSave( wxCommandEvent& event )
{
	EnsureSaved( true, true );
}

void CMainFrame::OnFileDecompile( wxCommandEvent& event )
{
	if ( m_project )
		m_project->GetEnv().GetPlatform()->DecodeImage( *m_logView, *m_project->GetEnv().GetDecodingContext() );
}

void CMainFrame::OnFileOpenImage( wxCommandEvent& event )
{
	if ( !EnsureSaved() )
		return;

	// ask for file name
	if ( !m_imageFiles.DoOpen( this ) )
		return;

	// get input file name
	const std::wstring imagePath = m_imageFiles.GetFile().wc_str();
	const wxString imageShortPath = wxString(imagePath.c_str()).BeforeLast('.').AfterLast('\\').Lower();

	// ask for project path
	std::wstring saveFilePath;
	if ( !m_projectFiles.DoSave( this, imageShortPath ) )
		return;

	// create the loading task
	const std::wstring projectPath = m_projectFiles.GetFile().wc_str();	

	// load the image
	Project* newProject = Project::LoadImageFile(*m_logView, imagePath, projectPath);
	if (!newProject)
	{
		wxMessageBox( wxT("Failed to import selected image. Check log for details."), wxT("Error"), wxOK | wxICON_ERROR, this );
		return;
	}

	// set new project
	SetProject(newProject);
}

void CMainFrame::OnFileOpenProject( wxCommandEvent& event )
{
	if ( !EnsureSaved() )
		return;

	// ask for file name
	if ( m_projectFiles.DoOpen( this ) )
	{
		// get file extension
		const std::wstring filePath = m_projectFiles.GetFile().wc_str();
		Project* loadedProject = Project::LoadProject(*m_logView, filePath);
		if ( nullptr != loadedProject )
		{
			SetProject( loadedProject );
		}
	}
}

void CMainFrame::OnTabClosed( wxAuiNotebookEvent& event )
{
}

void CMainFrame::OnTabChanged( wxAuiNotebookEvent& event )
{
}

void CMainFrame::OnToggleNameDecoration( wxCommandEvent& event )
{
	wxTheApp->GetGlobalConfig().m_undecorateSymbols = !wxTheApp->GetGlobalConfig().m_undecorateSymbols;
	UpdateUI();
}

void CMainFrame::OnToggleFullModulePath( wxCommandEvent& event )
{
	wxTheApp->GetGlobalConfig().m_fullModulePath = !wxTheApp->GetGlobalConfig().m_fullModulePath;
	UpdateUI();
}

void CMainFrame::OnSectionSelected( wxCommandEvent& event )
{
	if ( m_updatingSectionList )
	{
		return;
	}

	// get selection id
	wxChoice* ctrl = XRCCTRL( *this, "sectionList", wxChoice );
	const int selectedSectionID = ctrl->GetSelection();

	// get the section
	if ( m_project != nullptr && selectedSectionID != -1 )
	{
		const image::Section* section = m_project->GetEnv().GetImage()->GetSection( selectedSectionID );
		m_memoryView->SetActiveOffset( section->GetVirtualAddress(), true );
		UpdateNavigationUI();
	}
}

void CMainFrame::OnGotoAddress( wxCommandEvent& event )
{
	if ( m_project != nullptr )
	{
		CGotoAddressDialog dlg( this, m_project, m_memoryView );
		const uint32 newAddres = dlg.GetNewAddress();
		if ( newAddres != CGotoAddressDialog::INVALID_ADDRESS )
		{
			// go to
			const uint32 newOffset = newAddres - m_project->GetEnv().GetImage()->GetBaseAddress();
			m_memoryView->SetActiveOffset( newOffset, true );

			// refresh buttons
			UpdateNavigationUI();
		}
	}
}

void CMainFrame::OnGotoNext( wxCommandEvent& event )
{
	if ( m_project != nullptr )
	{
		const uint32 newAddress = m_project->GetAddressHistory().NavigateForward();
		if ( newAddress != CGotoAddressDialog::INVALID_ADDRESS )
		{
			const uint32 newOffset = newAddress - m_project->GetEnv().GetImage()->GetBaseAddress();
			m_memoryView->SetActiveOffset( newOffset, false );

			// refresh buttons
			UpdateNavigationUI();
		}
	}
}

void CMainFrame::OnGotoPrevious( wxCommandEvent& event )
{
	if ( m_project != nullptr )
	{
		const uint32 newAddress = m_project->GetAddressHistory().NavigateBack();
		if ( newAddress != CGotoAddressDialog::INVALID_ADDRESS )
		{
			const uint32 newOffset = newAddress - m_project->GetEnv().GetImage()->GetBaseAddress();
			m_memoryView->SetActiveOffset( newOffset, false );

			// refresh buttons
			UpdateNavigationUI();
		}
	}
}

void CMainFrame::OnConvertToDefault( wxCommandEvent& event )
{
}

void CMainFrame::OnConvertToCode( wxCommandEvent& event )
{
	// get the selection of memory ranges
	uint32 startRVA=0, endRVA=0;
	if ( m_memoryView->GetSelectionAddresses( startRVA, endRVA ) )
	{
		m_project->DecodeInstructionsFromMemory( *m_logView, startRVA, endRVA );
	}
}

void CMainFrame::OnUserMacro( wxCommandEvent& event )
{
}

void CMainFrame::OnReloadLibrary( wxCommandEvent& event )
{
	// invalid stuff
	if ( m_project != nullptr )
	{
		m_project->ClearCachedData();
	}

	// redraw
	m_memoryView->ClearCachedData();
	m_memoryView->Refresh();
}

static const wxString GetComboString(const wxChoice* configBox)
{
	if (!configBox || !configBox->IsEnabled())
		return "";

	const int option = configBox->GetSelection();
	if (option == -1)
		return "";

	return configBox->GetString(option);
}

void CMainFrame::OnCodeBuild( wxCommandEvent& event )
{
	// no project
	if ( !m_project )
		return;

	// update options
	wxChoice* ctrlMode = XRCCTRL(*this, "buildConfig", wxChoice);
	m_project->GetEnv().SetCompilationMode((decoding::CompilationMode)ctrlMode->GetSelection());

	wxChoice* ctrlPlatform = XRCCTRL(*this, "buildCompiler", wxChoice);
	m_project->GetEnv().SetCompilationPlatform((decoding::CompilationPlatform)ctrlPlatform->GetSelection());

	if (!m_project->GetEnv().CompileCode( *m_logView ) )
	{
		wxMessageBox( wxT("Code compilation failed!"), wxT("Error"), wxOK | wxICON_ERROR, this );
		return;
	}
}

/// log capturer
class CLogCapture : public ILogOutput
{
public:
	CLogCapture( wxString& str, std::vector< std::string >* traceFiles );
	~CLogCapture();

	virtual bool DoLog( const char* buffer ) override final;
	virtual bool DoWarn( const char* buffer ) override final;
	virtual bool DoError( const char* buffer ) override final;
	virtual bool DoSetTaskName( const char* buffer ) override final { return true; };
	virtual bool DoSetTaskProgress( int count, int max ) override final { return true; };
	virtual bool DoIsTaskCanceled() override final { return false; };
	virtual bool DoBeginTask( const char* buffer ) override final { return true; };
	virtual bool DoEndTask() override final { return true; };
	virtual bool DoFlush() override final { return true; };

private:
	CHTMLBuilder					m_html;
	std::vector< std::string >*		m_traceFiles;
};

CLogCapture::CLogCapture( wxString& str, std::vector< std::string >* traceFiles )
	: m_html( str )
	, m_traceFiles( traceFiles )
{
	m_html.Open("html");
	m_html.Open("body");
	m_html.Attr("bgcolor", "#282828");

	m_html.Open("font");
	m_html.Attr("face", "Courier New");
	m_html.Attr("size", "8px");
	m_html.Attr("color", "#EEEEEE");
}

CLogCapture::~CLogCapture()
{
	m_html.Close(); // font
	m_html.Close(); // body
	m_html.Close(); // html
}

bool CLogCapture::DoLog( const char* buffer )
{
	if ( strncmp( buffer, "Run: ", 5 ) == 0 )
		buffer += 5;

	if ( strncmp( buffer, "Warning: ", 9 ) == 0 )
	{
		buffer += 9;
		return DoWarn( buffer );
	}
	else if ( strncmp( buffer, "Error: ", 7 ) == 0 )
	{
		buffer += 7;
		return DoError( buffer );
	}
	else if ( m_traceFiles && (strncmp( buffer, "TraceLog: ", 10 ) == 0) )
	{
		const char* fileName = buffer + strlen( "TraceLog: Trace log '");

		char temp[512];
		strcpy_s( temp, fileName );

		char* endName = strrchr( temp, '\'' );
		if ( endName != nullptr )
		{
			*endName = 0;
			m_traceFiles->push_back( temp );
		}

		return true;
	}

	m_html.Print( buffer );
	m_html.Print( "<br>" );
	return true;
}

bool CLogCapture::DoWarn( const char* buffer )
{
	if ( strncmp( buffer, "Run: ", 5 ) == 0 )
		buffer += 5;

	m_html.Open("font");
	m_html.Attr("color", "#FFFF88");

	m_html.Print( buffer );
	m_html.Print( "<br>" );

	m_html.Close(); // font
	return true;
}

bool CLogCapture::DoError( const char* buffer )
{
	if ( strncmp( buffer, "Run: ", 5 ) == 0 )
		buffer += 5;

	m_html.Open("font");
	m_html.Attr("color", "#FF8888");

	m_html.Print( buffer );
	m_html.Print( "<br>" );

	m_html.Close(); // font
	return true;
}

void CMainFrame::OnCodeRun( wxCommandEvent& event )
{
/*	// no project
	if ( !m_project )
		return;

	// get the code profile to use
	const wxString codeProfileName = GetComboString(XRCCTRL(*this, "buildConfig", wxChoice));
	const platform::CodeProfile* codeProfile = m_project->FindCodeProfile( codeProfileName.c_str().AsChar() );
	if ( !codeProfile )
		return;

	// get the compiler to use
	const wxString compilerName = GetComboString(XRCCTRL(*this, "buildCompiler", wxChoice));
	const platform::CompilerDefinition* compiler = m_project->FindCompiler( compilerName.c_str().AsChar() );
	if ( !compiler )
		return;

	// show the run dialog
	CRunDialog dlg( this );
	if ( !dlg.ShowDialog( m_project->GetConfig() ) )
		return;

	// setup config
	decoding::EnvironmentRunContext runContext;
	runContext.m_dvdDirectory = m_project->GetConfig().m_dvdDir;
	runContext.m_devkitDirectory = m_project->GetConfig().m_devkitDir;
	runContext.m_verboseLog = m_project->GetConfig().m_verboseLog;

	// setup trace
	if ( m_project->GetConfig().m_trace )
	{
		runContext.m_traceInstructions = m_project->GetConfig().m_trace;

		std::wstring traceDirectory = decoding::Environment::GetInstance().GetProjectDir();
		traceDirectory += L"bin";
		CreateDirectoryW( traceDirectory.c_str(), NULL );
		traceDirectory += L"\\trace";
		CreateDirectoryW( traceDirectory.c_str(), NULL );

		runContext.m_traceDirectory = traceDirectory;
	}

	// custom log window
	wxString appLogTxt;
	std::vector< std::string > traceFiles;
	ILogOutput* appLog = m_logView;
	if ( m_project->GetConfig().m_showLog )
	{
		appLog = new CLogCapture( appLogTxt, runContext.m_traceInstructions ? &traceFiles : nullptr );
	}

	// run the code
	if (!decoding::Environment::GetInstance().RunCode( *m_logView, *appLog, codeProfile, compiler, runContext ) )
	{
		wxMessageBox( wxT("There were errors running application!"), wxT("Error"), wxOK | wxICON_ERROR, this );
		return;
	}

	// show the captured log in separate window
	if ( appLog != m_logView )
	{
		wxTheFrame->AddHTMLPanel( appLogTxt, "VM Log", true );
		delete appLog;
	}

	// and trace files created ?
	if ( m_project->GetConfig().m_trace )
	{
		if ( traceFiles.empty() )
		{
			wxMessageBox( wxT("No trace files were generated by application!"), wxT("Error"), wxOK | wxICON_ERROR, this );
		}
		else
		{
			m_logView->Log( "Trace: Created %d trace files", traceFiles.size() );

			CChoiceDialog choiceDlg( this );
			std::string traceFileName;
			if ( choiceDlg.ShowDialog( "Load trace", "Select trace to load:", traceFiles, traceFileName ) )
			{
				std::wstring fullTracePath = runContext.m_traceDirectory;
				fullTracePath += L"\\";
				fullTracePath += std::wstring( traceFileName.begin(), traceFileName.end() );

				// close all windows related to trace
				CloseTraceRelatedWindows();

				// load the trace
				if (m_project->LoadTrace(*m_logView, fullTracePath ))
				{
					// build histograms
					m_project->BuildHistogram();
					m_memoryView->SetHitcountDisplay(true);

					// update the reg view
					m_regView->InitializeRegisters(m_project->GetTrace()->GetRegisters());

					// refresh the UI
					RefreshTrace();

					// move to the start
					NavigateUrl("resettrace");

					// show the main window again
					m_tabs->SetSelection(0);
				}
				else
				{
					wxMessageBox( wxT("Failed to load generated trace file!"), wxT("Error"), wxOK | wxICON_ERROR, this );
				}
			}
		}
	}*/
}

void CMainFrame::CloseTraceRelatedWindows()
{
	// close callstack window
	if ( m_callstack )
	{
		delete m_callstack;
		m_callstack = nullptr;
	}

	// close timemachine window
	if ( m_timemachine )
	{
		delete m_timemachine;
		m_timemachine = nullptr;
	}

	// close memory window
	if ( m_memory )
	{
		delete m_memory;
		m_memory = nullptr;
	}
}

void CMainFrame::RefreshTrace()
{
	// refresh the trace UI
	UpdateTraceUI();

	// get the first trace address
	const uint32 traceAddress = m_project->GetTrace()->GetCurrentAddress();
	const uint32 offset = traceAddress - m_project->GetEnv().GetImage()->GetBaseAddress();
	m_memoryView->SetHighlightedOffset(offset);
	m_memoryView->SetActiveOffset(offset, true);

	// refresh registers
	m_regView->UpdateRegisters(m_project->GetTrace()->GetCurrentFrame(), &m_project->GetTrace()->GetNextFrame() );
	m_traceView->UpdateInfo(m_project->GetTrace());
}

void CMainFrame::OnTraceLoad( wxCommandEvent& event )
{
	if (!m_project)
		return;

	if (!m_traceFiles.DoOpen(this))
		return;

	// close all windows related to trace
	CloseTraceRelatedWindows();

	const wxString fileName = m_traceFiles.GetFile();
	if (m_project->LoadTrace(*m_logView, fileName.wc_str()))
	{
		// build histograms
		m_project->BuildHistogram();
		m_memoryView->SetHitcountDisplay(true);

		// update the reg view
		m_regView->InitializeRegisters(m_project->GetTrace()->GetRegisters());

		// refresh the UI
		RefreshTrace();

		// move to the end
		NavigateUrl("endtrace");
	}
}

void CMainFrame::OnTraceReset( wxCommandEvent& event )
{
	NavigateUrl("resettrace");
}

void CMainFrame::OnTraceEnd( wxCommandEvent& event )
{
	NavigateUrl("endtrace");
}

void CMainFrame::OnTraceRun( wxCommandEvent& event )
{
	NavigateUrl("runtrace");
}

void CMainFrame::OnTraceStepOver( wxCommandEvent& event )
{
	NavigateUrl("stepover");
}

void CMainFrame::OnTraceStepIn( wxCommandEvent& event )
{
	NavigateUrl("stepinto");
}

void CMainFrame::OnTraceStepOut( wxCommandEvent& event )
{
	NavigateUrl("stepout");
}

void CMainFrame::OnTraceStepBack( wxCommandEvent& event )
{
	NavigateUrl("stepback");
}

void CMainFrame::OnToggleBreakpoint( wxCommandEvent& event )
{
	if (m_project)
	{
		const uint32 currentAddress = m_memoryView->GetCurrentRVA();
		const bool hasBreakpoint = m_project->HasBreakpoint( currentAddress );
		m_project->SetBreakpoint( currentAddress, !hasBreakpoint );
		m_memoryView->Refresh();
	}
}

void CMainFrame::OnClearAllBreakpoints( wxCommandEvent& event )
{
	if (m_project)
	{
		m_project->ClearAllBreakpoints();
		m_memoryView->Refresh();
	}
}

void CMainFrame::OnFindSymbol( wxCommandEvent& event )
{
	CFindSymbolDialog dlg( m_project->GetEnv(), this );
	if (0 == dlg.ShowModal())
	{
		const uint32 selectedAddress = dlg.GetSelectedSymbolAddress();
		if (selectedAddress)
		{
			m_logView->Log("Symbol: Navigating to symbol '%s', addrses %06Xh", dlg.GetSelectedSymbolName(), selectedAddress);
			NavigateToAddress(selectedAddress);
		}
	}
}

void CMainFrame::OnBuildMemoryMap( wxCommandEvent& event )
{
	if (!m_project) 
		return;

	m_project->BuildMemoryMap();
}

void CMainFrame::OnOpenCallstack( wxCommandEvent& event )
{
	if ( !m_callstack )
	{
		m_callstack = new CCallstackFrame( this );
	}

	m_callstack->Show();
	m_callstack->SetFocus();
}

void CMainFrame::OnOpenMemory( wxCommandEvent& event )
{
	if ( !m_memory )
	{
		m_memory = new CMemoryFrame( this );
	}

	m_memory->Show();
	m_memory->SetFocus();
}

void CMainFrame::OnOpenTimeMachine( wxCommandEvent& event )
{
	OpenTimeMachine(-1);
}

void CMainFrame::OpenTimeMachine( const int traceEntry )
{
	if ( !m_timemachine )
	{
		m_timemachine = new CTimeMachineFrame( this );
	}

	m_timemachine->Show();
	m_timemachine->SetFocus();

	if ( traceEntry >= 0 )
	{
		m_timemachine->CreateNewTrace( traceEntry );
	}
}
