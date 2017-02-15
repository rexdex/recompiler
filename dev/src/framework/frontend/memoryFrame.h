#pragma once

struct ProjectTraceMemoryAccessEntry;

/// Frame with the stuff related to memory trace
class CMemoryFrame : public wxFrame
{
	DECLARE_EVENT_TABLE();

public:
	CMemoryFrame( wxWindow* parent );
	virtual ~CMemoryFrame();

	void ShowAddressHistory( const uint32 address, const uint32 size, const uint32 referenceTraceIndex=0 );

private:
	struct HistoryData
	{
		uint32												m_address;
		uint32												m_size;
		std::vector< ProjectTraceMemoryAccessEntry >		m_history;
	};

	std::map< uint64, HistoryData* >	m_cache;
	wxComboBox*							m_singleAddress;
	wxChoice*							m_singleAddressSize;
	wxHtmlWindow*						m_singleAddressHistory;

	const HistoryData* GetAddressHistory( const uint32 address, const uint32 size );

	void OnShowSingleAddress( wxCommandEvent& event );
	void OnRefreshData( wxCommandEvent& event );
	void OnClose( wxCloseEvent& event );

	// general HTML handler
	void OnLinkClicked(wxHtmlLinkEvent& link);
};
