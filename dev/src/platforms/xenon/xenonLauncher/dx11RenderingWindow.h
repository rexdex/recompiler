#pragma once

#include <Windows.h>

/// Rendering window for DX11 based GPU emulation
/// NOTE: window runs on yet another thread
class CDX11RenderingWindow
{
public:
	CDX11RenderingWindow();
	~CDX11RenderingWindow();

	// get window handle
	inline HWND GetWindowHandle() const { return m_hWnd; }

	// is window opened (valid after successful initialization)
	inline const bool IsOpened() const { return m_isOpened; }

private:
	HANDLE	m_hSync;		// simple sync objects (reusable)
	HANDLE	m_hThread;		// window thread
	HWND	m_hWnd;			// window itself

	bool	m_isOpened;		// is this window opened ?

	// thread func
	static DWORD WINAPI ThreadProc( LPVOID lpParameter );

	// message functions
	LRESULT HandleMessage( UINT uMsg, WPARAM wParam, LPARAM lParam );
	static LRESULT APIENTRY StaticWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );	

	// create window, returns FALSE if failed
	bool CreateThreadWindow( const char* title, const uint32 width, const uint32 height );
};
