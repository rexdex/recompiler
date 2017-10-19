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

	public:
		TraceInfoView(wxWindow* parent, trace::DataFile& traceData, Project* project);
		~TraceInfoView();

		void SetFrame(const TraceFrameID id, const trace::RegDisplayFormat format);

	private:
		void BuildCpuInstructionDoc(class HTMLBuilder& dic, const decoding::Instruction& op, const trace::DataFrame& frame, const trace::DataFrame& nextFrame, const trace::RegDisplayFormat format);
		void OnLinkClicked(wxHtmlLinkEvent& link);

		wxHtmlWindow*		m_infoView;

		Project* m_project;
		trace::DataFile* m_traceData;
	};

	//-----------------------------------------------------------------------------

} // tools