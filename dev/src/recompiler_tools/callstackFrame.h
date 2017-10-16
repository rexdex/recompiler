#pragma once

#include "eventListener.h"

namespace tools
{
	class CallstackFrame : public wxFrame, public IEventListener
	{
		DECLARE_EVENT_TABLE();

	public:
		CallstackFrame(wxWindow* parent);

	private:
		void Refresh();
		void BuildNormalTree();

		class CallstackFunctionTree*		m_tree;

		void OnRefreshData(wxCommandEvent& event);
		void OnToggleFunctionNames(wxCommandEvent& event);
		void OnExportData(wxCommandEvent& event);
		void OnClose(wxCloseEvent& event);
	};

} // tools