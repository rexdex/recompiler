#pragma once

//---------------------------------------------------------------------------

#include <mutex>

//---------------------------------------------------------------------------

template < class T >
static inline void SafeRelease(T*& ptr)
{
	if (nullptr != ptr)
	{
		ptr->Release();
		ptr = nullptr;
	}
}

template < class T >
static inline void SafeAddRef(T*& ptr)
{
	if (nullptr != ptr)
	{
		ptr->AddRef();
	}
}

template < class T >
static inline T SafeArray(const std::vector<T>& ptr, const int index, T defaultValue = T(0))
{
	if (index < 0 || index >= (int)ptr.size())
	{
		return defaultValue;
	}

	return ptr[index];
}

template < class T >
static inline void ReleaseVector(std::vector<T*>& table)
{
	for (std::vector<T*>::const_iterator it = table.begin();
		it != table.end(); ++it)
	{
		(*it)->Release();
	}

	table.clear();
}

template < typename V, typename K >
static inline void ReleaseMap(std::map<K, V*>& table)
{
	for (std::map<K, V*>::const_iterator it = table.begin();
		it != table.end(); ++it)
	{
		(*it).second->Release();
	}

	table.clear();
}

template < class T >
static inline void DeleteVector(std::vector<T*>& table)
{
	for (std::vector<T*>::const_iterator it = table.begin();
		it != table.end(); ++it)
	{
		delete (*it);
	}

	table.clear();
}

template < typename V, typename K >
static inline void DeleteMap(std::map<K, V*>& table)
{
	for (std::map<K, V*>::const_iterator it = table.begin();
		it != table.end(); ++it)
	{
		delete (*it).second;
	}

	table.clear();
}

template< typename T >
static inline void PushBackUnique(std::vector<T>& vec, const T& elem)
{
	if (vec.end() == std::find(vec.begin(), vec.end(), elem))
		vec.push_back(elem);
}

static void ConcatParam(std::string& txt, const char* name)
{
	if (txt.empty())
	{
		txt += ", ";
	}

	txt += name;
}

static const char* VersionString(int major, int minor)
{
	static char s[16];
	sprintf_s(s, 16, "%d.%d", major, minor);
	return s;
}

static inline void RemoveEndingSlash(wchar_t* str)
{
	size_t len = wcslen(str);
	if (len > 0)
	{
		wchar_t end = str[len - 1];
		if (end == '/' || end == '\\')
		{
			str[len - 1] = 0;
		}
	}
}

static inline void RemoveFileName(wchar_t* str)
{
	wchar_t* end = wcsrchr(str, '\\');
	if (!end) end = wcsrchr(str, '/');
	if (end) *end = 0;
}

template< typename T >
const T& Max(const T& a, const T& b)
{
	return (a > b) ? a : b;
}

template< typename T >
const T& Min(const T& a, const T& b)
{
	return (a < b) ? a : b;
}

//---------------------------------------------------------------------------

/// Split and format file path
extern RECOMPILER_API void FixFilePathA(const char* filePath, const char* newExtension, std::string& outDirectory, std::string& outFilePath);

/// Split and format file path
extern RECOMPILER_API void FixFilePathW(const wchar_t* filePath, const wchar_t* newExtension, std::wstring& outDirectory, std::wstring& outFilePath);

/// Extract directory name with ending "\"
extern RECOMPILER_API void ExtractDirW(const wchar_t* filePath, std::wstring& outDirectory);

//---------------------------------------------------------------------------

// Make sure path exists
extern RECOMPILER_API bool CreateFilePath(const wchar_t* filePath);

// Save string to file
extern RECOMPILER_API bool SaveStringToFileA(const wchar_t* filePath, const std::string& text);

// Save string to file (supports unicode)
extern RECOMPILER_API bool SaveStringToFileW(const wchar_t* filePath, const std::wstring& text);

//---------------------------------------------------------------------------

// Compress small data (zlib), output buffer MUST exist, returns false if the destination buffer size was to small
extern RECOMPILER_API bool CompressData(const void* src, const uint32 srcSize, std::vector<uint8>& destData);

// Decompress small data (zlib), size of the destination buffer MUST be known beforehand, buffer needs to be preallocated
extern RECOMPILER_API bool DecompressData(const void* src, const uint32 srcSize, void* destData, uint32& destDataSize);

//---------------------------------------------------------------------------

/// Check if file with given absolute path exists
extern RECOMPILER_API bool CheckFileExist(const wchar_t* filePath);

/// Get system temporary directory
extern RECOMPILER_API std::wstring GetTempDirectoryPath();

/// Get application directory
extern RECOMPILER_API std::wstring GetAppDirectoryPath();

/// Get magic file path
extern RECOMPILER_API std::wstring GetFileNameID(const int index);

//---------------------------------------------------------------------------

// convert ansi to unicode
extern RECOMPILER_API void AnsiToUnicode(wchar_t* dest, const uint32 destSize, const char* src);

// convert unicode to ansi
extern RECOMPILER_API void UnicodeToAnsi(char* dest, const uint32 destSize, const wchar_t* src);

// convert ansi string to unicode string
extern RECOMPILER_API std::wstring AnsiToUnicode(const std::string& src);

// convert unicode string to ansi string
extern RECOMPILER_API std::string UnicodeToAnsi(const std::wstring& src);

//---------------------------------------------------------------------------

class RECOMPILER_API Commandline
{
public:
	Commandline();
	Commandline(const char** argv, const int argc);
	Commandline(const wchar_t** argv, const int argc);
	Commandline(const wchar_t* cmd);

	/// do we have an option specified ?
	const bool HasOption(const char* name) const;

	/// get first value for given option
	const std::wstring GetOptionValueW(const char* name) const;

	/// get first value for given option
	const std::string GetOptionValueA(const char* name) const;

private:
	struct Option
	{
		std::string m_name;
		std::vector<std::wstring> m_values;
	};

	typedef std::vector< Option > TOptions;
	TOptions	m_options;

	void AddOption(const std::string& name, const std::wstring& value);
	void Parse(const wchar_t* cmdLine);
};
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

#pragma once

  //-----------------------------------------------------------------------------

static const uint64 CRC64Init = 0xCBF29CE484222325;

// CRC calculator
class RECOMPILER_API CRC64
{
public:
	inline CRC64(const uint64 initValue = CRC64Init)
		: m_crc(initValue)
	{};

	/// get current CRC value
	inline const uint64 GetCRC() const { return m_crc; }

	/// append raw data of large size
	void Append(const void* data, const uint64 size);

	/// wrappers for trivial types
	inline CRC64& operator<<(const uint8& data) { AppendStatic<sizeof(data)>((const uint8*)&data); return *this; }
	inline CRC64& operator<<(const uint16& data) { AppendStatic<sizeof(data)>((const uint8*)&data); return *this; }
	inline CRC64& operator<<(const uint32& data) { AppendStatic<sizeof(data)>((const uint8*)&data); return *this; }
	inline CRC64& operator<<(const uint64& data) { AppendStatic<sizeof(data)>((const uint8*)&data); return *this; }
	inline CRC64& operator<<(const int8& data) { AppendStatic<sizeof(data)>((const uint8*)&data); return *this; }
	inline CRC64& operator<<(const int16& data) { AppendStatic<sizeof(data)>((const uint8*)&data); return *this; }
	inline CRC64& operator<<(const int32& data) { AppendStatic<sizeof(data)>((const uint8*)&data); return *this; }
	inline CRC64& operator<<(const int64& data) { AppendStatic<sizeof(data)>((const uint8*)&data); return *this; }
	inline CRC64& operator<<(const float& data) { AppendStatic<sizeof(data)>((const uint8*)&data); return *this; }
	inline CRC64& operator<<(const double& data) { AppendStatic<sizeof(data)>((const uint8*)&data); return *this; }
	inline CRC64& operator<<(const bool& data) { AppendStatic<sizeof(data)>((const uint8*)&data); return *this; }

	/// wrappers for strings
	CRC64& operator<<(const char* str);
	CRC64& operator<<(const wchar_t* str);
	CRC64& operator<<(const std::string& str);
	CRC64& operator<<(const std::wstring& str);

private:
	uint64 m_crc;

	/// append typed data of given size
	template<uint32 size>
	inline void AppendStatic(const uint8* data);

	template<>
	inline void AppendStatic<1>(const uint8* data)
	{
		register auto crc = m_crc;
		crc = (crc >> 8) ^ CRCTable[*data ^ (uint8)crc];
		m_crc = crc;
	}

	template<>
	inline void AppendStatic<2>(const uint8* data)
	{
		register auto crc = m_crc;
		crc = (crc >> 8) ^ CRCTable[*data++ ^ (uint8)crc];
		crc = (crc >> 8) ^ CRCTable[*data ^ (uint8)crc];
		m_crc = crc;
	}

	template<>
	inline void AppendStatic<4>(const uint8* data)
	{
		register auto crc = m_crc;
		crc = (crc >> 8) ^ CRCTable[*data++ ^ (uint8)crc];
		crc = (crc >> 8) ^ CRCTable[*data++ ^ (uint8)crc];
		crc = (crc >> 8) ^ CRCTable[*data++ ^ (uint8)crc];
		crc = (crc >> 8) ^ CRCTable[*data ^ (uint8)crc];
		m_crc = crc;
	}

	template<>
	inline void AppendStatic<8>(const uint8* data)
	{
		register auto crc = m_crc;
		crc = (crc >> 8) ^ CRCTable[*data++ ^ (uint8)crc];
		crc = (crc >> 8) ^ CRCTable[*data++ ^ (uint8)crc];
		crc = (crc >> 8) ^ CRCTable[*data++ ^ (uint8)crc];
		crc = (crc >> 8) ^ CRCTable[*data++ ^ (uint8)crc];
		crc = (crc >> 8) ^ CRCTable[*data++ ^ (uint8)crc];
		crc = (crc >> 8) ^ CRCTable[*data++ ^ (uint8)crc];
		crc = (crc >> 8) ^ CRCTable[*data++ ^ (uint8)crc];
		crc = (crc >> 8) ^ CRCTable[*data ^ (uint8)crc];
		m_crc = crc;
	}

	static const uint64 CRCTable[256];
};

//-----------------------------------------------------------------------------

// compute CRC for string
extern RECOMPILER_API const uint64 StringCRC64(const char* txt);

// compute CRC for buffer
extern RECOMPILER_API const uint64 BufferCRC64(const void* data, const uint64 size);

//-----------------------------------------------------------------------------
