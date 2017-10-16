#pragma once

#include "eventListener.h"

namespace tools
{

	//---------------------------------------------------------------------------

	/// Event system
	class EventDispatcher
	{
	public:
		// get global instance
		static EventDispatcher& GetInstance();

		// listener register/remove
		void RegisterListener(IEventListener* listener);
		void UnregisterListener(IEventListener* listener);

		// dispatch queued posted events
		void DispatchPostedEvents();

		// post event, NOTE: posted events are not dispatched right away
		// NOTE: thread safe
		void PostEvent(const EventID& id);
		void PostEvent(const EventID& id, const EventArg& a);
		void PostEvent(const EventID& id, const EventArg& a, const EventArg& b);
		void PostEvent(const EventID& id, const EventArg& a, const EventArg& b, const EventArg& c);
		void PostEvent(const EventID& id, const EventArg& a, const EventArg& b, const EventArg& c, const EventArg& d);
		void PostEvent(const EventID& id, const EventArg& a, const EventArg& b, const EventArg& c, const EventArg& d, const EventArg& e);
		void PostEvent(const EventID& id, const EventArg& a, const EventArg& b, const EventArg& c, const EventArg& d, const EventArg& e, const EventArg& f);
		void PostEvent(const EventID& id, const EventArgs& args);

		// dispatch event, NOTE: dispatched events are processed right away (unsafe)
		// NOTE: not thread safe, can only be called from main thread
		void DispatchEvent(const EventID& id);
		void DispatchEvent(const EventID& id, const EventArg& a);
		void DispatchEvent(const EventID& id, const EventArg& a, const EventArg& b);
		void DispatchEvent(const EventID& id, const EventArg& a, const EventArg& b, const EventArg& c);
		void DispatchEvent(const EventID& id, const EventArg& a, const EventArg& b, const EventArg& c, const EventArg& d);
		void DispatchEvent(const EventID& id, const EventArg& a, const EventArg& b, const EventArg& c, const EventArg& d, const EventArg& e);
		void DispatchEvent(const EventID& id, const EventArg& a, const EventArg& b, const EventArg& c, const EventArg& d, const EventArg& e, const EventArg& f);
		void DispatchEvent(const EventID& id, const EventArgs& args);

	private:
		EventDispatcher();

		struct PostedEvent
		{
			EventID m_id;
			EventArgs m_args;
		};

		typedef std::vector<PostedEvent> TPostedEvents;
		TPostedEvents m_postedEvents;
		std::mutex m_postedEventsLock;

		typedef std::unordered_map<IEventListener*, bool> TListeners;
		TListeners m_listeners;
		std::mutex m_listenersLock;
		std::atomic<bool> m_listenersRemoved;

		void PurgeRemovedListeners();
	};

	//---------------------------------------------------------------------------

} // tools