#pragma once
#include "../recompiler_core/traceUtils.h"

namespace decoding
{
	class Instruction;
}

namespace tools
{

	//-----------------------------------------------------------------------------

	/// Displays trace info
	class TraceInfoView : public wxPanel
	{
		DECLARE_EVENT_TABLE();

	private:
		wxHtmlWindow*		m_infoView;

	public:
		TraceInfoView(wxWindow* parent);
		~TraceInfoView();

		// update information about current trace location
		void UpdateInfo(const class ProjectTraceData* data);

	private:
		void BuildDoc(class HTMLBuilder& dic, const class ProjectTraceData* data, const decoding::Instruction& op, const trace::DataFrame& frame, const trace::DataFrame& nextFrame);

		void OnLinkClicked(wxHtmlLinkEvent& link);

		trace::RegDisplayFormat		m_displayFormat;
	};

	//-----------------------------------------------------------------------------

} // tools