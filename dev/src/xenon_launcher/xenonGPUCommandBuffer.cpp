#include "build.h"
#include "xenonLibNatives.h"
#include "xenonGPUCommandBuffer.h"
#include "xenonCPU.h"
#include "xenonMemory.h"

//-------------------------------------

//#define DUMP_COMMAND_BUFFER

//-------------------------------------

CXenonGPUCommandBufferReader::CXenonGPUCommandBufferReader()
	: m_bufferBase(nullptr)
	, m_bufferSize(0)
	, m_readStartIndex(0)
	, m_readEndIndex(0)
	, m_readIndex(0)
	, m_readCount(0)
	, m_readMaxCount(0)
{
}

CXenonGPUCommandBufferReader::CXenonGPUCommandBufferReader(const uint32* bufferBase, const uint32 bufferSize, const uint32 readStartIndex, const uint32 readEndIndex)
	: m_bufferBase(bufferBase)
	, m_bufferSize(bufferSize)
	, m_readStartIndex(readStartIndex)
	, m_readEndIndex(readEndIndex)
	, m_readIndex(readStartIndex)
	, m_readCount(0)
{
	auto end = m_readEndIndex;
	if (end < m_readStartIndex)
		end += m_bufferSize;

	m_readMaxCount = end - m_readStartIndex;

	DEBUG_CHECK(m_readMaxCount > 0);
	DEBUG_CHECK(bufferSize > 0);
}

const uint32 CXenonGPUCommandBufferReader::Read()
{
	DEBUG_CHECK(m_readIndex != m_readEndIndex);

	std::atomic_thread_fence(std::memory_order_acq_rel);
	auto ret = cpu::mem::load< uint32 >(m_bufferBase + m_readIndex);
	Advance(1);

	return ret;
}

void CXenonGPUCommandBufferReader::GetBatch(const uint32 numWords, uint32* writePtr)
{
	DEBUG_CHECK(m_readCount + numWords <= m_readMaxCount);

	std::atomic_thread_fence(std::memory_order_acq_rel);

	auto pos = m_readIndex;
	for (uint32 index = 0; index<numWords; ++index)
	{
		// copy data
		writePtr[index] = m_bufferBase[pos]; // NOTE: no endianess swap

		// advance
		pos += 1;
		if (pos >= m_bufferSize)
			pos = pos - m_bufferSize;
	}
}

void CXenonGPUCommandBufferReader::Advance(const uint32 numWords)
{
	DEBUG_CHECK(m_readCount + numWords <= m_readMaxCount);

	// advance
	m_readIndex += numWords;
	m_readCount += numWords;

	// wrap around
	if (m_bufferSize && (m_readIndex >= m_bufferSize))
		m_readIndex = m_readIndex - m_bufferSize;
}

//-------------------------------------

CXenonGPUCommandBuffer::CXenonGPUCommandBuffer()
	: m_commandBufferPtr(nullptr)
	, m_writeBackPtr(0)
{
}

CXenonGPUCommandBuffer::~CXenonGPUCommandBuffer()
{
}

void CXenonGPUCommandBuffer::Initialize(const void* ptr, const uint32 numPages)
{
	// setup basics
	m_commandBufferPtr = (const uint32*)ptr;
	m_numWords = 1 << (1 + numPages); // size of the command buffer

	// reset writing/reading ptr
	m_writeIndex = 0;
	m_readIndex = 0;

	GLog.Log("GPU: Command buffer initialized, ptr=0x%08X, numWords=%d", ptr, m_numWords);

}

void CXenonGPUCommandBuffer::AdvanceWriteIndex(const uint32 newIndex)
{
	std::atomic_thread_fence(std::memory_order_acq_rel);
	uint32 oldIndex = m_writeIndex.exchange(newIndex);

#ifdef DUMP_COMMAND_BUFFER
	GLog.Log( "GPU: Command buffer writeIndex=%d (delta: %d)", newIndex, newIndex - oldIndex );
	for ( uint32 i=oldIndex; i<newIndex; ++i )
	{
		const auto memAddr = m_commandBufferPtr + i;
		GLog.Log( "GPU: Mem[+%d, @%d]: 0x%08X", i-oldIndex, i, cpu::mem::load<uint32>( memAddr) );
	}
#endif
}

void CXenonGPUCommandBuffer::SetWriteBackPointer(const uint32 addr)
{
	m_writeBackPtr = addr;
	GLog.Log("GPU: Writeback pointer set to 0x%08X", addr);
}

bool CXenonGPUCommandBuffer::BeginRead(CXenonGPUCommandBufferReader& outReader)
{
	// capture read offset
	auto curWriteIndex = m_writeIndex.load();
	if (m_readIndex == curWriteIndex)
	{
#ifdef DUMP_COMMAND_BUFFER
		GLog.Log("GPU: No new data @%d", curWriteIndex);
#endif
		return false;
	}

	// setup actual write index
	const uint32 readStartIndex = m_readIndex; // prev position
	const uint32 readEndIndex = curWriteIndex; // current position
#ifdef DUMP_COMMAND_BUFFER
	GLog.Log("GPU: Read started at %d, end %d", readStartIndex, readEndIndex);
	for (uint32 i = readStartIndex; i < readEndIndex; ++i)
	{
		auto data = cpu::mem::load<uint32>(m_commandBufferPtr + i);
		GLog.Log("GPU: RMem[+%d, @%d]: 0x%08X", i - readStartIndex, i, data);
	}
#endif
	// copy data out
	memcpy(&outReader, &CXenonGPUCommandBufferReader(m_commandBufferPtr, m_numWords, readStartIndex, readEndIndex), sizeof(CXenonGPUCommandBufferReader));
	m_readIndex = curWriteIndex;

	// advance
	return true;
}

void CXenonGPUCommandBuffer::EndRead()
{
	if (m_writeBackPtr)
	{
		const uint32 cpuAddr = GPlatform.GetMemory().TranslatePhysicalAddress(m_writeBackPtr);
		cpu::mem::storeAddr< uint32 >(cpuAddr, m_readIndex);
#ifdef DUMP_COMMAND_BUFFER
		GLog.Log("GPU STORE at %08Xh, val %d (%08Xh)", cpuAddr, m_readIndex, m_readIndex);
#endif
	}
}

//-------------------------------------
