#include "build.h"
#include "xenonLibNatives.h"
#include "xenonGPUCommandBuffer.h"
#include "xenonCPU.h"

//-------------------------------------

CXenonGPUCommandBufferReader::CXenonGPUCommandBufferReader()
	: m_bufferBase( nullptr )
	, m_bufferSize( 0 )
	, m_readStartIndex( 0 )
	, m_readEndIndex( 0 )
	, m_readIndex( 0 )
{
}

CXenonGPUCommandBufferReader::CXenonGPUCommandBufferReader( const uint32* bufferBase, const uint32 bufferSize, const uint32 readStartIndex, const uint32 readEndIndex )
	: m_bufferBase( bufferBase )
	, m_bufferSize( bufferSize )
	, m_readStartIndex( readStartIndex )
	, m_readEndIndex( readEndIndex )
	, m_readIndex( readStartIndex )
	, m_readOffset( 0 )
{
}

const uint32 CXenonGPUCommandBufferReader::Peek() const
{
	DEBUG_CHECK( m_readIndex < m_readEndIndex );
	return mem::load< uint32 >( m_bufferBase + m_readIndex );
}

const void* CXenonGPUCommandBufferReader::GetRawPointer( const uint32 numWords ) const
{
	DEBUG_CHECK( m_readEndIndex > m_readStartIndex ); // buffer needs to be one single block
	DEBUG_CHECK( m_readIndex + numWords <= m_readEndIndex ); // we must have enough memory
	return m_bufferBase + m_readIndex;
}

void CXenonGPUCommandBufferReader::Advance( const uint32 numWords )
{
	// advance
	m_readIndex += numWords;
	m_readOffset += numWords;

	// wrap around
	if ( m_bufferSize && (m_readIndex >= m_bufferSize) )
		m_readIndex = m_readIndex - m_bufferSize;
}

//-------------------------------------

CXenonGPUCommandBuffer::CXenonGPUCommandBuffer()
	: m_commandBufferPtr( nullptr )
	, m_writeBackPtr( 0 )
	, m_numPages( 0 )
{
}

CXenonGPUCommandBuffer::~CXenonGPUCommandBuffer()
{
}

void CXenonGPUCommandBuffer::Initialize( const void* ptr, const uint32 numPages )
{
	// setup basics
	m_commandBufferPtr = (const uint32*) ptr;
	m_numPages = numPages;

	// estimate number of words
	m_numWords = 1 << (0x1C - m_numPages - 1);

	// reset writing/reading ptr
	m_writeIndex = 0;
	m_readIndex = 0;

	GLog.Log( "GPU: Command buffer initialized, ptr=0x%08X, numPages=%d, numWords=%d", 
		ptr, numPages, m_numWords );
}

void CXenonGPUCommandBuffer::AdvanceWriteIndex( const uint32 newIndex )
{
	uint32 oldIndex = m_writeIndex;
	m_writeIndex = newIndex;

	//GLog.Log( "GPU: Command buffer writeIndex=%d (delta: %d)", newIndex, newIndex - oldIndex );

	for ( uint32 i=oldIndex; i<newIndex; ++i )
	{
		const uint32 memAddr = (const uint32) m_commandBufferPtr + i*4;
		//GLog.Log( "GPU: Mem[%d]: 0x%08X", (i-oldIndex)/4, mem::loadAddr<uint32>( memAddr) );
	}
}

void CXenonGPUCommandBuffer::SetWriteBackPointer( const uint32 addr )
{
	m_writeBackPtr = addr;
	GLog.Log( "GPU: Writeback pointer set to 0x%08X", addr );
}

bool CXenonGPUCommandBuffer::BeginRead( CXenonGPUCommandBufferReader& outReader )
{
	// no new data
	if ( m_readIndex == m_writeIndex )
		return false;

	// capture read offset
	m_writeIndexAtReadStart = m_writeIndex;
	//GLog.Log( "GPU: Read started: %d", m_writeIndexAtReadStart );

	// setup actual write index
	const uint32 readStartIndex = m_readIndex % m_numWords; // prev position
	const uint32 readEndIndex = m_writeIndexAtReadStart % m_numWords; // current position
	memcpy( &outReader, &CXenonGPUCommandBufferReader( m_commandBufferPtr, m_numWords, readStartIndex, readEndIndex ), sizeof(CXenonGPUCommandBufferReader) );

	// advance
	return true;
}

void CXenonGPUCommandBuffer::EndRead()
{
	if ( m_writeBackPtr )
	{
		m_readIndex = m_writeIndexAtReadStart;

		const uint32 cpuAddr = GPlatform.GetMemory().TranslatePhysicalAddress( m_writeBackPtr );
		mem::storeAddr< uint32 >( cpuAddr, m_readIndex );
		GLog.Log("GPU STORE at %08Xh, val %d (%08Xh)", cpuAddr, m_readIndex, m_readIndex);
	}
}

//-------------------------------------
