#pragma once

#include "canvas.h"
#include "../recompiler_core/traceDataFile.h"
#include "../xenon_launcher/xenonUtils.h"
#include "treelistctrl.h"

namespace tools
{
	/// tree view for the call stacks at given sequence point
	class CallTreeList : public wxcode::wxTreeListCtrl
	{
		DECLARE_EVENT_TABLE();

	public:
		CallTreeList(wxWindow* parent, INavigationHelper* navigator);
		
		/// show call stack for given sequence points
		void ShowCallstacks(const trace::DataFile& data, const TraceFrameID seq);

	private:
		bool m_isSelfNavigating;
		INavigationHelper* m_navigator;

		struct ItemData : public wxTreeItemData
		{
			TraceFrameID m_enter;
			TraceFrameID m_leave;

			inline ItemData(const TraceFrameID enter, const TraceFrameID leave)
				: m_enter(enter)
				, m_leave(leave)
			{}
		};
		
		void CreateFrameItems(wxTreeItemId itemId, const TraceFrameID seq, uint32 callFrameIndex, const trace::DataFile& data);

		void OnItemActivated(const wxTreeEvent& evt);
	};


} // tools