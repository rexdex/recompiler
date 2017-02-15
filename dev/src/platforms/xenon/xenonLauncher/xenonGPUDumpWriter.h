#pragma once

/// Dump writer for xenon GPU trace
class IXenonGPUDumpWriter
{
public:
	/// Create a dump file, automatic file naming
	static IXenonGPUDumpWriter* Create();

	// Close the file and finish the dump
	virtual void Close() = 0;

	/// Block (recursive, info only)
	virtual void BeginBlock( const char* tag, uint32& outBlockID ) = 0;
	virtual void EndBlock( uint32 outBlockID ) = 0;

	/// Packet (non recursive, actual stuff)
	virtual void Packet( const uint32 packetType, const void* dataWords, const unsigned int lengthInWords ) = 0;

	/// Register use of external memory for reading (memory will be restored just before this packed is executed)
	virtual void MemoryAccessRead( const uint32 realAddress, const uint32 size, const char* context ) = 0;

	/// Register use of external memory for writing (memory will be set to that state AFTER command was executed)
	virtual void MemoryAccessWrite( const uint32 realAddress, const uint32 size, const char* context ) = 0;

protected:
	virtual ~IXenonGPUDumpWriter() {};
};