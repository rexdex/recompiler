#include "build.h"
#include "project.h"
#include "projectAddressHistory.h"

ProjectAdddrsesHistory::ProjectAdddrsesHistory()
	: m_currentHistoryItem( 0 )
{
}

void ProjectAdddrsesHistory::Reset( const uint32 rvaAddress )
{
	AddrInfo newEntry;
	newEntry.m_addr = rvaAddress;
	newEntry.m_manual = true;

	m_addressHistory.clear();
	m_addressHistory.push_back( newEntry );
	m_currentHistoryItem = 0;
}

void ProjectAdddrsesHistory::UpdateAddress( const uint32 rvaAddress, const bool manualJump )
{
	// always remove everything after current history position
	while ( m_addressHistory.size() > (m_currentHistoryItem+1) )
	{
		m_addressHistory.pop_back();
	}

	// get the current address
	const AddrInfo& currentInfo = m_addressHistory[ m_currentHistoryItem ];

	// if we can to create a new jump, add it directly
	if ( manualJump )
	{
		if ( rvaAddress != currentInfo.m_addr )
		{
			// whatevery we had is not a valid navigation address
			m_addressHistory[ m_currentHistoryItem ].m_manual = true;

			// add new entry
			AddrInfo newEntry;
			newEntry.m_addr = rvaAddress;
			newEntry.m_manual = true;

			// add to stack
			m_currentHistoryItem = m_addressHistory.size();
			m_addressHistory.push_back( newEntry );
		}
	}
	else
	{
		if ( rvaAddress != currentInfo.m_addr )
		{
			// previous address is a "manual navigation" - we can't modify it, create new entry that we can modify
			if ( currentInfo.m_manual )
			{
				AddrInfo newEntry;
				newEntry.m_addr = rvaAddress;
				newEntry.m_manual = false; // automatic - we can modify it

				// add to stack
				m_currentHistoryItem = m_addressHistory.size();
				m_addressHistory.push_back( newEntry );
			}
			else // automatic entry
			{
				// modify current entry
				m_addressHistory[ m_currentHistoryItem ].m_addr = rvaAddress;

				// is it the same as the previous address ? if so, delete it
				if ( m_addressHistory.size() >= 2 && m_addressHistory[ m_currentHistoryItem-1 ].m_manual && 
					m_addressHistory[ m_currentHistoryItem-1 ].m_addr == rvaAddress )
				{
					m_currentHistoryItem -= 1;
					m_addressHistory.pop_back();
				}
			}
		}
	}
}

uint32 ProjectAdddrsesHistory::GetCurrentAddress() const
{
	return m_addressHistory[ m_currentHistoryItem ].m_addr;
}

uint32 ProjectAdddrsesHistory::GetTopAddress() const
{
	return m_addressHistory.back().m_addr;
}

bool ProjectAdddrsesHistory::CanNavigateBack() const
{
	return m_currentHistoryItem > 0;
}

uint32 ProjectAdddrsesHistory::NavigateBack()
{
	if ( m_currentHistoryItem > 0 )
	{
		m_currentHistoryItem -= 1;
		return m_addressHistory[ m_currentHistoryItem ].m_addr;
	}
	else
	{
		return INVALID_ADDRESS;
	}
}

bool ProjectAdddrsesHistory::CanNavigateForward() const
{
	return m_currentHistoryItem < (m_addressHistory.size() - 1 );
}

uint32 ProjectAdddrsesHistory::NavigateForward()
{
	if ( m_currentHistoryItem < (m_addressHistory.size() - 1 ) )
	{
		m_currentHistoryItem += 1;
		return m_addressHistory[ m_currentHistoryItem ].m_addr;
	}
	else
	{
		return INVALID_ADDRESS;
	}
}

void ProjectAdddrsesHistory::GetFullHistory( std::vector< uint32 >& outAddresses ) const
{
	for ( uint32 i=0; i<m_addressHistory.size(); ++i )
	{
		outAddresses.push_back( m_addressHistory[i].m_addr );
	}
}

