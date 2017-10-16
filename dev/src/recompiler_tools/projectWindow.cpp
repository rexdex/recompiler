#include "build.h"
#include "project.h"
#include "projectImage.h"
#include "projectWindow.h"
#include "projectMainTab.h"
#include "projectImageTab.h"
#include "app.h"
#include "progressDialog.h"

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

	void ProjectWindow::CreateLayout()
	{
		// create document tabs
		m_tabs = new wxAuiNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_DEFAULT_STYLE & ~wxAUI_NB_CLOSE_BUTTON & ~wxAUI_NB_CLOSE_ON_ACTIVE_TAB);
		m_layout.AddPane(m_tabs, wxAuiPaneInfo().Name(wxT("$Content")).Caption(wxT("Documents")).CenterPane().PaneBorder(false).CloseButton(false));

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
		EnableCommand(XRCID("fileSave"), true);

		EnableCommand(XRCID("codeBuild"), (m_project != nullptr));
		EnableCommand(XRCID("codeRun"), (m_project != nullptr));
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

} // tools