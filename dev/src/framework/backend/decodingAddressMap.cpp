#include "build.h"
#include "decodingMemoryMap.h"
#include "decodingAddressMap.h"
#include "internalFile.h"

namespace decoding
{

	//-------------------------------------------------------------------------

	AddressMap::AddressMap( class decoding::MemoryMap* map )
		: m_memoryMap( map )
		, m_isModified( false )
	{
	}

	AddressMap::~AddressMap()
	{
	}

	const uint32 AddressMap::GetReferencedAddress( const uint32 addr ) const
	{
		// read comment data
		const decoding::MemoryFlags flags = m_memoryMap->GetMemoryInfo( addr );
		if ( flags.IsExecutable() && 
			(flags.GetInstructionFlags().IsBranchTarget() || flags.GetInstructionFlags().IsDataRef()) )
		{
			TAddressMap::const_iterator it = m_addressMap.find( addr );
			if ( it != m_addressMap.end() )
			{
				return (*it).second;
			}
		}
		else if ( flags.IsGenericData() && 
			(flags.GetDataFlags().IsCodePtr() || flags.GetDataFlags().IsDataPtr()) )
		{
			TAddressMap::const_iterator it = m_addressMap.find( addr );
			if ( it != m_addressMap.end() )
			{
				return (*it).second;
			}
		}

		// no branchcomment
		return 0;
	}

	void AddressMap::GetAddressReferencers( const uint32 addr, std::vector< uint32 >& outAddresses ) const
	{
		for ( TAddressMap::const_iterator it = m_addressMap.begin();
			it != m_addressMap.end(); ++it )
		{
			if ( (*it).second == addr )
			{
				outAddresses.push_back( (*it).first );
			}
		}
	}

	void AddressMap::SetReferencedAddress( const uint32 addr, const uint32 targetAddress )
	{
		if ( m_memoryMap->IsValidAddress( addr ) )
		{
			// read comment data
			const decoding::MemoryFlags flags = m_memoryMap->GetMemoryInfo( addr );
			if ( flags.IsExecutable() )
			{
				if ( targetAddress != 0 )
				{
					// set the branch flag
					m_memoryMap->SetMemoryBlockSubType( ILogOutput::DevNull(), addr, (uint32)InstructionFlag::HasBranchTarget, 0 );

					// set new branch target
					TAddressMap::const_iterator it = m_addressMap.find( addr );
					if ( it == m_addressMap.end() || targetAddress != (*it).second )
					{
						m_addressMap[ addr ] = targetAddress;
						m_isModified = true;
					}
				}
				else
				{
					// clear the branch flag
					m_memoryMap->SetMemoryBlockSubType( ILogOutput::DevNull(), addr, 0, (uint32)InstructionFlag::HasBranchTarget );

					// remove from map
					TAddressMap::const_iterator it = m_addressMap.find( addr );
					if ( it != m_addressMap.end() )
					{
						m_addressMap.erase( it );
						m_isModified = true;
					}
				}
			}
			else if ( flags.IsGenericData() )
			{
				if ( targetAddress != 0 )
				{
					// set new branch target
					TAddressMap::const_iterator it = m_addressMap.find( addr );
					if ( it == m_addressMap.end() || targetAddress != (*it).second )
					{
						m_addressMap[ addr ] = targetAddress;
						m_isModified = true;
					}
				}
				else
				{
					// remove from map
					TAddressMap::const_iterator it = m_addressMap.find( addr );
					if ( it != m_addressMap.end() )
					{
						m_addressMap.erase( it );
						m_isModified = true;
					}
				}
			}
		}
	}

	bool AddressMap::Save( ILogOutput& log, class IBinaryFileWriter& writer ) const
	{
		CBinaryFileChunkScope chunk( writer, eFileChunk_BranchMap );
		writer << m_addressMap;

		m_isModified = false;
		return true;
	}

	bool AddressMap::Load( ILogOutput& log, class IBinaryFileReader& reader )
	{
		CBinaryFileChunkScope chunk( reader, eFileChunk_BranchMap );
		reader >> m_addressMap;

		m_isModified = false;
		return true;
	}

	//-------------------------------------------------------------------------

} // decoding