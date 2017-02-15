#pragma once
#include "..\backend\traceUtils.h"

namespace decoding
{
	class Instruction;
}

//-----------------------------------------------------------------------------

/// Displays trace info
class CTraceInfoView : public wxPanel
{
	DECLARE_EVENT_TABLE();

private:
	wxHtmlWindow*		m_infoView;

public:
	CTraceInfoView(wxWindow* parent);
	~CTraceInfoView();

	// update information about current trace location
	void UpdateInfo(const class ProjectTraceData* data);

private:
	void BuildDoc(class CHTMLBuilder& dic, const class ProjectTraceData* data, const decoding::Instruction& op, const trace::DataFrame& frame, const trace::DataFrame& nextFrame);

	void OnLinkClicked(wxHtmlLinkEvent& link);

	trace::RegDisplayFormat		m_displayFormat;
};

//-----------------------------------------------------------------------------
