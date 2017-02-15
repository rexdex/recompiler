#include "build.h"
#include "registerView.h"
#include "widgetHelpers.h"
#include "../backend/platformCPU.h"

//---------------------------------------------------------------------------

BEGIN_EVENT_TABLE(CRegisterView, wxPanel)
	EVT_TOOL( XRCID("filterRelevantOnly"), CRegisterView::OnToggleFilter )
	EVT_TOOL( XRCID("showHex"), CRegisterView::OnToggleHexView )
END_EVENT_TABLE()

CRegisterView::CRegisterView(wxWindow* parent)
	: m_list(NULL)
	, m_displayFormat(trace::RegDisplayFormat::Auto)
{
	wxXmlResource::Get()->LoadPanel(this, parent, wxT("Registers") );

	m_list = XRCCTRL(*this, "RegList", wxListCtrl);
	m_list->AppendColumn( "Reg", wxLIST_FORMAT_CENTER, 70 );
	m_list->AppendColumn( "In", wxLIST_FORMAT_LEFT, 200 );
	m_list->AppendColumn( "Out", wxLIST_FORMAT_LEFT, 200 );

	Layout();
	Show();
}

void CRegisterView::InitializeRegisters(const trace::Registers& registers)
{
	// just copy all registers used in the trace
	m_registers = registers;

	// refresh register list
	UpdateRegisterList();
}

void CRegisterView::UpdateRegisterList()
{
	// reset
	m_list->Freeze();
	m_list->DeleteAllItems();

	// prepare filtering list
	m_usedRegisters.clear();
	const uint32 numRegs = m_registers.GetTraceRegisters().size();
	for (uint32 i=0; i<numRegs; ++i)
	{
		const platform::CPURegister* reg = m_registers.GetTraceRegisters()[i];
		if ( reg->GetParent() == nullptr )
		{
			int index = m_list->InsertItem(m_list->GetItemCount(), reg->GetName() );
			m_usedRegisters.push_back(reg);
		}
	}

	// update register values
	UpdateRegisterValues();

	// end update
	m_list->Thaw();
	m_list->Refresh();
}


void CRegisterView::UpdateRegisterValues(const trace::DataFrame& frame, const trace::DataFrame& nextFrame)
{
	// start update
	m_list->Freeze();

	// update the values according to trace results
	for (uint32 i = 0; i < m_usedRegisters.size(); ++i )
	{
		const platform::CPURegister* reg = m_usedRegisters[i];

		// update value
		const auto curVal = trace::GetRegisterValueText(reg, frame, m_displayFormat);
		const auto nextVal = trace::GetRegisterValueText(reg, nextFrame, m_displayFormat);

		m_list->SetItem( i, 1, curVal.c_str() );
		m_list->SetItem( i, 2, nextVal.c_str());

		const bool isModified = (curVal != nextVal);
		const wxColour drawColor = isModified ? wxColour(255,0,0) : wxColour(0,0,0);
		m_list->SetItemTextColour(i, drawColor);
	}

	// end update
	m_list->Thaw();
	m_list->Refresh(false);
}

void CRegisterView::UpdateRegisterValues()
{
}

void CRegisterView::UpdateRegisters(const trace::DataFrame& frame, const trace::DataFrame* nextFrame)
{
	UpdateRegisterValues(frame, nextFrame ? *nextFrame : frame);
}

//---------------------------------------------------------------------------

void CRegisterView::OnRegisterContextMenu(wxCommandEvent& event)
{
}

void CRegisterView::OnToggleHexView(wxCommandEvent& event)
{
	UpdateRegisterValues();
}

void CRegisterView::OnToggleFilter(wxCommandEvent& event)
{
}

//---------------------------------------------------------------------------
