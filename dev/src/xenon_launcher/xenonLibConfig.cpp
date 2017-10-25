#include "build.h"
#include "xenonLibNatives.h" 
#include "xenonLib.h"  
#include "xenonBindings.h"

namespace xenon
{
	namespace lib
	{

		///------------------
		
		struct ConfigSettings
		{
			char OnlineNetworkID[4]; //??key 0x6 4 bytes?? key 0x8 4 bytes?? NOT SURE WHICH, 0x8 or 0x6
			uint8 MACAddress[6]; // key 0x1 6 bytes
			uint32 AVRegion; // key 0x2 4 bytes - 00 00 10 00 can/usa
			uint16 GameRegion; // key 0x3 2 bytes - 0x00FF can/usa
			uint32 DVDRegion;// key 0x4 4 bytes - 0x00000001 can/usa
			uint32 ResetKey;// key 0x5 4 bytes
			uint32 SystemFlags;// ??key 0x6 4 bytes?? key 0x8 4 bytes?? NOT SURE WHICH, 0x8 or 0x6
			XCONFIG_POWER_MODE PowerMode;// key 0x07 2 bytes
			XCONFIG_POWER_VCS_CONTROL PowerVcsControl;// key 0x9 2 bytes
			uint32 VideoMode;
		};

		ConfigSettings GConfig;

		void InitConfig()
		{
			memset(&GConfig, 0, sizeof(GConfig));

			// setup defaults
			GConfig.AVRegion = 0x00001000; // CAN/US
			GConfig.MACAddress[0] = 0xAB;
			GConfig.MACAddress[1] = 0x0E;
			GConfig.MACAddress[2] = 0x55;
			GConfig.MACAddress[3] = 0x44;
			GConfig.MACAddress[4] = 0xCA;
			GConfig.MACAddress[5] = 0x16;
			GConfig.DVDRegion = 1; // CAN/US
			GConfig.VideoMode = 7;
		}

		///------------------

		template< typename T >
		static HRESULT ExtractSetting(T value, MemoryAddress outputBufPtr, uint32_t outputBuffSize, Pointer<uint16> settingSizePtr)
		{
			// no buffer, extract size
			if (!outputBufPtr.IsValid())
			{
				// no size ptr :(
				if (!settingSizePtr.IsValid())
					return ERROR_INVALID_DATA;

				*settingSizePtr = (uint16)sizeof(T);
				return 0;
			}

			// buffer to small
			if (outputBuffSize < sizeof(T))
				return ERROR_INVALID_DATA;

			// output value
			*Pointer<T>(outputBufPtr) = value;

			// output size
			if (settingSizePtr.IsValid())
				*settingSizePtr = (uint16)sizeof(T);

			// valid
			return 0;
		}

		// HRESULT __stdcall ExGetXConfigSetting(u16 categoryNum, u16 settingNum, void* outputBuff, s32 outputBuffSize, u16* settingSize);
		HRESULT Xbox_ExGetXConfigSetting(uint16_t categoryNum, uint16_t settingNum, MemoryAddress outputBufPtr, uint32_t outputBuffSize, Pointer<uint16> settingSizePtr)
		{
			// invalid category num
			if (categoryNum >= XCONFIG_CATEGORY_MAX)
			{
				GLog.Warn("XConfig: invalid category id(%d)");
				return -1;
			}

			// extract the setting
			switch (categoryNum)
			{
				case XCONFIG_SECURED_CATEGORY:
				{
					switch (settingNum)
					{
						case XCONFIG_SECURED_MAC_ADDRESS: 
							return ExtractSetting(GConfig.MACAddress, outputBufPtr, outputBuffSize, settingSizePtr);

						case XCONFIG_SECURED_AV_REGION: 
							return ExtractSetting(GConfig.AVRegion, outputBufPtr, outputBuffSize, settingSizePtr);

						case XCONFIG_SECURED_GAME_REGION: 
							return ExtractSetting(GConfig.GameRegion, outputBufPtr, outputBuffSize, settingSizePtr);

						case XCONFIG_SECURED_DVD_REGION: 
							return ExtractSetting(GConfig.DVDRegion, outputBufPtr, outputBuffSize, settingSizePtr);

						case XCONFIG_SECURED_RESET_KEY: 
							return ExtractSetting(GConfig.ResetKey, outputBufPtr, outputBuffSize, settingSizePtr);

						case XCONFIG_SECURED_SYSTEM_FLAGS: 
							return ExtractSetting(GConfig.SystemFlags, outputBufPtr, outputBuffSize, settingSizePtr);

						case XCONFIG_SECURED_POWER_MODE: 
							return ExtractSetting(GConfig.PowerMode, outputBufPtr, outputBuffSize, settingSizePtr);

						case XCONFIG_SECURED_ONLINE_NETWORK_ID: 
							return ExtractSetting(GConfig.OnlineNetworkID, outputBufPtr, outputBuffSize, settingSizePtr);

						case XCONFIG_SECURED_POWER_VCS_CONTROL: 
							return ExtractSetting(GConfig.PowerVcsControl, outputBufPtr, outputBuffSize, settingSizePtr);
					}

					break;
				}

				case XCONFIG_USER_CATEGORY:
				{
					switch (settingNum)
					{
						case XCONFIG_USER_VIDEO_FLAGS: 
							return ExtractSetting(GConfig.VideoMode, outputBufPtr, outputBuffSize, settingSizePtr);
					}
					break;
				}
			}

			return ERROR_INVALID_DATA;
		}

		uint32_t Xbox_XGetLanguage()
		{
			return 1;
		}

		uint64_t Xbox_XGetAVPack()
		{
			return 0x010000;
		}

		///---

		void RegisterXboxConfig(runtime::Symbols& symbols)
		{
			REGISTER(ExGetXConfigSetting);
			REGISTER(XGetLanguage);
			REGISTER(XGetAVPack);
		}

	} // lib
} // xenon