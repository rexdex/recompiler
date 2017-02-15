#pragma once

#include "../backend/traceData.h"
#include "../backend/traceUtils.h"

//---------------------------------------------------------------------------

/// view showing reigster data
class CRegisterView : public wxPanel
{
	DECLARE_EVENT_TABLE();

public:
	CRegisterView(wxWindow* parent);

	// initialize register list from a CPU definition
	void InitializeRegisters(const trace::Registers& traceData);
	
	// update register values from a trace frame
	void UpdateRegisters(const trace::DataFrame& frame, const trace::DataFrame* nextFrame);

private:
	wxListCtrl*		m_list;

	trace::Registers		m_registers;
	trace::DataFrame*		m_frameCurrent;
	trace::DataFrame*		m_frameNext;

	trace::RegDisplayFormat	m_displayFormat;

	typedef std::vector< const platform::CPURegister* > TRegisterMap;
	TRegisterMap	m_usedRegisters;

	void UpdateRegisterList();
	void UpdateRegisterValues();

	void UpdateRegisterValues(const trace::DataFrame& frame, const trace::DataFrame& nextFrame);

	void OnRegisterContextMenu(wxCommandEvent& event);
	void OnToggleHexView(wxCommandEvent& event);
	void OnToggleFilter(wxCommandEvent& event);
};