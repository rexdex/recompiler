#pragma once

#include "xenonKernel.h"
#include "../host_core/runtimeCodeExecutor.h"

namespace runtime
{
	class TraceWriter;
}

namespace xenon
{
	/// Thread object
	class KernelThread : public IKernelWaitObject, public native::IRunnable
	{
	public:
		inline int32 GetPriority() const { return m_priority; }

		inline int32 GetQueuePriority() const { return m_queuePriority; }

		inline KernelTLS& GetTLS() { return m_tls; }

		inline KernelThreadMemory& GetMemoryBlock() { return m_memory; }

		inline Pointer<KernelDispatchHeader> GetTheadData() const { return Pointer<KernelDispatchHeader>(m_memory.GetThreadDataAddr()); }

	public:
		KernelThread(Kernel* kernel, native::IKernel* nativeKernel, const KernelThreadParams& params);
		virtual ~KernelThread();

		// returns true if the thread has crashed (an uncontinuable exception was thrown)
		inline bool HasCrashed() const { return m_isCrashed; }

		// returns true if the thread has stopped
		inline bool HasStopped() const { return m_isStopped; }

		// pause thread
		bool Pause();

		// resume thread
		bool Resume();

		// lock APC (Async procedure call)
		void LockAPC();

		// enqueue APC entry to the thread queue
		bool EnqueueAPC(Pointer<KernelAPCData> apcEntryPtr, const uint32 arg1, const uint32 arg2);

		// create and enqueue APC entry to the thread queue
		bool EnqueueAPC(MemoryAddress callbackFuncAddress, uint32 callbackContext, const uint32 arg1, const uint32 arg2);

		// remove APC entry from the thread queue
		bool DequeueAPC(Pointer<KernelAPCData> apcEntryPtr);

		// unlock APC
		void UnlockAPC();

		// process pending APC
		void ProcessAPC();

		// lower internal IRQL level
		void LowerIRQL(const uint32 level);

		// raise internal IRQL level
		uint32 RaiseIRQL(const uint32 level);

		// set thread priority
		int32 SetPriority(const int32 priority);

		// set thread core affinity
		uint32 SetAffinity(const uint32 affinity);

		// set thread queue priority
		int32 SetQueuePriority(const int32 priority);

		// kernel interface
		lib::XStatus Delay(const uint32 processor_mode, const uint32 alertable, const uint64 interval);

		// enter region where thread's execution cannot be interupted
		bool EnterCriticalRegion();

		// leave region where thread's execution was not interruptable
		// NOTE: may execute APCs
		bool LeaveCriticalRegion();

		// waitable
		virtual uint32 Wait(const uint32 waitReason, const uint32 processorMode, const bool alertable, const int64* optTimeout) override final;
		virtual native::IKernelObject* GetNativeObject() const override final;

		// get current thread
		static KernelThread* GetCurrentThread();

		// get current threadID
		static uint32 GetCurrentThreadID();

	private:
		virtual int Run(native::IThread& thread) override final;

		void CleanupAPCs();

		cpu::CpuRegs				m_regs;				// cpu registers
		runtime::CodeExecutor		m_code;				// code being executed on this thread

		std::atomic<uint32>			m_irql;				// IRQL

		native::IThread*			m_nativeThread;		// actual thread

		runtime::TraceWriter*		m_traceWriter;		// in case we are tracing this thread this is the output

		int32						m_priority;			// thread priority
		int32						m_queuePriority;	// queue priority
		uint32						m_affinity;			// thread affinity
		bool						m_criticalRegion;	// are we in critical region for this thread ?

		KernelThreadMemory			m_memory;			// VM state - thread memory + stack
		KernelTLS					m_tls;				// VM state - tls data

		volatile bool				m_isStopped;		// Thread state - stopped (Resume/Suspend)
		volatile bool				m_isWaiting;		// Thread state - waiting (critical section, semaphore, etc)
		volatile bool				m_isCrashed;		// Thead state - crashed (uncontinuable exception was thrown)
		volatile bool				m_requestExit;		// External exit was requested

		std::mutex m_apcLock;
		KernelList* m_apcList;
	};

	//---------------------------------------------------------------------------

} // xenon