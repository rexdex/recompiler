#pragma once

namespace utils
{
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

	template< typename V >
	static inline bool RemoveFromVector(std::vector< V >& data, const V& elem)
	{
		auto it = std::find(data.begin(), data.end(), elem);
		if (it != data.end())
		{
			data.erase(it);
			return true;
		}
		return false;
	}

	// name thread
	extern void SetThreadName( const uint32 threadID, const char* name );

	//---------------------------------------------------------------------------

} // utils