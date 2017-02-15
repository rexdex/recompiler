#pragma once

//---------------------------------------------------------------------------

/// Chunk map
enum EFileChunk
{
	eFileChunk_File = 0xDADA,
	eFileChunk_Data = 0xDADA,
	eFileChunk_Property = 0x90D5,
	eFileChunk_PropertyList = 0xAB18,

	eFileChunk_ImageSection = 0x6501,
	eFileChunk_ImageExport = 0x6502,
	eFileChunk_ImageImport = 0x6503,
	eFileChunk_ImageInfo = 0x6504,
	eFileChunk_ImageSymbol = 0x6507,
	eFileChunk_ImageMemory = 0x6817,

	eFileChunk_MemoryMap = 0xFAAB,
	eFileChunk_MemoryMapData = 0xFAAC,

	eFileChunk_CommentMap = 0xCEE3,

	eFileChunk_BranchMap = 0xBAA4,

	eFileChunk_NameMap = 0xCDA5,

	eFileChunk_TraceCallstack = 0xC488,
	eFileChunk_TraceCallstackFrame = 0xC489,
};

//---------------------------------------------------------------------------

/// Binary file writer, used to save the project data
class IBinaryFileWriter
{
public:
	virtual ~IBinaryFileWriter() {};

	// save raw data
	virtual void Save( const void* data, const uint32 size ) = 0;

	// get file size
	virtual uint64 GetSize() const = 0;

	// skipable block support
	virtual void BeginBlock( const uint16 chunkMagic ) = 0;
	virtual void FinishBlock() = 0;

	// stream operators
	inline IBinaryFileWriter& operator<<( const uint8& data ) { Save( &data, sizeof(data) ); return *this; }
	inline IBinaryFileWriter& operator<<( const uint16& data ) { Save( &data, sizeof(data) ); return *this; }
	inline IBinaryFileWriter& operator<<( const uint32& data ) { Save( &data, sizeof(data) ); return *this; }
	inline IBinaryFileWriter& operator<<( const uint64& data ) { Save( &data, sizeof(data) ); return *this; }
	inline IBinaryFileWriter& operator<<( const int8& data ) { Save( &data, sizeof(data) ); return *this; }
	inline IBinaryFileWriter& operator<<( const int16& data ) { Save( &data, sizeof(data) ); return *this; }
	inline IBinaryFileWriter& operator<<( const int32& data ) { Save( &data, sizeof(data) ); return *this; }
	inline IBinaryFileWriter& operator<<( const int64& data ) { Save( &data, sizeof(data) ); return *this; }

	// save boolean 
	inline IBinaryFileWriter& operator<<( const bool& data )
	{
		uint8 val = data ? 1 : 0;
		Save( &val, sizeof(val) );
		return *this;
	}

	// save string
	inline IBinaryFileWriter& operator<<( const std::string& data )
	{
		const uint32 len = (uint32)data.length();
		Save( &len, sizeof(len) );
		if ( len > 0 )
		{
			Save( &data[0], sizeof(data[0]) * len );
		}
		return *this;
	}

	// save wstring
	inline IBinaryFileWriter& operator<<( const std::wstring& data )
	{
		const uint32 len = (uint32)data.length();
		Save( &len, sizeof(len) );
		if ( len > 0 )
		{
			Save( &data[0], sizeof(data[0]) * len );
		}
		return *this;
	}

	// save vector of pointers
	template< typename T >
	inline IBinaryFileWriter& operator<<( const std::vector<T*>& data )
	{
		uint32 len = (uint32)data.size();
		Save( &len, sizeof(len) );
		if ( len > 0 )
		{
			for ( uint32 i=0; i<data.size(); ++i )
			{
				T* ptr = data[i];
				if ( ptr != NULL )
				{
					*this << ptr;
				}
			}
		}
		return *this;
	}
	// save plain vector
	template< typename T >
	inline IBinaryFileWriter& operator<<( const std::vector<T>& data )
	{
		uint32 len = (uint32)data.size();
		Save( &len, sizeof(len) );
		if ( len > 0 )
		{
			for ( uint32 i=0; i<data.size(); ++i )
			{
				*this << data[i];
			}
		}
		return *this;
	}

	// save plain map
	template< typename K, typename V >
	inline IBinaryFileWriter& operator<<( const std::map<K,V>& data )
	{
		const uint32 len = (uint32)data.size();
		Save( &len, sizeof(len) );
		if ( len > 0 )
		{
			for ( std::map<K,V>::const_iterator it = data.begin();
				it != data.end(); ++it )
			{
				*this << (*it).first;
				*this << (*it).second;
			}
		}
		return *this;
	}

	// save pointer
	template< typename T >
	inline IBinaryFileWriter& operator<<( T* data )
	{
		// NULL/ NotNull flag
		const uint8 flag = data != 0;
		Save( &flag, sizeof(flag) );

		// object data
		if ( flag )
		{
			data->Save( *this );
		}

		return *this;
	}
};

//---------------------------------------------------------------------------

/// Binary file reader, used to load the project data
class IBinaryFileReader
{
public:
	virtual ~IBinaryFileReader() {};

	// load raw data
	virtual void Load( void* data, const uint32 size ) = 0;

	// skipable block support
	virtual bool EnterBlock( const uint16 chunkMagic ) = 0;
	virtual void LeaveBlock() = 0;

	// check chunk existence
	virtual bool CheckBlock( const uint16 chunkMagic ) = 0;

	// stream operators
	inline IBinaryFileReader& operator>>( uint8& data ) { Load( &data, sizeof(data) ); return *this; }
	inline IBinaryFileReader& operator>>( uint16& data ) { Load( &data, sizeof(data) ); return *this; }
	inline IBinaryFileReader& operator>>( uint32& data ) { Load( &data, sizeof(data) ); return *this; }
	inline IBinaryFileReader& operator>>( uint64& data ) { Load( &data, sizeof(data) ); return *this; }
	inline IBinaryFileReader& operator>>( int8& data ) { Load( &data, sizeof(data) ); return *this; }
	inline IBinaryFileReader& operator>>( int16& data ) { Load( &data, sizeof(data) ); return *this; }
	inline IBinaryFileReader& operator>>( int32& data ) { Load( &data, sizeof(data) ); return *this; }
	inline IBinaryFileReader& operator>>( int64& data ) { Load( &data, sizeof(data) ); return *this; }

	// load boolean 
	inline IBinaryFileReader& operator>>( bool& data )
	{
		uint8 val = 0;
		Load( &val, sizeof(val) );
		data = (val != 0);
		return *this;
	}

	// load string
	inline IBinaryFileReader& operator>>( std::string& data )
	{
		uint32 len = 0;
		Load( &len, sizeof(len) );
		if ( len > 0 )
		{
			if ( len < 1024 )
			{
#pragma warning(push)
#pragma warning(disable: 6255)
				char* str = (char*)alloca( len+1 );
#pragma warning(pop)
				Load( str, len );
				str[len] = 0;
				data = str;
			}
			else
			{
				char* str = (char*)malloc(len + 1);
				if (str)
				{
					Load(str, len);
					str[len] = 0;
					data = str;
					free(str);
				}
			}
		}
		else
		{
			data = "";
		}
		return *this;
	}

	// load wstring
	inline IBinaryFileReader& operator>>( std::wstring& data )
	{
		uint32 len = 0;
		Load( &len, sizeof(len) );
		if ( len > 0 )
		{
			if ( len < 1024 )
			{
#pragma warning(push)
#pragma warning(disable: 6255)
				wchar_t* str = (wchar_t*)alloca( sizeof(wchar_t)*(len+1) );
#pragma warning(pop)
				Load( str, sizeof(wchar_t)*len );
				str[len] = 0;
				data = str;
			}
			else
			{
				wchar_t* str = (wchar_t*)malloc( sizeof(wchar_t)*(len+1) );
				if (str)
				{
					Load(str, sizeof(wchar_t)*len);
					str[len] = 0;
					data = str;
					free(str);
				}
			}
		}
		else
		{
			data = L"";
		}
		return *this;
	}

	// load vector of pointers
	template< typename T >
	inline IBinaryFileReader& operator>>( std::vector<T*>& data )
	{
		uint32 len = 0;
		Load( &len, sizeof(len) );
		for ( uint32 i=0; i<len; ++i )
		{
			T* ptr = NULL;
			*this >> ptr;

			if ( ptr != NULL )
			{
				data.push_back( ptr );
			}
		}
		return *this;
	}

	// load plain vector
	template< typename T >
	inline IBinaryFileReader& operator>>( std::vector<T>& data )
	{
		uint32 len = 0;
		Load( &len, sizeof(len) );
		data.resize( len );
		for ( uint32 i=0; i<len; ++i )
		{
			*this >> data[i];
		}
		return *this;
	}

	// load plain map
	template< typename K, typename V >
	inline IBinaryFileReader& operator>>( std::map<K,V>& data )
	{
		uint32 len = 0;
		Load( &len, sizeof(len) );
		for ( uint32 i=0; i<len; ++i )
		{
			K key;
			V value;
			*this >> key;
			*this >> value;
			data[key] = value;
		}
		return *this;
	}

	// load pointer
	template< typename T >
	inline IBinaryFileReader& operator>>( T*& data )
	{
		// load the NULL/NotNULL flag
		uint8 flag = 0;
		Load( &flag, sizeof(flag) );

		// create the object
		if ( flag )
		{
			T* ptr = new T();
			ptr->Load( *this );
			data = ptr;
		}
		else
		{
			data = NULL;
		}

		return *this;
	}
};

//---------------------------------------------------------------------------

/// Ensure that the path exists
extern bool CreateFilePath( const wchar_t* path );

/// Create interface for writing to binary files
extern IBinaryFileWriter* CreateFileWriter( const wchar_t* path, const bool compressed = false );

/// Create interface for reading form binary files
extern IBinaryFileReader* CreateFileReader( const wchar_t* path );

/// Check if file exists
extern bool FileExists( const wchar_t* path );

//---------------------------------------------------------------------------

/// Chunk scope
class CBinaryFileChunkScope
{
private:
	EFileChunk				m_chunkType;
	IBinaryFileWriter*		m_writer;
	IBinaryFileReader*		m_reader;
	bool					m_valid;

public:
	CBinaryFileChunkScope( IBinaryFileWriter& writer, const EFileChunk chunkType )
		: m_writer( &writer )
		, m_reader( NULL )
		, m_chunkType( chunkType )
	{
		writer.BeginBlock( chunkType );
		m_valid = true;
	}

	CBinaryFileChunkScope( IBinaryFileReader& reader, const EFileChunk chunkType )
		: m_writer( NULL )
		, m_reader( &reader )
		, m_chunkType( chunkType )
	{
		m_valid = reader.EnterBlock( chunkType );
	}

	~CBinaryFileChunkScope()
	{
		if ( m_writer != NULL )
		{
			m_writer->FinishBlock();
		}

		if ( m_reader != NULL )
		{
			m_reader->LeaveBlock();
		}
	}

	inline operator bool() const
	{
		return m_valid;
	}
};

//---------------------------------------------------------------------------
