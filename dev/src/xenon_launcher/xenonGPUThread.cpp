#include "build.h"
#include "xenonLibNatives.h"
#include "xenonGPUThread.h"
#include "xenonGPUAbstractLayer.h"
#include "xenonGPUExecutor.h"
#include "xenonGPUCommandBuffer.h"
#include "xenonPlatform.h"
#include "../host_core/runtimeTraceFile.h"

CXenonGPUThread::CXenonGPUThread( CXenonGPUCommandBuffer& cmdBuffer, CXenonGPUExecutor& executor, IXenonGPUAbstractLayer* abstractionLayer )
	: m_commandBuffer( &cmdBuffer )
	, m_executor( &executor )
	, m_abstractionLayer( abstractionLayer )
	, m_hThread( nullptr )
	, m_hTimerQueue( nullptr )
	, m_hVSyncTimer( nullptr )
	, m_killRequest( false )
	, m_traceWriter(nullptr)
{
	// create the trace writer
	if (GPlatform.GetTraceFile())
		m_traceWriter = GPlatform.GetTraceFile()->CreateInterruptWriter("GPU_THREAD");

	// create thread
	DWORD dwThreadID = 0;
	m_hSync = ::CreateEventA( NULL, FALSE, FALSE, NULL );
	m_hThread = ::CreateThread( NULL, 16 << 10, &ThreadProc, this, 0, &dwThreadID );

	// create VSYNC stuff
	m_hTimerQueue = CreateTimerQueue();
	const DWORD vsyncTime = 1000 / 60;
	CreateTimerQueueTimer( &m_hVSyncTimer, m_hTimerQueue, (WAITORTIMERCALLBACK)&VsyncCallbackThunk, this, 1, vsyncTime, WT_EXECUTEINPERSISTENTTHREAD );

	// wait for window being created
	WaitForSingleObject( m_hSync, INFINITE );
	GLog.Log( "GPU: Emulation thread ready" );

	// name GPU Thread
	utils::SetThreadName( dwThreadID, "GPU Thread" );
	SetThreadPriority( m_hThread, THREAD_PRIORITY_ABOVE_NORMAL );
}

CXenonGPUThread::~CXenonGPUThread()
{
	// signal kill request
	m_killRequest = true;

	// stop timer
	if ( m_hTimerQueue != NULL )
	{
		GLog.Log( "GPU: Shutting down fake VSYNC timer" );

		ResetEvent( m_hSync );
		DeleteTimerQueueTimer( m_hTimerQueue, m_hVSyncTimer, m_hSync );
		if ( WAIT_TIMEOUT == WaitForSingleObject( m_hSync, 2000 ) )
		{
			GLog.Err( "GPU: VSYNC timer failed to close after 2s, killing it" ); 
		}
		m_hVSyncTimer = NULL;

		DeleteTimerQueue( m_hTimerQueue );
		m_hTimerQueue = NULL;
	}

	// wait for the window thread to finish
	if ( m_hThread )
	{
		GLog.Log( "GPU: Waiting for emulation thread to finish..." );
		if ( WAIT_TIMEOUT == WaitForSingleObject( m_hThread, 2000 ) )
		{
			GLog.Err( "GPU: Emulation thread failed to close after 2s, killing it" ); 
			TerminateThread( m_hThread, 0 );
		}

		// close thread handle
		CloseHandle( m_hThread );
		m_hThread = NULL;
	}

	// close sync event handle
	CloseHandle( m_hSync );
	m_hSync = NULL;

	// final message
	GLog.Log( "GPU: Emulation thread closed" );
}

DWORD WINAPI CXenonGPUThread::ThreadProc( LPVOID lpParameter )
{
	CXenonGPUThread* obj = (CXenonGPUThread*) lpParameter;

	// thread started
	GLog.Log( "GPU: Emulation thread started" );
	SetEvent( obj->m_hSync );

	// initialize abstraction layer, do this ON THIS THREAD because that's the thread that will be using it
	if ( !obj->m_abstractionLayer->Initialize() )
	{
		GLog.Log( "GPU: Failed to initialize abstraction layer. Closing GPU thread." );
		return 1;
	}

	// process stuff
	obj->ThreadFunc();

	// exit
	GLog.Log( "GPU: Emulation thread closing" );
	return 0;
}

void CXenonGPUThread::ThreadFunc()
{
	xenon::BindMemoryTraceWriter(m_traceWriter);

	while ( !m_killRequest )
	{
		// TODO: the output "frame" should be cached here, if nothing gets rendered than we deal with this shit ourselves

		// process stuff from command buffer
		CXenonGPUCommandBufferReader reader;
		if ( m_commandBuffer->BeginRead( reader ) )
		{
			m_executor->Execute( reader );
			m_commandBuffer->EndRead();
		}		
	}
}

void CXenonGPUThread::VsyncCallbackThunk( LPVOID lpParameter, BOOLEAN )
{
	static bool bThreadNamed = false;
	if ( !bThreadNamed )
	{
		utils::SetThreadName( GetCurrentThreadId(), "VSync Thread" );
		SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_HIGHEST );
		bThreadNamed = true;
	}

	CXenonGPUThread* thread = (CXenonGPUThread*) lpParameter;
	thread->m_executor->SignalVBlank();
}
