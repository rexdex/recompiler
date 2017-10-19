#pragma once

namespace tools
{

	//---------------------------------------------------------------------------

	/// ID of the event
	class EventID
	{
	public:
		EventID();
		EventID(const char* name);

		inline const bool operator==(const EventID& other) const
		{
			return m_hash == other.m_hash;
		}

		inline const bool operator!=(const EventID& other) const
		{
			return m_hash != other.m_hash;
		}

		inline const bool operator<(const EventID& other) const
		{
			return m_hash < other.m_hash;
		}

		inline const bool IsValid() const
		{
			return m_hash != 0;
		}

		inline const char* GetName() const
		{
			return m_name;
		}

		inline const uint64 GetHash() const
		{
			return m_hash;
		}

	private:
		const char* m_name;
		uint64 m_hash;
	};
} // tools

namespace std
{
	template <> struct hash<tools::EventID>
	{
		size_t operator()(const tools::EventID& x) const
		{
			return x.GetHash();
		}
	};
} // std

namespace tools
{

	//---------------------------------------------------------------------------

	/// additional data
	class IEventData
	{
	public:
		IEventData(const char* tag);
		virtual ~IEventData();

		inline const uint64 GetTag() const { return m_tag; }

	private:
		const uint64 m_tag;
	};

	/// typed data
	template< typename T >
	class TEventData
	{
	public:
		TEventData()
			: IEventData(typeid(T).name())
		{}
	};

	/// event data
	struct EventArg
	{
	public:
		enum class Type
		{
			None,
			String,
			Pointer,
			ConstPointer,
			Value,
			CustomData,
		};

		EventArg();
		EventArg(nullptr_t);
		EventArg(const char* string);
		EventArg(const std::string& str);
		EventArg(void* ptr);
		EventArg(const void* ptr);
		EventArg(const uint64 val);
		EventArg(IEventData* data);

		inline const Type GetType() const { return m_type; }
		inline const bool IsValid() const { return m_type != Type::None; }

		const char* AsString() const;
		void* AsPointer() const;
		const void* AsConstPointer() const;
		const uint64 AsValue() const;
		const IEventData& AsData(const char* tag) const;

		template< typename T >
		inline const T& AsData() const
		{
			const auto tagName = typeid(T).name();
			return *(const T*)AsData(tagName);
		}

		template< typename T >
		inline const T* AsConstPointerToType() const
		{
			return static_cast<const T*>(AsConstPointer());
		}

		template< typename T >
		inline const T* AsPointer() const
		{
			return static_cast<T*>(AsPointer());
		}

	private:
		Type m_type;
		std::string m_string;
		void* m_pointer;
		uint64 m_value;
		std::shared_ptr<IEventData> m_data;
	};

	//---------------------------------------------------------------------------

	/// event arguments
	class EventArgs
	{
	public:
		EventArgs();
		EventArgs(const EventArg& a);
		EventArgs(const EventArg& a, const EventArg& b);
		EventArgs(const EventArg& a, const EventArg& b, const EventArg& c);
		EventArgs(const EventArg& a, const EventArg& b, const EventArg& c, const EventArg& d);
		EventArgs(const EventArg& a, const EventArg& b, const EventArg& c, const EventArg& d, const EventArg& e);
		EventArgs(const EventArg& a, const EventArg& b, const EventArg& c, const EventArg& d, const EventArg& e, const EventArg& f);

		inline const uint32 Size() const
		{
			return m_numArgs;
		}

		inline const EventArgs& operator[](const uint32 index) const
		{
			wxASSERT(index < m_numArgs);
			return m_args[index];
		}

	private:
		static const uint32 MAX_ARGS = 8;
		EventArg m_args[MAX_ARGS];
		uint32 m_numArgs;
	};

	//---------------------------------------------------------------------------

	/// event handling function
	typedef std::function<void(const EventID& id, const EventArgs& args)> TEventFunc;
	typedef std::vector<TEventFunc> TEventHandleTable;
	typedef std::unordered_map<EventID, TEventHandleTable> TEventHandleMap;

	//---------------------------------------------------------------------------

	/// Project stuff listener
	class IEventListener
	{
	public:
		IEventListener();
		virtual ~IEventListener();

		// called when event occurs
		void HandleEvent(const EventID& id, const EventArgs& args);

		// event assigner
		struct EventAssigner
		{
		public:
			EventAssigner(const EventID& id, TEventHandleMap* eventMap);
			EventAssigner(const EventAssigner& other) = default;
			EventAssigner(EventAssigner&& other) = default;
			EventAssigner& operator=(const EventAssigner& other) = default;
			EventAssigner& operator=(EventAssigner&& other) = default;

			// clear all events
			void Reset();

			// replace event handler
			EventAssigner& operator=(const TEventFunc& func);

			// append event handler
			EventAssigner& operator+=(const TEventFunc& func);

		private:
			TEventHandleMap* m_map;
			EventID m_id;
		};

		// register event handler
		EventAssigner Event(const EventID& id);

	private:
		TEventHandleMap m_eventMap;
	};

} // tools
