#include "build.h"
#include "internalUtils.h"
#include "zlib\zlib.h"

#if defined(_WIN64) || defined(_WIN32)
	#include <Windows.h>
#endif
#include <algorithm>

//---------------------------------------------------------------------------

void FixFilePathA(const char* filePath, const char* newExtension, std::string& outDirectory, std::string& outFilePath)
{
	// find the file division
	const char* filePart = strrchr(filePath, '\\');
	const char* filePart2 = strrchr(filePath, '/');
	filePart = (filePart > filePart2) ? filePart : filePart2;

	// get the directory
	if (NULL != filePart)
	{
		outDirectory = std::string(filePath, filePart - filePath);
	}
	else
	{
		outDirectory = "";
	}

	// get the file extension
	const char* fileExt = strrchr(filePart, '.');
	if (fileExt)
	{
		outFilePath = std::string(filePath, fileExt - filePath);
	}
	else
	{
		outFilePath = filePath;
	}

	// append new extension
	if (newExtension && newExtension[0])
	{
		outFilePath += ".";
		outFilePath += newExtension;
	}
}

//---------------------------------------------------------------------------

void FixFilePathW(const wchar_t* filePath, const wchar_t* newExtension, std::wstring& outDirectory, std::wstring& outFilePath)
{
	// find the file division
	const wchar_t* filePart = wcsrchr(filePath, '\\');
	const wchar_t* filePart2 = wcsrchr(filePath, '/');
	filePart = (filePart > filePart2) ? filePart : filePart2;

	// get the directory
	if (NULL != filePart)
	{
		outDirectory = std::wstring(filePath, filePart - filePath);
		outDirectory += L"\\";
	}
	else
	{
		outDirectory = L"\\";
	}

	// get the file extension
	const wchar_t* fileExt = wcsrchr(filePart, '.');
	if (fileExt)
	{
		outFilePath = std::wstring(filePath, fileExt - filePath);
	}
	else
	{
		outFilePath = filePath;
	}

	// append new extension
	if (newExtension && newExtension[0])
	{
		outFilePath += L".";
		outFilePath += newExtension;
	}
}

//---------------------------------------------------------------------------

void ExtractDirW(const wchar_t* filePath, std::wstring& outDirectory)
{
	// find the file division
	const wchar_t* filePart = wcsrchr(filePath, '\\');
	const wchar_t* filePart2 = wcsrchr(filePath, '/');
	filePart = (filePart > filePart2) ? filePart : filePart2;

	// get the directory
	if (NULL != filePart)
	{
		outDirectory = std::wstring(filePath, filePart - filePath);
		outDirectory += L"\\";
	}
	else
	{
		outDirectory = L"\\";
	}
}

//---------------------------------------------------------------------------

bool SaveStringToFileA(const wchar_t* filePath, const std::string& text)
{
	FILE* f = NULL;
	_wfopen_s(&f, filePath, L"wb");
	if (!f) return false;

	fwrite(text.c_str(), text.length(), 1, f);
	fclose(f);

	return true;
}

bool SaveStringToFileW(const wchar_t* filePath, const std::wstring& text)
{
	FILE* f = NULL;
	_wfopen_s(&f, filePath, L"wb");
	if (!f) return false;

	uint8 prefix[2] = { 0xFF, 0xFE };
	fwrite(prefix, 2, 1, f);

	fwrite(text.c_str(), text.length(), 1, f);
	fclose(f);

	return true;
}

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
	for (uint32 i = 0; i < m_pages.size(); ++i)
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
	const uint8* readPtr = (const uint8*)data;
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
			info.m_mem = (uint8*)mem;
			info.m_used = 0;
			m_pages.push_back(info);
			m_curPage = &m_pages.back();
			m_lastReadPage = NULL;
			m_lastReadPageStart = 0;
			m_lastReadPageEnd = 0;
			continue;
		}

		// copy as much as we can in one batch
		const uint32 sizeToCopy = Min<uint32>(leftInPage, left);
		memcpy(m_curPage->m_mem + m_curPage->m_used, readPtr, sizeToCopy);

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
	uint8* writePtr = (uint8*)outData;
	uint64 writtenSoFar = 0;
	for (uint32 i = 0; i < m_pages.size(); ++i)
	{
		const Page& page = m_pages[i];

		if (writtenSoFar + page.m_used > outDataSize)
			return false;

		memcpy(writePtr, page.m_mem, page.m_used);
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
		for (uint32 i = 0; i < m_pages.size(); ++i)
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
	return m_lastReadPage->m_mem[position - m_lastReadPageStart];
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
	const uint32 dataToCopy = Min<uint32>(dataLeft, sizeToRead);
	memcpy(outBuffer, m_data + m_offset, dataToCopy);
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
	const uint32 dataToCopy = Min<uint32>(dataLeft, sizeToWrite);
	memcpy(m_data + m_offset, data, dataToCopy);
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

bool CompressData(const void* src, const uint32 srcSize, std::vector<uint8>& destData)
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
	CBinaryConsumer reader(src, srcSize);
	CBinaryBigBuffer writer;

	// buffers
	uint8 inputData[ZLIB_CHUNK];
	uint8 outputData[ZLIB_CHUNK];

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
			if (!writer.Append(outputData, have))
			{
				deflateEnd(&strm);
				return false;  // not enough space in the output buffer
			}
		} while (strm.avail_out == 0);
	} while (flush != Z_FINISH);

	// clean up and return
	deflateEnd(&strm);

	// copy data out
	destData.resize(writer.GetSize());
	writer.CopyData(destData.data(), destData.size());
	return true;
}

bool CompressLargeData(class ILogOutput& log, const void* src, const uint32 srcSize, CBinaryBigBuffer& destData)
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
	CBinaryConsumer reader(src, srcSize);

	// buffers
	uint8 inputData[ZLIB_CHUNK];
	uint8 outputData[ZLIB_CHUNK];

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
		} while (strm.avail_out == 0);
	} while (flush != Z_FINISH);

	// clean up and return
	deflateEnd(&strm);
	return true;
}

bool DecompressData(const void* src, const uint32 srcSize, void* destData, uint32& destDataSize)
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
	CBinaryConsumer reader(src, srcSize);
	CBinaryWriter writer(destData, destDataSize);

	// chunks
	uint8 inputData[ZLIB_CHUNK];
	uint8 outputData[ZLIB_CHUNK];

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
		} while (strm.avail_out == 0);

		// done when inflate() says it's done
	} while (ret != Z_STREAM_END);

	// final cleanup
	inflateEnd(&strm);
	return true;
}

bool DecompressLargeData(class ILogOutput& log, const void* src, const uint32 srcSize, CBinaryBigBuffer& destData)
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
	CBinaryConsumer reader(src, srcSize);

	// chunks
	uint8 inputData[ZLIB_CHUNK];
	uint8 outputData[ZLIB_CHUNK];

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
		} while (strm.avail_out == 0);

		// done when inflate() says it's done
	} while (ret != Z_STREAM_END);

	// final cleanup
	inflateEnd(&strm);
	return true;
}

//---------------------------------------------------------------------------

bool CheckFileExist(const wchar_t* filePath)
{
	FILE* f = NULL;
	_wfopen_s(&f, filePath, L"rb");
	if (!f)
		return false;
	fclose(f);
	return true;
}

std::wstring GetTempDirectoryPath()
{
#if defined(_WIN64) || defined(_WIN32)
	static wchar_t tempPath[MAX_PATH];
	tempPath[0] = 0;
	GetTempPathW(MAX_PATH, tempPath);

	const size_t len = wcslen(tempPath);
	if (len < MAX_PATH && tempPath[len - 1] != '\\')
		wcscat_s(tempPath, L"\\");

	return tempPath;
#else
	return L"~/.uc/temp/";
#endif
}

std::wstring GetAppDirectoryPath()
{
#if defined(_WIN64) || defined(_WIN32)
	static wchar_t tempPath[MAX_PATH];
	tempPath[0] = 0;
	if (GetModuleFileNameW(GetModuleHandle(NULL), tempPath, MAX_PATH) <= 0)
		return L"";

	wchar_t* str = wcsrchr(tempPath, '\\');
	if (!str)
		return L"";

	str[1] = 0;
	return tempPath;
#else
	return L"";
#endif
}

bool CreateFilePath(const wchar_t* path)
{
#if defined(_WIN64) || defined(_WIN32)
	// make sure path exists
	wchar_t tempPath[MAX_PATH];
	wcscpy_s(tempPath, path);
	wchar_t* cur = tempPath;
	while (*cur != 0)
	{
		if (*cur == '\\' || *cur == '/')
		{
			if (cur - tempPath > 3)
			{
				*cur = 0;
				if (!CreateDirectoryW(tempPath, NULL))
				{
					if (GetLastError() != ERROR_ALREADY_EXISTS)
					{
						return false;
					}
				}
				*cur = '\\';
			}
		}

		++cur;
	}
#endif

	// path created
	return true;
}

std::wstring GetFileNameID(const int index)
{
	static wchar_t tempPath[MAX_PATH];
	swprintf_s(tempPath, L"%d", index);
	return tempPath;
}

std::wstring GetFileName(const std::wstring& path)
{
	const auto* part = std::max<const wchar_t*>(wcsrchr(path.c_str(), '/'), wcsrchr(path.c_str(), '\\'));
	if (!part)
		return path;

	return part + 1;
}

//---------------------------------------------------------------------------

void AnsiToUnicode(wchar_t* dest, const uint32 destSize, const char* src)
{
	wchar_t* destEnd = dest + destSize - 1;
	while (*src && (dest < destEnd))
	{
		*dest++ = (wchar_t)(*src++);
	}
	*dest = 0;
}

void UnicodeToAnsi(char* dest, const uint32 destSize, const wchar_t* src)
{
	char* destEnd = dest + destSize - 1;
	while (*src && (dest < destEnd))
	{
		wchar_t ch = *src++;
		*dest++ = ch < 255 ? (char)ch : '_';
	}
	*dest = 0;
}

std::wstring AnsiToUnicode(const std::string& src)
{
	wchar_t* dest = (wchar_t*)_alloca((src.length() + 1) * sizeof(wchar_t));
	AnsiToUnicode(dest, (uint32)(src.length() + 1), src.c_str());
	return std::wstring(dest);
}

std::string UnicodeToAnsi(const std::wstring& src)
{
	char* dest = (char*)_alloca((src.length() + 1) * sizeof(char));
	UnicodeToAnsi(dest, (uint32)(src.length() + 1), src.c_str());
	return std::string(dest);
}

//---------------------------------------------------------------------------

Commandline::Commandline()
{
}

Commandline::Commandline(const char** argv, const int argc)
{
    // argv contains strings of the format "-varName=value" or flags "-flag". We loop over all args and find valid ones.
    for (int i = 1; i < argc; ++i)
    {
        std::string s(argv[i]);

		// we are interested only in command arguments starting with '-'
		if (s.at(0) != '-')
			continue;
		s = s.substr(1);

        // Find the equals sign; only proceed if it really exists
        const auto equalsSign = s.find("=");
        if (equalsSign != std::string::npos)
		{
            // Get the option name from the string
            std::string part1 = s.substr(0, equalsSign);

            // If it was a valid option name (if it had a hyphen in the front) then proceed
			std::string part2 = s.substr(equalsSign + 1, std::string::npos);
            AddOption(part1, AnsiToUnicode(part2));
        }
		else
		{
			// It's a flag
			AddOption(s, std::wstring());
		}
    }
}

const bool Commandline::HasOption(const char* name) const
{
	for (const auto& option : m_options)
		if (option.m_name == name)
			return true;

	return false;
}

const std::wstring Commandline::GetOptionValueW(const char* name) const
{
	for (const auto& option : m_options)
		if (option.m_name == name && !option.m_values.empty())
			return option.m_values[0];

	return std::wstring();
}

const std::string Commandline::GetOptionValueA(const char* name) const
{
	for (const auto& option : m_options)
		if (option.m_name == name)
			return UnicodeToAnsi(option.m_values[0]);

	return std::string();
}

void Commandline::AddOption(const std::string& name, const std::wstring& value)
{
	for (auto& option : m_options)
	{
		if (option.m_name == name)
		{
			if (!value.empty())
				option.m_values.push_back(value);

			return;
		}
	}

	Option info;
	info.m_name = name;
	info.m_values.push_back(value);
	m_options.push_back(std::move(info));
}


//---------------------------------------------------------------------------

const uint64 CRC64::CRCTable[256] =
{
	UINT64_C(0x0000000000000000), UINT64_C(0x7ad870c830358979),
	UINT64_C(0xf5b0e190606b12f2), UINT64_C(0x8f689158505e9b8b),
	UINT64_C(0xc038e5739841b68f), UINT64_C(0xbae095bba8743ff6),
	UINT64_C(0x358804e3f82aa47d), UINT64_C(0x4f50742bc81f2d04),
	UINT64_C(0xab28ecb46814fe75), UINT64_C(0xd1f09c7c5821770c),
	UINT64_C(0x5e980d24087fec87), UINT64_C(0x24407dec384a65fe),
	UINT64_C(0x6b1009c7f05548fa), UINT64_C(0x11c8790fc060c183),
	UINT64_C(0x9ea0e857903e5a08), UINT64_C(0xe478989fa00bd371),
	UINT64_C(0x7d08ff3b88be6f81), UINT64_C(0x07d08ff3b88be6f8),
	UINT64_C(0x88b81eabe8d57d73), UINT64_C(0xf2606e63d8e0f40a),
	UINT64_C(0xbd301a4810ffd90e), UINT64_C(0xc7e86a8020ca5077),
	UINT64_C(0x4880fbd87094cbfc), UINT64_C(0x32588b1040a14285),
	UINT64_C(0xd620138fe0aa91f4), UINT64_C(0xacf86347d09f188d),
	UINT64_C(0x2390f21f80c18306), UINT64_C(0x594882d7b0f40a7f),
	UINT64_C(0x1618f6fc78eb277b), UINT64_C(0x6cc0863448deae02),
	UINT64_C(0xe3a8176c18803589), UINT64_C(0x997067a428b5bcf0),
	UINT64_C(0xfa11fe77117cdf02), UINT64_C(0x80c98ebf2149567b),
	UINT64_C(0x0fa11fe77117cdf0), UINT64_C(0x75796f2f41224489),
	UINT64_C(0x3a291b04893d698d), UINT64_C(0x40f16bccb908e0f4),
	UINT64_C(0xcf99fa94e9567b7f), UINT64_C(0xb5418a5cd963f206),
	UINT64_C(0x513912c379682177), UINT64_C(0x2be1620b495da80e),
	UINT64_C(0xa489f35319033385), UINT64_C(0xde51839b2936bafc),
	UINT64_C(0x9101f7b0e12997f8), UINT64_C(0xebd98778d11c1e81),
	UINT64_C(0x64b116208142850a), UINT64_C(0x1e6966e8b1770c73),
	UINT64_C(0x8719014c99c2b083), UINT64_C(0xfdc17184a9f739fa),
	UINT64_C(0x72a9e0dcf9a9a271), UINT64_C(0x08719014c99c2b08),
	UINT64_C(0x4721e43f0183060c), UINT64_C(0x3df994f731b68f75),
	UINT64_C(0xb29105af61e814fe), UINT64_C(0xc849756751dd9d87),
	UINT64_C(0x2c31edf8f1d64ef6), UINT64_C(0x56e99d30c1e3c78f),
	UINT64_C(0xd9810c6891bd5c04), UINT64_C(0xa3597ca0a188d57d),
	UINT64_C(0xec09088b6997f879), UINT64_C(0x96d1784359a27100),
	UINT64_C(0x19b9e91b09fcea8b), UINT64_C(0x636199d339c963f2),
	UINT64_C(0xdf7adabd7a6e2d6f), UINT64_C(0xa5a2aa754a5ba416),
	UINT64_C(0x2aca3b2d1a053f9d), UINT64_C(0x50124be52a30b6e4),
	UINT64_C(0x1f423fcee22f9be0), UINT64_C(0x659a4f06d21a1299),
	UINT64_C(0xeaf2de5e82448912), UINT64_C(0x902aae96b271006b),
	UINT64_C(0x74523609127ad31a), UINT64_C(0x0e8a46c1224f5a63),
	UINT64_C(0x81e2d7997211c1e8), UINT64_C(0xfb3aa75142244891),
	UINT64_C(0xb46ad37a8a3b6595), UINT64_C(0xceb2a3b2ba0eecec),
	UINT64_C(0x41da32eaea507767), UINT64_C(0x3b024222da65fe1e),
	UINT64_C(0xa2722586f2d042ee), UINT64_C(0xd8aa554ec2e5cb97),
	UINT64_C(0x57c2c41692bb501c), UINT64_C(0x2d1ab4dea28ed965),
	UINT64_C(0x624ac0f56a91f461), UINT64_C(0x1892b03d5aa47d18),
	UINT64_C(0x97fa21650afae693), UINT64_C(0xed2251ad3acf6fea),
	UINT64_C(0x095ac9329ac4bc9b), UINT64_C(0x7382b9faaaf135e2),
	UINT64_C(0xfcea28a2faafae69), UINT64_C(0x8632586aca9a2710),
	UINT64_C(0xc9622c4102850a14), UINT64_C(0xb3ba5c8932b0836d),
	UINT64_C(0x3cd2cdd162ee18e6), UINT64_C(0x460abd1952db919f),
	UINT64_C(0x256b24ca6b12f26d), UINT64_C(0x5fb354025b277b14),
	UINT64_C(0xd0dbc55a0b79e09f), UINT64_C(0xaa03b5923b4c69e6),
	UINT64_C(0xe553c1b9f35344e2), UINT64_C(0x9f8bb171c366cd9b),
	UINT64_C(0x10e3202993385610), UINT64_C(0x6a3b50e1a30ddf69),
	UINT64_C(0x8e43c87e03060c18), UINT64_C(0xf49bb8b633338561),
	UINT64_C(0x7bf329ee636d1eea), UINT64_C(0x012b592653589793),
	UINT64_C(0x4e7b2d0d9b47ba97), UINT64_C(0x34a35dc5ab7233ee),
	UINT64_C(0xbbcbcc9dfb2ca865), UINT64_C(0xc113bc55cb19211c),
	UINT64_C(0x5863dbf1e3ac9dec), UINT64_C(0x22bbab39d3991495),
	UINT64_C(0xadd33a6183c78f1e), UINT64_C(0xd70b4aa9b3f20667),
	UINT64_C(0x985b3e827bed2b63), UINT64_C(0xe2834e4a4bd8a21a),
	UINT64_C(0x6debdf121b863991), UINT64_C(0x1733afda2bb3b0e8),
	UINT64_C(0xf34b37458bb86399), UINT64_C(0x8993478dbb8deae0),
	UINT64_C(0x06fbd6d5ebd3716b), UINT64_C(0x7c23a61ddbe6f812),
	UINT64_C(0x3373d23613f9d516), UINT64_C(0x49aba2fe23cc5c6f),
	UINT64_C(0xc6c333a67392c7e4), UINT64_C(0xbc1b436e43a74e9d),
	UINT64_C(0x95ac9329ac4bc9b5), UINT64_C(0xef74e3e19c7e40cc),
	UINT64_C(0x601c72b9cc20db47), UINT64_C(0x1ac40271fc15523e),
	UINT64_C(0x5594765a340a7f3a), UINT64_C(0x2f4c0692043ff643),
	UINT64_C(0xa02497ca54616dc8), UINT64_C(0xdafce7026454e4b1),
	UINT64_C(0x3e847f9dc45f37c0), UINT64_C(0x445c0f55f46abeb9),
	UINT64_C(0xcb349e0da4342532), UINT64_C(0xb1eceec59401ac4b),
	UINT64_C(0xfebc9aee5c1e814f), UINT64_C(0x8464ea266c2b0836),
	UINT64_C(0x0b0c7b7e3c7593bd), UINT64_C(0x71d40bb60c401ac4),
	UINT64_C(0xe8a46c1224f5a634), UINT64_C(0x927c1cda14c02f4d),
	UINT64_C(0x1d148d82449eb4c6), UINT64_C(0x67ccfd4a74ab3dbf),
	UINT64_C(0x289c8961bcb410bb), UINT64_C(0x5244f9a98c8199c2),
	UINT64_C(0xdd2c68f1dcdf0249), UINT64_C(0xa7f41839ecea8b30),
	UINT64_C(0x438c80a64ce15841), UINT64_C(0x3954f06e7cd4d138),
	UINT64_C(0xb63c61362c8a4ab3), UINT64_C(0xcce411fe1cbfc3ca),
	UINT64_C(0x83b465d5d4a0eece), UINT64_C(0xf96c151de49567b7),
	UINT64_C(0x76048445b4cbfc3c), UINT64_C(0x0cdcf48d84fe7545),
	UINT64_C(0x6fbd6d5ebd3716b7), UINT64_C(0x15651d968d029fce),
	UINT64_C(0x9a0d8ccedd5c0445), UINT64_C(0xe0d5fc06ed698d3c),
	UINT64_C(0xaf85882d2576a038), UINT64_C(0xd55df8e515432941),
	UINT64_C(0x5a3569bd451db2ca), UINT64_C(0x20ed197575283bb3),
	UINT64_C(0xc49581ead523e8c2), UINT64_C(0xbe4df122e51661bb),
	UINT64_C(0x3125607ab548fa30), UINT64_C(0x4bfd10b2857d7349),
	UINT64_C(0x04ad64994d625e4d), UINT64_C(0x7e7514517d57d734),
	UINT64_C(0xf11d85092d094cbf), UINT64_C(0x8bc5f5c11d3cc5c6),
	UINT64_C(0x12b5926535897936), UINT64_C(0x686de2ad05bcf04f),
	UINT64_C(0xe70573f555e26bc4), UINT64_C(0x9ddd033d65d7e2bd),
	UINT64_C(0xd28d7716adc8cfb9), UINT64_C(0xa85507de9dfd46c0),
	UINT64_C(0x273d9686cda3dd4b), UINT64_C(0x5de5e64efd965432),
	UINT64_C(0xb99d7ed15d9d8743), UINT64_C(0xc3450e196da80e3a),
	UINT64_C(0x4c2d9f413df695b1), UINT64_C(0x36f5ef890dc31cc8),
	UINT64_C(0x79a59ba2c5dc31cc), UINT64_C(0x037deb6af5e9b8b5),
	UINT64_C(0x8c157a32a5b7233e), UINT64_C(0xf6cd0afa9582aa47),
	UINT64_C(0x4ad64994d625e4da), UINT64_C(0x300e395ce6106da3),
	UINT64_C(0xbf66a804b64ef628), UINT64_C(0xc5bed8cc867b7f51),
	UINT64_C(0x8aeeace74e645255), UINT64_C(0xf036dc2f7e51db2c),
	UINT64_C(0x7f5e4d772e0f40a7), UINT64_C(0x05863dbf1e3ac9de),
	UINT64_C(0xe1fea520be311aaf), UINT64_C(0x9b26d5e88e0493d6),
	UINT64_C(0x144e44b0de5a085d), UINT64_C(0x6e963478ee6f8124),
	UINT64_C(0x21c640532670ac20), UINT64_C(0x5b1e309b16452559),
	UINT64_C(0xd476a1c3461bbed2), UINT64_C(0xaeaed10b762e37ab),
	UINT64_C(0x37deb6af5e9b8b5b), UINT64_C(0x4d06c6676eae0222),
	UINT64_C(0xc26e573f3ef099a9), UINT64_C(0xb8b627f70ec510d0),
	UINT64_C(0xf7e653dcc6da3dd4), UINT64_C(0x8d3e2314f6efb4ad),
	UINT64_C(0x0256b24ca6b12f26), UINT64_C(0x788ec2849684a65f),
	UINT64_C(0x9cf65a1b368f752e), UINT64_C(0xe62e2ad306bafc57),
	UINT64_C(0x6946bb8b56e467dc), UINT64_C(0x139ecb4366d1eea5),
	UINT64_C(0x5ccebf68aecec3a1), UINT64_C(0x2616cfa09efb4ad8),
	UINT64_C(0xa97e5ef8cea5d153), UINT64_C(0xd3a62e30fe90582a),
	UINT64_C(0xb0c7b7e3c7593bd8), UINT64_C(0xca1fc72bf76cb2a1),
	UINT64_C(0x45775673a732292a), UINT64_C(0x3faf26bb9707a053),
	UINT64_C(0x70ff52905f188d57), UINT64_C(0x0a2722586f2d042e),
	UINT64_C(0x854fb3003f739fa5), UINT64_C(0xff97c3c80f4616dc),
	UINT64_C(0x1bef5b57af4dc5ad), UINT64_C(0x61372b9f9f784cd4),
	UINT64_C(0xee5fbac7cf26d75f), UINT64_C(0x9487ca0fff135e26),
	UINT64_C(0xdbd7be24370c7322), UINT64_C(0xa10fceec0739fa5b),
	UINT64_C(0x2e675fb4576761d0), UINT64_C(0x54bf2f7c6752e8a9),
	UINT64_C(0xcdcf48d84fe75459), UINT64_C(0xb71738107fd2dd20),
	UINT64_C(0x387fa9482f8c46ab), UINT64_C(0x42a7d9801fb9cfd2),
	UINT64_C(0x0df7adabd7a6e2d6), UINT64_C(0x772fdd63e7936baf),
	UINT64_C(0xf8474c3bb7cdf024), UINT64_C(0x829f3cf387f8795d),
	UINT64_C(0x66e7a46c27f3aa2c), UINT64_C(0x1c3fd4a417c62355),
	UINT64_C(0x935745fc4798b8de), UINT64_C(0xe98f353477ad31a7),
	UINT64_C(0xa6df411fbfb21ca3), UINT64_C(0xdc0731d78f8795da),
	UINT64_C(0x536fa08fdfd90e51), UINT64_C(0x29b7d047efec8728),
};

void CRC64::Append(const void* data, const uint64 size)
{
	register const auto* mem = (const uint8*)data;
	register const auto* end = mem + size;
	register auto crc = m_crc;
	while (mem < end)
		crc = (crc >> 8) ^ CRCTable[*mem++ ^ (uint8)crc];
	m_crc = crc;
}

CRC64& CRC64::operator<<(const char* str)
{
	Append(str, strlen(str));
	return *this;
}

CRC64& CRC64::operator<<(const wchar_t* str)
{
	Append(str, sizeof(wchar_t) * wcslen(str));
	return *this;
}

CRC64& CRC64::operator<<(const std::string& str)
{
	Append(str.c_str(), str.length());
	return *this;
}

CRC64& CRC64::operator<<(const std::wstring& str)
{
	Append(str.c_str(), sizeof(wchar_t) * str.length());
	return *this;
}

//---------------------------------------------------------------------------

const uint64 StringCRC64(const char* txt)
{
	CRC64 crc;
	crc << txt;
	return crc.GetCRC();
}

const uint64 BufferCRC64(const void* data, const uint64 size)
{
	CRC64 crc;
	crc.Append(data, size);
	return crc.GetCRC();
}

//---------------------------------------------------------------------------
