#pragma once

#include "projectWindow.h"
#include "projectMemoryView.h"
#include "timeMachineView.h"
#include "../recompiler_core/traceUtils.h"

namespace tools
{
	class MemoryView;
	class ImageMemoryView;
	class TraceInfoView;
	class RegisterView;

	/// tab for the project trace 
	class ProjectTraceTab : public ProjectTab, public IImageMemoryNavigationHelper, public ITimeMachineViewNavigationHelper
	{
		DECLARE_EVENT_TABLE();

	public:
		ProjectTraceTab(ProjectWindow* parent, wxWindow* tabs, std::unique_ptr<trace::DataFile>& traceData);
		virtual ~ProjectTraceTab();

		// get the trace data
		inline trace::DataFile& GetData() { return *m_data; }

		//--

		// navigate to trace entry
		bool NavigateToFrame(const TraceFrameID seq);

		// navigate to start of the trace
		bool NavigateToStart();

		// navigate to end of the trace
		bool NavigateToEnd();

	private:
		Project* m_project;

		// native trace file
		std::unique_ptr<trace::DataFile> m_data;

		// current trace entry
		TraceFrameID m_currentEntry;
		uint64 m_currentAddress;
		std::shared_ptr<ProjectImage> m_currentImage;

		// current image disassembly
		ImageMemoryView* m_disassemblyView;
		MemoryView* m_disassemblyPanel;

		// ui refresh timer
		wxTimer m_refreshTimer;

		// address history
		ProjectAdddrsesHistory m_addressHistory;

		// trace views
		TraceInfoView* m_traceInfoView;

		// register view
		wxNotebook* m_registerViewsTabs;
		std::vector<RegisterView*> m_registerViews;

		// time machine tabs
		wxAuiNotebook* m_timeMachineTabs;

		//--

		void OnRefreshTimer(wxTimerEvent & evt);
		void OnFindSymbol(wxCommandEvent& evt);
		void OnGoToAddress(wxCommandEvent& evt);
		void OnTraceToStart(wxCommandEvent& evt);
		void OnTraceToEnd(wxCommandEvent& evt);
		void OnTraceFreeRun(wxCommandEvent& evt);
		void OnTraceGlobalPrev(wxCommandEvent& evt);
		void OnTraceLocalPrev(wxCommandEvent& evt);
		void OnTraceLocalNext(wxCommandEvent& evt);
		void OnTraceGlobalNext(wxCommandEvent& evt);
		void OnTraceSyncPos(wxCommandEvent& evt);
		void OnCreateTimeMachine(wxCommandEvent& evt);
		void OnToggleHexView(wxCommandEvent& evt);

		void SyncImageView();
		void SyncRegisterView();
		void SyncTraceView();

		void RefreshState();
		void RefreshUI();

		trace::RegDisplayFormat GetValueDisplayFormat() const;

		bool OpenTimeMachine(const TraceFrameID id);

		virtual bool NavigateToAddress(const uint64 address, const bool addToHistory) override;
		virtual bool Navigate(const NavigationType type) override;
	};

} // tool