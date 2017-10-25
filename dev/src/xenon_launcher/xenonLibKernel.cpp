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

		X_STATUS Xbox_KeResumeThread()
		{
			DebugBreak();
			return X_STATUS_NOT_IMPLEMENTED;
		}

		X_STATUS Xbox_NtSuspendThread()
		{
			DebugBreak();
			return X_STATUS_NOT_IMPLEMENTED;
		}

		X_STATUS Xbox_KeSetAffinityThread(uint32 threadPtr, uint32 affinity)
		{
			// resolve
			auto* obj = GPlatform.GetKernel().ResolveObject(threadPtr, xenon::NativeKernelObjectType::ThreadObject);
			if (!obj)
				return X_ERROR_BAD_ARGUMENTS;

			// find thread
			auto* thread = static_cast<xenon::KernelThread*>(obj);
			const uint32 oldAffinity = thread->SetAffinity(affinity);
			return oldAffinity;
		}

		X_STATUS Xbox_KeQueryBasePriorityThread()
		{
			DebugBreak();
			return X_STATUS_NOT_IMPLEMENTED;
		}

		X_STATUS Xbox_KeSetBasePriorityThread(uint32 threadPtr, uint32 increment)
		{
			// resolve
			auto* obj = GPlatform.GetKernel().ResolveObject(threadPtr, xenon::NativeKernelObjectType::ThreadObject);
			if (!obj)
				return X_ERROR_BAD_ARGUMENTS;

			// not a thread
			auto* thread = static_cast<xenon::KernelThread*>(obj);
			int prevPriority = thread->GetPriority();
			thread->SetPriority(increment);
			return prevPriority;
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

		uint32 Xbox_KeSetEvent(Pointer<XEVENT> eventPtr, uint32 increment, uint32 wait)
		{
			auto* nativeEvent = eventPtr.GetNativePointer();

			cpu::mem::storeAtomic<uint32>(&nativeEvent->Dispatch.SignalState, 1);
			xenon::TagMemoryWrite(&nativeEvent->Dispatch.SignalState, 4, "KeSetEvent");

			auto* obj = GPlatform.GetKernel().ResolveObject(eventPtr.GetAddress().GetAddressValue(), xenon::NativeKernelObjectType::EventNotificationObject);
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

		uint32 Xbox_KeResetEvent(Pointer<XEVENT> eventPtr)
		{
			auto* nativeEvent = eventPtr.GetNativePointer();
			cpu::mem::storeAtomic<uint32>(&nativeEvent->Dispatch.SignalState, 0);
			xenon::TagMemoryWrite(&nativeEvent->Dispatch.SignalState, 0, "KeResetEvent");

			auto* obj = GPlatform.GetKernel().ResolveObject((uint32_t)nativeEvent, xenon::NativeKernelObjectType::EventNotificationObject);
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

		X_STATUS Xbox_NtCreateSemaphore()
		{
			DebugBreak();
			return X_STATUS_NOT_IMPLEMENTED;
		}

		X_STATUS Xbox_KeInitializeSemaphore()
		{
			DebugBreak();
			return X_STATUS_NOT_IMPLEMENTED;
		}

		X_STATUS Xbox_KeReleaseSemaphore()
		{
			DebugBreak();
			return X_STATUS_NOT_IMPLEMENTED;
		}

		X_STATUS Xbox_NtReleaseSemaphore()
		{
			DebugBreak();
			return X_STATUS_NOT_IMPLEMENTED;
		}

		//-----------------

		X_STATUS Xbox_NtCreateMutant()
		{
			DebugBreak();
			return X_STATUS_NOT_IMPLEMENTED;
		}

		X_STATUS Xbox_NtReleaseMutant()
		{
			DebugBreak();
			return X_STATUS_NOT_IMPLEMENTED;
		}

		//-----------------

		X_STATUS Xbox_NtCreateTimer()
		{
			DebugBreak();
			return X_STATUS_NOT_IMPLEMENTED;
		}

		X_STATUS Xbox_NtSetTimerEx()
		{
			DebugBreak();
			return X_STATUS_NOT_IMPLEMENTED;
		}

		X_STATUS Xbox_NtCancelTimer()
		{
			DebugBreak();
			return X_STATUS_NOT_IMPLEMENTED;
		}

		//-----------------

		X_STATUS Xbox_NtSignalAndWaitForSingleObjectEx()
		{
			DebugBreak();
			return X_STATUS_NOT_IMPLEMENTED;
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

		X_STATUS Xbox_KeEnterCriticalRegion()
		{
			auto* currentThread = xenon::KernelThread::GetCurrentThread();
			return currentThread->EnterCriticalRegion();
		}

		X_STATUS Xbox_KeLeaveCriticalRegion()
		{
			auto* currentThread = xenon::KernelThread::GetCurrentThread();
			return currentThread->LeaveCriticalRegion();
		}

		//-----------------

		X_STATUS Xbox_NtQueueApcThread()
		{
			DebugBreak();
			return X_STATUS_NOT_IMPLEMENTED;
		}

		X_STATUS Xbox_KeInitializeApc()
		{
			DebugBreak();
			return X_STATUS_NOT_IMPLEMENTED;
		}

		X_STATUS Xbox_KeInsertQueueApc()
		{
			DebugBreak();
			return X_STATUS_NOT_IMPLEMENTED;
		}

		X_STATUS Xbox_KeRemoveQueueApc()
		{
			DebugBreak();
			return X_STATUS_NOT_IMPLEMENTED;
		}

		X_STATUS Xbox_KiApcNormalRoutineNop()
		{
			return X_STATUS_SUCCESS;
		}

		//-----------------

		X_STATUS Xbox_KeWaitForSingleObject(uint32 objectPointer, uint32 waitReason, uint32 processorMode, uint32 alertable, Pointer<uint32> timeoutPtr)
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

		X_STATUS Xbox_NtWaitForSingleObjectEx(uint32 objectHandle, uint32 waitMode, uint32 alertable, Pointer<uint32> timeoutPtr)
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

		X_STATUS Xbox_KeWaitForMultipleObjects(uint32 count, Pointer<uint32> objectsPtr, uint32 waitType, uint32 waitReason, uint32 alertable, Pointer<uint32> timeoutPtr, Pointer<uint32> waitBlockPtr)
		{
			DEBUG_CHECK(waitType == 0 || waitType == 1);

			const auto timeOut = timeoutPtr.IsValid() ? (uint32_t)*timeoutPtr : 0;

			std::vector<xenon::IKernelWaitObject*> waitObjects;
			for (uint32_t i = 0; i < count; ++i)
			{
				auto objectPtr = objectsPtr[i];
				if (objectPtr)
				{
					auto objectData = GPlatform.GetKernel().ResolveObject(objectPtr, xenon::NativeKernelObjectType::Unknown, true);
					if (!objectData || !objectData->CanWait())
						return X_STATUS_INVALID_PARAMETER;

					waitObjects.push_back(static_cast<xenon::IKernelWaitObject*>(objectData));
				}
			}

			const auto waitAll = (waitType == 1);
			return GPlatform.GetKernel().WaitMultiple(waitObjects, waitAll, timeOut, alertable != 0);
		}

		X_STATUS Xbox_NtWaitForMultipleObjectsEx()
		{
			DebugBreak();
			return X_STATUS_NOT_IMPLEMENTED;
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

		X_STATUS Xbox_NtDuplicateObject()
		{
			DebugBreak();
			return X_STATUS_NOT_IMPLEMENTED;
		}

		X_STATUS Xbox_ObDereferenceObject(uint32 objectAddress)
		{
			if (!objectAddress)
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
				nativeAddr = static_cast< xenon::KernelThread* >(object)->GetMemoryBlock().GetThreadDataAddr();
			}
			else
			{
				if (object->GetType() == xenon::KernelObjectType::Thread)
				{
					// a thread ? if so, get the internal thread data
					nativeAddr = static_cast< xenon::KernelThread* >(object)->GetMemoryBlock().GetThreadDataAddr();
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
			DebugBreak();
			return X_STATUS_NOT_IMPLEMENTED;
		}

		X_STATUS Xbox_ObDeleteSymbolicLink()
		{
			DebugBreak();
			return X_STATUS_NOT_IMPLEMENTED;
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

			REGISTER(KeSetCurrentProcessType);

			symbols.RegisterInterrupt(20, (runtime::TInterruptFunc) &Xbox_Interrupt20);
			symbols.RegisterInterrupt(22, (runtime::TInterruptFunc) &Xbox_Interrupt22);
		}

	} // lib
} // xenon