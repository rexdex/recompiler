#include "build.h"
#include "internalUtils.h"
#include "internalFile.h"

//---------------------------------------------------------------------------

struct ChunkInfo
{
	static const uint32 MAGIC = 'CHNK';
		
	uint32 m_magic;
	uint32 m_version;
	uint64 m_crc; // crc of the data
	uint64 m_size; // allows to skip the data, computed from the positon after the header

	inline ChunkInfo()
		: m_magic(0)
		, m_version(0)
		, m_crc(0)
		, m_size(0)
	{}
};

//---------------------------------------------------------------------------

// load string
IBinaryFileReader& IBinaryFileReader::operator >> (std::string& data)
{
	uint32 len = 0;
	Load(&len, sizeof(len));
	if (len > 0)
	{
		if (len < 1024)
		{
			char* str = (char*)alloca(len + 1);
			Load(str, len);
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
IBinaryFileReader& IBinaryFileReader::operator >> (std::wstring& data)
{
	uint32 len = 0;
	Load(&len, sizeof(len));
	if (len > 0)
	{
		if (len < 1024)
		{
			wchar_t* str = (wchar_t*)alloca(sizeof(wchar_t)*(len + 1));
			Load(str, sizeof(wchar_t)*len);
			str[len] = 0;
			data = str;
		}
		else
		{
			wchar_t* str = (wchar_t*)malloc(sizeof(wchar_t)*(len + 1));
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

//---------------------------------------------------------------------------

/// ANSI C file writer implementation
class NativeFileWriter : public IBinaryFileWriter
{
public:
	NativeFileWriter(FILE* file)
		: m_file(file)
		, m_error(false)
	{}

	virtual ~NativeFileWriter() override final
	{
		if (m_file != NULL)
		{
			fclose(m_file);
			m_file = NULL;
		}
	}

	virtual uint64 GetSize() const  override final
	{
		return ftell(m_file);
	}

	virtual void BeginBlock(const char* chunkName, const uint32 version) override final
	{
		if (!m_error)
		{
			BackEntry entry;
			entry.m_chunkInfoPos = ftell(m_file);
			entry.m_header.m_magic = ChunkInfo::MAGIC;
			entry.m_header.m_crc = StringCRC64(chunkName);
			entry.m_header.m_version = version;
			entry.m_header.m_size = 0;

			// save the chunk magic (so we know what we are saving)
			Save(&entry.m_header, sizeof(entry.m_header));

			// remember current file offset
			entry.m_dataPos = ftell(m_file);
			m_chunks.push_back(entry);
		}
	}

	virtual void FinishBlock() override final
	{
		if (m_chunks.empty())
			return;

		if (!m_error)
		{
			// get the block skip offset position
			auto data = m_chunks.back();
			m_chunks.pop_back();

			// save the skip length
			const uint32 currentOffset = ftell(m_file);
			fseek(m_file, data.m_chunkInfoPos, SEEK_SET);

			data.m_header.m_size = (uint32)(currentOffset - data.m_dataPos);
			Save(&data.m_header, sizeof(data.m_header));

			fseek(m_file, currentOffset, SEEK_SET);
		}
	}

	virtual void Save(const void* data, const uint32 size) override final
	{
		if (!m_error)
		{
			if (1 != fwrite(data, size, 1, m_file))
			{
				fclose(m_file);
				m_file = NULL;
				m_error = true;
			}
		}
	}

private:
	FILE*		m_file;
	bool		m_error;

	struct BackEntry
	{
		uint32 m_chunkInfoPos;
		uint32 m_dataPos;
		ChunkInfo m_header;
	};

	std::vector<BackEntry> m_chunks;
};

std::shared_ptr<IBinaryFileWriter> CreateFileWriter(const wchar_t* path, const bool compressed /*= false*/)
{
	// create path
	if (!CreateFilePath(path))
		return nullptr;

	// open file
	FILE* f = NULL;
	_wfopen_s(&f, path, L"wb");
	if (!f)
		return nullptr;

	// create wrapper
	return std::shared_ptr<IBinaryFileWriter>(new NativeFileWriter(f));
}

//---------------------------------------------------------------------------

/// ANSI C file reader implementation
class NativeFileReader : public IBinaryFileReader
{
public:
	NativeFileReader(FILE* file)
		: m_file(file)
		, m_error(false)
	{}

	virtual ~NativeFileReader() override final
	{
		if (m_file != NULL)
		{
			fclose(m_file);
			m_file = NULL;
		}
	}

	virtual void Load(void* data, const uint32 size) override final
	{
		if (!m_error)
		{
			if (1 != fread(data, size, 1, m_file))
			{
				m_error = true;
			}
		}
	}

	virtual bool EnterBlock(const char* chunkName, uint32& outVersion) override final
	{
		// error
		if (m_error)
			return false;

		// original position
		const auto startPos = ftell(m_file);

		// compute the CRC
		const auto chunkCRC = StringCRC64(chunkName);

		// look for chunk
		const auto prevError = m_error;
		while (!m_error)
		{
			// read the chunk info
			ChunkInfo info;
			Load(&info, sizeof(info));

			// not a chunk ?
			if (info.m_magic != ChunkInfo::MAGIC)
			{
				if (m_file)
					fseek(m_file, startPos, SEEK_SET);
				break;
			}

			// is this the chunk we are looking for ?
			if (info.m_crc == chunkCRC)
			{
				const auto pos = ftell(m_file);
				outVersion = info.m_version;
				m_currentBlockEnd.push_back(pos + info.m_size);
				return true;
			}

			// skip this chunk
			fseek(m_file, (uint32)info.m_size, SEEK_CUR);
		}

		// chunk not found
		fseek(m_file, startPos, SEEK_CUR);
		m_error = prevError;
		return false;
	}

	virtual void LeaveBlock() override final
	{
		if (m_currentBlockEnd.empty())
		{
			m_error = true;
			return;
		}

		// go to the end position of block
		const auto endPos = m_currentBlockEnd.back();
		m_currentBlockEnd.pop_back();
		fseek(m_file, (uint32)endPos, SEEK_SET);
	}

private:
	FILE*		m_file;
	bool		m_error;

	std::vector<uint64>	m_currentBlockEnd;
};

std::shared_ptr<IBinaryFileReader> CreateFileReader(const wchar_t* path)
{
	// open file
	FILE* f = NULL;
	_wfopen_s(&f, path, L"rb");
	if (!f)
		return nullptr;

	// create wrapper
	return std::shared_ptr<IBinaryFileReader>(new NativeFileReader(f));
}

//---------------------------------------------------------------------------