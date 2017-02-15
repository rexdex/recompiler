#include "build.h"
#include "decodingMemoryMap.h"
#include "decodingNameMap.h"
#include "internalFile.h"

namespace decoding
{

	//-------------------------------------------------------------------------

	NameMap::NameMap( class MemoryMap* map )
		: m_memoryMap( map )
		, m_isModified( false )
	{
	}

	NameMap::~NameMap()
	{
	}

	const char* NameMap::GetName( const uint32 addr ) const
	{
		// read comment data
		TNameMap::const_iterator it = m_nameMap.find( addr );
		if ( it != m_nameMap.end() )
		{
			return (*it).second.c_str();
		}

		// no name stored for address
		return "";
	}

	void NameMap::SetName( const uint32 addr, const char* comment )
	{
		if ( m_memoryMap->IsValidAddress( addr ) )
		{
			if ( !comment || !comment[0] )
			{
				TNameMap::const_iterator it = m_nameMap.find( addr );
				if ( it != m_nameMap.end() )
				{
					m_nameMap.erase( it );
					m_isModified = true;
				}
			}
			else
			{
				// modify only when needed
				TNameMap::const_iterator it = m_nameMap.find( addr );
				if ( it == m_nameMap.end() || (*it).second != comment )
				{
					m_nameMap[ addr ] = comment;
					m_isModified = true;
				}
			}
		}
	}

	bool NameMap::Save( ILogOutput& log, class IBinaryFileWriter& writer ) const
	{
		CBinaryFileChunkScope chunk( writer, eFileChunk_NameMap );
		writer << m_nameMap;

		m_isModified = false;
		return true;
	}

	bool NameMap::Load( ILogOutput& log, class IBinaryFileReader& reader )
	{
		CBinaryFileChunkScope chunk( reader, eFileChunk_NameMap );
		reader >> m_nameMap;

		m_isModified = false;
		return true;
	}

	void NameMap::EnumerateSymbols( const char* nameFilter, ISymbolConsumer& reporter ) const
	{
		for ( TNameMap::const_iterator it=m_nameMap.begin();
			it != m_nameMap.end(); ++it )
		{
			const char* symbolName = it->second.c_str();
			if ( strstr( symbolName, nameFilter ) != NULL )
			{
				SymbolInfo info;
				info.m_address = it->first;
				info.m_name = symbolName;
				reporter.ProcessSymbols( &info, 1 );
			}
		}
	}

	//-------------------------------------------------------------------------

} // decoding