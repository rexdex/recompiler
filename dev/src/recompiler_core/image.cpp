#include "build.h"
#include "image.h"
#include "internalUtils.h"
#include "internalFile.h"

namespace image
{

	//---------------------------------------------------------------------------

	Section::Section()
		: m_name("none")
		, m_cpuName("none")
		, m_virtualOffset(0)
		, m_virtualSize(0)
		, m_physicalOffset(0)
		, m_physicalSize(0)
		, m_isReadable(false)
		, m_isWritable(false)
		, m_isExecutable(false)
	{
	}

	Section::Section(Binary* image,
		const char* name,
		const uint32 virtualAddress,
		const uint32 virtualSize,
		const uint32 physicalAddress,
		const uint32 physicalSize,
		const bool isReadable,
		const bool isWritable,
		const bool isExecutable,
		const char* cpuName)
		: m_name(name)
		, m_cpuName(cpuName)
		, m_virtualOffset(virtualAddress)
		, m_virtualSize(virtualSize)
		, m_physicalOffset(physicalAddress)
		, m_physicalSize(physicalSize)
		, m_isReadable(isReadable)
		, m_isWritable(isWritable)
		, m_isExecutable(isExecutable)
		, m_image(image)
	{
	}

	void Section::Save(IBinaryFileWriter& writer) const
	{
		FileChunk chunk(writer, "Section");
		writer << m_name;
		writer << m_virtualOffset;
		writer << m_virtualSize;
		writer << m_physicalOffset;
		writer << m_physicalSize;
		writer << m_isReadable;
		writer << m_isWritable;
		writer << m_isExecutable;
		writer << m_cpuName;
	}

	bool Section::Load(Binary* image, IBinaryFileReader& reader)
	{
		FileChunk chunk(reader, "Section");
		if (!chunk) 
			return false;

		reader >> m_name;
		reader >> m_virtualOffset;
		reader >> m_virtualSize;
		reader >> m_physicalOffset;
		reader >> m_physicalSize;
		reader >> m_isReadable;
		reader >> m_isWritable;
		reader >> m_isExecutable;
		reader >> m_cpuName;

		m_image = image;
		return true;
	}

	const bool Section::IsValidOffset(const uint32 offset) const
	{
		return (offset >= m_virtualOffset) && (offset < (m_virtualOffset + m_virtualSize));
	}

	//---------------------------------------------------------------------------

	Import::Import()
		: m_image(NULL)
		, m_exportIndex(0)
		, m_entryAddress(0)
		, m_tableAddress(0)
		, m_type(eImportType_Unknown)
	{
	}

	Import::Import(Binary* image, const uint32 index, const char* exportImageName, const uint64 tableAddress, const uint64 entryAddress, const EImportType type)
		: m_image(image)
		, m_exportIndex(index)
		, m_exportImageName(exportImageName)
		, m_entryAddress(entryAddress)
		, m_tableAddress(tableAddress)
		, m_type(type)
	{
	}

	Import::Import(Binary* image, const char* exportName, const char* exportImageName, const uint64 tableAddress, const uint64 entryAddress, const EImportType type)
		: m_image(image)
		, m_exportIndex(0)
		, m_exportName(exportName)
		, m_exportImageName(exportImageName)
		, m_entryAddress(entryAddress)
		, m_tableAddress(tableAddress)
		, m_type(type)
	{
	}

	void Import::Save(IBinaryFileWriter& writer) const
	{
		FileChunk chunk(writer, "Import", 2);
		writer << m_exportIndex;
		writer << m_exportName;
		writer << m_exportImageName;
		writer << m_tableAddress;
		writer << m_entryAddress;
		writer << (uint8)m_type;
	}

	bool Import::Load(Binary* image, IBinaryFileReader& reader)
	{
		FileChunk chunk(reader, "Import");
		if (!chunk)
			return false;

		reader >> m_exportIndex;
		reader >> m_exportName;
		reader >> m_exportImageName;

		if (chunk.GetVersion() < 2)
		{
			uint32 temp;
			reader >> temp;
			m_tableAddress = temp;
			reader >> temp;
			m_entryAddress = temp;
		}
		else
		{
			reader >> m_tableAddress;
			reader >> m_entryAddress;
		}

		uint8 type;
		reader >> type;
		m_type = (EImportType)type;
		m_image = image;

		return true;
	}

	//---------------------------------------------------------------------------

	Export::Export()
		: m_image(NULL)
		, m_index(0)
		, m_entryPointAddress(0)
		, m_forwardedImport(NULL)
	{
	}

	Export::Export(Binary* image, const uint32 index, const char* name, const uint64 entryPointAddress)
		: m_image(image)
		, m_index(index)
		, m_name(name)
		, m_entryPointAddress(entryPointAddress)
		, m_forwardedImport(NULL)
	{
	}

	Export::Export(Binary* image, const uint32 index, const char* name, Import* forwardedImport)
		: m_image(image)
		, m_index(index)
		, m_name(name)
		, m_entryPointAddress(0)
		, m_forwardedImport(forwardedImport)
	{
	}

	Export::~Export()
	{
		if (m_forwardedImport != NULL)
		{
			delete m_forwardedImport;
			m_forwardedImport = NULL;
		}
	}

	void Export::Save(IBinaryFileWriter& writer) const
	{
		FileChunk chunk(writer, "Export");
		writer << m_index;
		writer << m_name;
		writer << m_entryPointAddress;
		//writer << m_forwardedImport;
	}

	bool Export::Load(Binary* image, IBinaryFileReader& reader)
	{
		FileChunk chunk(reader, "Export");
		if (!chunk)
			return false;

		reader >> m_index;
		reader >> m_name;
		reader >> m_entryPointAddress;
		//reader >> m_forwardedImport;
		m_image = image;
		return true;
	}

	//---------------------------------------------------------------------------

	Symbol::Symbol()
		: m_image(nullptr)
		, m_index(0)
		, m_address(0)
		, m_type(0)
	{
	}

	Symbol::Symbol(Binary* image, const uint32 index, const ESymbolType type, const uint32 address, const char* name, const char* object)
		: m_image(image)
		, m_index(index)
		, m_type((uint8)type)
		, m_address(address)
		, m_name(name)
		, m_object(object)
	{
	}

	Symbol::~Symbol()
	{
	}

	void Symbol::Save(IBinaryFileWriter& writer) const
	{
		FileChunk chunk(writer, "Symbol");
		writer << m_index;
		writer << m_address;
		writer << m_type;
		writer << m_name;
		writer << m_object;
	}

	bool Symbol::Load(Binary* image, IBinaryFileReader& reader)
	{
		FileChunk chunk(reader, "Symbol");
		if (!chunk) 
			return false;

		reader >> m_index;
		reader >> m_address;
		reader >> m_type;
		reader >> m_name;
		reader >> m_object;
		m_image = image;
		return true;
	}

	//---------------------------------------------------------------------------

	Binary::Binary()
		: m_memorySize(0)
		, m_memoryData(NULL)
		, m_baseAddress(0)
		, m_entryAddress(0)
		, m_crc(0)
	{
	}

	Binary::~Binary()
	{
		DeleteVector(m_sections);
		DeleteVector(m_exports);
		DeleteVector(m_imports);
		DeleteVector(m_symbols);

		delete[] m_memoryData;
	}

	template <typename T >
	void SaveArray(const std::vector<T*>& theArray, IBinaryFileWriter& writer)
	{
		FileChunk chunk(writer, "Array");
	
		writer << (uint32)theArray.size();
		for (const auto* ptr : theArray)
			ptr->Save(writer);
	}

	void Binary::Save(class ILogOutput& log, IBinaryFileWriter& writer) const
	{
		FileChunk chunk(writer, "Image", 4);

		// general stuff
		writer << m_path;
		writer << m_memorySize;
		writer << m_baseAddress;
		writer << m_entryAddress;
		writer << m_crc;

		// save tables
		SaveArray(m_symbols, writer);
		SaveArray(m_imports, writer);
		SaveArray(m_exports, writer);
		SaveArray(m_sections, writer);

		// image data
		{
			FileChunk chunk2(writer, "Memory");
			writer.Save(m_memoryData, m_memorySize);
		}
	}

	template <typename T >
	bool LoadArray(Binary* image, std::vector<T*>& outArray, IBinaryFileReader& reader)
	{
		FileChunk chunk(reader, "Array");
		if (!chunk)
			return false;

		uint32 count = 0;
		reader >> count;

		outArray.reserve(count);
		for (uint32 i = 0; i < count; ++i)
		{
			auto* ptr = new T();
			if (!ptr->Load(image, reader))
				return false;
			outArray.push_back(ptr);
		}

		return true;
	}

	bool Binary::Load(class ILogOutput& log, IBinaryFileReader& reader)
	{
		FileChunk chunk(reader, "Image");
		if (!chunk) 
			return false;

		// general stuff
		reader >> m_path;
		reader >> m_memorySize;

		// addresses
		if (chunk.GetVersion() < 2)
		{
			uint32 temp;
			reader >> temp;
			m_baseAddress = temp;

			reader >> temp;
			m_entryAddress = temp;
		}
		else
		{
			reader >> m_baseAddress;
			reader >> m_entryAddress;
		}

		// crc
		if (chunk.GetVersion() >= 4)
			reader >> m_crc;

		// load tables
		if (!LoadArray(this, m_symbols, reader))
			return false;
		if (!LoadArray(this, m_imports, reader))
			return false;
		if (!LoadArray(this, m_exports, reader))
			return false;
		if (!LoadArray(this, m_sections, reader))
			return false;

		// image data
		{
			FileChunk chunk2(reader, "Memory");
			if (!chunk) return false;

			// allocate memory
			m_memoryData = new uint8[m_memorySize];
			if (!m_memoryData)
			{
				log.Error("Image: Failed to allocate %u bytes for image data", m_memorySize);
				return false;
			}

			reader.Load((void*)m_memoryData, m_memorySize);
		}

		// reload crc
		if (0 == m_crc)
			m_crc = BufferCRC64(m_memoryData, m_memorySize);

		return true;
	}

	const bool Binary::IsValidOffset(const uint32 offset) const
	{
		if (offset < m_memorySize)
		{
			return true;
		}

		return false;
	}

	const bool Binary::IsValidAddress(const uint32 address) const
	{
		if (address >= m_baseAddress && address < (m_baseAddress + m_memorySize))
		{
			return true;
		}

		return false;
	}

	const Section* Binary::FindSectionForOffset(const uint32 offset) const
	{
		if (offset < m_memorySize)
		{
			const uint32 numSections = GetNumSections();
			for (uint32 i = 0; i < numSections; ++i)
			{
				const Section* section = GetSection(i);
				if (section->IsValidOffset(offset))
				{
					return section;
				}
			}
		}

		// nothing found
		return nullptr;
	}

	const Section* Binary::FindSectionForAddress(const uint64 address) const
	{
		if (address >= m_baseAddress && address < (m_baseAddress + m_memorySize))
		{
			const auto offset = address - m_baseAddress;
			if (offset < m_memorySize)
			{
				return FindSectionForOffset((uint32)offset);
			}
		}

		// nothing found
		return nullptr;
	}

	//---------------------------------------------------------------------------

} // image