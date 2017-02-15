#include "build.h"
#include "internalUtils.h"
#include "internalFile.h"

//---------------------------------------------------------------------------

/// ANSI C file writer implementation
class NativeFileWriter : public IBinaryFileWriter
{
private:
	FILE*		m_file;
	bool		m_error;

	std::vector< uint32 >		m_blockOffsets;
	
public:
	NativeFileWriter( FILE* file )
		: m_file( file )
		, m_error( false )
	{}

	virtual ~NativeFileWriter() override
	{
		if ( m_file != NULL )
		{
			fclose( m_file );
			m_file = NULL;
		}
	}

	virtual uint64 GetSize() const override
	{
		return ftell( m_file );
	}

	virtual void BeginBlock( const uint16 chunkMagic )
	{
		if ( !m_error )
		{
			// save the chunk magic (so we know what we are saving)
			Save( &chunkMagic, sizeof(chunkMagic) );

			// remember current file offset
			const uint32 currentOffset = ftell( m_file );
			m_blockOffsets.push_back( currentOffset );

			// create the placeholder for skip offset
			const uint32 skipSize = 0;
			Save( &skipSize, sizeof(skipSize) );
		}
	}

	virtual void FinishBlock()
	{
		if ( m_blockOffsets.empty() )
		{
			return;
		}

		if ( !m_error )
		{
			// get the block skip offset position
			const uint32 backOffset = m_blockOffsets.back();
			m_blockOffsets.pop_back();

			// save the skip length
			const uint32 currentOffset = ftell( m_file );
			const uint32 skipLength = currentOffset - backOffset;
			fseek( m_file, backOffset, SEEK_SET );
			Save( &skipLength, sizeof(skipLength) );
			fseek( m_file, currentOffset, SEEK_SET );
		}
	}

	virtual void Save( const void* data, const uint32 size ) override
	{
		if ( !m_error )
		{
			if ( 1 != fwrite( data, size, 1, m_file ) )
			{
				fclose( m_file );
				m_file = NULL;
				m_error = true;
			}
		}
	}
};

bool CreateFilePath( const wchar_t* path )
{
	// make sure path exists
	wchar_t tempPath[ MAX_PATH ];
	wcscpy_s( tempPath, path );
	wchar_t* cur = tempPath;
	while ( *cur != 0 )
	{
		if( *cur == '\\' || *cur == '/' )
		{
			if ( cur-tempPath > 3 )
			{
				*cur = 0;
				if ( !CreateDirectoryW( tempPath, NULL ) )
				{
					if ( GetLastError() != ERROR_ALREADY_EXISTS )
					{
						return false;
					}
				}
				*cur = '\\';
			}
		}

		++cur;
	}

	// path created
	return true;
}

IBinaryFileWriter* CreateFileWriter( const wchar_t* path, const bool compressed /*= false*/ )
{
	// create path
	if ( !CreateFilePath( path ) )
	{
		return nullptr;
	}

	// open file
	FILE* f = NULL;
	_wfopen_s( &f, path, L"wb" );
	if ( !f )
	{
		return NULL;
	}

	// create wrapper
	return new NativeFileWriter( f );
}

//---------------------------------------------------------------------------

/// ANSI C file reader implementation
class NativeFileReader : public IBinaryFileReader
{
private:
	FILE*		m_file;
	bool		m_error;

	std::vector< uint32 >	m_currentBlockEnd;

public:
	NativeFileReader( FILE* file )
		: m_file( file )
		, m_error( false )
	{}

	virtual ~NativeFileReader() override
	{
		if ( m_file != NULL )
		{
			fclose( m_file );
			m_file = NULL;
		}
	}

	virtual void Load( void* data, const uint32 size ) override
	{
		if ( !m_error )
		{
			if ( 1 != fread( data, size, 1, m_file ) )
			{
				fclose( m_file );
				m_file = NULL;
				m_error = true;
			}
		}
	}

	virtual bool EnterBlock( const uint16 chunkMagic )
	{
		// read the block magic
		uint16 magic = 0;
		Load( &magic, sizeof( magic ) );

		// read the block skip offset
		uint32 skipOffset = 0;
		Load( &skipOffset, sizeof(skipOffset) );

		// in error state :( (EOF)
		if ( m_error )
		{
			return false;
		}

		// invalid chunk type - skip it
		if ( magic != chunkMagic )
		{
			fseek( m_file, skipOffset, SEEK_CUR );
			return false;
		}

		// compute final position
		const uint32 currentOffset = ftell( m_file );
		const uint32 finalOffset = currentOffset + skipOffset - sizeof(uint32);
		m_currentBlockEnd.push_back( finalOffset );

		// allowed to enter the block
		return true;
	}

	virtual void LeaveBlock()
	{
		if ( m_currentBlockEnd.empty() )
		{
			m_error = true;
			return;
		}

		// go to the end position of block
		const uint32 endPos = m_currentBlockEnd.back();
		m_currentBlockEnd.pop_back();
		fseek( m_file, endPos, SEEK_SET );
	}

	virtual bool CheckBlock( const uint16 chunkMagic )
	{
		if ( m_error )
		{
			return false;
		}

		uint16 magic = 0;
		Load( &magic, sizeof(magic) );
		fseek( m_file, -(int)sizeof(uint32), SEEK_CUR );

		return (magic == chunkMagic);
	}
};

IBinaryFileReader* CreateFileReader( const wchar_t* path )
{
	// open file
	FILE* f = NULL;
	_wfopen_s( &f, path, L"rb" );
	if ( !f )
	{
		return NULL;
	}

	// create wrapper
	return new NativeFileReader( f );
}

extern bool FileExists( const wchar_t* path )
{
	// open file
	FILE* f = NULL;
	_wfopen_s( &f, path, L"rb" );
	if ( f )
	{
		fclose(f);
		return true;
	}

	// does not exist
	return false;
}

//---------------------------------------------------------------------------