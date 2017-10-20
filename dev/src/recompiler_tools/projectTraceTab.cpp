#include "build.h"
#include "project.h"
#include "projectTraceTab.h"
#include "projectMemoryView.h"
#include "gotoAddressDialog.h"
#include "projectImage.h"
#include "traceInfoView.h"
#include "traceMemoryView.h"
#include "timeMachineView.h"

#include "../recompiler_core/decodingEnvironment.h"
#include "../recompiler_core/image.h"
#include "../recompiler_core/traceDataFile.h"
#include "registerView.h"
#include "callTreeView.h"
#include "callTreeList.h"

#pragma optimize ("",off)

namespace tools
{
	BEGIN_EVENT_TABLE(ProjectTraceTab, ProjectTab)
		EVT_TIMER(wxID_ANY, ProjectTraceTab::OnRefreshTimer)
		EVT_MENU(XRCID("navigateFind"), ProjectTraceTab::OnFindSymbol)
		EVT_MENU(XRCID("navigateGoTo"), ProjectTraceTab::OnGoToAddress)
		EVT_MENU(XRCID("traceToLocalStart"), ProjectTraceTab::OnTraceToLocalStart)
		EVT_MENU(XRCID("traceToLocalEnd"), ProjectTraceTab::OnTraceToLocalEnd)
		EVT_MENU(XRCID("traceToGlobalStart"), ProjectTraceTab::OnTraceToGlobalStart)
		EVT_MENU(XRCID("traceToGlobalEnd"), ProjectTraceTab::OnTraceToGlobalEnd)
		EVT_MENU(XRCID("traceRun"), ProjectTraceTab::OnTraceFreeRun)
		EVT_MENU(XRCID("traceAbsolutePrev"), ProjectTraceTab::OnTraceGlobalPrev)
		EVT_MENU(XRCID("traceLocalPrev"), ProjectTraceTab::OnTraceLocalPrev)
		EVT_MENU(XRCID("traceLocalNext"), ProjectTraceTab::OnTraceLocalNext)
		EVT_MENU(XRCID("traceAbsoluteNext"), ProjectTraceTab::OnTraceGlobalNext)
		EVT_MENU(XRCID("traceSyncPos"), ProjectTraceTab::OnTraceSyncPos)
		EVT_MENU(XRCID("timeMachineCreate"), ProjectTraceTab::OnCreateTimeMachine)
		EVT_MENU(XRCID("showValuesAsHex"), ProjectTraceTab::OnToggleHexView)
		EVT_MENU(XRCID("toggleGlobalStats"), ProjectTraceTab::OnToggleGlobalStats)
	END_EVENT_TABLE()

	ProjectTraceTab::ProjectTraceTab(ProjectWindow* parent, wxWindow* tabs, std::unique_ptr<trace::DataFile>& traceData)
		: ProjectTab(parent, tabs, ProjectTabType::TraceSession)
		, m_data(std::move(traceData))
		, m_disassemblyView(nullptr)
		, m_disassemblyPanel(nullptr)
		, m_currentEntry(0)
		, m_currentAddress(0)
		, m_refreshTimer(this)
		, m_traceInfoView(nullptr)
		, m_timeMachineTabs(nullptr)
		, m_callStackView(nullptr)
		, m_callTreeView(nullptr)
	{
		// load the ui
		wxXmlResource::Get()->LoadPanel(this, tabs, wxT("TraceTab"));

		// create the disassembly window
		{
			auto* panel = XRCCTRL(*this, "DisasmPanel", wxPanel);
			m_disassemblyPanel = new MemoryView(panel);
			m_disassemblyPanel->SetHitcountDisplay(true);
			panel->SetSizer(new wxBoxSizer(wxVERTICAL));
			panel->GetSizer()->Add(m_disassemblyPanel, 1, wxEXPAND, 0);
		}

		// the trace data
		{
			auto* panel = XRCCTRL(*this, "TraceDataPanel", wxPanel);
			m_traceInfoView = new TraceInfoView(panel, *m_data, GetProjectWindow()->GetProject().get());
			panel->SetSizer(new wxBoxSizer(wxVERTICAL));
			panel->GetSizer()->Add(m_traceInfoView, 1, wxEXPAND, 0);
		}

		// tabs
		{
			auto* panel = XRCCTRL(*this, "TimeMachineTabs", wxPanel);
			m_timeMachineTabs = new wxAuiNotebook(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_DEFAULT_STYLE | wxAUI_NB_CLOSE_BUTTON | wxAUI_NB_CLOSE_ON_ALL_TABS);
			panel->SetSizer(new wxBoxSizer(wxVERTICAL));
			panel->GetSizer()->Add(m_timeMachineTabs, 1, wxEXPAND, 0);
		}

		// registers
		{
			auto* tabs = XRCCTRL(*this, "RegListTabs", wxNotebook);
			m_registerViewsTabs = tabs;

			tabs->Bind(wxEVT_NOTEBOOK_PAGE_CHANGED, [this, tabs](wxNotebookEvent& evt)
			{
				SyncRegisterView();
			});

			{
				auto* view = new RegisterView(tabs);
				view->InitializeRegisters(*m_data->GetCPU(), platform::CPURegisterType::Control);
				tabs->AddPage(view, "Control");
				m_registerViews.push_back(view);
			}

			{
				auto* view = new RegisterView(tabs);
				view->InitializeRegisters(*m_data->GetCPU(), platform::CPURegisterType::Generic);
				tabs->AddPage(view, "Generic");
				m_registerViews.push_back(view);
			}

			{
				auto* view = new RegisterView(tabs);
				view->InitializeRegisters(*m_data->GetCPU(), platform::CPURegisterType::FloatingPoint);
				tabs->AddPage(view, "FPU");
				m_registerViews.push_back(view);
			}

			{
				auto* view = new RegisterView(tabs);
				view->InitializeRegisters(*m_data->GetCPU(), platform::CPURegisterType::Wide);
				tabs->AddPage(view, "Wide");
				m_registerViews.push_back(view);
			}
		}

		// callstack
		{
			auto* panel = XRCCTRL(*this, "HistoryPanel", wxPanel);
			m_callStackView = new CallTreeView(panel, this);
			m_callStackView->ExtractTraceData(*m_data);
			panel->SetSizer(new wxBoxSizer(wxVERTICAL));
			panel->GetSizer()->Add(m_callStackView, 1, wxEXPAND, 0);
		}

		// callstack tree
		{
			auto* panel = XRCCTRL(*this, "HistoryTree", wxPanel);
			m_callTreeView = new CallTreeList(panel, this);
			panel->SetSizer(new wxBoxSizer(wxVERTICAL));
			panel->GetSizer()->Add(m_callTreeView, 1, wxEXPAND, 0);
		}
	}

	ProjectTraceTab::~ProjectTraceTab()
	{
	}

	void ProjectTraceTab::OnRefreshTimer(wxTimerEvent & evt)
	{
		RefreshState();
	}

	void ProjectTraceTab::OnFindSymbol(wxCommandEvent& evt)
	{

	}

	void ProjectTraceTab::OnGoToAddress(wxCommandEvent& evt)
	{
		if (m_currentImage)
		{
			GotoAddressDialog dlg(this, m_currentImage, m_disassemblyPanel);
			const auto newAddres = dlg.GetNewAddress();
			NavigateToAddress(newAddres, true);
		}
	}

	void ProjectTraceTab::OnTraceToLocalStart(wxCommandEvent& evt)
	{
		if (!Navigate(NavigationType::LocalStart))
			wxMessageBox(wxT("Already at the beginning of scope"), wxT("Navigation"), wxICON_WARNING, this);
	}

	void ProjectTraceTab::OnTraceToLocalEnd(wxCommandEvent& evt)
	{
		if (!Navigate(NavigationType::LocalEnd))
			wxMessageBox(wxT("Already at the end of scope"), wxT("Navigation"), wxICON_WARNING, this);

	}

	void ProjectTraceTab::OnTraceToGlobalStart(wxCommandEvent& evt)
	{
		if (!Navigate(NavigationType::GlobalStart))
			wxMessageBox(wxT("Already at the beginning of file"), wxT("Navigation"), wxICON_WARNING, this);
	}

	void ProjectTraceTab::OnTraceToGlobalEnd(wxCommandEvent& evt)
	{
		if (!Navigate(NavigationType::GlobalEnd))
			wxMessageBox(wxT("Already at the end of file"), wxT("Navigation"), wxICON_WARNING, this);
	}

	void ProjectTraceTab::OnTraceFreeRun(wxCommandEvent& evt)
	{

	}

	void ProjectTraceTab::OnTraceGlobalPrev(wxCommandEvent& evt)
	{
		if (!Navigate(NavigationType::GlobalStepBack))
			wxMessageBox(wxT("No more instructions before this one"), wxT("Navigation"), wxICON_WARNING, this);
	}

	void ProjectTraceTab::OnTraceLocalPrev(wxCommandEvent& evt)
	{
		if (!Navigate(NavigationType::LocalStepBack))
			wxMessageBox(wxT("No more instructions before this one in current context"), wxT("Navigation"), wxICON_WARNING, this);
	}

	void ProjectTraceTab::OnTraceLocalNext(wxCommandEvent& evt)
	{
		if (!Navigate(NavigationType::LocalStepIn))
			wxMessageBox(wxT("No more instructions after this one in current context"), wxT("Navigation"), wxICON_WARNING, this);
	}

	void ProjectTraceTab::OnTraceGlobalNext(wxCommandEvent& evt)
	{
		if (!Navigate(NavigationType::GlobalStepIn))
			wxMessageBox(wxT("No more instructions after this one"), wxT("Navigation"), wxICON_WARNING, this);
	}

	void ProjectTraceTab::OnTraceSyncPos(wxCommandEvent& evt)
	{
		const auto& frame = m_data->GetFrame(m_currentEntry);
		if (frame.GetType() == trace::FrameType::CpuInstruction)
		{
			const auto address = frame.GetAddress();
			NavigateToAddress(address, false);
			SyncTraceView();
		}
	}

	bool ProjectTraceTab::OpenTimeMachine(const TraceFrameID id)
	{
		const auto& frame = m_data->GetFrame(m_currentEntry);
		if (frame.GetType() != trace::FrameType::CpuInstruction)
			return false;
			
		// look for existing entry
		const auto numPages = m_timeMachineTabs->GetPageCount();
		for (uint32_t i = 0; i < numPages; ++i)
		{
			auto* page = static_cast<TimeMachineView*>(m_timeMachineTabs->GetPage(i));
			if (page->GetRootTraceIndex() == id)
			{
				m_timeMachineTabs->SetSelection(i);
				page->SetFocus();
				return true;
			}
		}

		// retrieve decoding context for current image
		auto context = GetProjectWindow()->GetProject()->GetDecodingContext(frame.GetAddress());
		if (!context)
			return false;

		// crate new time machine trace
		auto* trace = timemachine::Trace::CreateTimeMachine(context, m_data.get(), id);
		if (!trace)
			return false;

		auto* view = new TimeMachineView(m_timeMachineTabs, trace, this);
		m_timeMachineTabs->AddPage(view, wxString::Format("%llu (0x%08llX)", id, frame.GetAddress()), true);
		return true;
	}

	bool ProjectTraceTab::GetGlobalStatsFlags() const
	{
		auto* toolbar = XRCCTRL(*this, "ToolBar", wxToolBar);

		const auto useGlobalStats = toolbar->GetToolState(XRCID("toggleGlobalStats"));
		return useGlobalStats;
	}

	trace::RegDisplayFormat ProjectTraceTab::GetValueDisplayFormat() const
	{
		auto* toolbar = XRCCTRL(*this, "RegistersToolbar", wxToolBar);

		const auto hexView = toolbar->GetToolState(XRCID("showValuesAsHex"));
		if (hexView)
			return trace::RegDisplayFormat::Hex;

		return trace::RegDisplayFormat::Auto;
	}

	void ProjectTraceTab::OnToggleHexView(wxCommandEvent& evt)
	{		
		SyncRegisterView();
		SyncTraceView();
	}

	void ProjectTraceTab::OnToggleGlobalStats(wxCommandEvent& evt)
	{
		SyncImageView();
	}

	void ProjectTraceTab::OnCreateTimeMachine(wxCommandEvent& evt)
	{
		OpenTimeMachine(m_currentEntry);
	}

	void ProjectTraceTab::RefreshState()
	{

	}

	void ProjectTraceTab::RefreshUI()
	{

	}

	bool ProjectTraceTab::Navigate(const NavigationType type)
	{
		switch (type)
		{
			case NavigationType::Back:
			{
				const auto newAddress = m_addressHistory.NavigateBack();
				return NavigateToAddress(newAddress, false);
			}

			case NavigationType::LocalStepIn:
			{
				const auto& entry = m_data->GetFrame(m_currentEntry);
				if (entry.GetType() != trace::FrameType::Invalid)
					return NavigateToFrame(entry.GetNavigationInfo().m_nextInContext);
			}

			case NavigationType::LocalStepBack:
			{
				const auto& entry = m_data->GetFrame(m_currentEntry);
				if (entry.GetType() != trace::FrameType::Invalid)
					return NavigateToFrame(entry.GetNavigationInfo().m_prevInContext);
			}

			case NavigationType::GlobalStepIn:
			{
				if (m_currentEntry < (m_data->GetNumDataFrames() - 1))
					return NavigateToFrame(m_currentEntry + 1);
				else
					return false;
			}

			case NavigationType::GlobalStepBack:
			{
				if (m_currentEntry > 0)
					return NavigateToFrame(m_currentEntry - 1);
				else
					return false;
			}

			case NavigationType::LocalStart:
			{
				const auto& entry = m_data->GetFrame(m_currentEntry);
				if (entry.GetType() != trace::FrameType::Invalid)
				{
					const auto& contextInfo = m_data->GetContextList()[entry.GetLocationInfo().m_contextId];
					return NavigateToFrame(contextInfo.m_first.m_seq);
				}
				else
				{
					return false;
				}
			}

			case NavigationType::LocalEnd:
			{
				const auto& entry = m_data->GetFrame(m_currentEntry);
				if (entry.GetType() != trace::FrameType::Invalid)
				{
					const auto& contextInfo = m_data->GetContextList()[entry.GetLocationInfo().m_contextId];
					return NavigateToFrame(contextInfo.m_last.m_seq);
				}
				else
				{
					return false;
				}
			}

			case NavigationType::GlobalStart:
			{
				return NavigateToFrame(0);
			}

			case NavigationType::GlobalEnd:
			{
				const auto lastFrame = m_data->GetNumDataFrames() - 1;
				const auto& entry = m_data->GetFrame(lastFrame);
				if (entry.GetType() != trace::FrameType::Invalid && entry.GetAddress())
					return NavigateToFrame(lastFrame);
				else
					return false;
			}
		}

		return false;
	}

	bool ProjectTraceTab::NavigateToAddress(const uint64 address, const bool addToHistory)
	{
		if (address == INVALID_ADDRESS)
			return false;

		if (m_currentAddress == address)
			return true;

		// get project image for given address
		auto projectImage = GetProjectWindow()->GetProject()->FindImageForAddress(address);
		if (!projectImage)
			return false;

		// recreate view
		if (m_currentImage != projectImage)
		{
			m_currentImage = projectImage;

			// change the disassembly to new image
			delete m_disassemblyView;
			if (projectImage)
				m_disassemblyView = new TraceMemoryView(projectImage, this, *m_data);
			else
				m_disassemblyView = nullptr;

			m_disassemblyPanel->SetDataView(m_disassemblyView);
			SyncImageView();
		}

		// sync address
		m_currentAddress = address;
		
		// sync address
		if (m_currentImage)
		{
			// check if the address if within the limits of the image
			const auto baseAddress = m_currentImage->GetEnvironment().GetImage()->GetBaseAddress();
			const auto endAddress = baseAddress + m_currentImage->GetEnvironment().GetImage()->GetMemorySize();
			if (address < baseAddress || address >= endAddress)
			{
				GetProjectWindow()->GetApp()->GetLogWindow().Error("Image: Trying to navigate to address 0x%08llX that is outside the image boundary", address);
				return false;
			}

			// move to offset
			const auto newOffset = address - baseAddress;
			m_disassemblyPanel->SetActiveOffset(newOffset, addToHistory);
		}

		return true;
	}
		
	bool ProjectTraceTab::NavigateToFrame(const TraceFrameID seq)
	{
		if (m_currentEntry != seq)
		{
			const auto& frame = m_data->GetFrame(seq);
			if (frame.GetType() == trace::FrameType::Invalid)
				return false;

			m_currentEntry = seq;	

			SyncImageView();
			SyncRegisterView();
			SyncTraceView();

			if (frame.GetType() == trace::FrameType::CpuInstruction)
			{
				const auto address = frame.GetAddress();
				m_addressHistory.Reset(address);

				if (!NavigateToAddress(address, false))
					return false;
			}
		}

		return true;
	}

	void ProjectTraceTab::SyncImageView()
	{
		if (m_disassemblyView != nullptr)
		{
			const auto useGlobalStats = GetGlobalStatsFlags();
			m_disassemblyView->SetTraceFrame(m_currentEntry, !useGlobalStats);
			m_disassemblyPanel->Refresh();
		}
	}

	void ProjectTraceTab::SyncRegisterView()
	{
		const auto displayFormat = GetValueDisplayFormat();
		const auto curFrame = m_data->GetFrame(m_currentEntry);
		const auto nextFrame = m_data->GetFrame(m_currentEntry+1);

		auto* curPage = (RegisterView*)m_registerViewsTabs->GetCurrentPage();
		if (curPage)
			curPage->UpdateRegisters(curFrame, nextFrame, displayFormat);
	}

	void ProjectTraceTab::SyncTraceView()
	{
		const auto displayFormat = GetValueDisplayFormat();
		m_traceInfoView->SetFrame(m_currentEntry, displayFormat);
		m_callStackView->SetPosition(m_currentEntry);
		m_callTreeView->ShowCallstacks(*m_data, m_currentEntry);
	}

} // tools

