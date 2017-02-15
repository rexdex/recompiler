#pragma once

/// generic thread safe log window
class CLogWindow : public wxWindow, public ILogOutput
{
	DECLARE_EVENT_TABLE();

private:
	enum ELogMesasgeType
	{
		eLogMesasgeType_CommandLine,

		eLogMesasgeType_Info,
		eLogMesasgeType_Warning,
		eLogMesasgeType_Error,

		eLogMesasgeType_ScriptInfo,
		eLogMesasgeType_ScriptWarning,
		eLogMesasgeType_ScriptError,

		eLogMesasgeType_MAX,
	};

	struct LogMessage
	{		
		ELogMesasgeType		m_type;
		wxLongLong			m_timestamp;
		wxString			m_module;
		wxString			m_text;

		static wxStopWatch*	st_timer;

		LogMessage( const ELogMesasgeType type, const wxString& module, const char* txt );
		~LogMessage();

		static LogMessage* Build( const ELogMesasgeType type, const char* txt );
	};

	//! Drawing style
	struct DrawingStyle
	{
		wxFont		m_font;
		wxBrush		m_backBrush[eLogMesasgeType_MAX][2];
		wxPen		m_backPen[eLogMesasgeType_MAX][2];
		wxBrush		m_textBrush[eLogMesasgeType_MAX][2];
		wxPen		m_textPen[eLogMesasgeType_MAX][2];

		uint32		m_lineHeight;
		uint32		m_moduleOffset;
		uint32		m_textOffset;

		DrawingStyle();
	};

	//! Drawing style data
	DrawingStyle		m_style;

	typedef std::vector< LogMessage* >		TMessages;
	TMessages					m_messages;

	TMessages					m_pendingMessages;
	wxCriticalSection			m_pendingMessagesLock;

	wxThreadIdType				m_mainThreadId;

	typedef std::vector< CLogWindow* >		TLogWindows;
	static TLogWindows			st_logWindows;
	static wxCriticalSection	st_logWindowsLock;

	bool						m_lockScroll;
	int							m_firstVisibleLine;

	std::vector< wxString >		m_commandLineHistory;
	int							m_commandLineHistoryIndex;

	wxString					m_commandLine;
	int							m_commandLineCursor;

	bool						m_cursorBlink;
	wxTimer						m_cursorTimer;

public:
	CLogWindow( wxWindow* parent );
	~CLogWindow();

	// clear log window
	void Clear();

	// lock/unlock automatic scrolling with new content
	void SetScrollLock( bool lockScroll );

	// refresh log windows (flush pending messages and redraw if necessary)
	static bool RefreshWindows();

private:
	// ILogOutput
	virtual bool DoLog( const char* buffer ) override;
	virtual bool DoWarn( const char* buffer ) override;
	virtual bool DoError( const char* buffer ) override;
	virtual bool DoSetTaskName( const char* buffer ) override;
	virtual bool DoSetTaskProgress( int count, int max ) override;
	virtual bool DoBeginTask( const char* buffer ) override;
	virtual bool DoEndTask()  override;
	virtual bool DoIsTaskCanceled() override;
	virtual bool DoFlush() override;

	// update the window scrolling position
	void UpdateScroll();

	// set the new first visible line
	void SetFirstVisible( int line );

	// execute the command line string
	void ExecuteCommandLine();

private:
	void OnMouseDown( wxMouseEvent& event );
	void OnMouseUp( wxMouseEvent& event );
	void OnMouseWheel( wxMouseEvent& event );
	void OnPaint( wxPaintEvent& event );
	void OnErase( wxEraseEvent& event );
	void OnKeyDown( wxKeyEvent& event );
	void OnScroll( wxScrollWinEvent& event );
	void OnSize( wxSizeEvent& event );
	void OnSetCursor( wxSetCursorEvent& event );
	void OnCursorTimer( wxTimerEvent& event );
	void OnChar( wxKeyEvent& event );
	void OnCharHook( wxKeyEvent& event );

	void OnNavigateToAddress( wxCommandEvent& event );
	void OnNavigateToTrace( wxCommandEvent& event );
};