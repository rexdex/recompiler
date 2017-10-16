#include "build.h"
#include "decodingMemoryMap.h"
#include "decodingNameMap.h"
#include "internalFile.h"

namespace decoding
{

	//-------------------------------------------------------------------------

	NameMap::NameMap(class MemoryMap* map)
		: m_memoryMap(map)
		, m_isModified(false)
	{
	}

	NameMap::~NameMap()
	{
	}

	const char* NameMap::GetName(const uint64 addr) const
	{
		// read comment data
		TNameMap::const_iterator it = m_nameMap.find(addr);
		if (it != m_nameMap.end())
		{
			return (*it).second.c_str();
		}

		// no name stored for address
		return "";
	}

	void NameMap::SetName(const uint64 addr, const char* comment)
	{
		if (m_memoryMap->IsValidAddress(addr))
		{
			if (!comment || !comment[0])
			{
				TNameMap::const_iterator it = m_nameMap.find(addr);
				if (it != m_nameMap.end())
				{
					m_nameMap.erase(it);
					m_isModified = true;
				}
			}
			else
			{
				// modify only when needed
				TNameMap::const_iterator it = m_nameMap.find(addr);
				if (it == m_nameMap.end() || (*it).second != comment)
				{
					m_nameMap[addr] = comment;
					m_isModified = true;
				}
			}
		}
	}

	void NameMap::Save(ILogOutput& log, class IBinaryFileWriter& writer) const
	{
		FileChunk chunk(writer, "NameMap", 2);
		writer << m_nameMap;
		m_isModified = false;
	}

	bool NameMap::Load(ILogOutput& log, class IBinaryFileReader& reader)
	{
		FileChunk chunk(reader, "NameMap");
		if (!chunk)
			return false;

		if (chunk.GetVersion() < 2)
		{
			std::map< uint32, std::string > oldData;
			reader >> oldData;

			for (const auto& it : oldData)
				m_nameMap[it.first] = it.second;
		}
		else
		{
			reader >> m_nameMap;
		}

		m_isModified = false;
		return true;
	}

	void NameMap::EnumerateSymbols(const char* nameFilter, ISymbolConsumer& reporter) const
	{
		for (TNameMap::const_iterator it = m_nameMap.begin();
			it != m_nameMap.end(); ++it)
		{
			const char* symbolName = it->second.c_str();
			if (strstr(symbolName, nameFilter) != NULL)
			{
				SymbolInfo info;
				info.m_address = it->first;
				info.m_name = symbolName;
				reporter.ProcessSymbols(&info, 1);
			}
		}
	}

	//-------------------------------------------------------------------------

} // decoding