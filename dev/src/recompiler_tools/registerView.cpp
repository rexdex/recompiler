#include "build.h"
#include "registerView.h"
#include "widgetHelpers.h"
#include "../recompiler_core/platformCPU.h"

namespace tools
{

	//---------------------------------------------------------------------------

	BEGIN_EVENT_TABLE(RegisterView, wxPanel)
		EVT_TOOL(XRCID("filterRelevantOnly"), RegisterView::OnToggleFilter)
		EVT_TOOL(XRCID("showHex"), RegisterView::OnToggleHexView)
	END_EVENT_TABLE()

	RegisterView::RegisterView(wxWindow* parent)
		: m_list(nullptr)
		, m_cpu(nullptr)
		, m_displayFormat(trace::RegDisplayFormat::Auto)
	{
		wxXmlResource::Get()->LoadPanel(this, parent, wxT("Registers"));

		m_list = XRCCTRL(*this, "RegList", wxListCtrl);
		m_list->AppendColumn("Reg", wxLIST_FORMAT_CENTER, 70);
		m_list->AppendColumn("In", wxLIST_FORMAT_LEFT, 200);
		m_list->AppendColumn("Out", wxLIST_FORMAT_LEFT, 200);

		Layout();
		Show();
	}

	void RegisterView::InitializeRegisters(const platform::CPU& cpuInfo)
	{
		m_cpu = &cpuInfo;
		UpdateRegisterList();
	}

	void RegisterView::UpdateRegisterList()
	{
		// reset
		m_list->Freeze();
		m_list->DeleteAllItems();

		// prepare filtering list
		m_usedRegisters.clear();
		const uint32 numRegs = m_cpu->GetNumRootRegisters();
		for (uint32 i = 0; i < numRegs; ++i)
		{
			const platform::CPURegister* reg = m_cpu->GetRootRegister(i);

			int index = m_list->InsertItem(m_list->GetItemCount(), reg->GetName());
			m_usedRegisters.push_back(reg);
		}

		// update register values
		UpdateRegisterValues();

		// end update
		m_list->Thaw();
		m_list->Refresh();
	}


	void RegisterView::UpdateRegisterValues(const trace::DataFrame& frame, const trace::DataFrame& nextFrame)
	{
		// start update
		m_list->Freeze();

		// update the values according to trace results
		for (uint32 i = 0; i < m_usedRegisters.size(); ++i)
		{
			const platform::CPURegister* reg = m_usedRegisters[i];

			// update value
			const auto curVal = trace::GetRegisterValueText(reg, frame, m_displayFormat);
			const auto nextVal = trace::GetRegisterValueText(reg, nextFrame, m_displayFormat);

			m_list->SetItem(i, 1, curVal.c_str());
			m_list->SetItem(i, 2, nextVal.c_str());

			const bool isModified = (curVal != nextVal);
			const wxColour drawColor = isModified ? wxColour(255, 0, 0) : wxColour(0, 0, 0);
			m_list->SetItemTextColour(i, drawColor);
		}

		// end update
		m_list->Thaw();
		m_list->Refresh(false);
	}

	void RegisterView::UpdateRegisterValues()
	{
	}

	void RegisterView::UpdateRegisters(const trace::DataFrame& frame, const trace::DataFrame* nextFrame)
	{
		UpdateRegisterValues(frame, nextFrame ? *nextFrame : frame);
	}

	//---------------------------------------------------------------------------

	void RegisterView::OnRegisterContextMenu(wxCommandEvent& event)
	{
	}

	void RegisterView::OnToggleHexView(wxCommandEvent& event)
	{
		UpdateRegisterValues();
	}

	void RegisterView::OnToggleFilter(wxCommandEvent& event)
	{
	}

	//---------------------------------------------------------------------------

} // tools