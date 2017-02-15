#include "build.h"
#include "internalUtils.h"
#include "zlib\zlib.h"

//---------------------------------------------------------------------------

ScanDirectoryA::ScanDirectoryA( const char* searchPath )
	: m_first( true )
{
	memset( &m_data, 0, sizeof(m_data) );
	m_hFind = FindFirstFileA( searchPath, &m_data );
}

ScanDirectoryA::~ScanDirectoryA()
{
	if ( m_hFind != INVALID_HANDLE_VALUE )
	{
		FindClose( m_hFind );
		m_hFind = INVALID_HANDLE_VALUE;
	}
}

bool ScanDirectoryA::Iterate()
{
	if ( !m_first )
	{
		if ( !FindNextFileA( m_hFind, &m_data ) )
		{
			FindClose( m_hFind );
			m_hFind = INVALID_HANDLE_VALUE;

			return false;
		}
	}

	m_first = false;

	return (m_hFind != INVALID_HANDLE_VALUE);
}

bool ScanDirectoryA::IsFile() const
{
	return 0 == (m_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
}

bool ScanDirectoryA::IsDirectory() const
{
	return 0 != (m_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
}

const char* ScanDirectoryA::GetFileName() const
{
	return m_data.cFileName;
}

//---------------------------------------------------------------------------

ScanDirectoryW::ScanDirectoryW( const wchar_t* searchPath )
	: m_first( true )
{
	memset( &m_data, 0, sizeof(m_data) );
	m_hFind = FindFirstFileW( searchPath, &m_data );
}

ScanDirectoryW::~ScanDirectoryW()
{
	if ( m_hFind != INVALID_HANDLE_VALUE )
	{
		FindClose( m_hFind );
		m_hFind = INVALID_HANDLE_VALUE;
	}
}

bool ScanDirectoryW::Iterate()
{
	if ( !m_first )
	{
		if ( !FindNextFileW( m_hFind, &m_data ) )
		{
			FindClose( m_hFind );
			m_hFind = INVALID_HANDLE_VALUE;

			return false;
		}
	}

	m_first = false;

	return (m_hFind != INVALID_HANDLE_VALUE);
}

bool ScanDirectoryW::IsFile() const
{
	return 0 == (m_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
}

bool ScanDirectoryW::IsDirectory() const
{
	return 0 != (m_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
}

const wchar_t* ScanDirectoryW::GetFileName() const
{
	return m_data.cFileName;
}

//---------------------------------------------------------------------------

void FixFilePathA( const char* filePath, const char* newExtension, std::string& outDirectory, std::string& outFilePath )
{
	// find the file division
	const char* filePart = strrchr( filePath, '\\' );
	const char* filePart2 = strrchr( filePath, '/' );
	filePart = (filePart > filePart2) ? filePart : filePart2;

	// get the directory
	if ( NULL != filePart )
	{
		outDirectory = std::string(filePath, filePart-filePath);
	}
	else
	{
		outDirectory = "";
	}
	
	// get the file extension
	const char* fileExt = strrchr( filePart, '.' );
	if ( fileExt )
	{
		outFilePath = std::string(filePath, fileExt-filePath);
	}
	else
	{
		outFilePath = filePath;
	}

	// append new extension
	if ( newExtension && newExtension[0] )
	{
		outFilePath += ".";
		outFilePath += newExtension;
	}
}

//---------------------------------------------------------------------------

void FixFilePathW( const wchar_t* filePath, const wchar_t* newExtension, std::wstring& outDirectory, std::wstring& outFilePath )
{
	// find the file division
	const wchar_t* filePart = wcsrchr( filePath, '\\' );
	const wchar_t* filePart2 = wcsrchr( filePath, '/' );
	filePart = (filePart > filePart2) ? filePart : filePart2;

	// get the directory
	if ( NULL != filePart )
	{
		outDirectory = std::wstring(filePath, filePart-filePath);
		outDirectory += L"\\";
	}
	else
	{
		outDirectory = L"\\";
	}
	
	// get the file extension
	const wchar_t* fileExt = wcsrchr( filePart, '.' );
	if ( fileExt )
	{
		outFilePath = std::wstring(filePath, fileExt-filePath);
	}
	else
	{
		outFilePath = filePath;
	}

	// append new extension
	if ( newExtension && newExtension[0] )
	{
		outFilePath += L".";
		outFilePath += newExtension;
	}
}

//---------------------------------------------------------------------------

void ExtractDirW( const wchar_t* filePath, std::wstring& outDirectory )
{
	// find the file division
	const wchar_t* filePart = wcsrchr( filePath, '\\' );
	const wchar_t* filePart2 = wcsrchr( filePath, '/' );
	filePart = (filePart > filePart2) ? filePart : filePart2;

	// get the directory
	if ( NULL != filePart )
	{
		outDirectory = std::wstring(filePath, filePart-filePath);
		outDirectory += L"\\";
	}
	else
	{
		outDirectory = L"\\";
	}
}

//---------------------------------------------------------------------------

bool SaveStringToFileA( const wchar_t* filePath, const std::string& text )
{
	FILE* f = NULL;
	_wfopen_s( &f, filePath, L"wb" );
	if ( !f ) return false;

	fwrite( text.c_str(), text.length(), 1, f );
	fclose( f );

	return true;
}

bool SaveStringToFileW( const wchar_t* filePath, const std::wstring& text )
{
	FILE* f = NULL;
	_wfopen_s( &f, filePath, L"wb" );
	if ( !f ) return false;

	uint8 prefix[2] = { 0xFF, 0xFE };
	fwrite( prefix, 2, 1, f );

	fwrite( text.c_str(), text.length(), 1, f );
	fclose( f );

	return true;
}

//---------------------------------------------------------------------------

CBinaryBigBuffer::CBinaryBigBuffer()
	: m_curPage(NULL)
	, m_size(0)
	, m_lastReadPage(NULL)
	, m_lastReadPageStart(0)
	, m_lastReadPageEnd(0)
{
}

CBinaryBigBuffer::~CBinaryBigBuffer()
{
	Clear();
}

void CBinaryBigBuffer::Clear()
{
	for (uint32 i=0; i<m_pages.size(); ++i)
	{
		free(m_pages[i].m_mem);
	}

	m_pages.clear();
	m_curPage = NULL;
	m_size = 0;

	m_lastReadPage = NULL;
	m_lastReadPageStart = 0;
	m_lastReadPageEnd = 0;
}

const bool CBinaryBigBuffer::Append(const void* data, const uint32 size)
{
	uint32 left = size;
	const uint8* readPtr = (const uint8*) data;
	while (left > 0)
	{
		// out of pages
		const uint32 leftInPage = m_curPage ? (PAGE_SIZE - m_curPage->m_used) : 0;
		if (!leftInPage)
		{
			// allocate new page memory
			const void* mem = malloc(PAGE_SIZE);
			if (!mem)
				return false;

			// allocate new page
			Page info;
			info.m_mem = (uint8*) mem;
			info.m_used = 0;
			m_pages.push_back(info);
			m_curPage = &m_pages.back();
			m_lastReadPage = NULL;
			m_lastReadPageStart = 0;
			m_lastReadPageEnd = 0;
			continue;
		}

		// copy as much as we can in one batch
		const uint32 sizeToCopy = Min<uint32>( leftInPage, left );
		memcpy( m_curPage->m_mem + m_curPage->m_used, readPtr, sizeToCopy );

		// advance
		left -= sizeToCopy;
		readPtr += sizeToCopy;
		m_size += sizeToCopy;
		m_curPage->m_used += sizeToCopy;
	}

	// all data copied
	return true;
}

const bool CBinaryBigBuffer::CopyData(void* outData, const uint64 outDataSize) const
{
	uint8* writePtr = (uint8*) outData;
	uint64 writtenSoFar = 0;
	for (uint32 i=0; i<m_pages.size(); ++i)
	{
		const Page& page = m_pages[i];

		if (writtenSoFar + page.m_used > outDataSize)
			return false;

		memcpy( writePtr, page.m_mem, page.m_used );
		writtenSoFar += page.m_used;
		writePtr += page.m_used;
	}

	// all data copied
	return true;
}

const uint8 CBinaryBigBuffer::GetByte(const uint64 position) const
{
	if (position < m_lastReadPageStart || position >= m_lastReadPageEnd)
	{
		// find page
		m_lastReadPage = NULL;
		uint64 currentPosition = 0;
		for (uint32 i=0; i<m_pages.size(); ++i)
		{
			const Page& page = m_pages[i];

			const uint64 pageStart = currentPosition;
			const uint64 pageEnd = currentPosition + page.m_used;
			if (position >= pageStart && position < pageEnd)
			{
				m_lastReadPage = &page;
				m_lastReadPageStart = pageStart;
				m_lastReadPageEnd = pageEnd;
				break;
			}

			currentPosition += page.m_used;
		}
	}
	
	// no page found
	if (!m_lastReadPage)
		return 0;

	// get data
	return m_lastReadPage->m_mem[ position - m_lastReadPageStart ];
}

//---------------------------------------------------------------------------

CBinaryConsumer::CBinaryConsumer(const void* data, const uint32 size)
	: m_data((const uint8*)data)
	, m_size(size)
	, m_offset(0)
{
}

const uint32 CBinaryConsumer::Read(void* outBuffer, const uint32 sizeToRead)
{
	if (m_offset >= m_size)
		return 0;

	const uint32 dataLeft = (m_size - m_offset);
	const uint32 dataToCopy = Min<uint32>( dataLeft, sizeToRead );
	memcpy( outBuffer, m_data + m_offset, dataToCopy );
	m_offset += dataToCopy;

	return dataToCopy;
}

//---------------------------------------------------------------------------

CBinaryWriter::CBinaryWriter(void* data, const uint32 maxSize)
	: m_data((uint8*)data)
	, m_offset(0)
	, m_size(maxSize)
{
}

const uint32 CBinaryWriter::Write(const void* data, const uint32 sizeToWrite)
{
	if (m_offset >= m_size)
		return 0;

	const uint32 dataLeft = (m_size - m_offset);
	const uint32 dataToCopy = Min<uint32>( dataLeft, sizeToWrite );
	memcpy( m_data + m_offset, data, dataToCopy );
	m_offset += dataToCopy;

	return dataToCopy;
}

//---------------------------------------------------------------------------

static const uint32 ZLIB_CHUNK = 4096;

static voidpf ZlibAlloc(voidpf opaque, uInt items, uInt size)
{
	return malloc(items * size);
}

static void ZlibFree(voidpf opaque, voidpf address)
{
	free(address);
}

bool CompressData( const void* src, const uint32 srcSize, void* destData, uint32& destDataSize )
{
	// allocate deflate state
    z_stream strm;
	memset(&strm, 0, sizeof(strm));
    strm.zalloc = &ZlibAlloc;
	strm.zfree = &ZlibFree;
    strm.opaque = NULL;
    int ret = deflateInit(&strm, 9);
    if (ret != Z_OK)
        return false;

	// consumer of the input data
	CBinaryConsumer reader( src, srcSize );
	CBinaryWriter writer( destData, destDataSize );

	// buffers
	uint8 inputData[ ZLIB_CHUNK ];
	uint8 outputData[ ZLIB_CHUNK ];

	// compress until end of input data
	int flush = Z_NO_FLUSH;
    do
	{
		// read input data
		strm.avail_in = reader.Read(inputData, ZLIB_CHUNK);
        flush = reader.Eof() ? Z_FINISH : Z_NO_FLUSH;
        strm.next_in = inputData;

		// run deflate() on input until output buffer not full, finish compression if all of source has been read in
        do
		{
			strm.avail_out = ZLIB_CHUNK;
            strm.next_out = outputData;

			ret = deflate(&strm, flush); // no bad return value
            if (ret == Z_STREAM_ERROR) return false;  // state not clobbered

			const uint32 have = ZLIB_CHUNK - strm.avail_out;
			if (have != writer.Write(outputData, have))
			{
				deflateEnd(&strm);
				return false;  // not enough space in the output buffer
			}
		}
		while (strm.avail_out == 0);
    }
	while (flush != Z_FINISH);

	// clean up and return
    deflateEnd(&strm);
	destDataSize = writer.GetOffset(); // actual data size
	return true;
}

bool CompressLargeData( class ILogOutput& log, const void* src, const uint32 srcSize, CBinaryBigBuffer& destData )
{
	// allocate deflate state
    z_stream strm;
	memset(&strm, 0, sizeof(strm));
    strm.zalloc = &ZlibAlloc;
	strm.zfree = &ZlibFree;
    strm.opaque = NULL;
    int ret = deflateInit(&strm, 9);
    if (ret != Z_OK)
        return false;

	// consumer of the input data
	CBinaryConsumer reader( src, srcSize );

	// buffers
	uint8 inputData[ ZLIB_CHUNK ];
	uint8 outputData[ ZLIB_CHUNK ];

	// compress until end of input data
	int flush = Z_NO_FLUSH;
    do
	{
		// read input data
		strm.avail_in = reader.Read(inputData, ZLIB_CHUNK);
		log.SetTaskProgress(reader.GetOffset(), reader.GetSize());
        flush = reader.Eof() ? Z_FINISH : Z_NO_FLUSH;
        strm.next_in = inputData;

		// run deflate() on input until output buffer not full, finish compression if all of source has been read in
        do
		{
			strm.avail_out = ZLIB_CHUNK;
            strm.next_out = outputData;

			ret = deflate(&strm, flush); // no bad return value
            if (ret == Z_STREAM_ERROR) return false;  // state not clobbered

			const uint32 have = ZLIB_CHUNK - strm.avail_out;
			if (!destData.Append(outputData, have))
			{
				deflateEnd(&strm);
				return false;  // not enough space in the output buffer
			}
		}
		while (strm.avail_out == 0);
    }
	while (flush != Z_FINISH);

	// clean up and return
    deflateEnd(&strm);
	return true;
}

bool DecompressData( const void* src, const uint32 srcSize, void* destData, uint32& destDataSize )
{
	// allocate inflate state
	z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    int ret = inflateInit(&strm);
    if (ret != Z_OK)
        return false;

	// data transfer buffers
	CBinaryConsumer reader( src, srcSize );
	CBinaryWriter writer( destData, destDataSize );

	// chunks
	uint8 inputData[ ZLIB_CHUNK ];
	uint8 outputData[ ZLIB_CHUNK ];

	// decompress until deflate stream ends or end of file
    do
	{
		strm.avail_in = reader.Read(inputData, ZLIB_CHUNK);
        if (strm.avail_in == 0)
            break;
        strm.next_in = inputData;

		// run inflate() on input until output buffer not full
        do
		{
			strm.avail_out = ZLIB_CHUNK;
            strm.next_out = outputData;

			// inflate
			ret = inflate(&strm, Z_NO_FLUSH);
            switch (ret)
			{
				case Z_NEED_DICT:
					ret = Z_DATA_ERROR;     /* and fall through */
				case Z_DATA_ERROR:
				case Z_MEM_ERROR:
					(void)inflateEnd(&strm);
					return false;
            }

			// copy to output
			const uint32 have = ZLIB_CHUNK - strm.avail_out;
			if (have != writer.Write(outputData, have))
			{
                inflateEnd(&strm);
                return false; // out of space in the output buffer
            }
		}
		while (strm.avail_out == 0);

        // done when inflate() says it's done
    }
	while (ret != Z_STREAM_END);

	// final cleanup
	inflateEnd(&strm);
    return true;
}

bool DecompressLargeData( class ILogOutput& log, const void* src, const uint32 srcSize, CBinaryBigBuffer& destData )
{
	// allocate inflate state
	z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    int ret = inflateInit(&strm);
    if (ret != Z_OK)
        return false;

	// data transfer buffers
	CBinaryConsumer reader( src, srcSize );

	// chunks
	uint8 inputData[ ZLIB_CHUNK ];
	uint8 outputData[ ZLIB_CHUNK ];

	// decompress until deflate stream ends or end of file
    do
	{
		strm.avail_in = reader.Read(inputData, ZLIB_CHUNK);
		log.SetTaskProgress(reader.GetOffset(), reader.GetSize());
        if (strm.avail_in == 0)
            break;
        strm.next_in = inputData;

		// run inflate() on input until output buffer not full
        do
		{
			strm.avail_out = ZLIB_CHUNK;
            strm.next_out = outputData;

			// inflate
			ret = inflate(&strm, Z_NO_FLUSH);
            switch (ret)
			{
				case Z_NEED_DICT:
					ret = Z_DATA_ERROR;     /* and fall through */
				case Z_DATA_ERROR:
				case Z_MEM_ERROR:
					(void)inflateEnd(&strm);
					return false;
            }

			// copy to output
			const uint32 have = ZLIB_CHUNK - strm.avail_out;
			if (!destData.Append(outputData, have))
			{
                inflateEnd(&strm);
                return false; // out of space in the output buffer
            }
		}
		while (strm.avail_out == 0);

        // done when inflate() says it's done
    }
	while (ret != Z_STREAM_END);

	// final cleanup
	inflateEnd(&strm);
    return true;
}

//---------------------------------------------------------------------------

bool CheckFileExist(const wchar_t* filePath)
{
	const DWORD attr = GetFileAttributesW(filePath);
	if (attr == INVALID_FILE_ATTRIBUTES)
		return false;
	if (attr & FILE_ATTRIBUTE_DIRECTORY)
		return false;

	return true;
}

std::wstring GetTempDirectoryPath()
{
	static wchar_t tempPath[MAX_PATH];
	tempPath[0] = 0;
	GetTempPathW(MAX_PATH, tempPath);

	const size_t len = wcslen(tempPath);
	if ( len < MAX_PATH && tempPath[len-1] != '\\' )
		wcscat_s(tempPath, L"\\");

	return tempPath;
}

std::wstring GetCurDirectoryPath()
{
	static wchar_t tempPath[MAX_PATH];
	tempPath[0] = 0;
	GetCurrentDirectoryW(MAX_PATH, tempPath);

	const size_t len = wcslen(tempPath);
	if ( len < MAX_PATH && tempPath[len-1] != '\\' )
		wcscat_s(tempPath, L"\\");

	return tempPath;
}

std::wstring GetAppDirectoryPath()
{
	static wchar_t tempPath[MAX_PATH];
	tempPath[0] = 0;
	if ( GetModuleFileNameW( GetModuleHandle(NULL), tempPath, MAX_PATH ) <= 0 )
		return L"";

	wchar_t* str = wcsrchr( tempPath, '\\' );
	if ( !str )
		return L"";

	str[1] = 0;
	return tempPath;
}

std::wstring GetFileNameID(const int index)
{
	static wchar_t tempPath[MAX_PATH];
	swprintf_s( tempPath, L"%d", index );
	return tempPath;
}

//---------------------------------------------------------------------------
