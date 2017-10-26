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
			throw new runtime::TerminateProcessException(0, 0);
		}
		
		X_STATUS Xbox_DmAbortProfiling()
		{
			return X_STATUS_SUCCESS;
		}

		X_STATUS Xbox_DmAddUser(Pointer<char> userName, uint32_t accessLevel)
		{
			return X_STATUS_SUCCESS;
		}

		MemoryAddress Xbox_DmAllocatePool(uint32_t numBytes)
		{
			return GPlatform.GetMemory().AllocateSmallBlock(numBytes);
		}

		MemoryAddress Xbox_DmAllocatePoolWithTag(uint32_t numBytes, uint32_t tag)
		{
			return GPlatform.GetMemory().AllocateSmallBlock(numBytes);
		}

		X_STATUS Xbox_DmCaptureStackBackTrace(uint32_t numFrames, Pointer<uint32_t> callStack)
		{
			return X_STATUS_UNSUCCESSFUL;
		}

		struct XM_DM_VERSION_INFO
		{
			Field<uint16_t> Major;
			Field<uint16_t> Minor;
			Field<uint16_t> Build;
			Field<uint16_t> QFE;
		};

		struct XM_DM_SYSTEM_INFO
		{
			Field<uint32_t> SizeOfStruct;
			XM_DM_VERSION_INFO BaseKernelVersion;
			XM_DM_VERSION_INFO KernelVersion;
			XM_DM_VERSION_INFO XDKVersion;
			Field<uint32_t> SystemInfoFlags;
		};

		X_STATUS Xbox_DmGetSystemInfo(Pointer<XM_DM_SYSTEM_INFO> info)
		{
			memset(info.GetNativePointer(), 0, sizeof(XM_DM_SYSTEM_INFO));
			info->SizeOfStruct = (uint32)sizeof(XM_DM_SYSTEM_INFO);
			info->SystemInfoFlags = 0x00000020;
			return X_STATUS_SUCCESS;
		}

		void RegisterXboxDebug(runtime::Symbols& symbols)
		{
			REGISTER(DbgBreakPoint);
			REGISTER(DmAbortProfiling);
			REGISTER(DmAddUser);
			REGISTER(DmAllocatePool);
			REGISTER(DmAllocatePoolWithTag);
			REGISTER(DmGetSystemInfo);

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
