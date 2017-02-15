#include "build.h"
#include "xenonLibNatives.h" 
#include "xenonLibUtils.h" 

//---------------------------------------------------------------------------

namespace xnative
{
	XCONFIG_SECURED_SETTINGS GXConfigSecuredSettings;

	XCONFIG_SECURED_SETTINGS& XGetConfigSecuredSettings()
	{
		return GXConfigSecuredSettings;
	}

	void InitConfig()
	{
		memset(&GXConfigSecuredSettings, 0, sizeof(GXConfigSecuredSettings));

		// setup defaults
		GXConfigSecuredSettings.AVRegion = 0x00001000; // CAN/US
		GXConfigSecuredSettings.MACAddress[0] = 0xAB;
		GXConfigSecuredSettings.MACAddress[1] = 0x0E;
		GXConfigSecuredSettings.MACAddress[2] = 0x55;
		GXConfigSecuredSettings.MACAddress[3] = 0x44;
		GXConfigSecuredSettings.MACAddress[4] = 0xCA;
		GXConfigSecuredSettings.MACAddress[5] = 0x16;
		GXConfigSecuredSettings.DVDRegion = 1; // CAN/US
	}

	XenonNativeData::XenonNativeData()
		: m_data( nullptr )
	{
	}

	XenonNativeData::~XenonNativeData()
	{
	}

	void XenonNativeData::Alloc(native::IMemory& memory, const uint32 size)
	{
		m_data = (uint8*) memory.VirtualAlloc(nullptr, size, native::IMemory::eAlloc_Commit | native::IMemory::eAlloc_Reserve, native::IMemory::eFlags_ReadWrite);
		memset(m_data, 0, size);
	}

};

//---------------------------------------------------------------------------

void InitializeXboxConfig()
{
	xnative::InitConfig();
}

//---------------------------------------------------------------------------

