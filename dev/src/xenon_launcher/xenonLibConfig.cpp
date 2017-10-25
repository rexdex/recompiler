#include "build.h"
#include "xenonLibNatives.h" 
#include "xenonLib.h"  

namespace xenon
{
	namespace lib
	{

		///------------------

		xnative::XCONFIG_SECURED_SETTINGS GConfig;

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
		}

		///------------------

		template< typename T >
		static inline uint32 ExtractSetting(T value, cpu::CpuRegs& regs)
		{
			auto outputPtr = Pointer<T>(regs.R5);
			const uint32 outputBufSize = (uint32)regs.R6;
			auto settingSizePtr = Pointer<uint16>(regs.R7);

			// no buffer, extract size
			if (!outputPtr.IsValid())
			{
				// no size ptr :(
				if (!settingSizePtr.IsValid())
					RETURN_ARG(ERROR_INVALID_DATA);

				*settingSizePtr = (uint16)sizeof(T);
				RETURN_ARG(0);
			}

			// buffer to small
			if (outputBufSize < sizeof(T))
				RETURN_ARG(ERROR_INVALID_DATA);

			// output value
			*outputPtr = value;

			// output size
			if (settingSizePtr.IsValid())
				*settingSizePtr = (uint16)sizeof(T);

			// valid
			RETURN_ARG(0);
		}

		// HRESULT __stdcall ExGetXConfigSetting(u16 categoryNum, u16 settingNum, void* outputBuff, s32 outputBuffSize, u16* settingSize);
		uint64 __fastcall Xbox_ExGetXConfigSetting(uint64 ip, cpu::CpuRegs& regs)
		{
			const auto categoryNum = (uint16)regs.R3;
			const auto settingNum = (uint16)regs.R4;
			const auto outputAddress = (uint32)(regs.R5);
			const auto outputBufSize = (uint32)regs.R6;
			auto settingSizePtr = Pointer<uint16>(regs.R7);

			// invalid category num
			if (categoryNum >= xnative::XCONFIG_CATEGORY_MAX)
			{
				GLog.Log("XConfig: invalid category id(%d)");
				RETURN_ARG(-1);
			}

			// category type
			GLog.Log("XConfig: Category type '%s' (%d)", xnative::XConfigNames[categoryNum], categoryNum);

			// extract the setting
			switch (categoryNum)
			{
				case xnative::XCONFIG_SECURED_CATEGORY:
				{
					switch (settingNum)
					{
						//case XCONFIG_SECURED_DATA:
						case xnative::XCONFIG_SECURED_MAC_ADDRESS: return ExtractSetting(GConfig.MACAddress, regs);
						case xnative::XCONFIG_SECURED_AV_REGION: return ExtractSetting(GConfig.AVRegion, regs);
						case xnative::XCONFIG_SECURED_GAME_REGION: return ExtractSetting(GConfig.GameRegion, regs);
						case xnative::XCONFIG_SECURED_DVD_REGION: return ExtractSetting(GConfig.DVDRegion, regs);
						case xnative::XCONFIG_SECURED_RESET_KEY: return ExtractSetting(GConfig.ResetKey, regs);
						case xnative::XCONFIG_SECURED_SYSTEM_FLAGS: return ExtractSetting(GConfig.SystemFlags, regs);
						case xnative::XCONFIG_SECURED_POWER_MODE: return ExtractSetting(GConfig.PowerMode, regs);
						case xnative::XCONFIG_SECURED_ONLINE_NETWORK_ID: return ExtractSetting(GConfig.OnlineNetworkID, regs);
						case xnative::XCONFIG_SECURED_POWER_VCS_CONTROL: return ExtractSetting(GConfig.PowerVcsControl, regs);
						default: break;
					}

					break;
				}

				case xnative::XCONFIG_USER_CATEGORY:
				{
					switch (settingNum)
					{
						case xnative::XCONFIG_USER_VIDEO_FLAGS: return ExtractSetting((uint32)0x7, regs);
					}
				}

				default:
					break;
			}

			// invalid config settings
			RETURN_ARG(ERROR_INVALID_DATA);
		}

		void RegisterXboxConfig(runtime::Symbols& symbols)
		{
			REGISTER(ExGetXConfigSetting);
		}

	} // lib
} // xenon