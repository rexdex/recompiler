#include "build.h"
#include "xenonGPUDumpFormat.h"

const uint32 CXenonGPUDumpFormat::FILE_MAGIC = 'GPUD';
const uint32 CXenonGPUDumpFormat::FILE_VERSION = 1;

CXenonGPUDumpFormat::CXenonGPUDumpFormat()
{
}

bool CXenonGPUDumpFormat::Load( const std::wstring& path, uint64& memoryDumpOffset )
{
	// open file
	FILE* f = NULL;
	_wfopen_s( &f, path.c_str(), L"rb" );
	if ( !f )
	{
		GLog.Err( "Failed to open file '%ls'", path.c_str() );
		return false;
	}

	// load header
	Header header;
	memset( &header, 0, sizeof(header) );
	fread( &header, sizeof(header), 1, f );

	// invalid file ?
	if ( header.m_magic != FILE_MAGIC )
	{
		GLog.Err( "Invalid file format of '%ls'", path.c_str() );
		fclose(f);
		return false;
	}

	// invalid version ?
	if ( header.m_version != FILE_VERSION )
	{
		GLog.Err( "Unsupported file version (%d) of '%ls'", header.m_version, path.c_str() );
		fclose(f);
		return false;
	}

	// load blocks
	{
		fsetpos( f, (const fpos_t*) &header.m_blocksOffset );
		m_blocks.resize( header.m_numBlocks );
		if ( header.m_numBlocks  )
			fread( &m_blocks[0], sizeof(Block), header.m_numBlocks, f );

		GLog.Log( "Loaded %d blocks", header.m_numBlocks );
	}

	// load packets
	{
		fsetpos( f, (const fpos_t*) &header.m_packetsOffset );
		m_packets.resize( header.m_numPackets );
		if ( header.m_numPackets  )
			fread( &m_packets[0], sizeof(Packet), header.m_numPackets, f );

		GLog.Log( "Loaded %d packets", header.m_numPackets );
	}

	// load data words
	{
		fsetpos( f, (const fpos_t*) &header.m_dataRegsOffset );
		m_dataRegs.resize( header.m_numDataRegs );
		if ( header.m_numDataRegs )
			fread( &m_dataRegs[0], sizeof(uint32), header.m_numDataRegs, f );

		GLog.Log( "Loaded %d data words", header.m_numDataRegs );
	}

	// load memory refs descriptors
	{
		fsetpos( f, (const fpos_t*) &header.m_memoryRefsOffset );
		m_memoryRefs.resize( header.m_numMemoryRefs );
		if ( header.m_numMemoryRefs )
			fread( &m_memoryRefs[0], sizeof(MemoryRef), header.m_numMemoryRefs, f );

		GLog.Log( "Loaded %d memory refs", header.m_numMemoryRefs );
	}

	// load memory blocks descriptors
	{
		fsetpos( f, (const fpos_t*) &header.m_memoryBlocksOffset );
		m_memoryBlocks.resize( header.m_numMemoryBlocks );
		if ( header.m_numMemoryBlocks )
			fread( &m_memoryBlocks[0], sizeof(Memory), header.m_numMemoryBlocks, f );

		GLog.Log( "Loaded %d memory blocks", header.m_numMemoryBlocks );
	}

	// extract base offset to actual memory dump
	memoryDumpOffset = header.m_memoryDumpOffset;

	// loaded
	fclose( f );
	return true;
}

bool CXenonGPUDumpFormat::Save( const std::wstring& path, const std::wstring& memoryBlobFile ) const
{
	// open file
	FILE* f = NULL;
	_wfopen_s( &f, path.c_str(), L"wb" );
	if ( !f )
	{
		GLog.Err( "Failed to open file '%ls'", path.c_str() );
		return false;
	}

	// write header
	Header header;
	memset( &header, 0, sizeof(header) );
	fwrite( &header, sizeof(header), 1, f );

	// write memory blocks
	{
		fgetpos( f, (fpos_t*) &header.m_memoryBlocksOffset );
		header.m_numMemoryBlocks = (uint32) m_memoryBlocks.size();

		if ( header.m_numMemoryBlocks )
			fwrite( &m_memoryBlocks[0], sizeof(m_memoryBlocks[0]), header.m_numMemoryBlocks, f );

		GLog.Log( "GPU Trace: Saved %d memory blocks", header.m_numMemoryBlocks );
	}

	// write memory references
	{
		fgetpos( f, (fpos_t*) &header.m_memoryRefsOffset );
		header.m_numMemoryRefs = (uint32) m_memoryRefs.size();

		if ( header.m_numMemoryRefs )
			fwrite( &m_memoryRefs[0], sizeof(m_memoryRefs[0]), header.m_numMemoryRefs, f );

		GLog.Log( "GPU Trace: Saved %d memory references", header.m_numMemoryRefs );
	}

	// write data regs
	{
		fgetpos( f, (fpos_t*) &header.m_dataRegsOffset );
		header.m_numDataRegs = (uint32) m_dataRegs.size();

		if ( header.m_numDataRegs )
			fwrite( &m_dataRegs[0], sizeof(m_dataRegs[0]), header.m_numDataRegs, f );

		GLog.Log( "GPU Trace: Saved %d data regs", header.m_numDataRegs );
	}

	// write packets
	{
		fgetpos( f, (fpos_t*) &header.m_packetsOffset );
		header.m_numPackets = (uint32) m_packets.size();

		if ( header.m_numPackets )
			fwrite( &m_packets[0], sizeof(m_packets[0]), header.m_numPackets, f );

		GLog.Log( "GPU Trace: Saved %d packets", header.m_numPackets );
	}

	// write blocks
	{
		fgetpos( f, (fpos_t*) &header.m_blocksOffset );
		header.m_numBlocks = (uint32) m_blocks.size();

		if ( header.m_numBlocks )
			fwrite( &m_blocks[0], sizeof(m_blocks[0]), header.m_numBlocks, f );

		GLog.Log( "GPU Trace: Saved %d blocks", header.m_numBlocks );
	}

	// write (copy) big memory block
	{
		fgetpos( f, (fpos_t*) &header.m_memoryDumpOffset );

		// copy data
		{
			FILE* fc = nullptr;
			_wfopen_s( &fc, memoryBlobFile.c_str(), L"rb" );
			if ( fc )
			{
				while ( !feof( fc  ) )
				{
					uint8 buf[ 65536 ];
					auto size = fread( buf, 1, ARRAYSIZE(buf), fc );
					if ( !size )
						break;

					fwrite( buf, size, 1, f );
				}

				fpos_t size = 0;
				fgetpos( fc, &size );

				GLog.Log( "GPU Trace: Saved %ull memory bytes", size );
				fclose( fc );
			}
		}
	}

	// write header again
	header.m_magic = FILE_MAGIC;
	header.m_version = FILE_VERSION;
	fseek( f, 0, SEEK_SET );
	fwrite( &header, sizeof(header), 1, f );

	// done
	fclose( f );
	return true;
}

