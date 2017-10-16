#include "build.h"
#include "dx11RenderingWindow.h"

//#pragma optimize("",off)

CDX11RenderingWindow::CDX11RenderingWindow()
	: m_hThread( NULL )
	, m_hWnd( NULL )
	, m_isOpened( false )
{
	// register window class
	static BOOL sRegisterWindowClass = true;
	if ( sRegisterWindowClass )
	{
		sRegisterWindowClass = false;

		WNDCLASSEXW info;
		memset( &info, 0, sizeof( WNDCLASSEX ) );
		info.cbSize = sizeof( WNDCLASSEX );

		// Register generic class
		info.cbWndExtra = 8;
		info.hbrBackground = (HBRUSH)GetStockObject( NULL_BRUSH );
		info.hCursor = ::LoadCursor( NULL, IDC_CROSS );
		info.hIcon = NULL;
		info.hInstance = ::GetModuleHandle( NULL );
		info.lpfnWndProc = (WNDPROC) &CDX11RenderingWindow::StaticWndProc;
		info.lpszClassName = L"DX11XenonGPUViewportClass";
		info.lpszMenuName = NULL;
		info.style = CS_VREDRAW | CS_HREDRAW;
		::RegisterClassExW( &info );
	}

	// create thread, this will create the actual window
	m_hSync = ::CreateEventA( NULL, FALSE, FALSE, NULL );
	m_hThread = ::CreateThread( NULL, 16 << 10, &ThreadProc, this, 0, NULL );

	// wait for window being created
	WaitForSingleObject( m_hSync, INFINITE );
	GLog.Log( "DX11: Rendering window created, HWND=0x%08X", m_hWnd );
}

CDX11RenderingWindow::~CDX11RenderingWindow()
{
	GLog.Log( "DX11: Closing window" );

	// close only if the window's there
	if ( m_hWnd )
	{
		// make sure event is reset
		ResetEvent( m_hSync );

		// send request to close window
		PostMessage( m_hWnd, WM_USER + 666, 0, 0 );
	
		// wait for window to close
		GLog.Log( "DX11: Waiting for window to close..." );
		WaitForSingleObject( m_hSync, INFINITE );
	}

	// wait for the window thread to finish
	if ( m_hThread )
	{
		GLog.Log( "DX11: Waiting for window thread to finish..." );
		if ( WAIT_TIMEOUT == WaitForSingleObject( m_hThread, 2000 ) )
		{
			GLog.Err( "DX11: Window thread failed to close after 2s, killing it" );
			TerminateThread( m_hThread, 0 );
		}

		// close thread handle
		CloseHandle( m_hThread );
		m_hThread = NULL;
	}

	// close sync event handle
	CloseHandle( m_hSync );
	m_hSync = NULL;

	// final message
	GLog.Log( "DX11: Window closed" );
}

DWORD CDX11RenderingWindow::ThreadProc( LPVOID lpParameter )
{
	CDX11RenderingWindow* obj = (CDX11RenderingWindow*) lpParameter;

	// name GPU Thread
	utils::SetThreadName( GetCurrentThreadId(), "Window Thread" );

	// thread started
	GLog.Log( "DX11: Window thread started" );

	// create window
	if ( !obj->CreateThreadWindow( "Xenon Recompiler by Dex (C) 2014-2015", 1280, 720 ) )
	{
		GLog.Err( "DX11: Failed to create rendering window" );
		GPlatform.RequestUserExit();
		return 0;
	}

	// signal initialization
	GLog.Log( "DX11: Window thread created the window" );
	SetEvent( obj->m_hSync );

	// process window messages
	MSG msg;
	while ( GetMessage( &msg, obj->m_hWnd, 0, 0 ) )
	{
		// magic
		if ( msg.message == (WM_USER + 666) )
		{
			GLog.Log( "DX11: Window received internal close request" );
			PostQuitMessage( 0 );
			continue;
		}

		// process messages
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}

	// request to exit app now
	GPlatform.RequestUserExit();

	// close the window
	DestroyWindow( obj->m_hWnd );

	// signal initialization
	GLog.Log( "DX11: Window thread closed the window" );
	SetEvent( obj->m_hSync );

	// exit
	GLog.Log( "DX11: Window thread finished" );
	return 0;
}

bool CDX11RenderingWindow::CreateThreadWindow( const char* title, const uint32 width, const uint32 height )
{
	// Get size of desktop
	INT desktopX = ::GetSystemMetrics( SM_CXSCREEN );
	INT desktopY = ::GetSystemMetrics( SM_CYSCREEN );

	// Try to plane in center
	RECT windowRect;
	windowRect.left = max( 0, desktopX/2 - width/2 );
	windowRect.top = max( 0, desktopY/2 - height/2 );
	windowRect.right = windowRect.left + width;
	windowRect.bottom = windowRect.top + height;

	// Normal floating window
	DWORD exStyle = WS_EX_APPWINDOW;
	DWORD mainStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;

	// Show window
	mainStyle |= WS_VISIBLE;

	// Adjust window rectangle with border size
	::AdjustWindowRect( &windowRect, mainStyle, false );

	// Create window
	HWND handle = ::CreateWindowExW( 
		exStyle,
		L"DX11XenonGPUViewportClass", 
		L"Xbox360 emulator - GPU Output",
		mainStyle | WS_CLIPCHILDREN, 
		windowRect.left,
		windowRect.top, 
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		NULL, 
		NULL, 
		::GetModuleHandle( NULL ),
		this
		);

	// no window created
	if ( !handle )
		return false;

	// Bring to front 
	::SetForegroundWindow( handle );
	::SetFocus( handle );

	// Initial refresh
	::UpdateWindow( handle );
	::SendMessage( handle, WM_ERASEBKGND, 0, 0 );

	// window created
	return true;
}

LRESULT CDX11RenderingWindow::HandleMessage( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch ( uMsg )
	{
		// Window is about to be destroyed
		case WM_DESTROY:
		{
			// Unlink window from this instance
			SetWindowLongPtr( m_hWnd, GWLP_USERDATA, NULL );
			break;
		}

		// User wants to close the window
		case WM_CLOSE:
		{
			// Floating windows cannot be closed like this
			GPlatform.RequestUserExit();
			DestroyWindow( m_hWnd );
			return 0;
		}

		// Debug keys
		case WM_KEYDOWN:
		{
			// hack
			if ( wParam == VK_ESCAPE )
			{
				GPlatform.RequestUserExit();
			}
			break;
		}
	}

	// Default processing
	return DefWindowProc( m_hWnd, uMsg, wParam, lParam );
}

LRESULT APIENTRY CDX11RenderingWindow::StaticWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	// Window creation
	if ( uMsg == WM_CREATE )
	{
		LPCREATESTRUCT data = (LPCREATESTRUCT)lParam;
		::SetWindowLongPtr( hWnd, GWLP_USERDATA, (LONG_PTR)data->lpCreateParams );
		CDX11RenderingWindow* window = (CDX11RenderingWindow*)data->lpCreateParams;
		window->m_hWnd = hWnd;
	}

	// Process messages by window message function
	CDX11RenderingWindow* window = (CDX11RenderingWindow*) ::GetWindowLongPtr( hWnd, GWLP_USERDATA );
	if ( window )
	{
		return window->HandleMessage( uMsg, wParam, lParam );
	}
	else
	{
		return DefWindowProc( hWnd, uMsg, wParam, lParam );
	}
}
