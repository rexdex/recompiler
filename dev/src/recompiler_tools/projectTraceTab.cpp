#include "build.h"
#include "project.h"
#include "projectTraceTab.h"
#include "projectMemoryView.h"
#include "gotoAddressDialog.h"
#include "projectImage.h"
#include "traceInfoView.h"
#include "traceMemoryView.h"
#include "timeMachineView.h"
#include "registerView.h"
#include "callTreeView.h"
#include "callTreeList.h"
#include "horizontalView.h"
#include "progressDialog.h"

#include "../recompiler_core/decodingEnvironment.h"
#include "../recompiler_core/image.h"
#include "../recompiler_core/traceDataFile.h"
#include "../recompiler_core/decodingAddressMap.h"
#include "../recompiler_core/decodingContext.h"
#include "memoryTraceView.h"
#include "memoryHistoryView.h"

#pragma optimize ("",off)

namespace tools
{
	BEGIN_EVENT_TABLE(ProjectTraceTab, ProjectTab)
		EVT_TIMER(wxID_ANY, ProjectTraceTab::OnRefreshTimer)
		//EVT_MENU(XRCID("navigateFind"), ProjectTraceTab::OnFindSymbol)
		EVT_MENU(XRCID("navigateGoTo"), ProjectTraceTab::OnGoToAddress)
		EVT_MENU(XRCID("traceToLocalStart"), ProjectTraceTab::OnTraceToLocalStart)
		EVT_MENU(XRCID("traceToLocalEnd"), ProjectTraceTab::OnTraceToLocalEnd)
		EVT_MENU(XRCID("traceToGlobalStart"), ProjectTraceTab::OnTraceToGlobalStart)
		EVT_MENU(XRCID("traceToGlobalEnd"), ProjectTraceTab::OnTraceToGlobalEnd)
		EVT_MENU(XRCID("traceRunForward"), ProjectTraceTab::OnTraceFreeRunForward)
		EVT_MENU(XRCID("traceRunBackward"), ProjectTraceTab::OnTraceFreeRunBackward)
		EVT_MENU(XRCID("traceAbsolutePrev"), ProjectTraceTab::OnTraceGlobalPrev)
		EVT_MENU(XRCID("traceLocalPrev"), ProjectTraceTab::OnTraceLocalPrev)
		EVT_MENU(XRCID("traceLocalNext"), ProjectTraceTab::OnTraceLocalNext)
		EVT_MENU(XRCID("traceAbsoluteNext"), ProjectTraceTab::OnTraceGlobalNext)
		EVT_MENU(XRCID("traceSyncPos"), ProjectTraceTab::OnTraceSyncPos)
		EVT_MENU(XRCID("timeMachineCreate"), ProjectTraceTab::OnCreateTimeMachine)
		EVT_MENU(XRCID("showValuesAsHex"), ProjectTraceTab::OnToggleHexView)
		EVT_MENU(XRCID("toggleGlobalStats"), ProjectTraceTab::OnToggleGlobalStats)
		EVT_MENU(XRCID("horizontalViewFunction"), ProjectTraceTab::OnShowHorizontalTraceFunction)
		EVT_MENU(XRCID("horizontalViewContext"), ProjectTraceTab::OnShowHorizontalTraceContext)
		EVT_MENU(XRCID("horizontalViewGlobal"), ProjectTraceTab::OnShowHorizontalTraceGlobal)
		EVT_TEXT_ENTER(XRCID("MemoryAddress"), ProjectTraceTab::OnMemoryDisplayAddressChanged)
		EVT_CHOICE(XRCID("MemoryDisplayMode"), ProjectTraceTab::OnMemoryDisplayParamsChanged)
		EVT_CHOICE(XRCID("MemoryRowSize"), ProjectTraceTab::OnMemoryDisplayParamsChanged)
		EVT_CHOICE(XRCID("MemoryEndianess"), ProjectTraceTab::OnMemoryDisplayParamsChanged)
		EVT_MENU(XRCID("traceMemoryWrites"), ProjectTraceTab::OnTraceMemoryWrites)
		EVT_MENU(XRCID("traceMemoryFull"), ProjectTraceTab::OnTraceMemoryFull)
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

		// remove the key shortcuts that cause problems
		//GetAcceleratorTable()->Remove(wxAcceleratorEntry(wxACCEL_NORMAL, WXK_RETURN));
		//GetAcceleratorTable()->Remove(wxAcceleratorEntry(wxACCEL_NORMAL, WXK_BA));

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
			m_traceInfoView = new TraceInfoView(panel, *m_data, GetProject().get(), this);
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

		// memory view
		{
			auto* panel = XRCCTRL(*this, "MemoryTraceView", wxPanel);
			m_memoryTraceView = new MemoryTraceView(panel, m_data.get());
			m_memoryTraceView->SetAddress(0);
			panel->SetSizer(new wxBoxSizer(wxVERTICAL));
			panel->GetSizer()->Add(m_memoryTraceView, 1, wxEXPAND, 0);
		}

		// memory display mode
		{
			auto* mode = XRCCTRL(*this, "MemoryDisplayMode", wxChoice);
			mode->AppendString("Hex8");
			mode->AppendString("Hex16");
			mode->AppendString("Hex32");
			mode->AppendString("Hex64");
			mode->AppendString("Uint8");
			mode->AppendString("Uint16");
			mode->AppendString("Uint32");
			mode->AppendString("Uint64");
			mode->AppendString("Int8");
			mode->AppendString("Int16");
			mode->AppendString("Int32");
			mode->AppendString("Int64");
			mode->AppendString("Float");
			mode->AppendString("Double");
			mode->AppendString("AnsiChars");
			mode->AppendString("UniChars");
			mode->SetSelection(0);
		}

		// memory row sizes
		{
			auto* mode = XRCCTRL(*this, "MemoryRowSize", wxChoice);
			mode->AppendString("8");
			mode->AppendString("16");
			mode->AppendString("32");
			mode->AppendString("64");
			mode->SetSelection(2);
		}

		// memory endianess mode list
		{
			auto* mode = XRCCTRL(*this, "MemoryEndianess", wxChoice);
			mode->AppendString("BigEndian");
			mode->AppendString("LittleEndian");
			mode->SetSelection(0);
		}

		// sync the memory view
		RefreshMemoryDisplayMode();		
	}

	ProjectTraceTab::~ProjectTraceTab()
	{
	}

	void ProjectTraceTab::OnRefreshTimer(wxTimerEvent & evt)
	{
		RefreshState();
	}

	void ProjectTraceTab::OnGoToAddress(wxCommandEvent& evt)
	{
		if (m_currentImage)
		{
			GotoAddressDialog dlg(this, m_currentImage, m_disassemblyPanel);
			const auto newAddres = dlg.GetNewAddress();
			NavigateToCodeAddress(newAddres, true);
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

	void ProjectTraceTab::OnTraceFreeRunForward(wxCommandEvent& evt)
	{
		if (!Navigate(NavigationType::RunForward))
			wxMessageBox(wxT("Unable to find next breakpoint"), wxT("Navigation"), wxICON_WARNING, this);
	}

	void ProjectTraceTab::OnTraceFreeRunBackward(wxCommandEvent& evt)
	{
		if (!Navigate(NavigationType::RunBackward))
			wxMessageBox(wxT("Unable to find previous breakpoint"), wxT("Navigation"), wxICON_WARNING, this);
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
		if (!Navigate(NavigationType::SyncPos))
			wxBell();
	}

	bool ProjectTraceTab::OpenHorizontalTrace(const TraceFrameID seq, const SliceMode mode)
	{
		TraceFrameID firstEntry = m_data->GetFirstFrame();
		TraceFrameID lastEntry = m_data->GetLastFrame();
		wxString title = "Global";

				// constrain to range of trace
		if (mode == SliceMode::Function)
		{
			if (!m_data->GetInnerMostCallFunction(seq, firstEntry, lastEntry))
				return false;
			title = "Function";
		}
		else if (mode == SliceMode::Context)
		{
			const auto frame = m_data->GetFrame(seq);
			const auto& context = m_data->GetContextList()[frame.GetLocationInfo().m_contextId];
			firstEntry = context.m_first.m_seq;
			lastEntry = context.m_last.m_seq;
			title = "Scope";
		}

		// get the trace frames the instruction is visited
		const auto codeAddress = m_data->GetFrame(seq).GetAddress();
		std::vector<TraceFrameID> sequenceList;
		if (mode == SliceMode::Global)
		{
			// process all contexts
			for (uint32_t i = 0; i < m_data->GetContextList().size(); ++i)
			{
				const auto* codePage = m_data->GetCodeTracePage(i, codeAddress);
				if (nullptr != codePage)
				{
					m_data->GetCodeTracePageHorizonstalSlice(*codePage, seq, firstEntry, lastEntry, codeAddress, sequenceList);
				}
			}
		}
		else
		{
			// get the code page matching the initial sequence point
			const auto* codePage = m_data->GetCodeTracePage(seq);
			if (!codePage)
				return false;

			// get the list of sequence points
			const auto codeAddress = m_data->GetFrame(seq).GetAddress();
			if (!m_data->GetCodeTracePageHorizonstalSlice(*codePage, seq, firstEntry, lastEntry, codeAddress, sequenceList))
				return false;
		}

		// get display mode
		const auto displayMode = GetValueDisplayFormat();

		// get more human readable info about the file
		auto codeAddressName = wxString::Format("0x%08llX", codeAddress);
		if (const auto* context = GetProject()->GetDecodingContext(codeAddress))
		{
			std::string functionName;
			uint64 functionBase;
			if (context->GetFunctionName(codeAddress, functionName, functionBase) && !functionName.empty())
			{
				const auto delta = codeAddress - functionBase;
				codeAddressName += wxString::Format(" %hs + %X", functionName.c_str());
			}
		}

		// create the horizontal list
		auto* view = new HorizontalRegisterView(m_timeMachineTabs, this);
		const auto name = wxString::Format("Slice %hs at %hs (%llu)", title.c_str().AsChar(), codeAddressName.c_str().AsChar(), seq);
		if (!view->FillTable(GetProject().get(), *m_data, seq, sequenceList))
		{
			delete view;
			return false;
		}

		view->UpdateValueTexts(displayMode);

		// add page
		m_timeMachineTabs->AddPage(view, name, true);
		return true;
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
			const auto pageTitle = m_timeMachineTabs->GetPageText(i);
			if (pageTitle.StartsWith("TimeMachine "))
			{
				auto* page = static_cast<TimeMachineView*>(m_timeMachineTabs->GetPage(i));
				if (page->GetRootTraceIndex() == id)
				{
					m_timeMachineTabs->SetSelection(i);
					page->SetFocus();
					return true;
				}
			}
		}

		// retrieve decoding context for current image
		auto context = GetProject()->GetDecodingContext(frame.GetAddress());
		if (!context)
			return false;

		// crate new time machine trace
		auto* trace = timemachine::Trace::CreateTimeMachine(context, m_data.get(), id);
		if (!trace)
			return false;

		auto* view = new TimeMachineView(m_timeMachineTabs, trace, this);
		m_timeMachineTabs->AddPage(view, wxString::Format("TimeMachine %llu (0x%08llX)", id, frame.GetAddress()), true);
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
		auto* toolbar = XRCCTRL(*this, "ToolBar", wxToolBar);

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

	void ProjectTraceTab::OnShowHorizontalTraceFunction(wxCommandEvent& evt)
	{
		OpenHorizontalTrace(m_currentEntry, SliceMode::Function);
	}

	void ProjectTraceTab::OnShowHorizontalTraceContext(wxCommandEvent& evt)
	{
		OpenHorizontalTrace(m_currentEntry, SliceMode::Context);
	}

	void ProjectTraceTab::OnShowHorizontalTraceGlobal(wxCommandEvent& evt)
	{
		OpenHorizontalTrace(m_currentEntry, SliceMode::Global);
	}

	void ProjectTraceTab::OnMemoryDisplayParamsChanged(wxCommandEvent& evt)
	{
		RefreshMemoryDisplayMode();
	}

	// TODO: move	

	void ProjectTraceTab::OnMemoryDisplayAddressChanged(wxCommandEvent& evt)
	{
		const auto* addressBox = XRCCTRL(*this, "MemoryAddress", wxTextCtrl);

		wxString text = addressBox->GetValue();
		uint64 newAddress = 0;
		if (ParseAddress(text, newAddress))
		{
			m_memoryTraceView->SetAddress(newAddress);
		}
		else
		{
			wxBell();
		}
	}

	void ProjectTraceTab::OnTraceMemoryWrites(wxCommandEvent& evt)
	{
		const auto* modeBox = XRCCTRL(*this, "MemoryDisplayMode", wxChoice);
		const auto displayMode = (MemoryDisplayMode)(modeBox->GetSelection());

		const auto* endianessBox = XRCCTRL(*this, "MemoryEndianess", wxChoice);
		const auto displayEndianess = (MemoryEndianess)(endianessBox->GetSelection());

		uint64 memoryStart = 0, memoryEnd = 0;
		if (!m_memoryTraceView->GetSelection(memoryStart, memoryEnd))
		{
			wxMessageBox("Please select some memory range from the memory window", "Memory history", wxICON_ERROR, this);
			return;
		}

		std::vector<trace::MemoryAccessInfo> history;
		if (!m_data->GetMemoryWriteHistory(memoryStart, memoryEnd - memoryStart, history))
		{
			wxMessageBox("Unable to get history for selected memory range", "Memory history", wxICON_ERROR, this);
			return;
		}


		auto* view = new MemoryHistoryView(m_timeMachineTabs, this);
		if (!view->FillTable(GetProject().get(), *m_data, history, displayMode, displayEndianess))
		{ 
			delete view;
			wxBell();
			return;
		}

		m_timeMachineTabs->AddPage(view, wxString::Format("Memory (0x%08llX + %u)", memoryStart, memoryEnd - memoryStart), true);
	}

	void ProjectTraceTab::OnTraceMemoryFull(wxCommandEvent& evt)
	{
		const auto* modeBox = XRCCTRL(*this, "MemoryDisplayMode", wxChoice);
		const auto displayMode = (MemoryDisplayMode)(modeBox->GetSelection());

		const auto* endianessBox = XRCCTRL(*this, "MemoryEndianess", wxChoice);
		const auto displayEndianess = (MemoryEndianess)(endianessBox->GetSelection());
	}

	void ProjectTraceTab::RefreshMemoryDisplayMode()
	{
		const auto* modeBox = XRCCTRL(*this, "MemoryDisplayMode", wxChoice);
		if (modeBox->GetSelection() == -1)
			return;

		const auto* sizeBox = XRCCTRL(*this, "MemoryRowSize", wxChoice);
		if (sizeBox->GetSelection() == -1)
			return;

		const auto* endianessBox = XRCCTRL(*this, "MemoryEndianess", wxChoice);
		if (endianessBox->GetSelection() == -1)
			return;

		const auto displayMode = (MemoryDisplayMode)(modeBox->GetSelection());
		const auto displayRowSize = 8 << sizeBox->GetSelection();
		const auto displayEndianess = (MemoryEndianess)(endianessBox->GetSelection());

		m_memoryTraceView->SetDisplayMode(displayMode, displayRowSize, displayEndianess);
	}

	void ProjectTraceTab::SyncMemoryView()
	{
		m_memoryTraceView->SetTraceFrame(m_currentEntry);	
	}

	void ProjectTraceTab::RefreshState()
	{

	}

	void ProjectTraceTab::RefreshUI()
	{

	}

	bool ProjectTraceTab::Navigate(const NavigationType type)
	{
		const auto image = GetCurrentImage();

		switch (type)
		{
			case NavigationType::HistoryBack:
			{
				const auto newAddress = m_addressHistory.NavigateBack();
				return NavigateToCodeAddress(newAddress, false);
			}

			case NavigationType::HistoryForward:
			{
				const auto newAddress = m_addressHistory.NavigateForward();
				return NavigateToCodeAddress(newAddress, false);
			}

			case NavigationType::ToggleBreakpoint:
			{
				const auto currentAddress = m_disassemblyPanel->GetCurrentRVA();
				GetProject()->GetBreakpoints().ToggleBreakpoint(currentAddress);
				return true;
			}

			case NavigationType::Follow:
			{
				// get the source section of the image
				uint64 lowAddres = 0, highAddress = 0;
				if (!m_disassemblyPanel->GetSelectionAddresses(lowAddres, highAddress))
					return false;

				if (!image)
					return false;

				const auto* section = image->GetEnvironment().GetImage()->FindSectionForAddress(lowAddres);
				if (section == nullptr)
					return false;

				const uint32 branchTargetAddress = image->GetEnvironment().GetDecodingContext()->GetAddressMap().GetReferencedAddress(lowAddres);
				if (branchTargetAddress)
					return NavigateToCodeAddress(branchTargetAddress, true);

				return false;
			}

			case NavigationType::ReverseFollow:
			{
				// get the source section of the image
				uint64 lowAddres = 0, highAddress = 0;
				if (!m_disassemblyPanel->GetSelectionAddresses(lowAddres, highAddress))
					return false;

				if (!image)
					return false;

				const auto* section = image->GetEnvironment().GetImage()->FindSectionForAddress(lowAddres);
				if (section == nullptr)
					return false;

				// back navigation?
				std::vector<uint64> sourceAddresses;
				image->GetEnvironment().GetDecodingContext()->GetAddressMap().GetAddressReferencers(lowAddres, sourceAddresses);
				if (sourceAddresses.size() > 1)
				{
					wxMenu menu;

					for (uint32 i = 0; i < sourceAddresses.size(); ++i)
					{
						const auto menuId = 16000 + i;
						menu.Append(16000 + i, wxString::Format("0x%08llx", sourceAddresses[i]));
					}

					menu.Bind(wxEVT_MENU, [this, sourceAddresses](const wxCommandEvent& evt)
					{
						const auto it = evt.GetId() - 16000;
						return NavigateToCodeAddress(sourceAddresses[it], true);
					});

					PopupMenu(&menu);
				}

				return true;
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
				return NavigateToFrame(m_data->GetFirstFrame());
			}

			case NavigationType::GlobalEnd:
			{
				return NavigateToFrame(m_data->GetLastFrame());
			}

			case NavigationType::SyncPos:
			{
				const auto& frame = m_data->GetFrame(m_currentEntry);
				m_currentEntry = 0;
				m_currentAddress = 0;
				NavigateToFrame(frame.GetIndex());
				return true;
			}

			case NavigationType::HorizontalNext:
			case NavigationType::HorizontalPrev:
			{
				// can we use current trace, if not recompute
				auto it = std::find(m_horizontalTraceFrames.begin(), m_horizontalTraceFrames.end(), m_currentEntry);
				if (it == m_horizontalTraceFrames.end())
				{
					const auto frame = m_data->GetFrame(m_currentEntry);
					const auto& context = m_data->GetContextList()[frame.GetLocationInfo().m_contextId];

					// get the code page matching the initial sequence point
					const auto* codePage = m_data->GetCodeTracePage(m_currentEntry);
					if (!codePage)
						return false;

					// get the list of sequence points
					const auto codeAddress = m_data->GetFrame(m_currentEntry).GetAddress();
					if (!m_data->GetCodeTracePageHorizonstalSlice(*codePage, m_currentEntry, context.m_first.m_seq, context.m_last.m_seq, codeAddress, m_horizontalTraceFrames))
						return false;

					// find again
					it = std::find(m_horizontalTraceFrames.begin(), m_horizontalTraceFrames.end(), m_currentEntry);
					if (it == m_horizontalTraceFrames.end())
						return false;
				}

				// get next/previous
				if (type == NavigationType::HorizontalNext)
				{
					++it;
					if (it == m_horizontalTraceFrames.end())
						return false;

					const auto newSeq = *it;
					return NavigateToFrame(newSeq);
				}
				else
				{
					if (it == m_horizontalTraceFrames.begin())
						return false;

					--it;
					const auto newSeq = *it;
					return NavigateToFrame(newSeq);
				}	
			}

			case NavigationType::RunForward:
			{
				ProgressDialog dlg(this, GetProjectWindow()->GetApp()->GetLogWindow(), true);

				const auto* breakpointList = &GetProject()->GetBreakpoints();

				TraceFrameID curSeq = m_currentEntry;
				TraceFrameID newSeq = INVALID_TRACE_FRAME_ID;
				dlg.RunLongTask([this, curSeq, &newSeq, breakpointList](ILogOutput& log)
				{
					newSeq = m_data->VisitForwards(log, curSeq, [curSeq, breakpointList](const trace::LocationInfo& info)
					{
						if (info.m_seq == curSeq)
							return false;
						return breakpointList->HasBreakpoint(info.m_ip);
					});
					return 0;
				});

				return NavigateToFrame(newSeq);
			}

			case NavigationType::RunBackward:
			{
				ProgressDialog dlg(this, GetProjectWindow()->GetApp()->GetLogWindow(), true);

				const auto* breakpointList = &GetProject()->GetBreakpoints();

				TraceFrameID curSeq = m_currentEntry;
				TraceFrameID newSeq = INVALID_TRACE_FRAME_ID;
				dlg.RunLongTask([this, curSeq, &newSeq, breakpointList](ILogOutput& log)
				{
					newSeq = m_data->VisitBackwards(log, curSeq, [curSeq, breakpointList](const trace::LocationInfo& info)
					{
						if (info.m_seq == curSeq)
							return false;
						return breakpointList->HasBreakpoint(info.m_ip);
					});
					return 0;
				});

				return NavigateToFrame(newSeq);
			}
		}

		return false;
	}

	std::shared_ptr<ProjectImage> ProjectTraceTab::GetCurrentImage()
	{
		return m_currentImage;
	}	

	MemoryView* ProjectTraceTab::GetCurrentMemoryView()
	{
		return m_disassemblyPanel;
	}

	bool ProjectTraceTab::NavigateToCodeAddress(const uint64 address, const bool addToHistory)
	{
		if (address == INVALID_ADDRESS)
			return false;

		if (m_currentAddress == address)
			return true;

		// get project image for given address
		auto projectImage = GetProject()->FindImageForAddress(address);
		if (!projectImage)
			return false;

		// recreate view
		if (m_currentImage != projectImage)
		{
			m_currentImage = projectImage;

			// change the disassembly to new image
			delete m_disassemblyView;
			if (projectImage)
				m_disassemblyView = new TraceMemoryView(GetProject(), projectImage, this, *m_data);
			else
				m_disassemblyView = nullptr;

			m_disassemblyPanel->SetDataView(m_disassemblyView);
			SyncImageView();
		}

		// sync address
		m_currentAddress = address;
		m_addressHistory.UpdateAddress(address, addToHistory);
		
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
		
	bool ProjectTraceTab::NavigateToMemoryAddress(const uint64 address)
	{
		auto* addressBox = XRCCTRL(*this, "MemoryAddress", wxTextCtrl);
		addressBox->SetValue(wxString::Format("%08llX", address));

		auto* traceTabs = XRCCTRL(*this, "TraceTabs", wxNotebook);
		m_memoryTraceView->SetAddress(address);
		traceTabs->SetSelection(2);
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
			SyncMemoryView();

			if (frame.GetType() == trace::FrameType::CpuInstruction || frame.GetType() == trace::FrameType::ExternalMemoryWrite)
			{
				const auto address = frame.GetAddress();
				m_addressHistory.Reset(address);

				if (!NavigateToCodeAddress(address, false))
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

		const auto numPages = m_timeMachineTabs->GetPageCount();
		for (uint32_t i = 0; i < numPages; ++i)
		{
			const auto pageTitle = m_timeMachineTabs->GetPageText(i);
			if (pageTitle.StartsWith("Slice "))
			{
				auto* page = static_cast<HorizontalRegisterView*>(m_timeMachineTabs->GetPage(i));
				page->UpdateValueTexts(displayFormat);
			}
		}
	}

	void ProjectTraceTab::SyncTraceView()
	{
		const auto displayFormat = GetValueDisplayFormat();
		m_traceInfoView->SetFrame(m_currentEntry, displayFormat);
		m_callStackView->SetPosition(m_currentEntry);
		m_callTreeView->ShowCallstacks(*m_data, m_currentEntry);
	}

} // tools

