#include "build.h"
#include "progressDialog.h"
#include "resource.h"

namespace tools
{
	BEGIN_EVENT_TABLE(ProgressDialog, wxDialog)
		EVT_BUTTON(XRCID("Cancel"), ProgressDialog::OnCancelTask)
		EVT_TIMER(wxID_ANY, ProgressDialog::OnRefresh)
	END_EVENT_TABLE();

	ProgressDialog::ProgressDialog(wxWindow* parent, ILogOutput& parentLog, const bool canCancel /*= false*/)
		: ILogOutput(&parentLog)
		, m_currentProgressTextChanged(false)
		, m_currentProgressValueChanged(false)
		, m_currentProgressText("Please wait...")
		, m_currentProgressValue(0.0f)
		, m_currentTimeSeconds(0)
		, m_canceled(false)
		, m_canCancel(canCancel)
		, m_refreshTimer(this)
		, m_requestEnd(false)
		, m_requestExitCode(0)
	{
		// load the dialog
		wxXmlResource::Get()->LoadDialog(this, parent, wxT("ProgressDialog"));

		// get elements
		m_progressBar = XRCCTRL(*this, "ProgressBar", wxGauge);
		m_progressText = XRCCTRL(*this, "Label", wxStaticText);
		m_timeText = XRCCTRL(*this, "Time", wxStaticText);

		// enable the cancel button
		XRCCTRL(*this, "Cancel", wxButton)->Enable(m_canCancel);

		// start timing
		m_timeCounter.Start();
		m_refreshTimer.Start(10, false);
	}

	ProgressDialog::~ProgressDialog()
	{}

	int32 ProgressDialog::RunLongTask(const TLongFunction& func)
	{
		m_function = func;
		m_requestEnd.exchange(false);
		m_timeCounter.Start();

		if (wxThreadHelper::CreateThread() != wxTHREAD_NO_ERROR)
		{
			Log("App: Failed to create background thread for a long operation");
			return -1;
		}

		if (GetThread()->Run() != wxTHREAD_NO_ERROR)
		{
			Log("App: Failed to start background thread for a long operation");
			return -1;
		}

		ShowModal();

		return m_requestExitCode;
	}

	void ProgressDialog::RequestEnd(const int32 returnValue)
	{
		m_requestEnd.exchange(true);
	}

	void ProgressDialog::DoSetTaskName(const char* buffer)
	{
		m_mutex.Enter();
		if (m_currentProgressText != buffer)
		{
			m_currentProgressText = buffer;
			m_currentProgressTextChanged.exchange(true);
		}
		m_mutex.Leave();
	}

	void ProgressDialog::DoSetTaskProgress(int count, int max)
	{
		const float value = (max > 0) ? std::max<float>(0.0f, std::min<float>(1.0f, (float)count / (float)max)) : 0.0f;

		m_mutex.Enter();
		if (value != m_currentProgressValue)
		{
			m_currentProgressValue = value;
			m_currentProgressValueChanged.exchange(true);
		}
		m_mutex.Leave();
	}

	bool ProgressDialog::DoIsTaskCanceled()
	{
		return m_canceled;
	}

	void ProgressDialog::Refresh()
	{
		// update progress value
		if (m_currentProgressValueChanged.exchange(false))
		{
			m_mutex.Enter();
			const auto newProgress = m_currentProgressValue;
			m_mutex.Leave();
			m_progressBar->SetValue((int)(newProgress * 1000));
		}

		// update progress text
		if (m_currentProgressTextChanged.exchange(false))
		{
			m_mutex.Enter();
			const auto newText = m_currentProgressText;
			m_mutex.Leave();
			m_progressText->SetLabel(newText);
		}

		// timer
		const auto curTime = m_timeCounter.Time() / 1000;
		if (m_currentTimeSeconds != curTime)
		{
			const auto mins = curTime / 60;
			const auto secs = curTime % 60;
			m_timeText->SetLabel(wxString::Format("Elapsed time: %02u:%02u", mins, secs));
		}

		// close the dialog
		if (m_requestEnd)
		{
			EndDialog(0);
		}
	}

	void ProgressDialog::OnCancelTask(wxCommandEvent& evt)
	{
		if (m_canCancel)
		{
			if (!m_canceled.exchange(true))
			{
				auto* button = XRCCTRL(*this, "Cancel", wxButton);
				button->Enable(false);
				button->SetLabel("Canceling...");
			}
		}
	}

	void ProgressDialog::OnRefresh(wxTimerEvent& evt)
	{
		Refresh();
	}

	wxThread::ExitCode ProgressDialog::Entry()
	{
		const auto ret = m_function(*this);
		RequestEnd(ret);
		return 0;
	}


} // tools