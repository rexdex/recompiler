#include "build.h"
#include "project.h"
#include "projectImage.h"
#include "projectWindow.h"
#include "projectMainTab.h"
#include "projectImageTab.h"
#include "app.h"
#include "progressDialog.h"
#include "projectTraceTab.h"
#include "gotoAddressDialog.h"
#include "../recompiler_core/traceDataFile.h"
#include "findSymbolDialog.h"

namespace tools
{
	//--

	BEGIN_EVENT_TABLE(ProjectTab, wxPanel)
		END_EVENT_TABLE()

	ProjectTab::ProjectTab(ProjectWindow* projectWindow, wxWindow* tabs, const ProjectTabType tabType)
		: m_projectWindow(projectWindow)
		, m_tabType(tabType)
	{}

	//--

	ProjectWindow* wxTheFrame = nullptr;

	BEGIN_EVENT_TABLE(ProjectWindow, wxFrame)
		EVT_ACTIVATE(ProjectWindow::OnActivate)
		EVT_CLOSE(ProjectWindow::OnClose)
		EVT_MENU(XRCID("fileSave"), ProjectWindow::OnProjectSave)
		EVT_MENU(XRCID("fileClose"), ProjectWindow::OnProjectClose)
		EVT_MENU(XRCID("fileExit"), ProjectWindow::OnExitApp)
		EVT_MENU(XRCID("openLog"), ProjectWindow::OnOpenLog)
		EVT_MENU(XRCID("imageFindSynbol"), ProjectWindow::OnFindSymbol)
		EVT_MENU(XRCID("imageGoTo"), ProjectWindow::OnGoToAddress)
		EVT_MENU(XRCID("imageNextAddress"), ProjectWindow::OnImageHistoryNext)
		EVT_MENU(XRCID("imagePrevAddress"), ProjectWindow::OnImageHistoryPrevious)
		EVT_MENU(XRCID("imageFollowAddress"), ProjectWindow::OnImageFollowAddress)
		EVT_MENU(XRCID("imageReverseFollowAddress"), ProjectWindow::OnImageUnfollowAddress)
		EVT_MENU(XRCID("traceRunForward"), ProjectWindow::OnTraceFreeRunForward)
		EVT_MENU(XRCID("traceRunBackward"), ProjectWindow::OnTraceFreeRunBackward)
		EVT_MENU(XRCID("traceToggleBreakpoint"), ProjectWindow::OnTraceToggleBreakpoint)
		EVT_MENU(XRCID("traceLocalNext"), ProjectWindow::OnTraceLocalNext)
		EVT_MENU(XRCID("traceLocalPrev"), ProjectWindow::OnTraceLocalPrev)
		EVT_MENU(XRCID("traceAbsoluteNext"), ProjectWindow::OnTraceGlobalNext)
		EVT_MENU(XRCID("traceAbsolutePrev"), ProjectWindow::OnTraceGlobalPrev)
		EVT_MENU(XRCID("traceToGlobalStart"), ProjectWindow::OnTraceToGlobalStart)
		EVT_MENU(XRCID("traceToGlobalEnd"), ProjectWindow::OnTraceToGlobalEnd)
		EVT_MENU(XRCID("traceToLocalStart"), ProjectWindow::OnTraceToLocalStart)
		EVT_MENU(XRCID("traceToLocalEnd"), ProjectWindow::OnTraceToLocalEnd)
		EVT_MENU(XRCID("traceHorizontalBack"), ProjectWindow::OnTraceHorizontalPrev)
		EVT_MENU(XRCID("traceHorizontalNext"), ProjectWindow::OnTraceHorizontalNext)
	END_EVENT_TABLE()

	ProjectWindow::ProjectWindow(const std::shared_ptr<Project>& project, App* app)
		: m_project(project)
		, m_tabs(nullptr)
		, m_mainTab(nullptr)
		, m_app(app)
	{
		// load the ui
		wxXmlResource::Get()->LoadFrame(this, nullptr, wxT("ProjectWindow"));

		// prepare layout
		m_layout.SetManagedWindow(this);
		m_layout.Update();

		// update title bar with path to the project
		SetTitle(GetTitle() + wxString(" - ") + project->GetProjectPath());

		// create the internal layout
		CreateLayout();

		// refresh the menu state
		UpdateUI();
	}

	void ProjectWindow::OnActivate(wxActivateEvent& event)
	{
		event.Skip();
	}

	void ProjectWindow::OnClose(wxCloseEvent& event)
	{
		if (!m_app->SwitchToProject(nullptr))
			event.Veto();
	}

	void ProjectWindow::OnTabClosed(wxAuiNotebookEvent& event)
	{
		if (m_tabs->GetSelection() == 0)
		{
			if (!m_app->SwitchToProject(nullptr))
				event.Veto();
		}
	}

	void ProjectWindow::OnTabChanged(wxAuiNotebookEvent& event)
	{
		UpdateUI();
	}

	void ProjectWindow::CreateLayout()
	{
		// create document tabs
		m_tabs = new wxAuiNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_DEFAULT_STYLE | wxAUI_NB_CLOSE_BUTTON | wxAUI_NB_CLOSE_ON_ALL_TABS);
		m_layout.AddPane(m_tabs, wxAuiPaneInfo().Name(wxT("$Content")).Caption(wxT("Documents")).CenterPane().PaneBorder(false).CloseButton(false));

		// events :)
		m_tabs->Bind(wxEVT_AUINOTEBOOK_PAGE_CHANGED, [this](wxAuiNotebookEvent& evt) { OnTabChanged(evt); });
		m_tabs->Bind(wxEVT_AUINOTEBOOK_PAGE_CLOSE, [this](wxAuiNotebookEvent& evt) { OnTabClosed(evt); });

		// create the main project tab
		{
			m_mainTab = new ProjectMainTab(this, m_tabs);
			m_tabs->AddPage(m_mainTab, wxT("Project"));
		}

		/*		// create the memory tab
				{
					m_memoryView = new MemoryView(m_tabs);
					m_tabs->AddPage(m_memoryView, wxT("Image"));
				}

				// Create the log window
				{
					m_logView = new LogDisplayBuffer(m_bottomTabs);
					m_bottomTabs->AddPage(m_logView, wxT("Log"));
				}

				// Create the register view window
				{
					m_regView = new RegisterView(m_bottomTabs);
					m_topTabs->AddPage(m_regView, wxT("Registers"));
				}

				// Create the trace view window
				{
					m_traceView = new TraceInfoView(m_bottomTabs);
					m_topTabs->AddPage(m_traceView, wxT("Trace"));
				}*/

				// update initial format
		UpdateUI();

		// update the layout once again
		m_layout.Update();
	}

	ProjectWindow::~ProjectWindow()
	{
		m_layout.UnInit();
		wxTheFrame = nullptr;
	}

	void ProjectWindow::ToggleCommand(const unsigned int id, const bool state)
	{
		wxToolBar* toolbar = XRCCTRL(*this, "ToolBar", wxToolBar);
		if (toolbar != nullptr)
		{
			toolbar->ToggleTool(id, state);
		}

		wxMenuBar* menu = GetMenuBar();
		if (menu != nullptr)
		{
			if (menu->FindItem(id) != nullptr)
			{
				menu->Check(id, state);
			}
		}
	}

	void ProjectWindow::EnableCommand(const unsigned int id, const bool state)
	{
		wxToolBar* toolbar = XRCCTRL(*this, "ToolBar", wxToolBar);
		if (toolbar != nullptr)
		{
			toolbar->EnableTool(id, state);
		}

		wxMenuBar* menu = GetMenuBar();
		if (menu != nullptr)
		{
			if (menu->FindItem(id) != nullptr)
			{
				menu->Enable(id, state);
			}
		}
	}

	void ProjectWindow::UpdateUI()
	{
		// generic
		EnableCommand(XRCID("fileSave"), true);

		// get current page
		const wxString pageTile = (m_tabs->GetSelection() == -1) ? wxEmptyString : m_tabs->GetPageText(m_tabs->GetSelection());
		const auto isTrace = (pageTile.StartsWith("Trace"));
		const auto isImage = (pageTile.StartsWith("Image"));

		// enable/disable top-level menus
		// TODO: less hacky :)
		GetMenuBar()->EnableTop(2, isTrace | isImage);
		GetMenuBar()->EnableTop(3, isTrace);
	}

	bool ProjectWindow::NavigateToStaticAddress(const uint64 address)
	{
		return false;
	}

	ProjectTab* ProjectWindow::GetCurrentTab()
	{
		auto* currentTab = static_cast<ProjectTab*>(m_tabs->GetCurrentPage());
		return currentTab;
	}

	ProjectImageTab* ProjectWindow::GetImageTab(const std::shared_ptr<ProjectImage>& img, const bool createIfMissing /*= true*/, const bool focus /*= true*/)
	{
		// no image
		if (!img)
			return nullptr;

		// find existing tab
		ProjectImageTab* existingTab = nullptr;
		for (uint32 i = 0; i < m_tabs->GetPageCount(); ++i)
		{
			auto* tab = static_cast<ProjectTab*>(m_tabs->GetPage(i));
			if (tab->GetTabType() == ProjectTabType::Image)
			{
				auto* imageTab = static_cast<ProjectImageTab*>(tab);
				if (imageTab->GetImage() == img)
				{
					existingTab = imageTab;
					break;
				}
			}
		}

		// focus
		if (nullptr == existingTab)
		{
			// create missing tab
			if (createIfMissing)
			{
				existingTab = new ProjectImageTab(this, m_tabs, img);
				m_tabs->AddPage(existingTab, wxString::Format("Image [%hs]", img->GetDisplayName().c_str()), focus);
			}
		}
		else
		{
			if (focus)
			{
				auto index = m_tabs->GetPageIndex(existingTab);
				m_tabs->SetSelection(index);
			}
		}

		// return the tab
		return existingTab;
	}

	ProjectTraceTab* ProjectWindow::GetTraceTab(std::unique_ptr<trace::DataFile>& data, const bool focus /*= true*/)
	{
		if (!data)
			return nullptr;

		const auto name = wxString::Format("Trace [%s]", data->GetDisplayName().c_str());

		auto* projectTraceTab = new ProjectTraceTab(this, m_tabs, data);
		projectTraceTab->Navigate(NavigationType::GlobalEnd);
		m_tabs->AddPage(projectTraceTab, name, focus);
		UpdateUI();

		return projectTraceTab;
	}

	bool ProjectWindow::ImportImage(const std::wstring& imageImportPath)
	{
		std::shared_ptr<ProjectImage> importedImage;

		// load the image
		{
			auto* projectOwner = m_project.get();

			ProgressDialog progress(this, m_app->GetLogWindow(), false);
			progress.RunLongTask([&importedImage, imageImportPath, projectOwner](ILogOutput& log)
			{
				// load the image
				importedImage = ProjectImage::Create(log, projectOwner, imageImportPath);
				return 0;
			});
		}

		// image not imported
		if (!importedImage)
		{
			wxMessageBox(wxT("Failed to import image from selected file. Inspect log for details."), wxT("Import image"), wxICON_ERROR, this);
			return false;
		}

		// add to project
		m_project->AddImage(importedImage);
		m_mainTab->RefreshImageList();

		// done
		return true;
	}

	void ProjectWindow::OnProjectSave(wxCommandEvent& event)
	{
		m_project->Save(m_app->GetLogWindow());
	}

	void ProjectWindow::OnProjectClose(wxCommandEvent& event)
	{
		m_app->SwitchToProject(nullptr);
	}

	void ProjectWindow::OnExitApp(wxCommandEvent& event)
	{
		m_app->SwitchToProject(nullptr);
	}

	void ProjectWindow::OnOpenLog(wxCommandEvent& event)
	{
		m_app->ShowLog();
	}

	//--

	INavigationHelper* ProjectWindow::GetNavigatorHelperForCurrentTab()
	{
		if (m_tabs->GetSelection() != -1)
		{
			const auto tabs = m_tabs->GetPageText(m_tabs->GetSelection());
			if (tabs.StartsWith("Image "))
			{
				auto* tab = (ProjectImageTab*)m_tabs->GetCurrentPage();
				return tab;
			}
			else if (tabs.StartsWith("Trace"))
			{
				auto* tab = (ProjectTraceTab*)m_tabs->GetCurrentPage();
				return tab;
			}
		}

		return nullptr;
	}

	void ProjectWindow::OnFindSymbol(wxCommandEvent& evt)
	{
		auto* navigator = GetNavigatorHelperForCurrentTab();
		if (nullptr != navigator)
		{
			auto image = navigator->GetCurrentImage();
			if (image)
			{
				FindSymbolDialog dlg(image->GetEnvironment(), this);
				if (0 == dlg.ShowModal())
				{
					const auto address = dlg.GetSelectedSymbolAddress();
					navigator->NavigateToCodeAddress(address, true);
				}
			}
		}
	}

	bool ProjectWindow::NavigateCurrentTab(const NavigationType type)
	{
		auto* navigator = GetNavigatorHelperForCurrentTab();
		if (nullptr == navigator)
			return false;

		return navigator->Navigate(type);
	}

	void ProjectWindow::OnGoToAddress(wxCommandEvent& evt)
	{
		auto* navigator = GetNavigatorHelperForCurrentTab();
		if (nullptr != navigator)
		{
			auto image = navigator->GetCurrentImage();
			auto memory = navigator->GetCurrentMemoryView();
			if (image)
			{
				GotoAddressDialog dlg(this, image, memory);
				const auto newAddr = dlg.GetNewAddress();
				navigator->NavigateToCodeAddress(newAddr, true);
			}
		}
	}

	void ProjectWindow::OnImageHistoryNext(wxCommandEvent& evt)
	{
		if (!NavigateCurrentTab(NavigationType::HistoryForward))
			wxBell();
	}

	void ProjectWindow::OnImageHistoryPrevious(wxCommandEvent& evt)
	{
		if (!NavigateCurrentTab(NavigationType::HistoryBack))
			wxBell();
	}

	void ProjectWindow::OnImageFollowAddress(wxCommandEvent& evt)
	{
		if (!NavigateCurrentTab(NavigationType::Follow))
			wxBell();
	}

	void ProjectWindow::OnImageUnfollowAddress(wxCommandEvent& evt)
	{
		if (!NavigateCurrentTab(NavigationType::ReverseFollow))
			wxBell();
	}

	void ProjectWindow::OnTraceHorizontalPrev(wxCommandEvent& evt)
	{
		if (!NavigateCurrentTab(NavigationType::HorizontalPrev))
			wxBell();
	}

	void ProjectWindow::OnTraceHorizontalNext(wxCommandEvent& evt)
	{
		if (!NavigateCurrentTab(NavigationType::HorizontalNext))
			wxBell();
	}

	void ProjectWindow::OnTraceToLocalStart(wxCommandEvent& evt)
	{
		if (!NavigateCurrentTab(NavigationType::LocalStart))
			wxBell();
	}

	void ProjectWindow::OnTraceToLocalEnd(wxCommandEvent& evt)
	{
		if (!NavigateCurrentTab(NavigationType::LocalEnd))
			wxBell();
	}

	void ProjectWindow::OnTraceToGlobalStart(wxCommandEvent& evt)
	{
		if (!NavigateCurrentTab(NavigationType::GlobalStart))
			wxBell();
	}

	void ProjectWindow::OnTraceToGlobalEnd(wxCommandEvent& evt)
	{
		if (!NavigateCurrentTab(NavigationType::GlobalEnd))
			wxBell();
	}

	void ProjectWindow::OnTraceFreeRunForward(wxCommandEvent& evt)
	{
		if (!NavigateCurrentTab(NavigationType::RunForward))
			wxBell();
	}

	void ProjectWindow::OnTraceFreeRunBackward(wxCommandEvent& evt)
	{
		if (!NavigateCurrentTab(NavigationType::RunBackward))
			wxBell();
	}

	void ProjectWindow::OnTraceToggleBreakpoint(wxCommandEvent& evt)
	{
		if (!NavigateCurrentTab(NavigationType::ToggleBreakpoint))
			wxBell();
	}

	void ProjectWindow::OnTraceGlobalPrev(wxCommandEvent& evt)
	{
		if (!NavigateCurrentTab(NavigationType::GlobalStepBack))
			wxBell();
	}

	void ProjectWindow::OnTraceLocalPrev(wxCommandEvent& evt)
	{
		if (!NavigateCurrentTab(NavigationType::LocalStepBack))
			wxBell();
	}

	void ProjectWindow::OnTraceLocalNext(wxCommandEvent& evt)
	{
		if (!NavigateCurrentTab(NavigationType::LocalStepIn))
			wxBell();
	}

	void ProjectWindow::OnTraceGlobalNext(wxCommandEvent& evt)
	{
		if (!NavigateCurrentTab(NavigationType::GlobalStepIn))
			wxBell();
	}

	void ProjectWindow::OnTraceSyncPos(wxCommandEvent& evt)
	{
		if (!NavigateCurrentTab(NavigationType::SyncPos))
			wxBell();

	}

} // tools