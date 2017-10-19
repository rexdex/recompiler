#include "build.h"
#include "buildDialog.h"

namespace tools
{

	//--

	BuildDialog::TaskProcess::TaskProcess()
		: m_hasFinished(false)
		, m_exitCode(0)
	{
		m_timer.Start();
	}

	void BuildDialog::TaskProcess::OnTerminate(int pid, int status)
	{
		m_timer.Pause();
		m_hasFinished = true;
		m_exitCode = status;
	}

	//--

	BEGIN_EVENT_TABLE(BuildDialog, wxDialog)
		EVT_BUTTON(XRCID("Cancel"), BuildDialog::OnCancel)
		EVT_BUTTON(XRCID("CopyLog"), BuildDialog::OnCopyLog)
		EVT_BUTTON(XRCID("CopyCommands"), BuildDialog::OnCopyCommands)
		EVT_TIMER(wxID_ANY, BuildDialog::OnRefresh)
	END_EVENT_TABLE()

	BuildDialog::BuildDialog(wxWindow* parent)
		: m_currentTask(0)
		, m_currentProcess(nullptr)
		, m_currentTimeSeconds(0)
		, m_hasFailed(false)
		, m_refreshTimer(this)
	{
		// load the window
		wxXmlResource::Get()->LoadDialog(this, nullptr, wxT("BuildDialog"));

		// create the display buffer
		{
			auto* panel = XRCCTRL(*this, "LogPanel", wxPanel);
			m_displayBuffer = new LogDisplayBuffer(panel);
			panel->SetSizer(new wxBoxSizer(wxVERTICAL));
			panel->GetSizer()->Add(m_displayBuffer, 1, wxEXPAND, 0);
		}

		// get the ui widgets
		m_taskList = XRCCTRL(*this, "TaskList", wxListCtrl);
		m_elapsedTimeText = XRCCTRL(*this, "Time", wxStaticText);

		// setup columns
		m_taskList->AppendColumn("Task name", wxLIST_FORMAT_LEFT, 150);
		m_taskList->AppendColumn("State", wxLIST_FORMAT_CENTER, 100);
		m_taskList->AppendColumn("Time", wxLIST_FORMAT_CENTER, 100);
		m_taskList->AppendColumn("Command line", wxLIST_FORMAT_LEFT, 1000);
	}

	const bool BuildDialog::RunCommands(const std::vector<BuildTask>& tasks)
	{
		// nothing to run
		if (tasks.empty())
			return true;

		// setup tasks
		m_tasks = tasks;
		m_currentTask = 0;
		m_currentProcess = nullptr;
		m_hasFailed = false;

		// setup list of processes
		{
			m_taskList->Freeze();
			m_taskList->DeleteAllItems();

			for (const auto& task : m_tasks)
			{
				auto itemIndex = m_taskList->InsertItem(m_taskList->GetItemCount(), task.m_displayName);
				m_taskList->SetItem(itemIndex, 1, "Pending...");
				m_taskList->SetItem(itemIndex, 2, "--:--");
				m_taskList->SetItem(itemIndex, 3, task.m_command + wxString(" ") + task.m_commandLine);
			}

			m_taskList->Thaw();
			m_taskList->Refresh();
		}

		// start the first task
		StartTaskProcess();

		// start timing
		m_timeCounter.Start();
		m_refreshTimer.Start(10, false);

		// show dialog
		ShowModal();

		// return status flag
		return !m_hasFailed;
	}

	void BuildDialog::TextBuffer::Append(wxInputStream& stream, LogDisplayBuffer* outLog, const LogDisplayBuffer::MessageType messageType)
	{
		while (stream.CanRead())
		{
			char buffer[1024];
			stream.Read(buffer, sizeof(buffer));

			const auto size = stream.LastRead();
			for (size_t i = 0; i < size; ++i)
			{
				const auto ch = buffer[i];
				if (ch == '\r')
					continue;

				if (ch == '\n')
				{
					m_bufferedChars.push_back(0);
					outLog->Print(messageType, m_bufferedChars.data());
					m_bufferedChars.clear();
				}
				else
				{
					m_bufferedChars.push_back(ch);
				}
			}
		}
	}

	void BuildDialog::StartTaskProcess()
	{
		wxASSERT(m_currentProcess == nullptr);

		// end of tasks
		if (m_currentTask == m_tasks.size())
		{
			const auto curTime = m_timeCounter.Time() / 1000;
			wxMessageBox(wxString::Format(wxT("All tasks finished\nTotal time: %u min and %u seconds"), curTime / 60, curTime % 60), wxT("Build"), wxICON_INFORMATION, this);
			XRCCTRL(*this, "Cancel", wxButton)->SetLabelText("Close");
			return;
		}

		// create wrapper
		m_currentProcess = new TaskProcess();
		m_currentProcess->Redirect();

		// change state
		m_taskList->SetItem(m_currentTask, 1, "Running...");

		// get the command line to execute and run
		const auto& tasks = m_tasks[m_currentTask];
		wxString command = tasks.m_command + wxString(" ") + tasks.m_commandLine;
		const auto pid = wxExecute(command, wxEXEC_ASYNC, m_currentProcess, nullptr);
		if (pid == 0)
		{
			m_taskList->SetItem(m_currentTask, 1, "Failed to start");
			m_displayBuffer->Print(LogDisplayBuffer::MessageType::Error, wxString::Format("Failed to start process '%s'", tasks.m_displayName.c_str()).c_str());

			wxMessageBox(wxT("Failed to start build process"), wxT("Build error"), wxICON_ERROR, this);
			m_currentTask = m_tasks.size();
			m_hasFailed = true;
			XRCCTRL(*this, "Cancel", wxButton)->SetLabelText("Close");
			return;
		}

		// process started
		m_taskList->SetItem(m_currentTask, 1, "Running...");
		m_displayBuffer->Print(LogDisplayBuffer::MessageType::Info, wxString::Format("Process '%s' started", tasks.m_displayName.c_str()).c_str());
	}

	void BuildDialog::OnRefresh(wxTimerEvent& evt)
	{
		// update the time counter
		if (IsRunning())
		{
			const auto curTime = m_timeCounter.Time() / 1000;
			if (m_currentTimeSeconds != curTime)
			{
				const auto mins = curTime / 60;
				const auto secs = curTime % 60;
				m_elapsedTimeText->SetLabel(wxString::Format("Elapsed time: %02u:%02u", mins, secs));
			}
		}

		// update the process state
		if (m_currentProcess)
		{
			// extract normal log
			if (m_currentProcess->GetInputStream())
				m_logOutput.Append(*m_currentProcess->GetInputStream(), m_displayBuffer, LogDisplayBuffer::MessageType::Info);

			// extract error log
			if (m_currentProcess->GetErrorStream())
				m_logOutput.Append(*m_currentProcess->GetErrorStream(), m_displayBuffer, LogDisplayBuffer::MessageType::Error);

			// update process running time
			{
				const auto executionTime = (uint32_t)m_currentProcess->GetElapsedTime() / 1000;
				m_taskList->SetItem(m_currentTask, 2, wxString::Format("%02u:%02u", executionTime / 60, executionTime % 60));
			}

			// handle process termination
			if (m_currentProcess->HasFinished())
			{
				// print the tail
				const auto exitCode = m_currentProcess->GetExitCode();
				const auto executionTime = m_currentProcess->GetElapsedTime() / 1000.0f;
				if (exitCode == 0)
				{
					m_displayBuffer->Print(LogDisplayBuffer::MessageType::Info, wxString::Format("Process finished after %1.2fs", executionTime).c_str());
					m_taskList->SetItem(m_currentTask, 1, "Done");
				}
				else
				{ 
					m_displayBuffer->Print(LogDisplayBuffer::MessageType::Error, wxString::Format("Process failed after %1.2fs with exit code %d", executionTime, exitCode).c_str());
					m_taskList->SetItem(m_currentTask, 1, "Failed");
					m_hasFailed = true;
				}

				// delete wrapper
				delete m_currentProcess;
				m_currentProcess = nullptr;

				// advance to next task
				if (exitCode == 0)
				{
					m_currentTask += 1;
					StartTaskProcess();
				}
				else
				{
					m_currentTask = m_tasks.size();
					wxMessageBox(wxT("Build failed"), wxT("Build"), wxICON_ERROR, this);
					XRCCTRL(*this, "Cancel", wxButton)->SetLabelText("Close");
				}							
			}
		}
	}

	void BuildDialog::OnCancel(wxCommandEvent& evt)
	{
		if (IsRunning())
		{
			if (wxNO == wxMessageBox(wxT("Are you sure you want to cancel pending tasks?"), wxT("Stop build process"), wxYES_NO | wxICON_QUESTION, this))
				return;
		}

		if (m_currentProcess)
		{
			m_displayBuffer->Print(LogDisplayBuffer::MessageType::Error, wxString::Format("Process killed by user"));
			m_hasFailed = true;

			wxKillError rc = wxProcess::Kill(m_currentProcess->GetPid(), wxSIGTERM, wxKILL_CHILDREN);
			if (rc != wxKILL_OK)
			{
				wxMessageBox(wxT("Unable to kill running process"), wxT("Stop build"), wxICON_ERROR, this);
				return;
			}

			delete m_currentProcess;
			m_currentProcess = nullptr;
			m_currentTask = m_tasks.size();
		}

		EndDialog(-1);
	}

	void BuildDialog::OnCopyLog(wxCommandEvent& evt)
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

	void BuildDialog::OnCopyCommands(wxCommandEvent& evt)
	{
		// get the commands
		wxString commandString;
		for (const auto& task : m_tasks)
		{
			commandString += task.m_command;
			commandString += " ";
			commandString += task.m_commandLine;
			commandString += wxTextFile::GetEOL();
		}
		
		// put it in clipboard
		wxClipboard *clip = new wxClipboard();
		if (clip->Open())
		{
			clip->Clear();
			clip->SetData(new wxTextDataObject(commandString));
			clip->Flush();
			clip->Close();
		}
	}

} // tools
