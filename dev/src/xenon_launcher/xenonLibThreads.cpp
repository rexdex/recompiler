#include "build.h"
#include "xenonLibNatives.h" 
#include "xenonLib.h"  
#include "xenonThread.h"
#include "xenonKernel.h"
#include "xenonPlatform.h"
#include "xenonBindings.h"

namespace xenon
{
	namespace lib
	{

		//---------------------------------------------------------------------------

		uint64 __fastcall Xbox_NtClose(uint64 ip, cpu::CpuRegs& regs)
		{
			const uint32 kernelHandle = (const uint32)regs.R3;

			// resolve
			auto* obj = GPlatform.GetKernel().ResolveAny(kernelHandle);
			if (!obj)
				RETURN_ARG(0);

			// closing handle
			if (!obj->IsRefCounted())
			{
				GLog.Err("Kernel: Trying to close '%s', handle=%08Xh which is not refcoutned", obj->GetName(), obj->GetHandle());
				RETURN_ARG(0);
			}

			// close the object
			GLog.Log("Kernel: Releasing '%s', handle=%08Xh", obj->GetName(), obj->GetHandle());

			auto* refCountedObj = static_cast<xenon::IKernelObjectRefCounted*>(obj);
			refCountedObj->Release();

			RETURN_ARG(0);
		}

		//---------------------------------------------------------------------------

		uint64 __fastcall Xbox_ExCreateThread(uint64 ip, cpu::CpuRegs& regs)
		{
			const uint32 handle_ptr = (const uint32)regs.R3;
			const uint32 stack_size = (const uint32)regs.R4;
			const uint32 thread_id_ptr = (const uint32)regs.R5;
			const uint32 xapi_thread_startup = (const uint32)regs.R6;
			const uint32 start_address = (const uint32)regs.R7;
			const uint32 start_context = (const uint32)regs.R8;
			const uint32 creation_flags = (const uint32)regs.R9;
			GLog.Log("ExCreateThread (handle=%08X, stack=%d, idPtr=%08X, xapi==%08Xh, start=%08Xh, context=%d, flags=%d)",
				handle_ptr, stack_size, thread_id_ptr, xapi_thread_startup, start_address, start_context, creation_flags);

			// Setup thread params
			static uint32 ThreadCounter = 0;
			xenon::KernelThreadParams params;
			sprintf_s(params.m_name, "XThread%d", ++ThreadCounter);
			params.m_stackSize = stack_size;
			params.m_suspended = (creation_flags & 0x1) != 0;

			// XAPI stuff
			if (xapi_thread_startup != 0)
			{
				params.m_entryPoint = xapi_thread_startup;
				params.m_args[0] = start_address;
				params.m_args[1] = start_context;
			}
			else
			{
				params.m_entryPoint = start_address;
				params.m_args[0] = start_context;
			}

			// start thread
			xenon::KernelThread* thread = GPlatform.GetKernel().CreateThread(params);
			if (!thread)
			{
				GLog.Err("ExCreateThread: failed to create thread in process");
				RETURN_ARG(X_ERROR_ACCESS_DENIED);
			}

			// thread created, return thread handle
			*(uint32*)handle_ptr = _byteswap_ulong(thread->GetHandle());

			// save thread id
			if (thread_id_ptr)
				*(uint32*)thread_id_ptr = _byteswap_ulong(thread->GetIndex());

			// no errors
			RETURN_ARG(0);
		}

		uint64 __fastcall Xbox_ExTerminateThread(uint64 ip, cpu::CpuRegs& regs)
		{
			// get exit code
			const auto tid = xenon::KernelThread::GetCurrentThreadID();
			const auto exitCode = (uint32)regs.R3;
			GLog.Log("ExTerminateThread(%d) for TID %d", exitCode, tid);

			// exit the thread directly
			throw runtime::TerminateThreadException(ip, tid, exitCode);

			// should not return
			RETURN_ARG(0);
		}

		uint64 __fastcall Xbox_NtResumeThread(uint64 ip, cpu::CpuRegs& regs)
		{
			const uint32 handle = (const uint32)regs.R3;
			const uint32 suspend_count_ptr = (const uint32)regs.R4;
			GLog.Log("NtResumeThread(%08Xh, %08Xh)", handle, suspend_count_ptr);

			// resolve
			auto* obj = GPlatform.GetKernel().ResolveHandle(handle, xenon::KernelObjectType::Thread);
			if (!obj)
				RETURN_ARG(X_ERROR_BAD_ARGUMENTS);

			// find thread
			auto* thread = static_cast<xenon::KernelThread*>(obj);

			// resume the thread
			uint32 suspend_count = 0;
			thread->Resume();
			RETURN_ARG(0);
		}

		uint64 __fastcall Xbox_KeResumeThread(uint64 ip, cpu::CpuRegs& regs)
		{
			DebugBreak();
			RETURN_DEFAULT();
		}

		uint64 __fastcall Xbox_NtSuspendThread(uint64 ip, cpu::CpuRegs& regs)
		{
			DebugBreak();
			RETURN_DEFAULT();
		}

		uint64 __fastcall Xbox_KeSetAffinityThread(uint64 ip, cpu::CpuRegs& regs)
		{
			const uint32 threadPtr = (const uint32)regs.R3;
			const uint32 affinity = (const uint32)regs.R4;

			// resolve
			auto* obj = GPlatform.GetKernel().ResolveObject(threadPtr, xenon::NativeKernelObjectType::ThreadObject);
			if (!obj)
				RETURN_ARG(X_ERROR_BAD_ARGUMENTS);

			// find thread
			auto* thread = static_cast<xenon::KernelThread*>(obj);
			const uint32 oldAffinity = thread->SetAffinity(affinity);
			RETURN_ARG(oldAffinity);
		}

		uint64 __fastcall Xbox_KeQueryBasePriorityThread(uint64 ip, cpu::CpuRegs& regs)
		{
			DebugBreak();
			RETURN_DEFAULT();
		}

		uint64 __fastcall Xbox_KeSetBasePriorityThread(uint64 ip, cpu::CpuRegs& regs)
		{
			const uint32 threadPtr = (const uint32)regs.R3;
			const uint32 increment = (const uint32)regs.R4;

			// resolve
			auto* obj = GPlatform.GetKernel().ResolveObject(threadPtr, xenon::NativeKernelObjectType::ThreadObject);
			if (!obj)
				RETURN_ARG(X_ERROR_BAD_ARGUMENTS);

			// not a thread
			auto* thread = static_cast<xenon::KernelThread*>(obj);
			int prevPriority = thread->GetPriority();
			thread->SetPriority(increment);

			RETURN_ARG(prevPriority);
		}

		uint64 __fastcall Xbox_KeDelayExecutionThread(uint64 ip, cpu::CpuRegs& regs)
		{
			const uint32 processor_mode = (const uint32)regs.R3;
			const uint32 alertable = (const uint32)regs.R4;
			const uint32 internalPtr = (const uint32)regs.R5;
			const uint64 interval = cpu::mem::loadAddr<uint64>(internalPtr);

			auto* currentThread = xenon::KernelThread::GetCurrentThread();
			const uint32 result = currentThread->Delay(processor_mode, alertable, interval);

			RETURN_ARG(result);
		}

		uint64 __fastcall Xbox_NtYieldExecution(uint64 ip, cpu::CpuRegs& regs)
		{
			auto* currentThread = xenon::KernelThread::GetCurrentThread();
			const uint32 result = 0;//currentThread->Delay(processor_mode, alertable, interval.Get());

			RETURN_ARG(result);
		}

		uint64 __fastcall Xbox_KeQuerySystemTime(uint64 ip, cpu::CpuRegs& regs)
		{
			const uint32 dataPtr = (const uint32)regs.R3;
			GLog.Log("KeQuerySystemTime(0x%08X)", dataPtr);
			cpu::mem::storeAddr<uint64>(dataPtr, 0);
			xenon::TagMemoryWrite(dataPtr, 8, "KeQuerySystemTime");
			RETURN_DEFAULT();
		}

		uint64 __fastcall Xbox_KeTlsAlloc(uint64 ip, cpu::CpuRegs& regs)
		{
			auto* currentThread = xenon::KernelThread::GetCurrentThread();
			const uint32 index = currentThread->GetOwner()->AllocTLSIndex();
			RETURN_ARG(index);
		}

		uint64 __fastcall Xbox_KeTlsFree(uint64 ip, cpu::CpuRegs& regs)
		{
			const uint32 index = (uint32)regs.R3;

			auto* currentThread = xenon::KernelThread::GetCurrentThread();
			currentThread->GetOwner()->FreeTLSIndex(index);

			RETURN();
		}

		uint64 __fastcall Xbox_KeTlsGetValue(uint64 ip, cpu::CpuRegs& regs)
		{
			const uint32 tlsIndex = (uint32)regs.R3;

			auto* currentThread = xenon::KernelThread::GetCurrentThread();
			const uint64 value = currentThread->GetTLS().Get(tlsIndex);

			RETURN_ARG(value);
		}

		uint64 __fastcall Xbox_KeTlsSetValue(uint64 ip, cpu::CpuRegs& regs)
		{
			const uint32 tlsIndex = (uint32)regs.R3;
			const uint64 tlsValue = regs.R4;

			auto* currentThread = xenon::KernelThread::GetCurrentThread();
			currentThread->GetTLS().Set(tlsIndex, tlsValue);

			RETURN_ARG(1)
		}

		//-----------------

		uint64 __fastcall Xbox_NtCreateEvent(uint64 ip, cpu::CpuRegs& regs)
		{
			const uint32 outPtr = (const uint32)regs.R3;
			const uint32 attrPtr = (const uint32)regs.R4;
			const uint32 event_type = (const uint32)regs.R5;
			const uint32 initial_state = (const uint32)regs.R6;

			const bool manualReset = !event_type;
			const bool initialState = (0 != initial_state);

			auto* evt = GPlatform.GetKernel().CreateEvent(initialState, manualReset);
			if (!evt)
				RETURN_ARG(X_ERROR_BAD_ARGUMENTS);

			cpu::mem::storeAddr(outPtr, evt->GetHandle());
			xenon::TagMemoryWrite(outPtr, 4, "NtCreateEvent");
			RETURN_ARG(0);
		}

		uint64 __fastcall Xbox_KeSetEvent(uint64 ip, cpu::CpuRegs& regs)
		{
			const uint32 eventPtr = (const uint32)regs.R3;
			const uint32 increment = (const uint32)regs.R4;
			const uint32 wait = (const uint32)regs.R5;

			auto* nativeEvent = (XEVENT*) eventPtr;
			cpu::mem::storeAtomic<uint32>(&nativeEvent->Dispatch.SignalState, 1);
			xenon::TagMemoryWrite(&nativeEvent->Dispatch.SignalState, 4, "KeSetEvent");

			auto* obj = GPlatform.GetKernel().ResolveObject(eventPtr, xenon::NativeKernelObjectType::EventNotificationObject);
			if (!obj)
			{
				DebugBreak();
				RETURN_ARG(0);
			}

			auto* evt = static_cast<xenon::KernelEvent*>(obj);
			const auto ret = evt->Set(increment, !!wait);
			RETURN_ARG(ret);
		}

		uint64 __fastcall Xbox_NtSetEvent(uint64 ip, cpu::CpuRegs& regs)
		{
			const uint32 eventHandle = (const uint32)regs.R3;
			const uint32 statePtr = (const uint32)regs.R4;

			auto* obj = GPlatform.GetKernel().ResolveHandle(eventHandle, xenon::KernelObjectType::Event);
			if (!obj)
				RETURN_ARG(-1);

			auto* evt = static_cast<xenon::KernelEvent*>(obj);
			const uint32 ret = evt->Set(0, false);

			if (statePtr)
			{
				cpu::mem::storeAddr(statePtr, ret);
				xenon::TagMemoryWrite(statePtr, 4, "NtSetEvent");
			}

			RETURN_ARG(0);
		}

		uint64 __fastcall Xbox_KePulseEvent(uint64 ip, cpu::CpuRegs& regs)
		{
			DebugBreak();
			RETURN_DEFAULT();
		}

		uint64 __fastcall Xbox_NtPulseEvent(uint64 ip, cpu::CpuRegs& regs)
		{
			const uint32 evtHandle = (const uint32)regs.R3;
			const uint32 statePtr = (const uint32)regs.R4;

			auto* obj = GPlatform.GetKernel().ResolveHandle(evtHandle, xenon::KernelObjectType::Event);
			if (!obj)
				RETURN_ARG(-1);

			auto* evt = static_cast<xenon::KernelEvent*>(obj);
			const uint32 ret = evt->Set(0, false);

			if (statePtr)
			{
				cpu::mem::storeAddr(statePtr, ret);
				xenon::TagMemoryWrite(statePtr, 4, "NtPulseEvent");
			}


			RETURN_ARG(ret);
		}

		uint64 __fastcall Xbox_KeResetEvent(uint64 ip, cpu::CpuRegs& regs)
		{
			const uint32 eventPtr = (const uint32)regs.R3;

			auto* nativeEvent = (XEVENT*) eventPtr;
			cpu::mem::storeAtomic<uint32>(&nativeEvent->Dispatch.SignalState, 0);
			xenon::TagMemoryWrite(&nativeEvent->Dispatch.SignalState, 0, "KeResetEvent");


			auto* obj = GPlatform.GetKernel().ResolveObject((uint32_t)nativeEvent, xenon::NativeKernelObjectType::EventNotificationObject);
			if (!obj)
			{
				DebugBreak();
				RETURN_ARG(0);
			}

			auto* evt = static_cast<xenon::KernelEvent*>(obj);
			const uint32 ret = evt->Reset();

			RETURN_ARG(ret);
		}

		uint64 __fastcall Xbox_NtClearEvent(uint64 ip, cpu::CpuRegs& regs)
		{
			const uint32 evtHandle = (const uint32)regs.R3;

			auto* obj = GPlatform.GetKernel().ResolveHandle(evtHandle, xenon::KernelObjectType::Event);
			if (!obj)
				RETURN_ARG(-1);

			auto* evt = static_cast<xenon::KernelEvent*>(obj);
			const uint32 ret = evt->Reset();
			RETURN_ARG(0);
		}

		//-----------------

		uint64 __fastcall Xbox_NtCreateSemaphore(uint64 ip, cpu::CpuRegs& regs)
		{
			DebugBreak();
			RETURN_DEFAULT();
		}

		uint64 __fastcall Xbox_KeInitializeSemaphore(uint64 ip, cpu::CpuRegs& regs)
		{
			DebugBreak();
			RETURN_DEFAULT();
		}

		uint64 __fastcall Xbox_KeReleaseSemaphore(uint64 ip, cpu::CpuRegs& regs)
		{
			DebugBreak();
			RETURN_DEFAULT();
		}

		uint64 __fastcall Xbox_NtReleaseSemaphore(uint64 ip, cpu::CpuRegs& regs)
		{
			DebugBreak();
			RETURN_DEFAULT();
		}

		//-----------------

		uint64 __fastcall Xbox_NtCreateMutant(uint64 ip, cpu::CpuRegs& regs)
		{
			DebugBreak();
			RETURN_DEFAULT();
		}

		uint64 __fastcall Xbox_NtReleaseMutant(uint64 ip, cpu::CpuRegs& regs)
		{
			DebugBreak();
			RETURN_DEFAULT();
		}

		//-----------------

		uint64 __fastcall Xbox_NtCreateTimer(uint64 ip, cpu::CpuRegs& regs)
		{
			DebugBreak();
			RETURN_DEFAULT();
		}

		uint64 __fastcall Xbox_NtSetTimerEx(uint64 ip, cpu::CpuRegs& regs)
		{
			DebugBreak();
			RETURN_DEFAULT();
		}

		uint64 __fastcall Xbox_NtCancelTimer(uint64 ip, cpu::CpuRegs& regs)
		{
			DebugBreak();
			RETURN_DEFAULT();
		}

		//-----------------

		uint64 __fastcall Xbox_NtSignalAndWaitForSingleObjectEx(uint64 ip, cpu::CpuRegs& regs)
		{
			RETURN_DEFAULT();
		}

		uint64 __fastcall Xbox_KfAcquireSpinLock(uint64 ip, cpu::CpuRegs& regs)
		{
			const uint32 lockAddr = (const uint32)regs.R3;

			volatile unsigned int* lock = reinterpret_cast<volatile unsigned int*>(lockAddr);
			while (!InterlockedCompareExchange(lock, 1, 0) != 0)
			{
			}

			// Raise IRQL to DISPATCH to make sure this thread is executed
			auto* thread = xenon::KernelThread::GetCurrentThread();
			const uint32 oldIRQL = thread ? thread->RaiseIRQL(2) : 0;

			RETURN_ARG(oldIRQL);
		}

		uint64 __fastcall Xbox_KfReleaseSpinLock(uint64 ip, cpu::CpuRegs& regs)
		{
			const uint32 lockAddr = (const uint32)regs.R3;
			const uint32 oldIRQL = (const uint32)regs.R4;

			// Restore IRQL
			auto* thread = xenon::KernelThread::GetCurrentThread();
			if (thread)
				thread->LowerIRQL(oldIRQL);

			// Unlock
			volatile unsigned int* lock = reinterpret_cast<volatile unsigned int*>(lockAddr);
			InterlockedDecrement(lock);

			RETURN();
		}

		uint64 __fastcall Xbox_KeAcquireSpinLockAtRaisedIrql(uint64 ip, cpu::CpuRegs& regs)
		{
			const uint32 lockAddr = (const uint32)regs.R3;

			volatile unsigned int* lock = reinterpret_cast<volatile unsigned int*>(lockAddr);
			while (!InterlockedCompareExchange(lock, 1, 0) != 0)
			{
			}

			RETURN();
		}

		uint64 __fastcall Xbox_KeReleaseSpinLockFromRaisedIrql(uint64 ip, cpu::CpuRegs& regs)
		{
			const uint32 lockAddr = (const uint32)regs.R3;

			// Unlock
			volatile unsigned int* lock = reinterpret_cast<volatile unsigned int*>(lockAddr);
			InterlockedDecrement(lock);

			RETURN();
		}

		//-----------------

		uint64 __fastcall Xbox_KeEnterCriticalRegion(uint64 ip, cpu::CpuRegs& regs)
		{
			auto* currentThread = xenon::KernelThread::GetCurrentThread();
			RETURN_ARG(currentThread->EnterCriticalRegion());
		}

		uint64 __fastcall Xbox_KeLeaveCriticalRegion(uint64 ip, cpu::CpuRegs& regs)
		{
			auto* currentThread = xenon::KernelThread::GetCurrentThread();
			RETURN_ARG(currentThread->LeaveCriticalRegion());
		}

		//-----------------

		uint64 __fastcall Xbox_NtQueueApcThread(uint64 ip, cpu::CpuRegs& regs)
		{
			DebugBreak();
			RETURN_DEFAULT();
		}

		uint64 __fastcall Xbox_KeInitializeApc(uint64 ip, cpu::CpuRegs& regs)
		{
			DebugBreak();
			RETURN_DEFAULT();
		}

		uint64 __fastcall Xbox_KeInsertQueueApc(uint64 ip, cpu::CpuRegs& regs)
		{
			DebugBreak();
			RETURN_DEFAULT();
		}

		uint64 __fastcall Xbox_KeRemoveQueueApc(uint64 ip, cpu::CpuRegs& regs)
		{
			DebugBreak();
			RETURN_DEFAULT();
		}

		uint64 __fastcall Xbox_KiApcNormalRoutineNop(uint64 ip, cpu::CpuRegs& regs)
		{
			const uint32 param0 = (const uint32)regs.R3;
			const uint32 param1 = (const uint32)regs.R4;

			GLog.Log("KiApcNormalRoutineNop(0x%08X, 0x%08X)", param0, param1);

			RETURN_ARG(0);
		}

		//-----------------

		uint64 __fastcall Xbox_KeWaitForSingleObject(uint64 ip, cpu::CpuRegs& regs)
		{
			const uint32 objectPtr = (const uint32)regs.R3;
			const uint32 waitReason = (const uint32)regs.R4;
			const uint32 processorMode = (const uint32)regs.R5;
			const uint32 alertable = (const uint32)regs.R6;
			const uint32 timeoutPtr = (const uint32)regs.R7;

			// resolve object
			auto* object = GPlatform.GetKernel().ResolveObject(objectPtr, xenon::NativeKernelObjectType::Unknown, false);
			if (!object)
			{
				GLog.Err("WaitSingle: object %08Xh does not exist", objectPtr);
				RETURN_ARG(X_STATUS_ABANDONED_WAIT_0);
			}

			// object is not waitable
			if (!object->CanWait())
			{
				GLog.Err("WaitSingle: object %08Xh, '%s' is not waitable", objectPtr, object->GetName());
				RETURN_ARG(X_STATUS_ABANDONED_WAIT_0);
			}

			const int64 timeout = timeoutPtr ? cpu::mem::loadAddr<int64>(timeoutPtr) : 0;
			auto* waitObject = static_cast<xenon::IKernelWaitObject*>(object);
			X_STATUS result = waitObject->Wait(waitReason, processorMode, !!alertable, timeoutPtr ? &timeout : nullptr);
			RETURN_ARG(result);
		}

		uint64 __fastcall Xbox_NtWaitForSingleObjectEx(uint64 ip, cpu::CpuRegs& regs)
		{
			const uint32 object_handle = (const uint32)regs.R3;
			const uint8 wait_mode = (const uint8)regs.R4;
			const uint32 alertable = (const uint32)regs.R5;
			const uint32 timeoutPtr = (const uint32)regs.R6;
			GLog.Log("WaitSingle(%08Xh, mode=%d, alert=%d, tptr=%08Xh, timeout=%d)", object_handle, wait_mode, alertable, timeoutPtr, timeoutPtr ? cpu::mem::loadAddr<int64>(timeoutPtr) : 0);

			auto* object = GPlatform.GetKernel().ResolveHandle(object_handle, xenon::KernelObjectType::Unknown);
			if (!object)
			{
				GLog.Err("WaitSingle: object with handle %08Xh does not exist", object_handle);
				RETURN_ARG(X_STATUS_ABANDONED_WAIT_0);
			}

			if (!object->CanWait())
			{
				GLog.Err("WaitSingle: object %08Xh, '%s' is not waitable", object_handle, object->GetName());
				RETURN_ARG(X_STATUS_ABANDONED_WAIT_0);
			}

			auto* waitObject = static_cast<xenon::IKernelWaitObject*>(object);

			const int64 timeout = timeoutPtr ? cpu::mem::loadAddr<int64>(timeoutPtr) : 0;
			const uint32 result = waitObject->Wait(3, wait_mode, !!alertable, timeoutPtr ? &timeout : nullptr);
			RETURN_ARG(result);
		}

		uint64 __fastcall Xbox_KeWaitForMultipleObjects(uint64 ip, cpu::CpuRegs& regs)
		{
			const auto count = (uint32)regs.R3;
			const auto objectsPtr = Pointer<uint32>(regs.R4);
			const auto waitType = (uint32)regs.R5;
			const auto waitReason = (uint32)regs.R6;
			const auto unkn = (uint32)regs.R7;
			const auto alterable = (uint32)regs.R8;
			const auto timeoutPtr = Pointer<uint32>(regs.R9);
			const auto waitBlockPtr = Pointer<uint32>(regs.R10);

			DEBUG_CHECK(waitType == 0 || waitType == 1);

			const auto timeOut = timeoutPtr.IsValid() ? *timeoutPtr : 0;

			std::vector<xenon::IKernelWaitObject*> waitObjects;
			for (uint32_t i = 0; i < count; ++i)
			{
				auto objectPtr = objectsPtr[i];
				if (objectPtr)
				{
					auto objectData = GPlatform.GetKernel().ResolveObject(objectPtr, xenon::NativeKernelObjectType::Unknown, true);
					if (!objectData || !objectData->CanWait())
						RETURN_ARG(X_STATUS_INVALID_PARAMETER);

					waitObjects.push_back(static_cast<xenon::IKernelWaitObject*>(objectData));
				}
			}

			const auto waitAll = (waitType == 1);
			const auto ret = GPlatform.GetKernel().WaitMultiple(waitObjects, waitAll, timeOut, alterable != 0);
			RETURN_ARG(ret);
		}

		uint64 __fastcall Xbox_NtWaitForMultipleObjectsEx(uint64 ip, cpu::CpuRegs& regs)
		{
			DebugBreak();
			RETURN_DEFAULT();
		}

		//---------------------------------------------------------------------------

		void RtlInitializeCriticalSection(XCRITICAL_SECTION* rtlCS)
		{
			auto* cs = GPlatform.GetKernel().CreateCriticalSection();
			auto* thread = xenon::KernelThread::GetCurrentThread();

			rtlCS->Synchronization.RawEvent[0] = 0xDAAD0FF0;
			rtlCS->Synchronization.RawEvent[1] = cs->GetHandle();
			rtlCS->OwningThread = thread ? thread->GetHandle() : 0;
			rtlCS->RecursionCount = 0;
			rtlCS->LockCount = 0;

			const uint32 kernelMarker = rtlCS->Synchronization.RawEvent[0];
			DEBUG_CHECK(kernelMarker == 0xDAAD0FF0);
		}

		uint64 __fastcall Xbox_RtlInitializeCriticalSection(uint64 ip, cpu::CpuRegs& regs)
		{
			auto rtlCS = Pointer<XCRITICAL_SECTION>(regs.R3);
			RtlInitializeCriticalSection(rtlCS.GetNativePointer());
			RETURN_ARG(0);
		}

		static void XInitCriticalSection(XCRITICAL_SECTION* ptr)
		{
			auto* cs = GPlatform.GetKernel().CreateCriticalSection();
			auto* thread = xenon::KernelThread::GetCurrentThread();

			ptr->Synchronization.RawEvent[0] = 0xDAAD0FF0;
			ptr->Synchronization.RawEvent[1] = cs->GetHandle();
			ptr->OwningThread = thread->GetHandle();
			ptr->RecursionCount = 0;
			ptr->LockCount = 0;

			const uint32 kernelMarker = ptr->Synchronization.RawEvent[0];
			DEBUG_CHECK(kernelMarker == 0xDAAD0FF0);
		}

		uint64 __fastcall Xbox_RtlInitializeCriticalSectionAndSpinCount(uint64 ip, cpu::CpuRegs& regs)
		{
			auto rtlCS = Pointer<XCRITICAL_SECTION>(regs.R3);
			XInitCriticalSection(rtlCS.GetNativePointer());
			RETURN_ARG(0);
		}

		uint64 __fastcall Xbox_RtlDeleteCriticalSection(uint64 ip, cpu::CpuRegs& regs)
		{
			auto rtlCS = Pointer<XCRITICAL_SECTION>(regs.R3);
			XInitCriticalSection(rtlCS.GetNativePointer());
			RETURN_ARG(0);
		}

		uint64 __fastcall Xbox_RtlLeaveCriticalSection(uint64 ip, cpu::CpuRegs& regs)
		{
			auto ptr = Pointer<XCRITICAL_SECTION>(regs.R3);
			DEBUG_CHECK(ptr.IsValid());

			const uint32 kernelMarker = ptr->Synchronization.RawEvent[0];
			DEBUG_CHECK(kernelMarker == 0xDAAD0FF0);

			const uint32 kernelHandle = ptr->Synchronization.RawEvent[1];

			auto* obj = GPlatform.GetKernel().ResolveHandle(kernelHandle, xenon::KernelObjectType::CriticalSection);
			DEBUG_CHECK(obj);

			auto* cs = static_cast<xenon::KernelCriticalSection*>(obj);
			cs->Leave();

			RETURN();
		}

		uint64 __fastcall Xbox_RtlEnterCriticalSection(uint64 ip, cpu::CpuRegs& regs)
		{
			const auto ptr = Pointer<XCRITICAL_SECTION>(regs.R3);
			DEBUG_CHECK(ptr.IsValid());

			const auto ptrAddr = ptr.GetAddress();
			const uint32 kernelMarker = ptr->Synchronization.RawEvent[0];
			if (ptrAddr >= 0x82000000)
			{
				if (kernelMarker != 0xDAAD0FF0)
				{
					XInitCriticalSection(ptr.GetNativePointer());
				}
			}
			else
			{
				DEBUG_CHECK(kernelMarker == 0xDAAD0FF0);
			}

			const uint32 kernelHandle = ptr->Synchronization.RawEvent[1];
			auto* obj = GPlatform.GetKernel().ResolveHandle(kernelHandle, xenon::KernelObjectType::CriticalSection);
			DEBUG_CHECK(obj);

			auto* cs = static_cast<xenon::KernelCriticalSection*>(obj);
			cs->Enter();

			RETURN();
		}

		//---------------------------------------------------------------------------

		void RegisterXboxThreads(runtime::Symbols& symbols)
		{
			REGISTER(NtClose);

			REGISTER(RtlInitializeCriticalSection);
			REGISTER(RtlInitializeCriticalSectionAndSpinCount);
			REGISTER(RtlDeleteCriticalSection);
			REGISTER(RtlEnterCriticalSection);
			REGISTER(RtlLeaveCriticalSection);

			REGISTER(KeTlsAlloc);
			REGISTER(KeTlsFree);
			REGISTER(KeTlsGetValue);
			REGISTER(KeTlsSetValue);

			REGISTER(ExCreateThread);
			REGISTER(ExTerminateThread);
			REGISTER(NtResumeThread);
			REGISTER(KeResumeThread);
			REGISTER(NtSuspendThread);
			REGISTER(KeSetAffinityThread);
			REGISTER(KeQueryBasePriorityThread);
			REGISTER(KeSetBasePriorityThread);
			REGISTER(KeDelayExecutionThread);
			REGISTER(NtYieldExecution);
			REGISTER(KeQuerySystemTime);

			REGISTER(NtCreateEvent);
			REGISTER(KeSetEvent);
			REGISTER(NtSetEvent);
			REGISTER(KePulseEvent);
			REGISTER(NtPulseEvent);
			REGISTER(KeResetEvent);
			REGISTER(NtClearEvent);

			REGISTER(NtCreateSemaphore);
			REGISTER(KeInitializeSemaphore);
			REGISTER(KeReleaseSemaphore);
			REGISTER(NtReleaseSemaphore);
			REGISTER(NtCreateMutant);
			REGISTER(NtReleaseMutant);
			REGISTER(NtCreateTimer);
			REGISTER(NtSetTimerEx);
			REGISTER(NtCancelTimer);
			REGISTER(KeWaitForSingleObject);
			REGISTER(NtWaitForSingleObjectEx);
			REGISTER(KeWaitForMultipleObjects);
			REGISTER(NtWaitForMultipleObjectsEx);
			REGISTER(NtSignalAndWaitForSingleObjectEx);
			REGISTER(KfAcquireSpinLock);
			REGISTER(KfReleaseSpinLock);
			REGISTER(KeAcquireSpinLockAtRaisedIrql);
			REGISTER(KeReleaseSpinLockFromRaisedIrql);
			REGISTER(KeEnterCriticalRegion);
			REGISTER(KeLeaveCriticalRegion);
			REGISTER(NtQueueApcThread);
			REGISTER(KeInitializeApc);
			REGISTER(KeInsertQueueApc);
			REGISTER(KeRemoveQueueApc);
			REGISTER(KiApcNormalRoutineNop);
		}

	} // lib
} // xenon