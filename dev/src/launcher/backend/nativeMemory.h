#pragma once

namespace native
{
	/// native memory allocator
	class LAUNCHER_API IMemory
	{
	public:
		virtual ~IMemory();

		enum EAllocFlags
		{
			eAlloc_Reserve = 1 << 0,
			eAlloc_Commit = 1 << 1,
			eAlloc_Decomit = 1 << 2,
			eAlloc_Release = 1 << 3,
			eAlloc_Top = 1 << 4,
			eAlloc_Page64K = 1 << 5,
			eAlloc_Page16MB = 1 << 6,
		};

		enum EProtectFlags
		{
			eFlags_ReadWrite = 1 << 0,
			eFlags_ReadOnly = 1 << 1,
			eFlags_NotCached = 1 << 2,
			eFlags_WriteCombine = 1 << 3,
		};

		// initialize virtual memory range
		virtual bool InitializeVirtualMemory(const uint32 totalVirtualMemoryAvaiable) = 0;

		// initialize physical memory range
		virtual bool InitializePhysicalMemory(const uint32 totalPhysicalMemoryAvaiable) = 0;

		// is this a valid virtual memory range ?
		virtual bool IsVirtualMemory(void* base, const uint32 size) const = 0;

		// is this virtual memory reserved ?
		virtual bool IsVirtualMemoryReserved(void* base, const uint32 size) const = 0;

		// is this virtual memory comitted ?
		virtual bool IsVirtualMemoryComitted(void* base, const uint32 size) const = 0;

		// get protection flags for this memory
		virtual uint32 GetVirtualMemoryProtectFlags(void* base) const = 0;

		// get size of virtual memory allocation, zero if not allocated
		virtual uint32 GetVirtualMemoryAllocationSize(void* base) const = 0;

		// set protection flag for range
		virtual void VirtualProtect(void* base, const uint32 size, const uint32 flags) = 0;

		// allocate virtual memory range
		virtual void* VirtualAlloc(void* base, const uint32 size, const uint32 allocFlags, const uint32 protectFlags) = 0;

		// release virtual memory range (partial regions are supported, committed memory will be freed)
		virtual bool VirtualFree(void* base, const uint32 size, const uint32 allocFlags, uint32& outFreedSize) = 0;

		// allocate physical memory range
		virtual void* PhysicalAlloc(const uint32 size, const uint32 alignment, const uint32 protectFlags) = 0;

		// free physical memory range
		virtual void PhysicalFree(void* base, const uint32 size) = 0;

		// translate local physical memory address to absolute address
		virtual uint32 TranslatePhysicalAddress(const uint32 localAddress) const = 0;
	};

} // native