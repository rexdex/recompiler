#include "build.h"
#include "xenonKernel.h"
#include "xenonThread.h"
#include "xenonInplaceExecution.h"

#include "..\..\..\launcher\backend\runtimeCodeExecutor.h"
#include "..\..\..\launcher\backend\runtimeTraceWriter.h"

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

	KernelThread::KernelThread(Kernel* kernel, runtime::TraceWriter* traceFile, const KernelThreadParams& params)
		: IKernelWaitObject(kernel, KernelObjectType::Thread, "Thread")
		, m_code(&m_regs, &kernel->GetCode(), params.m_entryPoint)
		, m_memory( kernel, params.m_stackSize, GetIndex(), params.m_entryPoint, 1 )
		, m_traceFile( traceFile )
		, m_tls( kernel )
		, m_isStopped(false) // not stopped yet
		, m_isCrashed(false) // not crashed yet
		, m_isWaiting(false) // no waiting state
		, m_priority(0) // thread priority normal
		, m_criticalRegion(false)
		, m_requestExit(false)
		, m_affinity(0xFF)
		, m_irql(0)
	{
		// create APC list
		m_apcList = new KernelList();
		m_apcLock = kernel->GetNativeKernel().CreateCriticalSection();

		// register descriptor in kernel linked lists
		SetNativePointer((void*)m_memory.GetThreadDataAddr()); // NOTE: we are not binding it to the thread

		// setup initial register state
		m_regs.R1 = (uint64)m_memory.GetStack().GetTop();
		m_regs.R3 = params.m_args[0];
		m_regs.R4 = params.m_args[1];
		m_regs.R13 = m_memory.GetPRCAddr(); // always at r13
		m_regs.RES = &GReservation;

		// reservation locking
		GReservation.LOCK = &ReservationLock;
		GReservation.UNLOCK = &ReservationUnlock;

		// create the native thread
		m_nativeThread = kernel->GetNativeKernel().CreateThread(this);
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

		// delete APC lock
		delete m_apcLock;
		m_apcLock = nullptr;

		// if we still have trace file close it now (should be closed on the thread)
		delete m_traceFile;
		m_traceFile = nullptr;
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

	static INT ThreadExceptions(DWORD code, struct _EXCEPTION_POINTERS *ep, const uint32 ip, int32* exitCode)
	{
		// debug print :)
		if (code == (cpu::EXCEPTION_CODE_TRAP | 0x1F))
		{
			const uint32 ip = (const uint32)ep->ExceptionRecord->ExceptionInformation[0];
			const uint32 type = (const uint32)ep->ExceptionRecord->ExceptionInformation[1];
			const cpu::CpuRegs& regs = *(const cpu::CpuRegs*) ep->ExceptionRecord->ExceptionInformation[2];

			if (type == 20)
			{
				const char* txt = (const char*)(regs.R3 & 0xFFFFFFFF);
				GLog.Log("Debug: %s", txt);
				return EXCEPTION_CONTINUE_EXECUTION;
			}
		}

		// execution exceptions
		if (code == EXCEPTION_ACCESS_VIOLATION)
		{
			GLog.Err("Thread: Access violation %s memory %06llXh",
				ep->ExceptionRecord->ExceptionInformation[0] ? "writing" : "reading",
				(uint64)ep->ExceptionRecord->ExceptionInformation[1]);
			*exitCode = -1;

			if (IsDebuggerPresent())
				return EXCEPTION_CONTINUE_SEARCH;
		}
		else if ((code & 0xFFFFFF00) == cpu::EXCEPTION_CODE_TRAP)
		{
			const uint32 flags = code & 0xFF;

			const uint32 ip = (const uint32)ep->ExceptionRecord->ExceptionInformation[0];
			const uint64 val = (const uint64)ep->ExceptionRecord->ExceptionInformation[1];
			const uint64 cmp = (const uint64)ep->ExceptionRecord->ExceptionInformation[2];

			if (flags == 0x1F) // all flags set - unconditional trap
			{
				GLog.Err("Thread: Unconditional trap at IP=%06Xh", ip);
				*exitCode = -2;
			}
			else
			{
				GLog.Err("Thread: Trap %d at IP=%06llXh with value=%d and trigger=%d",
					flags, ip, val, cmp);
				*exitCode = -3;
			}
		}
		else if ((code & 0xFFFFFF00) == cpu::EXCEPTION_INVALID_CODE_ADDRESS)
		{
			const uint32 ip = (const uint32)ep->ExceptionRecord->ExceptionInformation[0];
			const uint32 blockIp = (const uint32)ep->ExceptionRecord->ExceptionInformation[1];

			GLog.Err("Thread: Invalid branch IP=%06Xh in block at %06Xh", ip, blockIp);
			*exitCode = -4;
		}
		else if ((code & 0xFFFFFF00) == cpu::EXCEPTION_INVALID_CODE_INSTRUCTON)
		{
			const uint32 ip = (const uint32)ep->ExceptionRecord->ExceptionInformation[0];
			const uint32 blockIp = (const uint32)ep->ExceptionRecord->ExceptionInformation[1];

			GLog.Err("Thread: Invalid instruction at IP=%06Xh in block at %06Xh", ip, blockIp);
			*exitCode = -1;
		}
		else if ((code & 0xFFFFFF00) == cpu::EXCEPTION_UNIMPLEMENTED_FUNCTION)
		{
			const uint32 ip = (const uint32)ep->ExceptionRecord->ExceptionInformation[0];
			const char* name = (const char*)ep->ExceptionRecord->ExceptionInformation[1];

			GLog.Err("Thread: Called unimplemented import function '%s' at IP=%06Xh", name, ip);
			*exitCode = -5;
		}
		else if ((code & 0xFFFFFF00) == cpu::EXCEPTION_TERMINATE_PROCESS)
		{
			const uint32 retCode = (const uint32)ep->ExceptionRecord->ExceptionInformation[0];
			GLog.Log("Thread: Got request to terminate the process. This is not an error. ExitCode = %d (%08Xh)", retCode, retCode);
			*exitCode = retCode;
		}
		else if ((code & 0xFFFFFF00) == cpu::EXCEPTION_TERMINATE_THREAD)
		{
			const uint32 retCode = (const uint32)ep->ExceptionRecord->ExceptionInformation[0];
			GLog.Log("Thread: Got request to terminate thread. ExitCode = %d (%08Xh)", retCode, retCode);
			*exitCode = retCode;
		}
		else if ((code & 0xFFFFFF00) == cpu::EXCEPTION_UNHANDLED_INTERRUPT)
		{
			const uint32 ip = (const uint32)ep->ExceptionRecord->ExceptionInformation[0];
			GLog.Err("Thread: Unhandled interrupt at IP=%06Xh", ip);

			*exitCode = -6;
		}
		else if ((code & 0xFFFFFF00) == cpu::EXCEPTION_UNHANDLED_GLOBAL_READ)
		{
			const uint32 ip = (const uint32)ep->ExceptionRecord->ExceptionInformation[0];
			const uint32 address = (const uint32)ep->ExceptionRecord->ExceptionInformation[2];
			const uint32 size = (const uint32)ep->ExceptionRecord->ExceptionInformation[3];
			GLog.Err("Thread: Unhandled read from absolute memory at IP=%06Xh, memory address=%06Xh, size=%d", ip, address, size);

			*exitCode = -7;
		}
		else if ((code & 0xFFFFFF00) == cpu::EXCEPTION_UNHANDLED_GLOBAL_WRITE)
		{
			const uint32 ip = (const uint32)ep->ExceptionRecord->ExceptionInformation[0];
			const uint32 address = (const uint32)ep->ExceptionRecord->ExceptionInformation[2];
			const uint32 size = (const uint32)ep->ExceptionRecord->ExceptionInformation[3];
			GLog.Err("Thread: Unhandled write to absolute memory at IP=%06Xh, memory address=%06Xh, size=%d", ip, address, size);

			*exitCode = -8;
		}
		else if ((code & 0xFFFFFF00) == cpu::EXCEPTION_SYSTEM_KERNEL_ERROR)
		{
			const char* errorText = (const char*)ep->ExceptionRecord->ExceptionInformation[0];
			GLog.Err("Thread: Unhandled system error: %s", errorText);

			*exitCode = -9;
		}
		else if ((code & 0xFFFFFF00) == cpu::EXCEPTION_UNHANDLED_MMAP_READ)
		{
			const uint32 size = (const uint32)ep->ExceptionRecord->ExceptionInformation[2];
			const uint32 address = (const uint32)ep->ExceptionRecord->ExceptionInformation[3];
			GLog.Err("Thread: Unhandled memory mapped read from absolute address=%06Xh, size=%d", address, size);

			*exitCode = -12;
		}
		else if ((code & 0xFFFFFF00) == cpu::EXCEPTION_UNHANDLED_MMAP_WRITE)
		{
			const uint32 size = (const uint32)ep->ExceptionRecord->ExceptionInformation[2];
			const uint32 address = (const uint32)ep->ExceptionRecord->ExceptionInformation[3];
			GLog.Err("Thread: Unhandled memory mapped write to absolute address=%06Xh, size=%d", address, size);

			*exitCode = -13;
		}
		else
		{
			auto* currentThread = KernelThread::GetCurrentThread();
			GLog.Err("Thread: Fatal exception %08X occurred at IP=%06Xh in thread '%s' (ID=%d)", ip, currentThread->GetName(), currentThread->GetIndex());
			*exitCode = -10;
		}

		return EXCEPTION_EXECUTE_HANDLER;
	}

	int KernelThread::Run(native::IThread& thread)
	{
		// bind the current thread
		GCurrentThread = this;

		// stuff
		GLog.Log("Thread: Starting execution at thread %hs, ID=%u", GetName(), GetIndex());

		// start the execution loop
		int exitCode = 0;
		__try
		{
			if (m_traceFile != nullptr)
			{
				m_traceFile->AddFrame(m_code.GetInstructionPointer(), m_regs);

				while (!m_requestExit && m_code.Step())
					m_traceFile->AddFrame(m_code.GetInstructionPointer(), m_regs);
			}
			else
			{
				while (!m_requestExit && m_code.Step()) {};
			}
		}

		__except (ThreadExceptions(GetExceptionCode(), GetExceptionInformation(), (uint32)m_code.GetInstructionPointer(), &exitCode))
		{
			// report error
			if ((int)exitCode < 0)
			{
				GLog.Err("Thread: Thread '%hs' (ID=%u) finished unexpectedly with error code %d (%08Xh)", GetName(), GetIndex(), exitCode, exitCode);
				m_isCrashed = true;
			}
			else
			{
				GLog.Warn("Thread: Thread '%hs' (ID=%u) finished gracefully", GetName(), GetIndex());
			}
		}

		// close trace file, hopefully it contains the last executed instruction
		if (m_traceFile)
		{
			delete m_traceFile;
			m_traceFile = nullptr;
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
		m_apcLock->Acquire();
	}

	void KernelThread::UnlockAPC()
	{
		const bool needsAPC = m_apcList->HasPending();
		m_apcLock->Release();

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
			const uint32 kernelRoutine = mem::loadAddr<uint32>(apcAddress + 16);
			const uint32 normalRoutine = mem::loadAddr<uint32>(apcAddress + 24);
			const uint32 normalContext = mem::loadAddr<uint32>(apcAddress + 28);
			const uint32 systemArg1 = mem::loadAddr<uint32>(apcAddress + 32);
			const uint32 systemArg2 = mem::loadAddr<uint32>(apcAddress + 36);

			// Mark as uninserted so that it can be reinserted again by the routine.
			const uint32 oldFlags = mem::loadAddr<uint32>(apcAddress + 40);
			mem::storeAddr<uint32>(apcAddress + 40, oldFlags & ~0xFF00);

			// Call kernel routine.
			// The routine can modify all of its arguments before passing it on.
			// Since we need to give guest accessible pointers over, we copy things
			// into and out of scratch.
			const uint32 scratchAddr = m_memory.GetScratchAddr();
			mem::storeAddr<uint32>(scratchAddr + 0, normalRoutine);
			mem::storeAddr<uint32>(scratchAddr + 4, normalContext);
			mem::storeAddr<uint32>(scratchAddr + 8, systemArg1);
			mem::storeAddr<uint32>(scratchAddr + 12, systemArg2);

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
			InplaceExecution executor(GetOwner(), executionParams);
			if (executor.Execute())
			{
				const uint32 userNormalRoutine = mem::loadAddr<uint32>(scratchAddr + 0);
				const uint32 userNormalContext = mem::loadAddr<uint32>(scratchAddr + 4);
				const uint32 userSystemArg1 = mem::loadAddr<uint32>(scratchAddr + 8);
				const uint32 userSystemArg2 = mem::loadAddr<uint32>(scratchAddr + 12);

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

					InplaceExecution userExecutor(GetOwner(), userExecutionParams);
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
		LockAPC();

		while (m_apcList->HasPending())
		{
			// Get APC entry (offset for LIST_ENTRY offset) and cache what we need.
			// Calling the routine may delete the memory/overwrite it.
			const uint32 apcAddress = m_apcList->Pop() - 8;
			const uint32 rundownRoutine = mem::loadAddr<uint32>(apcAddress + 20);

			// Mark as uninserted so that it can be reinserted again by the routine.
			const uint32 oldFlags = mem::loadAddr<uint32>(apcAddress + 40);
			mem::storeAddr<uint32>(apcAddress + 40, oldFlags & ~0xFF00);

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
				InplaceExecution executor(GetOwner(), executionParams);
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