#include "build.h"
#include "xenonGPU.h"
#include "xenonGPUThread.h"
#include "xenonGPUExecutor.h"

#include "dx11AbstractLayer.h"

CXenonGPU::CXenonGPU(const launcher::Commandline& cmdLine)
	: m_abstractLayer( nullptr )
	, m_executor( nullptr)
	, m_thread(nullptr)
{
	// create rendering abstraction layer
	m_abstractLayer = new CDX11AbstractLayer();

	// create execution wrapper
	m_executor = new CXenonGPUExecutor(m_abstractLayer, cmdLine);
}

void CXenonGPU::RequestTraceDump()
{
	m_executor->RequestTraceDump();
}

bool CXenonGPU::Initialize( const void* ptr, const uint32 numPages)
{
	// initialize command buffer
	m_commandBuffer.Initialize( ptr, numPages );

	// initialize GPU execution thread
	m_thread = new CXenonGPUThread( m_commandBuffer, *m_executor, m_abstractLayer );

	// done
	GLog.Log( "GPU: Xenon GPU emulation initialized" );
	return true;
}

void CXenonGPU::Close()
{
	// disable interrupt calls
	extern bool GSuppressGPUInterrupts;
	GSuppressGPUInterrupts = true;

	// close thread
	if ( m_thread )
	{
		delete m_thread;
		m_thread = nullptr;
	}

	// close executor
	if ( m_executor )
	{
		delete m_executor;
		m_executor = nullptr;
	}

	// close abstract layer
	if ( m_abstractLayer )
	{
		delete m_abstractLayer;
		m_abstractLayer = nullptr;
	}	
}

void CXenonGPU::WriteWord( const uint64 val, const uint32 addr)
{
	const uint32 regIndex = addr & 0xFFFF; // lo word

	if ( regIndex == 0x0714 ) //CP_RB_WPTR
	{
		const uint32 newWriteAddress = (const uint32) val;
		m_commandBuffer.AdvanceWriteIndex( newWriteAddress ); // this allows GPU to read stuff
	}
	else if ( regIndex == 0x6110 )
	{
		GLog.Log( "GPU: Unimplemented GPU external register 0x%04X, value written=0x%08X", regIndex, val );
	}
	else
	{
		GLog.Err( "GPU: Unknown GPU external register 0x%04X, value written=0x%08X", regIndex, val );
	}

	//register_file_.values[r].u32 = static_cast<uint32_t>(value);
}

void CXenonGPU::ReadWord( uint64* val, const uint32 addr )
{
	const uint32 regIndex = addr & 0xFFFF; // lo word
	const uint64 regData = m_executor->ReadRegister( regIndex );
	*val = regData; // NO BYTESWAP SINCE THE TARGET IS A REGISTRY
}

void CXenonGPU::EnableReadPointerWriteBack( const uint32 ptr, const uint32 blockSize )
{
	m_commandBuffer.SetWriteBackPointer( ptr );
}

void CXenonGPU::SetInterruptCallbackAddr( const uint32 addr, const uint32 userData )
{
	m_executor->SetInterruptCallbackAddr( addr, userData );
}
