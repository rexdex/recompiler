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

		//---------------------------------------------------------------------------

		uint64 __fastcall Xbox_KeEnableFpuExceptions(uint64 ip, cpu::CpuRegs& regs)
		{
			const uint32 enabled = (const uint32)regs.R3;
			GLog.Log("KeEnableFpuExceptions(%d)", enabled);
			RETURN_ARG(0);
		}

		uint64 __fastcall Xbox_KeQueryPerformanceFrequency(uint64 ip, cpu::CpuRegs& regs)
		{
			__int64 ret = 0;
			QueryPerformanceFrequency((LARGE_INTEGER*)&ret);
			//ret *= 10;
			RETURN_ARG(ret); // english
		}

		uint64 __fastcall Xbox_XGetLanguage(uint64 ip, cpu::CpuRegs& regs)
		{
			GLog.Log("XGetLanguage: english returned");
			RETURN_ARG(1); // english
		}

		uint64 __fastcall Xbox_XGetAVPack(uint64 ip, cpu::CpuRegs& regs)
		{
			GLog.Log("XGetAVPack: returning USA/Canada");
			RETURN_ARG(0x010000);
		}

		uint64 __fastcall Xbox_XamLoaderTerminateTitle(uint64 ip, cpu::CpuRegs& regs)
		{
			GLog.Log("XamLoaderTerminateTitle: called");
			throw runtime::TerminateProcessException(ip, 0);
			RETURN();
		}

		uint64 __fastcall Xbox_XamShowMessageBoxUIEx(uint64 ip, cpu::CpuRegs& regs)
		{
			RETURN_DEFAULT();
		}

		// BOOL XexCheckExecutablePrivilege(DWORD priviledge);
		uint64 __fastcall Xbox_XexCheckExecutablePrivilege(uint64 ip, cpu::CpuRegs& regs)
		{
			const uint32 mask = (const uint32)regs.R3;
			GLog.Log("Privledge check: 0x%08X", mask);
			RETURN_ARG(1);
		}

		uint64 __fastcall Xbox___C_specific_handler(uint64 ip, cpu::CpuRegs& regs)
		{
			RETURN_DEFAULT();
		}

		uint64 __fastcall Xbox_HalReturnToFirmware(uint64 ip, cpu::CpuRegs& regs)
		{
			RETURN_DEFAULT();
		}
		
		uint64 __fastcall Xbox_NtAllocateVirtualMemory(uint64 ip, cpu::CpuRegs& regs)
		{
			auto basePtr = Pointer<uint32>(regs.R3);
			auto base = (void*)(uint32)*basePtr;

			auto sizePtr = Pointer<uint32>(regs.R4);
			auto allocType = regs.R5;
			auto protect = regs.R6;

			const auto size = *sizePtr;

			// invalid type
			if (allocType & XMEM_RESERVE && (base != NULL))
			{
				GLog.Err("VMEM: Trying to reserve predefined memory region. Not supported.");
				regs.R3 = ((DWORD)0xC0000005L);
				*basePtr = 0;
				RETURN();
			}

			// determine aligned size
			uint32 pageAlignment = 4096;
			if (allocType & XMEM_LARGE_PAGES) pageAlignment = 64 << 10;
			else if (allocType & XMEM_16MB_PAGES) pageAlignment = 16 << 20;

			// align size
			uint32 alignedSize = (size + (pageAlignment - 1)) & ~(pageAlignment - 1);
			if (alignedSize != size)
			{
				GLog.Warn("VMEM: Aligned memory allocation %08X->%08X.", size, alignedSize);
			}

			// allocate memory from system
			void* allocated = GPlatform.GetMemory().VirtualAlloc(base, alignedSize, (uint32)allocType, (uint16_t)protect);
			if (!allocated)
			{
				GLog.Err("VMEM: Allocation failed.");
				regs.R3 = ((DWORD)0xC0000005L);
				*basePtr = 0;
				RETURN();
			}

			// save allocated memory & size
			*basePtr = (uint32)allocated;
			*sizePtr = (uint32)alignedSize;

			// allocation was OK
			regs.R3 = 0;
			RETURN();
		}

		uint64 __fastcall Xbox_KeBugCheckEx(uint64 ip, cpu::CpuRegs& regs)
		{
			GLog.Err("BugCheck: Encountered unrecoverable system exception: %d (%016LLXX, %016LLXX, %016LLXX, %016LLXX)",
				(uint32)(regs.R3), regs.R4, regs.R5, regs.R6, regs.R7);
			RETURN();
		}

		uint32 GCurrentProcessType = X_PROCTYPE_USER;

		uint64 __fastcall Xbox_KeGetCurrentProcessType(uint64 ip, cpu::CpuRegs& regs)
		{
			RETURN_ARG(GCurrentProcessType); // ?
		}

		uint64 __fastcall Xbox_KeSetCurrentProcessType(uint64 ip, cpu::CpuRegs& regs)
		{
			const uint32 type = (const uint32)regs.R3;
			DEBUG_CHECK(type >= 0 && type <= 2);
			GCurrentProcessType = type;
			RETURN();
		}

		uint64 __fastcall Xbox_NtFreeVirtualMemory(uint64 ip, cpu::CpuRegs& regs)
		{
			auto basePtr = Pointer<uint32>(regs.R3);
			auto base = (void*)(uint32)*basePtr;

			auto sizePtr = Pointer<uint32>(regs.R4);
			auto size = sizePtr.IsValid() ? *sizePtr : 0;

			const auto freeType = regs.R5;
			const auto protect = regs.R6;

			if (!base)
				return X_STATUS_MEMORY_NOT_ALLOCATED;

			uint32 freedSize = 0;
			if (!GPlatform.GetMemory().VirtualFree(base, size, freeType, freedSize))
				return X_STATUS_UNSUCCESSFUL;

			if (sizePtr.IsValid())
				*sizePtr = freedSize;

			RETURN_ARG(0);
		}

		uint64 __fastcall Xbox_NtQueryVirtualMemory(uint64 ip, cpu::CpuRegs& regs)
		{
			RETURN_DEFAULT();
		}


		uint64 __fastcall Xbox_KeBugCheck(uint64 ip, cpu::CpuRegs& regs)
		{
			GLog.Err("BugCheck: Encountered unrecoverable system exception: %d", (uint32)(regs.R3));
			RETURN();
		}

		uint64 __fastcall Xbox_NtDuplicateObject(uint64 ip, cpu::CpuRegs& regs)
		{
			RETURN_DEFAULT();
		}

		uint64 __fastcall Xbox_ObDereferenceObject(uint64 ip, cpu::CpuRegs& regs)
		{
			const auto objectAddress = (const uint32)regs.R3;

			// Check if a dummy value from ObReferenceObjectByHandle.
			if (!objectAddress)
			{
				RETURN_ARG(0);
			}

			// Get the kernel object
			auto* object = GPlatform.GetKernel().ResolveObject(objectAddress, xenon::NativeKernelObjectType::Unknown);
			if (!object)
			{
				GLog.Err("Kernel: Pointer %08Xh does not match any kernel object", objectAddress);
				RETURN_ARG(0);
			}

			// object cannot be dereferenced
			if (!object->IsRefCounted())
			{
				GLog.Err("Kernel: Called dereference on object '%s' which is not refcounted (pointer %08Xh)", object->GetName(), objectAddress);
				RETURN_ARG(0);
			}

			// dereference the object
			auto* refCountedObject = static_cast<xenon::IKernelObjectRefCounted*>(object);
			refCountedObject->Release();
			RETURN_ARG(0);
		}

		uint64 __fastcall Xbox_ObReferenceObjectByHandle(uint64 ip, cpu::CpuRegs& regs)
		{
			const auto handle = (const uint32)regs.R3;
			const auto objectType = (const uint32)regs.R4;
			const auto outObjectPtr = Pointer<uint32>(regs.R5);

			// resolve handle to a kernel object
			auto* object = GPlatform.GetKernel().ResolveHandle(handle, xenon::KernelObjectType::Unknown);
			if (!object)
			{
				GLog.Warn("Unresolved object handle: %08Xh", handle);
				RETURN_ARG(X_ERROR_BAD_ARGUMENTS);
			}

			// object is not refcounted
			if (!object->IsRefCounted())
			{
				GLog.Err("Kernel object is not refcounted!");
			}

			// add additional reference
			auto* refCountedObject = static_cast<xenon::IKernelObjectRefCounted*>(object);
			refCountedObject->AddRef();

			// resolve native data
			uint32 nativeAddr = 0;;
			if (objectType == 0xD017BEEF) // ExSemaphoreObjectType
			{
				GLog.Err("Unsupported case");
				nativeAddr = 0xDEADF00D;
			}
			else if (objectType == 0xD01BBEEF) // ExThreadObjectType
			{
				nativeAddr = static_cast< xenon::KernelThread* >(object)->GetMemoryBlock().GetThreadDataAddr();
			}
			else
			{
				if (object->GetType() == xenon::KernelObjectType::Thread)
				{
					// a thread ? if so, get the internal thread data
					nativeAddr = static_cast< xenon::KernelThread* >(object)->GetMemoryBlock().GetThreadDataAddr();
				}
				else
				{
					GLog.Warn("Object '%s', handle %08Xh has undefined object data", object->GetName(), handle);
					nativeAddr = 0xDEADF00D;
				}
			}

			// save the shit
			if (outObjectPtr.IsValid())
				*outObjectPtr = nativeAddr;

			RETURN_ARG(0);
		}

		uint64 __fastcall Xbox_ObCreateSymbolicLink(uint64 ip, cpu::CpuRegs& regs)
		{
			RETURN_DEFAULT();
		}

		uint64 __fastcall Xbox_ObDeleteSymbolicLink(uint64 ip, cpu::CpuRegs& regs)
		{
			RETURN_DEFAULT();
		}

		uint64 __fastcall Xbox_ExRegisterTitleTerminateNotification(uint64 ip, cpu::CpuRegs& regs)
		{
			//const uint32 ptr = (uint32)regs.R3;
			//const uint32 create = (uint32)regs.R4;
			//const uint32 routine = _byteswap_ulong(((uint32*)ptr)[0]);
			//const uint32 priority = _byteswap_ulong(((uint32*)ptr)[1]);

			RETURN_ARG(0);
		}

		uint64 __fastcall Xbox_KeInitializeDpc(uint64 ip, cpu::CpuRegs& regs)
		{
			const uint32 dpcPtr = (const uint32)regs.R3;
			const uint32 addr = (const uint32)regs.R4;
			const uint32 context = (const uint32)regs.R4;

			// KDPC (maybe) 0x18 bytes?
			const uint32 type = 19;
			const uint32 importance = 0;
			const uint32 unkown = 0;

			cpu::mem::storeAddr<uint32>(dpcPtr + 0, (type << 24) | (importance << 16) | (unkown));
			cpu::mem::storeAddr<uint32>(dpcPtr + 4, 0);  // flink
			cpu::mem::storeAddr<uint32>(dpcPtr + 8, 0);  // blink
			cpu::mem::storeAddr<uint32>(dpcPtr + 12, addr);
			cpu::mem::storeAddr<uint32>(dpcPtr + 16, context);
			cpu::mem::storeAddr<uint32>(dpcPtr + 20, 0);  // arg1
			cpu::mem::storeAddr<uint32>(dpcPtr + 24, 0);  // arg2
			xenon::TagMemoryWrite(dpcPtr, 28, "KeInitializeDpc");

			RETURN();
		}

		uint64 __fastcall Xbox_KeInsertQueueDpc(uint64 ip, cpu::CpuRegs& regs)
		{
			const uint32 dpcPtr = (const uint32)regs.R3;
			const uint32 arg1 = (const uint32)regs.R4;
			const uint32 arg2 = (const uint32)regs.R5;

			const uint32 listEntryPtr = dpcPtr + 4;

			// Lock dispatcher.
			auto& dispatcher = GPlatform.GetKernel().GetDPCDispatcher();
			dispatcher.Lock();

			// If already in a queue, abort.
			auto* dpcList = dispatcher.GetList();
			if (dpcList->IsQueued(listEntryPtr))
			{
				dispatcher.Unlock();
				RETURN_ARG(0);
			}

			// Prep DPC.
			cpu::mem::storeAddr<uint32>(dpcPtr + 20, arg1);
			cpu::mem::storeAddr<uint32>(dpcPtr + 24, arg2);

			dpcList->Insert(listEntryPtr);
			dispatcher.Unlock();

			RETURN_ARG(1);
		}

		uint64 __fastcall Xbox_KeRemoveQueueDpc(uint64 ip, cpu::CpuRegs& regs)
		{
			const uint32 dpcPtr = (const uint32)regs.R3;

			GLog.Err("KeRemoveQueueDpc(0x%p)", dpcPtr);

			bool result = false;

			uint32 listentryPtr = dpcPtr + 4;

			auto& dispatcher = GPlatform.GetKernel().GetDPCDispatcher();
			dispatcher.Lock();

			auto dpcList = dispatcher.GetList();
			if (dpcList->IsQueued(listentryPtr))
			{
				dpcList->Remove(listentryPtr);
				result = true;
			}

			dispatcher.Unlock();

			RETURN_ARG(result ? 1 : 0);
		}

		static std::mutex GGlobalListMutex;

		// http://www.nirsoft.net/kernel_struct/vista/SLIST_HEADER.html
		uint64 __fastcall Xbox_InterlockedPopEntrySList(uint64 ip, cpu::CpuRegs& regs)
		{
			std::lock_guard<std::mutex> lock(GGlobalListMutex);

			auto plistPtr = Pointer<uint32>(regs.R3);

			uint32 first = *plistPtr;
			if (first == 0)
			{
				RETURN_ARG(0);
			}

			auto second = *Pointer<uint32>(first);
			*plistPtr = second;

			RETURN_ARG(first);
		}

		uint64 __fastcall Xbox_KeLockL2(uint64 ip, cpu::CpuRegs& regs)
		{
			RETURN_DEFAULT();
		}

		uint64 __fastcall Xbox_KeUnlockL2(uint64 ip, cpu::CpuRegs& regs)
		{
			RETURN_DEFAULT();
		}

		uint64 __fastcall Xbox_MmQueryAddressProtect(uint64 ip, cpu::CpuRegs& regs)
		{
			uint32 address = (uint32)(regs.R3);

			const auto protectFlags = GPlatform.GetMemory().GetVirtualMemoryFlags((void*)address, 0);
			RETURN_ARG(protectFlags);
		}

		uint64 __fastcall Xbox_MmSetAddressProtect(uint64 ip, cpu::CpuRegs& regs)
		{
			uint32 address = (uint32)(regs.R3);
			uint32 size = (uint32)(regs.R4);
			uint32 protectFlags = (uint32)(regs.R5) & 0xF;

			GPlatform.GetMemory().SetVirtualMemoryFlags((void*)address, size, protectFlags);
			RETURN_ARG(0);
		}

		uint64 __fastcall Xbox_MmQueryAllocationSize(uint64 ip, cpu::CpuRegs& regs)
		{
			uint32 address = (uint32)(regs.R3);
			uint32 size = GPlatform.GetMemory().GetVirtualMemoryAllocationSize((void*)address);
			RETURN_ARG(size);
		}

		void Xbox_Interrupt20(const uint64_t ip, const uint32_t index, const cpu::CpuRegs& regs)
		{
			const char* msg = (const char*)regs.R3;
			GPlatform.DebugTrace(msg);
		}

		void Xbox_Interrupt22(const uint64_t ip, const uint32_t index, const cpu::CpuRegs& regs)
		{

		}

		void RegisterXboxKernel(runtime::Symbols& symbols)
		{
			REGISTER(XGetLanguage);
			REGISTER(XGetAVPack);

			REGISTER(KeBugCheckEx);
			REGISTER(KeBugCheck);

			REGISTER(NtAllocateVirtualMemory);
			REGISTER(NtFreeVirtualMemory);
			REGISTER(NtQueryVirtualMemory);

			REGISTER(KeQueryPerformanceFrequency);

			REGISTER(XamLoaderTerminateTitle);
			REGISTER(XamShowMessageBoxUIEx);

			REGISTER(NtDuplicateObject);
			REGISTER(ObDereferenceObject);
			REGISTER(ObReferenceObjectByHandle);
			REGISTER(ObDeleteSymbolicLink);
			REGISTER(ObCreateSymbolicLink);

			REGISTER(XexCheckExecutablePrivilege);
			REGISTER(__C_specific_handler);
			REGISTER(HalReturnToFirmware);
			REGISTER(KeGetCurrentProcessType);
			REGISTER(ExRegisterTitleTerminateNotification);
			REGISTER(KeEnableFpuExceptions);

			REGISTER(KeInitializeDpc);
			REGISTER(KeInsertQueueDpc);
			REGISTER(KeRemoveQueueDpc);
			REGISTER(InterlockedPopEntrySList);

			REGISTER(KeLockL2);
			REGISTER(KeUnlockL2);

			REGISTER(KeSetCurrentProcessType);

			REGISTER(MmQueryAddressProtect);
			REGISTER(MmSetAddressProtect);
			REGISTER(MmQueryAllocationSize);

			symbols.RegisterInterrupt(20, (runtime::TInterruptFunc) &Xbox_Interrupt20);
			symbols.RegisterInterrupt(22, (runtime::TInterruptFunc) &Xbox_Interrupt22);
		}

	} // lib
} // xenon