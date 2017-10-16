#include "build.h"
#include "eventListener.h"
#include "eventDispatcher.h"

namespace tools
{
	EventDispatcher::EventDispatcher()
		: m_listenersRemoved(false)
	{}

	EventDispatcher& EventDispatcher::GetInstance()
	{
		static EventDispatcher theInstance;
		return theInstance;
	}

	void EventDispatcher::RegisterListener(IEventListener* listener)
	{
		std::lock_guard<std::mutex> lock(m_listenersLock);
		m_listeners[listener] = true;
	}

	void EventDispatcher::UnregisterListener(IEventListener* listener)
	{
		std::lock_guard<std::mutex> lock(m_listenersLock);
		m_listeners[listener] = false;
		m_listenersRemoved.exchange(true);
	}

	void EventDispatcher::PurgeRemovedListeners()
	{
		std::lock_guard<std::mutex> lock(m_listenersLock);

		for (auto it = m_listeners.begin(); it != m_listeners.end(); )
		{
			if (!it->second)
				it = m_listeners.erase(it); // previously this was something like m_map.erase(it++);
			else
				++it;
		}
	}

	void EventDispatcher::DispatchPostedEvents()
	{
		// finalize removal of listeners
		if (m_listenersRemoved.exchange(false))
			PurgeRemovedListeners();

		// get events to dispatch
		TPostedEvents events;
		{
			std::lock_guard<std::mutex> lock(m_postedEventsLock);
			events = std::move(m_postedEvents);
		}

		// dispatch the events
		for (const auto& evt : events)
			DispatchEvent(evt.m_id, evt.m_args);
	}

	void EventDispatcher::PostEvent(const EventID& id)
	{
		PostEvent(id, EventArgs());
	}

	void EventDispatcher::PostEvent(const EventID& id, const EventArg& a)
	{
		PostEvent(id, EventArgs(a));
	}

	void EventDispatcher::PostEvent(const EventID& id, const EventArg& a, const EventArg& b)
	{
		PostEvent(id, EventArgs(a, b));
	}

	void EventDispatcher::PostEvent(const EventID& id, const EventArg& a, const EventArg& b, const EventArg& c)
	{
		PostEvent(id, EventArgs(a, b, c));
	}

	void EventDispatcher::PostEvent(const EventID& id, const EventArg& a, const EventArg& b, const EventArg& c, const EventArg& d)
	{
		PostEvent(id, EventArgs(a, b, c, d));
	}

	void EventDispatcher::PostEvent(const EventID& id, const EventArg& a, const EventArg& b, const EventArg& c, const EventArg& d, const EventArg& e)
	{
		PostEvent(id, EventArgs(a, b, c, d, e));
	}

	void EventDispatcher::PostEvent(const EventID& id, const EventArg& a, const EventArg& b, const EventArg& c, const EventArg& d, const EventArg& e, const EventArg& f)
	{
		PostEvent(id, EventArgs(a, b, c, d, e, f));
	}

	void EventDispatcher::PostEvent(const EventID& id, const EventArgs& args)
	{
		if (id.IsValid())
		{
			std::lock_guard<std::mutex> lock(m_postedEventsLock);
			m_postedEvents.push_back({ id, args });
		}
	}

	void EventDispatcher::DispatchEvent(const EventID& id)
	{
		DispatchEvent(id, EventArgs());
	}

	void EventDispatcher::DispatchEvent(const EventID& id, const EventArg& a)
	{
		DispatchEvent(id, EventArgs(a));
	}

	void EventDispatcher::DispatchEvent(const EventID& id, const EventArg& a, const EventArg& b)
	{
		DispatchEvent(id, EventArgs(a, b));
	}

	void EventDispatcher::DispatchEvent(const EventID& id, const EventArg& a, const EventArg& b, const EventArg& c)
	{
		DispatchEvent(id, EventArgs(a, b, c));
	}

	void EventDispatcher::DispatchEvent(const EventID& id, const EventArg& a, const EventArg& b, const EventArg& c, const EventArg& d)
	{
		DispatchEvent(id, EventArgs(a, b, c, d));
	}

	void EventDispatcher::DispatchEvent(const EventID& id, const EventArg& a, const EventArg& b, const EventArg& c, const EventArg& d, const EventArg& e)
	{
		DispatchEvent(id, EventArgs(a, b, c, d, e));
	}

	void EventDispatcher::DispatchEvent(const EventID& id, const EventArg& a, const EventArg& b, const EventArg& c, const EventArg& d, const EventArg& e, const EventArg& f)
	{
		DispatchEvent(id, EventArgs(a, b, c, d, e, f));
	}

	void EventDispatcher::DispatchEvent(const EventID& id, const EventArgs& args)
	{
		if (id.IsValid())
		{
			std::lock_guard<std::mutex> lock(m_listenersLock);

			for (const auto it : m_listeners)
			{
				if (it.second)
					it.first->HandleEvent(id, args);
			}
		}
	}

} // tools
