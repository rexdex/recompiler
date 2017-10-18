#pragma once

namespace xenon
{
	//---

	/// user profile representation
	class UserProfile
	{
	public:
		UserProfile(const uint64_t id, const std::string& name);

		inline const uint64_t GetID() const { return m_uid; }

		inline const std::string& GetName() const { return m_name; }

		//---

		/// remove setting from profile
		void RemoveSetting(const uint32_t id);

		/// add setting to config
		void AddSetting(const uint32_t id, const xnative::X_USER_DATA_TYPE type, const void* src, const uint32_t size, const bool swapOnLoad);

		inline void AddSetting(const uint32_t id, const int32_t value)
		{
			AddSetting(id, xnative::X_USER_DATA_TYPE::XUSER_DATA_TYPE_INT32, &value, sizeof(value), false);
		}

		inline void AddSetting(const uint32_t id, const int64_t value)
		{
			AddSetting(id, xnative::X_USER_DATA_TYPE::XUSER_DATA_TYPE_INT64, &value, sizeof(value), false);
		}

		inline void AddSetting(const uint32_t id, const float value)
		{
			AddSetting(id, xnative::X_USER_DATA_TYPE::XUSER_DATA_TYPE_FLOAT, &value, sizeof(value), false);
		}

		inline void AddSetting(const uint32_t id, const double value)
		{
			AddSetting(id, xnative::X_USER_DATA_TYPE::XUSER_DATA_TYPE_DOUBLE, &value, sizeof(value), false);
		}

		inline void AddSetting(const uint32_t id, const wchar_t* value)
		{
			AddSetting(id, xnative::X_USER_DATA_TYPE::XUSER_DATA_TYPE_UNICODE, &value, sizeof(value), false);
		}

		inline void AddSetting(const uint32_t id, const uint64_t value)
		{
			AddSetting(id, xnative::X_USER_DATA_TYPE::XUSER_DATA_TYPE_DATETIME, &value, sizeof(value), false);
		}

		//---

		/// get value from config
		const bool GetSetting(const uint32_t id, const xnative::X_USER_DATA_TYPE type, void* outDest, const uint32_t size, const bool swapOnStore);

		//---

	private:
		static const uint32_t MAX_ENTRIES = 128;

		uint64_t m_uid;
		std::string m_name;

		struct Entry
		{
			uint32 m_id;
			xnative::X_USER_DATA_TYPE m_type;

			struct
			{
				uint32_t b32Data;
				uint64_t b64Data;
				std::vector<uint8> blob;
			} data;
		};

		std::vector<Entry> m_entries;

		void CreateDefaultEntries();
	};

	//---

	// manages user profiles
	class UserProfileManager
	{
	public:
		static const uint32_t MAX_USERS = 4;
		static const uint64_t DEFAULT_USER_UID = 0xDEAD0000BEAF0000;

		UserProfileManager();
		~UserProfileManager();

		/// find user by given UID
		UserProfile* FindUser(const uint64_t uid);

		/// get the user for given user index
		/// returns 0 if user is not signed in
		UserProfile* GetUser(const uint32_t userIndex);

		/// sign-in user with given ID
		/// NOTE: sends notification
		const bool SignInUser(const uint32_t userIndex, const uint64_t uid);

		/// sign-out user from given user slot
		/// NOTE: sends notification
		const bool SignOutUser(const uint32_t userIndex);

	private:
		typedef std::vector<UserProfile*> TAllUsers;
		TAllUsers m_allUsers;

		UserProfile* m_currentUser[MAX_USERS];
	};

	//---


} // xenon