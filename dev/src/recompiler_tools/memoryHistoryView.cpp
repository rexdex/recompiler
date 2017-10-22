#include "build.h"
#include "memoryHistoryView.h"
#include "project.h"
#include "../recompiler_core/decodingInstruction.h"
#include "../recompiler_core/decodingContext.h"

namespace tools
{

	BEGIN_EVENT_TABLE(MemoryHistoryView, wxPanel)
		EVT_LIST_ITEM_ACTIVATED(XRCID("DataList"), MemoryHistoryView::OnListItemActivated)
	END_EVENT_TABLE()

	MemoryHistoryView::MemoryHistoryView(wxWindow* parent, INavigationHelper* navigator)
		: m_list(nullptr)
		, m_navigator(navigator)
	{
		wxXmlResource::Get()->LoadPanel(this, parent, wxT("MemoryHistoryView"));

		m_list = XRCCTRL(*this, "DataList", wxListCtrl);
		m_list->AppendColumn("Order", wxLIST_FORMAT_LEFT, 100);
		m_list->AppendColumn("Trace", wxLIST_FORMAT_LEFT, 100);
		m_list->AppendColumn("Op", wxLIST_FORMAT_LEFT, 30);
		m_list->AppendColumn("Data", wxLIST_FORMAT_LEFT, 200);
		m_list->AppendColumn("Address", wxLIST_FORMAT_LEFT, 150);
		m_list->AppendColumn("Context", wxLIST_FORMAT_LEFT, 80);
		m_list->AppendColumn("Info", wxLIST_FORMAT_LEFT, 150);
		m_list->AppendColumn("Function", wxLIST_FORMAT_LEFT, 250);

		Layout();
		Show();
	}

	bool MemoryHistoryView::FillTable(Project* project, trace::DataFile& file, const std::vector<trace::MemoryAccessInfo>& history, const MemoryDisplayMode displayMode, const MemoryEndianess displayEndianess)
	{
		for (uint32_t i = 0; i < history.size(); ++i)
		{
			const auto& entry = history[i];

			// data entry
			wxString data;
			for (uint32 i = 0; i < entry.m_size; ++i)
			{
				if (entry.m_mask & (1ULL << i))
					data += wxString::Format("%02X ", entry.m_value[i]);
				else
					data += wxString::Format("-- ");
			}

			// create the entry
			Entry newEntry;
			newEntry.m_seq = entry.m_seq;
			newEntry.m_text[ColumnIndex_Trace] = wxString::Format("%u", entry.m_seq);
			newEntry.m_text[ColumnIndex_Op] = (entry.m_type == trace::MemoryAccessType::Write) ? "W" : "R";
			newEntry.m_text[ColumnIndex_Data] = data;

			// get the trace relative data
			const auto& frame = file.GetFrame(entry.m_seq);
			newEntry.m_text[ColumnIndex_Address] = wxString::Format("%08llXh", frame.GetAddress());

			// get context type
			const auto& context = file.GetContextList()[frame.GetLocationInfo().m_contextId];
			if (context.m_type == trace::ContextType::Thread)
				newEntry.m_text[ColumnIndex_Context] = wxString::Format("Thread%u", context.m_threadId);
			else if (context.m_type == trace::ContextType::IRQ)
				newEntry.m_text[ColumnIndex_Context] = wxString::Format("IRQ %u", context.m_id);
			else if (context.m_type == trace::ContextType::APC)
				newEntry.m_text[ColumnIndex_Context] = wxString::Format("APC %u", context.m_id);

			// get info
			auto* decodingContext = project->GetDecodingContext(frame.GetAddress());
			if (frame.GetType() == trace::FrameType::CpuInstruction)
			{
				if (decodingContext)
				{
					decoding::Instruction op;
					if (decodingContext->DecodeInstruction(wxTheApp->GetLogWindow(), frame.GetAddress(), op, false))
					{
						char instructionText[512];
						char* writeStream = instructionText;
						op.GenerateText(frame.GetAddress(), writeStream, instructionText + sizeof(instructionText));
						newEntry.m_text[ColumnIndex_Info] = instructionText;
					}
				}
			}
			else if (frame.GetType() == trace::FrameType::ExternalMemoryWrite)
			{
				const auto* readPtr = (const uint8_t*)frame.GetRawData();

				const auto stringLength = *readPtr++;
				std::string writeContextName;
				writeContextName.resize(stringLength);
				memcpy((char*)writeContextName.data(), readPtr, stringLength);

				newEntry.m_text[ColumnIndex_Info] = writeContextName;
			}

			// function name
			if (decodingContext)
			{
				std::string functionName;
				uint64 functionBase;

				if (decodingContext->GetFunctionName(frame.GetAddress(), functionName, functionBase) && !functionName.empty())
				{
					newEntry.m_text[ColumnIndex_Function] = functionName;
				}
			}

			m_entries.emplace_back(std::move(newEntry));
		}

		CreateListElements();
		return true;
	}

	void MemoryHistoryView::CreateListElements()
	{
		m_list->Freeze();
		m_list->DeleteAllItems();

		for (const auto& elem : m_entries)
		{
			// create item
			const auto index = m_list->GetItemCount();
			const auto name = wxString::Format("%u", index);
			auto id = m_list->InsertItem(index, name);
			m_list->SetItemData(id, elem.m_seq);

			// set additional data
			for (uint32 i = 1; i < ColumnIndex_MAX; ++i)
				m_list->SetItem(id, i, elem.m_text[i]);
		}

		m_list->Thaw();
		m_list->Refresh();
	}

	void MemoryHistoryView::OnListItemActivated(wxListEvent& evt)
	{
		const auto seq = evt.GetItem().GetData();
		m_navigator->NavigateToFrame(seq);
	}

} // tools
