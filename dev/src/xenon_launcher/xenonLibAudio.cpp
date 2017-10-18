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
	RETURN_DEFAULT();
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

//---------------------------------------------------------------------------

void RegisterXboxAudio(runtime::Symbols& symbols)
{
#define REGISTER(x) symbols.RegisterFunction(#x, (runtime::TBlockFunc) &Xbox_##x);
	REGISTER(XAudioSubmitRenderDriverFrame)
	REGISTER(XAudioUnregisterRenderDriverClient)
	REGISTER(XAudioRegisterRenderDriverClient);
	REGISTER(XAudioGetSpeakerConfig);
#undef REGISTER
}

//---------------------------------------------------------------------------
