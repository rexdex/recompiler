#pragma once

/// Context menu displayed for memory view
class CProjectMemoryViewContextMenu : public wxMenu
{
	enum IDS
	{
		ID_ConvertToDefault=100,

		ID_ConvertToCode=150,

		ID_ConvertToDataUint8=200,
		ID_ConvertToDataUint16=201,
		ID_ConvertToDataUint32=202,
		ID_ConvertToDataUint64=203,
		ID_ConvertToDataInt8=204,
		ID_ConvertToDataInt16=205,
		ID_ConvertToDataInt32=206,
		ID_ConvertToDataInt64=207,
		ID_ConvertToDataFloat=208,
		ID_ConvertToDataDouble=209,
		ID_ConvertToDataVec4=210,

		ID_ToggleMemoryFlags_First=1000,
		ID_ToggleCodeFlags_First=1100,
		ID_ToggleDataFlags_First=1200,

		ID_StartMacro_Frist=1300,
	};

private:
	class Project*			m_project;
	const image::Section*		m_section;
	uint32					m_startAddress;
	uint32					m_endAddress;

public:
	CProjectMemoryViewContextMenu( class Project* project, const class image::Section* section, const uint32 startAddress, const uint32 endAddress );
	void Show( wxWindow* window, const wxPoint& mousePoint );

private:
	void OnConvertToDefault( wxCommandEvent& event );
	void OnConvertToCode( wxCommandEvent& event );
	void OnConvertToDataUint8( wxCommandEvent& event );
	void OnConvertToDataUint16( wxCommandEvent& event );
	void OnConvertToDataUint32( wxCommandEvent& event );
	void OnConvertToDataUint64( wxCommandEvent& event );
	void OnConvertToDataInt8( wxCommandEvent& event );
	void OnConvertToDataInt16( wxCommandEvent& event );
	void OnConvertToDataInt32( wxCommandEvent& event );
	void OnConvertToDataInt64( wxCommandEvent& event );
	void OnConvertToDataFloat( wxCommandEvent& event );
	void OnConvertToDataDouble( wxCommandEvent& event );
	void OnConvertToDataVec4( wxCommandEvent& event );

	void OnToggleMemoryFlag( wxCommandEvent& event );
	void OnToggleInstructioFlag( wxCommandEvent& event );
	void OnToggleDataFlag( wxCommandEvent& event );

	void OnStartMacroSelection( wxCommandEvent& event );
	void OnStartMacroInstruction( wxCommandEvent& event );
};
