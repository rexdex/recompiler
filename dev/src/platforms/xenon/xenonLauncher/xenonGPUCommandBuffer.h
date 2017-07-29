#pragma once

#include <atomic>

/// Reader helper for command buffer
class CXenonGPUCommandBufferReader
{
public:
	CXenonGPUCommandBufferReader();
	CXenonGPUCommandBufferReader( const uint32* bufferBase, const uint32 bufferSize, const uint32 readStartIndex, const uint32 readEndIndex );

	/// Get we read data ?
	inline const bool CanRead() const { return m_readCount < m_readMaxCount; }

	/// Extract raw pointer, will assert if there's not enough space for read
	void GetBatch( const uint32 numWords, uint32* writePtr );

	/// Move command buffer
	void Advance( const uint32 numWords );

	/// Read single value from command buffer (Peek + Advance(1))
	const uint32 Read();

private:
	const uint32* m_bufferBase; // const, absolute
	const uint32 m_bufferSize; // const, in words

	const uint32 m_readStartIndex;
	const uint32 m_readEndIndex;

	uint32 m_readIndex;
	uint32 m_readCount;
	uint32 m_readMaxCount;
};

/// Command buffer management for the GPU
/// NOTE: this is mapped to virtual address at 0x
class CXenonGPUCommandBuffer
{
public:
	CXenonGPUCommandBuffer();
	~CXenonGPUCommandBuffer();

	/// Initialize the ring buffer interface
	void Initialize( const void* ptr, const uint32 numPages );

	/// Set the put pointer
	void AdvanceWriteIndex( const uint32 newIndex );

	/// Set write back pointer (read register content will be written there)
	void SetWriteBackPointer( const uint32 addr );

	/// Read word from command buffer, returns false if there's no data
	bool BeginRead( CXenonGPUCommandBufferReader& outReader );

	/// Signal completed read
	void EndRead();

private:
	// command buffer initialization data
	const uint32* m_commandBufferPtr;
	uint32 m_numWords; // in the whole command buffer

	// write index
	std::atomic< uint32 > m_writeIndex;

	// read position, used by one thread only
	uint32 m_readIndex;

	// event used to synchronize reading from command buffer
	uint32 m_writeBackPtr;	
};