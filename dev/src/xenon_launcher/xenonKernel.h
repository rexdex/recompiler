#pragma once

#include "xenonLibNatives.h"
#include "xenonCPU.h"

#include "../host_core/nativeKernel.h"
#include "../host_core/nativeMemory.h"

namespace native
{
	struct Systems;
}

namespace runtime
{
	class CodeExecutor;
	class CodeTable;
	class IDeviceCPU;
	class TraceWriter;
}

namespace launcher
{
	class Commandline;
}

namespace xenon
{

	//---------------------------------------------------------------------------

	class Kernel;
	class KernelEvent;
	class KernelCriticalSection;
	class KernelThread;

	//---------------------------------------------------------------------------

	/// IRQ level 
	enum KernelIrql : uint32_t
	{
		IRQL_Normal = 0,
		IRQL_APC = 1,
		IRQL_Dispatch = 2,
		IRQL_DPC = 3,
	};

	enum class KernelObjectType : uint8
	{
		Unknown = 0,
		Process = 10,
		APC = 18,
		Thread = 20,
		CriticalSection = 30,
		Semaphore = 40,
		Event = 50,
		Stack = 60,
		PhysicalMemoryBlock = 70,
		VirtualMemoryBlock = 80,
		DPC = 90,
		TLS = 100,
		FileHandle = 110,
		FileSysEntry = 120,
		ThreadBlock = 130,
		EventNotifier = 140,
		Timer = 150,
		Mutant = 160,
		Waitable = 255,
	};

	enum class NativeKernelObjectType : int32
	{
		Unknown = -1,

		EventNotificationObject=0,
		EventSynchronizationObject=1,
		MutantObject=2,
		SemaphoreObject=5,

		ProcessObject=3,
		QueueObject=4,
		ThreadObject=6,
		GateObject=7,
		TimerNotificationObject=8,
		TimerSynchronizationObject=9,
		ApcObject=18,
		DpcObject=19,
		DeviceQueueObject=20,
		EventPairObject=21,
		InterruptObject=22,
		ProfileObject=23,
		ThreadedDpcObject=24,
	};

	enum class KernelIRQLevel : uint32
	{
		PASSIVE = 0,
		APC = 1,
		DISPATCH = 2,
		DPC = 3,
	};

	//---------------------------------------------------------------------------

	/// entry in the list
	struct KernelListEntry
	{
		Pointer<KernelListEntry> m_flink;
		Pointer<KernelListEntry> m_blink;
	};

	/// List wrapper, all data big-endian
	class KernelList
	{
	public:
		KernelList();

		// insert address to the list
		void Insert(Pointer<KernelListEntry> listEntryPtr);

		// check if address is queued
		bool IsQueued(Pointer<KernelListEntry> listEntryPtr);

		// remove entry
		void Remove(Pointer<KernelListEntry> listEntryPtr);

		// shift data
		const Pointer<KernelListEntry> Pop();

		// do we have pending data ?
		bool HasPending() const;

	private:
		static const uint32 INVALID = 0xE0FE0FFF;
		Pointer<KernelListEntry> m_headAddr;
	};

	//---------------------------------------------------------------------------

	// http://www.nirsoft.net/kernel_struct/vista/DISPATCHER_HEADER.html
	// http://www.nirsoft.net/kernel_struct/vista/KEVENT.html
	struct KernelDispatchHeader
	{
		Field<uint32> m_typeAndFlags;
		Field<uint32> m_signalState;
		KernelListEntry m_listEntry;

		inline KernelDispatchHeader()
		{
			Reset();
		}

		inline void Reset()
		{
			m_typeAndFlags = 0;
			m_signalState = 0;
			m_listEntry = KernelListEntry();
		}
	};

	// in-memory header for the semaphore
	struct KernelSemaphoreHeader
	{
		KernelDispatchHeader m_dispatchHeader;
		Field<uint32_t> m_maxCount;
	};

	//---------------------------------------------------------------------------

	/// Base kernel object
	class IKernelObject
	{
	public:
		IKernelObject(Kernel* kernel, const KernelObjectType type, const char* name);
		virtual ~IKernelObject();

		// get owning kernel object
		inline Kernel* GetOwner() const { return m_kernel; }

		// get type of kernel object
		inline const KernelObjectType GetType() const { return m_type; }

		// get name of kernel object (may be empty)
		inline const char* GetName() const { return m_name.empty() ? nullptr : m_name.c_str(); }

		// get index of the object (NOT ID)
		inline const uint32 GetIndex() const { return m_index; }

		// change the kernel object name
		void SetObjectName(const char* name);

		// get object id (handle), combines index and type
		const uint32 GetHandle() const;

		// does this object support refcounting ?
		// object get's deleted when the refcount goes to zero
		virtual bool IsRefCounted() const { return false; }

		// does this object support waiting ?
		// the Wait method is still up to the object
		virtual bool CanWait() const { return false; }

	private:
		// object type & name
		KernelObjectType	m_type;
		std::string			m_name;

		// internal index in the object map (not an ID yet)
		uint32				m_index;

		// owner
		Kernel*				m_kernel;
	};

	//---------------------------------------------------------------------------

	/// Reference counted handles
	class IKernelObjectRefCounted : public IKernelObject
	{
	public:
		IKernelObjectRefCounted(Kernel* kernel, const KernelObjectType type, const char* name);
		virtual ~IKernelObjectRefCounted();

		// close (for CloseHandle)
		virtual bool IsRefCounted() const { return true; }

		// add a reference
		void AddRef();

		// release a reference to this object, once it gets to zero the object is deleted
		void Release();

	private:
		volatile LONG		m_refCount;
	};

	//---------------------------------------------------------------------------

	// data block for the aync procedure call
	class KernelAPCData
	{
	public:
		Field<KernelObjectType> m_type;
		Field<uint8_t> m_padding;
		Field<uint8_t> m_processorMode;
		Field<uint8_t> m_inqueue;
		Field<Pointer<KernelDispatchHeader>> m_threadPtr;
		KernelListEntry m_listEntry;
		Field<MemoryAddress> m_kernelRoutine;
		Field<MemoryAddress> m_rundownRoutine;
		Field<MemoryAddress> m_normalRoutine;
		Field<uint32_t> m_normalContext;
		Field<uint32_t> m_arg1;
		Field<uint32_t> m_arg2;

		inline KernelAPCData()
		{
			Reset();
		}

		void Reset()
		{
			m_threadPtr = nullptr;
			m_listEntry = KernelListEntry();
			m_type = KernelObjectType::APC;
			m_processorMode = 0;
			m_processorMode = 0;
			m_kernelRoutine = MemoryAddress();
			m_rundownRoutine = MemoryAddress();
			m_normalRoutine = MemoryAddress();
			m_inqueue = 0;
			m_normalContext = 0;
			m_arg1 = 0;
			m_arg2 = 0;
		}
	};
	static_assert(sizeof(KernelAPCData) == 40, "Size is invalid");

	//---------------------------------------------------------------------------

	/// Dispatcher
	class KernelDispatcher
	{
	public:
		KernelDispatcher(native::ICriticalSection* lock);
		virtual ~KernelDispatcher();

		void Lock();
		void Unlock();

		inline KernelList* GetList() const { return m_list; }

	private:
		native::ICriticalSection*	m_lock;
		KernelList*					m_list;
	};

	//---------------------------------------------------------------------------

	/// Stack memory
	class KernelStackMemory : public IKernelObject
	{
	public:
		KernelStackMemory(Kernel* kernel, const uint32 size);
		virtual ~KernelStackMemory();

		// get base of the stack
		inline void* GetBase() const { return m_base; }

		// get top of the stack
		inline void* GetTop() const { return m_top; }

		// get current size of the stack
		inline uint32 GetSize() const { return m_size; }

	protected:
		void*				m_base;		// base address (stack bottom)
		void*				m_top;		// stack top
		uint32				m_size;		// size of the stack buffer
	};

	//---------------------------------------------------------------------------

	/// Thread block
	class KernelThreadMemory : public IKernelObject
	{
	public:
		KernelThreadMemory(Kernel* kernel, const uint32 stackSize, const uint32 threadId, const uint64 entryPoint, const uint32 cpuIndex);
		virtual ~KernelThreadMemory();

		// get thread stack
		inline KernelStackMemory& GetStack() { return m_stack; }

		// get thread data block
		inline const void* GetBlockData() const { return m_block; }

		// get size of thread data
		inline const uint32 GetBlockSize() const { return m_blockSize; }

		// get the thread data object address
		inline const uint32 GetThreadDataAddr() const { return m_threadDataAddr;  }

		// get the PRC table address
		inline const uint32 GetPRCAddr() const { return m_prcAddr; }

		// get scratch address
		inline const uint32 GetScratchAddr() const { return m_scratchAddr; }

		// get TLS block address
		inline const uint32 GetTLSAddr() const { return m_tlsDataAddr; }

	protected:
		static const uint32 THREAD_DATA_SIZE = 0xAB0;
		static const uint32 PRC_DATA_SIZE = 0x2D8;
		static const uint32 SCRATCH_SIZE = 64;
		static const uint32 TLS_COUNT = 64;

		void*				m_block;	// memory block
		uint32				m_blockSize; // total size of thread data

		uint32				m_tlsDataAddr;		// runtime TLS table
		uint32				m_prcAddr;			// PRC table address
		uint32				m_threadDataAddr;	// Thread data address
		uint32				m_scratchAddr;		// internal thread scratch pad

		KernelStackMemory	m_stack;
	};

	//---------------------------------------------------------------------------

	/// Thread local storage
	class KernelTLS : public IKernelObject
	{
	public:
		KernelTLS(Kernel* kernel);
		virtual ~KernelTLS();

		// set TLS entry 
		void Set(const uint32 entryIndex, const uint64 value);

		// get TLS entry
		uint64 Get(const uint32 entryIndex) const;

	protected:
		static const uint32 MAX_ENTRIES = 64;
		uint64	m_entries[MAX_ENTRIES];
	};

	//---------------------------------------------------------------------------

	/// Critical section object
	class KernelCriticalSection : public IKernelObject
	{
	public:
		KernelCriticalSection( Kernel* kernel, native::ICriticalSection* nativeObject );
		virtual ~KernelCriticalSection();

		void Enter();
		void Leave();

	private:
		native::ICriticalSection*	m_lock;
		volatile bool				m_isLocked;
		uint32						m_spinCount;
	};

	//---------------------------------------------------------------------------

	/// Kernel object with waiting
	class IKernelWaitObject : public IKernelObjectRefCounted
	{
	public:
		IKernelWaitObject(Kernel* kernel, const KernelObjectType type, const char* name = nullptr);

		virtual bool CanWait() const override final { return true; }
		virtual native::IKernelObject* GetNativeObject() const = 0;
		virtual uint32 Wait(const uint32 waitReason, const uint32 processorMode, const bool alertable, const int64* optTimeout) = 0;
	};

	//---------------------------------------------------------------------------

	/// Event
	class KernelEvent : public IKernelWaitObject
	{
	public:
		KernelEvent(Kernel* kernel, native::IEvent* nativeEvent, const char* name = nullptr);
		virtual ~KernelEvent();

		uint32 Set( uint32 priorityIncrement, bool wait);
		uint32 Pulse(uint32 priorityIncrement, bool wait);
		uint32 Reset();

		virtual uint32 Wait(const uint32 waitReason, const uint32 processorMode, const bool alertable, const int64* optTimeout) override final;
		virtual native::IKernelObject* GetNativeObject() const override final;

	private:
		native::IEvent*		m_event;
	};

	//---------------------------------------------------------------------------

	/// Classic semaphore
	class KernelSemaphore : public IKernelWaitObject
	{
	public:
		KernelSemaphore(Kernel* kernel, native::ISemaphore* nativeSemaphore, const char* name = nullptr);
		virtual ~KernelSemaphore();

		uint32 Release(uint32 count);

		virtual uint32 Wait(const uint32 waitReason, const uint32 processorMode, const bool alertable, const int64* optTimeout) override final;
		virtual native::IKernelObject* GetNativeObject() const override final;

	private:
		native::ISemaphore*		m_semaphore;
	};

	//---------------------------------------------------------------------------

	/// Timer
	class KernelTimer : public IKernelWaitObject
	{
	public:
		KernelTimer(Kernel* kernel, native::ITimer* nativeTimer, const char* name = nullptr);
		virtual ~KernelTimer();

		// setup timer
		bool Setup(int64_t waitTime, int64_t periodMs, MemoryAddress callbackFuncAddress, uint32 callbackArg);

		// cancel pending timer
		bool Cancel();

		virtual uint32 Wait(const uint32 waitReason, const uint32 processorMode, const bool alertable, const int64* optTimeout) override final;
		virtual native::IKernelObject* GetNativeObject() const override final;

	private:
		native::ITimer* m_timer;
	};

	//---------------------------------------------------------------------------

	/// Mutant object
	class KernelMutant : public IKernelWaitObject
	{
	public:
		KernelMutant(Kernel* kernel, native::IMutant* nativeMutant, const char* name = nullptr);
		virtual ~KernelMutant();

		// release mutant
		bool Release();

		virtual uint32 Wait(const uint32 waitReason, const uint32 processorMode, const bool alertable, const int64* optTimeout) override final;
		virtual native::IKernelObject* GetNativeObject() const override final;

	private:
		native::IMutant* m_mutant;
	};

	//---------------------------------------------------------------------------

	/// Notifier
	class KernelEventNotifier : public IKernelObject
	{
	public:
		KernelEventNotifier(Kernel* kernel);
		virtual ~KernelEventNotifier();

		// checks if the queue is empty
		const bool Empty() const;

		// push the notification into the queue
		void PushNotification(const uint32 id, const uint32 data);

		// pop notification from the queue, returns false if the queue is empty
		// gets first from the list 
		const bool PopNotification(uint32& outId, uint32& outData);

		// pop notification with specific ID
		const bool PopSpecificNotification(const uint32 id, uint32& outData);

	private:
		struct Notifcation
		{
			uint32 m_id;
			uint32 m_data;
		};

		std::vector<Notifcation> m_notifications;
		mutable std::mutex m_notificationLock;
	};

	//---------------------------------------------------------------------------

	class KernelThreadParams
	{
	public:
		char		m_name[64];			// thread name
		uint32		m_entryPoint;		// thread entry point
		uint32		m_stackSize;		// thread stack size
		uint64		m_args[2];			// startup args, R3 R4
		bool		m_suspended;		// create thread in suspended mode

		inline KernelThreadParams()
			: m_stackSize(64 << 10)
			, m_entryPoint(0)
			, m_suspended(false)
		{
			m_args[0] = 0;
			m_args[1] = 0;
		}
	};

	//---------------------------------------------------------------------------

	/// Kernel system
	class Kernel
	{
	public:
		Kernel( const native::Systems& system, const launcher::Commandline& cmdline );
		~Kernel();

		// get DPC dispatcher
		inline KernelDispatcher& GetDPCDispatcher() const { return *m_dpcDispatcher; }

		// get master code table
		inline const runtime::CodeTable& GetCode() const { return *m_codeTable; }

		//---

		// stop all running threads
		void StopAllThreads();

		// allocate entry in the object list
		void AllocIndex(IKernelObject* object, uint32& outIndex);

		// release entry in the object list
		void ReleaseIndex(IKernelObject* object, uint32& outIndex);

		// resolve ANY kernel object by handle
		IKernelObject* ResolveAny(const uint32 handle);

		// resolve kernel object by handle
		IKernelObject* ResolveHandle(const uint32 handle, KernelObjectType requestedType);

		// resolve kernel object from dispatch object
		IKernelObject* ResolveObject(Pointer<KernelDispatchHeader> dispatchEntry, NativeKernelObjectType requestedType, const bool allowCreate = true);

		// resolve named kernel object (SLOW)
		IKernelObject* ResolveNamedObject(const char* name, KernelObjectType requestedType);

		// make a copy of handle
		bool DuplicateHandle(const uint32 handle, uint32& newHandle);

		// close handle
		bool CloseHandle(const uint32 handle);

		// set master code table
		void SetCode(const runtime::CodeTable* code);

		// execute interrupt code
		void ExecuteInterrupt(const uint32 cpuIndex, const uint32 callback, const uint64* args, const uint32 numArgs, const char* name = "IRQ");

		// send global notification to all event notifiers
		void PostEventNotification(const uint32 eventId, const uint32 eventData);

		// wait for multiple waitable objects
		uint32 WaitMultiple(const std::vector<IKernelWaitObject*>& waitObjects, const bool waitAll, const uint64* timeout, const bool alertable);

		// signal object and wait
		uint32 SignalAndWait(IKernelWaitObject* signalObject, IKernelWaitObject* waitObject, const bool alertable, const uint64_t* timeout);

		// bind dispatch header pointer to kernel object
		void BindDispatchObjectToKernelObject(IKernelObject* object, Pointer<KernelDispatchHeader> dispatchEntry);

		//---

		// allocate TLS index (global)
		uint32 AllocTLSIndex();

		// free a TLS index (global)
		void FreeTLSIndex(const uint32 index);

		///---

		/// create a thread
		KernelThread* CreateThread(const KernelThreadParams& params);

		/// create an event
		KernelEvent* CreateEvent(bool initialState, bool manualReset, const char* name = nullptr);

		/// create an semaphore
		KernelSemaphore* CreateSemaphore(uint32_t initialCount, uint32_t maxCount, const char* name = nullptr);

		/// create a critical section
		KernelCriticalSection* CreateCriticalSection();

		/// create an event notifier
		KernelEventNotifier* CreateEventNotifier();

		/// create a timer with manual reset
		KernelTimer* CreateManualResetTimer(const char* name = nullptr);

		/// create a synchronization timer
		KernelTimer* CreateSynchronizationTimer(const char* name = nullptr);

		/// create a mutant (mutex)
		KernelMutant* CreateMutant(const bool initiallyOwned, const char* name = nullptr);

		//----

		// raise IRLQ, returns previous value
		KernelIrql RaiseIRQL(const KernelIrql level);

		// lower IRQ level, returns previous value
		KernelIrql LowerIRQL(const KernelIrql level);

		//----

		/// update state, returns false if all threads are dead
		bool AdvanceThreads();

	private:
		static const uint32 MAX_OBJECT = 65536;
		static const uint32 MAX_TLS = 64;

		//---

		// create kernel object from scratch, used when it first seen in the Resolve
		IKernelObject* CreateInlinedObject(Pointer<KernelDispatchHeader> nativeAddres, NativeKernelObjectType requestedType);

		//---

		// External request to kill everything and leave
		std::atomic<bool> m_exitRequested;

		//---

		// native kernel is used to do the heavy lifting when the STD is not enough
		native::IKernel* m_nativeKernel;

		// all executable code is registered in this table
		// this provides the implementation for each executable region in memory
		const runtime::CodeTable* m_codeTable;

		//--

		// all kernel objects
		IKernelObject* m_objects[MAX_OBJECT];
		uint32 m_maxObjectIndex;

		// free indices for kernel objects
		uint32 m_freeIndices[MAX_OBJECT];
		uint32 m_numFreeIndices;
		std::mutex m_lock;

		// list of mapped objects
		std::unordered_map<std::string, IKernelObject*> m_namedObjets;

		// map of the dispatch headers to object pointers
		std::unordered_map<uint32_t, IKernelObject*> m_dispatchHeaderToObject;
		std::unordered_map<IKernelObject*, uint32_t> m_objectToDispatchHeader;

		// active threads
		typedef std::vector< KernelThread* > TThreads;
		TThreads m_threads;
		std::mutex m_threadLock;

		// active event notifiers
		typedef std::vector< KernelEventNotifier* > TEventNotifiers;
		TEventNotifiers m_eventNotifiers;
		std::mutex m_eventNotifiersLock;

		//--

		// kernel DPC (deferred procedure call) implementation
		KernelDispatcher* m_dpcDispatcher;

		//--

		// lock for interrupt execution (we can do only one)
		std::mutex m_interruptLock;

		//---

		// TLS (thread local storage) mapping
		// TODO: move to Process
		std::mutex m_tlsLock;
		bool m_tlsFreeEntries[ MAX_TLS ];

		///----

		// IRQ level
		std::atomic<KernelIrql> m_irqLevel;
	};

	//---------------------------------------------------------------------------

} // xenon