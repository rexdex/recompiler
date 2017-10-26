#include "build.h"
#include "xenonKernel.h"
#include "xenonThread.h"
#include "xenonInplaceExecution.h"

#include "../host_core/runtimeCodeExecutor.h"
#include "../host_core/runtimeTraceWriter.h"
#include "../host_core/runtimeTraceFile.h"
#include "xenonMemory.h"

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
		: IKernelWaitObject(kernel, KernelObjectType::Thread)
		, m_code(&m_regs, &kernel->GetCode(), params.m_entryPoint)
		, m_memory( kernel, params.m_stackSize, GetIndex(), params.m_entryPoint, 1 )
		, m_tls( kernel )
		, m_isStopped(false) // not stopped yet
		, m_isCrashed(false) // not crashed yet
		, m_isWaiting(false) // no waiting state
		, m_priority(0) // thread priority normal
		, m_queuePriority(0)
		, m_criticalRegion(false)
		, m_requestExit(false)
		, m_affinity(0xFF)
		, m_apcList(new KernelList())
		, m_traceWriter(nullptr)
		, m_irql(0)
	{
		// create trace writer
		if (GPlatform.GetTraceFile())
		{
			m_traceWriter = GPlatform.GetTraceFile()->CreateThreadWriter(GetIndex());
		}

		// register descriptor in kernel linked lists
		// our dispatch header is placed well within the thread memory block just allocated
		Pointer<KernelDispatchHeader> dispatchHeader(m_memory.GetThreadDataAddr());
		kernel->BindDispatchObjectToKernelObject(this, dispatchHeader);

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
		BindMemoryTraceWriter(nullptr);
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

	int32 KernelThread::SetQueuePriority(const int32 priority)
	{
		int32 prevPriority = m_queuePriority;
		m_queuePriority = priority;
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

	lib::XStatus KernelThread::Delay(const uint32 processor_mode, const uint32 alertable, const uint64 interval)
	{
		int64 timeout_ticks = interval;
		uint32 timeout_ms = 0;

		if (timeout_ticks < 0)
			timeout_ms = (uint32)(-timeout_ticks / 10000); // Ticks -> MS

		const auto result = m_nativeThread->Sleep(timeout_ms, alertable != 0);

		if (result == native::WaitResult::Success)
			return lib::X_STATUS_SUCCESS;
		else if (result == native::WaitResult::IOCompletion)
			return lib::X_STATUS_USER_APC;
		else
			return lib::X_STATUS_ALERTED;
	}

	bool KernelThread::EnterCriticalRegion()
	{
		if (m_criticalRegion)
		{
			GLog.Err("Kernel: Entering already set critical region on Thread '%s' (TID=%d)", GetName(), GetIndex());
			return false;
		}

		m_criticalRegion = true;
		return true;
	}

	bool KernelThread::LeaveCriticalRegion()
	{
		if (!m_criticalRegion)
		{
			GLog.Err("Kernel: Leaving non existing critical region on Thread '%s' (TID=%d)", GetName(), GetIndex());
			return false;
		}
		m_criticalRegion = false;

		// run the APC scheduled while we were busy
		ProcessAPC();

		return true;
	}

	int KernelThread::Run(native::IThread& thread)
	{
		// tag memory write of thread data
		BindMemoryTraceWriter(m_traceWriter);
		TagMemoryWrite(m_memory.GetBlockData(), m_memory.GetBlockSize(), "KERNEL_THEAD_SETUP");

		// bind the current thread
		GCurrentThread = this;

		// stuff
		GLog.Log("Thread: Starting execution at thread %hs, ID=%u", GetName(), GetIndex());

		// process APC that were scheduled befere we even started
		ProcessAPC();

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

	bool KernelThread::EnqueueAPC(MemoryAddress callbackFuncAddress, uint32 callbackContext, const uint32 arg1, const uint32 arg2)
	{
		// create the temporary memory structure
		auto* mem = GPlatform.GetMemory().AllocateSmallBlock(sizeof(MemoryAddress));
		Pointer<KernelAPCData> apcEntryPtr(new (mem) MemoryAddress());

		// setup 
		apcEntryPtr->m_normalRoutine = callbackFuncAddress;
		apcEntryPtr->m_normalContext = callbackContext;
		apcEntryPtr->m_threadPtr = GetTheadData();
		
		// add to list
		return EnqueueAPC(apcEntryPtr, arg1, arg2);
	}

	bool KernelThread::EnqueueAPC(Pointer<KernelAPCData> apcEntryPtr, const uint32 arg1, const uint32 arg2)
	{
		LockAPC();

		// we can insert only once
		if (apcEntryPtr->m_inqueue)
		{
			UnlockAPC();
			return false;
		}

		// set arguments
		apcEntryPtr->m_arg1 = arg1;
		apcEntryPtr->m_arg2 = arg2;
		apcEntryPtr->m_inqueue = 1;

		// push entry to list
		m_apcList->Insert(&apcEntryPtr->m_listEntry);

		// release APC lock, this may schedule APC for execution
		UnlockAPC();
		return true;
	}

	bool KernelThread::DequeueAPC(Pointer<KernelAPCData> apcEntryPtr)
	{
		LockAPC();

		// not in the list :)
		if (!apcEntryPtr->m_inqueue)
		{
			UnlockAPC();
			return false;
		}

		// check if in the list
		if (!m_apcList->IsQueued(&apcEntryPtr->m_listEntry))
		{
			UnlockAPC();
			return false;
		}

		// remove from the list
		m_apcList->Remove(&apcEntryPtr->m_listEntry);

		// release APC lock, this may schedule APC for execution
		UnlockAPC();
		return true;
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

	struct KernelThreadScratch
	{
		Field<MemoryAddress> m_functionPtr;
		Field<uint32_t> m_functionContext;
		Field<uint32_t> m_arg1;
		Field<uint32_t> m_arg2;
	};

	// http://www.drdobbs.com/inside-nts-asynchronous-procedure-call/184416590?pgno=1
	// http://www.drdobbs.com/inside-nts-asynchronous-procedure-call/184416590?pgno=7
	void KernelThread::ProcessAPC()
	{
		LockAPC();

		while (m_apcList->HasPending())
		{
			// pop first pending APC from the list
			auto apcAddress = Pointer<KernelAPCData>(m_apcList->Pop().GetAddress().GetAddressValue() - 8);

			// mark it as no longer int the list
			DEBUG_CHECK(apcAddress->m_inqueue == 1);
			apcAddress->m_inqueue = 0;

			// copy data for calling
			auto scratchData = Pointer<KernelThreadScratch>(m_memory.GetScratchAddr());
			scratchData->m_functionPtr = apcAddress->m_normalRoutine;
			scratchData->m_functionContext = apcAddress->m_normalContext;
			scratchData->m_arg1 = apcAddress->m_arg1;
			scratchData->m_arg2 = apcAddress->m_arg2;

			// setup execution params
			// kernel_routine(apc_address, &normalRoutine, &normalContext, &systemArg1, &systemArg2)
			InplaceExecutionParams executionParams;
			executionParams.m_entryPoint = apcAddress->m_kernelRoutine.GetAddress();
			executionParams.m_stackSize = 16 * 1024;
			executionParams.m_threadId = GetIndex();
			executionParams.m_args[0] = apcAddress.GetAddress().GetAddressValue();
			executionParams.m_args[1] = scratchData.GetAddress().GetAddressValue() + 0;
			executionParams.m_args[2] = scratchData.GetAddress().GetAddressValue() + 4;
			executionParams.m_args[3] = scratchData.GetAddress().GetAddressValue() + 8;
			executionParams.m_args[4] = scratchData.GetAddress().GetAddressValue() + 12;

			// execute code
			InplaceExecution executor(GetOwner(), executionParams, "APC_KERNEL");
			if (executor.Execute())
			{
				const auto userNormalRoutine = scratchData->m_functionPtr;
				const auto userNormalContext = scratchData->m_functionContext;
				const auto userSystemArg1 = scratchData->m_arg1;
				const auto userSystemArg2 = scratchData->m_arg2;

				// Call the normal routine. Note that it may have been killed by the kernel routine.
				if (userNormalRoutine.GetAddress())
				{
					UnlockAPC();

					// normal_routine(normal_context, system_arg1, system_arg2)
					InplaceExecutionParams userExecutionParams;
					userExecutionParams.m_entryPoint = userNormalRoutine.GetAddress();
					userExecutionParams.m_stackSize = 16 * 1024;
					userExecutionParams.m_threadId = GetIndex();
					userExecutionParams.m_args[0] = userNormalContext;
					userExecutionParams.m_args[1] = userSystemArg1;
					userExecutionParams.m_args[2] = userSystemArg2;

					InplaceExecution userExecutor(GetOwner(), userExecutionParams, "APC_NORMAL");
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
			const uint32 apcAddress = m_apcList->Pop().GetAddress().GetAddressValue() - 8;
			const uint32 rundownRoutine = cpu::mem::loadAddr<uint32>(apcAddress + 20);

			// Mark as uninserted so that it can be reinserted again by the routine.
			const uint32 oldFlags = cpu::mem::loadAddr<uint32>(apcAddress + 40);
			cpu::mem::storeAddr<uint32>(apcAddress + 40, oldFlags & ~0xFF00);
			TagMemoryWrite(apcAddress + 40, 4, "KERNEL_CLEANUP_APC");

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