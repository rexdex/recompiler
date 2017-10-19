#include "build.h"
#include "xenonAudio.h"

namespace xenon
{

	Audio::Audio(runtime::Symbols& symbols, const launcher::Commandline& commandline)
	{
		// register memory locations for memory mapped access
		symbols.RegisterMemoryIO(0x7FEA1804, (runtime::TGlobalMemReadFunc)&ReadAudioWord, (runtime::TGlobalMemWriteFunc)&WriteAudioWord);
	}

	Audio::~Audio()
	{
	}

	void Audio::WriteAudioWord(const uint64_t ip, const uint64_t addr, const uint32_t size, const void* inPtr)
	{
		const auto data = *(const uint32*)inPtr;
		GLog.Log("Audio: MemWrite 0x%08X", data);
	}

	void Audio::ReadAudioWord(const uint64_t ip, const uint64_t addr, const uint32_t size, void* outPtr)
	{
		GLog.Log("Audio: MemRead");
	}

} // xenon