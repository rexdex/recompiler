#pragma once

class CXenonGPUExecutor;
class CXenonGPUCommandBuffer;
class IXenonGPUAbstractLayer;

/// GPU execution thread
/// Normally, this is the GPU itself
/// The GPU is sucking stuff through the command buffer and communicating though interrupts
class CXenonGPUThread
{
public:
	CXenonGPUThread( CXenonGPUCommandBuffer& cmdBuffer, CXenonGPUExecutor& executor, IXenonGPUAbstractLayer* abstractionLayer );

	/// Stop thread, waits for thread to finish
	/// NOTE: without GPU thread application will eventually stall
	~CXenonGPUThread();

private:
	HANDLE	m_hThread;		// managing thread
	HANDLE	m_hSync;		// helper sync object

	HANDLE  m_hTimerQueue;
	HANDLE  m_hVSyncTimer;

	runtime::TraceWriter* m_traceWriter;

	volatile bool	m_killRequest;

	CXenonGPUCommandBuffer*		m_commandBuffer;	// related command buffer
	CXenonGPUExecutor*			m_executor;			// command data executor
	IXenonGPUAbstractLayer*		m_abstractionLayer;	// rendering abstraction layer

	static DWORD WINAPI ThreadProc( LPVOID lpParameter );
	static void WINAPI VsyncCallbackThunk( LPVOID lpParameter, BOOLEAN );

	void ThreadFunc();
};

