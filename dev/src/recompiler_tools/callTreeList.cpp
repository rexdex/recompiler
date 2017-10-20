#include "build.h"
#include "callTreeList.h"

namespace tools
{

	//---

	BEGIN_EVENT_TABLE(CallTreeList, wxcode::wxTreeListCtrl)
	END_EVENT_TABLE()

	//---

	CallTreeList::CallTreeList(wxWindow* parent, INavigationHelper* navigator)
		: wxcode::wxTreeListCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_HAS_BUTTONS | wxTR_FULL_ROW_HIGHLIGHT | wxTR_ROW_LINES | wxTR_HIDE_ROOT)
		, m_isSelfNavigating(false)
		, m_navigator(navigator)
	{
		AddColumn("Name", 250);
		AddColumn("Entry Addr", 200);
		AddColumn("Return Addr", 200);

		Bind(wxEVT_TREE_ITEM_ACTIVATED, [this](const wxTreeEvent& evt) { OnItemActivated(evt); });
	}

	static const wxString GetContextName(const trace::Context& c)
	{
		if (c.m_type == trace::ContextType::Thread)
		{
			return wxString::Format("Thread%u (#%u)", c.m_threadId, c.m_id);
		}
		else if (c.m_type == trace::ContextType::IRQ)
		{
			return wxString::Format("IRQ (#%u)", c.m_id);
		}
		else if (c.m_type == trace::ContextType::APC)
		{
			return wxString::Format("APC (#%u)", c.m_id);
		}
		else
		{
			return "Unknown";
		}
	}

	void CallTreeList::ShowCallstacks(const trace::DataFile& data, const TraceFrameID seq)
	{
		// prevent resync if we initialized the navigation (hack)
		if (m_isSelfNavigating)
			return;

		// clear the
		Freeze();
		DeleteRoot();
		AddRoot("");

		// process contexts
		uint32_t numContexts = data.GetContextList().size();
		for (uint32_t i = 0; i < numContexts; ++i)
		{
			// get group that matches the context type
			const auto& context = data.GetContextList()[i];
			if (context.m_rootCallFrame != 0)
			{
				if (seq >= context.m_first.m_seq && seq <= context.m_last.m_seq)
				{
					// create the entry for the call context itself
					const auto name = GetContextName(context);
					wxTreeItemId itemId = AppendItem(GetRootItem(), name);
					SetItemText(itemId, 1, wxString::Format("%08llXh", context.m_first.m_ip));
					SetItemText(itemId, 2, wxString::Format("%08llXh", context.m_last.m_ip));
					SetItemData(itemId, new ItemData(context.m_first.m_seq, context.m_last.m_seq));

					// create the call frames
					CreateFrameItems(itemId, seq, context.m_rootCallFrame, data);

					// make sure the element is expanded
					Expand(itemId);
				}
			}
		}

		// redraw
		Thaw();
		Refresh();
	}

	void CallTreeList::CreateFrameItems(wxTreeItemId parentItemId, const TraceFrameID seq, uint32 callFrameIndex, const trace::DataFile& data)
	{
		const auto& callFrame = data.GetCallFrames()[callFrameIndex];
		auto childIndex = callFrame.m_firstChildFrame;
		while (childIndex)
		{
			const auto& childCallFrame = data.GetCallFrames()[childIndex];
			if (seq >= childCallFrame.m_enterLocation.m_seq && seq <= childCallFrame.m_leaveLocation.m_seq)
			{
				// TODO: exceptions ?
				wxTreeItemId itemId = AppendItem(parentItemId, "Function Call");
				SetItemText(itemId, 1, wxString::Format("%08llXh", childCallFrame.m_enterLocation.m_ip));
				SetItemText(itemId, 2, wxString::Format("%08llXh", childCallFrame.m_leaveLocation.m_ip));
				SetItemData(itemId, new ItemData(childCallFrame.m_enterLocation.m_seq, childCallFrame.m_leaveLocation.m_seq));

				// recurse
				// NOTE: we readd to thread so the tree is not to deep
				CreateFrameItems(parentItemId, seq, childIndex, data);
			}

			// go to next child
			childIndex = childCallFrame.m_nextChildFrame;
		}		
	}

	void CallTreeList::OnItemActivated(const wxTreeEvent& evt)
	{
		auto id = evt.GetItem();
		if (id.IsOk())
		{
			auto* data = (ItemData*)GetItemData(id);
			if (data)
			{
				m_isSelfNavigating = true;
				const bool selectionMode = wxGetKeyState(WXK_SHIFT);
				if (selectionMode)
					m_navigator->NavigateToFrame(data->m_leave);
				else
					m_navigator->NavigateToFrame(data->m_enter);
				m_isSelfNavigating = false;
			}
		}
	}

} // tools