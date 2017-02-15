#pragma once

#include "xenonGPU.h"
#include "..\..\..\launcher\backend\launcherCommandline.h"

namespace xenon
{

	/// Graphics system wrapper (CPU side of rendering)
	class Graphics
	{
	public:
		Graphics(native::IMemory& memory, runtime::Symbols& symbols, const launcher::Commandline& commandline);
		~Graphics();

		// setup the ring buffer, this initializes the internal GPU processor
		void InitializeRingBuffer(const void* ptr, const uint32 numPages);

		// set internal interrupt callback
		void SetInterruptCallbackAddr(const uint32 addr, const uint32 userData);

		// command buffer stuff
		void EnableReadPointerWriteBack(const uint32 ptr, const uint32 blockSize);

		// request a trace dump of the whole GPU frame
		void RequestTraceDump();

	private:
		// mapped global stuff
		xnative::XenonNativeData	m_nativeVdGlobalDevice;
		xnative::XenonNativeData	m_nativeVdGlobalXamDevice;
		xnative::XenonNativeData	m_nativeVdGpuClockInMHz;
		xnative::XenonNativeData	m_nativeVdHSIOCalibrationLock;

		// GPU "chip" implementation
		CXenonGPU*	m_gpu;
		bool		m_gpuInitialized;

		// IO to/from GPU
		static void WriteGPUWord(uint64 addr, const uint32 size, const void* inPtr);
		static void ReadGPUWord(uint64 addr, const uint32 size, void* outPtr);
	};

} // xenon