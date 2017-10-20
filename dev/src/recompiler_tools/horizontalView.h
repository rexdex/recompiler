#pragma once

#include "../recompiler_core/traceDataFile.h"
#include "../recompiler_core/traceUtils.h"

//---------------------------------------------------------------------------

namespace tools
{

	/// horizontal view of instruction in various moments
	class HorizontalRegisterView : public wxPanel
	{
		DECLARE_EVENT_TABLE();

	public:
		HorizontalRegisterView(wxWindow* parent, INavigationHelper* navigator);

		// fill table with register values for 
		bool FillTable(Project* project, trace::DataFile& file, const TraceFrameID seq, std::vector<TraceFrameID>& otherSeq);

		// update display text, based on the display mode
		bool UpdateValueTexts(const trace::RegDisplayFormat displayFormat);

	private:
		wxListCtrl*	m_list;
		INavigationHelper* m_navigator;

		struct RegValue
		{
			const platform::CPURegister* m_reg;
			uint8_t m_data[64];
			uint32_t m_item;
			uint32_t m_column;
		};

		std::vector<RegValue> m_values;

		void OnListItemActivated(wxListEvent& evt);
	};

} // tools

  //---------------------------------------------------------------------------
