#include "build.h"
#include "frame.h"
#include "nameUndecorator.h"
#include "memoryView.h"
#include "logWindow.h"
#include "project.h"
#include "progressDialog.h"
#include "gotoAddressDialog.h"
#include "projectMemoryView.h"
#include "registerView.h"
#include "findSymbolDialog.h"
#include "memoryFrame.h"
#include "utils.h"
#include "../backend/decodingEnvironment.h"
#include "../backend/decodingContext.h"
#include "../backend/decodingAddressMap.h"
#include "../backend/image.h"

void CMainFrame::ProcessPendingNavigation()
{
	if (!m_pendingNavigation.empty())
	{
		const wxString url = m_pendingNavigation;
		m_pendingNavigation = "";

		NavigateUrl(url, false);
	}
}

bool CMainFrame::NavigateUrl( const wxString& url, const bool deferred/*=true*/ )
{
	if (deferred)
	{
		m_pendingNavigation = url;
		return true;
	}

	char urlText[ 512 ];
	strcpy_s(urlText, 512, url.c_str().AsChar());

	// parse text
	const char* txt = urlText;
	if (Parse::ParseUrlKeyword(txt, "entry:"))
	{
		int index = 0;
		if (Parse::ParseUrlInteger(txt, index))
		{
			return NavigateToTraceEntry(index);
		}
	}
	else if (Parse::ParseUrlKeyword(txt, "addr:"))
	{
		uint32 address = 0;
		if (Parse::ParseUrlAddress(txt, address))
		{
			return NavigateToAddress(address);
		}
	}
	else if (Parse::ParseUrlKeyword(txt, "endtrace"))
	{
		if (m_project && m_project->GetTrace())
		{
			if (m_project->GetTrace()->MoveToEntry( m_project->GetTrace()->GetNumEntries()-1 ) )
			{
				RefreshTrace();
				return true;
			}
		}
	}
	else if (Parse::ParseUrlKeyword(txt, "resettrace"))
	{
		if (m_project && m_project->GetTrace())
		{
			if (m_project->GetTrace()->MoveToEntry( 0 ) )
			{
				RefreshTrace();
				return true;
			}
		}
	}
	else if (Parse::ParseUrlKeyword(txt, "runtrace"))
	{
		if (m_project && m_project->GetTrace())
		{
			int newEntry = m_project->GetTrace()->GetNextBreakEntry();

			// no breakpoint, move to the end
			if (newEntry == -1)
			{
				newEntry = m_project->GetTrace()->GetNumEntries() - 1;
			}

			if (newEntry != -1)
			{
				if (m_project->GetTrace()->MoveToEntry(newEntry))
				{
					RefreshTrace();
					return true;
				}
			}
		}
	}
	else if (Parse::ParseUrlKeyword(txt, "stepinto"))
	{
		if (m_project && m_project->GetTrace())
		{
			int newEntry = m_project->GetTrace()->GetStepIntoEntry();
			if (newEntry != -1)
			{
				if (m_project->GetTrace()->MoveToEntry(newEntry))
				{
					RefreshTrace();
					return true;
				}
			}
		}
	}
	else if (Parse::ParseUrlKeyword(txt, "stepback"))
	{
		if (m_project && m_project->GetTrace())
		{
			int newEntry = m_project->GetTrace()->GetStepBackEntry();
			if (newEntry != -1)
			{
				if (m_project->GetTrace()->MoveToEntry(newEntry))
				{
					RefreshTrace();
					return true;
				}
			}
		}
	}
	else if (Parse::ParseUrlKeyword(txt, "stepout"))
	{
		if (m_project && m_project->GetTrace())
		{
			int newEntry = m_project->GetTrace()->GetStepOutEntry();
			if (newEntry != -1)
			{
				if (m_project->GetTrace()->MoveToEntry(newEntry))
				{
					RefreshTrace();
					return true;
				}
			}
		}
	}
	else if (Parse::ParseUrlKeyword(txt, "stepover"))
	{
		if (m_project && m_project->GetTrace())
		{
			int newEntry = m_project->GetTrace()->GetStepOverEntry();
			if (newEntry != -1)
			{
				if (m_project->GetTrace()->MoveToEntry(newEntry))
				{
					RefreshTrace();
					return true;
				}
			}
		}
	}
	else if (Parse::ParseUrlKeyword(txt, "back"))
	{
		// navigate back
		if (m_project && m_project->GetAddressHistory().CanNavigateBack())
		{
			const uint32 backAddress = m_project->GetAddressHistory().NavigateBack();
			return NavigateToAddress(backAddress, false);
		}
	}
	else if (Parse::ParseUrlKeyword(txt, "reference:"))
	{
		// decode address
		uint32 address = 0;
		if (Parse::ParseUrlAddress(txt, address))
		{
			// get branch target
			if (m_project)
			{
				const uint32 branchTargetAddress = m_project->GetEnv().GetDecodingContext()->GetAddressMap().GetReferencedAddress(address);
				if (branchTargetAddress)
				{
					return NavigateToAddress(branchTargetAddress);
				}
			}
		}
	}
	else if (Parse::ParseUrlKeyword(txt, "referencers:"))
	{
		// decode address
		uint32 address = 0;
		if (Parse::ParseUrlAddress(txt, address))
		{
			// get branch target
			if (m_project)
			{
				std::vector< uint32 > sourceAddresses;
				m_project->GetEnv().GetDecodingContext()->GetAddressMap().GetAddressReferencers(address, sourceAddresses);
				if ( sourceAddresses.size() == 0 )
				{
					m_logView->Log( "Reference: No locations referencing current address" );
					return false;
				}
				else if ( sourceAddresses.size() == 1 )
				{
					return NavigateToAddress(sourceAddresses[0]);
				}
				else
				{
					for ( uint32 i=0; i<sourceAddresses.size(); ++i )
					{
						m_logView->Log( "Reference: Location referenced from %08Xh", 
							sourceAddresses[i] );
					}

					return true;
				}
			}
		}
	}
	else if (Parse::ParseUrlKeyword(txt, "memscan:"))
	{
		// decode entry index
		int entryIndex = 0;
		if (!Parse::ParseUrlInteger(txt, entryIndex))
			return false;

		// decode access type
		int accessType = 0;
		if (!Parse::ParseUrlInteger(txt, accessType))
			return false;

		// decode search direction
		int searchDir = 0;
		if (!Parse::ParseUrlInteger(txt, searchDir))
			return false;

		// decode memory address
		uint32 memoryAddress = 0;
		if (!Parse::ParseUrlAddress(txt, memoryAddress))
			return false;

		// decode memory size
		int memorySize = 0;
		if (!Parse::ParseUrlInteger(txt, memorySize))
			return false;

		// no project
		if (!m_project || !m_project->GetTrace())
			return false;

		// scan for memory access
		uint32 foundEntryIndex = 0;
		uint32 foundCodeAddress = 0;
		if ( searchDir == 1 )
		{
			if (!m_project->GetTrace()->FindForwardMemoryAccess(entryIndex, memoryAddress, memorySize, accessType, foundEntryIndex, foundCodeAddress))
			{
				m_logView->Log( "MemTrace: No memory access found" );
				return false;
			}
		}
		else if ( searchDir == -1 )
		{
			if (!m_project->GetTrace()->FindBackwardMemoryAccess(entryIndex, memoryAddress, memorySize, accessType, foundEntryIndex, foundCodeAddress))
			{
				m_logView->Log( "MemTrace: No memory access found" );
				return false;
			}
		}
		else if ( searchDir == 0 )
		{
			if (!m_project->GetTrace()->FindFirstMemoryAccess(entryIndex, memoryAddress, memorySize, accessType, foundEntryIndex, foundCodeAddress))
			{
				m_logView->Log( "MemTrace: No memory access found" );
				return false;
			}
		}
		else
		{
			m_logView->Log( "MemTrace: Invalid search params" );
			return false;
		}

		m_logView->Log( "MemTrace: Memory access at IP=%06Xh, TI=#%06u", foundCodeAddress, foundEntryIndex);
		return true;
	}
	else if (Parse::ParseUrlKeyword(txt, "regscan:"))
	{
		// decode entry index
		int entryIndex = 0;
		if (!Parse::ParseUrlInteger(txt, entryIndex))
			return false;

		// decode access type
		int accessType = 0;
		if (!Parse::ParseUrlInteger(txt, accessType))
			return false;

		// decode search direction
		int searchDir = 0;
		if (!Parse::ParseUrlInteger(txt, searchDir))
			return false;

		// decode register name
		char regName[16];
		if (!Parse::ParseUrlPart(txt, regName, ARRAYSIZE(regName) ))
			return false;

		// no project
		if (!m_project || !m_project->GetTrace())
			return false;

		// scan for memory access
		uint32 foundEntryIndex = 0;
		uint32 foundCodeAddress = 0;
		if ( searchDir == -1 )
		{
			if (!m_project->GetTrace()->FindBackwardRegisterAccess(entryIndex, regName, accessType, foundEntryIndex, foundCodeAddress))
			{
				m_logView->Log( "RegTrace: No register access found" );
				return false;
			}
		}
		else if ( searchDir == 1 )
		{
			if (!m_project->GetTrace()->FindForwardRegisterAccess(entryIndex, regName, accessType, foundEntryIndex, foundCodeAddress))
			{
				m_logView->Log( "RegTrace: No register access found" );
				return false;
			}
		}
		else
		{
			m_logView->Log( "RegTrace: Invalid search directory" );
			return false;
		}

		m_logView->Log( "RegTrace: Register access at IP=%06Xh, TI=#%06u", foundCodeAddress, foundEntryIndex);
		return true;
	}
	else if (Parse::ParseUrlKeyword(txt, "memhistory:"))
	{
		// decode entry index
		int entryIndex = 0;
		if (!Parse::ParseUrlInteger(txt, entryIndex))
			return false;

		// decode memory address
		uint32 memoryAddress = 0;
		if (!Parse::ParseUrlAddress(txt, memoryAddress))
			return false;

		// decode memory size
		int memorySize = 0;
		if (!Parse::ParseUrlInteger(txt, memorySize))
			return false;

		// no project
		if (!m_project || !m_project->GetTrace())
			return false;

		wxCommandEvent fakeEvent;
		OnOpenMemory( fakeEvent );

		if ( m_memory )
		{
			m_memory->ShowAddressHistory( memoryAddress, memorySize, entryIndex );
		}

		return true;
	}
	else if (Parse::ParseUrlKeyword(txt, "history:"))
	{
		// address
		unsigned int address = 0;
		if (!Parse::ParseUrlAddress(txt, address))
			return false;

		// parse start trace
		int startTrace = 0;
		if (!Parse::ParseUrlInteger(txt, startTrace))
			return false;

		// parse end trace
		int endTrace = 0;
		if (!Parse::ParseUrlInteger(txt, endTrace))
			return false;

		// parse register name
		char regName[16];
		if (!Parse::ParseUrlPart(txt, regName, ARRAYSIZE(regName) ))
			return false;

		// no project
		if (!m_project || !m_project->GetTrace())
			return false;

		// get call history
		std::vector< ProjectTraceAddressHistoryInfo > history;
		if ( !m_project->GetTrace()->GetRegisterHistory( address, regName, startTrace, endTrace, history ) )
		{
			m_logView->Log( "History: Failed to get value history" );
			return false;
		}

		// print call history
		Parse::PrintValueHistory( *m_logView, history );
		return true;
	}
	else if (Parse::ParseUrlKeyword(txt, "timemachine:"))
	{
		// decode entry index
		int entryIndex = 0;
		if (!Parse::ParseUrlInteger(txt, entryIndex))
			return false;

		// open time machine for given trace entry
		OpenTimeMachine( entryIndex );
		return true;
	}


	// invalid navigation url
	return false;
}

bool CMainFrame::NavigateToTraceEntry(const uint32 entry)
{
	if (m_project && m_project->GetTrace())
	{
		if ( m_project->GetTrace()->MoveToEntry(entry) )
		{
			RefreshTrace();
			return true;
		}
	}

	return false;
}

bool CMainFrame::NavigateToAddress(const uint32 address, const bool addToHistory)
{
	if (m_project)
	{
		const uint32 offset = address - m_project->GetEnv().GetImage()->GetBaseAddress();
		m_memoryView->SetActiveOffset(offset, addToHistory);
		return true;
	}

	return false;
}
