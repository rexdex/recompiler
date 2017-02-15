#pragma once

class Project;
class CMemoryView;
class CTaskView;
class CLogWindow;
class CRegisterView;
class CTraceInfoView;
class CCallstackFrame;
class CMemoryFrame;
class CTimeMachineFrame;

class CMainFrame : public wxFrame
{
	friend class CProjectMemoryViewContextMenu;
	friend class CProjectMemoryView;

	wxDECLARE_CLASS( CMainFrame );
	DECLARE_EVENT_TABLE();

private:
	wxAuiManager		m_layout;
	wxAuiNotebook*		m_tabs;
	wxAuiNotebook*		m_topTabs;
	wxAuiNotebook*		m_bottomTabs;
	CFileDialog			m_imageFiles;
	CFileDialog			m_projectFiles;
	CFileDialog			m_traceFiles;
	bool				m_libraryInitialized;

	// loaded project
	Project*			m_project;

	// ui stuff
	bool				m_updatingSectionList;

	// project windows
	CMemoryView*		m_memoryView;
	CLogWindow*			m_logView;
	CTaskView*			m_taskView;
	CRegisterView*		m_regView;
	CTraceInfoView*		m_traceView;

	// views
	CCallstackFrame*	m_callstack;
	CTimeMachineFrame*	m_timemachine;
	CMemoryFrame*		m_memory;

	// pending URL navigation
	wxString			m_pendingNavigation;

public:
	//! Get current project
	inline Project* GetProject() const { return m_project; }

	//! Get the memory view table for current project
	inline CMemoryView* GetMemoryView() const { return m_memoryView; }

	//! Get the time machine view
	inline CTimeMachineFrame* GetTimeMachine() const { return m_timemachine; }

	//! Get the main log view
	inline CLogWindow* GetLogView() const { return m_logView; }

	//! Get the register view
	inline CRegisterView* GetRegView() const { return m_regView; }

public:
	CMainFrame( wxWindow *parent = NULL, int id = -1 );
	virtual ~CMainFrame();

	// create window layout
	void CreateLayout();

	// post library initialization
	void PostLibraryInit();

	// process pending navigation
	void ProcessPendingNavigation();
	
	// process navigation URL (internal)
	bool NavigateUrl(const wxString& url, const bool deferred=true);

	// navigate to trace entry
	bool NavigateToTraceEntry(const uint32 entry);

	// navigate to address
	bool NavigateToAddress(const uint32 address, const bool addToHistory=true);

	// add generic panel
	void AddHTMLPanel( const wxString& txt, const wxString& title, const bool canClose = true );

protected:
	void OnActivate( wxActivateEvent& event );
	void OnClose( wxCloseEvent& event );
	void OnFileOpenImage( wxCommandEvent& event );
	void OnFileOpenProject( wxCommandEvent& event );
	void OnFileSave( wxCommandEvent& event );
	void OnFileDecompile( wxCommandEvent& event );

	void OnTabClosed( wxAuiNotebookEvent& event );
	void OnTabChanged( wxAuiNotebookEvent& event );
	void OnReloadLibrary( wxCommandEvent& event );
	void OnToggleNameDecoration( wxCommandEvent& event );
	void OnToggleFullModulePath( wxCommandEvent& event );
	void OnSectionSelected( wxCommandEvent& event );
	void OnGotoAddress( wxCommandEvent& event );
	void OnGotoNext( wxCommandEvent& event );
	void OnGotoPrevious( wxCommandEvent& event );

	void OnConvertToDefault( wxCommandEvent& event );
	void OnConvertToCode( wxCommandEvent& event );

	void OnUserMacro( wxCommandEvent& event );

	void OnFindSymbol( wxCommandEvent& event );

	void OnCodeBuild( wxCommandEvent& event );
	void OnCodeRun( wxCommandEvent& event );

	void OnTraceLoad( wxCommandEvent& event );
	void OnTraceReset( wxCommandEvent& event );
	void OnTraceEnd( wxCommandEvent& event );
	void OnTraceRun( wxCommandEvent& event );
	void OnTraceStepOver( wxCommandEvent& event );
	void OnTraceStepIn( wxCommandEvent& event );
	void OnTraceStepOut( wxCommandEvent& event );
	void OnTraceStepBack( wxCommandEvent& event );

	void OnToggleBreakpoint( wxCommandEvent& event );
	void OnClearAllBreakpoints( wxCommandEvent& event );

	void OnBuildMemoryMap( wxCommandEvent& event );

	void OnOpenCallstack( wxCommandEvent& event );
	void OnOpenMemory( wxCommandEvent& event );
	void OnOpenTimeMachine( wxCommandEvent& event );

private:
	void UpdateUI();
	void UpdateNavigationUI();
	void UpdateTraceUI();
	void UpdateSectionList();
	void UpdateCompilationPlatformList();
	void UpdateCompilationModeList();

	void RefreshTrace();

	void CloseTraceRelatedWindows();

	void ToggleCommand( const unsigned int id, const bool state );
	void EnableCommand( const unsigned int id, const bool state );
	bool EnsureSaved( bool silent = false, bool forceSave = false );
	void SetProject( Project* newProject );

	void OpenTimeMachine( const int traceEntry = -1 );
};

extern CMainFrame* wxTheFrame;
