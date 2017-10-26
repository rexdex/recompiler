#include "build.h"
#include "winKernel.h"

namespace win
{
	//----

	static native::WaitResult ToWaitResult( const DWORD ret )
	{
		switch (ret)
		{
			case WAIT_OBJECT_0: return native::WaitResult::Success;
			case WAIT_IO_COMPLETION: return native::WaitResult::IOCompletion;
			case WAIT_TIMEOUT: return native::WaitResult::Timeout;
			case WAIT_ABANDONED: return native::WaitResult::Abandoned;
			case WAIT_FAILED: return native::WaitResult::Failed;
		}

		DEBUG_CHECK(!"Unknown return mode");
		return native::WaitResult::Failed;
	}

	//----

	CriticalSection::CriticalSection()
	{
		InitializeCriticalSection(&m_lock);
	}

	CriticalSection::~CriticalSection()
	{
		DeleteCriticalSection(&m_lock);
	}

	void CriticalSection::Acquire()
	{
		EnterCriticalSection(&m_lock);
	}

	void CriticalSection::Release()
	{
		LeaveCriticalSection(&m_lock);
	}

	//----

	Event::Event(bool initialState, bool manualReset)
	{
		m_hEvent = CreateEventA(NULL, manualReset, initialState, NULL);
		DEBUG_CHECK(m_hEvent != NULL);
	}

	Event::~Event()
	{
		CloseHandle(m_hEvent);
		m_hEvent = NULL;
	}

	bool Event::Set()
	{
		return !!SetEvent(m_hEvent);
	}

	bool Event::Pulse()
	{
		return !!PulseEvent(m_hEvent);
	}

	bool Event::Reset()
	{
		return !!ResetEvent(m_hEvent);
	}

	native::WaitResult Event::Wait(const uint32 timeout, const bool alertable)
	{
		auto ret = WaitForSingleObjectEx(m_hEvent, timeout, alertable);
		return ToWaitResult(ret);
	}

	void* Event::GetNativeHandle() const
	{
		return m_hEvent;
	}

	//----

	Semaphore::Semaphore(uint32 initalCount, uint32 maxCount)
	{
		m_hSemaphore = CreateSemaphore(NULL, initalCount, maxCount, NULL);
		DEBUG_CHECK(m_hSemaphore != NULL);
	}

	Semaphore::~Semaphore()
	{
		CloseHandle(m_hSemaphore);
		m_hSemaphore = NULL;
	}

	uint32 Semaphore::Release(const uint32 count)
	{
		LONG prevCount = 0;
		ReleaseSemaphore(m_hSemaphore, count, &prevCount);
		return prevCount;
	}

	native::WaitResult Semaphore::Wait(const uint32 timeout, const bool alertable)
	{
		auto ret = WaitForSingleObjectEx(m_hSemaphore, timeout, alertable);
		return ToWaitResult(ret);
	}

	void* Semaphore::GetNativeHandle() const
	{
		return m_hSemaphore;
	}

	//----

	Timer::Timer(bool manualReset)
	{	
		m_hTimer = CreateWaitableTimer(NULL, manualReset, NULL);
	}

	Timer::~Timer()
	{
		CloseHandle(m_hTimer);
		m_hTimer = NULL;
	}

	void Timer::WaitOrTimerCallback(PVOID param, DWORD dwTimerLowValue, DWORD dwTimerHighValue)
	{
		auto* timer = (Timer*)param;

		timer->m_callbackLock.lock();
		auto callback = timer->m_callback;
		timer->m_callbackLock.unlock();

		callback();
	}

	bool Timer::SetOnce(uint64_t nanosecondsToWait, const TCallbackFunc& func)
	{
		std::lock_guard<std::mutex> lock(m_callbackLock);

		m_callback = func;

		LARGE_INTEGER timeout;
		timeout.QuadPart = nanosecondsToWait / 100;
		return SetWaitableTimer(m_hTimer, &timeout, 0, &WaitOrTimerCallback, this, FALSE) != FALSE;
	}

	bool Timer::SetRepeating(uint64_t nanosecondsToWait, uint64_t milisecondPeriod, const TCallbackFunc& func)
	{
		std::lock_guard<std::mutex> lock(m_callbackLock);

		m_callback = func;

		LARGE_INTEGER timeout;
		timeout.QuadPart = nanosecondsToWait / 100;
		return SetWaitableTimer(m_hTimer, &timeout, (LONG)milisecondPeriod, &WaitOrTimerCallback, this, FALSE) != FALSE;
	}

	bool Timer::Cancel()
	{
		return CancelWaitableTimer(m_hTimer) != FALSE;
	}

	native::WaitResult Timer::Wait(const uint32 timeout, const bool alertable)
	{
		auto ret = WaitForSingleObjectEx(m_hTimer, timeout, alertable);
		return ToWaitResult(ret);
	}

	void* Timer::GetNativeHandle() const
	{
		return m_hTimer;
	}

	//----

	Mutant::Mutant(bool initialOwner)
	{
		m_hMutant = CreateMutex(NULL, initialOwner, NULL);
	}

	Mutant::~Mutant()
	{
		CloseHandle(m_hMutant);
		m_hMutant = NULL;
	}

	bool Mutant::Release()
	{
		return ReleaseMutex(m_hMutant) != FALSE;
	}

	void* Mutant::GetNativeHandle() const
	{
		return m_hMutant;
	}

	native::WaitResult Mutant::Wait(const uint32 timeout, const bool alertable)
	{
		auto ret = WaitForSingleObjectEx(m_hMutant, timeout, alertable);
		return ToWaitResult(ret);
	}

	//----

	Thread::Thread(native::IRunnable* runnable)
		: m_ThreadID(0)
		, m_runnable(runnable)
	{
		m_hThread = ::CreateThread(NULL, 16 << 10, &ThreadProc, this, CREATE_SUSPENDED, (LPDWORD)&m_ThreadID);
	}

	Thread::~Thread()
	{
		if (WAIT_OBJECT_0 != WaitForSingleObjectEx(m_hThread, 100, TRUE))
			TerminateThread(m_hThread, 0);

		m_hThread = NULL;
	}

	void* Thread::GetNativeHandle() const
	{
		return m_hThread;
	}

	native::WaitResult Thread::Wait(const uint32 timeout, const bool alertable)
	{
		auto ret = WaitForSingleObjectEx(m_hThread, timeout, alertable);
		return ToWaitResult(ret);
	}

	bool Thread::Pause()
	{
		return 0 == SuspendThread(m_hThread);
	}

	bool Thread::Resume()
	{
		return 0 == ResumeThread(m_hThread);
	}

	int32 Thread::SetPriority(const native::ThreadPriority priority)
	{
		return 0;
	}

	uint32 Thread::SetAffinity(const uint32 affinity)
	{
		return 0;
	}

	native::WaitResult Thread::Sleep(const uint32 ms, const bool alertable)
	{
		auto ret = SleepEx(ms, alertable ? TRUE : FALSE);
		return ToWaitResult(ret);
	}

	void Thread::DeliverAPCs(void* pData)
	{
		auto* thread = static_cast<Thread*>(pData);
		thread->m_runnable->ProcessAPC();
	}

	DWORD WINAPI Thread::ThreadProc(LPVOID lpParameter)
	{
		auto* thread = static_cast<Thread*>(lpParameter);
		return thread->m_runnable->Run(*thread);
	}

	void Thread::ScheduleAPC()
	{
		QueueUserAPC((PAPCFUNC)&DeliverAPCs, m_hThread, (ULONG_PTR)this);
	}


	//----

	//----

	Kernel::Kernel()
	{}

	Kernel::~Kernel()
	{}

	native::ICriticalSection* Kernel::CreateCriticalSection()
	{
		return new CriticalSection();
	}

	native::IEvent* Kernel::CreateEvent(const bool manualReset, const bool initialState)
	{
		return new Event(initialState, manualReset);
	}

	native::ISemaphore* Kernel::CreateSemaphore(const uint32 initialCount, const uint32 maxCount)
	{
		return new Semaphore(initialCount, maxCount);
	}

	native::IThread* Kernel::CreateThread(native::IRunnable* runnable)
	{
		return new Thread(runnable);
	}

	native::ITimer* Kernel::CreateManualResetTimer()
	{
		return new Timer(true);
	}

	native::ITimer* Kernel::CreateSynchronizationTimer()
	{
		return new Timer(false);
	}

	native::WaitResult Kernel::WaitMultiple(const std::vector<native::IKernelObject*>& objects, const bool waitAll, const uint32 timeOut, const bool alertable)
	{
		// collect the handles
		std::vector<HANDLE> handles;
		for (const auto* kernelObject : objects)
		{
			const auto handle = (HANDLE)kernelObject->GetNativeHandle();
			DEBUG_CHECK(handle != nullptr);
			handles.push_back(handle);
		}

		// wait
		const auto ret = WaitForMultipleObjectsEx((DWORD)handles.size(), handles.data(), waitAll, timeOut, alertable);
		return ToWaitResult(ret);
	}

	native::WaitResult Kernel::SignalAndWait(native::IKernelObject* nativeSignalObject, native::IKernelObject* nativeWaitObject, const uint32 timeOut, const bool alertable)
	{
		HANDLE hSignal = nativeSignalObject->GetNativeHandle();
		HANDLE hWait = nativeWaitObject->GetNativeHandle();

		const auto ret = SignalObjectAndWait(hSignal, hWait, timeOut, alertable);
		return ToWaitResult(ret);
	}

	native::IMutant* Kernel::CreateMutant(bool initiallyOpened)
	{
		return new Mutant(initiallyOpened);
	}

	//---

} // win
