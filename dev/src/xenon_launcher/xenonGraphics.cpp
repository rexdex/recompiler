#include "build.h"
#include "xenonPlatform.h"
#include "xenonGraphics.h"
#include "xenonGPU.h"

namespace xenon
{

	//----

	Graphics::Graphics(native::IMemory& memory, runtime::Symbols& symbols, const launcher::Commandline& commandline)
		: m_gpuInitialized(0)
	{
		// bind symbols - VdGlobalDevice
		m_nativeVdGlobalDevice.Alloc(memory, 4);
		m_nativeVdGlobalDevice.Write<uint32>(0, 0);
		symbols.RegisterSymbol("VdGlobalDevice", m_nativeVdGlobalDevice.Data());

		// bind symbols - VdGlobalXamDevice
		m_nativeVdGlobalXamDevice.Alloc(memory, 4);
		m_nativeVdGlobalXamDevice.Write<uint32>(0, 0);
		symbols.RegisterSymbol("VdGlobalXamDevice", m_nativeVdGlobalXamDevice.Data());

		// bind symbols - VdGlobalXamDevice
		m_nativeVdGpuClockInMHz.Alloc(memory, 4);
		m_nativeVdGpuClockInMHz.Write<uint32>(0, 500);
		symbols.RegisterSymbol("VdGpuClockInMHz", m_nativeVdGpuClockInMHz.Data());

		// bind symbols - VdGlobalXamDevice
		m_nativeVdHSIOCalibrationLock.Alloc(memory, 28);
		symbols.RegisterSymbol("VdHSIOCalibrationLock", m_nativeVdHSIOCalibrationLock.Data());
		extern void RtlInitializeCriticalSection(xnative::XCRITICAL_SECTION* rtlCS);
		RtlInitializeCriticalSection((xnative::XCRITICAL_SECTION*) m_nativeVdHSIOCalibrationLock.Data());

		// bind the GPU register
		symbols.RegisterMemoryIO(0x7FC80714, (runtime::TGlobalMemReadFunc) &Graphics::ReadGPUWord, (runtime::TGlobalMemWriteFunc) &Graphics::WriteGPUWord);
		symbols.RegisterMemoryIO(0x7FC8112C, (runtime::TGlobalMemReadFunc) &Graphics::ReadGPUWord, (runtime::TGlobalMemWriteFunc) &Graphics::WriteGPUWord);
		symbols.RegisterMemoryIO(0x7FC86544, (runtime::TGlobalMemReadFunc) &Graphics::ReadGPUWord, (runtime::TGlobalMemWriteFunc) &Graphics::WriteGPUWord);

		// crete the gpu interface
		m_gpu = new CXenonGPU(commandline);
	}

	Graphics::~Graphics()
	{
		if (m_gpuInitialized)
		{
			m_gpu->Close();
			m_gpuInitialized = false;
		}

		delete m_gpu;
		m_gpu = nullptr;
	}

	void Graphics::InitializeRingBuffer(const void* ptr, const uint32 numPages)
	{
		if (m_gpuInitialized)
		{
			GLog.Err("GPU: Ring buffer already initialized. Why is it called twice ?");
			return;
		}

		m_gpu->Initialize(ptr, numPages);
		m_gpuInitialized = true;
	}

	void Graphics::SetInterruptCallbackAddr(const uint32 addr, const uint32 userData)
	{
		m_gpu->SetInterruptCallbackAddr(addr, userData);
	}

	void Graphics::EnableReadPointerWriteBack(const uint32 ptr, const uint32 blockSize)
	{
		m_gpu->EnableReadPointerWriteBack(ptr, blockSize);
	}

	void Graphics::WriteGPUWord(const uint64_t ip, const uint64_t addr, const uint32_t size, const void* inPtr)
	{
		if (size == 4)
		{
			const uint32 val = *(const uint32*)inPtr;
			GPlatform.GetGraphics().m_gpu->WriteWord(val, (uint32)addr);
		}
	}

	void Graphics::ReadGPUWord(const uint64_t ip, const uint64_t addr, const uint32_t size, void* outPtr)
	{
		if (size == 4)
		{
			uint64 val = 0;
			GPlatform.GetGraphics().m_gpu->ReadWord(&val, (uint32)addr);
			*(uint32*)outPtr = (uint32)val;
		}
	}

	void Graphics::RequestTraceDump()
	{
		m_gpu->RequestTraceDump();
	}

} // xenon