#pragma once

#include "widgetHelpers.h"
#include "projectConfiguration.h"

/// Dialog for configuring application run
class CRunDialog : public wxDialog
{
	DECLARE_EVENT_TABLE();

private:
	ProjectConfig		m_config;

public:
	CRunDialog( wxWindow* parent );
	~CRunDialog();

	// edit config, returns true if the Run button was pressed
	bool ShowDialog( ProjectConfig& retConfig );

private:
	void UpdateSettings();
	bool ReadSettings();

private:
	void OnRun( wxCommandEvent& event );
	void OnCancel( wxCommandEvent& event );
};
