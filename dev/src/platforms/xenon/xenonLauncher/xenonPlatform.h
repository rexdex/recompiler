#pragma once

#include "..\..\..\launcher\backend\runtimePlatform.h"

//---------------------------------------------------------------------------

namespace xenon
{
	class Kernel;
	class FileSystem;
	class Graphics;

	/// Top level wrapper
	class Platform : public runtime::IPlatform
	{
	public:
		inline Kernel& GetKernel() { return *m_kernel; }

		inline FileSystem& GetFileSystem() { return *m_fileSys; }

		inline Graphics& GetGraphics() { return *m_graphics; }

		inline native::IMemory& GetMemory() { return *m_memory;  }

	public:
		Platform();
		virtual ~Platform();

		// IPlatform
		virtual const char* GetName() const override final;
		virtual uint32 GetNumCPUs() const override final;
		virtual runtime::IDeviceCPU* GetCPU(const uint32 index) const override final;
		virtual uint32 GetNumDevices() const override final;
		virtual runtime::IDevice* GetDevice(const uint32 index) const override final;

		//---

		virtual bool Initialize(const native::Systems& nativeSystems, const launcher::Commandline& commandline, runtime::Symbols& symbols) override final;
		virtual void Shutdown() override final;

		//---

		virtual int RunImage(const runtime::Image& image) override final;

		//---

		void RequestUserExit();

	private:
		// subsystems
		Kernel*					m_kernel;		// kernel object container
		FileSystem*				m_fileSys;		// file system
		Graphics*				m_graphics;		// graphics subsystem

		// native
		native::IMemory*		m_memory;		// native memory system

		// some runtime data
		xnative::XenonNativeData	m_nativeXexExecutableModuleHandle;
		xnative::XenonNativeData	m_nativeKeDebugMonitorData;
		xnative::XenonNativeData	m_nativeKeCertMonitorData;
		xnative::XenonNativeData	m_nativeXboxHardwareInfo;
		xnative::XenonNativeData	m_nativeXexExecutableModuleHandlePtr;
		xnative::XenonNativeData	m_nativeExLoadedCommandLine;
		xnative::XenonNativeData	m_nativeXboxKrnlVersion;
		xnative::XenonNativeData	m_nativeKeTimeStampBundle;
		xnative::XenonNativeData	m_nativeExThreadObjectType;

		// external exit
		bool m_userExitRequested;
	};

} // xenon

//---------------------------------------------------------------------------

extern xenon::Platform GPlatform;

//---------------------------------------------------------------------------
