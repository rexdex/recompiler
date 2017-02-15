#include "build.h"
#include "xenonLibNatives.h" 
#include "xenonLibUtils.h" 
#include "xenonThread.h"
#include "xenonKernel.h"
#include "xenonPlatform.h"

//---------------------------------------------------------------------------

uint64 __fastcall XboxThreads_NtClose( uint64 ip, cpu::CpuRegs& regs )
{
	const uint32 kernelHandle = (const uint32) regs.R3;
	
	// resolve
	auto* obj = GPlatform.GetKernel().ResolveAny( kernelHandle );
	if ( !obj )
		RETURN_ARG(0);

	// closing handle
	if ( !obj->IsRefCounted() )
	{
		GLog.Err( "Kernel: Trying to close '%s', handle=%08Xh which is not refcoutned", obj->GetName(), obj->GetHandle() );
		RETURN_ARG(0);
	}

	// close the object
	GLog.Err( "Kernel: Releasing '%s', handle=%08Xh", obj->GetName(), obj->GetHandle() );

	auto* refCountedObj = static_cast<xenon::IKernelObjectRefCounted*>(obj);
	refCountedObj->Release();

	RETURN_ARG(0);
}

//---------------------------------------------------------------------------

uint64 __fastcall XboxThreads_ExCreateThread( uint64 ip, cpu::CpuRegs& regs )
{
	const uint32 handle_ptr = (const uint32)regs.R3;
	const uint32 stack_size = (const uint32)regs.R4;
	const uint32 thread_id_ptr = (const uint32)regs.R5;
	const uint32 xapi_thread_startup = (const uint32)regs.R6;
	const uint32 start_address = (const uint32)regs.R7;
	const uint32 start_context = (const uint32)regs.R8;
	const uint32 creation_flags = (const uint32)regs.R9;
	GLog.Log( "ExCreateThread (handle=%08X, stack=%d, idPtr=%08X, xapi==%08Xh, start=%08Xh, context=%d, flags=%d)", 
		handle_ptr, stack_size, thread_id_ptr, xapi_thread_startup, start_address, start_context, creation_flags );

	// Setup thread params
	static uint32 ThreadCounter = 0;
	xenon::KernelThreadParams params;
	sprintf_s( params.m_name, "XThread%d", ++ThreadCounter );
	params.m_stackSize = stack_size;
	params.m_suspended = (creation_flags & 0x1) != 0;

	// XAPI stuff
	if ( xapi_thread_startup != 0 )
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
	if ( !thread)
	{
		GLog.Err( "ExCreateThread: failed to create thread in process" );
		RETURN_ARG( xnative::X_ERROR_ACCESS_DENIED );
	}

	// thread created, return thread handle
	*(uint32*)handle_ptr = _byteswap_ulong( thread->GetHandle() );

	// save thread id
	if ( thread_id_ptr )
		*(uint32*)thread_id_ptr = _byteswap_ulong( thread->GetIndex() );

	// no errors
	RETURN_ARG( 0 );
}

uint64 __fastcall XboxThreads_ExTerminateThread( uint64 ip, cpu::CpuRegs& regs )
{
	// get exit code
	uint32 exitCode = (uint32) regs.R3;
	GLog.Log( "ExTerminateThread(%d) for TID %d", exitCode, xenon::KernelThread::GetCurrentThreadID() );

	// exit the thread directly
	ULONG_PTR args[1] = { exitCode }; // exit code
	RaiseException(cpu::EXCEPTION_TERMINATE_THREAD, EXCEPTION_NONCONTINUABLE, 1, &args[0] );

	// should not return
	RETURN_ARG( 0 );
}

uint64 __fastcall XboxThreads_NtResumeThread( uint64 ip, cpu::CpuRegs& regs )
{
	const uint32 handle = (const uint32) regs.R3;
	const uint32 suspend_count_ptr = (const uint32) regs.R4;
	GLog.Log("NtResumeThread(%08Xh, %08Xh)", handle, suspend_count_ptr);

	// resolve
	auto* obj = GPlatform.GetKernel().ResolveHandle(handle, xenon::KernelObjectType::Thread);
	if (!obj)
		RETURN_ARG(xnative::X_ERROR_BAD_ARGUMENTS);

	// find thread
	auto* thread = static_cast<xenon::KernelThread*>(obj);

	// resume the thread
	uint32 suspend_count = 0;
	thread->Resume();
	RETURN_ARG( 0 );
}

uint64 __fastcall XboxThreads_KeResumeThread( uint64 ip, cpu::CpuRegs& regs )
{
	DebugBreak();
	RETURN_DEFAULT();
}

uint64 __fastcall XboxThreads_NtSuspendThread( uint64 ip, cpu::CpuRegs& regs )
{
	DebugBreak();
	RETURN_DEFAULT();
}

uint64 __fastcall XboxThreads_KeSetAffinityThread( uint64 ip, cpu::CpuRegs& regs )
{
	const uint32 threadPtr = (const uint32) regs.R3;
	const uint32 affinity = (const uint32) regs.R4;

	// resolve
	auto* obj = GPlatform.GetKernel().ResolveObject( (void*)threadPtr, xenon::NativeKernelObjectType::ThreadObject);
	if (!obj)
		RETURN_ARG(xnative::X_ERROR_BAD_ARGUMENTS);

	// find thread
	auto* thread = static_cast<xenon::KernelThread*>(obj);
	const uint32 oldAffinity = thread->SetAffinity( affinity );
	RETURN_ARG( oldAffinity );
}

uint64 __fastcall XboxThreads_KeQueryBasePriorityThread( uint64 ip, cpu::CpuRegs& regs )
{
	DebugBreak();
	RETURN_DEFAULT();
}

uint64 __fastcall XboxThreads_KeSetBasePriorityThread( uint64 ip, cpu::CpuRegs& regs )
{
	const uint32 threadPtr = (const uint32) regs.R3;
	const uint32 increment = (const uint32) regs.R4;

	// resolve
	auto* obj = GPlatform.GetKernel().ResolveObject((void*)threadPtr, xenon::NativeKernelObjectType::ThreadObject);
	if (!obj)
		RETURN_ARG(xnative::X_ERROR_BAD_ARGUMENTS);

	// not a thread
	auto* thread = static_cast<xenon::KernelThread*>(obj);
	int prevPriority = thread->GetPriority();
	thread->SetPriority( increment );

	RETURN_ARG(prevPriority);
}

uint64 __fastcall XboxThreads_KeDelayExecutionThread( uint64 ip, cpu::CpuRegs& regs )
{
	const uint32 processor_mode = (const uint32)regs.R3;
	const uint32 alertable = (const uint32)regs.R4;
	const uint32 internalPtr = (const uint32)regs.R5;
	const uint64 interval = mem::loadAddr<uint64>( internalPtr );

	auto* currentThread = xenon::KernelThread::GetCurrentThread();
	const uint32 result = currentThread->Delay(processor_mode, alertable, interval);

	RETURN_ARG(result);
}

uint64 __fastcall XboxThreads_NtYieldExecution( uint64 ip, cpu::CpuRegs& regs )
{
	auto* currentThread = xenon::KernelThread::GetCurrentThread();
	const uint32 result = 0;//currentThread->Delay(processor_mode, alertable, interval.Get());

	RETURN_ARG(result);
}

uint64 __fastcall XboxThreads_KeQuerySystemTime( uint64 ip, cpu::CpuRegs& regs )
{
	DebugBreak();
	RETURN_DEFAULT();
}

uint64 __fastcall XboxThreads_KeTlsAlloc( uint64 ip, cpu::CpuRegs& regs )
{
	auto* currentThread = xenon::KernelThread::GetCurrentThread();
	const uint32 index = currentThread->GetOwner()->AllocTLSIndex();
	RETURN_ARG(index);
}

uint64 __fastcall XboxThreads_KeTlsFree( uint64 ip, cpu::CpuRegs& regs )
{
	const uint32 index = (uint32)regs.R3;

	auto* currentThread = xenon::KernelThread::GetCurrentThread();
	currentThread->GetOwner()->FreeTLSIndex(index);

	RETURN();
}

uint64 __fastcall XboxThreads_KeTlsGetValue( uint64 ip, cpu::CpuRegs& regs )
{
	const uint32 tlsIndex = (uint32)regs.R3;

	auto* currentThread = xenon::KernelThread::GetCurrentThread();
	const uint64 value = currentThread->GetTLS().Get(tlsIndex);

	RETURN_ARG( value );
}

uint64 __fastcall XboxThreads_KeTlsSetValue( uint64 ip, cpu::CpuRegs& regs )
{
	const uint32 tlsIndex = (uint32)regs.R3;
	const uint64 tlsValue = regs.R4;

	auto* currentThread = xenon::KernelThread::GetCurrentThread();
	currentThread->GetTLS().Set( tlsIndex, tlsValue );

	RETURN_ARG(1)
}

//-----------------

uint64 __fastcall XboxThreads_NtCreateEvent( uint64 ip, cpu::CpuRegs& regs )
{
	const uint32 outPtr = (const uint32)regs.R3;
	const uint32 attrPtr = (const uint32)regs.R4;
	const uint32 event_type = (const uint32)regs.R5;
	const uint32 initial_state = (const uint32)regs.R6;

	const bool manualReset = !event_type;
	const bool initialState = (0 != initial_state);

	auto* evt = GPlatform.GetKernel().CreateEvent(initialState, manualReset);
	if (!evt)
		RETURN_ARG(xnative::X_ERROR_BAD_ARGUMENTS);

	mem::storeAddr( outPtr, evt->GetHandle() );
	RETURN_ARG(0);
}

uint64 __fastcall XboxThreads_KeSetEvent( uint64 ip, cpu::CpuRegs& regs )
{
	const uint32 eventPtr = (const uint32) regs.R3;
	const uint32 increment = (const uint32) regs.R4;
	const uint32 wait = (const uint32) regs.R5;

	auto* nativeEvent = (xnative::XEVENT*) eventPtr;
	mem::storeAtomic<uint32>( &nativeEvent->Dispatch.SignalState, 1 );
	
	auto* obj = GPlatform.GetKernel().ResolveObject((void*)eventPtr, xenon::NativeKernelObjectType::EventNotificationObject);
	if (!obj)
	{
		DebugBreak();
		RETURN_ARG(0);
	}

	auto* evt = static_cast<xenon::KernelEvent*>(obj);
	const auto ret = evt->Set(increment, !!wait);
	RETURN_ARG(ret);
}

uint64 __fastcall XboxThreads_NtSetEvent( uint64 ip, cpu::CpuRegs& regs )
{
	const uint32 eventHandle = (const uint32) regs.R3;
	const uint32 statePtr = (const uint32) regs.R4;

	auto* obj = GPlatform.GetKernel().ResolveHandle( eventHandle, xenon::KernelObjectType::Event);
	if (!obj)
		RETURN_ARG(-1);

	auto* evt = static_cast<xenon::KernelEvent*>(obj);
	const uint32 ret = evt->Set(0, false);

    if ( statePtr )
		mem::storeAddr( statePtr, ret );

	RETURN_ARG(0);
}

uint64 __fastcall XboxThreads_KePulseEvent( uint64 ip, cpu::CpuRegs& regs )
{
	DebugBreak();
	RETURN_DEFAULT();
}

uint64 __fastcall XboxThreads_NtPulseEvent( uint64 ip, cpu::CpuRegs& regs )
{
	const uint32 evtHandle = (const uint32) regs.R3;
	const uint32 statePtr = (const uint32) regs.R4;

	auto* obj = GPlatform.GetKernel().ResolveHandle(evtHandle, xenon::KernelObjectType::Event);
	if (!obj)
		RETURN_ARG(-1);

	auto* evt = static_cast<xenon::KernelEvent*>(obj);
	const uint32 ret = evt->Set(0, false);

    if (statePtr)
		mem::storeAddr(statePtr, ret);

	RETURN_ARG(ret);
}

uint64 __fastcall XboxThreads_KeResetEvent( uint64 ip, cpu::CpuRegs& regs )
{
	const uint32 eventPtr = (const uint32) regs.R3;	

	auto* nativeEvent = (xnative::XEVENT*) eventPtr;
	mem::storeAtomic<uint32>( &nativeEvent->Dispatch.SignalState, 0 );

	auto* obj = GPlatform.GetKernel().ResolveObject(nativeEvent, xenon::NativeKernelObjectType::EventNotificationObject);
	if (!obj)
	{
		DebugBreak();
		RETURN_ARG(0);
	}

	auto* evt = static_cast<xenon::KernelEvent*>(obj);
	const uint32 ret = evt->Reset();

	RETURN_ARG(ret);
}

uint64 __fastcall XboxThreads_NtClearEvent( uint64 ip, cpu::CpuRegs& regs )
{
	const uint32 evtHandle = (const uint32) regs.R3;

	auto* obj = GPlatform.GetKernel().ResolveHandle(evtHandle, xenon::KernelObjectType::Event);
	if (!obj)
		RETURN_ARG(-1);

	auto* evt = static_cast<xenon::KernelEvent*>(obj);
	const uint32 ret = evt->Reset();
	RETURN_ARG(0);
}

//-----------------

uint64 __fastcall XboxThreads_NtCreateSemaphore( uint64 ip, cpu::CpuRegs& regs )
{
	DebugBreak();
	RETURN_DEFAULT();
}

uint64 __fastcall XboxThreads_KeInitializeSemaphore( uint64 ip, cpu::CpuRegs& regs )
{
	DebugBreak();
	RETURN_DEFAULT();
}

uint64 __fastcall XboxThreads_KeReleaseSemaphore( uint64 ip, cpu::CpuRegs& regs )
{
	DebugBreak();
	RETURN_DEFAULT();
}

uint64 __fastcall XboxThreads_NtReleaseSemaphore( uint64 ip, cpu::CpuRegs& regs )
{
	DebugBreak();
	RETURN_DEFAULT();
}

//-----------------

uint64 __fastcall XboxThreads_NtCreateMutant( uint64 ip, cpu::CpuRegs& regs )
{
	DebugBreak();
	RETURN_DEFAULT();
}

uint64 __fastcall XboxThreads_NtReleaseMutant( uint64 ip, cpu::CpuRegs& regs )
{
	DebugBreak();
	RETURN_DEFAULT();
}

//-----------------

uint64 __fastcall XboxThreads_NtCreateTimer( uint64 ip, cpu::CpuRegs& regs )
{
	DebugBreak();
	RETURN_DEFAULT();
}

uint64 __fastcall XboxThreads_NtSetTimerEx( uint64 ip, cpu::CpuRegs& regs )
{
	DebugBreak();
	RETURN_DEFAULT();
}

uint64 __fastcall XboxThreads_NtCancelTimer( uint64 ip, cpu::CpuRegs& regs )
{
	DebugBreak();
	RETURN_DEFAULT();
}

//-----------------

uint64 __fastcall XboxThreads_NtSignalAndWaitForSingleObjectEx( uint64 ip, cpu::CpuRegs& regs )
{
	RETURN_DEFAULT();
}

uint64 __fastcall XboxThreads_KfAcquireSpinLock( uint64 ip, cpu::CpuRegs& regs )
{
	const uint32 lockAddr = (const uint32) regs.R3;

	volatile unsigned int* lock = reinterpret_cast<volatile unsigned int* >( lockAddr );
	while ( !InterlockedCompareExchange( lock, 1, 0 ) != 0 )
	{
	}

	// Raise IRQL to DISPATCH to make sure this thread is executed
	auto* thread = xenon::KernelThread::GetCurrentThread();
	const uint32 oldIRQL = thread ? thread->RaiseIRQL(2) : 0;

	RETURN_ARG( oldIRQL );
}

uint64 __fastcall XboxThreads_KfReleaseSpinLock( uint64 ip, cpu::CpuRegs& regs )
{
	const uint32 lockAddr = (const uint32) regs.R3;
	const uint32 oldIRQL = (const uint32) regs.R4;
	
	// Restore IRQL
	auto* thread = xenon::KernelThread::GetCurrentThread();
	if ( thread )
		thread->LowerIRQL( oldIRQL );

	// Unlock
	volatile unsigned int* lock = reinterpret_cast<volatile unsigned int* >( lockAddr );
	InterlockedDecrement( lock );

	RETURN();
}

uint64 __fastcall XboxThreads_KeAcquireSpinLockAtRaisedIrql( uint64 ip, cpu::CpuRegs& regs )
{
	const uint32 lockAddr = (const uint32) regs.R3;

	volatile unsigned int* lock = reinterpret_cast<volatile unsigned int* >( lockAddr );
	while ( !InterlockedCompareExchange( lock, 1, 0 ) != 0 )
	{
	}

	RETURN();
}

uint64 __fastcall XboxThreads_KeReleaseSpinLockFromRaisedIrql( uint64 ip, cpu::CpuRegs& regs )
{
	const uint32 lockAddr = (const uint32) regs.R3;

	// Unlock
	volatile unsigned int* lock = reinterpret_cast<volatile unsigned int* >( lockAddr );
	InterlockedDecrement( lock );

	RETURN();
}

//-----------------

uint64 __fastcall XboxThreads_KeEnterCriticalRegion( uint64 ip, cpu::CpuRegs& regs )
{
	auto* currentThread = xenon::KernelThread::GetCurrentThread();
	RETURN_ARG( currentThread->EnterCriticalRegion() );
}

uint64 __fastcall XboxThreads_KeLeaveCriticalRegion( uint64 ip, cpu::CpuRegs& regs )
{
	auto* currentThread = xenon::KernelThread::GetCurrentThread();
	RETURN_ARG( currentThread->LeaveCriticalRegion() );
}

//-----------------

uint64 __fastcall XboxThreads_NtQueueApcThread( uint64 ip, cpu::CpuRegs& regs )
{
	DebugBreak();
	RETURN_DEFAULT();
}

uint64 __fastcall XboxThreads_KeInitializeApc( uint64 ip, cpu::CpuRegs& regs )
{
	DebugBreak();
	RETURN_DEFAULT();
}

uint64 __fastcall XboxThreads_KeInsertQueueApc( uint64 ip, cpu::CpuRegs& regs )
{
	DebugBreak();
	RETURN_DEFAULT();
}

uint64 __fastcall XboxThreads_KeRemoveQueueApc( uint64 ip, cpu::CpuRegs& regs )
{
	DebugBreak();
	RETURN_DEFAULT();
}

uint64 __fastcall XboxThreads_KiApcNormalRoutineNop( uint64 ip, cpu::CpuRegs& regs )
{
	const uint32 param0 = (const uint32) regs.R3;
	const uint32 param1 = (const uint32) regs.R4;
	
	GLog.Log("KiApcNormalRoutineNop(0x%08X, 0x%08X)", param0, param1 );

	RETURN_ARG(0);
}

//-----------------

uint64 __fastcall XboxThreads_KeWaitForSingleObject( uint64 ip, cpu::CpuRegs& regs )
{
	const uint32 objectPtr = (const uint32) regs.R3;
	const uint32 waitReason = (const uint32) regs.R4;
	const uint32 processorMode = (const uint32) regs.R5;
	const uint32 alertable = (const uint32) regs.R6;
	const uint32 timeoutPtr = (const uint32) regs.R7;

	// resolve object
	auto* object = GPlatform.GetKernel().ResolveObject( (void*)objectPtr, xenon::NativeKernelObjectType::Unknown, false );
	if (!object)
	{
		GLog.Err( "WaitSingle: object %08Xh does not exist", objectPtr );
		RETURN_ARG(xnative::X_STATUS_ABANDONED_WAIT_0);
	}

	// object is not waitable
	if ( !object->CanWait() )
	{
		GLog.Err( "WaitSingle: object %08Xh, '%s' is not waitable", objectPtr, object->GetName() );
		RETURN_ARG(xnative::X_STATUS_ABANDONED_WAIT_0);
	}

	const int64 timeout = timeoutPtr ? mem::loadAddr<int64>(timeoutPtr) : 0;
	auto* waitObject = static_cast< xenon::IKernelWaitObject* >( object );
	xnative::X_STATUS result = waitObject->Wait( waitReason, processorMode, !!alertable, timeoutPtr ? &timeout : nullptr);
	RETURN_ARG(result);
}

uint64 __fastcall XboxThreads_NtWaitForSingleObjectEx( uint64 ip, cpu::CpuRegs& regs )
{
	const uint32 object_handle = (const uint32) regs.R3;
	const uint8 wait_mode = (const uint8) regs.R4;
	const uint32 alertable = (const uint32) regs.R5;
	const uint32 timeoutPtr = (const uint32) regs.R6;
	GLog.Log( "WaitSingle(%08Xh, mode=%d, alert=%d, tptr=%08Xh, timeout=%d)", object_handle, wait_mode, alertable, timeoutPtr, timeoutPtr ? mem::loadAddr<int64>(timeoutPtr) : 0 );

	auto* object = GPlatform.GetKernel().ResolveHandle( object_handle, xenon::KernelObjectType::Unknown );
	if (!object)
	{
		GLog.Err( "WaitSingle: object with handle %08Xh does not exist", object_handle );
		RETURN_ARG(xnative::X_STATUS_ABANDONED_WAIT_0);
	}

	if (!object->CanWait())
	{
		GLog.Err( "WaitSingle: object %08Xh, '%s' is not waitable", object_handle, object->GetName() );
		RETURN_ARG(xnative::X_STATUS_ABANDONED_WAIT_0);
	}

	auto* waitObject = static_cast<xenon::IKernelWaitObject*>( object );

	const int64 timeout = timeoutPtr ? mem::loadAddr<int64>(timeoutPtr) : 0;
	const uint32 result = waitObject->Wait( 3, wait_mode, !!alertable, timeoutPtr ? &timeout : nullptr );
	RETURN_ARG(result);
}

uint64 __fastcall XboxThreads_KeWaitForMultipleObjects( uint64 ip, cpu::CpuRegs& regs )
{
	DebugBreak();
	RETURN_DEFAULT();
}

uint64 __fastcall XboxThreads_NtWaitForMultipleObjectsEx( uint64 ip, cpu::CpuRegs& regs )
{
	DebugBreak();
	RETURN_DEFAULT();
}

//---------------------------------------------------------------------------

void RtlInitializeCriticalSection( xnative::XCRITICAL_SECTION* rtlCS )
{
	auto* cs = GPlatform.GetKernel().CreateCriticalSection();
	auto* thread = xenon::KernelThread::GetCurrentThread();

	rtlCS->Synchronization.RawEvent[0] = 0xDAAD0FF0;
	rtlCS->Synchronization.RawEvent[1] = cs->GetHandle();
	rtlCS->OwningThread = thread ? thread->GetHandle() : 0;
	rtlCS->RecursionCount = 0;
	rtlCS->LockCount = 0;

	const uint32 kernelMarker = rtlCS->Synchronization.RawEvent[0];
	DEBUG_CHECK( kernelMarker == 0xDAAD0FF0 );
}

uint64 __fastcall XboxThreads_RtlInitializeCriticalSection( uint64 ip, cpu::CpuRegs& regs )
{
	xnative::XCRITICAL_SECTION* rtlCS = GetPointer<xnative::XCRITICAL_SECTION>(regs.R3);
	RtlInitializeCriticalSection( rtlCS );
	RETURN_ARG(0);
}

static void XInitCriticalSection( xnative::XCRITICAL_SECTION* ptr )
{
	auto* cs = GPlatform.GetKernel().CreateCriticalSection();
	auto* thread = xenon::KernelThread::GetCurrentThread();

	ptr->Synchronization.RawEvent[0] = 0xDAAD0FF0;
	ptr->Synchronization.RawEvent[1] = cs->GetHandle();
	ptr->OwningThread = thread->GetHandle();
	ptr->RecursionCount = 0;
	ptr->LockCount = 0;

	const uint32 kernelMarker = ptr->Synchronization.RawEvent[0];
	DEBUG_CHECK( kernelMarker == 0xDAAD0FF0 );
}

uint64 __fastcall XboxThreads_RtlInitializeCriticalSectionAndSpinCount( uint64 ip, cpu::CpuRegs& regs )
{
	xnative::XCRITICAL_SECTION* rtlCS = GetPointer<xnative::XCRITICAL_SECTION>(regs.R3);
	XInitCriticalSection( rtlCS );
	RETURN_ARG(0);
}

uint64 __fastcall XboxThreads_RtlDeleteCriticalSection( uint64 ip, cpu::CpuRegs& regs )
{
	xnative::XCRITICAL_SECTION* rtlCS = GetPointer<xnative::XCRITICAL_SECTION>(regs.R3);
	XInitCriticalSection( rtlCS );
	RETURN_ARG(0);
}

uint64 __fastcall XboxThreads_RtlLeaveCriticalSection( uint64 ip, cpu::CpuRegs& regs )
{
	const xnative::XCRITICAL_SECTION* ptr = GetPointer< xnative::XCRITICAL_SECTION >(regs.R3);
	DEBUG_CHECK( ptr );

	const uint32 kernelMarker = ptr->Synchronization.RawEvent[0];
	DEBUG_CHECK( kernelMarker == 0xDAAD0FF0 );

	const uint32 kernelHandle = ptr->Synchronization.RawEvent[1];

	auto* obj = GPlatform.GetKernel().ResolveHandle( kernelHandle, xenon::KernelObjectType::CriticalSection );
	DEBUG_CHECK( obj );

	auto* cs = static_cast< xenon::KernelCriticalSection* >( obj );
	cs->Leave();

	RETURN();
}

uint64 __fastcall XboxThreads_RtlEnterCriticalSection( uint64 ip, cpu::CpuRegs& regs )
{
	const xnative::XCRITICAL_SECTION* ptr = GetPointer< xnative::XCRITICAL_SECTION >(regs.R3);
	DEBUG_CHECK( ptr );

	const uint32 ptrAddr = (const uint32) ptr;
	const uint32 kernelMarker = ptr->Synchronization.RawEvent[0];
	if ( ptrAddr >= 0x82000000 )
	{
		if ( kernelMarker != 0xDAAD0FF0 )
		{
			GLog.Warn( "CS: trying to enter CS at %08Xh that was not initialized", ptr );
			XInitCriticalSection( const_cast< xnative::XCRITICAL_SECTION* >( ptr ) );
		}
	}
	else
	{
		DEBUG_CHECK( kernelMarker == 0xDAAD0FF0 );
	}

	const uint32 kernelHandle = ptr->Synchronization.RawEvent[1];
	auto* obj = GPlatform.GetKernel().ResolveHandle( kernelHandle, xenon::KernelObjectType::CriticalSection );
	DEBUG_CHECK( obj );

	auto* cs = static_cast<xenon::KernelCriticalSection*>(obj);
	cs->Enter();

	RETURN();
}

//---------------------------------------------------------------------------

void RegisterXboxThreads(runtime::Symbols& symbols)
{
	#define REGISTER(x) symbols.RegisterFunction(#x, (runtime::TBlockFunc) &XboxThreads_##x);

	REGISTER( NtClose );

	REGISTER( RtlInitializeCriticalSection );
	REGISTER( RtlInitializeCriticalSectionAndSpinCount );
	REGISTER( RtlDeleteCriticalSection );
	REGISTER( RtlEnterCriticalSection );
	REGISTER( RtlLeaveCriticalSection );

	REGISTER( KeTlsAlloc );
	REGISTER( KeTlsFree );
	REGISTER( KeTlsGetValue );
	REGISTER( KeTlsSetValue );

	REGISTER( ExCreateThread );
	REGISTER( ExTerminateThread );
	REGISTER( NtResumeThread );
	REGISTER( KeResumeThread );
	REGISTER( NtSuspendThread );
	REGISTER( KeSetAffinityThread );
	REGISTER( KeQueryBasePriorityThread );
	REGISTER( KeSetBasePriorityThread );
	REGISTER( KeDelayExecutionThread );
	REGISTER( NtYieldExecution );
	REGISTER( KeQuerySystemTime );

	REGISTER( NtCreateEvent );
	REGISTER( KeSetEvent );
	REGISTER( NtSetEvent );
	REGISTER( KePulseEvent );
	REGISTER( NtPulseEvent );
	REGISTER( KeResetEvent );
	REGISTER( NtClearEvent );

	REGISTER( NtCreateSemaphore );
	REGISTER( KeInitializeSemaphore );
	REGISTER( KeReleaseSemaphore );
	REGISTER( NtReleaseSemaphore );
	REGISTER( NtCreateMutant );
	REGISTER( NtReleaseMutant );
	REGISTER( NtCreateTimer );
	REGISTER( NtSetTimerEx );
	REGISTER( NtCancelTimer );
	REGISTER( KeWaitForSingleObject );
	REGISTER( NtWaitForSingleObjectEx );
	REGISTER( KeWaitForMultipleObjects );
	REGISTER( NtWaitForMultipleObjectsEx );
	REGISTER( NtSignalAndWaitForSingleObjectEx );
	REGISTER( KfAcquireSpinLock );
	REGISTER( KfReleaseSpinLock );
	REGISTER( KeAcquireSpinLockAtRaisedIrql );
	REGISTER( KeReleaseSpinLockFromRaisedIrql );
	REGISTER( KeEnterCriticalRegion );
	REGISTER( KeLeaveCriticalRegion );
	REGISTER( NtQueueApcThread );
	REGISTER( KeInitializeApc );
	REGISTER( KeInsertQueueApc );
	REGISTER( KeRemoveQueueApc );
	REGISTER( KiApcNormalRoutineNop );
	#undef REGISTER
}

//---------------------------------------------------------------------------
