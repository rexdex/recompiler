#pragma once

namespace utils
{

	template< typename T >
	class MemPtr32
	{
	public:
		static_assert( sizeof(T) == 4, "Ptr size mismatch" );

		MemPtr32()
			: m_ptr(NULL)
		{};

		MemPtr32(const uint64 addr)
			: m_ptr((uint32*) (const uint32)addr)
		{}

		const uint32 Addr() const
		{
			return (uint32)m_ptr;
		}

		operator bool() const
		{
			return m_ptr != nullptr;
		}

		T Get() const
		{
			union
			{
				T v;
				uint32 u;
			} x;
			x.u = _byteswap_ulong(*m_ptr);
			return x.v;
		}

		void Set(T value)
		{
			union
			{
				T v;
				uint32 u;
			} x;
			x.v = value;
			*m_ptr = _byteswap_ulong(x.u);
		}

	private:
		uint32*	m_ptr;
	};

	template< typename T >
	class MemPtr64
	{
	public:
		static_assert( sizeof(T) == 8, "Ptr size mismatch" );

		MemPtr64()
			: m_ptr(NULL)
		{};

		MemPtr64(const uint64 addr)
			: m_ptr((uint64*)(const uint32)addr)
		{}

		const uint32 Addr() const
		{
			return (uint32)m_ptr;
		}

		operator bool() const
		{
			return m_ptr != nullptr;
		}

		T Get() const
		{
			union
			{
				T v;
				uint64 u;
			} x;
			x.u = _byteswap_uint64(*m_ptr);
			return x.v;
		}

		void Set(T value)
		{
			union
			{
				T v;
				uint64 u;
			} x;
			x.v = value;
			*m_ptr = _byteswap_uint64(x.u);
		}

	private:
		uint64*	m_ptr;
	};

	//---------------------------------------------------------------------------

	template< typename K, typename V >
	static inline bool Find( const std::map<K,V>& data, const K& key, V& outVal )
	{
		auto it = data.find( key );
		if ( it != data.end() )
		{
			outVal = it->second;
			return true;
		}

		return false;
	}

	template< typename V >
	static inline void ClearPtr( std::vector< V >& data )
	{
		for ( auto it : data )
			delete it;
		data.clear();
	}

	// name thread
	extern void SetThreadName( const uint32 threadID, const char* name );

	//---------------------------------------------------------------------------

} // utils