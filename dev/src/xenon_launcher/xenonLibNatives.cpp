#include "build.h"
#include "xenonLibNatives.h" 
#include "xenonLib.h"  
#include "xenonPlatform.h"
#include "xenonMemory.h"
#include "xenonBindings.h"

//---------------------------------------------------------------------------

namespace xenon
{
	namespace lib
	{

		XenonNativeData::XenonNativeData()
			: m_data(nullptr)
		{
		}

		XenonNativeData::~XenonNativeData()
		{
		}

		void XenonNativeData::Alloc(const uint32 size)
		{
			m_data = (uint8*)GPlatform.GetMemory().VirtualAlloc(nullptr, size, XMEM_COMMIT | XMEM_RESERVE, XPAGE_READWRITE);
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

		/*
		void X_FILE_INFO::Write(uint8* base, uint32 p)
		{
			cpu::mem::store<uint64>(base + p + 0, CreationTime);
			cpu::mem::store<uint64>(base + p + 8, LastAccessTime);
			cpu::mem::store<uint64>(base + p + 16, LastWriteTime);
			cpu::mem::store<uint64>(base + p + 24, ChangeTime);
			cpu::mem::store<uint64>(base + p + 32, AllocationSize);
			cpu::mem::store<uint64>(base + p + 40, FileLength);
			cpu::mem::store<uint32>(base + p + 48, Attributes);
			cpu::mem::store<uint32>(base + p + 52, Padding);
		}

		void X_DIR_INFO::Write(uint8* base, uint32 p)
		{
			uint8* dst = base + p;
			uint8* src = (uint8*)this;
			X_DIR_INFO* info;
			do
			{
				info = (X_DIR_INFO*)src;
				cpu::mem::store<uint32>(dst, info->NextEntryOffset);
				cpu::mem::store<uint32>(dst + 4, info->FileIndex);
				cpu::mem::store<uint64>(dst + 8, info->CreationTime);
				cpu::mem::store<uint64>(dst + 16, info->LastAccessTime);
				cpu::mem::store<uint64>(dst + 24, info->LastWriteTime);
				cpu::mem::store<uint64>(dst + 32, info->ChangeTime);
				cpu::mem::store<uint64>(dst + 40, info->EndOfFile);
				cpu::mem::store<uint64>(dst + 48, info->AllocationSize);
				cpu::mem::store<uint32>(dst + 56, info->Attributes);
				cpu::mem::store<uint32>(dst + 60, info->FileNameLength);
				memcpy(dst + 64, info->FileName, info->FileNameLength);
				xenon::TagMemoryWrite(dst, 64 + info->FileNameLength, "X_DIR_INFO");
				dst += info->NextEntryOffset;
				src += info->NextEntryOffset;
			} while (info->NextEntryOffset != 0);
		}

		void X_VOLUME_INFO::Write(uint8* base, uint32 p)
		{
			uint8* dst = base + p;
			cpu::mem::store<uint64>(dst + 0, this->CreationTime);
			cpu::mem::store<uint32>(dst + 8, this->SerialNumber);
			cpu::mem::store<uint32>(dst + 12, this->LabelLength);
			cpu::mem::store<uint32>(dst + 16, this->SupportsObjects);
			memcpy(dst + 20, this->Label, this->LabelLength);
			xenon::TagMemoryWrite(dst, 20 + this->LabelLength, "X_VOLUME_INFO");
		}

		void X_FILE_SYSTEM_ATTRIBUTE_INFO::Write(uint8* base, uint32 p)
		{
			uint8* dst = base + p;
			cpu::mem::store<uint32>(dst + 0, this->Attributes);
			cpu::mem::store<uint32>(dst + 4, this->MaximumComponentNameLength);
			cpu::mem::store<uint32>(dst + 8, this->FSNameLength);
			memcpy(dst + 12, this->FSName, this->FSNameLength);
			xenon::TagMemoryWrite(dst, 12 + this->FSNameLength, "X_FILE_SYSTEM_ATTRIBUTE_INFO");
		}

		*/

	} // lib
} // xenon