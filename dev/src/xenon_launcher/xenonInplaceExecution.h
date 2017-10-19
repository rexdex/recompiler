#pragma once

#include "xenonKernel.h"
#include "../host_core/runtimeCodeExecutor.h"

namespace xenon
{

	/// Inplace (without a thread) code execution params
	class InplaceExecutionParams
	{
	public:
		uint32			m_stackSize;		// thread stack size
		uint32			m_threadId;			// thead id of the parent thread
		uint32			m_cpu;				// cpu index
		uint64			m_args[6];			// startup arguments
		uint32			m_entryPoint;		// thread entry point

		inline InplaceExecutionParams()
			: m_stackSize(64 << 10)
			, m_threadId(0)
			, m_entryPoint(0)
			, m_cpu(0)
		{
			m_args[0] = 0;
			m_args[1] = 0;
			m_args[2] = 0;
			m_args[3] = 0;
			m_args[4] = 0;
			m_args[5] = 0;
		}
	};

	/// Handler for inplace (without a thread) code execution
	/// NOTE: no memory is allocated
	class InplaceExecution
	{
	public:
		InplaceExecution(Kernel* kernel, const InplaceExecutionParams& params, const char* name);
		~InplaceExecution();

		// process until exited or exception is thrown or the code finished, returns exit code
		int Execute();

	private:
		cpu::CpuRegs				m_regs;
		runtime::CodeExecutor		m_code;
		const char*					m_name;

		KernelThreadMemory			m_memory;
	};

} // xenon