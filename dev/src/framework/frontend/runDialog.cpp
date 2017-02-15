#include "build.h"
#include "runDialog.h"

BEGIN_EVENT_TABLE( CRunDialog, wxDialog )
	EVT_BUTTON( XRCID("Run"), CRunDialog::OnRun )
	EVT_BUTTON( XRCID("Cancel"), CRunDialog::OnCancel )
END_EVENT_TABLE();

CRunDialog::CRunDialog( wxWindow* parent )
{
	// load the dialog
	wxXmlResource::Get()->LoadDialog( this, parent, wxT("RunOptions") );

	// reset local config
	m_config.Reset( L".\\" );
}

CRunDialog::~CRunDialog()
{
}

bool CRunDialog::ShowDialog( ProjectConfig& retConfig )
{
	m_config = retConfig;
	UpdateSettings();	

	if ( 0 == wxDialog::ShowModal() )
	{
		retConfig = m_config;
		return true;
	}

	return false;
}

void CRunDialog::UpdateSettings()
{
	// generic options
	XRCCTRL( *this, "checkTraceResults", wxCheckBox )->Set3StateValue( m_config.m_trace ? wxCHK_CHECKED : wxCHK_UNCHECKED );
	//XRCCTRL( *this, "checkOpenTraceFile", wxCheckBox )->Set3StateValue( m_config.m_loadTrace ? wxCHK_CHECKED : wxCHK_UNCHECKED );
	XRCCTRL( *this, "checkShowResultLog", wxCheckBox )->Set3StateValue( m_config.m_showLog ? wxCHK_CHECKED : wxCHK_UNCHECKED );
	XRCCTRL( *this, "checkVerboseLog", wxCheckBox )->Set3StateValue( m_config.m_verboseLog ? wxCHK_CHECKED : wxCHK_UNCHECKED );
	XRCCTRL( *this, "dirDVD", wxTextCtrl )->SetValue( m_config.m_dvdDir.c_str() );
	XRCCTRL( *this, "dirDevkit", wxTextCtrl )->SetValue( m_config.m_devkitDir.c_str() );

	// project configurations

	//m_configToRun
}

bool CRunDialog::ReadSettings()
{
	m_config.m_trace = XRCCTRL( *this, "checkTraceResults", wxCheckBox )->IsChecked();
	//m_config.m_loadTrace = XRCCTRL( *this, "checkOpenTraceFile", wxCheckBox )->IsChecked();
	m_config.m_showLog = XRCCTRL( *this, "checkShowResultLog", wxCheckBox )->IsChecked();
	m_config.m_verboseLog = XRCCTRL( *this, "checkVerboseLog", wxCheckBox )->IsChecked();
	m_config.m_dvdDir = XRCCTRL( *this, "dirDVD", wxTextCtrl )->GetValue();
	m_config.m_devkitDir = XRCCTRL( *this, "dirDevkit", wxTextCtrl )->GetValue();
	return true;
}

void CRunDialog::OnRun( wxCommandEvent& event )
{
	if ( ReadSettings() )
	{
		EndDialog( 0 );
	}
	else
	{
		wxMessageBox( wxT("Specified settings are invalid"), wxT("Run configuration error"), wxICON_ERROR | wxOK, this );
	}
}

void CRunDialog::OnCancel( wxCommandEvent& event )
{
	EndDialog( -1 );
}