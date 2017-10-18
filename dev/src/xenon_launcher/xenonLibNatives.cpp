#include "build.h"
#include "xenonLibNatives.h" 
#include "xenonLibUtils.h" 
#include "xenonPlatform.h"
#include "xenonMemory.h"

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

	void XenonNativeData::Alloc(const uint32 size)
	{
		m_data = (uint8*) GPlatform.GetMemory().VirtualAlloc(nullptr, size, xnative::XMEM_COMMIT | xnative::XMEM_RESERVE, xnative::XPAGE_READWRITE);
		memset(m_data, 0, size);
	}

	//--

	const NotificationID XN_SYS_UI(0, X_NOTIFY_AREA::SYSTEM, 0x0009);
	const NotificationID XN_SYS_SIGNINCHANGED(0, X_NOTIFY_AREA::SYSTEM, 0x000a);
	const NotificationID XN_SYS_STORAGEDEVICESCHANGED(0, X_NOTIFY_AREA::SYSTEM, 0x000b);
	const NotificationID XN_SYS_PROFILESETTINGCHANGED(0, X_NOTIFY_AREA::SYSTEM, 0x000e);
	const NotificationID XN_SYS_MUTELISTCHANGED(0, X_NOTIFY_AREA::SYSTEM, 0x0011);
	const NotificationID XN_SYS_INPUTDEVICESCHANGED(0, X_NOTIFY_AREA::SYSTEM, 0x0012);
	const NotificationID XN_SYS_INPUTDEVICECONFIGCHANGED(1, X_NOTIFY_AREA::SYSTEM, 0x0013);
	const NotificationID XN_SYS_PLAYTIMERNOTICE(3, X_NOTIFY_AREA::SYSTEM, 0x0015);
	const NotificationID XN_SYS_AVATARCHANGED(4, X_NOTIFY_AREA::SYSTEM, 0x0017);
	const NotificationID XN_SYS_NUIHARDWARESTATUSCHANGED(6, X_NOTIFY_AREA::SYSTEM, 0x0019);
	const NotificationID XN_SYS_NUIPAUSE(6, X_NOTIFY_AREA::SYSTEM, 0x001a);
	const NotificationID XN_SYS_NUIUIAPPROACH(6, X_NOTIFY_AREA::SYSTEM, 0x001b);
	const NotificationID XN_SYS_DEVICEREMAP(6, X_NOTIFY_AREA::SYSTEM, 0x001c);
	const NotificationID XN_SYS_NUIBINDINGCHANGED(6, X_NOTIFY_AREA::SYSTEM, 0x001d);
	const NotificationID XN_SYS_AUDIOLATENCYCHANGED(8, X_NOTIFY_AREA::SYSTEM, 0x001e);
	const NotificationID XN_SYS_NUICHATBINDINGCHANGED(8, X_NOTIFY_AREA::SYSTEM, 0x001f);
	const NotificationID XN_SYS_INPUTACTIVITYCHANGED(9, X_NOTIFY_AREA::SYSTEM, 0x0020);
	const NotificationID XN_LIVE_CONNECTIONCHANGED(0, X_NOTIFY_AREA::LIVE, 0x0001);
	const NotificationID XN_LIVE_INVITE_ACCEPTED(0, X_NOTIFY_AREA::LIVE, 0x0002);
	const NotificationID XN_LIVE_LINK_STATE_CHANGED(0, X_NOTIFY_AREA::LIVE, 0x0003);
	const NotificationID XN_LIVE_CONTENT_INSTALLED(0, X_NOTIFY_AREA::LIVE, 0x0007);
	const NotificationID XN_LIVE_MEMBERSHIP_PURCHASED(0, X_NOTIFY_AREA::LIVE, 0x0008);
	const NotificationID XN_LIVE_VOICECHAT_AWAY(0, X_NOTIFY_AREA::LIVE, 0x0009);
	const NotificationID XN_LIVE_PRESENCE_CHANGED(0, X_NOTIFY_AREA::LIVE, 0x000A);
	const NotificationID XN_FRIENDS_PRESENCE_CHANGED(0, X_NOTIFY_AREA::FRIENDS, 0x0001);
	const NotificationID XN_FRIENDS_FRIEND_ADDED(0, X_NOTIFY_AREA::FRIENDS, 0x0002);
	const NotificationID XN_FRIENDS_FRIEND_REMOVED(0, X_NOTIFY_AREA::FRIENDS, 0x0003);
	const NotificationID XN_CUSTOM_ACTIONPRESSED(0, X_NOTIFY_AREA::CUSTOM, 0x0003);
	const NotificationID XN_CUSTOM_GAMERCARD(1, X_NOTIFY_AREA::CUSTOM, 0x0004);
	const NotificationID XN_XMP_STATECHANGED(0, X_NOTIFY_AREA::XMP, 0x0001);
	const NotificationID XN_XMP_PLAYBACKBEHAVIORCHANGED(0, X_NOTIFY_AREA::XMP, 0x0002);
	const NotificationID XN_XMP_PLAYBACKCONTROLLERCHANGED(0, X_NOTIFY_AREA::XMP, 0x0003);
	const NotificationID XN_PARTY_MEMBERS_CHANGED(4, X_NOTIFY_AREA::PARTY, 0x0002);

	//--

};

//---------------------------------------------------------------------------

void InitializeXboxConfig()
{
	xnative::InitConfig();
}

//---------------------------------------------------------------------------

