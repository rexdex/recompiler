#include "build.h"
#include "logWindow.h"
#include "progressDialog.h"
#include "frame.h"

wxStopWatch*				CLogWindow::LogMessage::st_timer = nullptr;

CLogWindow::TLogWindows		CLogWindow::st_logWindows;
wxCriticalSection			CLogWindow::st_logWindowsLock;

BEGIN_EVENT_TABLE( CLogWindow, wxWindow )
	EVT_LEFT_DOWN( CLogWindow::OnMouseDown )
	EVT_LEFT_DCLICK( CLogWindow::OnMouseDown )
	EVT_LEFT_UP( CLogWindow::OnMouseUp )
	EVT_MOUSEWHEEL( CLogWindow::OnMouseWheel )
	EVT_PAINT( CLogWindow::OnPaint )
	EVT_ERASE_BACKGROUND( CLogWindow::OnErase )
	EVT_KEY_DOWN( CLogWindow::OnKeyDown )
	EVT_SCROLLWIN( CLogWindow::OnScroll )
	EVT_SIZE( CLogWindow::OnSize )
	EVT_SET_CURSOR( CLogWindow::OnSetCursor )
	EVT_TIMER( 12345, CLogWindow::OnCursorTimer )
	EVT_CHAR( CLogWindow::OnChar )
	EVT_CHAR_HOOK( CLogWindow::OnCharHook )
END_EVENT_TABLE();

//---------------------------------------------------------------------------

CLogWindow::LogMessage::LogMessage( const ELogMesasgeType type, const wxString& module, const char* txt )
	: m_type( type )
	, m_timestamp( st_timer->TimeInMicro() )
	, m_module( module )
	, m_text( txt )
{
}

CLogWindow::LogMessage::~LogMessage()
{
}

CLogWindow::LogMessage* CLogWindow::LogMessage::Build( const ELogMesasgeType type, const char* txt )
{
	// create timer
	if ( !st_timer )
	{
		st_timer = new wxStopWatch();
	}

	// parse the module name
	const char* cur = txt;
	while ( *cur )
	{
		if ( *cur <= ' ' || *cur == ':' )
		{
			break;
		}

		cur += 1;
	}

	// do we have the module name ?
	wxString moduleName;
	if ( *cur == ':' && cur > txt )
	{
		moduleName = wxString( txt, cur );
		cur += 1;

		while ( *cur == ' ' )
		{
			cur += 1;
		}
	}
	else
	{
		cur = txt;

		if ( type == eLogMesasgeType_Error )
		{
			moduleName = "Error";
		}
		else if ( type == eLogMesasgeType_Warning )
		{
			moduleName = "Warning";
		}
		else
		{
			moduleName = "Log";
		}
	}

	// class swap
	ELogMesasgeType actualType = type;
	if ( 0 == strcmp( moduleName, "Script" ) )
	{
		if ( type == eLogMesasgeType_Info )
		{
			actualType = eLogMesasgeType_ScriptInfo;
		}
	}

	// create the message
	return new LogMessage( actualType, moduleName, cur );
}

//---------------------------------------------------------------------------

CLogWindow::DrawingStyle::DrawingStyle()
	: m_lineHeight( 18 )
	, m_moduleOffset( 10 )
	, m_textOffset( 100 )
{
	wxFontInfo fontInfo(8);
	fontInfo.Family( wxFONTFAMILY_SCRIPT );
	fontInfo.FaceName("Courier New");
	fontInfo.AntiAliased(true);
	m_font = wxFont( fontInfo );

	// generic colors
	wxColour normalBack( 40,40,40 );
	wxColour selectedBack( 40,40,40 );

	// main text colors
	wxColour textColors[ eLogMesasgeType_MAX ];
	textColors[eLogMesasgeType_CommandLine] = wxColour( 200,200,200 );	
	textColors[eLogMesasgeType_Info] = wxColour( 128,128,128 );
	textColors[eLogMesasgeType_Warning] = wxColour( 140,140,128 );
	textColors[eLogMesasgeType_Error] = wxColour( 255,150,150 );
	textColors[eLogMesasgeType_ScriptInfo] = wxColour( 128,180,128 );
	textColors[eLogMesasgeType_ScriptWarning] = wxColour( 140,140,128 );
	textColors[eLogMesasgeType_ScriptError] = wxColour( 200,200,100 );

	// create pens and brushes
	for ( uint32 i=0; i<eLogMesasgeType_MAX; ++i ) 
	{
		m_backBrush[i][0] = wxBrush( normalBack, wxBRUSHSTYLE_SOLID );
		m_backPen[i][0] = wxPen( normalBack, 1, wxPENSTYLE_SOLID );
		m_backBrush[i][1] = wxBrush( selectedBack, wxBRUSHSTYLE_SOLID );
		m_backPen[i][1] = wxPen( selectedBack, 1, wxPENSTYLE_SOLID );

		m_textBrush[i][0] = wxBrush( textColors[i], wxBRUSHSTYLE_SOLID );
		m_textBrush[i][1] = wxBrush( textColors[i], wxBRUSHSTYLE_SOLID );
		m_textPen[i][0] = wxPen( textColors[i], 1, wxPENSTYLE_SOLID );
		m_textPen[i][1] = wxPen( textColors[i], 1, wxPENSTYLE_SOLID );
	}

	// command line has darker background
	wxColour cmdNormalBack( 50,50,50 );
	m_backBrush[eLogMesasgeType_CommandLine][0] = wxBrush( cmdNormalBack, wxBRUSHSTYLE_SOLID );
	m_backPen[eLogMesasgeType_CommandLine][0] = wxPen( cmdNormalBack, 1, wxPENSTYLE_SOLID );
}

CLogWindow::CLogWindow( wxWindow* parent )
	: wxWindow( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER | wxWANTS_CHARS | wxVSCROLL )
	, m_firstVisibleLine( 0 )
	, m_lockScroll( true )
	, m_commandLineHistoryIndex( 0 )
	, m_commandLineCursor( 0 )
	, m_cursorBlink( false )
	, m_cursorTimer( this, 12345 )
{
	// cache the main thread ID
	m_mainThreadId = wxThread::GetCurrentId();

	// add to the list of log windows
	{
		st_logWindowsLock.Enter();
		st_logWindows.push_back( this );
		st_logWindowsLock.Leave();
	}

	// show teh scroll bar
	UpdateScroll();

	// redraw log
	m_cursorTimer.Start(500);
}

CLogWindow::~CLogWindow()
{
	// remove from the list of log windows
	{
		st_logWindowsLock.Enter();
		st_logWindows.erase( std::find( st_logWindows.begin(), st_logWindows.end(), this ) );
		st_logWindowsLock.Leave();
	}

	// remove the lines
	Clear();
}

void CLogWindow::SetScrollLock( bool lockScroll )
{
	m_lockScroll = lockScroll;
}

void CLogWindow::Clear()
{
	TMessages tempMessages, tempPendingMessages;

	// get the current lists
	{
		m_pendingMessagesLock.Enter();
		std::swap( tempMessages, m_messages );
		std::swap( tempPendingMessages, m_pendingMessages );
		m_pendingMessagesLock.Leave();
	}

	// cleanup the message list
	for ( uint32 i=0; i<tempMessages.size(); ++i )
	{
		delete tempMessages[i];
	}

	// cleanup the pending list
	for ( uint32 i=0; i<tempPendingMessages.size(); ++i )
	{
		delete tempPendingMessages[i];
	}

	// reset visibility
	m_firstVisibleLine = 0;
}

bool CLogWindow::RefreshWindows()
{
	bool updateNeeded = false;

	st_logWindowsLock.Enter();

	// process all windows
	for ( uint32 i=0; i<st_logWindows.size(); ++i )
	{
		CLogWindow* log = st_logWindows[i];

		// push the pending messages to the normal messages list
		TMessages newMessages;

		// get the current lists
		{
			log->m_pendingMessagesLock.Enter();
			std::swap( newMessages, log->m_pendingMessages );
			log->m_pendingMessagesLock.Leave();
		}

		// add the messages to the normal list
		if ( !newMessages.empty() )
		{
			for ( uint32 j=0; j<newMessages.size(); ++j )
				log->m_messages.push_back( newMessages[j] );
			
			log->UpdateScroll();
			log->Refresh();
			log->Update();

			updateNeeded = true;
		}
	}

	st_logWindowsLock.Leave();

	return updateNeeded;
}

bool CLogWindow::DoLog( const char* buffer )
{
	LogMessage* newMessage = LogMessage::Build( eLogMesasgeType_Info, buffer );
	if ( newMessage != nullptr )
	{
		if ( m_mainThreadId != wxThread::GetCurrentId() )
		{
			m_pendingMessagesLock.Enter();
			m_pendingMessages.push_back( newMessage );
			m_pendingMessagesLock.Leave();
		}
		else
		{
			m_messages.push_back( newMessage );
			UpdateScroll();
			Refresh();
			Update();
		}
	}

	return true;
}

bool CLogWindow::DoWarn( const char* buffer )
{
	LogMessage* newMessage = LogMessage::Build( eLogMesasgeType_Warning, buffer );
	if ( newMessage != nullptr )
	{
		if ( m_mainThreadId != wxThread::GetCurrentId() )
		{
			m_pendingMessagesLock.Enter();
			m_pendingMessages.push_back( newMessage );
			m_pendingMessagesLock.Leave();
		}
		else
		{
			m_messages.push_back( newMessage );
			UpdateScroll();
			Refresh();
			Update();
		}
	}

	return true;
}

bool CLogWindow::DoError( const char* buffer )
{
	LogMessage* newMessage = LogMessage::Build( eLogMesasgeType_Error, buffer );
	if ( newMessage != nullptr )
	{
		if ( m_mainThreadId != wxThread::GetCurrentId() )
		{
			m_pendingMessagesLock.Enter();
			m_pendingMessages.push_back( newMessage );
			m_pendingMessagesLock.Leave();			
		}
		else
		{
			m_messages.push_back( newMessage );
			UpdateScroll();
			Refresh();
			Update();
		}
	}

	return true;
}

bool CLogWindow::DoSetTaskName( const char* buffer )
{
	CProgressDialog::GetInstance().UpdateTaskText( buffer );
	return true;
}

bool CLogWindow::DoSetTaskProgress( int count, int max )
{
	int prc = (int)( ((__int64)count * 100) / (__int64)max );
	if (prc < 0) prc = 0;
	else if (prc > 100) prc = 100;

	CProgressDialog::GetInstance().UpdateTaskProgress( prc );
	return true;
}

bool CLogWindow::DoBeginTask( const char* buffer )
{
	CProgressDialog::GetInstance().StartTask( buffer );
	return true;
}

bool CLogWindow::DoEndTask()
{
	CProgressDialog::GetInstance().EndTask();
	return true;
}

bool CLogWindow::DoIsTaskCanceled()
{
	return CProgressDialog::GetInstance().IsTaskCanceled();
}

bool CLogWindow::DoFlush()
{
	CLogWindow::RefreshWindows();
	return true;
}

void CLogWindow::SetFirstVisible( int line )
{
	// get visible height
	const int clientHeight = GetClientSize().y;
	const int numVisLines = clientHeight / m_style.m_lineHeight;
	const int maxVisibleLine = wxMax( 0, (int)m_messages.size() - numVisLines );
	
	// clamp
	if ( line < 0 )
	{
		line = 0;
	}
	else if ( line > maxVisibleLine )
	{
		line = maxVisibleLine;
	}

	// change
	if ( line != m_firstVisibleLine )
	{
		m_firstVisibleLine = line;
		SetScrollPos( wxVERTICAL, m_firstVisibleLine, true );
		Refresh();
	}
}

void CLogWindow::UpdateScroll()
{
	// clamp the range
	if ( m_firstVisibleLine <= 0 )
	{
		m_firstVisibleLine = 0;
	}
	else if ( m_firstVisibleLine >= (int)m_messages.size() )
	{
		m_firstVisibleLine = (int)m_messages.size() - 1;
	}

	// get visible height
	const int clientHeight = GetClientSize().y;
	const int numVisLines = (clientHeight / m_style.m_lineHeight) - 1;

	// update the scrollbar
	const int lastScrollLine = wxMax( 0, (int)m_messages.size() );
	SetScrollbar( wxVERTICAL, m_firstVisibleLine, numVisLines, lastScrollLine, true );

	// scroll to the end
	if ( m_lockScroll )
	{
		if ( (int)m_messages.size() > numVisLines )
		{
			m_firstVisibleLine = m_messages.size() - numVisLines;
		}
		else
		{
			m_firstVisibleLine = 0;
		}
	}
}

inline static bool ParseHexValue( const char* start, uint32& outValue, uint32& outLength )
{
	char digits[ 10 ];
	uint32 numDigits = 0;

	// search for the 'h' at the end - also, validate characters
	bool validNubmer = false;
	for ( uint32 i=0; i<sizeof(digits); ++i )
	{
		const char ch = start[i];
		if ( ch == 'h' )
		{
			if ( numDigits != 0 ) validNubmer = true;
			break;
		}

		// grab digits
		char value = 0;
		if ( ch == '0' ) value = 0;
		else if ( ch == '1' ) value = 1;
		else if ( ch == '2' ) value = 2;
		else if ( ch == '3' ) value = 3;
		else if ( ch == '4' ) value = 4;
		else if ( ch == '5' ) value = 5;
		else if ( ch == '6' ) value = 6;
		else if ( ch == '7' ) value = 7;
		else if ( ch == '8' ) value = 8;
		else if ( ch == '9' ) value = 9;
		else if ( ch == 'A' ) value = 10;
		else if ( ch == 'a' ) value = 10;
		else if ( ch == 'B' ) value = 11;
		else if ( ch == 'b' ) value = 11;
		else if ( ch == 'C' ) value = 12;
		else if ( ch == 'c' ) value = 12;
		else if ( ch == 'D' ) value = 13;
		else if ( ch == 'd' ) value = 13;
		else if ( ch == 'E' ) value = 14;
		else if ( ch == 'e' ) value = 14;
		else if ( ch == 'F' ) value = 15;
		else if ( ch == 'f' ) value = 15;
		else
		{
			break;
		}

		// add to digit list
		digits[ numDigits ] = value;
		numDigits += 1;
	}

	// not a valid number
	if ( !validNubmer || numDigits < 5 )
	{
		return false;
	}

	// convert to number
	uint32 val = 0;
	for ( uint32 i=0; i<numDigits; ++i )
	{
		val = (val<<4) + digits[i];
	}

	// return final value
	outValue = val;
	outLength = numDigits+1;
	return true;
}

inline static bool ParseTraceValue( const char* start, uint32& outValue, uint32& outLength )
{
	char digits[ 10 ];
	uint32 numDigits = 0;

	// must start with '#'
	if ( start[0] != '#' )
		return false;

	// search for the 'h' at the end - also, validate characters
	for ( uint32 i=1; i<sizeof(digits); ++i )
	{
		const char ch = start[i];

		// grab digits
		char value = 0;
		if ( ch == '0' ) value = 0;
		else if ( ch == '1' ) value = 1;
		else if ( ch == '2' ) value = 2;
		else if ( ch == '3' ) value = 3;
		else if ( ch == '4' ) value = 4;
		else if ( ch == '5' ) value = 5;
		else if ( ch == '6' ) value = 6;
		else if ( ch == '7' ) value = 7;
		else if ( ch == '8' ) value = 8;
		else if ( ch == '9' ) value = 9;
		else
		{
			break;
		}

		// add to digit list
		digits[ numDigits ] = value;
		numDigits += 1;
	}

	// not a valid number
	if ( numDigits < 5 )
	{
		return false;
	}

	// convert to number
	uint32 val = 0;
	for ( uint32 i=0; i<numDigits; ++i )
	{
		val = (val*10) + digits[i];
	}

	// return final value
	outValue = val;
	outLength = numDigits+1;
	return true;
}

class wxAddressData : public wxObject
{
public:
	uint32	m_address;

public:
	wxAddressData( const uint32 data )
		: m_address( data )
	{}
};

void CLogWindow::OnMouseDown( wxMouseEvent& event )
{
	SetFocus();

	if ( event.LeftDClick() )
	{
		const int lineIndex = m_firstVisibleLine + (event.GetPosition().y / m_style.m_lineHeight );
		if ( lineIndex < (int) m_messages.size() )
		{
			// get the clicked line
			const char* lineText = m_messages[ lineIndex ]->m_text.c_str();
			if ( lineText[0] )
			{
				static const uint32 MAX_ADDRESSES = 16;
				uint32 addresses[ MAX_ADDRESSES ];
				uint32 numAdresses = 0;
				uint32 trace[ MAX_ADDRESSES ];
				uint32 numTrace = 0;

				// search the text for the numerical address
				const char* txt = lineText;
				const char* txtEnd = lineText + strlen(lineText);
				while ( txt < txtEnd )
				{
					uint32 value=0, length=0;
					if ( ParseHexValue( txt, value, length ) )
					{
						if ( numAdresses == MAX_ADDRESSES ) break;
						addresses[ numAdresses ] = value;
						numAdresses += 1;
						txt += length;
					}
					else if ( ParseTraceValue( txt, value, length ) )
					{
						if ( numTrace == MAX_ADDRESSES ) break;
						trace[ numTrace ] = value;
						numTrace += 1;
						txt += length;
					}
					else
					{
						txt += 1;
					}
				}

				// exactly one address - navigate directly
				if ( numAdresses == 1 && numTrace == 0 )
				{
					const uint32 address = addresses[0];
					wxTheFrame->NavigateToAddress( address );
				}
				else if ( numTrace == 1 && numAdresses == 0 )
				{
					const uint32 traceIndex = trace[0];
					wxTheFrame->NavigateToTraceEntry( traceIndex );
				}
				else
				{
					wxMenu popupMenu;

					for ( uint32 i=0; i<numAdresses; ++i )
					{
						char buf[32];
						sprintf_s( buf, "Address: %08Xh", addresses[i] );
						popupMenu.Append( 1000 + i, buf );
						popupMenu.Connect( 1000 + i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CLogWindow::OnNavigateToAddress ), new wxAddressData(addresses[i]), this );
					}

					if (numAdresses && numTrace)
						popupMenu.AppendSeparator();

					for ( uint32 i=0; i<numTrace; ++i )
					{
						char buf[32];
						sprintf_s( buf, "Trace: #%u", trace[i] );
						popupMenu.Append( 2000 + i, buf );
						popupMenu.Connect( 2000 + i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CLogWindow::OnNavigateToTrace ), new wxAddressData(trace[i]), this );
					}

					PopupMenu(&popupMenu);
				}
			}
		}
	}
}

void CLogWindow::OnMouseUp( wxMouseEvent& event )
{
}

void CLogWindow::OnMouseWheel( wxMouseEvent& event )
{
	int shift = 1;

	if ( wxGetKeyState( WXK_SHIFT ) )
	{
		shift = 10;
	}

	if ( event.GetWheelRotation() < 0 )
	{
		SetFirstVisible( m_firstVisibleLine + shift );
	}
	else if ( event.GetWheelRotation() > 0 )
	{
		SetFirstVisible( m_firstVisibleLine - shift );
	}
}

void CLogWindow::OnPaint( wxPaintEvent& event )
{
	wxBufferedPaintDC dc( this );
	PrepareDC( dc );

	// size of the DC
	const wxSize size = GetClientSize();

	// set font
	dc.SetFont( m_style.m_font );

	// command line size
	int cmdLineH = m_style.m_lineHeight;
	int cmdLineY = size.y - cmdLineH;

	// start drawing
	int lineY = 0;
	int lineIndex = m_firstVisibleLine;
	while ( lineIndex < (int)m_messages.size() && lineY <= cmdLineY )
	{
		const LogMessage* msg = m_messages[ lineIndex ];

		// is the log line selected
		const int isSelected = 0;

		// get drawing style
		const int style = msg->m_type;

		// compute line rect
		const wxRect lineRect( 0, lineY, size.x, m_style.m_lineHeight );
		dc.SetBrush( m_style.m_backBrush[style][isSelected] );
		dc.SetPen( m_style.m_backPen[style][isSelected] );
		dc.SetTextBackground( m_style.m_textBrush[style][isSelected].GetColour() );
		dc.SetTextForeground( m_style.m_textPen[style][isSelected].GetColour() );		
		dc.DrawRectangle( lineRect );

		// draw with clipping
		{
			// draw the line components
			const int textY = lineY + 2;
			{
				wxRect channelRect( 0, lineY, m_style.m_textOffset-1, lineY + m_style.m_lineHeight );

				// center
				wxSize textSize = dc.GetTextExtent( msg->m_module );
				const int xPos = m_style.m_textOffset + (m_style.m_moduleOffset-m_style.m_textOffset - textSize.x) / 2;

				dc.SetClippingRegion( channelRect );
				dc.DrawText( msg->m_module, xPos, textY );
				dc.DestroyClippingRegion();
			}

			dc.DrawText( msg->m_text, m_style.m_textOffset + 10, textY );
		}

		// go to next line
		lineY += m_style.m_lineHeight;
		lineIndex += 1;
	}

	// clear the rest
	if ( lineY < size.y )
	{
		dc.SetBrush( m_style.m_backBrush[eLogMesasgeType_Info][0] );
		dc.SetPen( m_style.m_backPen[eLogMesasgeType_Info][0] );
		dc.DrawRectangle( 0, lineY, size.x, size.y - lineY + 1 );
	}

	// draw the command background
	{
		dc.SetBrush( m_style.m_backBrush[eLogMesasgeType_CommandLine][0] );
		dc.SetPen( m_style.m_backPen[eLogMesasgeType_CommandLine][0] );
		dc.DrawRectangle( 0, cmdLineY, size.x, cmdLineH );
	}

	// draw the split line
	{
		dc.DrawLine( m_style.m_textOffset, 0, m_style.m_textOffset, cmdLineY );
		dc.DrawLine( 0, cmdLineY, size.x, cmdLineY );
	}

	// draw the command line text
	{
		dc.SetTextBackground( m_style.m_textBrush[eLogMesasgeType_CommandLine][0].GetColour() );
		dc.SetTextForeground( m_style.m_textPen[eLogMesasgeType_CommandLine][0].GetColour() );
		int lineX = 4;
		dc.DrawText( ">", lineX, lineY ); lineX += 10;
		dc.DrawText( m_commandLine, lineX, cmdLineY);
	}
}

void CLogWindow::OnErase( wxEraseEvent& event )
{
	// nothing
}

void CLogWindow::OnKeyDown( wxKeyEvent& event )
{
	if ( event.GetKeyCode() == WXK_UP )
	{
		if ( event.ControlDown() )
		{
			SetFirstVisible( m_firstVisibleLine - 1 );
		}
		else
		{
			if ( !m_commandLineHistory.empty() )
			{
				if ( m_commandLineHistoryIndex == -1 )
					m_commandLineHistoryIndex = m_commandLineHistory.size() - 1;
				else if ( m_commandLineHistoryIndex > 0 )
					m_commandLineHistoryIndex -= 1;

				m_commandLine = m_commandLineHistory[ m_commandLineHistoryIndex ];
			}
		}
	}
	else if ( event.GetKeyCode() == WXK_DOWN )
	{
		if ( event.ControlDown() )
		{
			SetFirstVisible( m_firstVisibleLine + 1 );
		}
		else
		{
			if ( !m_commandLineHistory.empty() )
			{
				if ( m_commandLineHistoryIndex == -1 )
					m_commandLineHistoryIndex = m_commandLineHistory.size() - 1;
				else if ( m_commandLineHistoryIndex < (int) (m_commandLineHistory.size()-1) )
					m_commandLineHistoryIndex += 1;

				m_commandLine = m_commandLineHistory[ m_commandLineHistoryIndex ];
			}
		}
	}
	else if ( event.GetKeyCode() == WXK_PAGEUP )
	{
		const int numLinesPerPage = GetClientSize().y / m_style.m_lineHeight;
		SetFirstVisible( m_firstVisibleLine - numLinesPerPage );
	}
	else if ( event.GetKeyCode() == WXK_PAGEDOWN )
	{
		const int numLinesPerPage = GetClientSize().y / m_style.m_lineHeight;
		SetFirstVisible( m_firstVisibleLine + numLinesPerPage );
	}
	else if ( event.GetKeyCode() == WXK_HOME )
	{
		SetFirstVisible( 0 );
	}
	else if ( event.GetKeyCode() == WXK_END )
	{
		SetFirstVisible( m_messages.size() );
	}
	else
	{
		event.Skip();
	}
}

void CLogWindow::OnScroll( wxScrollWinEvent& event )
{
	SetFirstVisible( event.GetPosition() );
}

void CLogWindow::OnSize( wxSizeEvent& event )
{
	UpdateScroll();
	Refresh();
	event.Skip();
}

void CLogWindow::OnSetCursor( wxSetCursorEvent& event )
{
	wxPoint clientPos = ScreenToClient( wxGetMousePosition() );
	if ( abs( (int)(clientPos.x - m_style.m_textOffset) ) < 3 )
	{
		event.SetCursor( wxCursor( wxCURSOR_SIZEWE ) );
		return;
	}

	event.Skip();
}

void CLogWindow::OnCursorTimer( wxTimerEvent& event )
{
	m_cursorBlink = !m_cursorBlink;
	Refresh();
}

void CLogWindow::OnCharHook( wxKeyEvent& event )
{
	if ( event.GetKeyCode() == WXK_RETURN )
	{
		ExecuteCommandLine();
		m_cursorBlink = true;
		m_cursorTimer.Start( 500 );
		Refresh();
	}

	event.Skip();
}

void CLogWindow::OnChar( wxKeyEvent& event )
{
	bool redraw = false;

	const wxChar ch = event.GetUnicodeKey();
	if ( ch == 8 )
	{
		m_commandLineHistoryIndex = -1;

		if ( m_commandLineCursor > 0 )
		{
			m_commandLine = m_commandLine.Left( m_commandLineCursor-1 );
			m_commandLineCursor -= 1;
			redraw = true;
		}
	}
	else if ( ch >= 32 )
	{
		m_commandLineHistoryIndex = -1;

		wxChar txt[2] = { ch, 0 };
		wxString str = txt;
		m_commandLine += str.Lower();
		m_commandLineCursor = m_commandLine.Length();
		redraw = true;
	}

	// redraw
	if ( redraw )
	{
		m_cursorBlink = true;
		m_cursorTimer.Start( 500 );
		Refresh();
	}
}

void CLogWindow::ExecuteCommandLine()
{
	if ( m_commandLine.empty() )
	{
		return;
	}

	// add command to the history
	if ( m_commandLineHistoryIndex == -1 )
	{
		m_commandLineHistory.push_back( m_commandLine );
		m_commandLineHistoryIndex = m_commandLineHistory.size();
	}

	// reset
	const wxString command = m_commandLine;
	m_commandLine = "";

	// execute
	//decoding::Environment::GetInstance().CallScript( *wxTheFrame->GetLogView(), command.c_str().AsChar() );
	Log( "Cmd: %s", command.c_str().AsChar() );
	wxTheFrame->NavigateUrl(command);
}

void CLogWindow::OnNavigateToAddress( wxCommandEvent& event )
{
	wxAddressData* data = static_cast< wxAddressData* >( event.m_callbackUserData );
	wxTheFrame->NavigateToAddress( data->m_address );
}

void CLogWindow::OnNavigateToTrace( wxCommandEvent& event )
{
	wxAddressData* data = static_cast< wxAddressData* >( event.m_callbackUserData );
	wxTheFrame->NavigateToTraceEntry( data->m_address );
}