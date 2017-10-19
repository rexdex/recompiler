#include "build.h"
#include "xenonKernel.h"
#include "xenonThread.h"
#include "xenonInplaceExecution.h"

#include "../host_core/runtimeCodeExecutor.h"
#include "../host_core/runtimeTraceWriter.h"
#include "../host_core/runtimeTraceFile.h"

namespace xenon
{

	//-----------------------------------------------------------------------------

	cpu::Reservation GReservation;

	volatile unsigned int ReservationLockValue = 0;

	void ReservationLock()
	{
		while (InterlockedExchange(&ReservationLockValue, 1) != 0) {};
	}

	void ReservationUnlock()
	{
		InterlockedAnd((volatile LONG*)&ReservationLockValue, 0);
	}

	//-----------------------------------------------------------------------------

	__declspec(thread) KernelThread* GCurrentThread = NULL;

	KernelThread::KernelThread(Kernel* kernel, native::IKernel* nativeKernel, const KernelThreadParams& params)
		: IKernelWaitObject(kernel, KernelObjectType::Thread, "Thread")
		, m_code(&m_regs, &kernel->GetCode(), params.m_entryPoint)
		, m_memory( kernel, params.m_stackSize, GetIndex(), params.m_entryPoint, 1 )
		, m_tls( kernel )
		, m_isStopped(false) // not stopped yet
		, m_isCrashed(false) // not crashed yet
		, m_isWaiting(false) // no waiting state
		, m_priority(0) // thread priority normal
		, m_criticalRegion(false)
		, m_requestExit(false)
		, m_affinity(0xFF)
		, m_apcList(nullptr)
		, m_traceWriter(nullptr)
		, m_irql(0)
	{
		// register descriptor in kernel linked lists
		SetNativePointer((void*)m_memory.GetThreadDataAddr()); // NOTE: we are not binding it to the thread

		// setup initial register state
		m_regs.R1 = (uint64)m_memory.GetStack().GetTop();
		m_regs.R3 = params.m_args[0];
		m_regs.R4 = params.m_args[1];
		m_regs.R13 = m_memory.GetPRCAddr(); // always at r13
		m_regs.RES = &GReservation;
		m_regs.INT = &GPlatform.GetInterruptTable();
		m_regs.IO = &GPlatform.GetIOTable();

		// reservation locking
		GReservation.LOCK = &ReservationLock;
		GReservation.UNLOCK = &ReservationUnlock;

		// create the native thread
		m_nativeThread = nativeKernel->CreateThread(this);

		// create trace writer
		if (GPlatform.GetTraceFile())
			m_traceWriter = GPlatform.GetTraceFile()->CreateThreadWriter(GetIndex());
	}

	KernelThread::~KernelThread()
	{
		// request exit, this should stop the thread nicely
		m_requestExit = true;
		Sleep(100);

		// wait for thread to close
		delete m_nativeThread; // this will kill it
		m_nativeThread = nullptr;

		// delete APC list
		delete m_apcList;
		m_apcList = nullptr;

		// if we still have trace file close it now (should be closed on the thread)
		delete m_traceWriter;
		m_traceWriter = nullptr;
	}

	bool KernelThread::Pause()
	{
		return m_nativeThread->Pause();
	}

	bool KernelThread::Resume()
	{
		return m_nativeThread->Resume();
	}

	int32 KernelThread::SetPriority(const int32 priority)
	{
		int32 prevPriority = m_priority;

		if (priority != m_priority)
		{
			m_priority = priority;
			//m_nativeThread->
		}

		return prevPriority;
	}

	uint32 KernelThread::SetAffinity(const uint32 affinity)
	{
		uint32 prevAffinity = m_affinity;

		if (affinity != m_affinity)
		{
			m_affinity = affinity;
			//m_nativeThread->
		}

		return prevAffinity;
	}

	xnative::XStatus KernelThread::Delay(const uint32 processor_mode, const uint32 alertable, const uint64 interval)
	{
		int64 timeout_ticks = interval;
		uint32 timeout_ms = 0;

		if (timeout_ticks < 0)
			timeout_ms = (uint32)(-timeout_ticks / 10000); // Ticks -> MS

		const auto result = m_nativeThread->Sleep(timeout_ms, alertable != 0);

		if (result == native::WaitResult::Success)
			return xnative::X_STATUS_SUCCESS;
		else if (result == native::WaitResult::IOCompletion)
			return xnative::X_STATUS_USER_APC;
		else
			return xnative::X_STATUS_ALERTED;
	}

	xnative::XStatus KernelThread::EnterCriticalRegion()
	{
		if (m_criticalRegion)
		{
			GLog.Err("Kernel: Entering already set critical region on Thread '%s' (TID=%d)", GetName(), GetIndex());
			return xnative::X_STATUS_UNSUCCESSFUL;
		}

		m_criticalRegion = true;
		return xnative::X_STATUS_SUCCESS;
	}

	xnative::XStatus KernelThread::LeaveCriticalRegion()
	{
		if (!m_criticalRegion)
		{
			GLog.Err("Kernel: Leaving non existing critical region on Thread '%s' (TID=%d)", GetName(), GetIndex());
			return xnative::X_STATUS_UNSUCCESSFUL;
		}

		m_criticalRegion = false;
		return xnative::X_STATUS_SUCCESS;
	}

	int KernelThread::Run(native::IThread& thread)
	{
		// bind the current thread
		GCurrentThread = this;

		// stuff
		GLog.Log("Thread: Starting execution at thread %hs, ID=%u", GetName(), GetIndex());

		// start the execution loop
		int exitCode = 0;
		try
		{
			if (m_traceWriter != nullptr)
			{
				m_traceWriter->AddFrame(m_code.GetInstructionPointer(), m_regs);

				while (!m_requestExit && m_code.RunTraced(*m_traceWriter))
				{}					
			}
			else
			{
				while (!m_requestExit && m_code.RunPure())
				{}
			}
		}

		catch (runtime::TerminateProcessException& e)
		{
			GLog.Log("Thread: Thread '%hs' (ID=%u) requested to terminate the process with exit code %u", GetName(), GetIndex(), e.GetExitCode());
		}
		catch (runtime::TerminateThreadException& e)
		{
			GLog.Log("Thread: Thread '%hs' (ID=%u) requested to terminate the thread with exit code %u", GetName(), GetIndex(), e.GetExitCode());
			exitCode = (int)e.GetExitCode();
		}
		catch (runtime::Exception& e)
		{
			GLog.Err("Thread: Thread '%hs' (ID=%u) received unhandled exception %s at 0x%08llX", GetName(), GetIndex(), e.what(), e.GetInstructionPointer());
			m_isCrashed = true;
		}

		// cleanup APCs
		CleanupAPCs();

		// exit the thread
		GLog.Log("Thread: Thread '%hs' (ID=%u) closed", GetName(), GetIndex());
		m_isStopped = true;
		return exitCode;
	}

	void KernelThread::LockAPC()
	{
		m_apcLock.lock();
	}

	void KernelThread::UnlockAPC()
	{
		const bool needsAPC = m_apcList->HasPending();
		m_apcLock.unlock();

		if (needsAPC)
			m_nativeThread->ScheduleAPC();
	}

	void KernelThread::LowerIRQL(const uint32 level)
	{
		m_irql = level;
	}

	uint32 KernelThread::RaiseIRQL(const uint32 level)
	{
		return m_irql.exchange(level);
	}

	// http://www.drdobbs.com/inside-nts-asynchronous-procedure-call/184416590?pgno=1
	// http://www.drdobbs.com/inside-nts-asynchronous-procedure-call/184416590?pgno=7
	void KernelThread::ProcessAPC()
	{
		LockAPC();

		while (m_apcList->HasPending())
		{
			// Get APC entry (offset for LIST_ENTRY offset) and cache what we need.
			// Calling the routine may delete the memory/overwrite it.
			const uint32 apcAddress = (const uint32)m_apcList->Pop() - 8;
			const uint32 kernelRoutine = cpu::mem::loadAddr<uint32>(apcAddress + 16);
			const uint32 normalRoutine = cpu::mem::loadAddr<uint32>(apcAddress + 24);
			const uint32 normalContext = cpu::mem::loadAddr<uint32>(apcAddress + 28);
			const uint32 systemArg1 = cpu::mem::loadAddr<uint32>(apcAddress + 32);
			const uint32 systemArg2 = cpu::mem::loadAddr<uint32>(apcAddress + 36);

			// Mark as uninserted so that it can be reinserted again by the routine.
			const uint32 oldFlags = cpu::mem::loadAddr<uint32>(apcAddress + 40);
			cpu::mem::storeAddr<uint32>(apcAddress + 40, oldFlags & ~0xFF00);

			// Call kernel routine.
			// The routine can modify all of its arguments before passing it on.
			// Since we need to give guest accessible pointers over, we copy things
			// into and out of scratch.
			const uint32 scratchAddr = m_memory.GetScratchAddr();
			cpu::mem::storeAddr<uint32>(scratchAddr + 0, normalRoutine);
			cpu::mem::storeAddr<uint32>(scratchAddr + 4, normalContext);
			cpu::mem::storeAddr<uint32>(scratchAddr + 8, systemArg1);
			cpu::mem::storeAddr<uint32>(scratchAddr + 12, systemArg2);

			// setup execution params
			// kernel_routine(apc_address, &normalRoutine, &normalContext, &systemArg1, &systemArg2)
			InplaceExecutionParams executionParams;
			executionParams.m_entryPoint = kernelRoutine;
			executionParams.m_stackSize = 16 * 1024;
			executionParams.m_threadId = GetIndex();
			executionParams.m_args[0] = apcAddress;
			executionParams.m_args[1] = scratchAddr + 0;
			executionParams.m_args[2] = scratchAddr + 4;
			executionParams.m_args[3] = scratchAddr + 8;
			executionParams.m_args[4] = scratchAddr + 12;

			// execute code
			InplaceExecution executor(GetOwner(), executionParams, "APC");
			if (executor.Execute())
			{
				const uint32 userNormalRoutine = cpu::mem::loadAddr<uint32>(scratchAddr + 0);
				const uint32 userNormalContext = cpu::mem::loadAddr<uint32>(scratchAddr + 4);
				const uint32 userSystemArg1 = cpu::mem::loadAddr<uint32>(scratchAddr + 8);
				const uint32 userSystemArg2 = cpu::mem::loadAddr<uint32>(scratchAddr + 12);

				// Call the normal routine. Note that it may have been killed by the kernel routine.
				if (userNormalRoutine)
				{
					UnlockAPC();

					// normal_routine(normal_context, system_arg1, system_arg2)
					InplaceExecutionParams userExecutionParams;
					userExecutionParams.m_entryPoint = userNormalRoutine;
					userExecutionParams.m_stackSize = 16 * 1024;
					userExecutionParams.m_threadId = GetIndex();
					userExecutionParams.m_args[0] = userNormalContext;
					userExecutionParams.m_args[1] = userSystemArg1;
					userExecutionParams.m_args[2] = userSystemArg2;

					InplaceExecution userExecutor(GetOwner(), userExecutionParams, "APC");
					userExecutor.Execute();

					LockAPC();
				}
			}
		}

		// done
		UnlockAPC();
	}

	void KernelThread::CleanupAPCs()
	{
		if (!m_apcList)
			return;

		LockAPC();

		while (m_apcList->HasPending())
		{
			// Get APC entry (offset for LIST_ENTRY offset) and cache what we need.
			// Calling the routine may delete the memory/overwrite it.
			const uint32 apcAddress = m_apcList->Pop() - 8;
			const uint32 rundownRoutine = cpu::mem::loadAddr<uint32>(apcAddress + 20);

			// Mark as uninserted so that it can be reinserted again by the routine.
			const uint32 oldFlags = cpu::mem::loadAddr<uint32>(apcAddress + 40);
			cpu::mem::storeAddr<uint32>(apcAddress + 40, oldFlags & ~0xFF00);

			// Call the rundown routine.
			if (rundownRoutine)
			{
				// setup
				// rundown_routine(apc)
				InplaceExecutionParams executionParams;
				executionParams.m_entryPoint = rundownRoutine;
				executionParams.m_threadId = GetIndex();
				executionParams.m_stackSize = 16 * 1024;
				executionParams.m_args[0] = apcAddress;

				// execute code
				InplaceExecution executor(GetOwner(), executionParams, "APC");
				executor.Execute();
			}
		}

		UnlockAPC();
	}


	KernelThread* KernelThread::GetCurrentThread()
	{
		return GCurrentThread;
	}

	uint32 KernelThread::GetCurrentThreadID()
	{
		return GCurrentThread ? GCurrentThread->GetIndex() : 0;
	}

} // xenon