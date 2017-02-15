#include "build.h"
#include "events.h"

//---------------------------------------------------------------------------

IEventListener::IEventListener()
{
}

IEventListener::~IEventListener()
{
}

//---------------------------------------------------------------------------

CEventDispatcher::CEventDispatcher()
{
}

CEventDispatcher::~CEventDispatcher()
{
}

void CEventDispatcher::RegisterListener( IEventListener* listener )
{
	if ( listener )
	{
		auto it = std::find( m_listeners.begin(), m_listeners.end(), listener );
		if ( it == m_listeners.end() )
		{
			m_listeners.push_back( listener );
		}
	}
}

void CEventDispatcher::UnregisterListener( IEventListener* listener )
{
	auto it = std::find( m_listeners.begin(), m_listeners.end(), listener );
	if ( it == m_listeners.end() )
	{
		m_listeners.erase( it );
	}
}

void CEventDispatcher::EventTraceOpened()
{
	for ( IEventListener* listener : m_listeners )
	{
		listener->OnTraceOpened();
	}
}

void CEventDispatcher::EventTraceClosed()
{
	for ( IEventListener* listener : m_listeners )
	{
		listener->OnTraceClosed();
	}
}

void CEventDispatcher::EventTraceIndexChanged( const uint32 currentEntry )
{
	for ( IEventListener* listener : m_listeners )
	{
		listener->OnTraceIndexChanged( currentEntry );
	}
}