#include "build.h"
#include "startWindow.h"
#include "htmlBuilder.h"
#include "project.h"

#include "../recompiler_core/platformLibrary.h"
#include "../recompiler_core/platformDefinition.h"
#include "config.h"
#include "choiceDialog.h"
#include "progressDialog.h"

namespace tools
{

	BEGIN_EVENT_TABLE(StartWindow, wxFrame)
		EVT_CLOSE(StartWindow::OnClose)
	END_EVENT_TABLE()

	StartWindow::StartWindow(App* app)
		: m_config(L"StartWindow")
		, m_app(app)
	{
		// load the dialog
		wxXmlResource::Get()->LoadFrame(this, nullptr, wxT("StartWindow"));

		// get controls
		m_html = XRCCTRL(*this, "HTML", wxHtmlWindow);
		m_html->Bind(wxEVT_HTML_LINK_CLICKED, [this](const wxHtmlLinkEvent& e) { HandleLink(e.GetLinkInfo().GetHref()); });

		// initialize content
		RefreshContent();

		// finalize
		Layout();
		Show();
	}

	void StartWindow::RefreshContent()
	{
		HTMLBuilder b;
		HTMLScope s(b, "center");

		// logo
		{
			HTMLGrid g(s, 2, 0, false, 600, 10);
			g.BeginRow();
			g.BeginCell();
			b.Image("images/reee.png", 256, 256);
			g.BeginCell();
			{
				HTMLScope s2(s, "center");
				HTMLScope s3(s2, "p");
				HTMLScope s4(s3, "font");
				s4.Attr("color", "#458B43");
				b.PrintBlock("h1", "REcompiler v1.0");
			}
			{
				HTMLScope s2(s, "center");
				HTMLScope s3(s2, "p");
				b.Print("The open source executable converter");
				b.LineBreak();
				b.Print("(C) RexDex, 2015 - 2018");
				b.LineBreak();
				b.LineBreak();
				b.LineBreak();
				b.PrintEncoded("Build " __DATE__ " " __TIME__);
			}
		}

		// actions
		{
			b.PrintBlock("h2", "Project actions");

			HTMLGrid g(s, 2, 0, false, -1, 2);
			g.BeginRow();
			{
				g.BeginCell();
				b.Image("icons/new.png");

				g.BeginCell();
				HTMLLink a(g);
				a.Link("NewProject");
				a.Text("New project");
			}
			g.BeginRow();
			{
				g.BeginCell();
				b.Image("icons/open.png");

				g.BeginCell();
				HTMLLink a(g);
				a.Link("OpenProject");
				a.Text("Open project");
			}
		}

		// recent projects
		const auto& recentProjects = m_app->GetRecentProjectPaths().GetRecentProjects();
		if (!recentProjects.empty())
		{
			b.PrintBlock("h2", "Recent projects");

			for (const auto& path : recentProjects)
			{
				HTMLGrid g(s, 2, 0, false, -1, 2);
				g.BeginRow();

				g.BeginCell();
				b.Image("icons/open.png");

				g.BeginCell();
				HTMLLink a(g);
				a.Link("RecentProject:%ls", path.wc_str());
				a.Text("%ls", path.wc_str());
			}
		}

		m_html->SetPage(b.Extract());
	}

	void StartWindow::HandleLink(const wxString& link)
	{
		if (link == "NewProject")
		{
			OnNewProject();
		}
		else if (link == "OpenProject")
		{
			OnOpenProject();
		}
		else if (link.StartsWith("RecentProject:"))
		{
			const auto path = link.Mid(strlen("RecentProject:"));
			OnOpenRecentProject(path);
		}
	}

	void StartWindow::OnNewProject()
	{
		// enumerate platforms
		const auto numPlatforms = platform::Library::GetInstance().GetNumPlatforms();
		if (numPlatforms == 0)
		{
			wxMessageBox(wxT("No platform libraries found"), wxT("Recompiler Error"), wxICON_ERROR, this);
			return;
		}

		// get platform names
		std::vector<std::string> platformNames;
		for (uint32 i = 0; i < numPlatforms; ++i)
		{
			const auto* pl = platform::Library::GetInstance().GetPlatform(i);

			// format the platform name string with the supported executable list e.g. "Xbox360 [xex]"
			std::string platformName = pl->GetName();
			platformName += " [";

			std::vector<platform::FileFormat> formats;
			pl->EnumerateImageFormats(formats);
			for (uint32 j = 0; j < formats.size(); ++j)
			{
				if (j > 0)
					platformName += ", ";

				platformName += formats[i].m_extension;
			}
			platformName += "]";
			platformNames.push_back(platformName);
		}

		// get the last selected platform
		std::string platformName = m_config.ReadStringStd(wxT("Platform"), "");
		if (platformName.empty())
			platformName = platformNames[0];

		// ask user to select the platform
		ChoiceDialog dlg(this);
		if (!dlg.ShowDialog("New project", "Please select platform for the new project:", platformNames, platformName))
			return;

		// find platform by name
		const auto platformIndex = std::find(platformNames.begin(), platformNames.end(), platformName) - platformNames.begin();
		const auto* platformDefinition = platform::Library::GetInstance().GetPlatform(platformIndex);
		if (!platformDefinition)
			return;

		// ask for the actual file name
		wxFileDialog saveFileDialog(this, "Save project file", "", "project.rep", "Project files (*.rep)|*.rep", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
		if (saveFileDialog.ShowModal() == wxID_CANCEL)
			return;

		// get file extension
		const std::wstring filePath = saveFileDialog.GetPath().wc_str();
		const auto createdProject = Project::CreateProject(m_app->GetLogWindow(), filePath, platformDefinition);
		if (!createdProject)
		{
			wxMessageBox(wxT("Failed to create project. Inspect log for details."), wxT("Create Project"), wxICON_ERROR);
			return;
		}

		// switch to project
		m_app->SwitchToProject(createdProject);
	}

	void StartWindow::OnOpenProject()
	{
		// ask for the actual file name
		wxFileDialog loadFileDialog(this, "Load project file", "", "", "Project files (*.rep)|*.rep", wxFD_OPEN);
		if (loadFileDialog.ShowModal() == wxID_CANCEL)
			return;

		// get file extension
		const std::wstring filePath = loadFileDialog.GetPath().wc_str();

		// load the recent projects
		std::shared_ptr<Project> loadedProject;
		ProgressDialog progress(this, m_app->GetLogWindow(), false);
		progress.RunLongTask([&loadedProject, filePath](ILogOutput& log)
		{
			loadedProject = Project::LoadProject(log, filePath);
			return 0;
		});

		// failed to load
		if (!loadedProject)
		{
			wxMessageBox("Failed to load selected project", "Open project", wxICON_ERROR, this);
			return;
		}

		// switch to loaded project
		m_app->SwitchToProject(loadedProject);
	}

	void StartWindow::OnOpenRecentProject(const wxString& path)
	{
		std::shared_ptr<Project> loadedProject;

		// load the recent projects
		ProgressDialog progress(this, m_app->GetLogWindow(), false);
		progress.RunLongTask([&loadedProject, path](ILogOutput& log)
		{
			loadedProject = Project::LoadProject(log, path.wc_str());
			return 0;
		});

		// check if the project was loaded
		if (!loadedProject)
		{
			if (wxYES == wxMessageBox("Failed to load selected project. Remove from the list of recent projects?", "Open project", wxICON_ERROR | wxYES_NO, this))
			{

			}
			return;
		}

		// switch to loaded project
		m_app->SwitchToProject(loadedProject);
	}

	void StartWindow::OnClose(wxCloseEvent& evt)
	{
		const auto ret = wxMessageBox("Exit application?", "Exiting", wxYES_NO | wxICON_QUESTION, this);
		if (ret == wxNO)
		{
			evt.Veto();
		}
		else
		{
			wxExit();
			// TODO
		}
	}

} // tools
