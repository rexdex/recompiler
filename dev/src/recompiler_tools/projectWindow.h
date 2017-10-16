#pragma once

namespace tools
{
	class ProjectMainTab;
	class ProjectImageTab;

	// type of the tab
	enum class ProjectTabType
	{
		Main,
		Image,
		DebugSession,
		TraceSession,
	};

	// tab in the project window
	class ProjectTab : public wxPanel
	{
		DECLARE_EVENT_TABLE();

	public:
		ProjectTab(ProjectWindow* projectWindow, wxWindow* tabs, const ProjectTabType tabType);

		inline const ProjectTabType GetTabType() const { return m_tabType; }

		inline ProjectWindow* GetProjectWindow() const { return m_projectWindow; }

	private:
		ProjectWindow* m_projectWindow;
		ProjectTabType m_tabType;
	};

	// main project frame
	class ProjectWindow : public wxFrame
	{
		DECLARE_EVENT_TABLE();

	public:
		//! Get current project
		inline const std::shared_ptr<Project>& GetProject() const { return m_project; }

		//! Get the app window
		inline App* GetApp() const { return m_app; }

	public:
		ProjectWindow(const std::shared_ptr<Project>& project, App* app);
		virtual ~ProjectWindow();

		// navigate to address, this will find image matching this address and go there
		// if no image is found this function returns false
		bool NavigateToStaticAddress(const uint64 address);

		// add image to the project
		bool ImportImage(const std::wstring& imageImportPath);

		//--

		// get current focused tab
		ProjectTab* GetCurrentTab();

		// get/create tab for given project image
		ProjectImageTab* GetImageTab(const std::shared_ptr<ProjectImage>& img, const bool createIfMissing = true, const bool focus = true);

	protected:
		void OnActivate(wxActivateEvent& event);
		void OnClose(wxCloseEvent& event);
		void OnTabClosed(wxAuiNotebookEvent& event);
		void OnTabChanged(wxAuiNotebookEvent& event);

		void OnProjectSave(wxCommandEvent& event);
		void OnProjectClose(wxCommandEvent& event);
		void OnExitApp(wxCommandEvent& event);
		void OnOpenLog(wxCommandEvent& event);

		void CreateLayout();
		void UpdateUI();

		void ToggleCommand(const unsigned int id, const bool state);
		void EnableCommand(const unsigned int id, const bool state);
		
		//---

		// owner app
		App* m_app;

		// ui
		wxAuiManager m_layout;
		wxAuiNotebook* m_tabs;
		ProjectMainTab* m_mainTab;

		// loaded project
		std::shared_ptr<Project> m_project;
	};

} // tools