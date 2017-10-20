#include "build.h"
#include "horizontalView.h"
#include "widgetHelpers.h"
#include "../recompiler_core/platformCPU.h"
#include "../recompiler_core/decodingInstructionInfo.h"
#include "project.h"
#include "../recompiler_core/decodingInstruction.h"
#include "../recompiler_core/decodingContext.h"

namespace tools
{

	//---------------------------------------------------------------------------

	BEGIN_EVENT_TABLE(HorizontalRegisterView, wxPanel)
		EVT_LIST_ITEM_ACTIVATED(XRCID("DataList"), HorizontalRegisterView::OnListItemActivated)
	END_EVENT_TABLE()

	HorizontalRegisterView::HorizontalRegisterView(wxWindow* parent, INavigationHelper* navigator)
		: m_list(nullptr)
		, m_navigator(navigator)
	{
		wxXmlResource::Get()->LoadPanel(this, parent, wxT("HorizontalView"));

		m_list = XRCCTRL(*this, "DataList", wxListCtrl);

		Layout();
		Show();
	}

	bool HorizontalRegisterView::UpdateValueTexts(const trace::RegDisplayFormat displayFormat)
	{
		m_list->Freeze();

		// update data
		for (const auto& val : m_values)
		{
			const auto txt = trace::GetRegisterValueText(val.m_reg, val.m_data, displayFormat);
			m_list->SetItem(val.m_item, val.m_column, txt);
		}

		m_list->Thaw();
		m_list->Refresh();
		return true;
	}

	bool HorizontalRegisterView::FillTable(Project* project, trace::DataFile& file, const TraceFrameID baseSeq, std::vector<TraceFrameID>& seqList)
	{
		// get trace frame
		const auto& frame = file.GetFrame(baseSeq);
		if (frame.GetType() != trace::FrameType::CpuInstruction)
			return false;

		// decode instruction
		const auto address = frame.GetAddress();

		// get instruction decoder
		decoding::Instruction op;
		const auto context = project->GetDecodingContext(address);
		if (!context)
			return false;

		// decode the instruction
		if (0 == context->DecodeInstruction(wxTheApp->GetLogWindow(), address, op, false))
			return false;

		// get instruction information
		decoding::InstructionExtendedInfo info;
		if (!op.GetExtendedInfo(frame.GetAddress(), *context, info))
			return false;

		// list dependencies as "Ins"
		std::vector<const platform::CPURegister*> inRegisters;
		for (uint32 i = 0; i < info.m_registersDependenciesCount; ++i)
			inRegisters.push_back(info.m_registersDependencies[i]);

		// and modified registers as "outs"
		std::vector<const platform::CPURegister*> outRegisters;
		for (uint32 i = 0; i < info.m_registersModifiedCount; ++i)
			outRegisters.push_back(info.m_registersModified[i]);

		// clear the list
		m_list->Freeze();
		m_list->DeleteAllColumns();
		m_list->AppendColumn("Order", wxLIST_FORMAT_LEFT, 80);
		m_list->AppendColumn("Trace", wxLIST_FORMAT_LEFT, 100);

		// columns for in-registers
		for (const auto* reg : inRegisters)
		{
			const auto name = wxString("In ") + reg->GetName();
			m_list->AppendColumn(name, wxLIST_FORMAT_LEFT, 200);
		}

		// columns for out-registers
		for (const auto* reg : outRegisters)
		{
			const auto name = wxString("Out ") + reg->GetName();
			m_list->AppendColumn(name, wxLIST_FORMAT_LEFT, 200);
		}

		// gather data
		uint32 orderIndex = 0;
		for (const auto seq : seqList)
		{
			// get the frames
			auto& curFrame = file.GetFrame(seq);
			auto& nextFrame = file.GetFrame(file.GetNextInContextFrame(seq));

			// create the entry
			auto title = wxString::Format("%5llu", orderIndex++);
			if (seq == baseSeq)
				title += "*"; // just mark it somehow
			auto itemId = m_list->InsertItem(m_list->GetItemCount(), title);
			m_list->SetItemData(itemId, seq);
			m_list->SetItem(itemId, 1, wxString::Format("%llu", seq));

			// create the entries with the values
			// the IN registers use the current frame value
			uint32_t columnIndex = 2;
			for (const auto* reg : inRegisters)
			{
				RegValue value;
				value.m_reg = reg;
				value.m_item = itemId;
				value.m_column = columnIndex;
				if (curFrame.GetRegData(reg, sizeof(value.m_data), &value.m_data))
					m_values.push_back(value);
				columnIndex++;
			}

			// out registers
			for (const auto* reg : outRegisters)
			{
				RegValue value;
				value.m_reg = reg;
				value.m_item = itemId;
				value.m_column = columnIndex;
				if (nextFrame.GetRegData(reg, sizeof(value.m_data), &value.m_data))
					m_values.push_back(value);
				columnIndex++;
			}
		}

		// update display text
		m_list->Thaw();
		m_list->Refresh();

		return true;
	}

	void HorizontalRegisterView::OnListItemActivated(wxListEvent& evt)
	{
		const auto seq = evt.GetItem().GetData();
		m_navigator->NavigateToFrame(seq);
	}

} // tools