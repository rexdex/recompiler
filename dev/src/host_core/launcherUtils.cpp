#include "build.h"
#include "launcherUtils.h"
#include "zlib\zlib.h"

static const uint32 ZLIB_CHUNK = 4096;

namespace launcher
{
	//---------------------------------------------------------------------------

	BinaryConsumer::BinaryConsumer(const void* data, const uint32 size)
		: m_data((const uint8*)data)
		, m_size(size)
		, m_offset(0)
	{
	}

	const uint32 BinaryConsumer::Read(void* outBuffer, const uint32 sizeToRead)
	{
		if (m_offset >= m_size)
			return 0;

		const uint32 dataLeft = (m_size - m_offset);
		const uint32 dataToCopy = (dataLeft < sizeToRead) ? dataLeft : sizeToRead;
		memcpy(outBuffer, m_data + m_offset, dataToCopy);
		m_offset += dataToCopy;

		return dataToCopy;
	}

	//---------------------------------------------------------------------------

	BinaryWriter::BinaryWriter(void* data, const uint32 maxSize)
		: m_data((uint8*)data)
		, m_offset(0)
		, m_size(maxSize)
	{
	}

	const uint32 BinaryWriter::Write(const void* data, const uint32 sizeToWrite)
	{
		if (m_offset >= m_size)
			return 0;

		const uint32 dataLeft = (m_size - m_offset);
		const uint32 dataToCopy = (dataLeft < sizeToWrite) ? dataLeft : sizeToWrite;
		memcpy(m_data + m_offset, data, dataToCopy);
		m_offset += dataToCopy;

		return dataToCopy;
	}

	//---------------------------------------------------------------------------

	static voidpf ZlibAlloc(voidpf opaque, uInt items, uInt size)
	{
		return malloc(items * size);
	}

	static void ZlibFree(voidpf opaque, voidpf address)
	{
		free(address);
	}

	bool DecompressData(const void* src, const uint32 srcSize, void* destData, uint32& destDataSize)
	{
		// allocate inflate state
		z_stream strm;
		memset(&strm, 0, sizeof(strm));
		strm.zalloc = &ZlibAlloc;
		strm.zfree = &ZlibFree;
		strm.opaque = Z_NULL;
		strm.avail_in = 0;
		strm.next_in = Z_NULL;
		int ret = inflateInit(&strm);
		if (ret != Z_OK)
			return false;

		// data transfer buffers
		BinaryConsumer reader(src, srcSize);
		BinaryWriter writer(destData, destDataSize);

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

	void MakeLower(char* dest, const uint32 destSize, const char* src)
	{
		char* destEnd = dest + destSize - 1;
		while (*src && (dest < destEnd))
		{
			*dest++ = (char)tolower(*src++);
		}
		*dest = 0;
	}

	void ConformPath(char* dest, const uint32 destSize, const char* src)
	{
		char* destEnd = dest + destSize - 1;
		while (*src && (dest < destEnd))
		{
			if (*src == '/')
			{
				*dest++ = '\\';
				src += 1;
			}
			else
			{
				*dest++ = (char)tolower(*src++);
			}
		}
		*dest = 0;
	}

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

} // utils