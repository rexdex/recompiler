#pragma once

namespace native
{
	/// native memory allocator
	class LAUNCHER_API IMemory
	{
	public:
		virtual ~IMemory();

		/// allocate and commit memory block
		virtual void* AllocVirtualMemory(const uint64_t prefferedBaseAddres, const uint64_t size) = 0;

		/// free virtual memory
		virtual void FreeVirtualMemory(void* address, const uint64_t size) = 0;
	};

} // native