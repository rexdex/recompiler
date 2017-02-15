#pragma once

#include "widgetHelpers.h"

/// Dialog that allows you to specify the target address
class CGotoAddressDialog : public wxDialog
{
	DECLARE_EVENT_TABLE();

public:
	static const uint32 INVALID_ADDRESS = 0xFFFFFFFF;

private:
	class Project*		m_project;
	class CMemoryView*	m_view;

	THelperPtr< CToggleButtonAuto >		m_decHex;
	THelperPtr< CTextBoxErrorHint >		m_addressErrorHint;

	uint32				m_address;

	wxTimer				m_navigateTimer;
	int					m_navigateIndex;

public:
	CGotoAddressDialog( wxWindow* parent, class Project* project, class CMemoryView* view );
	~CGotoAddressDialog();
	
	// show the dialog, returns the new address we want to go to or INVALID_ADDRESS
	const uint32 GetNewAddress();

private:
	void UpdateHistoryList();
	void UpdateReferenceList();
	void UpdateRelativeAddress();
	bool GrabCurrentAddress();

private:
	void OnReferenceChanged( wxCommandEvent& event );
	void OnReferenceDblClick( wxCommandEvent& event );
	void OnHistorySelected( wxCommandEvent& event );
	void OnHistoryDblClick( wxCommandEvent& event );
	void OnGoTo( wxCommandEvent& event );
	void OnCancel( wxCommandEvent& event );
	void OnAddressTypeChanged( wxCommandEvent& event );
	void OnTextKeyDown( wxKeyEvent& event );
	void OnNavigationTimer( wxTimerEvent& event );
};
