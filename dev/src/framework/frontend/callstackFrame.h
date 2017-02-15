#pragma once

#include "events.h"

class CCallstackFrame : public wxFrame, public IEventListener
{
	DECLARE_EVENT_TABLE();

public:
	CCallstackFrame( wxWindow* parent );
	~CCallstackFrame();

private:
	void Refresh();	
	void BuildNormalTree();

	class CCallstackFunctionTree*		m_tree;

	void OnRefreshData( wxCommandEvent& event );
	void OnToggleFunctionNames( wxCommandEvent& event );
	void OnExportData( wxCommandEvent& event );
	void OnClose( wxCloseEvent& event );

	virtual void OnTraceOpened();
	virtual void OnTraceClosed();
	virtual void OnTraceIndexChanged( const uint32 currentEntry );
};
