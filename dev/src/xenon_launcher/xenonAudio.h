#pragma once

#include "../host_core/blockAllocator.h"
#include "../host_core/bitset.h"

namespace xenon
{

	// emulated Audio device
	class Audio
	{
	public:
		Audio(runtime::Symbols& symbols, const launcher::Commandline& commandline);
		~Audio();

	private:
		// IO to/from the audio device
		static void WriteAudioWord(const uint64_t ip, const uint64_t addr, const uint32_t size, const void* inPtr);
		static void ReadAudioWord(const uint64_t ip, const uint64_t addr, const uint32_t size, void* outPtr);
	};

} // xenon