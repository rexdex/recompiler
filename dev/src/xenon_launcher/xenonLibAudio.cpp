#include "build.h"
#include "xenonLibNatives.h" 
#include "xenonLib.h"  
#include "xenonPlatform.h"
#include "xenonKernel.h"
#include "xenonMemory.h"

namespace xenon
{
	namespace lib
	{

		uint64 __fastcall Xbox_XAudioSubmitRenderDriverFrame(uint64 ip, cpu::CpuRegs& regs)
		{
			RETURN_DEFAULT();
		}

		uint64 __fastcall Xbox_XAudioUnregisterRenderDriverClient(uint64 ip, cpu::CpuRegs& regs)
		{
			auto callbackPtr = Pointer<uint32>(regs.R3);
			auto outputPtr = Pointer<uint32>(regs.R4);

			static uint32_t index = 0;
			++index;

			const uint32_t flags = 0x41550000;
			*outputPtr = flags | index;
			RETURN_ARG(0);
		}

		uint64 __fastcall Xbox_XAudioRegisterRenderDriverClient(uint64 ip, cpu::CpuRegs& regs)
		{
			RETURN_DEFAULT();
		}

		uint64 __fastcall Xbox_XAudioGetSpeakerConfig(uint64 ip, cpu::CpuRegs& regs)
		{
			auto memPtr = Pointer<uint32>(regs.R3);
			//memPtr.Set(1);
			*memPtr = 0x00010001;
			RETURN_ARG(0);
		}

		uint64 __fastcall Xbox_XAudioGetVoiceCategoryVolume(uint64 ip, cpu::CpuRegs& regs)
		{
			auto memPtr = Pointer<float>(regs.R4);
			*memPtr = 1.0f;
			RETURN_ARG(0);
		}

		//---------------------------------------------------------------------------

		void RegisterXboxAudio(runtime::Symbols& symbols)
		{
			REGISTER_RAW(XAudioSubmitRenderDriverFrame);
			REGISTER_RAW(XAudioUnregisterRenderDriverClient);
			REGISTER_RAW(XAudioRegisterRenderDriverClient);
			REGISTER_RAW(XAudioGetVoiceCategoryVolume);
			REGISTER_RAW(XAudioGetSpeakerConfig);
		}

		//---------------------------------------------------------------------------

	} // lib
} // xenon