#pragma once

class CProgressDialog
{
public:
	CProgressDialog(HWND hParent);
	~CProgressDialog();

	static CProgressDialog& Initialize(HWND hTopFrameWnd);
	static CProgressDialog& GetInstance();

	bool IsTaskCanceled() const;

	void StartTask(const char* taskName);
	void EndTask();

	void UpdateTaskText(const char* text);
	void UpdateTaskTextf(const char* text, ...);
	void UpdateTaskProgress(const int progress);

private:
	HWND	m_hParent;
	HWND	m_hWnd;

	HWND	m_hItemText;
	HWND	m_hItemBar;
	HWND	m_hItemTime;
	HWND	m_hItemCancelButton;

	DWORD	m_threadId;
	HANDLE	m_hThread;

	volatile LONG		m_taskCount;

	CRITICAL_SECTION	m_lock;
	char				m_taskTitle[512];
	char				m_taskText[512];
	int					m_taskProgress;

	bool	m_isActive;
	bool	m_isCanceled;

	const static uint32 WM_PROGRESS_SHOW = WM_USER + 1;
	const static uint32 WM_PROGRESS_HIDE = WM_USER + 2;
	const static uint32 WM_PROGRESS_UPDATE_BAR = WM_USER + 3;
	const static uint32 WM_PROGRESS_UPDATE_TEXT = WM_USER + 4;

	static CProgressDialog* s_instance;
	static DWORD WINAPI ThreadFunc(LPVOID param);
	static INT_PTR CALLBACK DialogFunc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

/*

/// Dialog to show when waiting for task group (or groups) to be finishes
class CProgressDialog : public wxDialog
{
	DECLARE_EVENT_TABLE();

private:
	// the task groups we are waiting
	typedef std::vector< CTaskGroup* >		TTaskGroups;
	TTaskGroups			m_taskGroups;

	// last progress data displayed
	std::string			m_lastProgressText;
	float				m_lastProgressValue;
	int					m_lastTimeSeconds;

	// time since start
	wxStopWatch			m_timer;

	// update timer
	wxTimer				m_updateTimer;

	// holds the active text and active progress value
	static std::string			st_currentText;
	static float				st_currentProgress;
	static wxCriticalSection	st_mutex;	

public:
	CProgressDialog( wxWindow* parent );
	~CProgressDialog();

	// add single task (creates a group)
	void AddTask( CTask* task );

	// add task group
	void AddTaskGroup( CTaskGroup* group );

	// wait for the result, returns true if all groups finished without problems
	bool WaitForTasks();

public:
	// update active progress
	static void UpdateProgress( const int cur, const int max );

	// update active text
	static void UpdateStatus( const char* txt, ... );

private:
	void OnTimer( wxTimerEvent& event );
	void OnCancel( wxCommandEvent& event );

private:
	// Update progress text and bar
	void UpdateProgress();

	// Did we finished ?
	bool AllTasksFinished() const;
};*/