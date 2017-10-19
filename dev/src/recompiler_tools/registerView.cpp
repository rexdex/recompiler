#include "build.h"
#include "registerView.h"
#include "widgetHelpers.h"
#include "../recompiler_core/platformCPU.h"

namespace tools
{

	//---------------------------------------------------------------------------

	BEGIN_EVENT_TABLE(RegisterView, wxPanel)
	END_EVENT_TABLE()

	RegisterView::RegisterView(wxWindow* parent)
		: m_list(nullptr)
	{
		wxXmlResource::Get()->LoadPanel(this, parent, wxT("Registers"));

		{
			auto* panel = XRCCTRL(*this, "RegListPanel", wxPanel);
			m_list = new wxcode::wxTreeListCtrl(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_HAS_BUTTONS | wxTR_FULL_ROW_HIGHLIGHT | wxTR_ROW_LINES | wxTR_HIDE_ROOT);
			panel->SetSizer(new wxBoxSizer(wxVERTICAL));
			panel->GetSizer()->Add(m_list, 1, wxEXPAND, 0);

			wxFont drawFont = m_list->GetFont();
			drawFont.SetPixelSize(wxSize(0,14));
			m_list->SetFont(drawFont);
		}

		m_list->AddColumn(wxT("Name"), 150, wxALIGN_LEFT);
		m_list->AddColumn(wxT("Current value"), 230, wxALIGN_LEFT);
		m_list->AddColumn(wxT("New value"), 230, wxALIGN_LEFT);
		m_list->AddColumn(wxT("Size"), 50, wxALIGN_LEFT);

		Layout();
		Show();
	}

	void RegisterView::InitializeRegisters(const platform::CPU& cpuInfo, const platform::CPURegisterType regType)
	{
		UpdateRegisterList(cpuInfo, regType);
	}

	void RegisterView::UpdateRegisters(const trace::DataFrame& frame, const trace::DataFrame& nextFrame, const trace::RegDisplayFormat format)
	{
		UpdateRegisterValues(frame, nextFrame, format);
	}

	void RegisterView::CreateRegisterInfo(wxTreeItemId parentItem, const platform::CPURegister* rootReg, const platform::CPURegister* reg)
	{
		// create the register item
		auto item = m_list->AppendItem(parentItem, reg->GetName());
		m_list->SetItemText(item, 1, "-");
		m_list->SetItemText(item, 2, "-");
		m_list->SetItemText(item, 3, wxString::Format("%u:%u", reg->GetBitOffset(), reg->GetBitSize()));

		// add to list
		RegInfo info;
		info.m_item = item;
		info.m_reg = reg;
		info.m_rootReg = rootReg;
		m_registers.push_back(info);

		// create child registers
		const auto numChildren = reg->GetNumChildRegisters();
		for (uint32_t i = 0; i < numChildren; ++i)
		{
			const auto* childReg = reg->GetChildRegister(i);
			CreateRegisterInfo(item, rootReg, childReg);
		}
	}

	void RegisterView::UpdateRegisterList(const platform::CPU& cpuInfo, const platform::CPURegisterType regType)
	{
		// reset
		m_list->Freeze();
		m_list->DeleteRoot();

		// create root
		auto root = m_list->AddRoot("Registers");

		// prepare filtering list
		const uint32 numRegs = cpuInfo.GetNumRootRegisters();
		for (uint32 i = 0; i < numRegs; ++i)
		{
			const auto* reg = cpuInfo.GetRootRegister(i);
			if (regType == platform::CPURegisterType::ANY || reg->GetType() == regType)
			{
				CreateRegisterInfo(m_list->GetRootItem(), reg, reg);
			}
		}

		// end update
		m_list->Thaw();
		m_list->Refresh();
	}
	
	void RegisterView::UpdateRegisterValues(const trace::DataFrame& frame, const trace::DataFrame& nextFrame, const trace::RegDisplayFormat format)
	{
		// start update
		m_list->Freeze();

		// update the values according to trace results
		const auto defaultColor = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
		for (auto& info : m_registers)
		{
			const platform::CPURegister* reg = info.m_reg;

			// update value
			const auto curVal = trace::GetRegisterValueText(reg, frame, format);
			const auto nextVal = trace::GetRegisterValueText(reg, nextFrame, format);

			if (curVal != info.m_curValue)
			{
				info.m_curValue = curVal;
				m_list->SetItemText(info.m_item, 1, curVal.c_str());
			}

			if (nextVal != info.m_nextValue)
			{
				info.m_nextValue = nextVal;
				m_list->SetItemText(info.m_item, 2, nextVal.c_str());
			}

			const bool isModified = (curVal != nextVal) && (curVal != "-") && (nextVal != "-");
			if (isModified != info.m_wasModified)
			{
				const wxColour drawColor = isModified ? wxColour(255, 50, 50) : defaultColor;
				m_list->SetItemBackgroundColour(info.m_item, drawColor);
			//m_list->SetItemBold(info.m_item, isModified);
				info.m_wasModified = isModified;
			}
		}

		// end update
		m_list->Thaw();
		m_list->Refresh(false);
	}

	//---------------------------------------------------------------------------

} // tools