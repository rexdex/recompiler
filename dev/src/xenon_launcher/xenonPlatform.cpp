MEM_READ
#include "build.h"
#include "xenonPlatform.h"
#include "xenonKernel.h"
#include "xenonFileSystem.h"
#include "xenonGraphics.h"
#include "xenonCPUDevice.h"
#include "xenonInput.h"

#include "../host_core/native.h"
#include "../host_core/runtimeImage.h"

//-----------------------------------------------------------------------------

xenon::Platform GPlatform;

//-----------------------------------------------------------------------------

namespace xenon
{

	Platform::Platform()
		: m_kernel(nullptr)
		, m_fileSys(nullptr)
		, m_graphics(nullptr)
		, m_memory(nullptr)
		, m_interruptTable(nullptr)
		, m_ioTable(nullptr)
		, m_userExitRequested(false)
	{
	}

	Platform::~Platform()
	{
	}

	const char* Platform::GetName() const
	{
		return "Xenon";
	}

	uint32 Platform::GetNumCPUs() const
	{
		return 1;
	}

	runtime::IDeviceCPU* Platform::GetCPU(const uint32 index) const
	{
		if (index == 0)
			return &CPU_PowerPC::GetInstance();

		return nullptr;
	}

	uint32 Platform::GetNumDevices() const
	{
		return 1;
	}

	runtime::IDevice* Platform::GetDevice(const uint32 index) const
	{
		if (index == 0)
			return &CPU_PowerPC::GetInstance();

		return nullptr;
	}

	static void UnimplementedInterrupt(const uint64_t ip, const uint32_t index, const runtime::RegisterBank& regs)
	{
		throw runtime::TrapException(ip, index);
	}

	static const runtime::Symbols* GGlobalSymbols = nullptr;

	static void GlobalMemReadFunc(const uint64_t ip, const uint64_t address, const uint64_t size, void* outPtr)
	{
		auto func = GGlobalSymbols->FindMemoryIOReader(address);
		func(ip, address, size, outPtr);
	}

	static void GlobalMemWriteFunc(const uint64_t ip, const uint64_t address, const uint64_t size, const void* inPtr)
	{
		auto func = GGlobalSymbols->FindMemoryIOWriter(address);
		func(ip, address, size, inPtr);
	}

	static void DefaultMemReadFunc(const uint64_t ip, const uint64_t address, const uint64_t size, void* outPtr)
	{
		switch (size)
		{
			case 8: *(uint64_t*)outPtr = cpu::mem::loadAddr<uint64>((const uint32_t)address); break;
			case 4: *(uint32_t*)outPtr = cpu::mem::loadAddr<uint32>((const uint32_t)address); break;
			case 2: *(uint16_t*)outPtr = cpu::mem::loadAddr<uint16>((const uint32_t)address); break;
			case 1: *(uint8_t*)outPtr = cpu::mem::loadAddr<uint8>((const uint32_t)address); break;
		}
	}

	static void DefaultMemWriteFunc(const uint64_t ip, const uint64_t address, const uint64_t size, const void* inPtr)
	{
		switch (size)
		{
			case 8: cpu::mem::storeAddr<uint64>((const uint32_t)address, *(const uint64_t*)inPtr); break;
			case 4: cpu::mem::storeAddr<uint32>((const uint32_t)address, *(const uint32_t*)inPtr); break;
			case 2: cpu::mem::storeAddr<uint16>((const uint32_t)address, *(const uint16_t*)inPtr); break;
			case 1: cpu::mem::storeAddr<uint8>((const uint32_t)address, *(const uint8_t*)inPtr); break;
		}
	}

	static void GlobalPortReadFunc(const uint64_t ip, const uint16_t portIndex, const uint64_t size, void* outPtr)
	{
		auto func = GGlobalSymbols->FindPortIOReader(portIndex);
		func(ip, portIndex, size, outPtr);
	}

	static void GlobalPortWriteFunc(const uint64_t ip, const uint16_t portIndex, const uint64_t size, const void* inPtr)
	{
		auto func = GGlobalSymbols->FindPortIOWriter(portIndex);
		func(ip, portIndex, size, inPtr);
	}

	bool Platform::Initialize(const native::Systems& nativeSystems, const launcher::Commandline& commandline, runtime::Symbols& symbols)
	{
		// stats
		GLog.Log("Runtime: Initializing Xenon runtime");

		// setup memory
		m_memory = nativeSystems.m_memory;

		// initialize virtual memory
		const uint32 virtualMemorySize = 512 << 20;
		if (!nativeSystems.m_memory->InitializeVirtualMemory(virtualMemorySize))
			return false;

		// initialize physical memory
		const uint32 physicalMemorySize = 256 << 20;
		if (!nativeSystems.m_memory->InitializePhysicalMemory(physicalMemorySize))
			return false;

		// create kernel
		GLog.Log("Runtime: Initializing Xenon kernel");
		m_kernel = new Kernel(nativeSystems, commandline);

		// create file syste
		GLog.Log("Runtime: Initializing Xenon file system");
		m_fileSys = new FileSystem(m_kernel, nativeSystems.m_fileSystem, commandline);

		// create graphics system
		GLog.Log("Runtime: Initializing Xenon graphics");
		m_graphics = new Graphics(*nativeSystems.m_memory, symbols, commandline);

		// create input system
		GLog.Log("Runtime: Initializing Xenon input system");
		m_inputSys = new InputSystem(m_kernel, commandline);

		// initialize config data
		extern void InitializeXboxConfig();
		InitializeXboxConfig();

		// create symbols
		extern void RegisterXboxKernel(runtime::Symbols& symbols);
		RegisterXboxKernel(symbols);
		extern void RegisterXboxUtils(runtime::Symbols& symbols);
		RegisterXboxUtils(symbols);
		extern void RegisterXboxVideo(runtime::Symbols& symbols);
		RegisterXboxVideo(symbols);
		extern void RegisterXboxThreads(runtime::Symbols& symbols);
		RegisterXboxThreads(symbols);
		extern void RegisterXboxFiles(runtime::Symbols& symbols);
		RegisterXboxFiles(symbols);
		extern void RegisterXboxDebug(runtime::Symbols& symbols);
		RegisterXboxDebug(symbols);
		extern void RegisterXboxInput(runtime::Symbols& symbols);
		RegisterXboxInput(symbols);

		// register the process native data

		m_nativeKeDebugMonitorData.Alloc(*m_memory, 256);
		symbols.RegisterSymbol("KeDebugMonitorData", m_nativeKeDebugMonitorData.Data());

		m_nativeKeCertMonitorData.Alloc(*m_memory, 4);
		symbols.RegisterSymbol("KeCertMonitorData", m_nativeKeCertMonitorData.Data());

		// setup basics - XexExecutableModuleHandle
		m_nativeXexExecutableModuleHandle.Alloc(*m_memory, 2048);
		m_nativeXexExecutableModuleHandlePtr.Alloc(*m_memory, 4);
		symbols.RegisterSymbol("XexExecutableModuleHandle", m_nativeXexExecutableModuleHandlePtr.Data());
		m_nativeXexExecutableModuleHandlePtr.Write<uint32>(0, (uint32)m_nativeXexExecutableModuleHandle.Data());
		m_nativeXexExecutableModuleHandle.Write<uint32>(0, 0x4000000);
		m_nativeXexExecutableModuleHandle.Write<uint32>(0x58, 0x80101100); // from actual memory

		// setup basics - XboxHardwareInfo
		m_nativeXboxHardwareInfo.Alloc(*m_memory, 16);
		symbols.RegisterSymbol("XboxHardwareInfo", m_nativeXboxHardwareInfo.Data());
		m_nativeXboxHardwareInfo.Write<uint32>(0, 0x00000000); // flags
		m_nativeXboxHardwareInfo.Write<uint8>(4, 0x6); // num cpus

		// setup basics - ExLoadedCommandLine
		m_nativeExLoadedCommandLine.Alloc(*m_memory, 1024);
		symbols.RegisterSymbol("ExLoadedCommandLine", m_nativeExLoadedCommandLine.Data());
		memcpy((void*)m_nativeExLoadedCommandLine.Data(), "default.xex", strlen("default.xex") + 1);

		// setup basics - XboxKrnlVersion
		m_nativeXboxKrnlVersion.Alloc(*m_memory, 8);
		symbols.RegisterSymbol("XboxKrnlVersion", m_nativeXboxKrnlVersion.Data());
		m_nativeXboxKrnlVersion.Write<uint16>(0, 2);
		m_nativeXboxKrnlVersion.Write<uint16>(2, 0xFFFF);
		m_nativeXboxKrnlVersion.Write<uint16>(4, 0xFFFF);
		m_nativeXboxKrnlVersion.Write<uint16>(6, 0xFFFF);

		// setup basics - KeTimeStampBundle
		m_nativeKeTimeStampBundle.Alloc(*m_memory, 24);
		symbols.RegisterSymbol("KeTimeStampBundle", m_nativeKeTimeStampBundle.Data());

		// process object
		m_nativeExThreadObjectType.Alloc(*m_memory, 4);
		m_nativeExThreadObjectType.Write<uint32>(0, 0xD01BBEEF);
		symbols.RegisterSymbol("ExThreadObjectType", m_nativeExThreadObjectType.Data());

		// initialize interrupt table
		m_interruptTable = new cpu::Interrupts();
		for (uint32_t i = 0; i < cpu::Interrupts::MAX_INTERRUPTS; ++i)
		{
			const auto func = symbols.FindInterruptCallback(i);
			m_interruptTable->INTERRUPTS[i] = func ? func : &UnimplementedInterrupt;
		}

		// set default memory read/write function in case of dynamic access
		symbols.SetDefaultMemoryIO(&DefaultMemReadFunc, &DefaultMemWriteFunc);

		// initialize IO table
		GGlobalSymbols = &symbols; // HACK
		m_ioTable = new cpu::GeneralIO();
		m_ioTable->MEM_READ = &GlobalMemReadFunc;
		m_ioTable->MEM_WRITE = &GlobalMemWriteFunc;
		m_ioTable->PORT_READ = &GlobalPortReadFunc;
		m_ioTable->PORT_WRITE = &GlobalPortWriteFunc;

		GLog.Log("Runtime: Xenon platform initialized");
		return true;
	}

	void Platform::Shutdown()
	{
		delete m_graphics;
		m_graphics = nullptr;

		delete m_fileSys;
		m_fileSys = nullptr;

		delete m_kernel;
		m_kernel = nullptr;

		delete m_inputSys;
		m_inputSys = nullptr;

		delete m_interruptTable;
		m_interruptTable = nullptr;

		delete m_ioTable;
		m_ioTable = nullptr;
	}

	void Platform::RequestUserExit()
	{
		m_userExitRequested = true;
	}

	int Platform::RunImage(const runtime::Image& image)
	{
		// bind code image
		m_kernel->SetCode(image.GetCodeTable());

		// setup main thread
		KernelThreadParams mainThreadParams;
		mainThreadParams.m_entryPoint = (uint32)image.GetEntryPoint();
		mainThreadParams.m_stackSize = 512 * 1024;
		mainThreadParams.m_suspended = false;

		// create main thread
		m_kernel->CreateThread(mainThreadParams);

		// main loop
		while (m_kernel->AdvanceThreads())
		{
			// user exit
			if (m_userExitRequested)
			{
				GLog.Warn("Runtime: User exit requested");
				break;
			}

			// exit
			if ((GetAsyncKeyState(VK_F10) & 0x8000) && (GetAsyncKeyState(VK_F12) & 0x8000))
			{
				GLog.Err("Runtime: External request to quit");
				break;
			}

			// on demand trace dump
			if ((GetAsyncKeyState(VK_F3) & 0x8000))
			{
				GLog.Err("Runtime: Requesting trace dump");
				m_graphics->RequestTraceDump();
			}

			// TODO: update GPU
			Sleep(5);
		}

		// exit
		Sleep(1000);
		return 0;
	}

} // xenon

  //---------------------------------------------------------------------------

namespace runtime
{
	IPlatform* GetSimulatedPlatform()
	{
		return &GPlatform;
	}
}

//---------------------------------------------------------------------------
