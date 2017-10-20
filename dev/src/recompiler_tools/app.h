#pragma once

namespace tools
{

	//---------------------------------------------------------------------------

	class StartWindow;
	class LogWindow;
	class ProjectWindow;
	class Project;

	//---------------------------------------------------------------------------

	class RecentProjectFiles : public AutoConfig
	{
	public:
		RecentProjectFiles();

		// get list of recent projects
		inline const wxArrayString& GetRecentProjects() const { return m_projects; }

		// register project in the list of recent project file
		void AddProject(const wxString& path);

	private:
		wxArrayString m_projects;
	};

	//---------------------------------------------------------------------------

	class App : public wxApp
	{
	public:
		App();
		~App();

		// get the path to the editor data
		inline const wxString& GetEditorPath() const { return m_editorPath; }

		// get the global log window (never closed)
		inline LogWindow& GetLogWindow() const { return *m_logWindow; }

		// get current project, may be null during startup
		inline const std::shared_ptr<Project>& GetProject() const { return m_project; }

		// get project window (may not exist)
		inline ProjectWindow* GetProjectWindow() const { return m_projectWindow; }

		// get the list of recent project
		inline RecentProjectFiles& GetRecentProjectPaths() { return m_recentProjectPaths; }

		// get the recompiler commandline api executable
		inline const wxString& GetCommandLineTool() const { return m_commandLineTool; }
		
		//--

		// switch to given loaded project
		// this closes existing project window and creates a new one
		// switching to null project reopens the startup window
		// NOTE: switching may be denied if user canceled it
		bool SwitchToProject(const std::shared_ptr<Project>& project);

		// show log window
		void ShowLog();

	private:
		// application data path
		wxString m_editorPath;
		wxString m_binaryPath;

		// Top level frames
		StartWindow* m_startWindow;
		LogWindow* m_logWindow;

		// path to command line tool
		wxString m_commandLineTool;

		// Current project
		std::shared_ptr<Project> m_project;

		// current project window
		ProjectWindow* m_projectWindow;

		// settings
		RecentProjectFiles m_recentProjectPaths;
		
		// internal events
		virtual bool OnInit();
		virtual int OnExit();
		//virtual bool OnInitGui();
		virtual void OnIdle(wxIdleEvent& event);
	};

	//---------------------------------------------------------------------------

} // tools

#undef wxTheApp
#define wxTheApp wx_static_cast( tools::App*, wxApp::GetInstance() )
