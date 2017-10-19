#pragma once

namespace tools
{
	/// tab for the workspace setup
	class ProjectMainTab : public ProjectTab
	{
		DECLARE_EVENT_TABLE();

	public:
		ProjectMainTab(ProjectWindow* parent, wxWindow* tabs);

		void RefreshImageList();

	private:
		wxListCtrl* m_imageList;
		uint32_t m_activeProjectInstance;
		wxString m_activeTraceFile;
		wxTimer m_refreshTimer;

		void OnAddImage(wxCommandEvent& evt);
		void OnRemoveImages(wxCommandEvent& evt);
		void OnShowImageDetails(wxListEvent& evt);

		void OnShowLog(wxCommandEvent& evt);
		void OnCodeBuild(wxCommandEvent& evt);
		void OnCodeRunDebug(wxCommandEvent& evt);
		void OnCodeRunNormal(wxCommandEvent& evt);
		void OnCodeRunTrace(wxCommandEvent& evt);
		void OnLoadTrace(wxCommandEvent& evt);
		void OnImportTrace(wxCommandEvent& evt);
		void OnKill(wxCommandEvent& evt);
		void OnRefreshTimer(wxTimerEvent & evt);

		void RefreshState();
		void RefreshUI();

		const bool CheckDebugProjects();
		const bool KillProject();
		const bool StartProject(const wxString& extraCommandLine);
		const bool ImportTraceFile(const wxString& traceFilePath);
		const bool LoadTraceFile(const wxString& traceFilePath);
		const bool OpenTraceTab(std::unique_ptr<trace::DataFile>& traceData);
	};

} // tool