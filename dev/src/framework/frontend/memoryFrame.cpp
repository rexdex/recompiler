#include "build.h"
#include "progressDialog.h"
#include "memoryFrame.h"
#include "utils.h"
#include "project.h"
#include "frame.h"

BEGIN_EVENT_TABLE( CMemoryFrame, wxFrame )
	EVT_TOOL( XRCID("refreshMemory"), CMemoryFrame::OnRefreshData )
	EVT_COMBOBOX( XRCID("singleAddress"), CMemoryFrame::OnShowSingleAddress )
	EVT_TEXT_ENTER( XRCID("singleAddress"), CMemoryFrame::OnShowSingleAddress )
	EVT_HTML_LINK_CLICKED( XRCID("singleAddressHistory"), CMemoryFrame::OnLinkClicked )
	EVT_CLOSE( CMemoryFrame::OnClose )
END_EVENT_TABLE(); 

CMemoryFrame::CMemoryFrame( wxWindow* parent )
	: m_singleAddress( nullptr )
	, m_singleAddressHistory( nullptr )
{
	wxXmlResource::Get()->LoadFrame( this, parent, wxT("Memory") );

	// extract stuff
	m_singleAddress = XRCCTRL( *this, "singleAddress", wxComboBox );
	m_singleAddressHistory = XRCCTRL( *this, "singleAddressHistory", wxHtmlWindow );
	m_singleAddressSize = XRCCTRL( *this, "singleAddressSize", wxChoice );

	// refresh layout
	Layout();
	Show();
}

CMemoryFrame::~CMemoryFrame()
{
}

void CMemoryFrame::ShowAddressHistory( const uint32 address, const uint32 size, const uint32 referenceTraceIndex/*=0*/ )
{
	// build HTML document
	wxString htmlText;
	CHTMLBuilder doc( htmlText );
	doc.Open("html");
	doc.Open("body");

	// get the project
	const HistoryData* data = GetAddressHistory( address, size );
	if ( data )
	{
		doc.PrintBlock( "h1", "Memory history for %08Xh (+%d)", address, size );

		if ( data->m_history.empty() )
		{
			doc.Print( "No history for given address" );
		}
		else
		{
			doc.Open( "table" );

			// reference trace index
			uint32 traceReference = referenceTraceIndex;
			if ( !traceReference )
			{
				Project* project = wxTheFrame->GetProject();
				ProjectTraceData* trace = project ? project->GetTrace() : nullptr;
				if ( trace )
				{
					traceReference = trace->GetCurrentEntryIndex();
				}
			}

			// table header
			{
				doc.Open( "tr" );
				doc.TableColumn( "Mode", 50 );
				doc.TableColumn( "Code", 100 );
				doc.TableColumn( "Trace", 100 );
				doc.TableColumn( "Value", 200 );
				doc.TableColumn( "Function", 400 );
				doc.Close();
			}

			// table data
			for ( uint32 i=0; i<data->m_history.size(); ++i )
			{
				const ProjectTraceMemoryAccessEntry& entry = data->m_history[i];

				doc.Open( "tr" );							

				// color
				if ( entry.m_type == 1 )
				{
					doc.Attr( "bgcolor", "#DDFFDD" );
				}
				else if ( entry.m_type == 2 ) 
				{
					doc.Attr( "bgcolor", "#FFDDDD" );
				}
				else
				{
					doc.Attr( "bgcolor", "#FFDDFF" );
				}

				// highlight the trace
				bool active = false;
				if ( referenceTraceIndex && entry.m_trace == traceReference )
				{
					active = true;
					doc.Print( "<b>" );
				}

				doc.PrintBlock( "td", entry.m_type==1 ? "READ" : "WRITE" );
				doc.PrintBlock( "td", "<a href=\"addr:%08Xh\">%08Xh</a>", entry.m_address, entry.m_address );
				doc.PrintBlock( "td", "<a href=\"entry:%d\">#%d</a>", entry.m_trace, entry.m_trace );

				if ( entry.m_size == 1 )
				{
					doc.PrintBlock( "td", "%d (%02X)", entry.m_data[0], entry.m_data[0] );
				}
				else if ( entry.m_size == 2 )
				{
					doc.PrintBlock( "td", "%d (%02X %02X)", *(short*)&entry.m_data[0], entry.m_data[0], entry.m_data[1] );
				}
				else if ( entry.m_size == 4 )
				{
					doc.PrintBlock( "td", "%d (%02X %02X %02X %02X)", *(int*)&entry.m_data[0], entry.m_data[0], entry.m_data[1], entry.m_data[2], entry.m_data[3] );
				}
				else if ( entry.m_size == 8 )
				{
					doc.PrintBlock( "td", "%d (%016llX)", *(__int64*)&entry.m_data[0], *(__int64*)&entry.m_data[0] );
				}
				doc.PrintBlock( "td", "" );

				if ( active )
				{
					doc.Print( "</b>" );
				}

				doc.Close();
			}

			doc.Close();
		}
	}
	else
	{
		doc.PrintBlock( "h1", "No trace data" );
	}

	// update text
	doc.Close();
	doc.Close();
	m_singleAddressHistory->SetPage( htmlText );
}

void CMemoryFrame::OnShowSingleAddress( wxCommandEvent& event )
{
	// get combo text
	const std::string comboText = m_singleAddress->GetValue();
	if ( !comboText.empty() )
	{
		// parse address
		uint32 memoryAddress = 0;
		const char* stream = comboText.c_str();
		if ( Parse::ParseUrlAddress( stream, memoryAddress ) )
		{
			// get memory size
			uint32 memorySize = 4;
			switch ( m_singleAddressSize->GetSelection() )
			{
				case 0: memorySize = 1; break;
				case 1: memorySize = 2; break;
				case 2: memorySize = 4; break;
				case 3: memorySize = 8; break;
				case 4: memorySize = 16; break;
			}

			// show address history
			ShowAddressHistory( memoryAddress, memorySize, 0 );
		}
		else
		{
			wxMessageBox( wxT("Invalid address"), wxT("Error"), wxICON_ERROR | wxOK, this );
		}
	}
}

const CMemoryFrame::HistoryData* CMemoryFrame::GetAddressHistory( const uint32 address, const uint32 size )
{
	// find in map
	const uint64 key = (uint64)((uint64)address << 4) + size;
	const auto it = m_cache.find( key );
	if ( it != m_cache.end() )
	{
		assert( (*it).second->m_address == address );
		assert( (*it).second->m_size == size );
		return (*it).second;
	}

	// build address history
	Project* project = wxTheFrame->GetProject();
	ProjectTraceData* trace = project ? project->GetTrace() : nullptr;
	if ( trace )
	{
		HistoryData* history = new HistoryData;
		history->m_address = address;
		history->m_size = size;
		if ( trace->FindAllMemoryAccess( address, size, 3, history->m_history ) )
		{
			// add to cache
			m_cache[key] = history;
			return history;
		}
		else
		{
			delete history;
		}
	}

	// not aviable
	return nullptr;
}

void CMemoryFrame::OnRefreshData( wxCommandEvent& event )
{
}

void CMemoryFrame::OnClose( wxCloseEvent& event )
{
	event.Veto();
	Hide();
}

void CMemoryFrame::OnLinkClicked(wxHtmlLinkEvent& link)
{
	const wxString href = link.GetLinkInfo().GetHref();
	wxTheFrame->NavigateUrl(href);
}
