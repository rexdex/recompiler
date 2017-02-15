#include "build.h"
#include "project.h"
#include "projectMemoryContextMenu.h"
#include "frame.h"
#include "widgetHelpers.h"
#include "bitmaps.h"

#include "../backend/image.h"
#include "../backend/decodingEnvironment.h"
#include "../backend/decodingContext.h"
#include "../backend/decodingMemoryMap.h"

CProjectMemoryViewContextMenu::CProjectMemoryViewContextMenu( class Project* project, const class image::Section* section, const uint32 startOffset, const uint32 endOffset )
	: m_project( project )
	, m_section( section )
	, m_startAddress( startOffset )
	, m_endAddress( endOffset )
{
}

void CProjectMemoryViewContextMenu::Show( wxWindow* window, const wxPoint& mousePoint )
{
	// section does allow conversion to code ?
	const bool canConvertToCode = m_section->CanExecute();

	// get the selected size
	const uint32 selectedSize = (m_endAddress - m_startAddress);

	// extract memory flags from the selected memory range
	bool hasValidCode = false;
	bool hasInvalidCode = false;
	for ( uint32 i=m_startAddress; i<=m_endAddress; ++i )
	{
		const auto flags = m_project->GetEnv().GetDecodingContext()->GetMemoryMap().GetMemoryInfo( i );
		if ( flags.IsExecutable() )
		{
			const auto iflags = flags.GetInstructionFlags();
			if ( iflags.IsValid() )
			{
				hasValidCode = true;
			}
			else
			{
				hasInvalidCode = true;
			}
		}
	}

	// memory range information
	{
		char buffer[64];
		sprintf_s( buffer, sizeof(buffer), "Memory %06Xh-%06Xh (size: %u)", m_startAddress, m_endAddress, m_endAddress-m_startAddress );
		wxMenuItem* item = new wxMenuItem( this, 0, buffer );
		item->SetFont( wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT ).Bold() );
		item->Enable( false );
		Append( item );
		AppendSeparator();
	}

	bool needsSeparator = false;

	// macro id counter
	int macroID = ID_StartMacro_Frist;

	// show the options
	if ( hasInvalidCode || canConvertToCode )
	{
		if ( needsSeparator ) AppendSeparator();
		Append( ID_ConvertToCode, "Decode as instructions\tC" );
		Connect( ID_ConvertToCode, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CProjectMemoryViewContextMenu::OnConvertToCode ), NULL, this  );
		needsSeparator = true;
	}

	// data options
	if ( !hasValidCode )
	{
		if ( needsSeparator ) AppendSeparator();
		Append( ID_ConvertToDataUint8, "Convert to Uint8\tShift+1" );
		Connect( ID_ConvertToDataUint8, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CProjectMemoryViewContextMenu::OnConvertToDataUint8 ), NULL, this  );
		Append( ID_ConvertToDataUint16, "Convert to Uint16\tShift+2" );
		Connect( ID_ConvertToDataUint16, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CProjectMemoryViewContextMenu::OnConvertToDataUint16 ), NULL, this  );
		Append( ID_ConvertToDataUint32, "Convert to Uint32\tShift+3" );
		Connect( ID_ConvertToDataUint32, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CProjectMemoryViewContextMenu::OnConvertToDataUint32 ), NULL, this  );
		Append( ID_ConvertToDataUint64, "Convert to Uint64\tShift+4" );
		Connect( ID_ConvertToDataUint64, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CProjectMemoryViewContextMenu::OnConvertToDataUint64 ), NULL, this  );
		Append( ID_ConvertToDataFloat, "Convert to Float\tShift+F" );
		Connect( ID_ConvertToDataFloat, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CProjectMemoryViewContextMenu::OnConvertToDataFloat ), NULL, this  );
		Append( ID_ConvertToDataDouble, "Convert to Double\tShift+D" );
		Connect( ID_ConvertToDataDouble, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CProjectMemoryViewContextMenu::OnConvertToDataDouble ), NULL, this  );
		Append( ID_ConvertToDataVec4, "Convert to Vec4\tShift+V" );
		Connect( ID_ConvertToDataVec4, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CProjectMemoryViewContextMenu::OnConvertToDataVec4 ), NULL, this  );
		needsSeparator = true;
	}

	// memory flags

	// reset
	{
		if ( needsSeparator ) AppendSeparator();
		Append( ID_ConvertToDataUint8, "Restore defaults\tCtrl+D" );
		Connect( ID_ConvertToDataUint8, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CProjectMemoryViewContextMenu::OnConvertToDefault ), NULL, this  );
	}

	// show the menu
	window->PopupMenu( this );
}

void CProjectMemoryViewContextMenu::OnConvertToDefault( wxCommandEvent& event )
{
}

void CProjectMemoryViewContextMenu::OnConvertToCode( wxCommandEvent& event )
{
	wxTheFrame->OnConvertToCode( event );
}

void CProjectMemoryViewContextMenu::OnConvertToDataUint8( wxCommandEvent& event )
{
}

void CProjectMemoryViewContextMenu::OnConvertToDataUint16( wxCommandEvent& event )
{
}

void CProjectMemoryViewContextMenu::OnConvertToDataUint32( wxCommandEvent& event )
{
}

void CProjectMemoryViewContextMenu::OnConvertToDataUint64( wxCommandEvent& event )
{
}

void CProjectMemoryViewContextMenu::OnConvertToDataInt8( wxCommandEvent& event )
{
}

void CProjectMemoryViewContextMenu::OnConvertToDataInt16( wxCommandEvent& event )
{
}

void CProjectMemoryViewContextMenu::OnConvertToDataInt32( wxCommandEvent& event )
{
}

void CProjectMemoryViewContextMenu::OnConvertToDataInt64( wxCommandEvent& event )
{
}

void CProjectMemoryViewContextMenu::OnConvertToDataFloat( wxCommandEvent& event )
{
}

void CProjectMemoryViewContextMenu::OnConvertToDataDouble( wxCommandEvent& event )
{
}

void CProjectMemoryViewContextMenu::OnConvertToDataVec4( wxCommandEvent& event )
{
}

void CProjectMemoryViewContextMenu::OnStartMacroSelection( wxCommandEvent& event )
{
}

void CProjectMemoryViewContextMenu::OnStartMacroInstruction( wxCommandEvent& event )
{
}

