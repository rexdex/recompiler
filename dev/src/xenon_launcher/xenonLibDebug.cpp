#include "build.h"
#include "xenonLibUtils.h" 
#include "xenonLibNatives.h"
#include "xenonThread.h"
#include <mutex>

//---------------------------------------------------------------------------

uint64 __fastcall XboxDebug_DbgBreakPoint( uint64 ip, cpu::CpuRegs& regs )
{
	GLog.Log( "DbgBreakPoint()" );
	DebugBreak();
	RETURN();
}

uint64 __fastcall XboxDebug_RtlUnwind( uint64 ip, cpu::CpuRegs& regs )
{
	RETURN_DEFAULT();
}

uint64 __fastcall XboxDebug_RtlUnwind2( uint64 ip, cpu::CpuRegs& regs )
{
	RETURN_DEFAULT();
}

void RegisterXboxDebug(runtime::Symbols& symbols)
{
	#define REGISTER(x) symbols.RegisterFunction(#x, *(const runtime::TBlockFunc*) &XboxDebug_##x);
	REGISTER(DbgBreakPoint);
	REGISTER(RtlUnwind);
	REGISTER(RtlUnwind2);
	#undef REGISTER
}

//---------------------------------------------------------------------------
