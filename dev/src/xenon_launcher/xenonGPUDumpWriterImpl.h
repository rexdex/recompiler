#pragma once

#include "xenonGPUDumpFormat.h"

class CXenonGPUDumpWriterImpl : public IXenonGPUDumpWriter
{
public:
	// Close the file and finish the dump
	virtual void Close() override;

	/// Block (recursive)
	virtual void BeginBlock( const char* tag, uint32& outBlockID ) override;
	virtual void EndBlock( uint32 blockId ) override;

	/// Packet (NON RECURSIVE)
	virtual void Packet( const uint32 packetType, const void* dataWords, const unsigned int lengthInWords ) override;

	/// Register use of external memory for reading (memory will be restored just before this packed is executed)
	virtual void MemoryAccessRead( const uint32 realAddress, const uint32 size, const char* context ) override;

	/// Register use of external memory for writing (memory will be set to that state AFTER command was executed)
	virtual void MemoryAccessWrite( const uint32 realAddress, const uint32 size, const char* context ) override;

public:
	CXenonGPUDumpWriterImpl( const std::wstring& targetPath );
	~CXenonGPUDumpWriterImpl();


private:
	// where to write the file
	std::wstring			m_targetPath;

	// temporary file for memory dumps
	FILE*					m_memoryDumpFile;

	// memory dump map - by data CRC
	typedef std::pair< uint64, uint64 > TMemoryKey; // crc, offset, size
	typedef std::map< TMemoryKey, uint32 > TMemoryMap; // key -> block index

	// stack of blocks
	typedef std::vector< uint32 > TBlockStack;
	TBlockStack				m_blockStack;

	// map of memory mapped blocks
	TMemoryMap				m_memoryMap;

	// data to write
	CXenonGPUDumpFormat		m_tables;

	// add memory block, returns memory block index, NOTE: memory block may be reused
	uint32 MapMemoryBlock( const uint32 address, const uint32 size );
};
