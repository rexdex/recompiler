#include "build.h"
#include "xenonLibNatives.h" 
#include "xenonLib.h"  
#include "xenonPlatform.h"
#include "xenonKernel.h"
#include "xenonMemory.h"
#include "xenonBindings.h"

namespace xenon
{
	namespace lib
	{

		int32_t Xbox_XAudioSubmitRenderDriverFrame()
		{
			return 0;
		}

		uint32 Xbox_XAudioUnregisterRenderDriverClient(uint32 callbackAddress, Pointer<uint32> outputPtr)
		{
			static uint32_t index = 0;
			++index;

			const uint32_t flags = 0x41550000;
			*outputPtr = flags | index;
			return 0;
		}

		void Xbox_XAudioRegisterRenderDriverClient()
		{
		}

		uint32 Xbox_XAudioGetSpeakerConfig(Pointer<uint32> configPtr)
		{
			*configPtr = 0x00010001;
			return 0;
		}

		uint32 Xbox_XAudioGetVoiceCategoryVolume(Pointer<float> volumePtr)
		{
			*volumePtr = 1.0f;
			return 0;
		}

		uint64 __fastcall Xbox_XAudioGetVoiceCategoryVolume(uint64 ip, cpu::CpuRegs& regs)
		{
			auto memPtrAddress = (uint32)regs.R4;
			cpu::mem::storeAddr<float>(memPtrAddress, 1.0f);
			regs.R3 = 0;
			return regs.LR;
		}

		//---------------------------------------------------------------------------

		void RegisterXboxAudio(runtime::Symbols& symbols)
		{
			REGISTER(XAudioSubmitRenderDriverFrame);
			REGISTER(XAudioUnregisterRenderDriverClient);
			REGISTER(XAudioRegisterRenderDriverClient);
			REGISTER(XAudioGetVoiceCategoryVolume);
			REGISTER(XAudioGetSpeakerConfig);
		}

		//---------------------------------------------------------------------------

	} // lib
} // xenon