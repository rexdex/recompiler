#pragma once

namespace tools
{
	/// Long function to run under a progress dialog
	typedef std::function<int32(ILogOutput& log)> TLongFunction;

	/// Dialog to show when waiting for task group (or groups) to be finishes
	class ProgressDialog : public wxDialog, public ILogOutput, public wxThreadHelper
	{
		DECLARE_EVENT_TABLE();

	public:
		ProgressDialog(wxWindow* parent, ILogOutput& parentLog, const bool canCancel = false);
		~ProgressDialog();

		// show the progress dialog in a modal way
		int32 RunLongTask(const TLongFunction& func);

	private:
		virtual void DoSetTaskName(const char* buffer) override final;
		virtual void DoSetTaskProgress(int count, int max) override final;
		virtual bool DoIsTaskCanceled() override final;

		void Refresh();

		void OnCancelTask(wxCommandEvent& evt);
		void OnRefresh(wxTimerEvent& evt);

		// request closing of the progress dialog, can be called from any thread
		void RequestEnd(const int32 returnValue);

		// thread processing function
		virtual wxThread::ExitCode Entry() override final;

		//---

		wxGauge* m_progressBar;
		wxStaticText* m_progressText;
		wxStaticText* m_timeText;

		//---

		// last progress data displayed
		std::atomic<bool> m_currentProgressTextChanged;
		std::string	m_currentProgressText;
		std::atomic<bool> m_currentProgressValueChanged;
		float m_currentProgressValue;
		int m_currentTimeSeconds;
		bool m_canCancel;

		// cancel button was pressed
		std::atomic<bool> m_canceled;

		// closing of the dialog was requested
		std::atomic<bool> m_requestEnd;
		std::atomic<int32> m_requestExitCode;

		// processing function
		TLongFunction m_function;

		// time since start
		wxStopWatch m_timeCounter;

		// timer used for the refect
		wxTimer m_refreshTimer;

		// holds the active text and active progress value
		wxCriticalSection m_mutex;
	};

} // tools