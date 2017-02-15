#pragma once

//-----------------------------------------------------------------------------

/// Line mapping buffer
/// Offsers mapping from lines to memory offsets
/// Each line can have a length (number of bytes) set
class CLineMemoryMapping
{
private:
	static const uint32 MAX_LINES_PER_BLOCK	= 200;
	static const uint32 LINEAR_SEARCH_LIMIT	= 10;
	static const uint32 INVALID_INDEX = 0xFFFFFFFF;

	// entry - for physical line being displayed
	struct Entry
	{
		uint16	m_offset;		// memory offset, relative to the block start
		uint16	m_line;			// line index, relative to the block start
		uint16	m_numBytes;		// number of bytes in block
		uint16	m_numLines;		// number of lines in block
	};

	// internal pool
	template< class T >
	class TObjectPool
	{
	private:
		std::vector< T* >		m_freeList;

	public:
		~TObjectPool()
		{
			for ( uint32 i=0; i<m_freeList.size(); ++i )
			{
				delete m_freeList[i];
			}
		}

		inline T* Alloc()
		{
			if ( m_freeList.empty() )
			{
				return new T();
			}
			else
			{
				T* ret = m_freeList.back();
				m_freeList.pop_back();
				return ret;
			}
		}

		void Free( T* obj )
		{
			m_freeList.push_back( obj );
		}
	};

	// entry to insert
	struct EntryToInsert
	{
		uint16		m_numBytes;
		uint16		m_numLines;
	};

	// block of lines
	class Block
	{
	public:
		uint32		m_firstLine;
		uint32		m_firstByte;
		uint32		m_numLines;
		uint32		m_numBytes;

		Entry		m_entries[ MAX_LINES_PER_BLOCK+1 ];
		uint32		m_numEntries;

	public:
		Block();

		// find local entry for given line number (binary search)
		Entry* FindEntryForLine( uint32 lineOffset );
		const Entry* FindEntryForLine( uint32 lineOffset ) const;
		const uint32 FindEntryIndexForLine( uint32 lineOffset ) const;
	
		// find local entry for given memory offset (binary search)
		Entry* FindEntryForOffset( uint32 memoryOffset );
		const Entry* FindEntryForOffset( uint32 memoryOffset ) const;
		const uint32 FindEntryIndexForOffset( uint32 memoryOffset ) const;
	
		// reindex the entry ofsets, recompute the total lines and total bytes, returns true if the block sizes were updated
		bool Reindex();
	};

	// ordered blocks
	typedef std::vector< Block* >		TBlocks;
	TBlocks		m_blocks;

	// memory pools
	TObjectPool< Block >		m_poolBlocks;

	// number of all of the lines and totam memory size
	uint32		m_totalBytes;
	uint32		m_totalLines;

public:
	// size of mapped memory range
	inline const uint32	GetNumberOfBytes() const { return m_totalBytes; }

	// current number of lines
	inline const uint32 GetNumberOfLines() const { return m_totalLines; }

public:
	CLineMemoryMapping();
	~CLineMemoryMapping();

	//! Clear the mapping (num of lines = 0)
	void Clear();

	//! Initialize mapping for given memory range, you need to supply a class that will specify current memory size for each line
	void Initialize( const class IMemoryDataView& view );

	//! Get the line index for offset
	bool GetLineForOffset( const uint32 offset, uint32& outLineIndex ) const;

	//! Get the offset for given line
	bool GetOffsetForLine( const uint32 line, uint32& outOffset, uint32& outSize, uint32* firstLine = nullptr, uint32* numLines = nullptr ) const;

	//! Update the line sizes starting from given offset, offsets may be adjusted internally, returns index of the first and last line modified
	bool UpdateLines( const uint32 startOffset, const uint32 endOffset, class IMemoryDataView& data, uint32& outFirstModifiedLine, uint32& outLastModifiedLine );

private:
	//! Validate the content
	void Validate() const;

	//! Find the block index for given line
	const uint32 FindBlockIndexForLine( uint32 lineOffset ) const;

	//! Find the block index for given memory
	const uint32 FindBlockIndexForOffset( uint32 memoryOffset ) const;
};


