#pragma once

namespace tools
{

	/// build task
	struct BuildTask
	{
		wxString m_displayName;
		wxString m_command;
		wxString m_commandLine;
	};

	/// run list of commands
	class BuildDialog : public wxDialog
	{
		DECLARE_EVENT_TABLE();

	public:
		BuildDialog(wxWindow* parent);

		/// run commands, returns false if commands failed or when user cancels the processing
		const bool RunCommands(const std::vector<BuildTask>& tasks);

	public:
		class TaskProcess : public wxProcess
		{
		public:
			TaskProcess();

			inline const bool HasFinished() const { return m_hasFinished; }
			inline const int32_t GetExitCode() const { return m_exitCode; }
			inline const uint32_t GetElapsedTime() const { return m_timer.Time(); }

		private:
			virtual void OnTerminate(int pid, int status) override final;

			wxStopWatch m_timer;
			bool m_hasFinished;
			int32_t m_exitCode;
		};

		struct TextBuffer
		{
			void Append(wxInputStream& stream, LogDisplayBuffer* outLog, const LogDisplayBuffer::MessageType messageType);
			std::vector<char> m_bufferedChars;
		};

		wxTimer m_refreshTimer;
		wxListCtrl* m_taskList;
		wxStaticText* m_elapsedTimeText;
		LogDisplayBuffer* m_displayBuffer;

		std::vector<BuildTask> m_tasks;
		uint32_t m_currentTask;
		TaskProcess* m_currentProcess;
		uint32_t m_currentTimeSeconds;
		bool m_hasFailed;

		TextBuffer m_logOutput;
		TextBuffer m_errorOutput;

		wxStopWatch m_timeCounter;

		const bool IsRunning() const { return m_currentTask < m_tasks.size(); }

		void StartTaskProcess();

		void OnRefresh(wxTimerEvent& evt);
		void OnCancel(wxCommandEvent& evt);
		void OnCopyLog(wxCommandEvent& evt);
		void OnCopyCommands(wxCommandEvent& evt);
	};

} // tools