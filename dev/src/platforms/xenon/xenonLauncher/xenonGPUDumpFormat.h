#pragma once

/// Format of GPU dump file
class CXenonGPUDumpFormat
{
public:
#pragma pack( push, 4 )
	struct Block
	{
		char		m_tag[16];				// block tag (primary, indirect, etc)

		uint32		m_firstSubBlock;		// first sub block
		uint32		m_numSubBlocks;			// number of sub blocks

		uint32		m_firstPacket;			// first packed executed in this block
		uint32		m_numPackets;			// number of packets executed in this block
	};

	struct Packet
	{
		uint32		m_packetData;			// packet data itself
		uint32		m_firstDataWord;		// first extra data word
		uint32		m_numDataWords;			// number of extra data words

		uint32		m_firstMemoryRef;		// first reference to memory block used by this command
		uint32		m_numMemoryRefs;		// number of memory references
	};

	struct MemoryRef
	{
		uint32		m_blockIndex;			// index to actual memory block
		uint32		m_mode;					// 0-read, 1-write
		char		m_tag[16];				// memory tag (texture, shader, etc)
	};

	struct Memory
	{
		uint64		m_crc;					// crc of the memory block
		uint64		m_fileOffset;			// offset in file where the memory is
		uint32		m_address;				// address where to put the data
		uint32		m_size;					// size of the data
	};

	struct Header
	{
		uint32		m_magic;				// file magic number
		uint32		m_version;				// file version number
					
		uint32		m_numBlocks;			// number of blocks in the file
		uint64		m_blocksOffset;			// offset to packet blocks

		uint32		m_numPackets;			// number of packets in the file
		uint64		m_packetsOffset;		// offset to packet data

		uint32		m_numMemoryRefs;		// number of memory refs in the file
		uint64		m_memoryRefsOffset;		// offset to memory refs

		uint32		m_numMemoryBlocks;		// number of memory blocks in the file
		uint64		m_memoryBlocksOffset;	// offset to memory blocks

		uint32		m_numDataRegs;			// number of additional data registers in the file
		uint64		m_dataRegsOffset;		// offset to data regs

		uint64		m_memoryDumpOffset;		// offset to memory dump block
	};
#pragma pack( pop, 4 )

	std::vector< Block >		m_blocks;		// blocks (top level hierarchy of the frame)
	std::vector< Packet >		m_packets;		// packets to execute (NON recursive)
	std::vector< MemoryRef >	m_memoryRefs;	// memory references
	std::vector< Memory >		m_memoryBlocks;	// memory block dumps
	std::vector< uint32 >		m_dataRegs;		// packets data

public:
	CXenonGPUDumpFormat();

	/// load from file, offset to memory dump data is extracted to
	bool Load( const std::wstring& path, uint64& memoryDumpOffset );

	// save to file, raw memory dump is copied from the file specified
	bool Save( const std::wstring& path, const std::wstring& memoryBlobFile ) const;

private:
	static const uint32 FILE_MAGIC;
	static const uint32 FILE_VERSION;
};