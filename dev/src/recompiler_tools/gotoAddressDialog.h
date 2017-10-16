#pragma once

#include "widgetHelpers.h"

namespace tools
{
	class ProjectImage;
	class MemoryView;

	/// Dialog that allows you to specify the target address
	class GotoAddressDialog : public wxDialog
	{
		DECLARE_EVENT_TABLE();

	public:
		static const uint32 INVALID_ADDRESS = 0xFFFFFFFF;

	public:
		GotoAddressDialog(wxWindow* parent, const std::shared_ptr<ProjectImage>& projectImage, MemoryView* view);
		~GotoAddressDialog();

		// show the dialog, returns the new address we want to go to or INVALID_ADDRESS
		const uint64 GetNewAddress();

	private:
		void UpdateHistoryList();
		void UpdateReferenceList();
		void UpdateRelativeAddress();
		bool GrabCurrentAddress();

		void OnReferenceChanged(wxCommandEvent& event);
		void OnReferenceDblClick(wxCommandEvent& event);
		void OnHistorySelected(wxCommandEvent& event);
		void OnHistoryDblClick(wxCommandEvent& event);
		void OnGoTo(wxCommandEvent& event);
		void OnCancel(wxCommandEvent& event);
		void OnAddressTypeChanged(wxCommandEvent& event);
		void OnTextKeyDown(wxKeyEvent& event);
		void OnNavigationTimer(wxTimerEvent& event);

		///--

		std::shared_ptr<ProjectImage> m_image;
		class MemoryView* m_view;

		THelperPtr< ToggleButtonAuto >		m_decHex;
		THelperPtr< TextBoxErrorHint >		m_addressErrorHint;

		uint64 m_address;

		wxTimer m_navigateTimer;
		int	m_navigateIndex;
	};
} // tools