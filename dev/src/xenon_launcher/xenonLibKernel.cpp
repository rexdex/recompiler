#include "build.h"
#include "xenonLib.h"  
#include "xenonLibNatives.h"
#include "xenonThread.h"
#include "xenonPlatform.h"
#include "xenonMemory.h"
#include "xenonBindings.h"

namespace xenon
{
	namespace lib
	{

		//---------------------------------------------------------------------------

		X_STATUS Xbox_NtClose(uint32 handle)
		{
			// resolve
			auto* obj = GPlatform.GetKernel().ResolveAny(handle);
			if (!obj)
				return X_STATUS_INVALID_HANDLE;

			// closing handle
			if (!obj->IsRefCounted())
			{
				GLog.Err("Kernel: Trying to close '%s', handle=%08Xh which is not refcoutned", obj->GetName(), obj->GetHandle());
				return X_STATUS_INVALID_HANDLE;
			}

			auto* refCountedObj = static_cast<xenon::IKernelObjectRefCounted*>(obj);
			refCountedObj->Release();

			return X_STATUS_SUCCESS;
		}

		X_STATUS Xbox_ExCreateThread(Pointer<uint32> handlePtr, uint32 stackSize, Pointer<uint32> threadIdPtr, MemoryAddress xapiThreadStartup, MemoryAddress startAddress, uint32 startContext, uint32 creationFlags)
		{
			// Setup thread params
			static uint32 ThreadCounter = 0;
			xenon::KernelThreadParams params;
			sprintf_s(params.m_name, "XThread%d", ++ThreadCounter);
			params.m_stackSize = stackSize;
			params.m_suspended = (creationFlags & 0x1) != 0;

			// XAPI stuff
			if (xapiThreadStartup.IsValid())
			{
				params.m_entryPoint = xapiThreadStartup.GetAddressValue();
				params.m_args[0] = startAddress.GetAddressValue();
				params.m_args[1] = startContext;
			}
			else
			{
				params.m_entryPoint = startAddress.GetAddressValue();
				params.m_args[0] = startContext;
			}

			// start thread
			auto* thread = GPlatform.GetKernel().CreateThread(params);
			if (!thread)
			{
				GLog.Err("ExCreateThread: failed to create thread in process");
				return X_ERROR_ACCESS_DENIED;
			}

			// thread created, return thread handle
			*handlePtr = thread->GetHandle();

			// save thread id
			if (threadIdPtr.IsValid())
				*threadIdPtr = thread->GetIndex();

			// no errors
			return X_STATUS_SUCCESS;
		}

		X_STATUS Xbox_ExTerminateThread(uint32 exitCode)
		{
			// get exit code
			const auto tid = xenon::KernelThread::GetCurrentThreadID();
			GLog.Log("ExTerminateThread(%d) for TID %d", exitCode, tid);

			// exit the thread directly
			throw runtime::TerminateThreadException(0, tid, exitCode);
		}

		X_STATUS Xbox_NtResumeThread(uint32 handle, Pointer<uint32> suspendCountPtr)
		{
			// resolve
			auto* obj = GPlatform.GetKernel().ResolveHandle(handle, xenon::KernelObjectType::Thread);
			if (!obj)
				return X_ERROR_BAD_ARGUMENTS;

			// find thread
			auto* thread = static_cast<xenon::KernelThread*>(obj);

			// resume the thread
			*suspendCountPtr = 0;
			thread->Resume();
			return X_STATUS_SUCCESS;
		}

		uint32_t Xbox_KeResumeThread()
		{
			//DebugBreak();
			return X_STATUS_SUCCESS;
		}

		X_STATUS Xbox_NtSuspendThread()
		{
			//DebugBreak();
			return X_STATUS_SUCCESS;
		}

		uint32_t Xbox_KeSetAffinityThread(Pointer<KernelDispatchHeader> threadPtr, uint32 affinity)
		{
			// resolve
			auto* obj = GPlatform.GetKernel().ResolveObject(threadPtr, xenon::NativeKernelObjectType::ThreadObject);
			if (!obj)
				return 0;

			// find thread
			auto* thread = static_cast<xenon::KernelThread*>(obj);
			const uint32 oldAffinity = thread->SetAffinity(affinity);
			return oldAffinity;
		}

		uint32_t Xbox_KeQueryBasePriorityThread(Pointer<KernelDispatchHeader> threadPtr)
		{
			// resolve
			auto* obj = GPlatform.GetKernel().ResolveObject(threadPtr, xenon::NativeKernelObjectType::ThreadObject);
			if (!obj)
				return 0;

			// return queue priority for the thread
			auto* thread = static_cast<xenon::KernelThread*>(obj);
			return thread->GetQueuePriority();
		}

		uint32_t Xbox_KeSetBasePriorityThread(Pointer<KernelDispatchHeader> threadPtr, uint32 increment)
		{
			// resolve using the dispatch pointer
			auto* obj = GPlatform.GetKernel().ResolveObject(threadPtr, xenon::NativeKernelObjectType::ThreadObject);
			if (!obj)
				return 0;

			// not a thread
			auto* thread = static_cast<xenon::KernelThread*>(obj);
			return thread->SetQueuePriority(increment);
		}

		X_STATUS Xbox_KeDelayExecutionThread(uint32 processorMode, uint32 alertable, Pointer<uint64> internalPtr)
		{
			auto* currentThread = xenon::KernelThread::GetCurrentThread();
			return currentThread->Delay(processorMode, alertable, *internalPtr);
		}

		X_STATUS Xbox_NtYieldExecution()
		{
			// TODO
			auto* currentThread = xenon::KernelThread::GetCurrentThread();
			return X_STATUS_SUCCESS;
		}

		void Xbox_KeQuerySystemTime(Pointer<uint64> dataPtr)
		{
			*dataPtr = 0;
		}

		uint32_t Xbox_KeSetDisableBoostThread(Pointer<KernelDispatchHeader> threadPtr, uint32 disabled)
		{
			// resolve using the dispatch pointer
			auto* obj = GPlatform.GetKernel().ResolveObject(threadPtr, xenon::NativeKernelObjectType::ThreadObject);
			if (!obj)
				return 0;

			// not a thread
			auto* thread = static_cast<xenon::KernelThread*>(obj);
			return 0;
		}

		//---------------

		uint32 Xbox_KeTlsAlloc()
		{
			auto* currentThread = xenon::KernelThread::GetCurrentThread();
			return currentThread->GetOwner()->AllocTLSIndex();
		}

		void Xbox_KeTlsFree(uint32 index)
		{
			auto* currentThread = xenon::KernelThread::GetCurrentThread();
			currentThread->GetOwner()->FreeTLSIndex(index);
		}

		uint64_t Xbox_KeTlsGetValue(uint32 tlsIndex)
		{
			auto* currentThread = xenon::KernelThread::GetCurrentThread();
			return currentThread->GetTLS().Get(tlsIndex);
		}

		uint32_t Xbox_KeTlsSetValue(uint32 tlsIndex, uint64 tlsValue)
		{
			auto* currentThread = xenon::KernelThread::GetCurrentThread();
			currentThread->GetTLS().Set(tlsIndex, tlsValue);
			return 1;
		}

		//-----------------


		X_STATUS Xbox_NtCreateEvent(Pointer<uint32> outPtr, Pointer<uint32> attrPtr, uint32 eventType, uint32 initialState)
		{
			const bool manualReset = !eventType;

			auto* evt = GPlatform.GetKernel().CreateEvent(!!initialState, manualReset);
			if (!evt)
				return X_ERROR_BAD_ARGUMENTS;

			*outPtr = evt->GetHandle();
			return X_STATUS_SUCCESS;
		}

		uint32 Xbox_KeSetEvent(Pointer<KernelDispatchHeader> eventPtr, uint32 increment, uint32 wait)
		{
			auto* nativeEvent = eventPtr.GetNativePointer();

			cpu::mem::storeAtomic<uint32>(&nativeEvent->m_signalState, 1);
			xenon::TagMemoryWrite(&nativeEvent->m_signalState, 4, "KeSetEvent");

			auto* obj = GPlatform.GetKernel().ResolveObject(eventPtr, xenon::NativeKernelObjectType::EventNotificationObject);
			if (!obj)
			{
				DebugBreak();
				return 0;
			}

			auto* evt = static_cast<xenon::KernelEvent*>(obj);
			return evt->Set(increment, !!wait);
		}

		X_STATUS Xbox_NtSetEvent(uint32 eventHandle, Pointer<uint32> statePtr)
		{
			auto* obj = GPlatform.GetKernel().ResolveHandle(eventHandle, xenon::KernelObjectType::Event);
			if (!obj)
				return X_ERROR_BAD_ARGUMENTS;

			auto* evt = static_cast<xenon::KernelEvent*>(obj);
			auto ret = evt->Set(0, false);

			if (statePtr.IsValid())
				*statePtr = ret;

			return X_STATUS_SUCCESS;
		}

		X_STATUS Xbox_KePulseEvent()
		{
			DebugBreak();
			return 0;
		}

		X_STATUS Xbox_NtPulseEvent(uint32 eventHandle, Pointer<uint32> statePtr)
		{
			auto* obj = GPlatform.GetKernel().ResolveHandle(eventHandle, xenon::KernelObjectType::Event);
			if (!obj)
				return X_ERROR_BAD_ARGUMENTS;

			auto* evt = static_cast<xenon::KernelEvent*>(obj);
			const uint32 ret = evt->Set(0, false);

			if (statePtr.IsValid())
				*statePtr = ret;

			return X_STATUS_SUCCESS;
		}

		uint32 Xbox_KeResetEvent(Pointer<KernelDispatchHeader> eventPtr)
		{
			cpu::mem::storeAtomic<uint32>(&eventPtr->m_signalState, 0);
			xenon::TagMemoryWrite(&eventPtr->m_signalState, 0, "KeResetEvent");

			auto* obj = GPlatform.GetKernel().ResolveObject(eventPtr, xenon::NativeKernelObjectType::EventNotificationObject);
			if (!obj)
			{
				DebugBreak();
				return 0;
			}

			auto* evt = static_cast<xenon::KernelEvent*>(obj);
			return evt->Reset();
		}

		X_STATUS Xbox_NtClearEvent(uint32 eventHandle)
		{
			auto* obj = GPlatform.GetKernel().ResolveHandle(eventHandle, xenon::KernelObjectType::Event);
			if (!obj)
				return X_ERROR_BAD_ARGUMENTS;

			auto* evt = static_cast<xenon::KernelEvent*>(obj);
			evt->Reset();
			return X_STATUS_SUCCESS;
		}

		//-----------------

		struct KernelObjectProperties
		{
			Field<uint32_t> m_flags;
			X_ANSI_STRING m_name;
		};

		X_STATUS Xbox_NtCreateSemaphore(Pointer<uint32_t> handlePtr, Pointer<KernelObjectProperties> propertiesPtr, int32_t count, int32_t maxCount)
		{
			// if the object is named make sure we don't create a duplicate
			const char* objectName = nullptr;
			if (propertiesPtr->m_name.Length > 0)
			{
				// get object name to use
				objectName = propertiesPtr->m_name.BufferPtr.AsData().Get().GetNativePointer();

				// object may already exist, check it
				auto existingObject = GPlatform.GetKernel().ResolveNamedObject(objectName, KernelObjectType::Unknown);
				if (existingObject != nullptr)
				{
					// we can reuse the semaphore
					if (existingObject->GetType() == KernelObjectType::Semaphore)
					{
						auto* refCountedObject = (IKernelObjectRefCounted*)existingObject;
						refCountedObject->AddRef();

						DEBUG_CHECK(handlePtr.IsValid());
						*handlePtr = refCountedObject->GetHandle();
						return X_STATUS_SUCCESS;
					}
					// we have an object but it's not a semaphore
					else
					{
						GLog.Warn("Kernel: Found named object '%hs' but its not a semaphore", objectName);
						return X_STATUS_INVALID_HANDLE;
					}
				}
			}

			// create the semaphore semaphore
			auto* semaphore = GPlatform.GetKernel().CreateSemaphore(count, maxCount, objectName);
			if (!semaphore)
				return X_STATUS_INVALID_PARAMETER;

			// bind handle
			if (handlePtr.IsValid())
				*handlePtr = semaphore->GetHandle();

			// semaphore created
			return X_STATUS_SUCCESS;
		}

		// https://msdn.microsoft.com/en-us/library/windows/hardware/ff552150(v=vs.85).aspx
		// https://github.com/benvanik/xenia/blob/0c20f1c0fcac56939a82151f825c854219763eaa/src/xenia/kernel/xboxkrnl/xboxkrnl_threading.cc
		void Xbox_KeInitializeSemaphore(Pointer<KernelSemaphoreHeader> semaphoreHeader, uint32_t count, uint32_t maxCount)
		{
			// setup header
			semaphoreHeader->m_dispatchHeader.m_typeAndFlags = (uint32_t)NativeKernelObjectType::SemaphoreObject;
			semaphoreHeader->m_dispatchHeader.m_signalState = count;
			semaphoreHeader->m_maxCount = maxCount;

			// let the object create itself
			auto object = GPlatform.GetKernel().ResolveObject(&semaphoreHeader->m_dispatchHeader, NativeKernelObjectType::SemaphoreObject, true);
			DEBUG_CHECK(object);
		}

		uint32_t Xbox_KeReleaseSemaphore(Pointer<KernelDispatchHeader> semaphoreHeader, uint32_t increment, uint32_t adjustment, uint32_t waitFlag)
		{
			// resolve semaphore by dispatch structure pointer 
			auto sem = (KernelSemaphore*)GPlatform.GetKernel().ResolveObject(semaphoreHeader, NativeKernelObjectType::SemaphoreObject, false);
			DEBUG_CHECK(sem != nullptr);

			// release the semaphore
			return sem->Release(adjustment);
		}

		X_STATUS Xbox_NtReleaseSemaphore(uint32_t objectHandle, uint32_t count, Pointer<uint32_t> previousCountPtr)
		{
			// resolve semaphore by objects' handle
			auto sem = (KernelSemaphore*)GPlatform.GetKernel().ResolveHandle(objectHandle, KernelObjectType::Semaphore);
			if (!sem)
				return X_STATUS_INVALID_HANDLE;

			// release the semaphore
			auto prevCount = sem->Release(count);

			// save the previous count
			if (previousCountPtr.IsValid())
				*previousCountPtr = prevCount;

			// semaphore was released
			return X_STATUS_SUCCESS;
		}

		//-----------------

		X_STATUS Xbox_NtCreateMutant(Pointer<uint32_t> handlePtr, Pointer<KernelObjectProperties> propertiesPtr, uint32 initiallyOwned)
		{
			// if the object is named make sure we don't create a duplicate
			const char* objectName = nullptr;
			if (propertiesPtr->m_name.Length > 0)
			{
				// get object name to use
				objectName = propertiesPtr->m_name.BufferPtr.AsData().Get().GetNativePointer();

				// object may already exist, check it
				auto existingObject = GPlatform.GetKernel().ResolveNamedObject(objectName, KernelObjectType::Unknown);
				if (existingObject != nullptr)
				{
					// we can reuse the semaphore
					if (existingObject->GetType() == KernelObjectType::Mutant)
					{
						auto* refCountedObject = (IKernelObjectRefCounted*)existingObject;
						refCountedObject->AddRef();

						DEBUG_CHECK(handlePtr.IsValid());
						*handlePtr = refCountedObject->GetHandle();
						return X_STATUS_SUCCESS;
					}
					// we have an object but it's not a semaphore
					else
					{
						GLog.Warn("Kernel: Found named object '%hs' but its not a mutant", objectName);
						return X_STATUS_INVALID_HANDLE;
					}
				}
			}

			// create the semaphore semaphore
			auto* semaphore = GPlatform.GetKernel().CreateMutant(initiallyOwned != 0, objectName);
			if (!semaphore)
				return X_STATUS_INVALID_PARAMETER;

			// bind handle
			if (handlePtr.IsValid())
				*handlePtr = semaphore->GetHandle();

			// semaphore created
			return X_STATUS_SUCCESS;
		}

		X_STATUS Xbox_NtReleaseMutant(uint32 objectHandle)
		{
			// resolve semaphore by objects' handle
			auto sem = (KernelMutant*)GPlatform.GetKernel().ResolveHandle(objectHandle, KernelObjectType::Mutant);
			if (!sem)
				return X_STATUS_INVALID_HANDLE;

			// release the semaphore
			if (!sem->Release())
				return X_STATUS_MUTANT_NOT_OWNED;

			// mutant was released
			return X_STATUS_SUCCESS;
		}

		//-----------------

		X_STATUS Xbox_NtCreateTimer(Pointer<uint32> handlePtr, Pointer<KernelObjectProperties> propertiesPtr, uint32 timerType)
		{
			// if the object is named make sure we don't create a duplicate
			const char* objectName = nullptr;
			if (propertiesPtr->m_name.Length > 0)
			{
				// get object name to use
				objectName = propertiesPtr->m_name.BufferPtr.AsData().Get().GetNativePointer();

				// object may already exist, check it
				auto existingObject = GPlatform.GetKernel().ResolveNamedObject(objectName, KernelObjectType::Unknown);
				if (existingObject != nullptr)
				{
					// we can reuse the timer
					if (existingObject->GetType() == KernelObjectType::Timer)
					{
						auto* refCountedObject = (IKernelObjectRefCounted*)existingObject;
						refCountedObject->AddRef();

						DEBUG_CHECK(handlePtr.IsValid());
						*handlePtr = refCountedObject->GetHandle();
						return X_STATUS_SUCCESS;
					}
					// we have an object but it's not a semaphore
					else
					{
						GLog.Warn("Kernel: Found named object '%hs' but its not a timer", objectName);
						return X_STATUS_INVALID_HANDLE;
					}
				}
			}

			// create a timer
			auto timer = (timerType == 0)
				? GPlatform.GetKernel().CreateManualResetTimer(objectName)
				: GPlatform.GetKernel().CreateSynchronizationTimer(objectName);

			// not created?
			if (!timer)
				return X_STATUS_INVALID_PARAMETER;

			// save handle
			if (handlePtr.IsValid())
				*handlePtr = timer->GetHandle();

			// done
			return X_STATUS_SUCCESS;
		}

		X_STATUS Xbox_NtSetTimerEx(uint32 timerHandle, Pointer<uint64_t> delayPtr, MemoryAddress functionAddress, uint32_t, uint32_t functionArg, uint32 resume, uint32 periodMs)
		{
			auto timer = (KernelTimer*)GPlatform.GetKernel().ResolveHandle(timerHandle, KernelObjectType::Timer);
			if (!timer)
				return X_STATUS_INVALID_HANDLE;

			// set timer
			auto delay = (uint64_t)*delayPtr;
			if (!timer->Setup(delay, periodMs, functionAddress, functionArg))
				return X_STATUS_INVALID_PARAMETER;

			// tiemr set
			return X_STATUS_SUCCESS;
		}

		X_STATUS Xbox_NtCancelTimer(uint32 timerHandle, Pointer<uint32> currentStatePtr)
		{
			// resolve timer by objects' handle
			auto sem = (KernelTimer*)GPlatform.GetKernel().ResolveHandle(timerHandle, KernelObjectType::Timer);
			if (!sem)
				return X_STATUS_INVALID_HANDLE;

			if (!sem->Cancel())
				return X_STATUS_INVALID_PARAMETER;

			if (currentStatePtr.IsValid())
				*currentStatePtr = 0;

			return X_STATUS_SUCCESS;;
		}

		//-----------------

		X_STATUS Xbox_NtSignalAndWaitForSingleObjectEx(uint32 signalObjectHandle, uint32 waitObjectHandle, uint32_t alertable, uint32, Pointer<uint64_t> timeoutPtr)
		{
			auto signalObject = GPlatform.GetKernel().ResolveHandle(signalObjectHandle, KernelObjectType::Unknown);
			if (!signalObject)
				return X_STATUS_INVALID_HANDLE;

			auto waitObject = GPlatform.GetKernel().ResolveHandle(waitObjectHandle, KernelObjectType::Unknown);
			if (!waitObject)
				return X_STATUS_INVALID_HANDLE;

			const auto timeout = timeoutPtr.IsValid() ? (uint64_t)(*timeoutPtr) : 0u;
			return GPlatform.GetKernel().SignalAndWait((IKernelWaitObject*)signalObject, (IKernelWaitObject*)waitObject, alertable != 0, timeoutPtr.IsValid() ? &timeout : nullptr);
		}

		uint32 Xbox_KfAcquireSpinLock(MemoryAddress lockAddr)
		{
			volatile unsigned int* lock = reinterpret_cast<volatile unsigned int*>(lockAddr.GetNativePointer());
			while (!InterlockedCompareExchange(lock, 1, 0) != 0)
			{
			}

			// Raise IRQL to DISPATCH to make sure this thread is executed
			auto* thread = xenon::KernelThread::GetCurrentThread();
			return thread ? thread->RaiseIRQL(2) : 0;
		}

		void Xbox_KfReleaseSpinLock(MemoryAddress lockAddr, uint32 oldIRQL)
		{
			// Restore IRQL
			auto* thread = xenon::KernelThread::GetCurrentThread();
			if (thread)
				thread->LowerIRQL(oldIRQL);

			// Unlock
			volatile unsigned int* lock = reinterpret_cast<volatile unsigned int*>(lockAddr.GetNativePointer());
			InterlockedDecrement(lock);
		}

		void Xbox_KeAcquireSpinLockAtRaisedIrql(MemoryAddress lockAddr)
		{
			volatile unsigned int* lock = reinterpret_cast<volatile unsigned int*>(lockAddr.GetNativePointer());
			while (!InterlockedCompareExchange(lock, 1, 0) != 0)
			{
			}
		}

		void Xbox_KeReleaseSpinLockFromRaisedIrql(MemoryAddress lockAddr)
		{
			volatile unsigned int* lock = reinterpret_cast<volatile unsigned int*>(lockAddr.GetNativePointer());
			InterlockedDecrement(lock);
		}

		//-----------------

		uint32_t Xbox_KeEnterCriticalRegion()
		{
			auto* currentThread = xenon::KernelThread::GetCurrentThread();
			return currentThread->EnterCriticalRegion() ? 1 : 0;
		}

		uint32_t Xbox_KeLeaveCriticalRegion()
		{
			auto* currentThread = xenon::KernelThread::GetCurrentThread();
			return currentThread->LeaveCriticalRegion() ? 1 : 0;
		}

		uint32_t Xbox_KeRaiseIrqlToDpcLevel()
		{
			return GPlatform.GetKernel().RaiseIRQL(IRQL_DPC);
		}

		uint32_t Xbox_KfLowerIrql(uint32_t oldValue)
		{
			return GPlatform.GetKernel().LowerIRQL((KernelIrql)oldValue);
		}

		//-----------------

		uint32_t Xbox_NtQueueApcThread()
		{
			DebugBreak();
			return 0;
		}

		uint32_t Xbox_KeInitializeApc(Pointer<KernelAPCData> apcPtr, MemoryAddress threadPtr, MemoryAddress kernelProc, MemoryAddress rundownProc, MemoryAddress normalProc, uint32 processorMode, uint32 normalContext)
		{
			apcPtr->Reset();
			apcPtr->m_processorMode = processorMode;
			apcPtr->m_threadPtr = threadPtr;
			apcPtr->m_kernelRoutine = kernelProc;
			apcPtr->m_rundownRoutine = rundownProc;
			apcPtr->m_normalRoutine = normalProc;
			apcPtr->m_normalContext = normalProc.IsValid() ? normalContext : 0;
			return 0;
		}

		uint32_t Xbox_KeInsertQueueApc(Pointer<KernelAPCData> apcPtr, uint32 arg1, uint32 arg2, uint32 priorityIncrement)
		{
			// resolve the thread
			auto* thread = (KernelThread*)GPlatform.GetKernel().ResolveObject(apcPtr->m_threadPtr, xenon::NativeKernelObjectType::ThreadObject, false);
			if (!thread)
				return 0;

			// queue the APC in the thread
			if (!thread->EnqueueAPC(apcPtr, arg1, arg2))
				return 0;

			// enqueued
			return 1;
		}

		uint32_t Xbox_KeRemoveQueueApc(Pointer<KernelAPCData> apcPtr)
		{
			// resolve the thread
			auto* thread = (KernelThread*)GPlatform.GetKernel().ResolveObject(apcPtr->m_threadPtr, xenon::NativeKernelObjectType::ThreadObject, false);
			if (!thread)
				return 0;

			// queue the APC in the thread
			if (!thread->DequeueAPC(apcPtr))
				return 0;

			// enqueued
			return 1;
		}

		uint32_t Xbox_KiApcNormalRoutineNop(uint32_t flags)
		{
			return 0;
		}

		//-----------------

		X_STATUS Xbox_KeWaitForSingleObject(Pointer<KernelDispatchHeader> objectPointer, uint32 waitReason, uint32 processorMode, uint32 alertable, Pointer<uint32> timeoutPtr)
		{
			// resolve object
			auto* object = GPlatform.GetKernel().ResolveObject(objectPointer, xenon::NativeKernelObjectType::Unknown, false);
			if (!object)
			{
				GLog.Err("WaitSingle: object %08Xh does not exist", objectPointer);
				return X_STATUS_ABANDONED_WAIT_0;
			}

			// object is not waitable
			if (!object->CanWait())
			{
				GLog.Err("WaitSingle: object %08Xh, '%s' is not waitable", objectPointer, object->GetName());
				return X_STATUS_ABANDONED_WAIT_0;
			}

			const int64 timeout = timeoutPtr.IsValid() ? *timeoutPtr : 0;
			auto* waitObject = static_cast<xenon::IKernelWaitObject*>(object);
			return waitObject->Wait(waitReason, processorMode, !!alertable, timeoutPtr.IsValid() ? &timeout : nullptr);
		}

		X_STATUS Xbox_NtWaitForSingleObjectEx(uint32_t objectHandle, uint32 waitMode, uint32 alertable, Pointer<uint32> timeoutPtr)
		{
			auto* object = GPlatform.GetKernel().ResolveHandle(objectHandle, xenon::KernelObjectType::Unknown);
			if (!object)
			{
				GLog.Err("WaitSingle: object with handle %08Xh does not exist", objectHandle);
				return X_STATUS_ABANDONED_WAIT_0;
			}

			if (!object->CanWait())
			{
				GLog.Err("WaitSingle: object %08Xh, '%s' is not waitable", objectHandle, object->GetName());
				return X_STATUS_ABANDONED_WAIT_0;
			}

			auto* waitObject = static_cast<xenon::IKernelWaitObject*>(object);

			const int64_t timeout = timeoutPtr.IsValid() ? (uint32_t)*timeoutPtr : 0;
			return waitObject->Wait(3, waitMode, !!alertable, timeoutPtr.IsValid() ? &timeout : nullptr);
		}

		X_STATUS Xbox_KeWaitForMultipleObjects(uint32 count, Pointer<Pointer<KernelDispatchHeader>> objectsPtr, uint32 waitType, uint32 waitReason, uint32 alertable, Pointer<uint64> timeoutPtr, Pointer<uint32> waitBlockPtr)
		{
			DEBUG_CHECK(waitType == 0 || waitType == 1);

			const auto timeOut = timeoutPtr.IsValid() ? (uint64_t)*timeoutPtr : 0;

			std::vector<xenon::IKernelWaitObject*> waitObjects;
			for (uint32_t i = 0; i < count; ++i)
			{
				auto objectPtr = objectsPtr[i].Get();
				if (objectPtr.IsValid())
				{
					auto objectData = GPlatform.GetKernel().ResolveObject(objectPtr, xenon::NativeKernelObjectType::Unknown, true);
					if (!objectData || !objectData->CanWait())
						return X_STATUS_INVALID_PARAMETER;

					waitObjects.push_back(static_cast<xenon::IKernelWaitObject*>(objectData));
				}
			}

			const auto waitAll = (waitType == 1);
			return GPlatform.GetKernel().WaitMultiple(waitObjects, waitAll, timeoutPtr.IsValid() ? &timeOut : nullptr, alertable != 0);
		}

		X_STATUS Xbox_NtWaitForMultipleObjectsEx(uint32 count, Pointer<uint32> handlesPtr, uint32 waitType, uint32 waitReason, uint32 alertable, Pointer<uint64> timeoutPtr)
		{
			DEBUG_CHECK(waitType == 0 || waitType == 1);

			std::vector<xenon::IKernelWaitObject*> waitObjects;
			for (uint32_t i = 0; i < count; ++i)
			{
				auto objectHandle = handlesPtr[i];
				auto objectPtr = GPlatform.GetKernel().ResolveHandle(objectHandle, xenon::KernelObjectType::Unknown);
				if (objectPtr && objectPtr->CanWait())
				{
					waitObjects.push_back(static_cast<xenon::IKernelWaitObject*>(objectPtr));
				}
			}

			const auto timeOut = timeoutPtr.IsValid() ? (uint64_t)*timeoutPtr : 0;

			const auto waitAll = (waitType == 1);
			return GPlatform.GetKernel().WaitMultiple(waitObjects, waitAll, timeoutPtr.IsValid() ? &timeOut : nullptr, alertable != 0);
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

		uint64 Xbox_RtlInitializeCriticalSection(Pointer<XCRITICAL_SECTION> rtlCS)
		{
			RtlInitializeCriticalSection(rtlCS.GetNativePointer());
			return 0;
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

		X_STATUS Xbox_RtlInitializeCriticalSectionAndSpinCount(Pointer<XCRITICAL_SECTION> rtlCS)
		{
			XInitCriticalSection(rtlCS.GetNativePointer());
			return X_STATUS_SUCCESS;
		}

		X_STATUS Xbox_RtlDeleteCriticalSection(Pointer<XCRITICAL_SECTION> rtlCS)
		{
			// TODO
			return X_STATUS_SUCCESS;
		}

		X_STATUS Xbox_RtlLeaveCriticalSection(Pointer<XCRITICAL_SECTION> ptr)
		{
			DEBUG_CHECK(ptr.IsValid());

			const uint32 kernelMarker = ptr->Synchronization.RawEvent[0];
			DEBUG_CHECK(kernelMarker == 0xDAAD0FF0);

			const uint32 kernelHandle = ptr->Synchronization.RawEvent[1];

			auto* obj = GPlatform.GetKernel().ResolveHandle(kernelHandle, xenon::KernelObjectType::CriticalSection);
			DEBUG_CHECK(obj);

			auto* cs = static_cast<xenon::KernelCriticalSection*>(obj);
			cs->Leave();

			return X_STATUS_SUCCESS;
		}

		X_STATUS Xbox_RtlEnterCriticalSection(Pointer<XCRITICAL_SECTION> ptr)
		{
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

			return X_STATUS_SUCCESS;
		}

		//---------------------------------------------------------------------------

		uint32 Xbox_KeEnableFpuExceptions(uint32 enabled)
		{
			GLog.Warn("KeEnableFpuExceptions(%d)", enabled);
			return 0;
		}

		uint64_t Xbox_KeQueryPerformanceFrequency()
		{
			uint64_t ret = 0;
			QueryPerformanceFrequency((LARGE_INTEGER*)&ret);
			return ret;
		}

		uint32 Xbox_XexCheckExecutablePrivilege(uint32 mask)
		{
			GLog.Log("Privledge check: 0x%08X", mask);
			return 1;
		}

		uint32_t Xbox___C_specific_handler()
		{
			return 0;
		}

		uint32_t Xbox_HalReturnToFirmware()
		{
			return 0;
		}

		void Xbox_KeBugCheckEx()
		{
			GLog.Err("Runtime: KernelPanic!");
			throw new runtime::TerminateProcessException(0, 0);
		}

		uint32 GCurrentProcessType = X_PROCTYPE_USER;

		uint64 Xbox_KeGetCurrentProcessType()
		{
			return GCurrentProcessType;
		}

		void Xbox_KeSetCurrentProcessType(uint32 type)
		{
			DEBUG_CHECK(type >= 0 && type <= 2);
			GCurrentProcessType = type;
		}

		void Xbox_KeBugCheck()
		{
			GLog.Err("Runtime: KernelPanic!");
			throw new runtime::TerminateProcessException(0, 0);
		}

		X_STATUS Xbox_NtDuplicateObject(uint32 objectHandle, Pointer<uint32> newObjectHandlePtr, uint32 flags)
		{
			uint32 newHandle = 0;

			// create a new handle pointing to the same object
			if (!GPlatform.GetKernel().DuplicateHandle(objectHandle, newHandle))
				return X_STATUS_INVALID_HANDLE;

			// write the new handle
			if (newObjectHandlePtr.IsValid())
				*newObjectHandlePtr = newHandle;

			// close the source handle
			if (flags & 1)
				GPlatform.GetKernel().CloseHandle(objectHandle);

			return X_STATUS_SUCCESS;
		}

		X_STATUS Xbox_ObDereferenceObject(Pointer<KernelDispatchHeader> objectAddress)
		{
			// sometimes an invalid pointer is passed
			if (!objectAddress.IsValid())
				return X_ERROR_BAD_ARGUMENTS;

			// resolve object
			auto* object = GPlatform.GetKernel().ResolveObject(objectAddress, xenon::NativeKernelObjectType::Unknown);
			if (!object)
			{
				GLog.Err("Kernel: Pointer %08Xh does not match any kernel object", objectAddress);
				return X_ERROR_BAD_ARGUMENTS;
			}

			// object cannot be dereferenced
			if (!object->IsRefCounted())
			{
				GLog.Err("Kernel: Called dereference on object '%s' which is not refcounted (pointer %08Xh)", object->GetName(), objectAddress);
				return X_ERROR_BAD_ARGUMENTS;
			}

			// dereference the object
			auto* refCountedObject = static_cast<xenon::IKernelObjectRefCounted*>(object);
			refCountedObject->Release();
			return X_ERROR_SUCCESS;
		}

		X_STATUS Xbox_ObReferenceObjectByHandle(uint32 handle, uint32 objectType, Pointer<uint32> outObjectPtr)
		{
			// resolve handle to a kernel object
			auto* object = GPlatform.GetKernel().ResolveHandle(handle, xenon::KernelObjectType::Unknown);
			if (!object)
			{
				GLog.Warn("Unresolved object handle: %08Xh", handle);
				return X_ERROR_BAD_ARGUMENTS;
			}

			// object is not refcounted
			if (!object->IsRefCounted())
			{
				GLog.Err("Kernel object is not refcounted!");
			}

			// add additional reference
			auto* refCountedObject = static_cast<xenon::IKernelObjectRefCounted*>(object);
			refCountedObject->AddRef();

			// resolve native data
			uint32 nativeAddr = 0;;
			if (objectType == 0xD017BEEF) // ExSemaphoreObjectType
			{
				GLog.Err("Unsupported case");
				nativeAddr = 0xDEADF00D;
			}
			else if (objectType == 0xD01BBEEF) // ExThreadObjectType
			{
				nativeAddr = static_cast<xenon::KernelThread*>(object)->GetMemoryBlock().GetThreadDataAddr();
			}
			else
			{
				if (object->GetType() == xenon::KernelObjectType::Thread)
				{
					// a thread ? if so, get the internal thread data
					nativeAddr = static_cast<xenon::KernelThread*>(object)->GetMemoryBlock().GetThreadDataAddr();
				}
				else
				{
					GLog.Warn("Object '%s', handle %08Xh has undefined object data", object->GetName(), handle);
					nativeAddr = 0xDEADF00D;
				}
			}

			// save the shit
			if (outObjectPtr.IsValid())
				*outObjectPtr = nativeAddr;

			return X_STATUS_SUCCESS;
		}

		X_STATUS Xbox_ObCreateSymbolicLink()
		{
			return X_STATUS_UNSUCCESSFUL;
			//DebugBreak();
			//return X_STATUS_NOT_IMPLEMENTED;
		}

		X_STATUS Xbox_ObDeleteSymbolicLink()
		{
			return X_STATUS_UNSUCCESSFUL;
			//DebugBreak();
			//return X_STATUS_NOT_IMPLEMENTED;
		}

		X_STATUS Xbox_ExRegisterTitleTerminateNotification()
		{
			return X_STATUS_SUCCESS;
		}

		void Xbox_KeInitializeDpc(Pointer<uint32_t> dpcPtr, uint32 address, uint32 context)
		{
			const uint32 type = 19;
			const uint32 importance = 0;
			const uint32 unkown = 0;

			dpcPtr[0] = (type << 24) | (importance << 16) | (unkown);
			dpcPtr[1] = 0;  // flink
			dpcPtr[2] = 0;  // blink
			dpcPtr[3] = address;
			dpcPtr[4] = context;
			dpcPtr[5] = 0;  // arg1
			dpcPtr[6] = 0;  // arg2
		}

		uint32 Xbox_KeInsertQueueDpc(Pointer<uint32_t> dpcPtr, uint32 arg1, uint32 arg2)
		{
			Pointer<KernelListEntry> listEntryPtr = MemoryAddress(dpcPtr[1]);

			// lock access
			auto& dispatcher = GPlatform.GetKernel().GetDPCDispatcher();
			dispatcher.Lock();

			// remove from list
			auto* dpcList = dispatcher.GetList();
			if (dpcList->IsQueued(listEntryPtr))
			{
				dispatcher.Unlock();
				return 0;
			}

			// store data
			dpcPtr[5] = arg1;
			dpcPtr[6] = arg2;

			// insert back to list
			dpcList->Insert(listEntryPtr);

			// unlock
			dispatcher.Unlock();
			return 1;
		}

		uint32 Xbox_KeRemoveQueueDpc(Pointer<uint32_t> dpcPtr)
		{
			Pointer<KernelListEntry> listEntryPtr = MemoryAddress(dpcPtr[1]);

			// lock access
			auto& dispatcher = GPlatform.GetKernel().GetDPCDispatcher();
			dispatcher.Lock();

			// remove from list
			auto dpcList = dispatcher.GetList();
			const auto queued = dpcList->IsQueued(listEntryPtr);
			if (queued)
				dpcList->Remove(listEntryPtr);

			// unlock access
			dispatcher.Unlock();

			return queued ? 1 : 0;
		}

		static std::mutex GGlobalListMutex;

		// http://www.nirsoft.net/kernel_struct/vista/SLIST_HEADER.html
		uint32 Xbox_InterlockedPopEntrySList(Pointer<uint32> plistPtr)
		{
			std::lock_guard<std::mutex> lock(GGlobalListMutex);

			uint32 first = *plistPtr;
			if (first == 0)
				return 0;

			auto second = *Pointer<uint32>(first);
			*plistPtr = second;

			return first;
		}

		void Xbox_Interrupt20(const uint64_t ip, const uint32_t index, const cpu::CpuRegs& regs)
		{
			const char* msg = (const char*)regs.R3;
			GPlatform.DebugTrace(msg);
		}

		void Xbox_Interrupt22(const uint64_t ip, const uint32_t index, const cpu::CpuRegs& regs)
		{

		}

		void Xbox_XamLoaderGetLaunchDataSize()
		{

		}

		void Xbox_XamContentDelete()
		{

		}

		void Xbox_XamContentFlush()
		{

		}

		void Xbox_XamContentGetLicenseMask()
		{

		}

		void Xbox_XamContentGetDeviceData()
		{

		}

		void Xbox_XamContentLaunchImage()
		{

		}

		void Xbox_XMsgStartIORequest()
		{

		}

		void Xbox_XCustomUnregisterDynamicActions()
		{

		}

		void Xbox_XCustomRegisterDynamicActions()
		{

		}

		void Xbox_XamLoaderSetLaunchData()
		{

		}

		void Xbox_XMsgStartIORequestEx()
		{

		}

		void Xbox_XamVoiceCreate()
		{

		}

		void Xbox_XamVoiceSubmitPacket()
		{

		}

		void Xbox_XMsgCancelIORequest()
		{

		}

		void Xbox_XamVoiceClose()
		{

		}

		void Xbox_XamVoiceHeadsetPresent()
		{

		}

		void Xbox_XamUserReadProfileSettings()
		{

		}

		void Xbox_XamCreateEnumeratorHandle()
		{

		}

		void Xbox_XamGetPrivateEnumStructureFromHandle()
		{

		}

		void Xbox_XamSessionCreateHandle()
		{

		}

		void Xbox_XamSessionRefObjByHandle()
		{

		}

		void Xbox_XNetLogonGetTitleID()
		{

		}

		void Xbox_XNetLogonGetMachineID()
		{

		}

		void Xbox_XamAlloc()
		{

		}

		void Xbox_XamFree()
		{

		}

		void Xbox_XamUserWriteProfileSettings()
		{

		}

		void Xbox_XamGetExecutionId()
		{

		}

		void Xbox_XamUserGetSigninInfo()
		{

		}

		void Xbox_XamUserCreateAchievementEnumerator()
		{

		}

		void Xbox_XCustomGetCurrentGamercard()
		{

		}

		void Xbox_XCustomGetLastActionPressEx()
		{

		}

		void Xbox_XNotifyDelayUI()
		{

		}

		void Xbox_XNotifyPositionUI()
		{

		}

		void Xbox_XCustomSetDynamicActions()
		{

		}

		void Xbox_XGetGameRegion()
		{

		}

		void Xbox_XMsgInProcessCall()
		{

		}

		void Xbox_XamShowCustomMessageComposeUI()
		{

		}

		void Xbox_XamShowMarketplaceDownloadItemsUI()
		{

		}

		void Xbox_XamShowFriendsUI()
		{

		}

		void Xbox_XamShowGamerCardUIForXUID()
		{

		}

		void Xbox_XamShowPlayerReviewUI()
		{

		}

		void Xbox_XamShowMarketplaceUI()
		{

		}

		void Xbox_XamShowFriendRequestUI()
		{

		}

		void Xbox_XamShowDirtyDiscErrorUI()
		{

		}

		void Xbox_XamUserGetName()
		{

		}

		void Xbox_XamUserAreUsersFriends()
		{

		}

		void Xbox_XamUserCheckPrivilege()
		{

		}

		void Xbox_XamGetSystemVersion()
		{

		}

		void Xbox_InterlockedFlushSList()
		{

		}

		void Xbox_XeCryptBnQw_SwapDwQwLeBe()
		{

		}

		void Xbox_XeCryptBnQwNeRsaPubCrypt()
		{

		}

		void Xbox_XeCryptBnDwLePkcs1Verify()
		{

		}

		void Xbox_XeCryptRandom()
		{

		}

		void Xbox_XeCryptAesKey()
		{

		}

		void Xbox_XeCryptAesCbc()
		{

		}

		void Xbox_XeCryptBnDwLePkcs1Format()
		{

		}

		void Xbox_RtlTryEnterCriticalSection()
		{

		}

		void Xbox_RtlCaptureContext()
		{

		}

		void Xbox_XeCryptShaInit()
		{

		}

		void Xbox_XeCryptShaUpdate()
		{

		}

		void Xbox_XeCryptShaFinal()
		{

		}

		void Xbox_MmQueryStatistics()
		{

		}

		void Xbox_XexGetModuleHandle()
		{

		}

		void Xbox_XexGetModuleSection()
		{

		}

		void Xbox_NtCreateIoCompletion()
		{

		}

		void Xbox_NtRemoveIoCompletion()
		{

		}

		void Xbox_MmCreateKernelStack()
		{

		}

		void Xbox_MmDeleteKernelStack()
		{

		}

		void Xbox_KeSetCurrentStackPointers()
		{

		}

		void Xbox_ObOpenObjectByPointer()
		{

		}

		void Xbox_ExAllocatePool()
		{

		}

		void Xbox_NtDeviceIoControlFile()
		{

		}

		void Xbox_IoDismountVolumeByFileHandle()
		{

		}

		void Xbox_IoRemoveShareAccess()
		{

		}

		void Xbox_IoSetShareAccess()
		{

		}

		void Xbox_IoCheckShareAccess()
		{

		}

		void Xbox_ObIsTitleObject()
		{

		}

		void Xbox_RtlUpcaseUnicodeChar()
		{

		}

		void Xbox_IoCompleteRequest()
		{

		}

		void Xbox_RtlTimeToTimeFields()
		{

		}

		void Xbox_RtlTimeFieldsToTime()
		{

		}

		void Xbox_RtlCompareStringN()
		{

		}

		void Xbox_ExFreePool()
		{

		}

		void Xbox_ExAllocatePoolTypeWithTag()
		{

		}

		void Xbox_IoDeleteDevice()
		{

		}

		void Xbox_IoCreateDevice()
		{

		}

		void Xbox_ObReferenceObject()
		{

		}

		void Xbox_IoInvalidDeviceRequest()
		{

		}

		void Xbox_FscSetCacheElementCount()
		{

		}

		void Xbox_IoDismountVolume()
		{

		}

		void Xbox_XeKeysConsolePrivateKeySign()
		{

		}

		void Xbox_XeCryptSha()
		{

		}

		void Xbox_XeKeysConsoleSignatureVerification()
		{

		}

		void Xbox_StfsCreateDevice()
		{

		}

		void Xbox_StfsControlDevice()
		{

		}

		void Xbox_KeTryToAcquireSpinLockAtRaisedIrql()
		{

		}

		void Xbox_XAudioGetVoiceCategoryVolumeChangeMask()
		{

		}

		void Xbox_XMAReleaseContext()
		{

		}

		void Xbox_XMACreateContext()
		{

		}

		void Xbox_ObLookupAnyThreadByThreadId()
		{

		}

		void Xbox_RtlImageNtHeader()
		{

		}

			void Xbox_MmIsAddressValid()
		{

		}


		void RegisterXboxKernel(runtime::Symbols& symbols)
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

			REGISTER(KeBugCheckEx);
			REGISTER(KeBugCheck);

			REGISTER(KeQueryPerformanceFrequency);

			REGISTER(NtDuplicateObject);
			REGISTER(ObDereferenceObject);
			REGISTER(ObReferenceObjectByHandle);
			REGISTER(ObDeleteSymbolicLink);
			REGISTER(ObCreateSymbolicLink);

			REGISTER(XexCheckExecutablePrivilege);
			REGISTER(__C_specific_handler);
			REGISTER(HalReturnToFirmware);
			REGISTER(KeGetCurrentProcessType);
			REGISTER(ExRegisterTitleTerminateNotification);
			REGISTER(KeEnableFpuExceptions);

			REGISTER(KeInitializeDpc);
			REGISTER(KeInsertQueueDpc);
			REGISTER(KeRemoveQueueDpc);
			REGISTER(InterlockedPopEntrySList);

			REGISTER(KeQueryBasePriorityThread);
			REGISTER(KeSetBasePriorityThread);
			REGISTER(KeSetDisableBoostThread);
			REGISTER(KeSetCurrentProcessType);

			REGISTER(KeRaiseIrqlToDpcLevel);
			REGISTER(KfLowerIrql);






			// testing kernel functions by Squaresome
			REGISTER(XamLoaderGetLaunchDataSize);
			REGISTER(XamContentDelete);
			REGISTER(XamContentFlush);
			REGISTER(XamContentGetLicenseMask);
			REGISTER(XamContentGetDeviceData);
			REGISTER(XamContentLaunchImage);
			REGISTER(XMsgStartIORequest);
			REGISTER(XCustomUnregisterDynamicActions);
			REGISTER(XCustomRegisterDynamicActions);
			REGISTER(XamLoaderSetLaunchData);
			REGISTER(XMsgStartIORequestEx);
			REGISTER(XamVoiceCreate);
			REGISTER(XamVoiceSubmitPacket);
			REGISTER(XMsgCancelIORequest);
			REGISTER(XamVoiceClose);
			REGISTER(XamVoiceHeadsetPresent);
			REGISTER(XamUserReadProfileSettings);
			REGISTER(XamCreateEnumeratorHandle);
			REGISTER(XamGetPrivateEnumStructureFromHandle);
			REGISTER(XamSessionCreateHandle);
			REGISTER(XamSessionRefObjByHandle);
			REGISTER(XNetLogonGetTitleID);
			REGISTER(XNetLogonGetMachineID);
			REGISTER(XamAlloc);
			REGISTER(XamFree);
			REGISTER(XamUserWriteProfileSettings);
			REGISTER(XamGetExecutionId);
			REGISTER(XamUserGetSigninInfo);
			REGISTER(XamUserCreateAchievementEnumerator);
			REGISTER(XCustomGetCurrentGamercard);
			REGISTER(XCustomGetLastActionPressEx);
			REGISTER(XNotifyDelayUI);
			REGISTER(XNotifyPositionUI);
			REGISTER(XCustomSetDynamicActions);
			REGISTER(XGetGameRegion);
			REGISTER(XMsgInProcessCall);
			REGISTER(XamShowCustomMessageComposeUI);
			REGISTER(XamShowMarketplaceDownloadItemsUI);
			REGISTER(XamShowFriendsUI);
			REGISTER(XamShowGamerCardUIForXUID);
			REGISTER(XamShowPlayerReviewUI);
			REGISTER(XamShowMarketplaceUI);
			REGISTER(XamShowFriendRequestUI);
			REGISTER(XamShowDirtyDiscErrorUI);
			REGISTER(XamUserGetName);
			REGISTER(XamUserAreUsersFriends);
			REGISTER(XamUserCheckPrivilege);
			REGISTER(XamGetSystemVersion);
			REGISTER(InterlockedFlushSList);
			REGISTER(XeCryptBnQw_SwapDwQwLeBe);
			REGISTER(XeCryptBnQwNeRsaPubCrypt);
			REGISTER(XeCryptBnDwLePkcs1Verify);
			REGISTER(XeCryptRandom);
			REGISTER(XeCryptAesKey);
			REGISTER(XeCryptAesCbc);
			REGISTER(XeCryptBnDwLePkcs1Format);
			REGISTER(RtlTryEnterCriticalSection);
			REGISTER(RtlCaptureContext);
			REGISTER(XeCryptShaInit);
			REGISTER(XeCryptShaUpdate);
			REGISTER(XeCryptShaFinal);
			REGISTER(MmQueryStatistics);
			REGISTER(XexGetModuleHandle);
			REGISTER(XexGetModuleSection);
			REGISTER(NtCreateIoCompletion);
			REGISTER(NtRemoveIoCompletion);
			REGISTER(MmCreateKernelStack);
			REGISTER(MmDeleteKernelStack);
			REGISTER(KeSetCurrentStackPointers);
			REGISTER(ObOpenObjectByPointer);
			REGISTER(ExAllocatePool);
			REGISTER(NtDeviceIoControlFile);
			REGISTER(IoDismountVolumeByFileHandle);
			REGISTER(IoRemoveShareAccess);
			REGISTER(IoSetShareAccess);
			REGISTER(IoCheckShareAccess);
			REGISTER(ObIsTitleObject);
			REGISTER(RtlUpcaseUnicodeChar);
			REGISTER(IoCompleteRequest);
			REGISTER(RtlTimeToTimeFields);
			REGISTER(RtlTimeFieldsToTime);
			REGISTER(RtlCompareStringN);
			REGISTER(ExFreePool);
			REGISTER(ExAllocatePoolTypeWithTag);
			REGISTER(IoDeleteDevice);
			REGISTER(IoCreateDevice);
			REGISTER(ObReferenceObject);
			REGISTER(IoInvalidDeviceRequest);
			REGISTER(FscSetCacheElementCount);
			REGISTER(IoDismountVolume);
			REGISTER(XeKeysConsolePrivateKeySign);
			REGISTER(XeCryptSha);
			REGISTER(XeKeysConsoleSignatureVerification);
			REGISTER(StfsCreateDevice);
			REGISTER(StfsControlDevice);
			REGISTER(KeTryToAcquireSpinLockAtRaisedIrql);
			REGISTER(XAudioGetVoiceCategoryVolumeChangeMask);
			REGISTER(XMAReleaseContext);
			REGISTER(XMACreateContext);
			REGISTER(ObLookupAnyThreadByThreadId);
			REGISTER(RtlImageNtHeader);
			REGISTER(MmIsAddressValid);






			symbols.RegisterInterrupt(20, (runtime::TInterruptFunc) &Xbox_Interrupt20);
			symbols.RegisterInterrupt(22, (runtime::TInterruptFunc) &Xbox_Interrupt22);
		}

	} // lib
} // xenon