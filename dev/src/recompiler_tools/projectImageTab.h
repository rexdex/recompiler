#pragma once

#include "projectWindow.h"

namespace tools
{
	class MemoryView;
	class ImageMemoryView;

	/// tab for the image
	class ProjectImageTab : public ProjectTab
	{
		DECLARE_EVENT_TABLE();

	public:
		ProjectImageTab(ProjectWindow* parent, wxWindow* tabs, const std::shared_ptr<ProjectImage>& img);
		virtual ~ProjectImageTab();

		// get the image
		inline const std::shared_ptr<ProjectImage>& GetImage() const { return m_image; }

		// navigate to previous address from the address history
		bool NavigateBack();

		// navigate to next address in the address history
		bool NavigateForward();

		// navigate to address in this image
		bool NavigateToAddress(const uint64 address, const bool addToHistory);

	private:
		std::shared_ptr<ProjectImage> m_image;

		ImageMemoryView* m_disassemblyView;
		MemoryView* m_disassemblyPanel;
		wxChoice* m_sectionList;

		wxTimer m_importSearchTimer;
		wxTimer m_exportSearchTimer;
		wxTimer m_symbolSearchTimer;

		void RefreshSectionList();
		void RefreshSettings();
		void WriteSettings();

		void DeleteTab(const char* tabName);
		void SelectTab(const char* tabName);

		void RefreshImportList();
		void RefreshExportList();
		void RefreshSymbolList();
		void RefreshFunctionList();

		void OnSelectSection(wxCommandEvent& evt);
		void OnGotoAddress(wxCommandEvent& evt);
		void OnGotoEntryAddress(wxCommandEvent& evt);
		void OnNextAddress(wxCommandEvent& evt);
		void OnPrevAddress(wxCommandEvent& evt);
		void OnSettingsChanged(wxCommandEvent& evt);
		void OnImportSelected(wxListEvent& evt);
		void OnExportSelected(wxListEvent& evt);
		void OnSymbolSelected(wxListEvent& evt);
		void OnImportSearch(wxCommandEvent& evt);
		void OnExportSearch(wxCommandEvent& evt);
		void OnSymbolSearch(wxCommandEvent& evt);
		void OnImportSearchTimer(wxTimerEvent& evt);
		void OnExportSearchTimer(wxTimerEvent& evt);
		void OnSymbolSearchTimer(wxTimerEvent& evt);
		void OnSelectFunction(wxListEvent& evt);
		void OnRefreshFunctionList(wxCommandEvent& evt);
		void OnBuildCode(wxCommandEvent& evt);
	};

} // tool