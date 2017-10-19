#pragma once

class IXenonGPUAbstractLayer;
class CXenonGPUThread;
class CXenonGPUExecutor;

#include "xenonGPUCommandBuffer.h"

namespace launcher
{
	class Commandline;
}

/// GPU emulation
class CXenonGPU
{
public:
	CXenonGPU(const launcher::Commandline& cmdLine);

	/// Request a trace dump of next frame
	void RequestTraceDump();

	/// Initialize, usually creates the window
	bool Initialize( const void* ptr, const uint32 numPages);

	/// Close the GPU
	void Close();

	// Write stuff to GPU external register
	void WriteWord( const uint64 val, const uint32 addr);

	// Read stuff from GPU external register
	void ReadWord( uint64* val, const uint32 addr );

	// command buffer stuff
	void EnableReadPointerWriteBack( const uint32 ptr, const uint32 blockSize );

	// set internal interrupt callback
	void SetInterruptCallbackAddr( const uint32 addr, const uint32 userData );

private:
	// low level command buffer
	CXenonGPUCommandBuffer		m_commandBuffer;

	// executor of commands
	CXenonGPUExecutor*			m_executor;

	// execution thread
	CXenonGPUThread*			m_thread;

	// abstract layer
	IXenonGPUAbstractLayer*		m_abstractLayer;
};