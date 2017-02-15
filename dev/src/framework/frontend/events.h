#pragma once

//---------------------------------------------------------------------------

/// Project stuff listener
class IEventListener
{
public:
	IEventListener();
	virtual ~IEventListener();
	virtual void OnTraceOpened() {};
	virtual void OnTraceClosed() {};
	virtual void OnTraceIndexChanged( const uint32 currentEntry ) {};
};

//---------------------------------------------------------------------------

/// Event system
class CEventDispatcher
{
public:
	CEventDispatcher();
	~CEventDispatcher();

	// listener register/remove
	void RegisterListener( IEventListener* listener );
	void UnregisterListener( IEventListener* listener );

	// dispatch events
	void EventTraceOpened();
	void EventTraceClosed();
	void EventTraceIndexChanged( const uint32 currentEntry );

private:
	typedef std::vector< IEventListener* >		TListeners;
	TListeners		m_listeners;
};

//---------------------------------------------------------------------------
