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

	enum class KernelObjectType : uint32
	{
		Unknown = 0,
		Process = 10,
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
		inline const char* GetName() const { return m_name.c_str(); }

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

	protected:
		// bind the kernel object with internal xenon dispatch list
		void SetNativePointer(void* nativePtr);

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

	/// List wrapper, all data big-endian
	class KernelList
	{
	public:
		KernelList();

		// insert address to the list
		void Insert(const uint32 listEntryPtr);

		// check if address is queued
		bool IsQueued(const uint32 listEntryPtr);

		// remove entry
		void Remove(const uint32 listEntryPtr);

		// shift data
		const uint32 Pop();

		// do we have pending data ?
		bool HasPending() const;

	private:
		static const uint32 INVALID = 0xE0FE0FFF;
		uint32 m_headAddr;
	};

	//---------------------------------------------------------------------------

	/// Dispatcher
	class KernelDispatcher
	{
	public:
		KernelDispatcher(native::ICriticalSection* lock);
		~KernelDispatcher();

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
		~KernelStackMemory();

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
		native::IMemory*	m_memory;	// memory subsystem
	};

	//---------------------------------------------------------------------------

	/// Thread block
	class KernelThreadMemory : public IKernelObject
	{
	public:
		KernelThreadMemory(Kernel* kernel, const uint32 stackSize, const uint32 threadId, const uint64 entryPoint, const uint32 cpuIndex);
		~KernelThreadMemory();

		// get thread stack
		inline KernelStackMemory& GetStack() { return m_stack; }

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
		~KernelTLS();

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
		~KernelCriticalSection();

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
		IKernelWaitObject(Kernel* kernel, const KernelObjectType type, const char* name);

		virtual bool CanWait() const override final { return true; }
		virtual uint32 Wait(const uint32 waitReason, const uint32 processorMode, const bool alertable, const int64* optTimeout) = 0;
	};

	//---------------------------------------------------------------------------

	/// Event
	class KernelEvent : public IKernelWaitObject
	{
	public:
		KernelEvent(Kernel* kernel, native::IEvent* nativeEvent, const char* name = "Event");
		~KernelEvent();

		uint32 Set( uint32 priorityIncrement, bool wait);
		uint32 Pulse(uint32 priorityIncrement, bool wait);
		uint32 Reset();
		void Clear();

		virtual uint32 Wait(const uint32 waitReason, const uint32 processorMode, const bool alertable, const int64* optTimeout) override final;

	private:
		native::IEvent*		m_event;
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

		// get memory allocator
		inline native::IMemory& GetNativeMemory() const { return *m_nativeMemory;  }

		// get the native kernel
		inline native::IKernel& GetNativeKernel() const { return *m_nativeKernel; }

		// get master code table
		inline const runtime::CodeTable& GetCode() const { return *m_codeTable; }

		//---

		// allocate entry in the object list
		void AllocIndex(IKernelObject* object, uint32& outIndex);

		// release entry in the object list
		void ReleaseIndex(IKernelObject* object, uint32& outIndex);

		// resolve ANY kernel object by handle
		IKernelObject* ResolveAny(const uint32 handle);

		// resolve kernel object by handle
		IKernelObject* ResolveHandle(const uint32 handle, KernelObjectType requestedType);

		// resolve kernel object from dispatch object
		IKernelObject* ResolveObject(void* nativePtr, NativeKernelObjectType requestedType, const bool allowCreate = true);

		// set master code table
		void SetCode(const runtime::CodeTable* code);

		// execute interrupt code
		void ExecuteInterrupt(const uint32 cpuIndex, const uint32 callback, const uint64* args, const uint32 numArgs, const bool trace);

		//---

		// allocate TLS index (global)
		uint32 AllocTLSIndex();

		// free a TLS index (global)
		void FreeTLSIndex(const uint32 index);

		///---

		/// create a thread
		KernelThread* CreateThread(const KernelThreadParams& params);

		/// create an event
		KernelEvent* CreateEvent(bool initialState, bool manualReset);

		/// create a critical section
		KernelCriticalSection* CreateCriticalSection();

		/// create an event notifier
		KernelEventNotifier* CreateEventNotifier();

		//----

		/// update state, returns false if all threads are dead
		bool AdvanceThreads();

	private:
		static const uint32			MAX_OBJECT = 65536;
		static const uint32			MAX_TLS = 64;

		native::IKernel*			m_nativeKernel;
		native::IMemory*			m_nativeMemory;

		const runtime::CodeTable*	m_codeTable;

		IKernelObject*				m_objects[MAX_OBJECT];
		uint32						m_maxObjectIndex;

		uint32						m_freeIndices[MAX_OBJECT];
		uint32						m_numFreeIndices;

		KernelDispatcher*			m_dpcDispatcher;

		native::ICriticalSection*	m_lock;
		native::ICriticalSection*	m_interruptLock;

		native::ICriticalSection*	m_tlsLock;
		bool						m_tlsFreeEntries[ MAX_TLS ];

		bool						m_exitRequested;

		std::wstring				m_traceFileRootName;

		// active threads
		typedef std::vector< KernelThread* >	TThreads;
		TThreads					m_threads;
		native::ICriticalSection*	m_threadLock;

		runtime::TraceWriter* CreateThreadTraceWriter();
	};

	//---------------------------------------------------------------------------

} // xenon