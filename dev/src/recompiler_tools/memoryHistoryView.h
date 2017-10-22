#pragma once

#include "../recompiler_core/traceDataFile.h"
#include "../recompiler_core/traceUtils.h"
#include "memoryTraceView.h"

//---------------------------------------------------------------------------

namespace tools
{

	/// memory history view
	class MemoryHistoryView : public wxPanel
	{
		DECLARE_EVENT_TABLE();

	public:
		MemoryHistoryView(wxWindow* parent, INavigationHelper* navigator);

		// fill table with register values for 
		bool FillTable(Project* project, trace::DataFile& file, const std::vector<trace::MemoryAccessInfo>& history, const MemoryDisplayMode displayMode, const MemoryEndianess displayEndianess);

	private:
		wxListCtrl*	m_list;
		INavigationHelper* m_navigator;

		enum ColumnIndex
		{
			ColumnIndex_Trace=1,
			ColumnIndex_Op,
			ColumnIndex_Data,
			ColumnIndex_Address,
			ColumnIndex_Context,
			ColumnIndex_Info,
			ColumnIndex_Function,

			ColumnIndex_MAX,
		};

		struct Entry
		{
			TraceFrameID m_seq;
			wxString m_text[ColumnIndex_MAX];
		};

		std::vector<Entry> m_entries;

		void CreateListElements();
		void OnListItemActivated(wxListEvent& evt);
	};

} // tools

  //---------------------------------------------------------------------------
