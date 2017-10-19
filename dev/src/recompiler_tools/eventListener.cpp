#include "build.h"
#include "eventListener.h"
#include "eventDispatcher.h"
#include "../recompiler_core/internalUtils.h"

namespace tools
{
	//---------------------------------------------------------------------------

	EventID::EventID()
		: m_hash(0)
		, m_name(nullptr)
	{}

	EventID::EventID(const char* name)
		: m_name(name)
		, m_hash((name && *name) ? StringCRC64(name) : 0)
	{}

	//---------------------------------------------------------------------------

	IEventData::IEventData(const char* tag)
		: m_tag(StringCRC64(tag))
	{}

	IEventData::~IEventData()
	{}

	//---------------------------------------------------------------------------

	EventArg::EventArg()
		: m_type(Type::None)
		, m_value(0)
		, m_pointer(0)
	{}

	EventArg::EventArg(nullptr_t)
		: m_type(Type::None)
		, m_value(0)
		, m_pointer(0)
	{}

	EventArg::EventArg(const char* string)
		: m_type(Type::String)
		, m_string(string)
		, m_pointer(0)
		, m_value(0)
	{}

	EventArg::EventArg(const std::string& str)
		: m_type(Type::String)
		, m_string(str)
		, m_pointer(0)
		, m_value(0)
	{}

	EventArg::EventArg(void* ptr)
		: m_type(Type::Pointer)
		, m_pointer(ptr)
		, m_value(0)
	{}

	EventArg::EventArg(const void* ptr)
		: m_type(Type::ConstPointer)
		, m_pointer((void*)ptr)
		, m_value(0)
	{}

	EventArg::EventArg(const uint64 val)
		: m_type(Type::Value)
		, m_pointer(nullptr)
		, m_value(val)
	{}

	EventArg::EventArg(IEventData* data)
		: m_type(data ? Type::CustomData : Type::None)
		, m_pointer(nullptr)
		, m_value(0)
		, m_data(data)
	{}

	const char* EventArg::AsString() const
	{
		wxASSERT(m_type == Type::String);
		return m_string.c_str();
	}

	void* EventArg::AsPointer() const
	{
		wxASSERT(m_type == Type::Pointer);
		return m_pointer;
	}

	const void* EventArg::AsConstPointer() const
	{
		wxASSERT(m_type == Type::Pointer || m_type == Type::ConstPointer);
		return m_pointer;
	}

	const uint64 EventArg::AsValue() const
	{
		wxASSERT(m_type == Type::Value);
		return m_value;
	}

	const IEventData& EventArg::AsData(const char* tag) const
	{
		const auto tagCRC = StringCRC64(tag);
		wxASSERT(m_type == Type::CustomData);
		wxASSERT(m_data);
		wxASSERT(m_data->GetTag() == tagCRC);
		return *m_data;
	}

	//---------------------------------------------------------------------------

	EventArgs::EventArgs()
		: m_numArgs(0)
	{}

	EventArgs::EventArgs(const EventArg& a)
		: m_numArgs(1)
	{
		m_args[0] = a;
	}

	EventArgs::EventArgs(const EventArg& a, const EventArg& b)
		: m_numArgs(2)
	{
		m_args[0] = a;
		m_args[1] = b;
	}

	EventArgs::EventArgs(const EventArg& a, const EventArg& b, const EventArg& c)
		: m_numArgs(3)
	{
		m_args[0] = a;
		m_args[1] = b;
		m_args[2] = c;
	}

	EventArgs::EventArgs(const EventArg& a, const EventArg& b, const EventArg& c, const EventArg& d)
		: m_numArgs(4)
	{
		m_args[0] = a;
		m_args[1] = b;
		m_args[2] = c;
		m_args[3] = d;
	}

	EventArgs::EventArgs(const EventArg& a, const EventArg& b, const EventArg& c, const EventArg& d, const EventArg& e)
		: m_numArgs(5)
	{
		m_args[0] = a;
		m_args[1] = b;
		m_args[2] = c;
		m_args[3] = d;
		m_args[4] = e;
	}

	EventArgs::EventArgs(const EventArg& a, const EventArg& b, const EventArg& c, const EventArg& d, const EventArg& e, const EventArg& f)
		: m_numArgs(6)
	{
		m_args[0] = a;
		m_args[1] = b;
		m_args[2] = c;
		m_args[3] = d;
		m_args[4] = e;
		m_args[5] = f;
	}

	//---------------------------------------------------------------------------

	IEventListener::IEventListener()
	{
		EventDispatcher::GetInstance().RegisterListener(this);
	}

	IEventListener::~IEventListener()
	{
		EventDispatcher::GetInstance().UnregisterListener(this);
	}

	void IEventListener::HandleEvent(const EventID& id, const EventArgs& args)
	{
		const auto it = m_eventMap.find(id);
		if (it != m_eventMap.end())
		{
			for (const auto& handler : it->second)
				handler(id, args);
		}
	}

	IEventListener::EventAssigner::EventAssigner(const EventID& id, TEventHandleMap* eventMap)
		: m_id(id)
		, m_map(eventMap)
	{}

	void IEventListener::EventAssigner::Reset()
	{
		m_map->erase(m_id);
	}

	IEventListener::EventAssigner& IEventListener::EventAssigner::operator=(const TEventFunc& func)
	{
		if (m_id.IsValid())
		{
			(*m_map)[m_id].clear();
			(*m_map)[m_id].push_back(func);
		}
		return *this;
	}

	IEventListener::EventAssigner& IEventListener::EventAssigner::operator+=(const TEventFunc& func)
	{
		if (m_id.IsValid())
			(*m_map)[m_id].push_back(func);

		return *this;
	}

	//---------------------------------------------------------------------------

	IEventListener::EventAssigner IEventListener::Event(const EventID& id)
	{
		return EventAssigner(id, &m_eventMap);
	}

	//---------------------------------------------------------------------------

} // tools
