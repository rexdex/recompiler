#include "build.h"
#include "xenonUserManager.h"
#include "xenonPlatform.h"
#include "xenonMemory.h"

namespace xenon
{

	UserProfile::UserProfile(const uint64_t id, const std::string& name)
		: m_uid(id)
		, m_name(name)
	{
		CreateDefaultEntries();
	}

	void UserProfile::RemoveSetting(const uint32_t id)
	{
		for (auto it = m_entries.begin(); it != m_entries.end(); ++it)
		{
			if ((*it).m_id == id)
			{
				m_entries.erase(it);
				break;
			}
		}
	}

	void UserProfile::AddSetting(const uint32_t id, const lib::X_USER_DATA_TYPE type, const void* src, const uint32_t size, const bool swapOnLoad)
	{
		// TODO: optimize 
		RemoveSetting(id);

		// create entry
		Entry entry;
		entry.m_id = id;
		entry.m_type = type;

		// set value
		switch (type)
		{
			case lib::X_USER_DATA_TYPE::XUSER_DATA_TYPE_FLOAT:
			case lib::X_USER_DATA_TYPE::XUSER_DATA_TYPE_INT32:
			{
				DEBUG_CHECK(size == 4);
				if (swapOnLoad)
					entry.data.b32Data = cpu::mem::load<uint32>(src);
				else
					entry.data.b32Data = *(uint32*)src;
				break;
			}

			case lib::X_USER_DATA_TYPE::XUSER_DATA_TYPE_DOUBLE:
			case lib::X_USER_DATA_TYPE::XUSER_DATA_TYPE_DATETIME:
			case lib::X_USER_DATA_TYPE::XUSER_DATA_TYPE_INT64:
			{
				DEBUG_CHECK(size == 8);
				if (swapOnLoad)
					entry.data.b64Data = cpu::mem::load<uint64>(src);
				else
					entry.data.b64Data = *(uint64*)src;
				break;
			}

			case lib::X_USER_DATA_TYPE::XUSER_DATA_TYPE_UNICODE:
			{
				uint32_t len = 0;
				for (;;)
				{
					const auto readPtr = (wchar_t*)src + len;
					const auto data = swapOnLoad ? cpu::mem::load<wchar_t>(readPtr) : *readPtr;

					if (!data)
						break;
					++len;
				}

				const auto memorySize = (len + 1) * sizeof(wchar_t);
				entry.data.blob.resize((len + 1) * sizeof(wchar_t));

				if (swapOnLoad)
				{
					const auto* readPtr = (const wchar_t*)src;
					auto* writePtr = (wchar_t*)entry.data.blob.data();

					for (uint32 i = 0; i <= len; ++i, ++readPtr, ++writePtr)
					{
						*writePtr = cpu::mem::load<wchar_t>(readPtr);
					}
				}
				else
				{
					memcpy(entry.data.blob.data(), src, memorySize);
				}				
				break;
			}

			case lib::X_USER_DATA_TYPE::XUSER_DATA_TYPE_BINARY:
			{
				entry.data.blob.resize(size);
				memcpy(entry.data.blob.data(), src, size);
				break;
			}
		}
	}

	//---
	
	void UserProfile::CreateDefaultEntries()
	{
		// http://cs.rin.ru/forum/viewtopic.php?f=38&t=60668&hilit=gfwl+live&start=195
		// https://github.com/arkem/py360/blob/master/py360/constants.py
		// https://github.com/benvanik/xenia/blob/bc0ddbb05a9872a72d6af0afc5caa464ddb1a417/src/xenia/kernel/xam/user_profile.cc
		// XPROFILE_GAMER_YAXIS_INVERSION
		AddSetting(0x10040002, (int32_t)0);
		// XPROFILE_OPTION_CONTROLLER_VIBRATION
		AddSetting(0x10040003, (int32_t)3);
		// XPROFILE_GAMERCARD_ZONE
		AddSetting(0x10040004, (int32_t)0);
		// XPROFILE_GAMERCARD_REGION
		AddSetting(0x10040005, (int32_t)0);
		// XPROFILE_GAMERCARD_CRED
		AddSetting(0x10040006, (int32_t)0xFA);
		// XPROFILE_GAMERCARD_REP
		AddSetting(0x5004000B, (float)0.0f);
		// XPROFILE_OPTION_VOICE_MUTED
		AddSetting(0x1004000C, (int32_t)0);
		// XPROFILE_OPTION_VOICE_THRU_SPEAKERS
		AddSetting(0x1004000D, (int32_t)0);
		// XPROFILE_OPTION_VOICE_VOLUME
		AddSetting(0x1004000E, (int32_t)0x64);
		// XPROFILE_GAMERCARD_MOTTO
		AddSetting(0x402C0011, L"");
		// XPROFILE_GAMERCARD_TITLES_PLAYED
		AddSetting(0x10040012, (int32_t)1);
		// XPROFILE_GAMERCARD_ACHIEVEMENTS_EARNED
		AddSetting(0x10040013, (int32_t)0);
		// XPROFILE_GAMER_DIFFICULTY
		AddSetting(0x10040015, (int32_t)0);
		// XPROFILE_GAMER_CONTROL_SENSITIVITY
		AddSetting(0x10040018, (int32_t)0);
		// Preferred color 1
		AddSetting(0x1004001D, (int32_t)0xFFFF0000u);
		// Preferred color 2
		AddSetting(0x1004001E, (int32_t)0xFF00FF00u);
		// XPROFILE_GAMER_ACTION_AUTO_AIM
		AddSetting(0x10040022, (int32_t)1);
		// XPROFILE_GAMER_ACTION_AUTO_CENTER
		AddSetting(0x10040023, (int32_t)0);
		// XPROFILE_GAMER_ACTION_MOVEMENT_CONTROL
		AddSetting(0x10040024, (int32_t)0);
		// XPROFILE_GAMER_RACE_TRANSMISSION
		AddSetting(0x10040026, (int32_t)0);
		// XPROFILE_GAMER_RACE_CAMERA_LOCATION
		AddSetting(0x10040027, (int32_t)0);
		// XPROFILE_GAMER_RACE_BRAKE_CONTROL
		AddSetting(0x10040028, (int32_t)0);
		// XPROFILE_GAMER_RACE_ACCELERATOR_CONTROL
		AddSetting(0x10040029, (int32_t)0);
		// XPROFILE_GAMERCARD_TITLE_CRED_EARNED
		AddSetting(0x10040038, (int32_t)0);
		// XPROFILE_GAMERCARD_TITLE_ACHIEVEMENTS_EARNED
		AddSetting(0x10040039, (int32_t)0);

		// If we set this, games will try to get it.
		// XPROFILE_GAMERCARD_PICTURE_KEY
		AddSetting(0x4064000F, L"gamercard_picture_key");

		// XPROFILE_TITLE_SPECIFIC1
		AddSetting(0x63E83FFF, lib::X_USER_DATA_TYPE::XUSER_DATA_TYPE_BINARY, nullptr, 0, true);
		AddSetting(0x63E83FFE, lib::X_USER_DATA_TYPE::XUSER_DATA_TYPE_BINARY, nullptr, 0, true);
		AddSetting(0x63E83FFD, lib::X_USER_DATA_TYPE::XUSER_DATA_TYPE_BINARY, nullptr, 0, true);
	}

	const bool UserProfile::GetSetting(const uint32_t id, const lib::X_USER_DATA_TYPE type, void* outDest, const uint32_t size, const bool swapOnStore)
	{
		return false;
	}

	//---

	UserProfileManager::UserProfileManager()
	{
		memset(m_currentUser, 0, sizeof(m_currentUser));

		// create default user
		auto* defaultUser = new UserProfile(DEFAULT_USER_UID, "DefaultUser");
		m_allUsers.push_back(defaultUser);
	}

	UserProfileManager::~UserProfileManager()
	{
		for (uint32_t i=0; i<MAX_USERS; ++i)
			SignOutUser(i);

		utils::ClearPtr(m_allUsers);
	}

	UserProfile* UserProfileManager::FindUser(const uint64_t uid)
	{
		// TODO: optimize
		for (auto* user : m_allUsers)
			if (user->GetID() == uid)
				return user;

		// not found
		return nullptr;
	}

	UserProfile* UserProfileManager::GetUser(const uint32_t userIndex)
	{
		DEBUG_CHECK(userIndex < MAX_USERS);
		return m_currentUser[userIndex];
	}

	const bool UserProfileManager::SignInUser(const uint32_t userIndex, const uint64_t uid)
	{
		DEBUG_CHECK(userIndex < MAX_USERS);

		// find user
		auto* user = FindUser(uid);
		if (!user)
			return false;

		// do nothing if signing in again
		if (user == m_currentUser[userIndex])
			return true;

		// sign out previous user
		SignOutUser(userIndex);

		// sign in the user
		m_currentUser[userIndex] = user;
		GLog.Log("XAM: User '%hs' signed in as user %u", user->GetName().c_str(), userIndex);

		// send system-wide notification
		const auto validUserMask = GetValidUserMask();
		GPlatform.GetKernel().PostEventNotification(lib::XN_SYS_SIGNINCHANGED.GetCode(), validUserMask);

		return true;
	}

	uint32_t UserProfileManager::GetValidUserMask() const
	{
		uint32_t ret = 0;

		for (uint32_t i = 0; i < MAX_USERS; ++i)
		{
			if (m_currentUser[i] != nullptr)
			{
				ret |= 1 << i;
			}
		}

		return ret;
	}

	const bool UserProfileManager::SignOutUser(const uint32_t userIndex)
	{
		DEBUG_CHECK(userIndex < MAX_USERS);

		auto* user = m_currentUser[userIndex];
		if (user != nullptr)
		{
			GLog.Log("XAM: User '%hs' signed out from slot %u", user->GetName().c_str(), userIndex);
			m_currentUser[userIndex] = 0;

			// send system-wide notification
			const auto validUserMask = GetValidUserMask();
			GPlatform.GetKernel().PostEventNotification(lib::XN_SYS_SIGNINCHANGED.GetCode(), validUserMask);

			return true;
		}

		return false;
	}

	//---

} // xenon

