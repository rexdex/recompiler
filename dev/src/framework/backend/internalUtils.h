#pragma once

//---------------------------------------------------------------------------

#ifdef _MSC_VER
	#include <Windows.h>
#endif
#include <mutex>

//---------------------------------------------------------------------------

template < class T >
static inline void SafeRelease( T*& ptr )
{
	if ( nullptr != ptr )
	{
		ptr->Release();
		ptr = nullptr;
	}
}

template < class T >
static inline void SafeAddRef( T*& ptr )
{
	if ( nullptr != ptr )
	{
		ptr->AddRef();
	}
}

template < class T >
static inline T SafeArray( const std::vector<T>& ptr, const int index, T defaultValue = T(0) )
{
	if ( index < 0 || index >= (int)ptr.size() )
	{
		return defaultValue;
	}

	return ptr[index];
}

template < class T >
static inline void ReleaseVector( std::vector<T*>& table )
{
	for ( std::vector<T*>::const_iterator it = table.begin();
		it != table.end(); ++it )
	{
		(*it)->Release();
	}

	table.clear();
}

template < typename V, typename K >
static inline void ReleaseMap( std::map<K,V*>& table )
{
	for ( std::map<K,V*>::const_iterator it = table.begin();
		it != table.end(); ++it )
	{
		(*it).second->Release();
	}

	table.clear();
}

template < class T >
static inline void DeleteVector( std::vector<T*>& table )
{
	for ( std::vector<T*>::const_iterator it = table.begin();
		it != table.end(); ++it )
	{
		delete (*it);
	}

	table.clear();
}

template < typename V, typename K >
static inline void DeleteMap( std::map<K,V*>& table )
{
	for ( std::map<K,V*>::const_iterator it = table.begin();
		it != table.end(); ++it )
	{
		delete (*it).second;
	}

	table.clear();
}

template< typename T >
static inline void PushBackUnique( std::vector<T>& vec, const T& elem )
{
	if ( vec.end() == std::find( vec.begin(), vec.end(), elem ) )
		vec.push_back( elem );
}

static void ConcatParam( std::string& txt, const char* name )
{
	if ( txt.empty() )
	{
		txt += ", ";
	}

	txt += name;
}

static const char* VersionString( int major, int minor )
{
	static char s[16];
	sprintf_s( s, 16, "%d.%d", major, minor );
	return s;
}

static inline void RemoveEndingSlash( wchar_t* str )
{
	size_t len = wcslen( str );
	if ( len > 0 )
	{
		wchar_t end = str[ len-1 ];
		if ( end == '/' || end == '\\' ) 
		{
			str[ len-1 ] = 0;
		}
	}
}

static inline void RemoveFileName( wchar_t* str )
{
	wchar_t* end = wcsrchr( str, '\\' );
	if ( !end ) end = wcsrchr( str, '/' );
	if ( end ) *end = 0;
}

template< typename T >
const T& Max( const T& a, const T& b )
{
	return (a>b) ? a : b;
}

template< typename T >
const T& Min( const T& a, const T& b )
{
	return (a<b) ? a : b;
}

//---------------------------------------------------------------------------

static inline int AtomicIncrement( volatile int* ptr )
{
	return _InterlockedIncrement( (volatile long*) ptr );
}

static inline int AtomicDecrement( volatile int* ptr )
{
	return _InterlockedDecrement( (volatile long*) ptr );
}

//---------------------------------------------------------------------------

/// Scan directory to find files or other directories
class ScanDirectoryA
{
private:
	WIN32_FIND_DATAA	m_data;
	HANDLE				m_hFind;
	bool				m_first;

public:
	ScanDirectoryA( const char* searchPah );
	~ScanDirectoryA();

	bool Iterate();

	bool IsFile() const;
	bool IsDirectory() const;

	const char* GetFileName() const;
};

//---------------------------------------------------------------------------

/// Scan directory to find files or other directories
class ScanDirectoryW
{
private:
	WIN32_FIND_DATAW	m_data;
	HANDLE				m_hFind;
	bool				m_first;

public:
	ScanDirectoryW( const wchar_t* searchPah );
	~ScanDirectoryW();

	bool Iterate();

	bool IsFile() const;
	bool IsDirectory() const;

	const wchar_t* GetFileName() const;
};

//---------------------------------------------------------------------------

// Big buffer (big data generation)
class CBinaryBigBuffer
{
public:
	CBinaryBigBuffer();
	~CBinaryBigBuffer();

	// get current size
	inline const uint64 GetSize() const { return m_size; }

	// clear current content
	void Clear();

	// append data to big buffer
	const bool Append(const void* data, const uint32 size);

	// copy out the data
	const bool CopyData(void* outData, const uint64 outDataSize) const;

	// get item
	const uint8 GetByte(const uint64 position) const;

private:
	CBinaryBigBuffer(const CBinaryBigBuffer& other);
	CBinaryBigBuffer& operator=(const CBinaryBigBuffer& other);

	const static uint32 PAGE_SIZE = 64 << 10;

	struct Page
	{
		uint8*	m_mem;
		uint32	m_used;
	};

	std::vector<Page>	m_pages;
	Page*				m_curPage;
	uint64				m_size;

	mutable const Page*		m_lastReadPage;
	mutable uint64			m_lastReadPageStart;
	mutable uint64			m_lastReadPageEnd;
};

//---------------------------------------------------------------------------

class CBinaryConsumer
{
public:
	CBinaryConsumer(const void* data, const uint32 size);

	inline const uint32 GetSize() const { return m_size; }
	inline const uint32 GetOffset() const { return m_offset; }

	inline const bool Eof() const { return (m_size == m_offset); }

	const uint32 Read(void* outBuffer, const uint32 sizeToRead);

private:
	const uint8*	m_data;
	uint32			m_size;
	uint32			m_offset;

	CBinaryConsumer(const CBinaryConsumer& other);
	CBinaryConsumer& operator=(const CBinaryConsumer& other);
};

//---------------------------------------------------------------------------

class CBinaryWriter
{
public:
	CBinaryWriter(void* data, const uint32 maxSize);

	inline const uint32 GetSize() const { return m_size; }
	inline const uint32 GetOffset() const { return m_offset; }

	inline const bool IsFull() const { return (m_size == m_offset); }

	const uint32 Write(const void* data, const uint32 sizeToWrite);

private:
	uint8*		m_data;
	uint32		m_size;
	uint32		m_offset;

	CBinaryWriter(const CBinaryConsumer& other);
	CBinaryWriter& operator=(const CBinaryWriter& other);
};

//---------------------------------------------------------------------------

/// Split and format file path
extern void FixFilePathA( const char* filePath, const char* newExtension, std::string& outDirectory, std::string& outFilePath );

/// Split and format file path
extern void FixFilePathW( const wchar_t* filePath, const wchar_t* newExtension, std::wstring& outDirectory, std::wstring& outFilePath );

/// Extract directory name with ending "\"
extern void ExtractDirW( const wchar_t* filePath, std::wstring& outDirectory );

//---------------------------------------------------------------------------

// Save string to file
extern bool SaveStringToFileA( const wchar_t* filePath, const std::string& text );

// Save string to file (supports unicode)
extern bool SaveStringToFileW( const wchar_t* filePath, const std::wstring& text );

//---------------------------------------------------------------------------

// Compress small data (zlib), output buffer MUST exist, returns false if the destination buffer size was to small
extern bool CompressData( const void* src, const uint32 srcSize, void* destData, uint32& destDataSize );

// Compress large data block into the big buffer
extern bool CompressLargeData( class ILogOutput& log, const void* src, const uint32 srcSize, CBinaryBigBuffer& destData );

// Decompress small data (zlib), size of the destination buffer MUST be known beforehand, buffer needs to be preallocated
extern bool DecompressData( const void* src, const uint32 srcSize, void* destData, uint32& destDataSize );

// Decompress large data block into the big buffer
extern bool DecompressLargeData( class ILogOutput& log, const void* src, const uint32 srcSize, CBinaryBigBuffer& destData );

//---------------------------------------------------------------------------

/// Check if file with given absolute path exists
extern bool CheckFileExist(const wchar_t* filePath);

/// Get system temporary directory
extern std::wstring GetTempDirectoryPath();

/// Get current system directory
extern std::wstring GetCurDirectoryPath();

/// Get application directory
extern std::wstring GetAppDirectoryPath();

/// Get magic file path
extern std::wstring GetFileNameID(const int index);

//---------------------------------------------------------------------------

namespace std
{
	class semaphore
	{
	public:
		semaphore()
			: m_count(0)
		{}

		void notify()
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			m_count++;
			m_condition.notify_one();
		}

		void wait()
		{
			std::unique_lock<std::mutex> lock(m_mutex);

			while (m_count == 0)
				m_condition.wait(lock);

			m_count;
		}

	private:
		mutex m_mutex;
		condition_variable m_condition;
		unsigned long m_count;
	};

} // std