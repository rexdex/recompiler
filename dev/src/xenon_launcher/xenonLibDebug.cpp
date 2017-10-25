#include "build.h"
#include "xenonLib.h"  
#include "xenonLibNatives.h"
#include "xenonThread.h"
#include "xenonMemory.h"
#include "xenonBindings.h"

namespace xenon
{ 
	namespace lib
	{

		//---------------------------------------------------------------------------

		void Xbox_DbgBreakPoint()
		{
			GLog.Log( "DbgBreakPoint()" );
			DebugBreak();			
		}
		
		xnative::X_STATUS Xbox_DmAbortProfiling()
		{
			return xnative::X_STATUS_SUCCESS;
		}

		xnative::X_STATUS Xbox_DmAddUser(Pointer<char> userName, uint32_t accessLevel)
		{
			return xnative::X_STATUS_SUCCESS;
		}

		MemoryAddress Xbox_DmAllocatePool(uint32_t numBytes)
		{
			return GPlatform.GetMemory().AllocateSmallBlock(numBytes);
		}

		MemoryAddress Xbox_DmAllocatePoolWithTag(uint32_t numBytes, uint32_t tag)
		{
			return GPlatform.GetMemory().AllocateSmallBlock(numBytes);
		}

		xnative::X_STATUS Xbox_DmCaptureStackBackTrace(uint32_t numFrames, Pointer<uint32_t> callStack)
		{
			return xnative::X_STATUS_UNSUCCESSFUL;
		}

		void RegisterXboxDebug(runtime::Symbols& symbols)
		{
			REGISTER(DbgBreakPoint);
			REGISTER(DmAbortProfiling);
			REGISTER(DmAddUser);
			REGISTER(DmAllocatePool);
			REGISTER(DmAllocatePoolWithTag);

			NOT_IMPLEMENTED(RtlUnwind);
			NOT_IMPLEMENTED(RtlUnwind2);
			NOT_IMPLEMENTED(DmAutomationBindController);
			NOT_IMPLEMENTED(DmAutomationClearGamepadQueue);
			NOT_IMPLEMENTED(DmAutomationConnectController);
			NOT_IMPLEMENTED(DmAutomationDisconnectController);
			NOT_IMPLEMENTED(DmAutomationGetInputProcess);
			NOT_IMPLEMENTED(DmAutomationGetUserDefaultProfile);
			NOT_IMPLEMENTED(DmAutomationQueryGamepadQueue);
			NOT_IMPLEMENTED(DmAutomationQueueGamepadState);
			NOT_IMPLEMENTED(DmAutomationSetGamepadState);
			NOT_IMPLEMENTED(DmAutomationSetUserDefaultProfile);
			NOT_IMPLEMENTED(DmAutomationUnbindController);
		}

		//---------------------------------------------------------------------------

	} // lib
} // xenon
