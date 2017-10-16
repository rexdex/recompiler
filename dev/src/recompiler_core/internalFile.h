#pragma once

//---------------------------------------------------------------------------

/// Binary file writer, used to save the project data
class RECOMPILER_API IBinaryFileWriter
{
public:
	virtual ~IBinaryFileWriter() {};

	// save raw data
	virtual void Save(const void* data, const uint32 size) = 0;

	// get file size
	virtual uint64 GetSize() const = 0;

	// skipable block support
	virtual void BeginBlock(const char* chunkName, const uint32 version) = 0;
	virtual void FinishBlock() = 0;

	// stream operators
	inline IBinaryFileWriter& operator<<(const uint8& data) { Save(&data, sizeof(data)); return *this; }
	inline IBinaryFileWriter& operator<<(const uint16& data) { Save(&data, sizeof(data)); return *this; }
	inline IBinaryFileWriter& operator<<(const uint32& data) { Save(&data, sizeof(data)); return *this; }
	inline IBinaryFileWriter& operator<<(const uint64& data) { Save(&data, sizeof(data)); return *this; }
	inline IBinaryFileWriter& operator<<(const int8& data) { Save(&data, sizeof(data)); return *this; }
	inline IBinaryFileWriter& operator<<(const int16& data) { Save(&data, sizeof(data)); return *this; }
	inline IBinaryFileWriter& operator<<(const int32& data) { Save(&data, sizeof(data)); return *this; }
	inline IBinaryFileWriter& operator<<(const int64& data) { Save(&data, sizeof(data)); return *this; }

	// save boolean 
	inline IBinaryFileWriter& operator<<(const bool& data)
	{
		uint8 val = data ? 1 : 0;
		Save(&val, sizeof(val));
		return *this;
	}

	// save string
	inline IBinaryFileWriter& operator<<(const std::string& data)
	{
		const uint32 len = (uint32)data.length();
		Save(&len, sizeof(len));
		if (len > 0)
		{
			Save(&data[0], sizeof(data[0]) * len);
		}
		return *this;
	}

	// save wstring
	inline IBinaryFileWriter& operator<<(const std::wstring& data)
	{
		const uint32 len = (uint32)data.length();
		Save(&len, sizeof(len));
		if (len > 0)
		{
			Save(&data[0], sizeof(data[0]) * len);
		}
		return *this;
	}

	// save vector of pointers
	template< typename T >
	inline IBinaryFileWriter& operator<<(const std::vector<T*>& data)
	{
		uint32 len = (uint32)data.size();
		Save(&len, sizeof(len));
		if (len > 0)
		{
			for (uint32 i = 0; i < data.size(); ++i)
			{
				T* ptr = data[i];
				if (ptr != NULL)
				{
					*this << ptr;
				}
			}
		}
		return *this;
	}
	// save plain vector
	template< typename T >
	inline IBinaryFileWriter& operator<<(const std::vector<T>& data)
	{
		uint32 len = (uint32)data.size();
		Save(&len, sizeof(len));
		if (len > 0)
		{
			for (uint32 i = 0; i < data.size(); ++i)
			{
				*this << data[i];
			}
		}
		return *this;
	}

	// save plain map
	template< typename K, typename V >
	inline IBinaryFileWriter& operator<<(const std::map<K, V>& data)
	{
		const uint32 len = (uint32)data.size();
		Save(&len, sizeof(len));
		if (len > 0)
		{
			for (std::map<K, V>::const_iterator it = data.begin();
				it != data.end(); ++it)
			{
				*this << (*it).first;
				*this << (*it).second;
			}
		}
		return *this;
	}

	// save plain map
	template< typename K >
	inline IBinaryFileWriter& operator<<(const std::unordered_set<K>& data)
	{
		const uint32 len = (uint32)data.size();
		Save(&len, sizeof(len));
		if (len > 0)
		{
			for (auto it = data.begin(); it != data.end(); ++it)
				*this << (*it);
		}
		return *this;
	}

	// save pointer
	template< typename T >
	inline IBinaryFileWriter& operator<<(T* data)
	{
		// NULL/ NotNull flag
		const uint8 flag = data != 0;
		Save(&flag, sizeof(flag));

		// object data
		if (flag)
		{
			data->Save(*this);
		}

		return *this;
	}
};

//---------------------------------------------------------------------------

/// Binary file reader, used to load the project data
class RECOMPILER_API IBinaryFileReader
{
public:
	virtual ~IBinaryFileReader() {};

	// load raw data
	virtual void Load(void* data, const uint32 size) = 0;

	// skipable block support
	virtual bool EnterBlock(const char* chunkName, uint32& outVersion) = 0;
	virtual void LeaveBlock() = 0;
	
	// stream operators
	inline IBinaryFileReader& operator >> (uint8& data) { Load(&data, sizeof(data)); return *this; }
	inline IBinaryFileReader& operator >> (uint16& data) { Load(&data, sizeof(data)); return *this; }
	inline IBinaryFileReader& operator >> (uint32& data) { Load(&data, sizeof(data)); return *this; }
	inline IBinaryFileReader& operator >> (uint64& data) { Load(&data, sizeof(data)); return *this; }
	inline IBinaryFileReader& operator >> (int8& data) { Load(&data, sizeof(data)); return *this; }
	inline IBinaryFileReader& operator >> (int16& data) { Load(&data, sizeof(data)); return *this; }
	inline IBinaryFileReader& operator >> (int32& data) { Load(&data, sizeof(data)); return *this; }
	inline IBinaryFileReader& operator >> (int64& data) { Load(&data, sizeof(data)); return *this; }

	// load boolean 
	inline IBinaryFileReader& operator >> (bool& data)
	{
		uint8 val = 0;
		Load(&val, sizeof(val));
		data = (val != 0);
		return *this;
	}

	// load string
	IBinaryFileReader& operator >>(std::string& data);

	// load wstring
	IBinaryFileReader& operator >>(std::wstring& data);

	// load vector of pointers
	template< typename T >
	inline IBinaryFileReader& operator >> (std::vector<T*>& data)
	{
		uint32 len = 0;
		Load(&len, sizeof(len));
		for (uint32 i = 0; i < len; ++i)
		{
			T* ptr = NULL;
			*this >> ptr;

			if (ptr != NULL)
			{
				data.push_back(ptr);
			}
		}
		return *this;
	}

	// load plain vector
	template< typename T >
	inline IBinaryFileReader& operator >> (std::vector<T>& data)
	{
		uint32 len = 0;
		Load(&len, sizeof(len));
		data.resize(len);
		for (uint32 i = 0; i < len; ++i)
		{
			*this >> data[i];
		}
		return *this;
	}

	// load plain map
	template< typename K, typename V >
	inline IBinaryFileReader& operator >> (std::map<K, V>& data)
	{
		uint32 len = 0;
		Load(&len, sizeof(len));
		for (uint32 i = 0; i < len; ++i)
		{
			K key;
			V value;
			*this >> key;
			*this >> value;
			data[key] = value;
		}
		return *this;
	}

	// load plain map
	template< typename K>
	inline IBinaryFileReader& operator >> (std::unordered_set<K>& data)
	{
		uint32 len = 0;
		Load(&len, sizeof(len));
		data.reserve(len);
		for (uint32 i = 0; i < len; ++i)
		{
			K key;
			*this >> key;
			data.insert(key);
		}
		return *this;
	}
};

//---------------------------------------------------------------------------

/// Ensure that the path exists
extern RECOMPILER_API bool CreateFilePath(const wchar_t* path);

/// Create interface for writing to binary files
extern RECOMPILER_API std::shared_ptr<IBinaryFileWriter> CreateFileWriter(const wchar_t* path, const bool compressed = false);

/// Create interface for reading form binary files
extern RECOMPILER_API std::shared_ptr<IBinaryFileReader> CreateFileReader(const wchar_t* path);

//---------------------------------------------------------------------------

/// Chunk scope
class RECOMPILER_API FileChunk
{
public:
	FileChunk(IBinaryFileWriter& writer, const char* chunkName, const uint32 version=1)
		: m_writer(&writer)
		, m_reader(NULL)
		, m_version(version)
	{
		writer.BeginBlock(chunkName, version);
		m_valid = true;
	}

	FileChunk(IBinaryFileReader& reader, const char* chunkName)
		: m_writer(NULL)
		, m_reader(&reader)
		, m_version(0)
	{
		m_valid = reader.EnterBlock(chunkName, m_version);
	}

	~FileChunk()
	{
		if (m_valid)
		{
			if (m_writer != NULL)
				m_writer->FinishBlock();

			if (m_reader != NULL)
				m_reader->LeaveBlock();
		}
	}

	inline operator bool() const
	{
		return m_valid;
	}

	inline const uint32 GetVersion() const
	{
		return m_version;
	}

private:
	IBinaryFileWriter* m_writer;
	IBinaryFileReader* m_reader;

	bool m_valid;
	uint32 m_version;
};

//---------------------------------------------------------------------------
