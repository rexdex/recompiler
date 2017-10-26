#include "build.h"
#include "xenonLibNatives.h"
#include "xenonThread.h"
#include "xenonKernel.h"
#include "xenonCPU.h"
#include "xenonCPUDevice.h"
#include "xenonInplaceExecution.h"
#include "xenonMemory.h"

#include "../host_core/nativeMemory.h"
#include "../host_core/nativeKernel.h"
#include "../host_core/native.h"
#include "../host_core/runtimeCodeExecutor.h"
#include "../host_core/launcherCommandline.h"
#include "../host_core/runtimeTraceWriter.h"
#include "xenonTimeBase.h"

namespace xenon
{
	//-----------------------------------------------------------------------------

	static uint32 ConvWaitResult(native::WaitResult result)
	{
		switch (result)
		{
			case native::WaitResult::Success: return lib::X_STATUS_SUCCESS;
			case native::WaitResult::IOCompletion:return lib::X_STATUS_USER_APC;
			case native::WaitResult::Timeout: return lib::X_STATUS_TIMEOUT;
		}

		return lib::X_STATUS_ABANDONED_WAIT_0;
	}

	static uint32 TimeoutTicksToMs(int64 timeout_ticks)
	{
		if (timeout_ticks > 0)
		{
			DEBUG_CHECK(!"Invalid time base");
			return 0;
		}
		else if (timeout_ticks < 0)
		{
			return (uint32)(-timeout_ticks / 10000); // Ticks -> MS
		}

		return 0;
	}

	//-----------------------------------------------------------------------------

	IKernelObject::IKernelObject(Kernel* kernel, const KernelObjectType type, const char* name)
		: m_type(type)
		, m_name(name ? name : "")
		, m_kernel(kernel)
		, m_index(0)
	{
		m_kernel->AllocIndex(this, m_index);
		DEBUG_CHECK(m_index != 0);
	}

	IKernelObject::~IKernelObject()
	{
		m_kernel->ReleaseIndex(this, m_index);
		m_kernel = nullptr;
		m_index = 0;
	}

	void IKernelObject::SetObjectName(const char* name)
	{
		if (m_name != name)
			m_name = name;
	}

	const uint32 IKernelObject::GetHandle() const
	{
		uint32 id = (uint32)m_type << 24;
		id |= m_index;
		return id;
	}

	//-----------------------------------------------------------------------------

	IKernelObjectRefCounted::IKernelObjectRefCounted(Kernel* kernel, const KernelObjectType type, const char* name)
		: IKernelObject(kernel, type, name)
		, m_refCount(1)
	{
	}

	IKernelObjectRefCounted::~IKernelObjectRefCounted()
	{
		//DEBUG_CHECK(m_refCount == 0);
	}

	void IKernelObjectRefCounted::AddRef()
	{
		DEBUG_CHECK(m_refCount > 0);
		InterlockedIncrement(&m_refCount);
	}

	void IKernelObjectRefCounted::Release()
	{
		if (0 == InterlockedDecrement(&m_refCount))
		{
			delete this;
		}
	}

	//-----------------------------------------------------------------------------

	IKernelWaitObject::IKernelWaitObject(Kernel* kernel, const KernelObjectType type, const char* name)
		: IKernelObjectRefCounted(kernel, type, name)
	{}

	//-----------------------------------------------------------------------------

	KernelList::KernelList()
		: m_headAddr()
	{}

	void KernelList::Insert(Pointer<KernelListEntry> listData)
	{
		listData->m_flink = m_headAddr;
		listData->m_blink = m_headAddr;

		if (m_headAddr.IsValid())
		{
			Pointer<KernelListEntry> headData(m_headAddr);
			headData->m_blink = listData;
		}
	}

	bool KernelList::IsQueued(Pointer<KernelListEntry> listData)
	{
		return (m_headAddr == listData) || (listData->m_flink.IsValid()) || (listData->m_blink.IsValid());
	}

	void KernelList::Remove(Pointer<KernelListEntry> listData)
	{
		if (listData == m_headAddr)
		{
			m_headAddr = listData->m_flink;

			if (listData->m_flink.IsValid())
				listData->m_flink = nullptr;
		}
		else
		{
			if (listData->m_blink.IsValid())
				listData->m_blink->m_flink = nullptr;

			if (listData->m_flink.IsValid())
				listData->m_flink->m_blink = nullptr;
		}

		listData->m_blink = nullptr;
		listData->m_flink = nullptr;
	}

	const Pointer<KernelListEntry> KernelList::Pop()
	{
		if (!m_headAddr.IsValid())
			return nullptr;

		auto ptr = m_headAddr;
		Remove(ptr);

		DEBUG_CHECK(m_headAddr != ptr);
		return ptr;
	}

	bool KernelList::HasPending() const
	{
		return (m_headAddr.IsValid()) && (m_headAddr.GetAddress().GetAddressValue() != INVALID);
	}

	//-----------------------------------------------------------------------------

	KernelDispatcher::KernelDispatcher(native::ICriticalSection* lock)
		: m_list(new KernelList())
		, m_lock(lock)
	{
	}

	KernelDispatcher::~KernelDispatcher()
	{
		delete m_lock;
		delete m_list;
	}

	void KernelDispatcher::Lock()
	{
		m_lock->Acquire();
	}

	void KernelDispatcher::Unlock()
	{
		m_lock->Release();
	}

	//-----------------------------------------------------------------------------

	KernelStackMemory::KernelStackMemory(Kernel* kernel, const uint32 size)
		: IKernelObject(kernel, KernelObjectType::Stack, nullptr)
		, m_base(nullptr)
		, m_top(nullptr)
		, m_size(size)
	{
		// alloc stack
		m_base = GPlatform.GetMemory().VirtualAlloc(NULL, m_size, /*lib::XMEM_TOP_DOWN | */lib::XMEM_RESERVE | lib::XMEM_COMMIT, lib::XPAGE_READWRITE);
		DEBUG_CHECK(m_base != nullptr);

		// setup
		m_top = (uint8*)m_base + size - 0xB0;
		m_size = size;

		// cleanup stack with pattern
		memset(m_base, 0xCD, size);
	}

	KernelStackMemory::~KernelStackMemory()
	{
		uint32 freedSize = 0;
		GPlatform.GetMemory().VirtualFree(m_base, m_size, lib::XMEM_DECOMMIT | lib::XMEM_RELEASE, freedSize);

		m_base = nullptr;
		m_top = nullptr;
		m_size = 0;
	}

	//-----------------------------------------------------------------------------

	KernelThreadMemory::KernelThreadMemory(Kernel* kernel, const uint32 stackSize, const uint32 threadId, const uint64 entryPoint, const uint32 cpuIndex)
		: IKernelObject(kernel, KernelObjectType::ThreadBlock, nullptr)
		, m_stack(kernel, stackSize)
	{
		// total size to allocate
		const uint32 totalThreaDataSize = (THREAD_DATA_SIZE + PRC_DATA_SIZE + TLS_COUNT + SCRATCH_SIZE) * sizeof(uint32);

		// alloc stack
		m_block = GPlatform.GetMemory().VirtualAlloc(NULL, totalThreaDataSize, lib::XMEM_TOP_DOWN | lib::XMEM_RESERVE | lib::XMEM_COMMIT, lib::XPAGE_READWRITE);
		memset(m_block, 0, totalThreaDataSize);
		DEBUG_CHECK(m_block != nullptr);

		// prepare data buffers
		const auto* base = ((uint32*)m_block);
		m_tlsDataAddr = (uint32)&base[0];
		m_prcAddr = (uint32)&base[TLS_COUNT];
		m_threadDataAddr = (uint32)&base[TLS_COUNT + PRC_DATA_SIZE];
		m_scratchAddr = (uint32)&base[TLS_COUNT + PRC_DATA_SIZE + THREAD_DATA_SIZE];
		m_blockSize = totalThreaDataSize;
		
		// setup prc data block
		cpu::mem::storeAddr<uint32>(m_prcAddr + 0x000, m_tlsDataAddr); // tls address
		cpu::mem::storeAddr<uint32>(m_prcAddr + 0x030, m_prcAddr); // prc address
		cpu::mem::storeAddr<uint32>(m_prcAddr + 0x070, (uint32)m_stack.GetTop()); // stack to
		cpu::mem::storeAddr<uint32>(m_prcAddr + 0x070, (uint32)m_stack.GetBase()); // stack base
		cpu::mem::storeAddr<uint32>(m_prcAddr + 0x100, m_threadDataAddr); // thread data block
		cpu::mem::storeAddr<uint8>(m_prcAddr + 0x10C, cpuIndex); // cpu flag
		cpu::mem::storeAddr<uint32>(m_prcAddr + 0x150, 1); // dpc flag (guess)

													  // setup internal descriptor layout
													  // lib::XDISPATCH_HEADER
		cpu::mem::storeAddr<uint32>(m_threadDataAddr + 0x000, 6); // ThreadObject
		cpu::mem::storeAddr<uint32>(m_threadDataAddr + 0x008, m_threadDataAddr + 0x008); // list pointer
		cpu::mem::storeAddr<uint32>(m_threadDataAddr + 0x00C, m_threadDataAddr + 0x008);
		cpu::mem::storeAddr<uint32>(m_threadDataAddr + 0x010, m_threadDataAddr + 0x010); // list pointer

		cpu::mem::storeAddr<uint32>(m_threadDataAddr + 0x014, m_threadDataAddr + 0x010);
		cpu::mem::storeAddr<uint32>(m_threadDataAddr + 0x040, m_threadDataAddr + 0x020); // list pointer
		cpu::mem::storeAddr<uint32>(m_threadDataAddr + 0x044, m_threadDataAddr + 0x020);
		cpu::mem::storeAddr<uint32>(m_threadDataAddr + 0x048, m_threadDataAddr + 0x000);
		cpu::mem::storeAddr<uint32>(m_threadDataAddr + 0x04C, m_threadDataAddr + 0x018);
		cpu::mem::storeAddr<uint16>(m_threadDataAddr + 0x054, 0x102);
		cpu::mem::storeAddr<uint16>(m_threadDataAddr + 0x056, 0x1);
		cpu::mem::storeAddr<uint32>(m_threadDataAddr + 0x05C, (uint32)m_stack.GetTop());
		cpu::mem::storeAddr<uint32>(m_threadDataAddr + 0x060, (uint32)m_stack.GetBase());
		cpu::mem::storeAddr<uint32>(m_threadDataAddr + 0x068, m_tlsDataAddr);
		cpu::mem::storeAddr<uint8>(m_threadDataAddr + 0x06C, 0);
		cpu::mem::storeAddr<uint32>(m_threadDataAddr + 0x074, m_threadDataAddr + 0x074);
		cpu::mem::storeAddr<uint32>(m_threadDataAddr + 0x078, m_threadDataAddr + 0x074);
		cpu::mem::storeAddr<uint32>(m_threadDataAddr + 0x07C, m_threadDataAddr + 0x07C);
		cpu::mem::storeAddr<uint32>(m_threadDataAddr + 0x080, m_threadDataAddr + 0x07C);
		cpu::mem::storeAddr<uint8>(m_threadDataAddr + 0x08B, 1);
		cpu::mem::storeAddr<uint32>(m_threadDataAddr + 0x09C, 0xFDFFD7FF);
		cpu::mem::storeAddr<uint32>(m_threadDataAddr + 0x0D0, (uint32)m_stack.GetTop());
		cpu::mem::storeAddr<uint32>(m_threadDataAddr + 0x144, m_threadDataAddr + 0x144);
		cpu::mem::storeAddr<uint32>(m_threadDataAddr + 0x148, m_threadDataAddr + 0x144);
		cpu::mem::storeAddr<uint32>(m_threadDataAddr + 0x14C, (uint32)GetIndex());
		cpu::mem::storeAddr<uint32>(m_threadDataAddr + 0x150, (uint32)entryPoint);
		cpu::mem::storeAddr<uint32>(m_threadDataAddr + 0x154, m_threadDataAddr + 0x154);
		cpu::mem::storeAddr<uint32>(m_threadDataAddr + 0x158, m_threadDataAddr + 0x154);
		cpu::mem::storeAddr<uint32>(m_threadDataAddr + 0x160, 0); // last error
		cpu::mem::storeAddr<uint32>(m_threadDataAddr + 0x16C, 0); // creation flags
		cpu::mem::storeAddr<uint32>(m_threadDataAddr + 0x17C, 1);

		FILETIME t;
		GetSystemTimeAsFileTime(&t);
		cpu::mem::storeAddr<uint64>(m_threadDataAddr + 0x130, ((uint64)t.dwHighDateTime << 32) | t.dwLowDateTime);
	}

	KernelThreadMemory::~KernelThreadMemory()
	{
		uint32 freedSize = 0;
		GPlatform.GetMemory().VirtualFree(m_block, 0, lib::XMEM_DECOMMIT | lib::XMEM_RELEASE, freedSize);

		m_block = nullptr;
		m_prcAddr = 0;
		m_threadDataAddr = 0;
		m_scratchAddr = 0;
		m_tlsDataAddr = 0;
	}

	//-----------------------------------------------------------------------------

	KernelTLS::KernelTLS(Kernel* kernel)
		: IKernelObject(kernel, KernelObjectType::TLS, nullptr)
	{
		memset(&m_entries, 0, sizeof(m_entries));
	}

	KernelTLS::~KernelTLS()
	{
	}

	void KernelTLS::Set(const uint32 entryIndex, const uint64 value)
	{
		if (entryIndex < MAX_ENTRIES)
			m_entries[entryIndex] = value;
	}

	uint64 KernelTLS::Get(const uint32 entryIndex) const
	{
		if (entryIndex < MAX_ENTRIES)
			return m_entries[entryIndex];

		return 0;
	}

	//-----------------------------------------------------------------------------

	KernelCriticalSection::KernelCriticalSection(Kernel* kernel, native::ICriticalSection* nativeObject)
		: IKernelObject(kernel, KernelObjectType::CriticalSection, nullptr)
		, m_lock(nativeObject)
		, m_isLocked(false)
		, m_spinCount(0)
	{
	}

	KernelCriticalSection::~KernelCriticalSection()
	{
		delete m_lock;
		m_lock = nullptr;
	}

	void KernelCriticalSection::Enter()
	{
		m_isLocked = true;
		m_lock->Acquire();
	}

	void KernelCriticalSection::Leave()
	{
		m_lock->Release();
		m_isLocked = false;
	}

	//-----------------------------------------------------------------------------

	KernelEvent::KernelEvent(Kernel* kernel, native::IEvent* nativeEvent, const char* name)
		: IKernelWaitObject(kernel, KernelObjectType::Event, name)
		, m_event(nativeEvent)
	{}

	KernelEvent::~KernelEvent()
	{
		delete m_event;
		m_event = nullptr;
	}

	uint32 KernelEvent::Set(uint32 priority_increment, bool wait)
	{
		return m_event->Set();
	}

	uint32 KernelEvent::Pulse(uint32 priority_increment, bool wait)
	{
		return m_event->Pulse();
	}

	uint32 KernelEvent::Reset()
	{
		return m_event->Reset();
	}

	uint32 KernelEvent::Wait(const uint32 waitReason, const uint32 processorMode, const bool alertable, const int64* optTimeout)
	{
		const auto timeoutValue = optTimeout ? TimeoutTicksToMs(*optTimeout) : native::TimeoutInfinite;
		const auto result = m_event->Wait(timeoutValue, alertable);
		return ConvWaitResult(result);
	}

	native::IKernelObject* KernelEvent::GetNativeObject() const
	{
		return m_event;
	}

	//-----------------------------------------------------------------------------

	KernelSemaphore::KernelSemaphore(Kernel* kernel, native::ISemaphore* nativeSemaphore, const char* name /*= "Semaphore"*/)
		: IKernelWaitObject(kernel, KernelObjectType::Semaphore, name)
		, m_semaphore(nativeSemaphore)
	{}

	KernelSemaphore::~KernelSemaphore()
	{
		delete m_semaphore;
		m_semaphore = nullptr;
	}

	uint32 KernelSemaphore::Release(uint32 count)
	{
		return m_semaphore->Release(count);
	}

	uint32 KernelSemaphore::Wait(const uint32 waitReason, const uint32 processorMode, const bool alertable, const int64* optTimeout)
	{
		const auto timeoutValue = optTimeout ? TimeoutTicksToMs(*optTimeout) : native::TimeoutInfinite;
		const auto result = m_semaphore->Wait(timeoutValue, alertable);
		return ConvWaitResult(result);
	}

	native::IKernelObject* KernelSemaphore::GetNativeObject() const
	{
		return m_semaphore;
	}

	//-----------------------------------------------------------------------------

	KernelTimer::KernelTimer(Kernel* kernel, native::ITimer* nativeTimer, const char* name/* = nullptr*/)
		: IKernelWaitObject(kernel, KernelObjectType::Timer, name)
		, m_timer(nativeTimer)
	{}

	KernelTimer::~KernelTimer()
	{
		delete m_timer;
		m_timer = nullptr;
	}

	bool KernelTimer::Setup(int64_t waitTime, int64_t periodMs, MemoryAddress callbackFuncAddress, uint32 callbackArg)
	{
		// the callback runs in context of the calling thread
		auto currentThead = KernelThread::GetCurrentThread();
		DEBUG_CHECK(currentThead != nullptr);

		// setup callback, we will use the system's timer callback to push APC to our thread
		std::function<void()> callback = nullptr;
		if (callbackFuncAddress.IsValid())
		{
			callback = [currentThead, callbackFuncAddress, callbackArg]()
			{				
				// https://github.com/benvanik/xenia/blob/bc0ddbb05a9872a72d6af0afc5caa464ddb1a417/src/xenia/kernel/xtimer.cc
				auto currentTime = GPlatform.GetTimeBase().GetSystemTime();
				auto currentTimeLow = (uint32_t)(currentTime);
				auto currentTimeHigh = (uint32_t)(currentTime >> 32);
				currentThead->EnqueueAPC(callbackFuncAddress, callbackArg, currentTimeLow, currentTimeHigh);
			};
		}

		if (!periodMs)
			return m_timer->SetOnce(100 * waitTime, callback);
		else
			return m_timer->SetRepeating(100 * waitTime, periodMs, callback);
	}

	bool KernelTimer::Cancel()
	{
		return m_timer->Cancel();
	}

	uint32 KernelTimer::Wait(const uint32 waitReason, const uint32 processorMode, const bool alertable, const int64* optTimeout)
	{
		const auto timeoutValue = optTimeout ? TimeoutTicksToMs(*optTimeout) : native::TimeoutInfinite;
		const auto result = m_timer->Wait(timeoutValue, alertable);
		return ConvWaitResult(result);
	}

	native::IKernelObject* KernelTimer::GetNativeObject() const
	{
		return m_timer;
	}

	//-----------------------------------------------------------------------------

	KernelMutant::KernelMutant(Kernel* kernel, native::IMutant* nativeMutant, const char* name /*= nullptr*/)
		: IKernelWaitObject(kernel, KernelObjectType::Mutant, name)
		, m_mutant(nativeMutant)
	{}

	KernelMutant::~KernelMutant()
	{
		delete m_mutant;
		m_mutant = nullptr;
	}

	bool KernelMutant::Release()
	{
		return m_mutant->Release();
	}

	uint32 KernelMutant::Wait(const uint32 waitReason, const uint32 processorMode, const bool alertable, const int64* optTimeout)
	{
		const auto timeoutValue = optTimeout ? TimeoutTicksToMs(*optTimeout) : native::TimeoutInfinite;
		const auto result = m_mutant->Wait(timeoutValue, alertable);
		return ConvWaitResult(result);
	}

	native::IKernelObject* KernelMutant::GetNativeObject() const
	{
		return m_mutant;
	}

	//-----------------------------------------------------------------------------

	KernelEventNotifier::KernelEventNotifier(Kernel* kernel)
		: IKernelObject(kernel, KernelObjectType::EventNotifier, nullptr)
	{}

	KernelEventNotifier::~KernelEventNotifier()
	{}

	const bool KernelEventNotifier::Empty() const
	{
		std::lock_guard<std::mutex> lock(m_notificationLock);
		return m_notifications.empty();
	}

	void KernelEventNotifier::PushNotification(const uint32 id, const uint32 data)
	{
		std::lock_guard<std::mutex> lock(m_notificationLock);
		m_notifications.push_back({ id, data });
	}

	const bool KernelEventNotifier::PopNotification(uint32& outId, uint32& outData)
	{
		std::lock_guard<std::mutex> lock(m_notificationLock);
		if (m_notifications.empty())
			return false;

		const auto data = m_notifications[0];
		m_notifications.erase(m_notifications.begin());

		outId = data.m_id;
		outData = data.m_data;
		return true;
	}

	const bool KernelEventNotifier::PopSpecificNotification(const uint32 id, uint32& outData)
	{
		std::lock_guard<std::mutex> lock(m_notificationLock);
		if (m_notifications.empty())
			return false;

		for (auto it = m_notifications.begin(); it != m_notifications.end(); ++it)
		{
			if ((*it).m_id == id)
			{
				outData = (*it).m_data;
				return true;
			}
		}

		return false;
	}

	//-----------------------------------------------------------------------------

	Kernel::Kernel(const native::Systems& system, const launcher::Commandline& cmdline)
		: m_nativeKernel(system.m_kernel)
		, m_maxObjectIndex(1)
		, m_numFreeIndices(0)
		, m_exitRequested(false)
		, m_irqLevel(IRQL_Normal)
	{
		memset(&m_objects, 0, sizeof(m_objects));
		memset(&m_freeIndices, 0, sizeof(m_freeIndices));

		for (uint32 i = 0; i < MAX_TLS; ++i)
			m_tlsFreeEntries[i] = true;
	}

	Kernel::~Kernel()
	{
		DEBUG_CHECK(m_threads.empty());
	}

	KernelIrql Kernel::RaiseIRQL(const KernelIrql level)
	{
		const auto oldLevel = m_irqLevel.exchange(level);
		DEBUG_CHECK(oldLevel < level);
		return oldLevel;
	}

	KernelIrql Kernel::LowerIRQL(const KernelIrql level)
	{
		const auto oldLevel = m_irqLevel.exchange(level);
		DEBUG_CHECK(oldLevel > level);
		KernelThread::GetCurrentThread()->ProcessAPC();
		return oldLevel;
	}

	void Kernel::StopAllThreads()
	{
		if (!m_threads.empty())
		{
			GLog.Log("Kernel: There are still %d threads running, stopping them", m_threads.size());

			for (auto* ptr : m_threads)
				delete ptr;
			m_threads.clear();
		}
	}

	void Kernel::AllocIndex(IKernelObject* object, uint32& outIndex)
	{
		DEBUG_CHECK(object != nullptr);
		DEBUG_CHECK(outIndex == 0);

		std::lock_guard<std::mutex> lock(m_lock);

		// get from list
		if (m_maxObjectIndex < MAX_OBJECT)
		{
			outIndex = m_maxObjectIndex++;
		}
		else if (m_numFreeIndices > 0)
		{
			outIndex = m_freeIndices[m_numFreeIndices - 1];
			m_numFreeIndices -= 1;
		}

		// add to list
		DEBUG_CHECK(m_objects[outIndex] == NULL);
		m_objects[outIndex] = object;

		// in case of named objects, add it to the named list
		if (object->GetName() != nullptr)
		{
			auto it = m_namedObjets.find(object->GetName());
			DEBUG_CHECK(it == m_namedObjets.end());
			m_namedObjets[object->GetName()] = object;

			GLog.Log("Kernel: Registered name object '%hs', type %u, handle 0x%08X", object->GetName(), object->GetType(), object->GetHandle());
		}
	}

	void Kernel::ReleaseIndex(IKernelObject* object, uint32& outIndex)
	{
		DEBUG_CHECK(object != nullptr);
		DEBUG_CHECK(outIndex != 0);

		// HACK: keep track of the event notifier table
		if (object->GetType() == KernelObjectType::EventNotifier)
		{
			std::lock_guard<std::mutex> lock(m_eventNotifiersLock);
			utils::RemoveFromVector(m_eventNotifiers, (KernelEventNotifier*)object);
		}

		std::lock_guard<std::mutex> lock(m_lock);

		// if the object was named remove it from the list of named objects
		if (object->GetName() != nullptr)
		{
			auto it = m_namedObjets.find(object->GetName());
			DEBUG_CHECK(it != m_namedObjets.end());
			m_namedObjets.erase(it);

			GLog.Log("Kernel: Unregistered name object '%hs', type %u, handle 0x%08X", object->GetName(), object->GetType(), object->GetHandle());
		}

		// unregister from the dispatch map
		{
			auto it = m_objectToDispatchHeader.find(object);
			if (it != m_objectToDispatchHeader.end())
			{
				m_dispatchHeaderToObject.erase(it->second);
				m_objectToDispatchHeader.erase(it);
			}
		}

		// remove from object list
		DEBUG_CHECK(m_objects[outIndex] == object);
		m_objects[outIndex] = nullptr;

		// read the object's index to the list so it can be reused
		m_freeIndices[m_numFreeIndices] = outIndex;
		outIndex = 0;
		m_numFreeIndices += 1;
	}

	bool Kernel::DuplicateHandle(const uint32 handle, uint32& newHandle)
	{
		auto* obj = ResolveHandle(handle, KernelObjectType::Unknown);
		if (!obj)
			return false;

		if (obj->IsRefCounted())
		{
			auto* refCounted = (IKernelWaitObject*)obj;
			refCounted->AddRef();
		}

		// TODO!
		newHandle = handle;
		return true;
	}

	bool Kernel::CloseHandle(const uint32 handle)
	{
		// TODO!

		auto* obj = ResolveHandle(handle, KernelObjectType::Unknown);
		if (!obj)
			return nullptr;

		// remove object from mapping
		if (obj->IsRefCounted())
		{
			auto* refCounted = (IKernelWaitObject*)obj;
			refCounted->Release();
		}

		return true;
	}

	IKernelObject* Kernel::ResolveAny(const uint32 handle)
	{
		std::lock_guard<std::mutex> lock(m_lock);

		// no object
		if (!handle)
			return nullptr;

		// special cases
		if (handle == 0xFFFFFFFF)
		{
			GLog.Err("Kernel: CurrentProcess, WTF? ");
			return 0;
		}
		else if (handle == 0xFFFFFFFE)
		{
			return KernelThread::GetCurrentThread();
		}

		// generic case - get the id
		const uint32 index = handle & (MAX_OBJECT - 1);
		if (!index)
		{
			GLog.Err("Kernel: unresolved object, ID=%08X", handle);
			return nullptr;
		}

		return m_objects[index];
	}

	IKernelObject* Kernel::ResolveHandle(const uint32 handle, KernelObjectType requestedType)
	{
		std::lock_guard<std::mutex> lock(m_lock);

		// no object
		if (!handle)
			return nullptr;

		// special cases
		if (handle == 0xFFFFFFFF)
		{
			GLog.Err("Kernel: CurrentProcess, WTF? ");
			return 0;
		}
		else if (handle == 0xFFFFFFFE)
		{
			if (requestedType == KernelObjectType::Thread || requestedType == KernelObjectType::Unknown)
				return KernelThread::GetCurrentThread();

			return nullptr;
		}

		// generic case - get the id
		const uint32 index = handle & (MAX_OBJECT - 1);
		if (!index)
		{
			GLog.Err("Kernel: unresolved handle, ID=%08X", handle);
			return nullptr;
		}

		IKernelObject* object = m_objects[index];
		if (!object)
		{
			GLog.Err("Kernel: unresolved object, ID=%08X", handle);
			return nullptr;
		}

		const KernelObjectType type = (KernelObjectType)(handle >> 24);
		if (object->GetType() != type)
		{
			GLog.Err("Kernel: unresolved object, ID=%08X, type=%d/%d", handle, type, object->GetType());
			return nullptr;
		}

		if ((requestedType != KernelObjectType::Unknown) && (requestedType != type))
		{
			GLog.Err("Kernel: object type mismatch, ID=%08X, actual type=%d, requested type=%d", handle, type, requestedType);
			return nullptr;
		}

		return object;
	}

	IKernelObject* Kernel::ResolveNamedObject(const char* name, KernelObjectType requestedType)
	{
		std::lock_guard<std::mutex> lock(m_lock);

		// invalid name
		if (!name || !*name)
			return nullptr;

		// find in the named map
		auto it = m_namedObjets.find(name);
		if (it == m_namedObjets.end())
			return nullptr;

		// check type
		auto* object = it->second;
		if (requestedType != KernelObjectType::Unknown && object->GetType() != requestedType)
			return nullptr;

		// we've found the object
		return object;
	}

	IKernelObject* Kernel::CreateInlinedObject(Pointer<KernelDispatchHeader> dispatchHeader, NativeKernelObjectType requestedType)
	{
		// due to the kernel function in-lining (how brilliant...) sometimes we don't have the proper initialization call for the object
		// this function attempts to create our wrapper in case we see an object for the first time

		// http://www.nirsoft.net/kernel_struct/vista/KOBJECTS.html
		// https://github.com/benvanik/xenia/blob/bc0ddbb05a9872a72d6af0afc5caa464ddb1a417/src/xenia/kernel/xobject.cc
		IKernelObject* object = NULL;
		switch (requestedType)
		{
			case NativeKernelObjectType::EventNotificationObject:
			case NativeKernelObjectType::EventSynchronizationObject:
			{
				const bool manualReset = (dispatchHeader->m_typeAndFlags >> 24) == 0;
				const bool initialState = dispatchHeader->m_signalState ? true : false;
				auto* nativeEvent = m_nativeKernel->CreateEvent(manualReset, initialState);
				auto* object = new KernelEvent(this, nativeEvent, nullptr);
				BindDispatchObjectToKernelObject(object, dispatchHeader);
				return object;
			}

			case NativeKernelObjectType::MutantObject:
			{
				GLog.Err("Kernel: GetObject - trying to initialize mutant");
				//object = new CMutant(nativePtr, header);
				break;
			}

			case NativeKernelObjectType::SemaphoreObject:
			{
				const auto semaphoreHeader = Pointer<KernelSemaphoreHeader>(dispatchHeader.GetNativePointer());
				const auto maxCount = semaphoreHeader->m_maxCount;
				const auto currentCount = semaphoreHeader->m_dispatchHeader.m_signalState;

				auto* nativeSemaphore = m_nativeKernel->CreateSemaphore(currentCount, maxCount);
				auto* object = new KernelSemaphore(this, nativeSemaphore, nullptr);
				BindDispatchObjectToKernelObject(object, dispatchHeader);
				return object;
			}
		}

		// unable to initialize ad-hock object
		GLog.Err("Kernel: Unable to initialize ad-hoc object from the DispatchHeader only: %d", requestedType);
		DebugBreak();
		return nullptr;
	}

	void Kernel::BindDispatchObjectToKernelObject(IKernelObject* object, Pointer<KernelDispatchHeader> dispatchHeader)
	{
		const auto dispatchHeaderAddress = dispatchHeader.GetAddress().GetAddressValue();

		std::lock_guard<std::mutex> lock(m_lock);

		const auto it = m_dispatchHeaderToObject.find(dispatchHeaderAddress);
		DEBUG_CHECK(it == m_dispatchHeaderToObject.end());
		m_dispatchHeaderToObject[dispatchHeaderAddress] = object;
		m_objectToDispatchHeader[object] = dispatchHeaderAddress;
	}

	IKernelObject* Kernel::ResolveObject(Pointer<KernelDispatchHeader> dispatchHeader, NativeKernelObjectType requestedType, const bool allowCreate)
	{
		DEBUG_CHECK(dispatchHeader.IsValid());

		// use the existing type
		if (requestedType == NativeKernelObjectType::Unknown)
		{
			requestedType = (NativeKernelObjectType)(dispatchHeader->m_typeAndFlags & 0xFF);
		}
		else
		{
			const auto actualType = (NativeKernelObjectType)(dispatchHeader->m_typeAndFlags & 0xFF);
			if (actualType != requestedType)
			{
				GLog.Err("Kernel: object type mismatch, ptr=%08X, actual type=%d, requested type=%d", dispatchHeader.GetAddress().GetAddressValue(), actualType, requestedType);
				return nullptr;
			}
		}

		// translate into the actual object pointer, this is hacked - instead of placing objects in a list we use 
		{
			std::lock_guard<std::mutex> lock(m_lock);

			const auto dispatchHeaderAddress = dispatchHeader.GetAddress().GetAddressValue();
			const auto it = m_dispatchHeaderToObject.find(dispatchHeaderAddress);
			if (it != m_dispatchHeaderToObject.end())
				return it->second;
		}

		// we haven't found the object, if it's allowed to create it do it now
		if (allowCreate)
			return CreateInlinedObject(dispatchHeader, requestedType);

		// no object create
		return nullptr;
	}

	void Kernel::PostEventNotification(const uint32 eventId, const uint32 eventData)
	{
		GLog.Log("Kernel: Posting global event 0x%08X, data 0x%08X", eventId, eventData);

		std::lock_guard<std::mutex> lock(m_eventNotifiersLock);

		for (auto* notifier : m_eventNotifiers)
			notifier->PushNotification(eventId, eventData);
	}

	uint32 Kernel::WaitMultiple(const std::vector<IKernelWaitObject*>& waitObjects, const bool waitAll, const uint64* timeout, const bool alertable)
	{
		// collect native kernel object
		std::vector<native::IKernelObject*> nativeKernelObjects;
		for (const auto* waitObject : waitObjects)
		{
			auto* nativeObject = waitObject->GetNativeObject();
			DEBUG_CHECK(nativeObject != nullptr);
			nativeKernelObjects.push_back(nativeObject);
		}

		// wait
		const auto timeoutValue = timeout ? TimeoutTicksToMs(*timeout) : native::TimeoutInfinite;
		const auto ret = m_nativeKernel->WaitMultiple(nativeKernelObjects, waitAll, timeoutValue, alertable);
		return ConvWaitResult(ret);
	}

	uint32 Kernel::SignalAndWait(IKernelWaitObject* signalObject, IKernelWaitObject* waitObject, const bool alertable, const uint64_t* timeout)
	{
		auto* nativeSignalObject = signalObject->GetNativeObject();
		auto* nativeWaitObject = waitObject->GetNativeObject();

		const auto timeoutValue = timeout ? TimeoutTicksToMs(*timeout) : native::TimeoutInfinite;
		const auto ret = m_nativeKernel->SignalAndWait(nativeSignalObject, nativeWaitObject, timeoutValue, alertable);
		return ConvWaitResult(ret);
	}

	void Kernel::ExecuteInterrupt(const uint32 cpuIndex, const uint32 callback, const uint64* args, const uint32 numArgs, const char* name /*= "IRQ"*/)
	{
		std::lock_guard<std::mutex> lock(m_interruptLock);

		static uint32 s_nextThreadID = 10000;

		// setup execution context
		InplaceExecutionParams cpuExecutionParams;
		cpuExecutionParams.m_stackSize = 32 << 10;
		cpuExecutionParams.m_entryPoint = callback;
		cpuExecutionParams.m_cpu = cpuIndex;
		cpuExecutionParams.m_threadId = s_nextThreadID++;
		cpuExecutionParams.m_args[0] = numArgs >= 1 ? args[0] : 0;
		cpuExecutionParams.m_args[1] = numArgs >= 2 ? args[1] : 0;

		// execute code
		InplaceExecution cpuExecution(this, cpuExecutionParams, name);
		cpuExecution.Execute();
	}

	uint32 Kernel::AllocTLSIndex()
	{
		std::lock_guard<std::mutex> lock(m_tlsLock);

		int32 freeIndex = -1;
		for (uint32 i = 0; i < MAX_TLS; ++i)
		{
			if (m_tlsFreeEntries[i])
			{
				freeIndex = i;
				break;
			}
		}

		DEBUG_CHECK(freeIndex != -1);

		m_tlsFreeEntries[freeIndex] = false;

		return freeIndex;
	}

	void Kernel::FreeTLSIndex(const uint32 index)
	{
		std::lock_guard<std::mutex> lock(m_interruptLock);

		DEBUG_CHECK(m_tlsFreeEntries[index] == false);
		m_tlsFreeEntries[index] = true;
	}

	void Kernel::SetCode(const runtime::CodeTable* code)
	{
		m_codeTable = code;
	}

	bool Kernel::AdvanceThreads()
	{
		bool hasRunningThreads = false;
		bool hasCrashedThreads = false;

		// locked access
		{
			std::lock_guard<std::mutex> lock(m_interruptLock);

			// process the thread update
			for (auto it = m_threads.begin(); it != m_threads.end(); )
			{
				auto* thread = *it;

				if (thread->HasCrashed())
				{
					GLog.Err("Kernel: Thread '%s' (ID=%d) has crashed", thread->GetName(), thread->GetIndex());
					hasCrashedThreads = true;
				}

				if (thread->HasStopped())
				{
					GLog.Warn("Process: Thread '%s' (ID=%d) removed from thread list", thread->GetName(), thread->GetIndex());

					delete thread;
					it = m_threads.erase(it);
				}
				else
				{
					hasRunningThreads = true;
					++it;
				}
			}
		}

		// all threads exited
		if (!hasRunningThreads)
		{
			GLog.Log("Process: All threads exited. Killing process.");
			return false;
		}

		// thread crashed
		if (hasCrashedThreads)
		{
			GLog.Err("Process: A thread crashed during program execution. Killing process.");
			return false;
		}

		// keep running until exit is request
		return !m_exitRequested;
	}

	//---

	KernelThread* Kernel::CreateThread(const KernelThreadParams& params)
	{
		auto* thread = new KernelThread(this, m_nativeKernel, params);

		{
			std::lock_guard<std::mutex> lock(m_threadLock);
			m_threads.push_back(thread);
		}
	
		// resume execution
		if (!params.m_suspended)
			thread->Resume();

		return thread;
	}

	KernelEvent* Kernel::CreateEvent(bool initialState, bool manualReset, const char* name /*= nullptr*/)
	{
		auto* nativeEvent = m_nativeKernel->CreateEvent(manualReset, initialState);
		return new KernelEvent(this, nativeEvent, name);
	}

	KernelSemaphore* Kernel::CreateSemaphore(uint32_t initialCount, uint32_t maxCount, const char* name /*= nullptr*/)
	{
		auto* nativeSemaphore = m_nativeKernel->CreateSemaphore(initialCount, maxCount);
		return new KernelSemaphore(this, nativeSemaphore, name);
	}

	KernelCriticalSection* Kernel::CreateCriticalSection()
	{
		auto* nativeCriticalSection = m_nativeKernel->CreateCriticalSection();
		return new KernelCriticalSection(this, nativeCriticalSection);
	}

	KernelEventNotifier* Kernel::CreateEventNotifier()
	{
		auto* ret = new KernelEventNotifier(this);

		{
			std::lock_guard<std::mutex> lock(m_eventNotifiersLock);
			m_eventNotifiers.push_back(ret);
		}

		return ret;
	}

	KernelTimer* Kernel::CreateManualResetTimer(const char* name /*= nullptr*/)
	{
		auto* nativeTimer = m_nativeKernel->CreateManualResetTimer();
		return new KernelTimer(this, nativeTimer, name);

	}

	KernelTimer* Kernel::CreateSynchronizationTimer(const char* name /*= nullptr*/)
	{
		auto* nativeTimer = m_nativeKernel->CreateSynchronizationTimer();
		return new KernelTimer(this, nativeTimer, name);
	}

	KernelMutant* Kernel::CreateMutant(const bool initiallyOwned, const char* name /*= nullptr*/)
	{
		auto* nativeMutant = m_nativeKernel->CreateMutant(initiallyOwned);
		return new KernelMutant(this, nativeMutant, name);
	}

	//----

	uint32 KernelThread::Wait(const uint32 waitReason, const uint32 processorMode, const bool alertable, const int64* optTimeout)
	{
		const auto timeoutValue = optTimeout ? TimeoutTicksToMs(*optTimeout) : native::TimeoutInfinite;
		const auto result = m_nativeThread->Wait(timeoutValue, alertable);
		return ConvWaitResult(result);
	}

	native::IKernelObject* KernelThread::GetNativeObject() const
	{
		return m_nativeThread;
	}

	//----

} // xenon
