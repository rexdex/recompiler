#pragma once

#include <atomic>

/// Reader helper for command buffer
class CXenonGPUCommandBufferReader
{
public:
	CXenonGPUCommandBufferReader();
	CXenonGPUCommandBufferReader( const uint32* bufferBase, const uint32 bufferSize, const uint32 readStartIndex, const uint32 readEndIndex );

	/// Get we read data ?
	inline const bool CanRead() const { return m_readIndex < m_readEndIndex; }

	/// Get current read offset (0-N)
	inline const uint32 GetReadOffset() const { return m_readOffset; }

	/// Get raw pointer, will assert if there's not enough space for read
	const void* GetRawPointer( const uint32 numWords ) const;

	/// Peek value at the command buffer
	const uint32 Peek() const;

	/// Move command buffer
	void Advance( const uint32 numWords );

	/// Read single value from command buffer (Peek + Advance(1))
	inline const uint32 Read()
	{
		const uint32 ret = Peek();
		Advance(1);
		return ret;
	}

private:
	const uint32*	m_bufferBase; // const, absolute
	const uint32	m_bufferSize; // const, in words

	const uint32	m_readStartIndex;
	const uint32	m_readEndIndex;

	uint32			m_readIndex;
	uint32			m_readOffset; // always incremental
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
	const uint32*		m_commandBufferPtr;
	uint32				m_numPages;
	uint32				m_numWords; // in the whole command buffer

	// write/read index
	std::atomic< uint32 >		m_writeIndex;
	std::atomic< uint32 >		m_readIndex;

	// batch
	uint32						m_writeIndexAtReadStart;

	// event used to synchronize reading from command buffer
	uint32						m_writeBackPtr;	
};