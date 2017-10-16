#pragma once

namespace tools
{

	//-----------------------------------------------------------------------------

	class TimeMachineView;

	//-----------------------------------------------------------------------------

	/// Time machine helper shit
	class TimeMachineFrame : public wxFrame
	{
		DECLARE_EVENT_TABLE();

	public:
		TimeMachineFrame(wxWindow* parent);
		virtual ~TimeMachineFrame();

		// close all tabs
		void CloseAllTabs();

		// create new time machine view starting at given trace frame
		void CreateNewTrace(const uint32 traceFrame);

	private:
		wxAuiNotebook*							m_tabs;
		std::vector< TimeMachineView* >		m_views;

		// original project trace data - used to generate the time machine trace 
		const class ProjectTraceData*	m_traceData;

		// events
		void OnClose(wxCloseEvent& event);
	};

} // tools