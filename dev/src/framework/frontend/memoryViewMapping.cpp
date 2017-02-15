#include "build.h"
#include "memoryView.h"
#include "memoryViewMapping.h"

//---------------------------------------------------------------------------

CLineMemoryMapping::Block::Block()
	: m_firstLine(0) 
	, m_firstByte(0)
	, m_numLines(0)
	, m_numBytes(0)
	, m_numEntries(0)
{
}

CLineMemoryMapping::Entry* CLineMemoryMapping::Block::FindEntryForLine( uint32 lineOffset )
{
	const uint32 index = FindEntryIndexForLine( lineOffset );
	return ( index != INVALID_INDEX ) ? &m_entries[index] : nullptr;
}

const CLineMemoryMapping::Entry* CLineMemoryMapping::Block::FindEntryForLine( uint32 lineOffset ) const
{
	const uint32 index = FindEntryIndexForLine( lineOffset );
	return ( index != INVALID_INDEX ) ? &m_entries[index] : nullptr;
}

CLineMemoryMapping::Entry* CLineMemoryMapping::Block::FindEntryForOffset( uint32 memoryOffset )
{
	const uint32 index = FindEntryIndexForOffset( memoryOffset );
	return ( index != INVALID_INDEX ) ? &m_entries[index] : nullptr;
}

const CLineMemoryMapping::Entry* CLineMemoryMapping::Block::FindEntryForOffset( uint32 memoryOffset ) const
{
	const uint32 index = FindEntryIndexForOffset( memoryOffset );
	return ( index != INVALID_INDEX ) ? &m_entries[index] : nullptr;
}

bool CLineMemoryMapping::Block::Reindex()
{
	// rebuild the local line and memory offsets
	uint32 numLines = 0;
	uint32 numBytes = 0;
	for ( uint32 i=0; i<m_numEntries; ++i )
	{
		Entry& entry = m_entries[i];
		entry.m_line = numLines;
		entry.m_offset = numBytes;
		numLines += entry.m_numLines;
		numBytes += entry.m_numBytes;
	}

	// update counts
	m_numLines = numLines;
	m_numBytes = numBytes;
	return true;
}

const uint32 CLineMemoryMapping::Block::FindEntryIndexForLine( uint32 lineOffset ) const
{
	// binary search, start somewhere in the middle
	uint32 start = 0;
	uint32 stop = m_numEntries;
	while ( start + LINEAR_SEARCH_LIMIT < stop )
	{
		const uint32 mid = (start+stop)/2;
		const Entry& entry = m_entries[mid];

		// smaller or greater ?
		if ( lineOffset < entry.m_line )
		{
			stop = mid;
		}
		else if ( lineOffset >= (uint32)(entry.m_line + entry.m_numLines) )
		{
			start = mid;
		}
		else
		{
			return mid;
		}
	}

	// finish with linear search
	for ( uint32 i=start; i<stop; ++i )
	{
		const Entry& entry = m_entries[i];
		if ( lineOffset >= entry.m_line && lineOffset < (uint32)(entry.m_line + entry.m_numLines) )
		{
			return i;
		}
	}

	// not found
	return INVALID_INDEX;
}

const uint32 CLineMemoryMapping::Block::FindEntryIndexForOffset( uint32 memoryOffset ) const
{
	// binary search, start somewhere in the middle
	uint32 start = 0;
	uint32 stop = m_numEntries;
	while ( start + LINEAR_SEARCH_LIMIT < stop )
	{
		const uint32 mid = (start+stop)/2;
		const Entry& entry = m_entries[mid];

		// smaller or greater ?
		if ( memoryOffset < entry.m_offset )
		{
			stop = mid;
		}
		else if ( memoryOffset >= (uint32)(entry.m_offset + entry.m_numBytes) )
		{
			start = mid;
		}
		else
		{
			return mid;
		}
	}

	// finish with linear search
	for ( uint32 i=start; i<stop; ++i )
	{
		const Entry& entry = m_entries[i];
		if ( memoryOffset >= entry.m_offset && memoryOffset < (uint32)(entry.m_offset + entry.m_numBytes) )
		{
			return i;
		}
	}

	// not found
	return INVALID_INDEX;
}

//---------------------------------------------------------------------------

CLineMemoryMapping::CLineMemoryMapping()
	: m_totalBytes( 0 )
	, m_totalLines( 0 )
{
}

CLineMemoryMapping::~CLineMemoryMapping()
{
	Clear();
}

void CLineMemoryMapping::Clear()
{
	// cleanup the blocks
	for ( uint32 i=0; i<m_blocks.size(); ++i )
	{
		m_poolBlocks.Free( m_blocks[i] );
	}

	// reset
	m_blocks.clear();
	m_totalBytes = 0;
	m_totalLines = 0;
}

void CLineMemoryMapping::Initialize( const class IMemoryDataView& view )
{
	// reset
	Clear();

	// generate the top level blocks and the entries - that's the only structure that has the size related to the mapped memory range
	uint32 currentLine = 0;
	uint32 currentOffset = 0;
	const uint32 memorySize = view.GetLength();
	while ( currentOffset < memorySize )
	{
		// temp entries table
		uint32 numEntries = 0;
		Entry tempEntries[ MAX_LINES_PER_BLOCK+2 ];

		// remember the first line
		const uint32 firstLine = currentLine;
		const uint32 firstByte = currentOffset;

		// fill the block
		uint32 blockLineOffset = 0;
		uint32 blockMemoryOffset = 0;
		while ( currentOffset < memorySize && numEntries < MAX_LINES_PER_BLOCK )
		{
			// get address info
			uint32 numLines=0, numBytes=0;
			view.GetAddressInfo( currentOffset, numLines, numBytes );
			assert( numLines > 0 );
			assert( numBytes > 0 );
			assert( currentOffset + numBytes <= memorySize );

			// create entry
			if ( !numBytes )
			{
				currentOffset += 1;
				continue;
			}

			// size checks
			assert( numLines <= 255 );
			assert( numBytes <= 255 );

			// create entry
			Entry& entry = tempEntries[ numEntries++ ];
			entry.m_numLines = numLines & 0xFF;
			entry.m_numBytes = numBytes & 0xFF;
			entry.m_line = blockLineOffset;
			entry.m_offset = blockMemoryOffset;

			// advance local block offsets
			blockLineOffset += entry.m_numLines;
			blockMemoryOffset += entry.m_numBytes;

			// advance global offsets
			currentOffset += entry.m_numBytes;
			currentLine += entry.m_numLines;
		}

		// add the guard entry (num lines=0, num blocks=0)
		Entry& guardEntry = tempEntries[ numEntries++ ];
		guardEntry.m_line = blockLineOffset;
		guardEntry.m_offset = blockMemoryOffset;
		guardEntry.m_numBytes = 0;
		guardEntry.m_numLines = 0;

		// setup the block
		Block* newBlock = m_poolBlocks.Alloc();
		newBlock->m_firstLine = firstLine;
		newBlock->m_firstByte = firstByte;
		newBlock->m_numBytes = blockMemoryOffset;
		newBlock->m_numLines = blockLineOffset;

		// copy entries
		newBlock->m_numEntries = numEntries;
		assert( sizeof(Entry) * numEntries <= sizeof(newBlock->m_entries) );
		memcpy( &newBlock->m_entries, &tempEntries, sizeof(Entry) * numEntries );

		// add to the block list
		m_blocks.push_back( newBlock );
	}

	// set final values
	m_totalBytes = currentOffset;
	m_totalLines = currentLine;
	assert( m_totalBytes == memorySize );
}

bool CLineMemoryMapping::GetLineForOffset( const uint32 offset, uint32& outLineIndex ) const
{
	const uint32 blockIndex = FindBlockIndexForOffset( offset );
	if ( blockIndex != INVALID_INDEX )
	{
		const Block* block = m_blocks[ blockIndex ];
		const uint32 localIndex = block->FindEntryIndexForOffset( offset - block->m_firstByte );
		if ( localIndex != INVALID_INDEX )
		{
			outLineIndex = block->m_firstLine + block->m_entries[ localIndex ].m_line;
			return true;
		}
	}

	// not found
	return false;
}

bool CLineMemoryMapping::GetOffsetForLine( const uint32 line, uint32& outOffset, uint32& outSize, uint32* firstLine, uint32* numLines ) const
{
	const uint32 blockIndex = FindBlockIndexForLine( line );
	if ( blockIndex != INVALID_INDEX )
	{
		const Block* block = m_blocks[ blockIndex ];
		const uint32 localIndex = block->FindEntryIndexForLine( line - block->m_firstLine );
		if ( localIndex != INVALID_INDEX )
		{
			const Entry& entry = block->m_entries[ localIndex ];
			outOffset = block->m_firstByte + entry.m_offset;
			outSize = entry.m_numBytes;
			if ( firstLine ) *firstLine = block->m_firstLine + entry.m_line;
			if ( numLines ) *numLines = entry.m_numLines;
			return true;
		}
	}

	// not found
	return false;
}

bool CLineMemoryMapping::UpdateLines( const uint32 initialStartOffset, const uint32 initialEndOffset, class IMemoryDataView& data, uint32& outFirstModifiedLine, uint32& outLastModifiedLine )
{
	// start offset larged then end offset
	if ( initialStartOffset > initialEndOffset )
	{
		return false;
	}

	// end offset beyond the memory end
	const uint32 length = data.GetLength();
	if ( initialEndOffset >= length )
	{
		return false;
	}

	// adjust the starting offset
	uint32 startOffset = initialStartOffset;
	{
		uint32 numValidLines=0, numValidBytes=0;
		const uint32 adjustedOffset = data.GetAddressInfo( initialStartOffset, numValidLines, numValidBytes );
		if ( numValidLines == 0 && numValidBytes == 0 )
		{
			startOffset = adjustedOffset;
		}
	}

	// generate entries to be inserted
	std::vector< EntryToInsert > entriesToInsert;
	entriesToInsert.reserve( 2048 );

	// insert stats
	uint32 entriesToInsertTotalLength = 0;
	uint32 sourceToInsertTotalLength = 0;

	// range of modified blocks to cleanup later
	uint32 startBlockIndex;
	uint32 startBlockFirstByte;
	uint32 startBlockFirstLine;
	uint32 finalBlockIndex;

	// reinsert the elements before start offset
	{
		// find the block in which the stat offset is
		startBlockIndex = FindBlockIndexForOffset( startOffset );
		if ( startBlockIndex == INVALID_INDEX )
		{
			return false;
		}

		// get the entry index in start block of the start location
		Block* block = m_blocks[startBlockIndex];
		assert( startOffset >= block->m_firstByte );
		assert( startOffset < block->m_firstByte + block->m_numBytes );
		uint16 localOffset = (uint16)( startOffset - block->m_firstByte );
		const uint16 firstEntry = block->FindEntryIndexForOffset( localOffset );
		assert( firstEntry != INVALID_INDEX );

		// keep this around
		startBlockFirstByte = block->m_firstByte;
		startBlockFirstLine = block->m_firstLine;

		// make sure it's the valid one
		const Entry& firstEntryData = block->m_entries[firstEntry];
		assert( firstEntryData.m_offset <= localOffset );
		assert( localOffset < firstEntryData.m_offset + firstEntryData.m_numBytes );

		// move all of the data to the reentry list
		for ( uint32 i=0; i<firstEntry; ++i )
		{
			const Entry& entry = block->m_entries[i];
			assert( entry.m_offset < localOffset );
			assert( entry.m_numBytes > 0 );

			EntryToInsert entryToInsert;
			entryToInsert.m_numBytes = entry.m_numBytes;
			entryToInsert.m_numLines = entry.m_numLines;
			entriesToInsert.push_back( entryToInsert );
			entriesToInsertTotalLength += entry.m_numBytes;
		}

		// a gap ?
		if ( firstEntryData.m_offset != localOffset )
		{
			const uint16 gapSize = localOffset - firstEntryData.m_offset;

			EntryToInsert gap;
			gap.m_numLines = 0;
			gap.m_numBytes = gapSize;
			entriesToInsert.push_back( gap );
			entriesToInsertTotalLength += gapSize;
			sourceToInsertTotalLength += gapSize;
		}
	}

	// generate the local entries
	uint32 currentOffset = startOffset;
	uint32 localLine = 0;
	uint32 localByte = 0;
	while ( currentOffset < initialEndOffset )
	{
		// get the number of lines and bytes for current entry
		uint32 numValidLines=0, numValidBytes=0;
		const uint32 adjustedOffset = data.GetAddressInfo( currentOffset, numValidLines, numValidBytes );
		assert( adjustedOffset == currentOffset );
		assert( numValidLines > 0 );
		assert( numValidBytes > 0 );

		// create entry info
		EntryToInsert entry;
		entry.m_numBytes = (uint16)numValidBytes;
		entry.m_numLines = (uint16)numValidLines;
		entriesToInsert.push_back( entry );

		// advance
		localLine += numValidLines;
		localByte += numValidBytes;

		// debug
		entriesToInsertTotalLength += numValidBytes;
		sourceToInsertTotalLength += numValidBytes;
		currentOffset += numValidBytes;
	}

	// get the final memory offset up to which we generated the data
	const uint32 lastValidOffset = currentOffset - 1;
	const uint32 totalDataSize = currentOffset - startOffset;

	// extract the elements from the block after the patch position
	{
		// find the block in which the stat offset is
		finalBlockIndex = FindBlockIndexForOffset( lastValidOffset );
		if ( finalBlockIndex == INVALID_INDEX )
		{
			return false;
		}

		// get the entry index in start block of the start location
		Block* block = m_blocks[ finalBlockIndex ];
		assert( lastValidOffset >= block->m_firstByte );
		assert( lastValidOffset < block->m_firstByte + block->m_numBytes );
		uint16 localOffset = (uint16)( lastValidOffset - block->m_firstByte );
		const uint16 finalEntry = block->FindEntryIndexForOffset( localOffset );
		assert( finalEntry != INVALID_INDEX );
		assert( finalEntry < block->m_numEntries-1 ); // should not be the guard entry

		// make sure it's the valid one
		const Entry& finalEntryData = block->m_entries[finalEntry];
		assert( finalEntryData.m_offset <= localOffset );
		assert( localOffset < finalEntryData.m_offset + finalEntryData.m_numBytes );

		// gap ?
		const Entry& firstEntryAfterEnd = block->m_entries[ finalEntry+1 ];
		if ( firstEntryAfterEnd.m_offset > (localOffset+1) ) 
		{
			const uint16 gapSize = (firstEntryAfterEnd.m_offset - localOffset) + 1;

			// add gap
			EntryToInsert entryToInsert;
			entryToInsert.m_numBytes = gapSize;
			entryToInsert.m_numLines = 0;
			entriesToInsert.push_back( entryToInsert );

			// debug
			entriesToInsertTotalLength += gapSize;
			sourceToInsertTotalLength += gapSize;
		}

		// copy rest of the entries, skipping the guard
		for ( uint32 i = finalEntry+1 ; i<block->m_numEntries-1; ++i )
		{
			const Entry& entry = block->m_entries[i];
			assert( entry.m_numBytes > 0 );
			assert( entry.m_offset > localOffset );

			EntryToInsert entryToInsert;
			entryToInsert.m_numBytes = entry.m_numBytes;
			entryToInsert.m_numLines = entry.m_numLines;
			entriesToInsert.push_back( entryToInsert );
			entriesToInsertTotalLength += entry.m_numBytes;
		}
	}

	// validate that we got teh same length of source and destination data
	if ( sourceToInsertTotalLength != totalDataSize )
	{
		return false;
	}

	// last sanity check - make sure that the amount of memory we will reinsert matches the one removed
	uint32 totalMemoryInRemovedBlocks = 0;
	for ( uint32 i=startBlockIndex; i<=finalBlockIndex; ++i )
	{
		const Block& block = *m_blocks[ i ];
		totalMemoryInRemovedBlocks += block.m_numBytes;
	}

	// validate final memory size
	if ( totalMemoryInRemovedBlocks != entriesToInsertTotalLength )
	{
		return false;
	}

	// unlink all the block between start and finish location
	for ( uint32 i=startBlockIndex; i<=finalBlockIndex; ++i )
	{
		Block* freeBlock = m_blocks[ startBlockIndex ];
		m_blocks.erase( m_blocks.begin() + startBlockIndex );
		m_poolBlocks.Free( freeBlock );
		totalMemoryInRemovedBlocks += freeBlock->m_numBytes;
	}

	// start rebuilding the blocks, keeping the block entry length limit
	uint32 insertEntryIndex = 0;
	uint32 numBlocksInserted = 0;
	uint32 currentBlockLine = startBlockFirstLine;
	uint32 currentBlockByte = startBlockFirstByte;
	while ( insertEntryIndex < entriesToInsert.size() )
	{
		// get the new block
		Block* newBlock = m_poolBlocks.Alloc();

		// set the block placement
		newBlock->m_numEntries = 0;
		newBlock->m_firstByte = currentBlockByte;
		newBlock->m_firstLine = currentBlockLine;

		// add the entries
		uint16 localByte = 0;
		uint16 localLine = 0;
		while ( insertEntryIndex < entriesToInsert.size() && newBlock->m_numEntries < MAX_LINES_PER_BLOCK )
		{
			const EntryToInsert& toInsert = entriesToInsert[ insertEntryIndex ];

			// add to block
			Entry& newEntry = newBlock->m_entries[ newBlock->m_numEntries++ ];
			newEntry.m_line = localLine;
			newEntry.m_offset = localByte;
			newEntry.m_numLines = toInsert.m_numLines;
			newEntry.m_numBytes = toInsert.m_numBytes;

			// advance
			localByte += toInsert.m_numBytes;
			localLine += toInsert.m_numLines;
			insertEntryIndex += 1;
		}

		// add the guard
		{
			Entry& guardEntry = newBlock->m_entries[ newBlock->m_numEntries++ ];
			guardEntry.m_line = localLine;
			guardEntry.m_offset = localByte;
			guardEntry.m_numLines = 0;
			guardEntry.m_numBytes = 0;
		}

		// update the block sizes
		newBlock->m_numLines = localLine;
		newBlock->m_numBytes = localByte;

		// advance the block
		currentBlockLine += localLine;
		currentBlockByte += localByte;

		// insert the block back in to the big block lists
		m_blocks.insert( m_blocks.begin() + startBlockIndex + numBlocksInserted, newBlock );
		numBlocksInserted += 1;
	}

	// rebind all of the blocks following the last block that was inserted
	const uint32 lastInsertedBlock = startBlockIndex + numBlocksInserted;
	for ( uint32 i=lastInsertedBlock; i<m_blocks.size(); ++i )
	{
		Block* block = m_blocks[i];

		assert( block->m_firstByte == currentBlockByte );
		block->m_firstLine = currentBlockLine;

		currentBlockLine += block->m_numLines;
		currentBlockByte += block->m_numBytes;
	}

	// update total line count
	assert( currentBlockByte == m_totalBytes );
	m_totalLines = currentBlockLine;

	// Validate that the internal structure is OK
	Validate();

	// done
	return true;
}

const uint32 CLineMemoryMapping::FindBlockIndexForLine( uint32 lineOffset ) const
{
	// for small lists use linear search
	const uint32 numEntries = m_blocks.size();

	// binary search, start somewhere in the middle
	uint32 start = 0;
	uint32 stop = numEntries;
	while ( start + LINEAR_SEARCH_LIMIT < stop )
	{
		const uint32 mid = (start+stop)/2;
		const Block& entry = *m_blocks[mid];

		// smaller or greater ?
		if ( lineOffset < entry.m_firstLine )
		{
			stop = mid;
		}
		else if ( lineOffset >= entry.m_firstLine + entry.m_numLines )
		{
			start = mid;
		}
		else
		{
			return mid;
		}
	}

	// finish with linear search
	for ( uint32 i=start; i<stop; ++i )
	{
		const Block& entry = *m_blocks[i];
		if ( lineOffset >= entry.m_firstLine && lineOffset < entry.m_firstLine + entry.m_numLines )
		{
			return i;
		}
	}

	// not found
	return INVALID_INDEX;
}

const uint32 CLineMemoryMapping::FindBlockIndexForOffset( uint32 memoryOffset ) const
{
	// linear search
	// TODO: optimize
	for ( uint32 i=0; i<m_blocks.size(); ++i )
	{
		const Block* block = m_blocks[i];
		if ( memoryOffset >= block->m_firstByte )
		{
			if ( memoryOffset < block->m_firstByte + block->m_numBytes )
			{
				return i;
			}
		}
	}

	// not found
	assert( memoryOffset >= m_totalBytes );
	return INVALID_INDEX;
}

void CLineMemoryMapping::Validate() const
{
	uint32 currentLine = 0;
	uint32 currentMemory = 0;
	for ( uint32 i=0; i<m_blocks.size(); ++i )
	{
		const Block* block = m_blocks[i];
		assert( block->m_firstLine == currentLine );
		assert( block->m_firstByte == currentMemory );

		// check entries
		uint32 blockLine = 0;
		uint32 blockMemory = 0;
		assert( block->m_numEntries > 1 );
		for ( uint32 j=0; j<block->m_numEntries-1; ++j )
		{
			const Entry& entry = block->m_entries[j];
			assert( entry.m_numBytes > 0 );
			assert( entry.m_numLines > 0 );
			assert( entry.m_offset == blockMemory );
			assert( entry.m_line == blockLine );
			blockMemory += entry.m_numBytes;
			blockLine += entry.m_numLines;
		}

		// guard entry
		const Entry& guardEntry = block->m_entries[ block->m_numEntries-1 ];
		assert( guardEntry.m_numBytes == 0 );
		assert( guardEntry.m_numLines == 0 );
		assert( guardEntry.m_line == blockLine );
		assert( guardEntry.m_offset == blockMemory );

		// advance
		currentLine += blockLine;
		currentMemory += blockMemory;
	}

	// final size assert
	assert( m_totalLines == currentLine );
	assert( m_totalBytes == currentMemory );
}

