#pragma once

namespace tools
{
	/// the start window
	class StartWindow : public wxFrame
	{
		DECLARE_EVENT_TABLE();

	public:
		StartWindow(App* app);

		void RefreshContent();

	private:
		wxHtmlWindow* m_html;
		App* m_app;

		void OnNewProject();
		void OnOpenProject();
		void OnOpenRecentProject(const wxString& path);

		void HandleLink(const wxString& link);

		FileDialog m_projectFiles;
		ConfigSection m_config;

		void OnClose(wxCloseEvent& evt);
	};

} // tools