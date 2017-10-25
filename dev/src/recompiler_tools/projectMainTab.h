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
		void OnAddExistingImage(wxCommandEvent& evt);
		void OnRemoveImages(wxCommandEvent& evt);
		void OnCopyStartCommandline(wxCommandEvent& evt);
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

		void GetSelectedImages(std::vector<std::shared_ptr<ProjectImage>>& outImages);

		bool ImportImage(const std::wstring& imageImportPath);
		bool ImportExistingImage(const std::wstring& imageImportPath);
		bool RemoveImages(const std::vector<std::shared_ptr<ProjectImage>>& images);

		void RefreshState();
		void RefreshUI();

		const bool CheckDebugProjects();
		const bool KillProject();
		const bool StartProject(const wxString& extraCommandLine);
		const bool ImportTraceFile(const wxString& traceFilePath);
		const bool LoadTraceFile(const wxString& traceFilePath);
		const bool OpenTraceTab(std::unique_ptr<trace::DataFile>& traceData);
		const bool GetStartCommandline(wxString& outCommandLine);
	};

} // tool