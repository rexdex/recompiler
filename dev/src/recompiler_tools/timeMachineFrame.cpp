#include "build.h"
#include "timeMachineView.h"
#include "timeMachineFrame.h"
#include "project.h"
#include "projectTraceData.h"

namespace tools
{

	//---------------------------------------------------------------------------

	BEGIN_EVENT_TABLE(TimeMachineFrame, wxFrame)
		EVT_CLOSE(OnClose)
		END_EVENT_TABLE()

		//---------------------------------------------------------------------------

		TimeMachineFrame::TimeMachineFrame(wxWindow* parent)
	{
		wxXmlResource::Get()->LoadFrame(this, parent, wxT("TimeMachine"));

		// create notebook
		{
			wxPanel* tabPanel = XRCCTRL(*this, "TabsPanel", wxPanel);

			m_tabs = new wxAuiNotebook(tabPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_DEFAULT_STYLE & ~wxAUI_NB_CLOSE_BUTTON & ~wxAUI_NB_CLOSE_ON_ACTIVE_TAB);

			wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
			sizer->Add(m_tabs, 1, wxALL | wxEXPAND, 0);
			tabPanel->SetSizer(sizer);
			tabPanel->Layout();
		}

		// refresh layout
		Layout();
		Show();
	}

	TimeMachineFrame::~TimeMachineFrame()
	{
	}

	void TimeMachineFrame::CloseAllTabs()
	{
	}

	void TimeMachineFrame::CreateNewTrace(const uint32 traceFrame)
	{
		// look for existing entry
		const uint32 numTabs = m_tabs->GetPageCount();
		for (uint32 i = 0; i < numTabs; ++i)
		{
			const TimeMachineView* view = static_cast<const TimeMachineView*>(m_tabs->GetPage(i));
			if (view && view->GetRootTraceIndex() == traceFrame)
			{
				m_tabs->SetSelection(i);
				return;
			}
		}

		// no trace loaded
		/*if (!wxTheFrame->GetProject() || !wxTheFrame->GetProject()->GetTrace())
			return;*/

		// create time machine trace
		timemachine::Trace* trace = nullptr;// wxTheFrame->GetProject()->GetTrace()->CreateTimeMachineTrace(traceFrame);
		if (!trace)
			return;

		// create trace entry
		TimeMachineView* traceView = new TimeMachineView(m_tabs, trace);
		m_tabs->AddPage(traceView, wxString::Format("Trace #%05d", traceFrame), true);

		// show the window
		Layout();
		Show();
	}

	void TimeMachineFrame::OnClose(wxCloseEvent& event)
	{
		event.Veto();
		Hide();
	}

	//---------------------------------------------------------------------------

} // tools