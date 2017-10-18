#include "build.h"
#include "xenonLibNatives.h" 
#include "xenonLibUtils.h" 
#include "xenonPlatform.h"
#include "xenonKernel.h"
#include "xenonMemory.h"

uint64 __fastcall Xbox_XAudioSubmitRenderDriverFrame(uint64 ip, cpu::CpuRegs& regs)
{
	RETURN_DEFAULT();
}

uint64 __fastcall Xbox_XAudioUnregisterRenderDriverClient(uint64 ip, cpu::CpuRegs& regs)
{
	auto callbackPtr = utils::MemPtr32<uint32>(regs.R3);
	auto outputPtr = utils::MemPtr32<uint32>(regs.R4);

	static uint32_t index = 0;
	++index;

	const uint32_t flags = 0x41550000;
	outputPtr.Set(flags | index);
	RETURN_ARG(0);
}

uint64 __fastcall Xbox_XAudioRegisterRenderDriverClient(uint64 ip, cpu::CpuRegs& regs)
{
	RETURN_DEFAULT();
}

uint64 __fastcall Xbox_XAudioGetSpeakerConfig(uint64 ip, cpu::CpuRegs& regs)
{
	auto memPtr = utils::MemPtr32<uint32>(regs.R3);
	//memPtr.Set(1);
	memPtr.Set(0x00010001);
	RETURN_ARG(0);
}

uint64 __fastcall Xbox_XAudioGetVoiceCategoryVolume(uint64 ip, cpu::CpuRegs& regs)
{
	auto memPtr = utils::MemPtr32<float>(regs.R4);
	memPtr.Set(1.0f); // full volume
	RETURN_ARG(0);
}

//---------------------------------------------------------------------------

void RegisterXboxAudio(runtime::Symbols& symbols)
{
#define REGISTER(x) symbols.RegisterFunction(#x, (runtime::TBlockFunc) &Xbox_##x);
	REGISTER(XAudioSubmitRenderDriverFrame)
	REGISTER(XAudioUnregisterRenderDriverClient)
	REGISTER(XAudioRegisterRenderDriverClient);
	REGISTER(XAudioGetVoiceCategoryVolume);
	REGISTER(XAudioGetSpeakerConfig);
#undef REGISTER
}

//---------------------------------------------------------------------------
