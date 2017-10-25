#include "build.h"
#include "xenonLib.h"  
#include "xenonLibNatives.h"
#include "xenonPlatform.h"
#include "xenonGraphics.h"
#include "xenonMemory.h"
#include "xenonBindings.h"

namespace xenon
{
	namespace lib
	{

		//---------------------------------------------------------------------------

		X_STATUS Xbox_XGetVideoMode(Pointer<XVIDEO_MODE> outModePtr)
		{
			// 720p
			memset(outModePtr.GetNativePointer(), 0, sizeof(XVIDEO_MODE));
			outModePtr->dwDisplayWidth = 1280;
			outModePtr->dwDisplayHeight = 720;
			outModePtr->fIsInterlaced = FALSE;
			outModePtr->fIsWideScreen = TRUE;
			outModePtr->fIsHiDef = TRUE;
			outModePtr->RefreshRate = 60.0f;
			outModePtr->VideoStandard = XC_VIDEO_STANDARD_PAL_I;

			return X_STATUS_SUCCESS;
		}

		X_STATUS Xbox_VdGetCurrentDisplayGamma(Pointer<uint32> arg0, Pointer<float> arg1)
		{
			*arg0 = 2;
			*arg1 = 2.0f;
			return X_STATUS_SUCCESS;
		}

		X_STATUS Xbox_VdGetCurrentDisplayInformation(Pointer<uint32> ptr)
		{
			ptr[0] = (1280 << 16) | 720;
			ptr[1] = 0;
			ptr[2] = 0;
			ptr[3] = 0;
			ptr[4] = 1280;
			ptr[5] = 720;
			ptr[6] = 1280;
			ptr[7] = 720;
			ptr[8] = 1;
			ptr[9] = 0;
			ptr[10] = 0;
			ptr[11] = 0;
			ptr[12] = 1;
			ptr[13] = 0;
			ptr[14] = 0;
			ptr[15] = 0;
			ptr[16] = 0x014000B4;
			ptr[17] = 0x014000B4;
			ptr[18] = (1280 << 16) | 720;
			ptr[19] = 0x42700000;
			ptr[20] = 0;
			ptr[21] = 1280;
			return X_STATUS_SUCCESS;
		}

		uint32 Xbox_VdQueryVideoFlags()
		{
			const uint32 videoFlags = 0x00000003; // widescreen + >=1024
			//const uint32 videoFlags = 0x00000007; // widescreen + >=1920
			return videoFlags;
		}

		X_STATUS Xbox_VdQueryVideoMode(Pointer<XVIDEO_MODE> outVideoModePtr)
		{
			if (outVideoModePtr.IsValid())
			{
				memset(outVideoModePtr.GetNativePointer(), 0, sizeof(XVIDEO_MODE));
				outVideoModePtr->dwDisplayWidth = 1280;
				outVideoModePtr->dwDisplayHeight = 720;
				outVideoModePtr->fIsInterlaced = 0;
				outVideoModePtr->fIsWideScreen = 1;
				outVideoModePtr->fIsHiDef = 1;
				outVideoModePtr->RefreshRate = 60.0f;
				outVideoModePtr->VideoStandard = 1;
				outVideoModePtr->ReservedA = 0x4a;
				outVideoModePtr->ReservedB = 1;
			}

			return X_STATUS_SUCCESS;
		}

		X_STATUS Xbox_VdInitializeEngines()
		{
			return X_STATUS_SUCCESS;
		}

		X_STATUS Xbox_VdShutdownEngines()
		{
			return X_STATUS_SUCCESS;
		}

		X_STATUS Xbox_VdSetGraphicsInterruptCallback(MemoryAddress callbackAddress, uint32 userData)
		{
			GPlatform.GetGraphics().SetInterruptCallbackAddr(callbackAddress.GetAddressValue(), userData);
			return X_STATUS_SUCCESS;
		}

		X_STATUS Xbox_VdEnableRingBufferRPtrWriteBack(MemoryAddress ringBufferAddress, uint32 blockSize)
		{
			GPlatform.GetGraphics().EnableReadPointerWriteBack(ringBufferAddress.GetAddressValue(), blockSize);
			return X_STATUS_SUCCESS;
		}

		X_STATUS Xbox_VdGetSystemCommandBuffer(Pointer<uint32> outPointerA, Pointer<uint32> outPointerB)
		{
			*outPointerA = 0xBEEF0000;
			*outPointerB = 0xBEEF0001;
			return X_STATUS_SUCCESS;
		}

		X_STATUS Xbox_VdInitializeRingBuffer(MemoryAddress ringBufferAddress, uint32 pageCount)
		{
			GPlatform.GetGraphics().InitializeRingBuffer(ringBufferAddress.GetNativePointer(), pageCount);
			return X_STATUS_SUCCESS;
		}

		uint64 Xbox_VdIsHSIOTrainingSucceeded()
		{
			return 1;
		}

		uint64 Xbox_VdPersistDisplay()
		{
			return 1;
		}

		uint64 Xbox_VdRetrainEDRAMWorker()
		{
			return 1;
		}

		uint64 Xbox_VdRetrainEDRAM()
		{
			return 0;
		}

		uint64 Xbox_VdSetDisplayMode()
		{
			return 0;
		}

		uint32 GLastFrontBufferWidth = 0;
		uint32 GLastFrontBufferHeight = 0;

		X_STATUS Xbox_VdSwap(Pointer<uint32_t> ringBufferPtr, uint32, uint32, uint32, uint32, Pointer<MemoryAddress> frontBufferPtr, uint32, uint32)
		{
			const auto frontbuffer = *frontBufferPtr;

			memset(ringBufferPtr.GetNativePointer(), 0, 64 * 4);

			static const uint32 PM4_HACK_SWAP = 0x42;
			ringBufferPtr[0] = (3 << 30) | (62 << 16) | (PM4_HACK_SWAP << 8);
			ringBufferPtr[1] = frontbuffer.GetAddress();
			ringBufferPtr[2] = GLastFrontBufferWidth;
			ringBufferPtr[3] = GLastFrontBufferHeight;
			return X_STATUS_SUCCESS;
		}

		X_STATUS Xbox_VdEnableDisableClockGating()
		{
			return X_STATUS_SUCCESS;
		}

		X_STATUS Xbox_VdCallGraphicsNotificationRoutines(uint32, Pointer<uint16> argsPtr)
		{
			uint16 frontWidth = argsPtr[0];
			uint16 frontHeight = argsPtr[1];
			uint16 backWidth = argsPtr[2];
			uint16 backHeight = argsPtr[3];

			GLastFrontBufferWidth = frontWidth;
			GLastFrontBufferHeight = frontHeight;

			return X_STATUS_SUCCESS;
		}

		X_STATUS Xbox_VdSetSystemCommandBufferGpuIdentifierAddress()
		{
			return X_STATUS_SUCCESS;
		}

		X_STATUS Xbox_VdSetDisplayModeOverride()
		{
			return X_STATUS_SUCCESS;
		}

		uint64 __fastcall Xbox_VdInitializeScalerCommandBuffer(uint64 ip, cpu::CpuRegs& regs)
		{
			const auto destPtr = Pointer<uint32>((const uint32)(regs.R1 + 0x54));
			const auto numWords = 0x1CC / 4;

			for (uint32 i = 0; i < numWords; ++i)
				destPtr[i] = _byteswap_ulong(0x80000000);

			regs.R3 = numWords >> 2;
			return regs.LR;
		}

		//---------------------------------------------------------------------------

		void RegisterXboxVideo(runtime::Symbols& symbols)
		{
			REGISTER(XGetVideoMode);

			REGISTER(VdGetCurrentDisplayGamma);
			REGISTER(VdGetCurrentDisplayInformation);
			REGISTER(VdQueryVideoFlags);
			REGISTER(VdQueryVideoMode);
			REGISTER(VdInitializeEngines);
			REGISTER(VdShutdownEngines);
			REGISTER(VdSetGraphicsInterruptCallback);
			REGISTER(VdInitializeRingBuffer);
			REGISTER(VdEnableRingBufferRPtrWriteBack);
			REGISTER(VdGetSystemCommandBuffer);
			REGISTER(VdIsHSIOTrainingSucceeded);
			REGISTER(VdPersistDisplay);
			REGISTER(VdRetrainEDRAMWorker);
			REGISTER(VdRetrainEDRAM);
			REGISTER(VdCallGraphicsNotificationRoutines);
			REGISTER(VdSwap);
			REGISTER(VdSetDisplayMode);
			REGISTER(VdEnableDisableClockGating);
			REGISTER(VdSetSystemCommandBufferGpuIdentifierAddress);
			REGISTER(VdInitializeScalerCommandBuffer);
			REGISTER(VdSetDisplayModeOverride);
		}

	} // lib
} // xenon