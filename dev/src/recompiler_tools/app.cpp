#include "build.h"
#include "eventDispatcher.h"
#include "configManager.h"
#include "startWindow.h"
#include "project.h"
#include "projectWindow.h"

#include "../recompiler_core/platformLibrary.h"

//---------------------------------------------------------------------------

IMPLEMENT_APP(tools::App);

//---------------------------------------------------------------------------

namespace tools
{

	//---

	RecentProjectFiles::RecentProjectFiles()
		: AutoConfig("RecentProjects")
	{
		AddConfigValue(m_projects, "Paths");
	}

	void RecentProjectFiles::AddProject(const wxString& path)
	{
		if (m_projects.Index(path) != -1)
			m_projects.Remove(path);

		m_projects.Insert(path, 0);
	}

	//---

	App::App()
		: m_startWindow(nullptr)
		, m_logWindow(nullptr)
		, m_projectWindow(nullptr)
	{
		wxFileName f(wxStandardPaths::Get().GetExecutablePath());
		f.ClearExt();
		f.SetName(wxEmptyString);

		// get the path to binary folder
		m_binaryPath = f.GetPath();
		m_binaryPath += wxFileName::GetPathSeparator();

		// get the data path
		f.RemoveLastDir();
		f.RemoveLastDir();
		f.AppendDir("data");
		f.AppendDir("ui");
		f.Normalize();
		m_editorPath = f.GetPath();
		m_editorPath += wxFileName::GetPathSeparator();
	}

	App::~App()
	{
	}

	void App::OnIdle(wxIdleEvent& event)
	{
		// update application
		wxApp::OnIdle(event);

		// Dispatch events
		EventDispatcher::GetInstance().DispatchPostedEvents();

		// send the tick
		EventDispatcher::GetInstance().DispatchEvent("Tick");

		// request more events if we still have something to process
		event.RequestMore();
	}

	bool App::OnInit()
	{
		//	LoadLibraryA( "comctl32.dll" );
		//InitCommonControls();

		// app name
		SetVendorName(wxT("Recompiler"));
		SetAppName(wxT("Recompiler v1.0"));

		/// initialize GDI+
		//Gdiplus::GdiplusStartupInput gdiplusStartupInput;
		//Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);

		// initialize XML resources
		wxInitAllImageHandlers();
		wxXmlResource::Get()->InitAllHandlers();

		// Initialize XRC resources
		wxXmlResource::Get()->LoadAllFiles(m_editorPath);

		// load all config sections registered so far
		ConfigurationManager::GetInstance().LoadConfig();

		// create the log window
		m_logWindow = new LogWindow();
		m_logWindow->SetIcon(wxICON(amain));

		// initialize library
		if (!platform::Library::GetInstance().Initialize(GetLogWindow()))
		{
			wxMessageBox(wxT("Failed to load data library"), wxT("Error initializing application"), wxOK | wxICON_ERROR);
			return false;
		}

		// determine path to the command line tool
		{
			wxFileName file(wxStandardPaths::Get().GetExecutablePath());
			file.SetName("recompiler_api");
			m_commandLineTool = file.GetFullPath();
		}

		// connect the OnIdle
		Connect(wxEVT_IDLE, wxIdleEventHandler(App::OnIdle), NULL, this);

		// show the startup dialog
		m_startWindow = new StartWindow(this);
		m_startWindow->SetIcon(wxICON(amain));
		SetTopWindow(m_startWindow);

		// Initialized
		return true;
	}

	int App::OnExit()
	{
		//// close GDI+
		//Gdiplus::GdiplusShutdown(m_gdiplusToken);

		// deferred cleanup
		DeletePendingObjects();

		// close library
		platform::Library::GetInstance().Close();

		// close application
		return wxApp::OnExit();
	}

	void App::ShowLog()
	{
		m_logWindow->Show();
		m_logWindow->SetFocus();
	}

	bool App::SwitchToProject(const std::shared_ptr<Project>& project)
	{
		// save existing project
		if (m_project)
		{
			// ask if we should save the changes
			if (m_project->IsModified())
			{
				const auto ret = wxMessageBox("There were changes in the project. Save them?", "Exiting", wxYES_NO | wxCANCEL | wxICON_QUESTION, nullptr);
				if (ret == wxCANCEL)
					return false;

				else if (ret == wxYES)
				{
					if (!m_project->Save(wxTheApp->GetLogWindow()))
					{
						wxMessageBox("Failed to save project changes", "Exiting", wxICON_ERROR, nullptr);
						return false;
					}
				}
			}

			// drop project and window
			ScheduleForDestruction(m_projectWindow);
			m_projectWindow = nullptr;
			m_project.reset();
		}

		// create a new window or show the startup window
		if (project)
		{
			// create new project window
			m_project = project;
			m_projectWindow = new ProjectWindow(project, this);

			// make the project current
			m_recentProjectPaths.AddProject(m_project->GetProjectPath());
			ConfigurationManager::GetInstance().SaveConfig();

			// switch to project window
			SetTopWindow(m_projectWindow);
			m_projectWindow->Maximize();
			m_projectWindow->Show();
			m_projectWindow->SetFocus();
			m_startWindow->Hide();
		}
		else
		{
			// switch to startup window
			SetTopWindow(m_startWindow);
			m_startWindow->RefreshContent();
			m_startWindow->Show();
		}

		// switched
		return true;
	}


} // tools