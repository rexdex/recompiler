#include "build.h"
#include "xenonLibUtils.h" 
#include "xenonLibNatives.h"
#include "xenonThread.h"
#include <mutex>
#include "xenonPlatform.h"

#pragma once 

//---------------------------------------------------------------------------

uint64 __fastcall XboxKernel_KeEnableFpuExceptions( uint64 ip, cpu::CpuRegs& regs )
{
	const uint32 enabled = (const uint32) regs.R3;
	GLog.Log( "KeEnableFpuExceptions(%d)", enabled );
	RETURN_ARG(0);
}

uint64 __fastcall XboxKernel_KeQueryPerformanceFrequency( uint64 ip, cpu::CpuRegs& regs )
{
	__int64 ret = 0;
	QueryPerformanceFrequency((LARGE_INTEGER*)&ret);
	//ret *= 10;
	RETURN_ARG(ret); // english
}

uint64 __fastcall XboxKernel_XGetLanguage( uint64 ip, cpu::CpuRegs& regs )
{
	GLog.Log("XGetLanguage: english returned");
	RETURN_ARG(1); // english
}

uint64 __fastcall XboxKernel_XGetAVPack( uint64 ip, cpu::CpuRegs& regs )
{
	GLog.Log("XGetAVPack: returning USA/Canada");
	RETURN_ARG(0x010000);
}

uint64 __fastcall XboxKernel_XamLoaderTerminateTitle( uint64 ip, cpu::CpuRegs& regs )
{
	GLog.Log("XamLoaderTerminateTitle: called");
	ULONG_PTR args[1] = { 0 }; // exit code
	RaiseException(cpu::EXCEPTION_TERMINATE_PROCESS, EXCEPTION_NONCONTINUABLE, 1, &args[0] );
	RETURN();
}

uint64 __fastcall XboxKernel_XamShowMessageBoxUIEx( uint64 ip, cpu::CpuRegs& regs )
{
	RETURN_DEFAULT();
}

uint64 __fastcall XboxKernel_RtlInitAnsiString( uint64 ip, cpu::CpuRegs& regs )
{
	xnative::X_ANSI_STRING* ansiString = GetPointer<xnative::X_ANSI_STRING>( regs.R3 );
	const char* srcString = GetPointer<char>( regs.R4 );

	uint32 len = (uint32)( srcString ? strlen(srcString) : 0 );
	if (len >= 65534)
		len = 65534;

	ansiString->Length = ToPlatform((uint16)(len));
	ansiString->MaximumLength = ToPlatform((uint16)(len+1));
	ansiString->BufferPtr = ToPlatform((uint32)srcString);

	RETURN();
}

template< typename T >
static inline uint32 ExtractSetting(T value, cpu::CpuRegs& regs )
{
	void* outputBuf = GetPointer<void>(regs.R5);
	const uint32 outputBufSize = (uint32) regs.R6;
	uint16* settingSizePtr = GetPointer<uint16>(regs.R7);

	// no buffer, extract size
	if (!outputBuf)
	{
		// no size ptr :(
		if (!settingSizePtr)
			RETURN_ARG(ERROR_INVALID_DATA);

		// output data size (watch for endianess)
		*settingSizePtr = ToPlatform((uint16)sizeof(T));
		RETURN_ARG(0);
	}

	// buffer to small
	if (outputBufSize < sizeof(T))
		RETURN_ARG(ERROR_INVALID_DATA);

	// output value
	memcpy(outputBuf, &value, sizeof(T));

	// output size
	if (settingSizePtr)
		*settingSizePtr = ToPlatform((uint16)sizeof(T));

	// valid
	RETURN_ARG(0);
}

// HRESULT __stdcall ExGetXConfigSetting(u16 categoryNum, u16 settingNum, void* outputBuff, s32 outputBuffSize, u16* settingSize);
uint64 __fastcall XboxKernel_ExGetXConfigSetting( uint64 ip, cpu::CpuRegs& regs )
{
	const uint16 categoryNum = (uint16) regs.R3;
	const uint16 settingNum = (uint16) regs.R4;
	void* outputBuf = GetPointer<void>(regs.R5);
	const uint32 outputBufSize = (uint32) regs.R6;
	uint16* settingSizePtr = GetPointer<uint16>(regs.R7);
	GLog.Log( "XConfig: cat(%d), setting(%d), buf(0x%08X), bufsize(%d), sizePtr(0x%08X)",
		categoryNum, settingNum, (uint32)outputBuf, outputBufSize, (uint32)settingSizePtr);

	// invalid category num
	if (categoryNum >= xnative::XCONFIG_CATEGORY_MAX)
	{
		GLog.Log( "XConfig: invalid category id(%d)" );
		RETURN_ARG(-1);
	}

	// category type
	GLog.Log( "XConfig: Category type '%s' (%d)", xnative::XConfigNames[categoryNum], categoryNum );

	// extract the setting
	switch (categoryNum)
	{
		case xnative::XCONFIG_SECURED_CATEGORY:
		{
			switch (settingNum)
			{
				//case XCONFIG_SECURED_DATA:
				case xnative::XCONFIG_SECURED_MAC_ADDRESS: return ExtractSetting( xnative::XGetConfigSecuredSettings().MACAddress, regs );
				case xnative::XCONFIG_SECURED_AV_REGION: return ExtractSetting( ToPlatform(xnative::XGetConfigSecuredSettings().AVRegion), regs );
				case xnative::XCONFIG_SECURED_GAME_REGION: return ExtractSetting( ToPlatform(xnative::XGetConfigSecuredSettings().GameRegion), regs );
				case xnative::XCONFIG_SECURED_DVD_REGION: return ExtractSetting( ToPlatform(xnative::XGetConfigSecuredSettings().DVDRegion), regs );
				case xnative::XCONFIG_SECURED_RESET_KEY: return ExtractSetting( ToPlatform(xnative::XGetConfigSecuredSettings().ResetKey), regs );
				case xnative::XCONFIG_SECURED_SYSTEM_FLAGS: return ExtractSetting( ToPlatform(xnative::XGetConfigSecuredSettings().SystemFlags), regs );
				case xnative::XCONFIG_SECURED_POWER_MODE: return ExtractSetting( xnative::XGetConfigSecuredSettings().PowerMode, regs );
				case xnative::XCONFIG_SECURED_ONLINE_NETWORK_ID: return ExtractSetting( xnative::XGetConfigSecuredSettings().OnlineNetworkID, regs );
				case xnative::XCONFIG_SECURED_POWER_VCS_CONTROL: return ExtractSetting( xnative::XGetConfigSecuredSettings().PowerVcsControl, regs );
				default: break;
			}

			break;
		}

		case xnative::XCONFIG_USER_CATEGORY:
		{
			switch (settingNum)
			{
				case xnative::XCONFIG_USER_VIDEO_FLAGS: return ExtractSetting( (uint32)0x7, regs );
			}
		}

		default:
			break;
	}

	// invalid config settings
	RETURN_ARG(ERROR_INVALID_DATA);
}

// BOOL XexCheckExecutablePrivilege(DWORD priviledge);
uint64 __fastcall XboxKernel_XexCheckExecutablePrivilege( uint64 ip, cpu::CpuRegs& regs )
{
	const uint32 mask = (const uint32) regs.R3;
	GLog.Log("Privledge check: 0x%08X", mask);
	RETURN_ARG(1);
}

uint64 __fastcall XboxKernel_DbgPrint( uint64 ip, cpu::CpuRegs& regs )
{
	// input string
	const char* txt = GetPointer<const char>(regs.R3);
	if ( txt )
	{
		const char* cur = txt;
		const char* end = txt + strlen(txt);;

		// output string
		char buffer[ 4096 ];
		char* write = buffer;
		char* writeEnd = buffer + sizeof(buffer) - 1;

		// format values
		const TReg* curReg = &regs.R4;
		while ( cur < end )
		{
			const char ch = *cur++;

			if (ch == '%')
			{
				// skip the numbers (for now)
				while (*cur >= '0' && *cur <= '9' )
					++cur;

				// get the code				
				const char code = *cur++;
				switch ( code )
				{
					// just %
					case '%':
					{
						Append(write, writeEnd, '%');
						break;
					}

					// address
					case 'x':
					case 'X':
					{
						const uint64 val = *curReg;
						AppendHex(write, writeEnd, val);
						++curReg;
						break;
					}

					// string
					case 's':
					case 'S':
					{
						const char* str = (const char*)( *curReg & 0xFFFFFFFF );
						AppendString(write, writeEnd, str);
						++curReg;
						break;
					}
				}
			}
			else
			{
				Append(write,writeEnd,ch);
			}
		}

		// end of string
		Append(write, writeEnd,0);

		// print the generated buffer
		GLog.Log( "DbgPrint: %s", buffer );
	}

	RETURN();
}

uint64 __fastcall XboxKernel___C_specific_handler( uint64 ip, cpu::CpuRegs& regs )
{
	RETURN_DEFAULT();
}

uint64 __fastcall XboxKernel_RtlImageXexHeaderField( uint64 ip, cpu::CpuRegs& regs )
{
	RETURN_DEFAULT();
}

uint64 __fastcall XboxKernel_HalReturnToFirmware( uint64 ip, cpu::CpuRegs& regs )
{
	RETURN_DEFAULT();
}
////#pragma optimize( "", off )
uint64 __fastcall XboxKernel_NtAllocateVirtualMemory( uint64 ip, cpu::CpuRegs& regs )
{
	uint32* basePtr = GetPointer<uint32>(regs.R3);
	void* base = (void*)_byteswap_ulong(*basePtr);
	uint32* sizePtr = GetPointer<uint32>(regs.R4);
	uint32 size = _byteswap_ulong((uint32) *sizePtr);
	uint64 allocType = regs.R5;
	uint64 protect = regs.R6;

	char* commitFlag = (allocType & xnative::XMEM_COMMIT) ? "COMIT " : "";
	char* reserveFlag = (allocType & xnative::XMEM_RESERVE) ? "RESERVE " : "";
	GLog.Log( "NtAllocateVirtualMemory(size=%d, base=0x%08X) %s%s", (uint32)size, (uint32)base, reserveFlag, commitFlag );
	/*GLog.Log( "  *BasePtr = 0x%08X", (uint32)basePtr);
	GLog.Log( "  *Base = 0x%08X", (uint32)base);
	GLog.Log( "  *SizePtr = 0x%08X", (uint32)sizePtr);
	GLog.Log( "  *Size = 0x%08X", (uint32)size);
	GLog.Log( "  *AllocFlag = %08X", allocType);
	if ( allocType & xnative::XMEM_COMMIT ) GLog.Log( "  *AllocFlag = MEM_COMMIT" );
	if ( allocType & xnative::XMEM_RESERVE ) GLog.Log( "  *AllocFlag = MEM_RESERVE" );
	if ( allocType & xnative::XMEM_DECOMMIT ) GLog.Log( "  *AllocFlag = MEM_DECOMMIT" );
	if ( allocType & xnative::XMEM_RELEASE ) GLog.Log( "  *AllocFlag = MEM_RELEASE" );
	if ( allocType & xnative::XMEM_FREE ) GLog.Log( "  *AllocFlag = MEM_FREE" );
	if ( allocType & xnative::XMEM_PRIVATE ) GLog.Log( "  *AllocFlag = MEM_PRIVATE" );
	if ( allocType & xnative::XMEM_RESET ) GLog.Log( "  *AllocFlag = MEM_RESET" );
	if ( allocType & xnative::XMEM_TOP_DOWN ) GLog.Log( "  *AllocFlag = MEM_TOP_DOWN" );
	if ( allocType & xnative::XMEM_NOZERO ) GLog.Log( "  *AllocFlag = MEM_NOZERO" );
	if ( allocType & xnative::XMEM_LARGE_PAGES ) GLog.Log( "  *AllocFlag = MEM_LARGE_PAGES" );
	if ( allocType & xnative::XMEM_HEAP ) GLog.Log( "  *AllocFlag = MEM_HEAP" );
	if ( allocType & xnative::XMEM_16MB_PAGES ) GLog.Log( "  *AllocFlag = MEM_16MB_PAGES" );
	GLog.Log( "  *Protect = %08X", protect);
	if ( protect & xnative::XPAGE_NOACCESS ) GLog.Log( "  *Protect = PAGE_NOACCESS" );
	if ( protect & xnative::XPAGE_READONLY ) GLog.Log( "  *Protect = PAGE_READONLY" );
	if ( protect & xnative::XPAGE_READWRITE ) GLog.Log( "  *Protect = PAGE_READWRITE" );
	if ( protect & xnative::XPAGE_WRITECOPY ) GLog.Log( "  *Protect = PAGE_WRITECOPY" );
	if ( protect & xnative::XPAGE_EXECUTE ) GLog.Log( "  *Protect = PAGE_EXECUTE" );
	if ( protect & xnative::XPAGE_EXECUTE_READ ) GLog.Log( "  *Protect = PAGE_EXECUTE_READ" );
	if ( protect & xnative::XPAGE_EXECUTE_READWRITE ) GLog.Log( "  *Protect = PAGE_EXECUTE_READWRITE" );
	if ( protect & xnative::XPAGE_EXECUTE_WRITECOPY ) GLog.Log( "  *Protect = PAGE_EXECUTE_WRITECOPY" );
	if ( protect & xnative::XPAGE_GUARD ) GLog.Log( "  *Protect = PAGE_GUARD" );
	if ( protect & xnative::XPAGE_NOCACHE ) GLog.Log( "  *Protect = PAGE_NOCACHE" );
	if ( protect & xnative::XPAGE_WRITECOMBINE ) GLog.Log( "  *Protect = PAGE_WRITECOMBINE" );
	if ( protect & xnative::XPAGE_USER_READONLY ) GLog.Log( "  *Protect = PAGE_USER_READONLY" );
	if ( protect & xnative::XPAGE_USER_READWRITE ) GLog.Log( "  *Protect = PAGE_USER_READWRITE" );*/
	
	// invalid type
	if (allocType & xnative::XMEM_RESERVE && (base != NULL))
	{
		GLog.Err("VMEM: Trying to reserve predefined memory region. Not supported.");
		regs.R3 = ((DWORD   )0xC0000005L);
		*basePtr = NULL;
		RETURN();
	}

	// translate allocation flags
	uint32 realAllocFlags = 0;
	if ( allocType & xnative::XMEM_COMMIT ) realAllocFlags |= native::IMemory::eAlloc_Commit;
	if ( allocType & xnative::XMEM_RESERVE )  realAllocFlags |= native::IMemory::eAlloc_Reserve;
	if ( allocType & xnative::XMEM_DECOMMIT ) realAllocFlags |= native::IMemory::eAlloc_Decomit;
	if ( allocType & xnative::XMEM_RELEASE )  realAllocFlags |= native::IMemory::eAlloc_Release;
	if ( allocType & xnative::XMEM_LARGE_PAGES )  realAllocFlags |= native::IMemory::eAlloc_Page64K;
	if ( allocType & xnative::XMEM_16MB_PAGES )  realAllocFlags |= native::IMemory::eAlloc_Page16MB;

	// access flags
	uint32 realAccessFlags = 0;
	if ( protect & xnative::XPAGE_READONLY ) realAccessFlags |= native::IMemory::eFlags_ReadOnly;
	if ( protect & xnative::XPAGE_READWRITE ) realAccessFlags |= native::IMemory::eFlags_ReadWrite;
	if ( protect & xnative::XPAGE_NOCACHE ) realAccessFlags |= native::IMemory::eFlags_NotCached;
	if ( protect & xnative::XPAGE_WRITECOMBINE ) realAccessFlags |= native::IMemory::eFlags_WriteCombine;

	// determine aligned size
	uint32 pageAlignment = 4096;
	if ( allocType & xnative::XMEM_LARGE_PAGES ) pageAlignment = 64 << 10;
	else if ( allocType & xnative::XMEM_16MB_PAGES ) pageAlignment = 16 << 20;

	// align size
	uint32 alignedSize = (size + (pageAlignment-1)) & ~(pageAlignment-1);
	if ( alignedSize != size )
	{
		GLog.Warn("VMEM: Aligned memory allocation %08X->%08X.", size, alignedSize);
	}

	// allocate memory from system
	void* allocated = GPlatform.GetMemory().VirtualAlloc(base,alignedSize,realAllocFlags,realAccessFlags);
	if (!allocated)
	{
		GLog.Err("VMEM: Allocation failed.");
		regs.R3 = ((DWORD   )0xC0000005L);
		*basePtr = NULL;
		RETURN();
	}

	// save allocated memory & size
	*basePtr = _byteswap_ulong((uint32)allocated);	
	*sizePtr = _byteswap_ulong((uint32)alignedSize);

	// allocation was OK
	regs.R3 = 0;
	RETURN();
}

uint64 __fastcall XboxKernel_KeBugCheckEx( uint64 ip, cpu::CpuRegs& regs )
{
	GLog.Err( "BugCheck: Encountered unrecoverable system exception: %d (%016LLXX, %016LLXX, %016LLXX, %016LLXX)",
		(uint32)(regs.R3), regs.R4, regs.R5, regs.R6, regs.R7 );
	RETURN();
}

uint32 GCurrentProcessType = xnative::X_PROCTYPE_USER;

uint64 __fastcall XboxKernel_KeGetCurrentProcessType( uint64 ip, cpu::CpuRegs& regs )
{
	RETURN_ARG(GCurrentProcessType); // ?
}

uint64 __fastcall XboxKernel_KeSetCurrentProcessType( uint64 ip, cpu::CpuRegs& regs )
{
	const uint32 type = (const uint32) regs.R3;
	DEBUG_CHECK( type >= 0 && type <= 2 );
	GCurrentProcessType = type;
	RETURN();
}

uint64 __fastcall XboxKernel_NtFreeVirtualMemory( uint64 ip, cpu::CpuRegs& regs )
{
	void** basePtr = GetPointer<void*>(regs.R3);
	void* base = (void*)_byteswap_ulong((uint32) *basePtr);
	uint32* sizePtr = GetPointer<uint32>(regs.R4);
	uint32 size = _byteswap_ulong((uint32) *sizePtr);
	uint64 freeType = regs.R5;
	uint64 protect = regs.R6;

	char* decoimtFlag = (freeType & xnative::XMEM_DECOMMIT) ? "DECOMIT " : "";
	char* releaseFlag = (freeType & xnative::XMEM_RELEASE) ? "RELEASE " : "";
	GLog.Log( "NtFreeVirtualMemory(size=%d, base=0x%08X, %s%s)", (uint32)size, (uint32)base, decoimtFlag, releaseFlag );

	if (!base)
		return xnative::X_STATUS_MEMORY_NOT_ALLOCATED;

	// translate allocation flags
	uint32 realFreeFlags = 0;
	if ( freeType & xnative::XMEM_COMMIT ) realFreeFlags |= native::IMemory::eAlloc_Commit;
	if ( freeType & xnative::XMEM_RESERVE )  realFreeFlags |= native::IMemory::eAlloc_Reserve;
	if ( freeType & xnative::XMEM_DECOMMIT ) realFreeFlags |= native::IMemory::eAlloc_Decomit;
	if ( freeType & xnative::XMEM_RELEASE )  realFreeFlags |= native::IMemory::eAlloc_Release;

	uint32 freedSize = 0;
	if ( !GPlatform.GetMemory().VirtualFree(base, size, realFreeFlags, freedSize) )
		return xnative::X_STATUS_UNSUCCESSFUL;

	if ( sizePtr )
		*sizePtr = _byteswap_ulong( freedSize );

	RETURN_ARG(0);
}

uint64 __fastcall XboxKernel_RtlCompareMemoryUlong( uint64 ip, cpu::CpuRegs& regs )
{
	RETURN_DEFAULT();
}

uint64 __fastcall XboxKernel_NtQueryVirtualMemory( uint64 ip, cpu::CpuRegs& regs )
{
	RETURN_DEFAULT();
}

uint64 __fastcall XboxKernel_RtlRaiseException( uint64 ip, cpu::CpuRegs& regs )
{
	RETURN_DEFAULT();
}

uint64 __fastcall XboxKernel_RtlNtStatusToDosError( uint64 ip, cpu::CpuRegs& regs )
{
	uint32 status = (const uint32) regs.R3;
	GLog.Log("RtlNtStatusToDosError(%04X)", status);

	if (!status || (status & 0x20000000))
	{
	    RETURN_ARG(0);
	}
	else if ((status & 0xF0000000) == 0xD0000000)
	{
		// High bit doesn't matter.
		status &= ~0x10000000;
	}

	GLog.Err("RtlNtStatusToDosError lookup NOT IMPLEMENTED");
	uint32 result = 317;  // ERROR_MR_MID_NOT_FOUND
	RETURN_ARG(result);
}

uint64 __fastcall XboxKernel_KeBugCheck( uint64 ip, cpu::CpuRegs& regs )
{
	GLog.Err( "BugCheck: Encountered unrecoverable system exception: %d",
		(uint32)(regs.R3) );

	RETURN();
}

uint64 __fastcall XboxKernel_RtlFreeAnsiString( uint64 ip, cpu::CpuRegs& regs )
{
	RETURN_DEFAULT();
}

uint64 __fastcall XboxKernel_RtlUnicodeStringToAnsiString( uint64 ip, cpu::CpuRegs& regs )
{
	RETURN_DEFAULT();
}

uint64 __fastcall XboxKernel_RtlInitUnicodeString( uint64 ip, cpu::CpuRegs& regs )
{
	RETURN_DEFAULT();
}

uint64 __fastcall XboxKernel_NtDuplicateObject( uint64 ip, cpu::CpuRegs& regs )
{
	RETURN_DEFAULT();
}

uint64 __fastcall XboxKernel_ObDereferenceObject( uint64 ip, cpu::CpuRegs& regs )
{
	const uint32 objectPtr = (const uint32) regs.R3;
	GLog.Log("ObDereferenceObject(%08Xh)", objectPtr);

	// Check if a dummy value from ObReferenceObjectByHandle.
	if (!objectPtr)
	{
		RETURN_ARG(0);
	}

	// Get the kernel object
	auto* object = GPlatform.GetKernel().ResolveObject( (void*) objectPtr, xenon::NativeKernelObjectType::Unknown );
	if (!object)
	{
		GLog.Err( "Kernel: Pointer %08Xh does not match any kernel object", (uint32)objectPtr );
		RETURN_ARG(0);
	}

	// object cannot be dereferenced
	if (!object->IsRefCounted())
	{
		GLog.Err( "Kernel: Called dereference on object '%s' which is not refcounted (pointer %08Xh)", object->GetName(), (uint32)objectPtr );
		RETURN_ARG(0);
	}

	// dereference the object
	auto* refCountedObject = static_cast<xenon::IKernelObjectRefCounted*>(object);
	refCountedObject->Release();
	RETURN_ARG(0);
}

uint64 __fastcall XboxKernel_ObReferenceObjectByHandle( uint64 ip, cpu::CpuRegs& regs )
{
	const uint32 handle = (const uint32) regs.R3;
	const uint32 objectTypePtr = (const uint32) regs.R4;
	const uint32 outObjectPtr = (const uint32) regs.R5;

	// resolve handle to a kernel object
	auto* object = GPlatform.GetKernel().ResolveHandle( handle, xenon::KernelObjectType::Unknown );
	if ( !object )
	{
		GLog.Warn( "Unresolved object handle: %08Xh", handle );
		RETURN_ARG( xnative::X_ERROR_BAD_ARGUMENTS );
	}

	// object is not refcounted
	if ( !object->IsRefCounted() )
	{
		GLog.Err( "Kernel object is not refcounted!" );
	}

	// add additional reference
	auto* refCountedObject = static_cast<xenon::IKernelObjectRefCounted*>(object);
	refCountedObject->AddRef();

	// resolve native data
	uint32 nativeAddr = 0;;
	if ( objectTypePtr == 0xD017BEEF ) // ExSemaphoreObjectType
	{
		GLog.Err( "Unsupported case" );
		nativeAddr = 0xDEADF00D;
	}
	else if ( objectTypePtr == 0xD01BBEEF ) // ExThreadObjectType
	{
		nativeAddr = static_cast< xenon::KernelThread* >( object )->GetMemoryBlock().GetThreadDataAddr();
	}
	else 
	{
		if ( object->GetType() == xenon::KernelObjectType::Thread )
		{
			// a thread ? if so, get the internal thread data
			nativeAddr = static_cast< xenon::KernelThread* >(object)->GetMemoryBlock().GetThreadDataAddr();
		}
		else
		{
			GLog.Warn( "Object '%s', handle %08Xh has undefined object data", object->GetName(), handle );
			nativeAddr = 0xDEADF00D;
		}
	}

	// save the shit
    if ( outObjectPtr )
	{
		mem::storeAddr<uint32>( outObjectPtr, nativeAddr );
    }

	RETURN_ARG( 0 );
}

uint64 __fastcall XboxKernel_ObCreateSymbolicLink( uint64 ip, cpu::CpuRegs& regs )
{
	RETURN_DEFAULT();
}

uint64 __fastcall XboxKernel_ObDeleteSymbolicLink( uint64 ip, cpu::CpuRegs& regs )
{
	RETURN_DEFAULT();
}

uint64 __fastcall XboxKernel_ExRegisterTitleTerminateNotification( uint64 ip, cpu::CpuRegs& regs )
{
	const uint32 ptr = (uint32) regs.R3;
	const uint32 create = (uint32) regs.R4;
	const uint32 routine =  _byteswap_ulong( ((uint32*)ptr)[0] );
	const uint32 priority = _byteswap_ulong( ((uint32*)ptr)[1] );
	
	GLog.Log( "ExRegisterTitleTerminateNotification, routine=0x%08Xh, prio=%d", routine, priority );
	RETURN_ARG(0);
}

// copied from http://msdn.microsoft.com/en-us/library/ff552263
uint64 __fastcall XboxKernel_RtlFillMemoryUlong( uint64 ip, cpu::CpuRegs& regs )
{
	// _Out_  PVOID Destination,
	// _In_   SIZE_T Length,
	// _In_   ULONG Pattern

	const uint32 destPtr = (const uint32) regs.R3;
	const uint32 size = (const uint32) regs.R4;
	const uint32 pattern = (const uint32) regs.R5;

	const uint32 count = size >> 2;
	const uint32 swappedPattern = _byteswap_ulong( pattern );

	uint32* mem = (uint32*) destPtr;
	for ( uint32 i=0; i<count; ++i )
	{
		mem[i] = swappedPattern;
	}

	RETURN();
}

uint64 __fastcall XboxKernel_KeInitializeDpc( uint64 ip, cpu::CpuRegs& regs )
{
	const uint32 dpcPtr = (const uint32) regs.R3;
	const uint32 addr = (const uint32) regs.R4;
	const uint32 context = (const uint32) regs.R4;

	GLog.Log( "KeIKeInitializeDpc(0x%08X, 0x%08X, 0x%08X)", dpcPtr, addr, context );

	// KDPC (maybe) 0x18 bytes?
	const uint32 type = 19;
	const uint32 importance = 0;
	const uint32 unkown = 0;

	mem::storeAddr<uint32>( dpcPtr+0, (type << 24) | (importance << 16) | (unkown) );
	mem::storeAddr<uint32>( dpcPtr+4, 0);  // flink
	mem::storeAddr<uint32>( dpcPtr+8, 0);  // blink
	mem::storeAddr<uint32>( dpcPtr+12, addr);
	mem::storeAddr<uint32>( dpcPtr+16, context);
	mem::storeAddr<uint32>( dpcPtr+20, 0);  // arg1
	mem::storeAddr<uint32>( dpcPtr+24, 0);  // arg2

	RETURN();
}

uint64 __fastcall XboxKernel_KeInsertQueueDpc( uint64 ip, cpu::CpuRegs& regs )
{
	const uint32 dpcPtr = (const uint32) regs.R3;
	const uint32 arg1 = (const uint32) regs.R4;
	const uint32 arg2 = (const uint32) regs.R5;
	
	GLog.Err("KeInsertQueueDpc(0x%08X, 0x%08X, 0x%08X)", dpcPtr, arg1, arg2 );

	const uint32 listEntryPtr = dpcPtr + 4;

	// Lock dispatcher.
	auto& dispatcher = GPlatform.GetKernel().GetDPCDispatcher();
	dispatcher.Lock();

	// If already in a queue, abort.
	auto* dpcList = dispatcher.GetList();
	if ( dpcList->IsQueued( listEntryPtr ) )
	{
		dispatcher.Unlock();
		RETURN_ARG(0);
	}

	// Prep DPC.
	mem::storeAddr<uint32>( dpcPtr + 20, arg1 );
	mem::storeAddr<uint32>( dpcPtr + 24, arg2 );

	dpcList->Insert( listEntryPtr );
	dispatcher.Unlock();

	RETURN_ARG(1);
}

uint64 __fastcall XboxKernel_KeRemoveQueueDpc( uint64 ip, cpu::CpuRegs& regs )
{
	const uint32 dpcPtr = (const uint32) regs.R3;

	GLog.Err("KeRemoveQueueDpc(0x%p)", dpcPtr);

	bool result = false;

	uint32 listentryPtr = dpcPtr + 4;

	auto& dispatcher = GPlatform.GetKernel().GetDPCDispatcher();
	dispatcher.Lock();

	auto dpcList = dispatcher.GetList();
	if ( dpcList->IsQueued( listentryPtr ) )
	{
		dpcList->Remove( listentryPtr );
		result = true;
	}

	dispatcher.Unlock();

	RETURN_ARG(result ? 1 : 0);
}

static std::mutex GGlobalListMutex;

// http://www.nirsoft.net/kernel_struct/vista/SLIST_HEADER.html
uint64 __fastcall XboxKernel_InterlockedPopEntrySList( uint64 ip, cpu::CpuRegs& regs )
{
	const uint32 plistPtr = (const uint32) regs.R3;

	GLog.Log( "InterlockedPopEntrySList(0x%p)", plistPtr );
	
	std::lock_guard<std::mutex> lock( GGlobalListMutex );

	uint32 first = mem::loadAddr<uint32>( plistPtr );
	if (first == 0)
	{
		RETURN_ARG(0);
	}

	uint32 second = mem::loadAddr<uint32>( first );
	mem::storeAddr< uint32 >( plistPtr, second );

	RETURN_ARG(first);
}

uint64 __fastcall XboxKernel_KeLockL2( uint64 ip, cpu::CpuRegs& regs )
{
	RETURN_DEFAULT();
}

uint64 __fastcall XboxKernel_KeUnlockL2( uint64 ip, cpu::CpuRegs& regs )
{
	RETURN_DEFAULT();
}

uint64 __fastcall XboxKernel_XamInputGetState( uint64 ip, cpu::CpuRegs& regs )
{
	uint32 user = (uint32)( regs.R3 );
	uint32 flags = (uint32)( regs.R4 );
	void* statePtr = (void*) (uint32)( regs.R5 );

	memset( statePtr, 0, sizeof(xnative::X_INPUT_STATE) );
	RETURN_ARG(0);
}

uint64 __fastcall XboxKernel_XamInputGetCapabilities( uint64 ip, cpu::CpuRegs& regs )
{
	RETURN_ARG(xnative::XResult::X_ERROR_DEVICE_NOT_CONNECTED);
}

uint64 __fastcall XboxKernel_MmQueryAddressProtect( uint64 ip, cpu::CpuRegs& regs )
{
	uint32 address = (uint32)( regs.R3 );

	uint32 protectFlags = GPlatform.GetMemory().GetVirtualMemoryProtectFlags( (void*) address );
	RETURN_ARG( protectFlags );
}

uint64 __fastcall XboxKernel_MmSetAddressProtect( uint64 ip, cpu::CpuRegs& regs )
{
	uint32 address = (uint32)( regs.R3 );
	uint32 size = (uint32)( regs.R4 );
	uint32 protectFlags = (uint32)( regs.R5 ) & 0xF;

	GPlatform.GetMemory().VirtualProtect( (void*) address, size, protectFlags );
	RETURN_ARG( 0 );
}

uint64 __fastcall XboxKernel_MmQueryAllocationSize( uint64 ip, cpu::CpuRegs& regs )
{
	uint32 address = (uint32)( regs.R3 );
	uint32 size = GPlatform.GetMemory().GetVirtualMemoryAllocationSize( (void*) address );
	RETURN_ARG( size );
}

void XboxKernel_Interrupt20(uint64 ip, cpu::CpuRegs& regs)
{

}

void XboxKernel_Interrupt22(uint64 ip, cpu::CpuRegs& regs)
{

}

void RegisterXboxKernel(runtime::Symbols& symbols)
{
	#define REGISTER(x) symbols.RegisterFunction(#x, (runtime::TBlockFunc) &XboxKernel_##x);
	REGISTER(XGetLanguage);
	REGISTER(XGetAVPack);

	REGISTER(DbgPrint);
	REGISTER(KeBugCheckEx);
	REGISTER(KeBugCheck);

	REGISTER(NtAllocateVirtualMemory);
	REGISTER(NtFreeVirtualMemory);
	REGISTER(NtQueryVirtualMemory);

	REGISTER(KeQueryPerformanceFrequency);

	REGISTER(RtlInitUnicodeString);
	REGISTER(RtlInitAnsiString);
	REGISTER(RtlFreeAnsiString);
	REGISTER(RtlUnicodeStringToAnsiString);

	REGISTER(RtlFillMemoryUlong);

	REGISTER(XamLoaderTerminateTitle);
	REGISTER(XamShowMessageBoxUIEx);

	REGISTER(NtDuplicateObject);
	REGISTER(ObDereferenceObject);
	REGISTER(ObReferenceObjectByHandle);
	REGISTER(ObDeleteSymbolicLink);
	REGISTER(ObCreateSymbolicLink);

	REGISTER(ExGetXConfigSetting);
	REGISTER(XexCheckExecutablePrivilege);
	REGISTER(__C_specific_handler);
	REGISTER(RtlImageXexHeaderField);
	REGISTER(HalReturnToFirmware);
	REGISTER(KeGetCurrentProcessType);
	REGISTER(RtlCompareMemoryUlong);
	REGISTER(RtlRaiseException);
	REGISTER(RtlNtStatusToDosError);
	REGISTER(ExRegisterTitleTerminateNotification);
	REGISTER(KeEnableFpuExceptions);

	REGISTER(KeInitializeDpc);
	REGISTER(KeInsertQueueDpc);
	REGISTER(KeRemoveQueueDpc);
	REGISTER(InterlockedPopEntrySList);

	REGISTER(KeLockL2);
	REGISTER(KeUnlockL2);

	REGISTER(KeSetCurrentProcessType);
	
	REGISTER(XamInputGetState);
	REGISTER(XamInputGetCapabilities);

	REGISTER( MmQueryAddressProtect );
	REGISTER( MmSetAddressProtect );
	REGISTER( MmQueryAllocationSize );

	symbols.RegisterInterrupt(20, (runtime::TInterruptFunc) &XboxKernel_Interrupt20);
	symbols.RegisterInterrupt(22, (runtime::TInterruptFunc) &XboxKernel_Interrupt22);

	#undef REGISTER
}