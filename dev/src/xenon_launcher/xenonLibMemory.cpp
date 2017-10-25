#include "build.h"
#include "xenonLib.h"  
#include "xenonLibNatives.h"
#include "xenonThread.h"
#include "xenonPlatform.h"
#include "xenonMemory.h"
#include "xenonBindings.h"

namespace xenon
{
	namespace lib
	{

		//-----------------------

		X_STATUS Xbox_NtFreeVirtualMemory(Pointer<uint32> basePtr, Pointer<uint32> sizePtr, uint32 freeType, uint32 protect)
		{
			auto base = (void*)(uint32)*basePtr;
			auto size = sizePtr.IsValid() ? (uint32_t)*sizePtr : 0;

			if (!base)
				return X_STATUS_MEMORY_NOT_ALLOCATED;

			uint32 freedSize = 0;
			if (!GPlatform.GetMemory().VirtualFree(base, size, freeType, freedSize))
				return X_STATUS_UNSUCCESSFUL;

			if (sizePtr.IsValid())
				*sizePtr = freedSize;

			return X_STATUS_SUCCESS;
		}

		X_STATUS Xbox_NtAllocateVirtualMemory(Pointer<uint32> basePtr, Pointer<uint32> sizePtr, uint32 allocType, uint32 protect)
		{
			const auto base = (void*)(uint32)*basePtr;
			const auto size = (uint32_t)*sizePtr;

			// invalid type
			if (allocType & XMEM_RESERVE && (base != NULL))
			{
				GLog.Err("VMEM: Trying to reserve predefined memory region. Not supported.");
				*basePtr = 0;
				return  X_STATUS_NOT_IMPLEMENTED;
			}

			// determine aligned size
			uint32 pageAlignment = 4096;
			if (allocType & XMEM_LARGE_PAGES) pageAlignment = 64 << 10;
			else if (allocType & XMEM_16MB_PAGES) pageAlignment = 16 << 20;

			// align size
			uint32 alignedSize = (size + (pageAlignment - 1)) & ~(pageAlignment - 1);
			if (alignedSize != size)
				GLog.Warn("VMEM: Aligned memory allocation %08X->%08X.", size, alignedSize);

			// allocate memory from system
			void* allocated = GPlatform.GetMemory().VirtualAlloc(base, alignedSize, (uint32)allocType, (uint16_t)protect);
			if (!allocated)
			{
				GLog.Err("VMEM: Allocation failed.");
				*basePtr = 0;
				return  X_STATUS_NO_MEMORY;
			}

			// save allocated memory & size
			*basePtr = (uint32)allocated;
			*sizePtr = (uint32)alignedSize;

			// allocation was OK
			return X_STATUS_SUCCESS;
		}

		//-----------------------

		MemoryAddress Xbox_MmAllocatePhysicalMemoryEx(uint32, uint32 size, uint32 protect, uint32, MemoryAddress address, uint32 align)
		{
			// Calculate page size.
			uint32 pageSize = 4 * 1024;
			if (protect & XMEM_LARGE_PAGES)
				pageSize = 64 * 1024;
			else if (protect & XMEM_16MB_PAGES)
				pageSize = 16 * 1024 * 1024;

			// Round up the region size and alignment to the next page.
			const auto adjustedSize = (size + (pageSize - 1)) & ~(pageSize - 1);
			const auto adjustedAlign = (align + (pageSize - 1)) & ~(pageSize - 1);
			if (adjustedSize != size)
				GLog.Log("XPhysicalAlloc: adjusted size %d->%d (%08X->%08X)", size, adjustedSize, size, adjustedSize);
			if (adjustedAlign != align)
				GLog.Log("XPhysicalAlloc: adjusted align %d->%d (%08X->%08X)", align, adjustedAlign, align, adjustedAlign);

			// allocate physical memory
			return GPlatform.GetMemory().PhysicalAlloc(adjustedSize, adjustedAlign, protect);
		}

		MemoryAddress Xbox_MmGetPhysicalAddress(uint32 addr)
		{
			return MemoryAddress(addr);
		}

		X_STATUS Xbox_MmFreePhysicalMemory(uint32 type, uint32 addr)
		{
			GPlatform.GetMemory().PhysicalFree((void*)addr, 0);
			return 0;
		}

		X_STATUS Xbox_NtQueryVirtualMemory()
		{
			return 0;
		}

		//---------------------------------------------------------------------------

		void Xbox_KeLockL2()
		{
		}

		void Xbox_KeUnlockL2()
		{
		}

		uint64 Xbox_MmQueryAddressProtect(MemoryAddress address)
		{
			return GPlatform.GetMemory().GetVirtualMemoryFlags(address.GetNativePointer(), 0);
		}

		void Xbox_MmSetAddressProtect(MemoryAddress address, uint32 size, uint64 protectFlags)
		{
			GPlatform.GetMemory().SetVirtualMemoryFlags(address.GetNativePointer(), size, protectFlags);
		}

		uint32 Xbox_MmQueryAllocationSize(MemoryAddress address)
		{
			return GPlatform.GetMemory().GetVirtualMemoryAllocationSize(address.GetNativePointer());
		}

		//---------------------------------------------------------------------------

		void RegisterXboxMemory(runtime::Symbols& symbols)
		{
			REGISTER(MmAllocatePhysicalMemoryEx);
			REGISTER(MmGetPhysicalAddress);
			REGISTER(MmFreePhysicalMemory);
			REGISTER(NtAllocateVirtualMemory);
			REGISTER(NtFreeVirtualMemory);

			REGISTER(NtQueryVirtualMemory);

			REGISTER(KeLockL2);
			REGISTER(KeUnlockL2);
			REGISTER(MmQueryAddressProtect);
			REGISTER(MmSetAddressProtect);
			REGISTER(MmQueryAllocationSize);
		}

		//---------------------------------------------------------------------------

	} // lib
} // xenon