#include "build.h"
#include "progressDialog.h"
#include "resource.h"

CProgressDialog* CProgressDialog::s_instance = NULL;

CProgressDialog::CProgressDialog(HWND hParent)
	: m_hParent(hParent)
	, m_hWnd(NULL)
	, m_hItemText(NULL)
	, m_hItemBar(NULL)
	, m_hItemTime(NULL)
	, m_hThread(NULL)
	, m_taskProgress(0)
	, m_taskCount(0)
	, m_isCanceled(false)
{
	// create critical section
	InitializeCriticalSection( &m_lock );

	// reset
	memset(m_taskText, 0, sizeof(m_taskText));
	memset(m_taskTitle, 0, sizeof(m_taskTitle));

	// create dialog thread
	m_hThread = CreateThread(NULL, 64 << 10, &ThreadFunc, this, 0, &m_threadId);
}

CProgressDialog::~CProgressDialog()
{
	// release critical section
	DeleteCriticalSection( &m_lock );
}

CProgressDialog& CProgressDialog::Initialize(HWND hTopFrameWnd)
{
	if (!s_instance)
	{
		s_instance = new CProgressDialog(hTopFrameWnd);
	}
	return *s_instance;
}

CProgressDialog& CProgressDialog::GetInstance()
{
	return *s_instance;
}

bool CProgressDialog::IsTaskCanceled() const
{
	return m_isCanceled;
}

void CProgressDialog::StartTask(const char* taskName)
{
	// reset cancellation flag
	m_isCanceled = false;

	// update text
	{
		EnterCriticalSection(&m_lock);
		strcpy_s(m_taskTitle, taskName);
		LeaveCriticalSection(&m_lock);
	}

	// update window
	const long taskCount = InterlockedIncrement(&m_taskCount);
	if (taskCount == 1 && m_hWnd)
	{
		//SendMessage(m_hWnd, WM_PROGRESS_SHOW, 0, 0);
		PostThreadMessage(m_threadId, WM_PROGRESS_SHOW, 0, 0);
	}
}

void CProgressDialog::EndTask()
{
	// update window
	const long taskCount = InterlockedDecrement(&m_taskCount);
	if (taskCount == 0 && m_hWnd)
	{
		//SendMessage(m_hWnd, WM_PROGRESS_HIDE, 0, 0);
		PostThreadMessage(m_threadId, WM_PROGRESS_HIDE, 0, 0);
	}
}

void CProgressDialog::UpdateTaskText(const char* text)
{
	if (m_taskCount && m_hWnd)
	{
		// update text
		{
			EnterCriticalSection(&m_lock);
			strcpy_s(m_taskText, text);
			LeaveCriticalSection(&m_lock);
		}

		// refresh window
		//PostThreadMessage(m_threadId, WM_PROGRESS_UPDATE_TEXT, 0, 0);
		PostMessage(m_hWnd, WM_PROGRESS_UPDATE_TEXT, 0, 0);
	}
}

void CProgressDialog::UpdateTaskTextf(const char* text, ...)
{
	char buffer[ 512 ];
	va_list args;

	va_start( args, text );
	vsnprintf_s( buffer, ARRAYSIZE(buffer), text, args );
	va_end( args );

	UpdateTaskText( buffer );
}

void CProgressDialog::UpdateTaskProgress(const int progress)
{
	if (m_taskCount && m_hWnd && m_taskProgress != progress)
	{
		// update text
		{
			EnterCriticalSection(&m_lock);
			m_taskProgress = progress;
			LeaveCriticalSection(&m_lock);
		}

		// refresh window
		//PostThreadMessage(m_threadId, WM_PROGRESS_UPDATE_BAR, 0, 0);
		PostMessage(m_hWnd, WM_PROGRESS_UPDATE_BAR, 0, 0);
	}
}

DWORD WINAPI CProgressDialog::ThreadFunc(LPVOID param)
{
	CProgressDialog* dlg = (CProgressDialog*) param;

	// create dialog
	HWND hWnd = CreateDialogParamA(NULL, MAKEINTRESOURCEA(IDD_PROGRESS_DIALOG), NULL, &DialogFunc, (LPARAM)dlg);
	dlg->m_hWnd = hWnd;

	// message loop
	MSG msg;
	for (;;)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			if (GetMessage(&msg, NULL, 0, 0) <= 0)
				break;

			if (msg.hwnd == NULL && msg.message == WM_PROGRESS_SHOW)
			{
				{
					EnterCriticalSection( &dlg->m_lock );
					SetWindowTextA(hWnd, dlg->m_taskTitle);
					LeaveCriticalSection( &dlg->m_lock );
				}

				EnableWindow( dlg->m_hItemCancelButton, true );

				ShowWindow(hWnd, SW_SHOW);
				UpdateWindow(hWnd);
				continue;
			}
			else if (msg.hwnd == NULL && msg.message == WM_PROGRESS_HIDE)
			{
				ShowWindow(hWnd, SW_HIDE);
				UpdateWindow(hWnd);
				continue;
			}
			/*else if (msg.hwnd == NULL && msg.message == WM_PROGRESS_UPDATE_BAR)
			{
				PostMessage(hWnd, WM_PROGRESS_UPDATE_BAR, 0, 0);
				continue;
			}
			else if (msg.hwnd == NULL && msg.message == WM_PROGRESS_UPDATE_TEXT)
			{
				PostMessage(hWnd, WM_PROGRESS_UPDATE_TEXT, 0, 0);
				continue;
			}*/

			if (!IsDialogMessage(hWnd, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		//OutputDebugStringA("Loop!\n");
		///Sleep(50);
	}

	// done
	OutputDebugStringA("ProgressDlg thread exit!\n");
	return 0;
}

INT_PTR CALLBACK CProgressDialog::DialogFunc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_INITDIALOG:
		{
			CProgressDialog* dlg = (CProgressDialog*) lParam;
			SetWindowLongPtr( hWnd, GWLP_USERDATA, (LONG_PTR) dlg );

			dlg->m_hItemBar = GetDlgItem(hWnd, IDC_PROGRESS_BAR);
			dlg->m_hItemText = GetDlgItem(hWnd, IDC_PROGRESS_TEXT);
			dlg->m_hItemTime = GetDlgItem(hWnd, IDC_PROGRESS_TIME);
			dlg->m_hItemCancelButton = GetDlgItem(hWnd, IDC_CANCEL_BUTTON);

			if (dlg->m_hItemBar)
			{
				SendMessage(dlg->m_hItemBar, PBM_SETRANGE, 0, MAKELPARAM(0,100));
				SendMessage(dlg->m_hItemBar, PBM_SETPOS, 0, 0);
			}

			return 0;
		}

		case WM_PROGRESS_UPDATE_TEXT:
		{
			CProgressDialog* dlg = (CProgressDialog*) GetWindowLongPtr( hWnd, GWLP_USERDATA );
			if (dlg->m_hItemText)
			{
				EnterCriticalSection( &dlg->m_lock );
				SetWindowTextA(dlg->m_hItemText, dlg->m_taskText);
				UpdateWindow(dlg->m_hItemText);
				UpdateWindow(dlg->m_hWnd);
				LeaveCriticalSection( &dlg->m_lock );
			}
			return 0;
		}

		case WM_PROGRESS_UPDATE_BAR:
		{
			CProgressDialog* dlg = (CProgressDialog*) GetWindowLongPtr( hWnd, GWLP_USERDATA );
			if (dlg->m_hItemBar)
			{
				EnterCriticalSection( &dlg->m_lock );
				SendMessage(dlg->m_hItemBar, PBM_SETPOS, dlg->m_taskProgress, 0 ); 
				UpdateWindow(dlg->m_hItemBar);
				UpdateWindow(dlg->m_hWnd);
				LeaveCriticalSection( &dlg->m_lock );
			}
			return 0;
		}

		case WM_COMMAND:
		{
			if ( LOWORD(wParam) == IDC_CANCEL_BUTTON && HIWORD(wParam) == BN_CLICKED )
			{
				CProgressDialog* dlg = (CProgressDialog*) GetWindowLongPtr( hWnd, GWLP_USERDATA );
				SetWindowTextA(dlg->m_hItemText, "Canceling...");
				EnableWindow(dlg->m_hItemCancelButton, false );
				dlg->m_isCanceled = true;
			}
			return 0;
		}
	}

	return 0;
}

