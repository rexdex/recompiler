#include "build.h"
#include "progressDialog.h"
#include "callstackFrame.h"
#include "callstackFunctionTree.h"
#include "project.h"

#include "../recompiler_core/traceCalltree.h"
#include "../recompiler_core/decodingNameMap.h"
#include "../recompiler_core/decodingEnvironment.h"
#include "../recompiler_core/decodingContext.h"

namespace tools
{

	BEGIN_EVENT_TABLE(CallstackFrame, wxFrame)
		EVT_TOOL(XRCID("refreshCallstack"), CallstackFrame::OnRefreshData)
		EVT_TOOL(XRCID("exportCallstack"), CallstackFrame::OnExportData)
		EVT_TOOL(XRCID("showFunctionNames"), CallstackFrame::OnToggleFunctionNames)
		EVT_CLOSE(CallstackFrame::OnClose)
	END_EVENT_TABLE();

	CallstackFrame::CallstackFrame(wxWindow* parent)
		: m_tree(nullptr)
	{
		wxXmlResource::Get()->LoadFrame(this, parent, wxT("Callstack"));

		// create tree view
		{
			wxPanel* panel = XRCCTRL(*this, "TreeTab", wxPanel);
			m_tree = new CallstackFunctionTree(panel);
			wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
			sizer->Add(m_tree, 1, wxALL | wxEXPAND, 0);
			panel->SetSizer(sizer);
			panel->Layout();
			m_tree->Enable(false);
		}

		// rebuild views
		BuildNormalTree();

		// refresh layout
		Layout();
		Show();
	}

	void CallstackFrame::Refresh()
	{
/*		// no project
		if (!wxTheFrame->GetProject())
		{
			wxMessageBox(wxT("No loaded project, unable to build callstack data"), wxT("Error"), wxICON_ERROR | wxOK, this);
			return;
		}

		// no trace data
		if (!wxTheFrame->GetProject()->GetTrace())
		{
			wxMessageBox(wxT("No loaded trace data, unable to build callstack data"), wxT("Error"), wxICON_ERROR | wxOK, this);
			return;
		}

		// we alredy have a callstack
		if (wxTheFrame->GetProject()->GetTrace()->GetCallstack())
		{
			if (wxNO == wxMessageBox(wxT("Callstack data already extracted, recompute ?"), wxT("Data override"), wxICON_QUESTION | wxYES_NO, this))
				return;
		}

		// update data
		if (!wxTheFrame->GetProject()->GetTrace()->BuildCallstack(wxTheApp->GetLogWindow()))
		{
			wxMessageBox(wxT("Failed to build callstack data"), wxT("Error"), wxICON_ERROR | wxOK, this);
		}*/

		BuildNormalTree();
	}

	void CallstackFrame::BuildNormalTree()
	{
		const bool showFunctionNames = XRCCTRL(*this, "Toolbar", wxToolBar)->GetToolState(XRCID("showFunctionNames"));
		const uint32 maxDepth = 100;

		/*if (wxTheFrame->GetProject()->GetTrace())
		{
			m_tree->Rebuild(wxTheFrame->GetProject()->GetTrace()->GetCallstack(), maxDepth, showFunctionNames);
		}*/
	}

	void CallstackFrame::OnRefreshData(wxCommandEvent& event)
	{
		Refresh();
	}

	void CallstackFrame::OnToggleFunctionNames(wxCommandEvent& event)
	{
		BuildNormalTree();
	}

	void CallstackFrame::OnExportData(wxCommandEvent& event)
	{
	}

	void CallstackFrame::OnClose(wxCloseEvent& event)
	{
		event.Veto();
		Hide();
	}

	/*
	void CallstackFrame::OnTraceOpened()
	{
		BuildNormalTree();
	}

	void CallstackFrame::OnTraceClosed()
	{
		BuildNormalTree();
	}

	void CallstackFrame::OnTraceIndexChanged(const uint32 currentEntry)
	{
		m_tree->Highlight(currentEntry);
	}*/

} // tools