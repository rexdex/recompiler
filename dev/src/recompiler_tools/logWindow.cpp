#include "build.h"
#include "logWindow.h"

namespace tools
{

	wxStopWatch* LogDisplayBuffer::LogMessage::st_timer = nullptr;

	LogDisplayBuffer::TLogWindows LogDisplayBuffer::st_logWindows;
	wxCriticalSection LogDisplayBuffer::st_logWindowsLock;

	BEGIN_EVENT_TABLE(LogDisplayBuffer, wxWindow)
		EVT_LEFT_DOWN(LogDisplayBuffer::OnMouseDown)
		EVT_LEFT_DCLICK(LogDisplayBuffer::OnMouseDown)
		EVT_LEFT_UP(LogDisplayBuffer::OnMouseUp)
		EVT_MOUSEWHEEL(LogDisplayBuffer::OnMouseWheel)
		EVT_PAINT(LogDisplayBuffer::OnPaint)
		EVT_ERASE_BACKGROUND(LogDisplayBuffer::OnErase)
		EVT_KEY_DOWN(LogDisplayBuffer::OnKeyDown)
		EVT_SCROLLWIN(LogDisplayBuffer::OnScroll)
		EVT_SIZE(LogDisplayBuffer::OnSize)
		EVT_SET_CURSOR(LogDisplayBuffer::OnSetCursor)
	END_EVENT_TABLE();

	//---------------------------------------------------------------------------

	LogDisplayBuffer::LogMessage::LogMessage(const MessageType type, const char* txt)
		: m_type(type)
		, m_timestamp(st_timer->TimeInMicro())
		, m_text(txt)
	{
	}

	LogDisplayBuffer::LogMessage::~LogMessage()
	{
	}

	LogDisplayBuffer::LogMessage* LogDisplayBuffer::LogMessage::Build(const MessageType type, const char* txt)
	{
		// create timer
		if (!st_timer)
		{
			st_timer = new wxStopWatch();
		}

		/*// parse the module name
		const char* cur = txt;
		while (*cur)
		{
			if (*cur <= ' ' || *cur == ':')
			{
				break;
			}

			cur += 1;
		}

		// do we have the module name ?
		wxString moduleName;
		if (*cur == ':' && cur > txt)
		{
			moduleName = wxString(txt, cur);
			cur += 1;

			while (*cur == ' ')
			{
				cur += 1;
			}
		}
		else
		{
			cur = txt;

			if (type == MessageType::Error)
			{
				moduleName = "Error";
			}
			else if (type == MessageType::Warning)
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
		if (0 == strcmp(moduleName, "Script"))
		{
			if (type == MessageType::Info)
			{
				actualType = MessageType::ScriptInfo;
			}
		}*/

		// create the message
		return new LogMessage(type, txt);
	}

	//---------------------------------------------------------------------------

	LogDisplayBuffer::DrawingStyle::DrawingStyle()
		: m_lineHeight(18)
		, m_moduleOffset(10)
	{
		wxFontInfo fontInfo(8);
		fontInfo.Family(wxFONTFAMILY_SCRIPT);
		fontInfo.FaceName("Courier New");
		fontInfo.AntiAliased(true);
		m_font = wxFont(fontInfo);

		// generic colors
		wxColour normalBack(40, 40, 40);
		wxColour selectedBack(40, 40, 40);

		// main text colors
		wxColour textColors[(uint8)MessageType::MAX];
		textColors[(uint8)MessageType::CommandLine] = wxColour(200, 200, 200);
		textColors[(uint8)MessageType::Info] = wxColour(128, 128, 128);
		textColors[(uint8)MessageType::Warning] = wxColour(140, 140, 128);
		textColors[(uint8)MessageType::Error] = wxColour(255, 150, 150);
		textColors[(uint8)MessageType::ScriptInfo] = wxColour(128, 180, 128);
		textColors[(uint8)MessageType::ScriptWarning] = wxColour(140, 140, 128);
		textColors[(uint8)MessageType::ScriptError] = wxColour(200, 200, 100);

		// create pens and brushes
		for (uint32 i = 0; i < (uint8)MessageType::MAX; ++i)
		{
			m_backBrush[i][0] = wxBrush(normalBack, wxBRUSHSTYLE_SOLID);
			m_backPen[i][0] = wxPen(normalBack, 1, wxPENSTYLE_SOLID);
			m_backBrush[i][1] = wxBrush(selectedBack, wxBRUSHSTYLE_SOLID);
			m_backPen[i][1] = wxPen(selectedBack, 1, wxPENSTYLE_SOLID);

			m_textBrush[i][0] = wxBrush(textColors[i], wxBRUSHSTYLE_SOLID);
			m_textBrush[i][1] = wxBrush(textColors[i], wxBRUSHSTYLE_SOLID);
			m_textPen[i][0] = wxPen(textColors[i], 1, wxPENSTYLE_SOLID);
			m_textPen[i][1] = wxPen(textColors[i], 1, wxPENSTYLE_SOLID);
		}

		// command line has darker background
		wxColour cmdNormalBack(50, 50, 50);
		m_backBrush[(uint8)MessageType::CommandLine][0] = wxBrush(cmdNormalBack, wxBRUSHSTYLE_SOLID);
		m_backPen[(uint8)MessageType::CommandLine][0] = wxPen(cmdNormalBack, 1, wxPENSTYLE_SOLID);
	}

	LogDisplayBuffer::LogDisplayBuffer(wxWindow* parent)
		: wxWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER | wxWANTS_CHARS | wxVSCROLL)
		, m_firstVisibleLine(0)
		, m_lockScroll(true)
		, m_errorsVisible(true)
		, m_warningsVisible(true)
		, m_infosVisible(true)
	{
		// cache the main thread ID
		m_mainThreadId = wxThread::GetCurrentId();

		// add to the list of log windows
		{
			st_logWindowsLock.Enter();
			st_logWindows.push_back(this);
			st_logWindowsLock.Leave();
		}

		// show the scroll bar
		UpdateScroll();

		// register for periodic updates
		Event("Tick") += [this](const EventID&, const EventArgs&) { FlushPendingLogLines(); };
	}

	LogDisplayBuffer::~LogDisplayBuffer()
	{
		// remove from the list of log windows
		{
			st_logWindowsLock.Enter();
			st_logWindows.erase(std::find(st_logWindows.begin(), st_logWindows.end(), this));
			st_logWindowsLock.Leave();
		}

		// remove the lines
		Clear();
	}

	void LogDisplayBuffer::Extract(wxString& outString) const
	{
		for (const auto* msg : m_allMessages)
		{
			outString += wxString::Format("[%7.3f]", (double)msg->m_timestamp.GetValue() / 1000000.0);

			switch (msg->m_type)
			{
				case MessageType::ScriptError: outString += "[E]: "; break;
				case MessageType::ScriptWarning: outString += "[W]: "; break;
				case MessageType::ScriptInfo: outString += "[I]: "; break;
				case MessageType::Error: outString += "[E]: "; break;
				case MessageType::Warning: outString += "[W]: "; break;
				case MessageType::Info: outString += "[I]: "; break;
			}

			outString += msg->m_text;
			outString += wxTextFile::GetEOL();
		}
	}

	void LogDisplayBuffer::SetScrollLock(bool lockScroll)
	{
		m_lockScroll = lockScroll;
	}

	void LogDisplayBuffer::GetFilterState(bool& outShowErrors, bool& outShowWarnings, bool& outShowInfos) const
	{
		outShowInfos = m_errorsVisible;
		outShowWarnings = m_warningsVisible;
		outShowInfos = m_infosVisible;
	}

	void LogDisplayBuffer::SetFilterState(const bool showErrors, const bool showWarnings, const bool showInfos)
	{
		if (m_errorsVisible != showErrors || m_warningsVisible != showWarnings || m_infosVisible != showInfos)
		{
			m_errorsVisible = showErrors;
			m_warningsVisible = showWarnings;
			m_infosVisible = showInfos;

			m_visibleMessages.clear();
			m_visibleMessages.reserve(m_allMessages.size());
			for (auto* msg : m_allMessages)
				if (FilterMessage(*msg))
					m_visibleMessages.push_back(msg);

			Refresh();
			Update();
			UpdateScroll();
		}		
	}

	const bool LogDisplayBuffer::FilterMessage(const LogMessage& msg) const
	{
		if (msg.m_type == MessageType::Error || msg.m_type == MessageType::ScriptError)
			return m_errorsVisible;
		if (msg.m_type == MessageType::Warning || msg.m_type == MessageType::ScriptWarning)
			return m_warningsVisible;
		if (msg.m_type == MessageType::Info || msg.m_type == MessageType::ScriptInfo)
			return m_infosVisible;
		return false;
	}

	void LogDisplayBuffer::Print(const MessageType type, const char* txt)
	{
		LogMessage* newMessage = LogMessage::Build(type, txt);
		if (newMessage != nullptr)
		{
			if (m_mainThreadId != wxThread::GetCurrentId())
			{
				m_pendingMessagesLock.Enter();
				m_pendingMessages.push_back(newMessage);
				m_pendingMessagesLock.Leave();
			}
			else
			{
				m_allMessages.push_back(newMessage);

				if (FilterMessage(*newMessage))
				{
					m_visibleMessages.push_back(newMessage);
					UpdateScroll();
					Refresh();
					Update();
				}
			}
		}
	}

	void LogDisplayBuffer::Clear()
	{
		TMessages tempMessages, tempPendingMessages;

		// get the current lists
		{
			m_pendingMessagesLock.Enter();
			std::swap(tempMessages, m_allMessages);
			std::swap(tempPendingMessages, m_pendingMessages);
			m_visibleMessages.clear();
			m_pendingMessagesLock.Leave();
		}

		// cleanup the message list
		for (uint32 i = 0; i < tempMessages.size(); ++i)
		{
			delete tempMessages[i];
		}

		// cleanup the pending list
		for (uint32 i = 0; i < tempPendingMessages.size(); ++i)
		{
			delete tempPendingMessages[i];
		}

		// reset visibility
		m_firstVisibleLine = 0;
	}

	void LogDisplayBuffer::FlushPendingLogLines()
	{
		st_logWindowsLock.Enter();

		// process all windows
		for (uint32 i = 0; i < st_logWindows.size(); ++i)
		{
			LogDisplayBuffer* log = st_logWindows[i];

			// push the pending messages to the normal messages list
			TMessages newMessages;

			// get the current lists
			{
				log->m_pendingMessagesLock.Enter();
				std::swap(newMessages, log->m_pendingMessages);
				log->m_pendingMessagesLock.Leave();
			}

			// add the messages to the normal list
			if (!newMessages.empty())
			{
				bool contentAdded = false;

				for (uint32 j = 0; j < newMessages.size(); ++j)
				{
					auto* msg = newMessages[j];
					log->m_allMessages.push_back(msg);

					if (log->FilterMessage(*msg))
					{
						log->m_visibleMessages.push_back(msg);
						contentAdded = true;
					}
				}

				if (contentAdded)
				{
					log->UpdateScroll();
					log->Refresh();
					log->Update();
				}
			}
		}

		st_logWindowsLock.Leave();
	}

	void LogDisplayBuffer::SetFirstVisible(int line)
	{
		// get visible height
		const int clientHeight = GetClientSize().y;
		const int numVisLines = clientHeight / m_style.m_lineHeight;
		const int maxVisibleLine = wxMax(0, (int)m_visibleMessages.size() - numVisLines);

		// clamp
		if (line < 0)
		{
			line = 0;
		}
		else if (line > maxVisibleLine)
		{
			line = maxVisibleLine;
		}

		// change
		if (line != m_firstVisibleLine)
		{
			m_firstVisibleLine = line;
			SetScrollPos(wxVERTICAL, m_firstVisibleLine, true);
			Refresh();
		}
	}

	void LogDisplayBuffer::UpdateScroll()
	{
		// clamp the range
		if (m_firstVisibleLine <= 0)
		{
			m_firstVisibleLine = 0;
		}
		else if (m_firstVisibleLine >= (int)m_visibleMessages.size())
		{
			m_firstVisibleLine = (int)m_visibleMessages.size() - 1;
		}

		// get visible height
		const int clientHeight = GetClientSize().y;
		const int numVisLines = (clientHeight / m_style.m_lineHeight) - 1;

		// update the scrollbar
		const int lastScrollLine = wxMax(0, (int)m_visibleMessages.size());
		SetScrollbar(wxVERTICAL, m_firstVisibleLine, numVisLines, lastScrollLine, true);

		// scroll to the end
		if (m_lockScroll)
		{
			if ((int)m_visibleMessages.size() > numVisLines)
			{
				m_firstVisibleLine = m_visibleMessages.size() - numVisLines;
			}
			else
			{
				m_firstVisibleLine = 0;
			}
		}
	}

	inline static bool ParseHexValue(const char* start, uint32& outValue, uint32& outLength)
	{
		char digits[10];
		uint32 numDigits = 0;

		// search for the 'h' at the end - also, validate characters
		bool validNubmer = false;
		for (uint32 i = 0; i < sizeof(digits); ++i)
		{
			const char ch = start[i];
			if (ch == 'h')
			{
				if (numDigits != 0) validNubmer = true;
				break;
			}

			// grab digits
			char value = 0;
			if (ch == '0') value = 0;
			else if (ch == '1') value = 1;
			else if (ch == '2') value = 2;
			else if (ch == '3') value = 3;
			else if (ch == '4') value = 4;
			else if (ch == '5') value = 5;
			else if (ch == '6') value = 6;
			else if (ch == '7') value = 7;
			else if (ch == '8') value = 8;
			else if (ch == '9') value = 9;
			else if (ch == 'A') value = 10;
			else if (ch == 'a') value = 10;
			else if (ch == 'B') value = 11;
			else if (ch == 'b') value = 11;
			else if (ch == 'C') value = 12;
			else if (ch == 'c') value = 12;
			else if (ch == 'D') value = 13;
			else if (ch == 'd') value = 13;
			else if (ch == 'E') value = 14;
			else if (ch == 'e') value = 14;
			else if (ch == 'F') value = 15;
			else if (ch == 'f') value = 15;
			else
			{
				break;
			}

			// add to digit list
			digits[numDigits] = value;
			numDigits += 1;
		}

		// not a valid number
		if (!validNubmer || numDigits < 5)
		{
			return false;
		}

		// convert to number
		uint32 val = 0;
		for (uint32 i = 0; i < numDigits; ++i)
		{
			val = (val << 4) + digits[i];
		}

		// return final value
		outValue = val;
		outLength = numDigits + 1;
		return true;
	}

	inline static bool ParseTraceValue(const char* start, uint32& outValue, uint32& outLength)
	{
		char digits[10];
		uint32 numDigits = 0;

		// must start with '#'
		if (start[0] != '#')
			return false;

		// search for the 'h' at the end - also, validate characters
		for (uint32 i = 1; i < sizeof(digits); ++i)
		{
			const char ch = start[i];

			// grab digits
			char value = 0;
			if (ch == '0') value = 0;
			else if (ch == '1') value = 1;
			else if (ch == '2') value = 2;
			else if (ch == '3') value = 3;
			else if (ch == '4') value = 4;
			else if (ch == '5') value = 5;
			else if (ch == '6') value = 6;
			else if (ch == '7') value = 7;
			else if (ch == '8') value = 8;
			else if (ch == '9') value = 9;
			else
			{
				break;
			}

			// add to digit list
			digits[numDigits] = value;
			numDigits += 1;
		}

		// not a valid number
		if (numDigits < 5)
		{
			return false;
		}

		// convert to number
		uint32 val = 0;
		for (uint32 i = 0; i < numDigits; ++i)
		{
			val = (val * 10) + digits[i];
		}

		// return final value
		outValue = val;
		outLength = numDigits + 1;
		return true;
	}

	class wxAddressData : public wxObject
	{
	public:
		uint32	m_address;

	public:
		wxAddressData(const uint32 data)
			: m_address(data)
		{}
	};

	void LogDisplayBuffer::OnMouseDown(wxMouseEvent& event)
	{
		SetFocus();

		if (event.LeftDClick())
		{
			const int lineIndex = m_firstVisibleLine + (event.GetPosition().y / m_style.m_lineHeight);
			if (lineIndex < (int)m_visibleMessages.size())
			{
				// get the clicked line
				const char* lineText = m_visibleMessages[lineIndex]->m_text.c_str();
				if (lineText[0])
				{
					static const uint32 MAX_ADDRESSES = 16;
					uint32 addresses[MAX_ADDRESSES];
					uint32 numAdresses = 0;
					uint32 trace[MAX_ADDRESSES];
					uint32 numTrace = 0;

					// search the text for the numerical address
					const char* txt = lineText;
					const char* txtEnd = lineText + strlen(lineText);
					while (txt < txtEnd)
					{
						uint32 value = 0, length = 0;
						if (ParseHexValue(txt, value, length))
						{
							if (numAdresses == MAX_ADDRESSES) break;
							addresses[numAdresses] = value;
							numAdresses += 1;
							txt += length;
						}
						else if (ParseTraceValue(txt, value, length))
						{
							if (numTrace == MAX_ADDRESSES) break;
							trace[numTrace] = value;
							numTrace += 1;
							txt += length;
						}
						else
						{
							txt += 1;
						}
					}

					// exactly one address - navigate directly
					if (numAdresses == 1 && numTrace == 0)
					{
						const uint32 address = addresses[0];
						//wxTheFrame->NavigateToAddress(address);
					}
					else if (numTrace == 1 && numAdresses == 0)
					{
						const uint32 traceIndex = trace[0];
						//wxTheFrame->NavigateToTraceEntry(traceIndex);
					}
					else
					{
						wxMenu popupMenu;

						for (uint32 i = 0; i < numAdresses; ++i)
						{
							char buf[32];
							sprintf_s(buf, "Address: %08Xh", addresses[i]);
							popupMenu.Append(1000 + i, buf);
							popupMenu.Connect(1000 + i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(LogDisplayBuffer::OnNavigateToAddress), new wxAddressData(addresses[i]), this);
						}

						if (numAdresses && numTrace)
							popupMenu.AppendSeparator();

						for (uint32 i = 0; i < numTrace; ++i)
						{
							char buf[32];
							sprintf_s(buf, "Trace: #%u", trace[i]);
							popupMenu.Append(2000 + i, buf);
							popupMenu.Connect(2000 + i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(LogDisplayBuffer::OnNavigateToTrace), new wxAddressData(trace[i]), this);
						}

						PopupMenu(&popupMenu);
					}
				}
			}
		}
	}

	void LogDisplayBuffer::OnMouseUp(wxMouseEvent& event)
	{
	}

	void LogDisplayBuffer::OnMouseWheel(wxMouseEvent& event)
	{
		int shift = 1;

		if (wxGetKeyState(WXK_SHIFT))
		{
			shift = 10;
		}

		if (event.GetWheelRotation() < 0)
		{
			SetFirstVisible(m_firstVisibleLine + shift);
		}
		else if (event.GetWheelRotation() > 0)
		{
			SetFirstVisible(m_firstVisibleLine - shift);
		}
	}

	void LogDisplayBuffer::OnPaint(wxPaintEvent& event)
	{
		wxBufferedPaintDC dc(this);
		PrepareDC(dc);

		// size of the DC
		const wxSize size = GetClientSize();

		// set font
		dc.SetFont(m_style.m_font);

		// start drawing
		int lineY = 0;
		int lineIndex = m_firstVisibleLine;
		while (lineIndex < (int)m_visibleMessages.size() && lineY <= size.y)
		{
			const LogMessage* msg = m_visibleMessages[lineIndex];

			// is the log line selected
			const int isSelected = 0;

			// get drawing style
			const int style = (int)msg->m_type;

			// compute line rect
			const wxRect lineRect(0, lineY, size.x, m_style.m_lineHeight);
			dc.SetBrush(m_style.m_backBrush[style][isSelected]);
			dc.SetPen(m_style.m_backPen[style][isSelected]);
			dc.SetTextBackground(m_style.m_textBrush[style][isSelected].GetColour());
			dc.SetTextForeground(m_style.m_textPen[style][isSelected].GetColour());
			dc.DrawRectangle(lineRect);

			// draw with clipping
			const int textY = lineY + 2;
			dc.DrawText(msg->m_text, 10, textY);

			// go to next line
			lineY += m_style.m_lineHeight;
			lineIndex += 1;
		}

		// clear the rest
		if (lineY < size.y)
		{
			dc.SetBrush(m_style.m_backBrush[(uint8)MessageType::Info][0]);
			dc.SetPen(m_style.m_backPen[(uint8)MessageType::Info][0]);
			dc.DrawRectangle(0, lineY, size.x, size.y - lineY + 1);
		}
	}

	void LogDisplayBuffer::OnErase(wxEraseEvent& event)
	{
		// nothing
	}

	void LogDisplayBuffer::OnKeyDown(wxKeyEvent& event)
	{
		if (event.GetKeyCode() == WXK_UP)
		{
			SetFirstVisible(m_firstVisibleLine - 1);
		}
		else if (event.GetKeyCode() == WXK_DOWN)
		{
			SetFirstVisible(m_firstVisibleLine + 1);
		}
		else if (event.GetKeyCode() == WXK_PAGEUP)
		{
			const int numLinesPerPage = GetClientSize().y / m_style.m_lineHeight;
			SetFirstVisible(m_firstVisibleLine - numLinesPerPage);
		}
		else if (event.GetKeyCode() == WXK_PAGEDOWN)
		{
			const int numLinesPerPage = GetClientSize().y / m_style.m_lineHeight;
			SetFirstVisible(m_firstVisibleLine + numLinesPerPage);
		}
		else if (event.GetKeyCode() == WXK_HOME)
		{
			SetFirstVisible(0);
		}
		else if (event.GetKeyCode() == WXK_END)
		{
			SetFirstVisible(m_visibleMessages.size());
		}
		else
		{
			event.Skip();
		}
	}

	void LogDisplayBuffer::OnScroll(wxScrollWinEvent& event)
	{
		SetFirstVisible(event.GetPosition());
	}

	void LogDisplayBuffer::OnSize(wxSizeEvent& event)
	{
		UpdateScroll();
		Refresh();
		event.Skip();
	}

	void LogDisplayBuffer::OnSetCursor(wxSetCursorEvent& event)
	{
		event.Skip();
	}

	void LogDisplayBuffer::OnNavigateToAddress(wxCommandEvent& event)
	{
		wxAddressData* data = static_cast<wxAddressData*>(event.m_callbackUserData);
		//wxTheFrame->NavigateToAddress(data->m_address);
	}

	void LogDisplayBuffer::OnNavigateToTrace(wxCommandEvent& event)
	{
		wxAddressData* data = static_cast<wxAddressData*>(event.m_callbackUserData);
		//wxTheFrame->NavigateToTraceEntry(data->m_address);
	}

	//--

	BEGIN_EVENT_TABLE(LogWindow, wxFrame)
		EVT_BUTTON(XRCID("LogClear"), LogWindow::OnClearLog)
		EVT_BUTTON(XRCID("LogCopy"), LogWindow::OnCopyLog)
		EVT_BUTTON(XRCID("LogSave"), LogWindow::OnSaveLog)
		EVT_TOOL(XRCID("LogFilterErrors"), LogWindow::OnToggleFilters)
		EVT_TOOL(XRCID("LogFilterWarnings"), LogWindow::OnToggleFilters)
		EVT_TOOL(XRCID("LogFilterInfo"), LogWindow::OnToggleFilters)
	END_EVENT_TABLE()

	LogWindow::LogWindow()
	{
		// load the window
		wxXmlResource::Get()->LoadFrame(this, nullptr, wxT("LogWindow"));

		// create the display buffer
		{
			auto* panel = XRCCTRL(*this, "LogPanel", wxPanel);
			m_displayBuffer = new LogDisplayBuffer(panel);
			panel->SetSizer(new wxBoxSizer(wxVERTICAL));
			panel->GetSizer()->Add(m_displayBuffer, 1, wxEXPAND, 0);
		}

		// layout the elements
		SetSize(800, 800);
		SetMinSize(wxSize(600, 400));
		Layout();

		// refresh filter state
		{
			bool showErrors = true, showWarnings = true, showInfos = true;
			m_displayBuffer->GetFilterState(showErrors, showWarnings, showInfos);

			auto* toolBar = XRCCTRL(*this, "Toolbar", wxToolBar);
			toolBar->ToggleTool(XRCID("LogFilterErrors"), showErrors);
			toolBar->ToggleTool(XRCID("LogFilterWarnings"), showWarnings);
			toolBar->ToggleTool(XRCID("LogFilterInfo"), showInfos);
		}

		// Hide when user tries to close this window
		Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& evt) { Hide(); evt.Veto(); });
	}

	void LogWindow::DoLog(const LogLevel level, const char* buffer)
	{
		auto type = LogDisplayBuffer::MessageType::Info;
		if (level == ILogOutput::LogLevel::Warning)
		{
			type = LogDisplayBuffer::MessageType::Warning;
		}
		else if (level == ILogOutput::LogLevel::Error)
		{
			type = LogDisplayBuffer::MessageType::Error;
		}

		m_displayBuffer->Print(type, buffer);
	}

	void LogWindow::OnClearLog(wxCommandEvent& evt)
	{
		m_displayBuffer->Clear();
	}

	void LogWindow::OnCopyLog(wxCommandEvent& evt)
	{
		// get the log text
		wxString logString;
		m_displayBuffer->Extract(logString);

		// put it in clipboard
		wxClipboard *clip = new wxClipboard();
		if (clip->Open())
		{
			clip->Clear();
			clip->SetData(new wxTextDataObject(logString));
			clip->Flush();
			clip->Close();
		}
	}

	void LogWindow::OnSaveLog(wxCommandEvent& evt)
	{
		// select file to save to
		wxFileDialog saveFileDialog(this, "Save log file", "", "log.txt", "Log  files (*.txt)|*.txt", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
		if (saveFileDialog.ShowModal() == wxID_CANCEL)
			return;

		// get text
		wxString logString;
		m_displayBuffer->Extract(logString);

		// save the current contents in the file;
		// this can be done with e.g. wxWidgets output streams:
		wxFileOutputStream output_stream(saveFileDialog.GetPath());
		if (!output_stream.IsOk())
		{
			wxMessageBox("Cannot open selected file for writing", "File error", wxICON_ERROR);
			return;
		}

		if (!output_stream.WriteAll(logString.c_str(), logString.length()))
		{
			wxMessageBox("Failed to write to selected file", "File error", wxICON_ERROR);
			return;
		}		
	}

	void LogWindow::RefreshFilters()
	{
		const auto* toolBar = XRCCTRL(*this, "Toolbar", wxToolBar);
		const bool showErrors = toolBar->GetToolState(XRCID("LogFilterErrors"));
		const bool showWarnings = toolBar->GetToolState(XRCID("LogFilterWarnings"));
		const bool showInfos = toolBar->GetToolState(XRCID("LogFilterInfo"));
		m_displayBuffer->SetFilterState(showErrors, showWarnings, showInfos);
	}

	void LogWindow::OnToggleFilters(wxCommandEvent& evt)
	{
		RefreshFilters();
	}

	//--

} // tools