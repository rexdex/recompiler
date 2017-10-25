#include "build.h"
#include "xenonLib.h"  
#include "xenonLibNatives.h"
#include "xenonThread.h"

namespace xenon
{ 
	namespace lib
	{

		//---------------------------------------------------------------------------

		uint64 __fastcall Xbox_DbgBreakPoint( uint64 ip, cpu::CpuRegs& regs )
		{
			GLog.Log( "DbgBreakPoint()" );
			DebugBreak();
			RETURN();
		}

		uint64 __fastcall Xbox_RtlUnwind( uint64 ip, cpu::CpuRegs& regs )
		{
			RETURN_DEFAULT();
		}

		uint64 __fastcall Xbox_RtlUnwind2( uint64 ip, cpu::CpuRegs& regs )
		{
			RETURN_DEFAULT();
		}

		uint64 __fastcall Xbox_DmAbortProfiling(uint64 ip, cpu::CpuRegs& regs)
		{
			RETURN_ARG(0);
		}

		uint64 __fastcall Xbox_DmAddUser(uint64 ip, cpu::CpuRegs& regs)
		{
			RETURN_ARG(0);
		}

		uint64 __fastcall Xbox_DmAllocatePool(uint64 ip, cpu::CpuRegs& regs)
		{
			const uint32 size = (uint32)regs.R3;
			RETURN_ARG(0);
		}

		uint64 __fastcall Xbox_DmAllocatePoolWithTag(uint64 ip, cpu::CpuRegs& regs)
		{
			RETURN_ARG(0);
		}

		void RegisterXboxDebug(runtime::Symbols& symbols)
		{
			REGISTER_RAW(DbgBreakPoint);
			REGISTER_RAW(RtlUnwind);
			REGISTER_RAW(RtlUnwind2);
			REGISTER_RAW(DmAbortProfiling);
			REGISTER_RAW(DmAddUser);
			REGISTER_RAW(DmAllocatePool);
			REGISTER_RAW(DmAllocatePoolWithTag);
	}

		//---------------------------------------------------------------------------

	} // lib
} // xenon
