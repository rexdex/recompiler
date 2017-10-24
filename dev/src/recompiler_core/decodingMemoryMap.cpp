#include "build.h"
#include "decodingMemoryMap.h"
#include "internalFile.h"
#include "image.h"

namespace decoding
{

	MemoryMap::MemoryMap()
		: m_memoryMap(NULL)
		, m_memorySize(0)
		, m_baseAddress(0)
		, m_lastDirtyAddress(0)
		, m_firstDirtyAddress(0)
		, m_isModified(false)
	{
	}

	MemoryMap::~MemoryMap()
	{
		if (NULL != m_memoryMap)
		{
			free(m_memoryMap);
			m_memoryMap = NULL;
		}
	}

	void MemoryMap::Save(ILogOutput& log, IBinaryFileWriter& writer) const
	{
		FileChunk chunk(writer, "MemoryMap");

		// props
		writer << m_baseAddress;
		writer << m_memorySize;

		// data
		{
			FileChunk chunk2(writer, "Data");
			writer.Save(m_memoryMap, sizeof(Entry) * m_memorySize);
		}

		// block map
		{
			FileChunk chunk2(writer, "BlockRoots");
			writer << m_blockRoots;
		}

		// function map
		{
			FileChunk chunk2(writer, "FunctionRoots");
			writer << m_functionRoots;
		}

		// validate
		m_isModified = false;
	}

	bool MemoryMap::Load(ILogOutput& log, IBinaryFileReader& reader)
	{
		FileChunk chunk(reader, "MemoryMap");
		if (!chunk)
			return false;

		// props
		uint64 baseAddress = 0;
		uint32 memorySize = 0;
		reader >> baseAddress;
		reader >> memorySize;
		if (!memorySize)
			return false;

		// create memory
		auto* mem = (Entry*)malloc(sizeof(Entry) * memorySize);

		// setup
		m_baseAddress = baseAddress;
		m_memorySize = memorySize;
		m_memoryMap = mem;

		// data
		{
			FileChunk chunk2(reader, "Data");
			if (chunk)
			{
				reader.Load(m_memoryMap, sizeof(Entry) * m_memorySize);
			}
			else
			{
				memset(m_memoryMap, 0, sizeof(Entry) * m_memorySize);
			}
		}

		// block map
		{
			FileChunk chunk2(reader, "BlockRoots");
			if (chunk2)
			{
				reader >> m_blockRoots;
			}
			else
			{
				uint64 prevAddress = 0;
				const auto end = m_baseAddress + m_memorySize;
				for (auto addr = m_baseAddress; addr < end; ++addr)
				{
					const auto info = GetMemoryInfo(addr);
					if (info.IsExecutable())
					{
						if (info.GetInstructionFlags().IsBlockStart())
						{
							if (addr > (prevAddress + 1))
								m_blockRoots.insert(addr);
							prevAddress = addr;
						}
					}
				}
			}
		}

		// function map
		{
			FileChunk chunk2(reader, "FunctionRoots");
			if (chunk2)
			{
				reader >> m_functionRoots;
			}
			else
			{
				uint64 prevAddress = 0;
				const auto end = m_baseAddress + m_memorySize;
				for (auto addr = m_baseAddress; addr < end; ++addr)
				{
					const auto info = GetMemoryInfo(addr);
					if (info.IsExecutable())
					{
						if (info.GetInstructionFlags().IsFunctionStart())
						{
							if (addr > (prevAddress + 1))
								m_functionRoots.insert(addr);
							prevAddress = addr;
						}
					}
				}
			}
		}

		// validate
		m_lastDirtyAddress = 0;
		m_firstDirtyAddress = 0;
		m_isModified = false;
		return true;
	}

	void MemoryMap::Validate()
	{
		m_lastDirtyAddress = 0;
		m_firstDirtyAddress = 0;
	}

	void MemoryMap::InvalidateRange(const uint64 rva, const uint64 size)
	{
		if (rva >= m_baseAddress && (rva + size) < (m_baseAddress + m_memorySize) && size > 0)
		{
			if (m_lastDirtyAddress == m_firstDirtyAddress)
			{
				m_firstDirtyAddress = rva;
				m_lastDirtyAddress = rva + size;
			}
			else
			{
				if (rva < m_firstDirtyAddress)
				{
					m_firstDirtyAddress = rva;
				}

				if (rva + size > m_lastDirtyAddress)
				{
					m_lastDirtyAddress = rva + size;
				}
			}

			m_isModified = true;
		}
	}

	bool MemoryMap::GetDirtyRange(uint64& outFirstDirtyAddressRVA, uint64& outLastDirtyAddressRVA) const
	{
		if (m_lastDirtyAddress > m_firstDirtyAddress)
		{
			outFirstDirtyAddressRVA = m_firstDirtyAddress;
			outLastDirtyAddressRVA = m_lastDirtyAddress;
			return true;
		}

		return false;
	}

	bool MemoryMap::Initialize(ILogOutput& log, const image::Binary& baseImage)
	{
		// setup members
		m_baseAddress = baseImage.GetBaseAddress();
		m_memorySize = baseImage.GetMemorySize();

		// create the initial memory map
		m_memoryMap = (Entry*)malloc(sizeof(Entry) * m_memorySize);
		if (!m_memoryMap)
		{
			return false;
		}

		// clean initial memory
		memset(m_memoryMap, 0, sizeof(Entry) * m_memorySize);

		// set the initial section values
		const uint32 numSections = baseImage.GetNumSections();
		for (uint32 i = 0; i < numSections; ++i)
		{
			const image::Section* section = baseImage.GetSection(i);
			const uint32 sectionOffset = section->GetVirtualOffset();
			const uint32 sectionSize = section->GetVirtualSize();

			// non-readable sections are not mapped here
			if (!section->CanRead())
			{
				continue;
			}

			// executable sections are code :)
			uint32 flags = (uint32)MemoryFlag::Valid;
			if (section->CanExecute()) flags |= (uint32)MemoryFlag::Executable;
			if (!section->CanWrite()) flags |= (uint32)MemoryFlag::ReadOnly;

			// data section ? VERY naive ATM
			if (nullptr != strstr(section->GetName().c_str(), "data"))
			{
				flags |= (uint32)MemoryFlag::GenericData;
			}

			// stats
			log.SetTaskName("Preparing section '%s'...", section->GetName().c_str());

			// setup the sections
			if (section->CanExecute())
			{
				// code
				Entry* ptr = m_memoryMap + sectionOffset;
				for (uint32 j = 0; j < sectionSize; j += 4, ptr += 4)
				{
					log.SetTaskProgress(j, sectionSize);

					ptr[0].m_flags = flags | (uint32)MemoryFlag::FirstByte;
					ptr[0].m_size = 4;
					ptr[1].m_flags = flags;
					ptr[1].m_size = 4;
					ptr[2].m_flags = flags;
					ptr[2].m_size = 4;
					ptr[3].m_flags = flags;
					ptr[3].m_size = 4;
				}
			}
			else
			{
				// generic data
				Entry* ptr = m_memoryMap + sectionOffset;
				for (uint32 j = 0; j < sectionSize; j += 1, ptr += 1)
				{
					log.SetTaskProgress(j, sectionSize);

					ptr[0].m_flags = flags | (uint32)MemoryFlag::FirstByte;
					ptr[0].m_size = 1;
				}
			}

			// set the "section start" flag for each started section
			m_memoryMap[sectionOffset].m_flags |= (uint32)MemoryFlag::SectionStart;
		}

		// set the entry point flags (the only known location in the whole file so far)
		SetMemoryBlockType(log, baseImage.GetEntryAddress(), (uint32)MemoryFlag::Executable, 0);
		SetMemoryBlockSubType(log, baseImage.GetEntryAddress(), (uint32)InstructionFlag::EntryPoint | (uint32)InstructionFlag::StaticCallTarget | (uint32)InstructionFlag::BlockStart | (uint32)InstructionFlag::FunctionStart, 0);

		// initialized
		return true;
	}

	const MemoryFlags MemoryMap::GetMemoryInfo(const uint64 rva) const
	{
		if (rva >= m_baseAddress && rva < (m_baseAddress + m_memorySize))
		{
			const Entry& entry = m_memoryMap[rva - m_baseAddress];
			return MemoryFlags(rva, entry.m_size, entry.m_flags, entry.m_specyficFlags);
		}
		else
		{
			return MemoryFlags(rva, 0, 0, 0);
		}
	}

	bool MemoryMap::SetMemoryBlockLength(ILogOutput& log, const uint64 rva, const uint64 size)
	{
		if (rva >= m_baseAddress && (rva + size) < (m_baseAddress + m_memorySize))
		{
			// get first entry
			Entry* entry = &m_memoryMap[rva - m_baseAddress];

			// we MUST point to the first byte here
			if (!entry->HasFlag(MemoryFlag::FirstByte))
			{
				log.Error("RVA %08Xh is not the first byte of memory block", rva);
				return false;
			}

			// first convert all the touching blocks to bytes
			const auto start = rva - m_baseAddress;

			// see if our action is going to destroy any existing block
			uint64 numBytesAffected = size;
			if (entry->m_size > size)
			{
				// we will affect more bytes because the current ocupant is larger
				numBytesAffected = entry->m_size;
			}
			else if (size > entry->m_size)
			{
				// we wat to create something larger than the current occupant, break up all tochuing block
				numBytesAffected = size;
				for (uint64 i = 0; i < size; ++i)
				{
					const Entry& otherEntry = m_memoryMap[start + i];
					if (otherEntry.HasFlag(MemoryFlag::FirstByte))
					{
						numBytesAffected = i + otherEntry.m_size;
					}
				}
			}

			// reset all the affected bytes to one block 
			for (uint64 i = size; i < numBytesAffected; ++i)
			{
				Entry& brokenEntries = m_memoryMap[start + i];
				brokenEntries.SetFlag(MemoryFlag::FirstByte);
				brokenEntries.m_size = 1;
			}

			// set the new size of the actual block
			entry->m_size = size;
			entry->SetFlag(MemoryFlag::FirstByte);

			// mark the rest of the bytes as part of the block
			for (uint64 i = 1; i < size; ++i)
			{
				Entry& brokenEntries = m_memoryMap[start + i];
				brokenEntries.ClearFlag(MemoryFlag::FirstByte);
				brokenEntries.m_size = size;
			}

			// invalidate the memory range
			InvalidateRange(rva, numBytesAffected);
			return true;
		}
		else
		{
			log.Error("RVA %08Xh (size %u) is outside the range of image memory map", rva, size);
			return false;
		}
	}

	bool MemoryMap::SetMemoryBlockType(ILogOutput& log, const uint64 rva, const uint32 setFlags, const uint32 clearFlags)
	{
		if (rva >= m_baseAddress && rva < (m_baseAddress + m_memorySize))
		{
			// check the first byte flag
			Entry* entry = &m_memoryMap[rva - m_baseAddress];
			if (!entry->HasFlag(MemoryFlag::FirstByte))
			{
				//log.Error( "Address %08Xh is not the first byte of memory block", rva );
				return false;
			}

			// set the flag on all of the entries
			const uint32 size = entry->m_size;
			for (uint32 i = 0; i < size; ++i, ++entry)
			{
				entry->m_flags |= setFlags;
				entry->m_flags &= ~clearFlags;
			}

			// invalidate the memory range
			InvalidateRange(rva, size);

			// done
			return true;
		}
		else
		{
			log.Error("Address %08llXh is outside the memory map range (0x%08llX-0x%08llX)", rva, m_baseAddress, m_baseAddress+m_memorySize);
			return false;
		}
	}

	bool MemoryMap::SetMemoryBlockSubType(ILogOutput& log, const uint64 rva, const uint32 setFlags, const uint32 clearFlags)
	{
		if (rva >= m_baseAddress && rva < (m_baseAddress + m_memorySize))
		{
			// check the first byte flag
			Entry* entry = &m_memoryMap[rva - m_baseAddress];
			if (!entry->HasFlag(MemoryFlag::FirstByte))
			{
				log.Error("Address %08Xh is not the first byte of memory block", rva);
				return false;
			}

			// set the flag on all of the entries
			const uint32 size = entry->m_size;
			for (uint32 i = 0; i < size; ++i, ++entry)
			{
				entry->m_specyficFlags |= setFlags;
				entry->m_specyficFlags &= ~clearFlags;
			}

			// update function/block stuff
			if (entry->HasFlag(MemoryFlag::Executable))
			{
				if (setFlags & (uint32)InstructionFlag::BlockStart)
				{
					m_blockRoots.insert(rva);
				}
				else if (clearFlags& (uint32)InstructionFlag::BlockStart)
				{
					m_blockRoots.erase(rva);
				}

				if (setFlags & (uint32)InstructionFlag::FunctionStart)
				{
					m_functionRoots.insert(rva);
				}
				else if (clearFlags& (uint32)InstructionFlag::FunctionStart)
				{
					m_functionRoots.erase(rva);
				}
			}

			// invalidate the memory range
			InvalidateRange(rva, size);

			// done
			return true;
		}
		else
		{
			log.Error("Address %08llXh is outside the memory map range (0x%08llX-0x%08llX)", rva, m_baseAddress, m_baseAddress + m_memorySize);
			return false;
		}
	}

} // decoding