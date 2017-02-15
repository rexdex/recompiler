#pragma once

#include "widgetHelpers.h"

/// Dialog for selecting one option from many
class CChoiceDialog : public wxDialog
{
	DECLARE_EVENT_TABLE();

private:
	std::vector< std::string >		m_options;
	std::string*					m_selected;

public:
	CChoiceDialog( wxWindow* parent );
	~CChoiceDialog();

	// edit config, returns true if the Run button was pressed
	bool ShowDialog( const std::string& title, const std::string& message, const std::vector< std::string >& options, std::string& selected );

private:
	void OnOK( wxCommandEvent& event );
	void OnCancel( wxCommandEvent& event );
};
