#pragma once

#include "projectWindow.h"
#include "projectMemoryView.h"
#include "timeMachineView.h"
#include "../recompiler_core/traceUtils.h"

namespace tools
{
	class MemoryView;
	class TraceMemoryView;
	class TraceInfoView;
	class RegisterView;
	class CallTreeView;
	class CallTreeList;

	/// tab for the project trace 
	class ProjectTraceTab : public ProjectTab, public INavigationHelper
	{
		DECLARE_EVENT_TABLE();

	public:
		ProjectTraceTab(ProjectWindow* parent, wxWindow* tabs, std::unique_ptr<trace::DataFile>& traceData);
		virtual ~ProjectTraceTab();

		// get the trace data
		inline trace::DataFile& GetData() { return *m_data; }

		//--

		// navigate to trace entry
		virtual bool NavigateToFrame(const TraceFrameID seq) override;

		// general navigation
		virtual bool Navigate(const NavigationType type) override;

	private:
		// native trace file
		std::unique_ptr<trace::DataFile> m_data;

		// current trace entry
		TraceFrameID m_currentEntry;
		uint64 m_currentAddress;
		std::shared_ptr<ProjectImage> m_currentImage;

		// current image disassembly
		TraceMemoryView* m_disassemblyView;
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

		// call stack view
		CallTreeView* m_callStackView;
		CallTreeList* m_callTreeView;

		// horizontal trace
		std::vector<TraceFrameID> m_horizontalTraceFrames;

		//--

		void OnRefreshTimer(wxTimerEvent & evt);
		void OnGoToAddress(wxCommandEvent& evt);
		void OnTraceToLocalStart(wxCommandEvent& evt);
		void OnTraceToLocalEnd(wxCommandEvent& evt);
		void OnTraceToGlobalStart(wxCommandEvent& evt);
		void OnTraceToGlobalEnd(wxCommandEvent& evt);
		void OnTraceFreeRunForward(wxCommandEvent& evt);
		void OnTraceFreeRunBackward(wxCommandEvent& evt);
		void OnTraceGlobalPrev(wxCommandEvent& evt);
		void OnTraceLocalPrev(wxCommandEvent& evt);
		void OnTraceLocalNext(wxCommandEvent& evt);
		void OnTraceGlobalNext(wxCommandEvent& evt);
		void OnTraceSyncPos(wxCommandEvent& evt);
		void OnCreateTimeMachine(wxCommandEvent& evt);
		void OnToggleHexView(wxCommandEvent& evt);
		void OnToggleGlobalStats(wxCommandEvent& evt);
		void OnShowHorizontalTraceFunction(wxCommandEvent& evt);
		void OnShowHorizontalTraceContext(wxCommandEvent& evt);
		void OnShowHorizontalTraceGlobal(wxCommandEvent& evt);

		void SyncImageView();
		void SyncRegisterView();
		void SyncTraceView();

		void RefreshState();
		void RefreshUI();

		trace::RegDisplayFormat GetValueDisplayFormat() const;
		bool GetGlobalStatsFlags() const;

		enum class SliceMode
		{
			Function,
			Context,
			Global,
		};

		bool OpenTimeMachine(const TraceFrameID id);
		bool OpenHorizontalTrace(const TraceFrameID id, const SliceMode mode);

		virtual std::shared_ptr<ProjectImage> GetCurrentImage() override;
		virtual MemoryView* GetCurrentMemoryView() override;
		virtual bool NavigateToAddress(const uint64 address, const bool addToHistory) override;
	};

} // tool