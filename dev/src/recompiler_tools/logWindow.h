#pragma once

#include "eventListener.h"

namespace tools
{
	/// generic thread safe log window
	class LogDisplayBuffer : public wxWindow, public IEventListener
	{
		DECLARE_EVENT_TABLE();

	public:
		LogDisplayBuffer(wxWindow* parent);
		~LogDisplayBuffer();

		enum class MessageType
		{
			CommandLine=0,
			Info,
			Warning,
			Error,
			ScriptInfo,
			ScriptWarning,
			ScriptError,
			MAX,
		};

		// clear log window
		void Clear();

		// extract text from the log window
		void Extract(wxString& outString) const;

		// lock/unlock automatic scrolling with new content
		void SetScrollLock(bool lockScroll);

		// get filter state
		void GetFilterState(bool& outShowErrors, bool& outShowWarnings, bool& outShowInfos) const;

		// set filter state
		void SetFilterState(const bool showErrors, const bool showWarnings, const bool showInfos);

		// print log message
		void Print(const MessageType type, const char* txt);

	private:
		// update the window scrolling position
		void UpdateScroll();

		// set the new first visible line
		void SetFirstVisible(int line);

		// execute the command line string
		void FlushPendingLogLines();

		//---

		void OnMouseDown(wxMouseEvent& event);
		void OnMouseUp(wxMouseEvent& event);
		void OnMouseWheel(wxMouseEvent& event);
		void OnPaint(wxPaintEvent& event);
		void OnErase(wxEraseEvent& event);
		void OnKeyDown(wxKeyEvent& event);
		void OnScroll(wxScrollWinEvent& event);
		void OnSize(wxSizeEvent& event);
		void OnSetCursor(wxSetCursorEvent& event);

		void OnNavigateToAddress(wxCommandEvent& event);
		void OnNavigateToTrace(wxCommandEvent& event);

		//---

		struct LogMessage
		{
			MessageType m_type;
			wxLongLong m_timestamp;
			wxString m_text;

			static wxStopWatch*	st_timer;

			LogMessage(const MessageType type, const char* txt);
			~LogMessage();

			static LogMessage* Build(const MessageType type, const char* txt);
		};

		//! Drawing style
		struct DrawingStyle
		{
			wxFont		m_font;
			wxBrush		m_backBrush[(uint8)MessageType::MAX][2];
			wxPen		m_backPen[(uint8)MessageType::MAX][2];
			wxBrush		m_textBrush[(uint8)MessageType::MAX][2];
			wxPen		m_textPen[(uint8)MessageType::MAX][2];

			uint32		m_lineHeight;
			uint32		m_moduleOffset;

			DrawingStyle();
		};

		//! Drawing style data
		DrawingStyle		m_style;

		typedef std::vector< LogMessage* > TMessages;
		TMessages m_allMessages;
		TMessages m_visibleMessages;

		TMessages m_pendingMessages;
		wxCriticalSection m_pendingMessagesLock;

		wxThreadIdType m_mainThreadId;

		typedef std::vector< LogDisplayBuffer* > TLogWindows;
		static TLogWindows st_logWindows;
		static wxCriticalSection st_logWindowsLock;

		const bool FilterMessage(const LogMessage& msg) const;

		bool m_lockScroll;
		bool m_errorsVisible;
		bool m_warningsVisible;
		bool m_infosVisible;
		int m_firstVisibleLine;
	};

	// logging frame
	class LogWindow : public wxFrame, public ILogOutput
	{
		DECLARE_EVENT_TABLE();

	public:
		LogWindow();

	public:
		LogDisplayBuffer* m_displayBuffer;

		// ILogOutput
		virtual void DoLog(const LogLevel level, const char* buffer) override final;

	private:
		void OnClearLog(wxCommandEvent& evt);
		void OnCopyLog(wxCommandEvent& evt);
		void OnSaveLog(wxCommandEvent& evt);
		void OnToggleFilters(wxCommandEvent& evt);

		void RefreshFilters();
	};

} // tools