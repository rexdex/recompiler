#pragma once

#include "../host_core/nativeMemory.h"

namespace win
{
	/// memory handler
	class Memory : public native::IMemory
	{
	public:
		Memory();
		~Memory();

		virtual void* AllocVirtualMemory(const uint64_t prefferedBaseAddres, const uint64_t size) override final;
		virtual void FreeVirtualMemory(void* address, const uint64_t size) override final;
	};

} // win