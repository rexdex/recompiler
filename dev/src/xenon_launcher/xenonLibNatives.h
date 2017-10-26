#pragma once

#include "xenonLib.h"

//---------------------------------------------------------------------------

namespace xenon
{
	namespace lib
	{
		typedef uint64 HRESULT;
		typedef uint32 X_STATUS;
		typedef uint32 X_FILE;
		
		/// native data wrapper
		class XenonNativeData
		{
		public:
			XenonNativeData();
			~XenonNativeData();

			void Alloc(const uint32 size);

			inline const uint8* Data() const
			{
				return &m_data[0];
			}

			inline uint8* Data()
			{
				return &m_data[0];
			}

			template< typename T >
			inline void Write(const uint32 offset, const T& val)
			{
				cpu::mem::store(&m_data[offset], val);
			}

			template< typename T >
			inline T Read(const uint32 offset) const
			{
				return cpu::mem::load(&m_data[offset]);
			}

		private:
			uint8*		m_data;
		};

		enum XStatus
		{
			X_STATUS_SUCCESS = ((X_STATUS)0x00000000L),
			X_STATUS_ABANDONED_WAIT_0 = ((X_STATUS)0x00000080L),
			X_STATUS_USER_APC = ((X_STATUS)0x000000C0L),
			X_STATUS_ALERTED = ((X_STATUS)0x00000101L),
			X_STATUS_TIMEOUT = ((X_STATUS)0x00000102L),
			X_STATUS_PENDING = ((X_STATUS)0x00000103L),
			X_STATUS_TIMER_RESUME_IGNORED = ((X_STATUS)0x40000025L),
			X_STATUS_BUFFER_OVERFLOW = ((X_STATUS)0x80000005L),
			X_STATUS_NO_MORE_FILES = ((X_STATUS)0x80000006L),
			X_STATUS_UNSUCCESSFUL = ((X_STATUS)0xC0000001L),
			X_STATUS_NOT_IMPLEMENTED = ((X_STATUS)0xC0000002L),
			X_STATUS_INFO_LENGTH_MISMATCH = ((X_STATUS)0xC0000004L),
			X_STATUS_ACCESS_VIOLATION = ((X_STATUS)0xC0000005L),
			X_STATUS_INVALID_HANDLE = ((X_STATUS)0xC0000008L),
			X_STATUS_INVALID_PARAMETER = ((X_STATUS)0xC000000DL),
			X_STATUS_NO_SUCH_FILE = ((X_STATUS)0xC000000FL),
			X_STATUS_END_OF_FILE = ((X_STATUS)0xC0000011L),
			X_STATUS_NO_MEMORY = ((X_STATUS)0xC0000017L),
			X_STATUS_ALREADY_COMMITTED = ((X_STATUS)0xC0000021L),
			X_STATUS_ACCESS_DENIED = ((X_STATUS)0xC0000022L),
			X_STATUS_BUFFER_TOO_SMALL = ((X_STATUS)0xC0000023L),
			X_STATUS_OBJECT_TYPE_MISMATCH = ((X_STATUS)0xC0000024L),
			X_STATUS_INVALID_PAGE_PROTECTION = ((X_STATUS)0xC0000045L),
			X_STATUS_MUTANT_NOT_OWNED = ((X_STATUS)0xC0000046L),
			X_STATUS_MEMORY_NOT_ALLOCATED = ((X_STATUS)0xC00000A0L),
			X_STATUS_INVALID_PARAMETER_1 = ((X_STATUS)0xC00000EFL),
			X_STATUS_INVALID_PARAMETER_2 = ((X_STATUS)0xC00000F0L),
			X_STATUS_INVALID_PARAMETER_3 = ((X_STATUS)0xC00000F1L),
		};

		enum XFile
		{
			X_FILE_SUPERSEDED = ((X_FILE)0x00000000),
			X_FILE_OPENED = ((X_FILE)0x00000001),
			X_FILE_CREATED = ((X_FILE)0x00000002),
			X_FILE_OVERWRITTEN = ((X_FILE)0x00000003),
			X_FILE_EXISTS = ((X_FILE)0x00000004),
			X_FILE_DOES_NOT_EXIST = ((X_FILE)0x00000005),
		};

		enum XFileMode
		{
			CreateAlways = 0,
			Open = 1,
			Create = 2,
			OpenIf = 3,
			Overwrite = 4,
			OverwriteIf = 5,
		};

		// HRESULT (ERROR_*)
		typedef uint32 X_RESULT;
#define X_FACILITY_WIN32 7
#define X_HRESULT_FROM_WIN32(x) ((X_RESULT)(x) <= 0 ? ((X_RESULT)(x)) : ((X_RESULT) (((x) & 0x0000FFFF) | (X_FACILITY_WIN32 << 16) | 0x80000000)))
		enum XResult
		{
			X_ERROR_SUCCESS = X_HRESULT_FROM_WIN32(0x00000000L),
			X_ERROR_ACCESS_DENIED = X_HRESULT_FROM_WIN32(0x00000005L),
			X_ERROR_NO_MORE_FILES = X_HRESULT_FROM_WIN32(0x00000018L),
			X_ERROR_INSUFFICIENT_BUFFER = X_HRESULT_FROM_WIN32(0x0000007AL),
			X_ERROR_BAD_ARGUMENTS = X_HRESULT_FROM_WIN32(0x000000A0L),
			X_ERROR_BUSY = X_HRESULT_FROM_WIN32(0x000000AAL),
			X_ERROR_DEVICE_NOT_CONNECTED = X_HRESULT_FROM_WIN32(0x0000048FL),
			X_ERROR_NOT_FOUND = X_HRESULT_FROM_WIN32(0x00000490L),
			X_ERROR_CANCELLED = X_HRESULT_FROM_WIN32(0x000004C7L),
			X_ERROR_NO_SUCH_USER = X_HRESULT_FROM_WIN32(0x00000525L),
			X_ERROR_EMPTY = X_HRESULT_FROM_WIN32(0x000010D2L),
		};

		// used by KeGetCurrentProcessType
		enum XProcessType
		{
			X_PROCTYPE_IDLE = 0,
			X_PROCTYPE_USER = 1,
			X_PROCTYPE_SYSTEM = 2,
		};

		enum XVirtualMemFlags
		{
			XMEM_COMMIT = 0x1000,
			XMEM_RESERVE = 0x2000,
			XMEM_DECOMMIT = 0x4000,
			XMEM_RELEASE = 0x8000,
			XMEM_FREE = 0x10000,
			XMEM_PRIVATE = 0x20000,
			XMEM_RESET = 0x80000,
			XMEM_TOP_DOWN = 0x100000,
			XMEM_NOZERO = 0x800000,
			XMEM_LARGE_PAGES = 0x20000000,
			XMEM_HEAP = 0x40000000,
			XMEM_16MB_PAGES = 0x80000000,
		};

		enum XPageFlags
		{
			XPAGE_NOACCESS = 0x01,
			XPAGE_READONLY = 0x02,
			XPAGE_READWRITE = 0x04,
			XPAGE_WRITECOPY = 0x08,
			XPAGE_EXECUTE = 0x10,
			XPAGE_EXECUTE_READ = 0x20,
			XPAGE_EXECUTE_READWRITE = 0x40,
			XPAGE_EXECUTE_WRITECOPY = 0x80,
			XPAGE_GUARD = 0x100,
			XPAGE_NOCACHE = 0x200,
			XPAGE_WRITECOMBINE = 0x400,
			XPAGE_USER_READONLY = 0x1000,
			XPAGE_USER_READWRITE = 0x2000,
		};

		enum XMemoryProtectFlag
		{
			XPROTECT_Read = 1 << 0,
			XPROTECT_Write = 1 << 1,
			XPROTECT_NoCache = 1 << 2,
			XPROTECT_WriteCombine = 1 << 3,
			XPROTECT_NoAccess = 0,
		};

		struct XVIDEO_MODE
		{
			Field<uint32> dwDisplayWidth;
			Field<uint32> dwDisplayHeight;
			Field<uint32> fIsInterlaced; // bool
			Field<uint32> fIsWideScreen; // bool
			Field<uint32> fIsHiDef; // bool
			Field<float> RefreshRate;
			Field<uint32> VideoStandard;
			Field<uint32> ReservedA;
			Field<uint32> ReservedB;
			Field<uint32> ReservedC;
			Field<uint32> ReservedD;
			Field<uint32> ReservedE;
		};

		enum EXCVideoStandard
		{
			XC_VIDEO_STANDARD_NTSC_M = 1,
			XC_VIDEO_STANDARD_NTSC_J = 2,
			XC_VIDEO_STANDARD_PAL_I = 3,
		};

		enum EXCVideoFlags
		{
			XC_VIDEO_FLAGS_WIDESCREEN = 0x00000001,
			XC_VIDEO_FLAGS_HDTV_720p = 0x00000002,
			XC_VIDEO_FLAGS_HDTV_480p = 0x00000008,
			XC_VIDEO_FLAGS_HDTV_1080i = 0x00000004,
		};

		const char XConfigNames[][38] =
		{
			"XCONFIG_STATIC_CATEGORY",
			"XCONFIG_STATISTIC_CATEGORY",
			"XCONFIG_SECURED_CATEGORY",
			"XCONFIG_USER_CATEGORY",
			"XCONFIG_XNET_MACHINE_ACCOUNT_CATEGORY",
			"XCONFIG_XNET_PARAMETERS_CATEGORY",
			"XCONFIG_MEDIA_CENTER_CATEGORY",
			"XCONFIG_CONSOLE_CATEGORY",
			"XCONFIG_DVD_CATEGORY",
			"XCONFIG_IPTV_CATEGORY",
			"XCONFIG_SYSTEM_CATEGORY"
		};

		enum XCONFIG_CATEGORY_TYPES
		{
			XCONFIG_STATIC_CATEGORY = 0x0,					//_XCONFIG_STATIC_SETTINGS
			XCONFIG_STATISTIC_CATEGORY = 0x1,				//_XCONFIG_STATISTIC_SETTINGS
			XCONFIG_SECURED_CATEGORY = 0x2,					//_XCONFIG_SECURED_SETTINGS
			XCONFIG_USER_CATEGORY = 0x3,					//_XCONFIG_USER_SETTINGS
			XCONFIG_XNET_MACHINE_ACCOUNT_CATEGORY = 0x4,	//_XCONFIG_XNET_SETTINGS
			XCONFIG_XNET_PARAMETERS_CATEGORY = 0x5,			//_XCONFIG_XNET_SETTINGS
			XCONFIG_MEDIA_CENTER_CATEGORY = 0x6,			//_XCONFIG_MEDIA_CENTER_SETTINGS
			XCONFIG_CONSOLE_CATEGORY = 0x7, 				//_XCONFIG_CONSOLE_SETTINGS
			XCONFIG_DVD_CATEGORY = 0x8, 					//_XCONFIG_DVD_SETTINGS
			XCONFIG_IPTV_CATEGORY = 0x9, 					//_XCONFIG_IPTV_SETTINGS
			XCONFIG_SYSTEM_CATEGORY = 0xa,					//_XCONFIG_SYSTEM_SETTINGS
			XCONFIG_CATEGORY_MAX = 0xb,
		};

		enum XCONFIG_SECURED_ENTRIES
		{
			XCONFIG_SECURED_DATA = 0x0,
			XCONFIG_SECURED_MAC_ADDRESS = 0x1,
			XCONFIG_SECURED_AV_REGION = 0x2,
			XCONFIG_SECURED_GAME_REGION = 0x3,
			XCONFIG_SECURED_DVD_REGION = 0x4,
			XCONFIG_SECURED_RESET_KEY = 0x5,
			XCONFIG_SECURED_SYSTEM_FLAGS = 0x6,
			XCONFIG_SECURED_POWER_MODE = 0x7,
			XCONFIG_SECURED_ONLINE_NETWORK_ID = 0x8,
			XCONFIG_SECURED_POWER_VCS_CONTROL = 0x9,
			XCONFIG_SECURED_ENTRIES_MAX = 0xa
		};

		enum XCONFIG_USER_ENTRIES
		{
			XCONFIG_USER_DATA = 0x0,
			XCONFIG_USER_TIME_ZONE_BIAS = 0x1,
			XCONFIG_USER_TIME_ZONE_STD_NAME = 0x2,
			XCONFIG_USER_TIME_ZONE_DLT_NAME = 0x3,
			XCONFIG_USER_TIME_ZONE_STD_DATE = 0x4,
			XCONFIG_USER_TIME_ZONE_DLT_DATE = 0x5,
			XCONFIG_USER_TIME_ZONE_STD_BIAS = 0x6,
			XCONFIG_USER_TIME_ZONE_DLT_BIAS = 0x7,
			XCONFIG_USER_DEFAULT_PROFILE = 0x8,
			XCONFIG_USER_LANGUAGE = 0x9,
			XCONFIG_USER_VIDEO_FLAGS = 0xa,
			XCONFIG_USER_AUDIO_FLAGS = 0xb,
			XCONFIG_USER_RETAIL_FLAGS = 0xc,
			XCONFIG_USER_DEVKIT_FLAGS = 0xd,
			XCONFIG_USER_COUNTRY = 0xe,
			XCONFIG_USER_PC_FLAGS = 0xf,
			XCONFIG_USER_SMB_CONFIG = 0x10,
			XCONFIG_USER_LIVE_PUID = 0x11,
			XCONFIG_USER_LIVE_CREDENTIALS = 0x12,
			XCONFIG_USER_AV_COMPOSITE_SCREENSZ = 0x13,
			XCONFIG_USER_AV_COMPONENT_SCREENSZ = 0x14,
			XCONFIG_USER_AV_VGA_SCREENSZ = 0x15,
			XCONFIG_USER_PC_GAME = 0x16,
			XCONFIG_USER_PC_PASSWORD = 0x17,
			XCONFIG_USER_PC_MOVIE = 0x18,
			XCONFIG_USER_PC_GAME_RATING = 0x19,
			XCONFIG_USER_PC_MOVIE_RATING = 0x1a,
			XCONFIG_USER_PC_HINT = 0x1b,
			XCONFIG_USER_PC_HINT_ANSWER = 0x1c,
			XCONFIG_USER_PC_OVERRIDE = 0x1d,
			XCONFIG_USER_MUSIC_PLAYBACK_MODE = 0x1e,
			XCONFIG_USER_MUSIC_VOLUME = 0x1f,
			XCONFIG_USER_MUSIC_FLAGS = 0x20,
			XCONFIG_USER_ARCADE_FLAGS = 0x21,
			XCONFIG_USER_PC_VERSION = 0x22,
			XCONFIG_USER_PC_TV = 0x23,
			XCONFIG_USER_PC_TV_RATING = 0x24,
			XCONFIG_USER_PC_EXPLICIT_VIDEO = 0x25,
			XCONFIG_USER_PC_EXPLICIT_VIDEO_RATING = 0x26,
			XCONFIG_USER_PC_UNRATED_VIDEO = 0x27,
			XCONFIG_USER_PC_UNRATED_VIDEO_RATING = 0x28,
			XCONFIG_USER_VIDEO_OUTPUT_BLACK_LEVELS = 0x29,
			XCONFIG_USER_VIDEO_PLAYER_DISPLAY_MODE = 0x2a,
			XCONFIG_USER_ALTERNATE_VIDEO_TIMING_ID = 0x2b,
			XCONFIG_USER_VIDEO_DRIVER_OPTIONS = 0x2c,
			XCONFIG_USER_MUSIC_UI_FLAGS = 0x2d,
			XCONFIG_USER_VIDEO_MEDIA_SOURCE_TYPE = 0x2e,
			XCONFIG_USER_MUSIC_MEDIA_SOURCE_TYPE = 0x2f,
			XCONFIG_USER_PHOTO_MEDIA_SOURCE_TYPE = 0x30,
			XCONFIG_USER_ENTRIES_MAX = 0x31
		};

		struct XCONFIG_POWER_MODE
		{
			uint8 VIDDelta;
			uint8 Reserved;
		};

		struct XCONFIG_POWER_VCS_CONTROL
		{
			uint16 Configured : 1;
			uint16 Reserved : 3;
			uint16 Full : 4;
			uint16 Quiet : 4;
			uint16 Fuse : 4;
		};

		struct XCRITICAL_SECTION
		{
			//
			//  The following field is used for blocking when there is contention for
			//  the resource
			//

			union {
				uint32	RawEvent[4];
			} Synchronization;

			//
			//  The following three fields control entering and exiting the critical
			//  section for the resource
			//

			uint32 LockCount;
			uint32 RecursionCount;
			uint32 OwningThread;
		};

		static_assert(sizeof(XCRITICAL_SECTION) == 28, "Critical section size mismatch");

#pragma pack(push)
#pragma pack(4)
		class X_ANSI_STRING
		{
		public:
			Field<uint16>	Length;
			Field<uint16>	MaximumLength;
			Field<Pointer<char>> BufferPtr;

			inline X_ANSI_STRING()
			{
				Length = 0;
				MaximumLength = 0;
				BufferPtr = nullptr;
			}

			inline std::string Duplicate() const
			{
				if (BufferPtr == nullptr || Length == 0)
					return "";

				const auto* srcStr = BufferPtr.AsData().Get().GetNativePointer();
				return std::string(srcStr);
			}
		};
#pragma pack(pop)

		static_assert(sizeof(X_ANSI_STRING) == 8, "Size error");

#pragma pack(push)
#pragma pack(4)
		class X_UNICODE_STRING
		{
		public:
			Field<uint16>	Length;
			Field<uint16>	MaximumLength;
			Field<Pointer<wchar_t>> BufferPtr;

			inline X_UNICODE_STRING()
			{
				Length = 0;
				MaximumLength = 0;
				BufferPtr = nullptr;
			}

			inline std::wstring Duplicate() const
			{
				if (BufferPtr == nullptr || Length == 0)
					return L"";

				std::wstring ret;
				ret.resize(Length);

				for (uint32 i = 0; i < Length; ++i)
					ret[i] = BufferPtr.AsData().Get()[i];

				return ret;
			}
		};
#pragma pack(pop)

		class X_OBJECT_ATTRIBUTES
		{
		public:
			Field<uint32> RootDirectory;
			Field<Pointer<X_ANSI_STRING>> ObjectName;
			Field<uint32> Attributes;

			X_OBJECT_ATTRIBUTES()
				: RootDirectory()
				, Attributes(0)
			{}
		};

		static_assert(sizeof(X_OBJECT_ATTRIBUTES) == 12, "Size error");

		// http://code.google.com/p/vdash/source/browse/trunk/vdash/include/kernel.h
		enum X_FILE_INFORMATION_CLASS
		{
			XFileDirectoryInformation = 1,
			XFileFullDirectoryInformation,
			XFileBothDirectoryInformation,
			XFileBasicInformation,
			XFileStandardInformation,
			XFileInternalInformation,
			XFileEaInformation,
			XFileAccessInformation,
			XFileNameInformation,
			XFileRenameInformation,
			XFileLinkInformation,
			XFileNamesInformation,
			XFileDispositionInformation,
			XFilePositionInformation,
			XFileFullEaInformation,
			XFileModeInformation,
			XFileAlignmentInformation,
			XFileAllInformation,
			XFileAllocationInformation,
			XFileEndOfFileInformation,
			XFileAlternateNameInformation,
			XFileStreamInformation,
			XFileMountPartitionInformation,
			XFileMountPartitionsInformation,
			XFilePipeRemoteInformation,
			XFileSectorInformation,
			XFileXctdCompressionInformation,
			XFileCompressionInformation,
			XFileObjectIdInformation,
			XFileCompletionInformation,
			XFileMoveClusterInformation,
			XFileIoPriorityInformation,
			XFileReparsePointInformation,
			XFileNetworkOpenInformation,
			XFileAttributeTagInformation,
			XFileTrackingInformation,
			XFileMaximumInformation
		};

		enum X_FILE_ATTRIBUTES : uint32
		{
			X_FILE_ATTRIBUTE_NONE = 0x0000,
			X_FILE_ATTRIBUTE_READONLY = 0x0001,
			X_FILE_ATTRIBUTE_HIDDEN = 0x0002,
			X_FILE_ATTRIBUTE_SYSTEM = 0x0004,
			X_FILE_ATTRIBUTE_DIRECTORY = 0x0010,
			X_FILE_ATTRIBUTE_ARCHIVE = 0x0020,
			X_FILE_ATTRIBUTE_DEVICE = 0x0040,
			X_FILE_ATTRIBUTE_NORMAL = 0x0080,
			X_FILE_ATTRIBUTE_TEMPORARY = 0x0100,
			X_FILE_ATTRIBUTE_COMPRESSED = 0x0800,
			X_FILE_ATTRIBUTE_ENCRYPTED = 0x4000,
		};

		class X_FILE_INFO
		{
		public:
			Field<uint64> CreationTime;
			Field<uint64> LastAccessTime;
			Field<uint64> LastWriteTime;
			Field<uint64> ChangeTime;
			Field<uint64> AllocationSize;
			Field<uint64> FileLength;
			Field<X_FILE_ATTRIBUTES> Attributes;
			Field<uint32> Padding;
		};
		static_assert(sizeof(X_FILE_INFO) == 56, "Structure size mismatch");

		class X_DIR_INFO
		{
		public:
			Field<uint32> NextEntryOffset;
			Field<uint32> FileIndex;
			Field<uint64> CreationTime;
			Field<uint64> LastAccessTime;
			Field<uint64> LastWriteTime;
			Field<uint64> ChangeTime;
			Field<uint64> EndOfFile;
			Field<uint64> AllocationSize;
			Field<X_FILE_ATTRIBUTES> Attributes;
			Field<uint32> FileNameLength;
			char FileName[1];
		};
		static_assert(sizeof(X_DIR_INFO) == 72, "Structure size mismatch");

		class X_VOLUME_INFO
		{
		public:
			Field<uint64> CreationTime;
			Field<uint32> SerialNumber;
			Field<uint32> LabelLength;
			Field<uint32> SupportsObjects;
			char Label[1];
		};
		static_assert(sizeof(X_VOLUME_INFO) == 24, "Structure size mismatch");

		class X_FILE_SYSTEM_ATTRIBUTE_INFO
		{
		public:
			Field<uint32> Attributes;
			Field<int32> MaximumComponentNameLength;
			Field<uint32> FSNameLength;
			char FSName[1];
		};
		static_assert(sizeof(X_FILE_SYSTEM_ATTRIBUTE_INFO) == 16, "Structure size mismatch");

		class X_FILE_IO_STATUS
		{
		public:
			Field<uint32> Result;
			Field<uint32> Info;
		};
		static_assert(sizeof(X_FILE_IO_STATUS) == 8, "Structure size mismatch");

		struct X_INPUT_GAMEPAD
		{
			Field<uint16> buttons;
			Field<uint8> left_trigger;
			Field<uint8> right_trigger;
			Field<uint16> thumb_lx;
			Field<uint16> thumb_ly;
			Field<uint16> thumb_rx;
			Field<uint16> thumb_ry;
		};
		static_assert(sizeof(X_INPUT_GAMEPAD) == 12, "Structure size mismatch");

		struct X_INPUT_STATE
		{
			Field<uint32> packet_number;
			X_INPUT_GAMEPAD gamepad;
		};
		static_assert(sizeof(X_INPUT_STATE) == 16, "Structure size mismatch");

		struct X_INPUT_VIBRATION
		{
			Field<uint16> left_motor_speed;
			Field<uint16> right_motor_speed;
		};

		struct X_INPUT_CAPABILITIES
		{
			Field<uint8> type;
			Field<uint8> sub_type;
			Field<uint16> flags;
			X_INPUT_GAMEPAD gamepad;
			X_INPUT_VIBRATION vibration;
		};

		struct X_INPUT_KEYSTROKE
		{
			Field<uint16_t> virtual_key;
			Field<uint16_t> unicode;
			Field<uint16_t> flags;
			Field<uint8_t> user_index;
			Field<uint8_t> hid_code;
		};

		enum class X_USER_DATA_TYPE : uint8
		{
			XUSER_DATA_TYPE_INT32 = 1, //32-bit integer
			XUSER_DATA_TYPE_INT64 = 2, //64-bit integer
			XUSER_DATA_TYPE_DOUBLE = 3, //A double floating - point value.
			XUSER_DATA_TYPE_UNICODE = 4, //Unicode string.
			XUSER_DATA_TYPE_FLOAT = 5, //A float floating - point value.
			XUSER_DATA_TYPE_BINARY = 6, //Binary data
			XUSER_DATA_TYPE_DATETIME = 7, //Date and time value
			XUSER_DATA_TYPE_CONTEXT
		};

		struct X_USER_DATA
		{
			X_USER_DATA_TYPE type;
			union
			{
				Field<uint32_t> nData;
				Field<uint64_t> i64Data;
				Field<double> dblData;
				struct {
					Field<uint32_t> cbData;
					Field<uint32_t> pwszData;
				} string;
				Field<float> fData;
				struct {
					Field<uint32_t> cbData;
					Field<uint32_t> pbData;
				} binary;
				Field<uint64_t> ftData;
			} data;
		};

		enum class X_NOTIFY_FLAGS : uint8_t
		{
			XNOTIFY_SYSTEM = 1,
			XNOTIFY_LIVE = 2,
			XNOTIFY_FRIENDS = 4,
			XNOTIFY_CUSTOM = 8,
			XNOTIFY_XMP = 0x20,
			XNOTIFY_MSGR = 0x40,
			XNOTIFY_PARTY = 0x80,
			XNOTIFY_ALL = XNOTIFY_SYSTEM | XNOTIFY_LIVE | XNOTIFY_FRIENDS | XNOTIFY_CUSTOM | XNOTIFY_XMP | XNOTIFY_MSGR | XNOTIFY_PARTY
		};

		enum class X_NOTIFY_AREA : uint8_t
		{
			SYSTEM = 0,
			LIVE = 1,
			FRIENDS = 2,
			CUSTOM = 3,
			XMP = 5,
			MSGR = 6,
			PARTY = 7,
		};

		//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
		//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
		//  +-+-----------+-----------------+-------------------------------+
		//  |R|    Area   |    Version      |            Index              |
		//  +-+-----+-----+-----------------+-------------------------------+
		class NotificationID
		{
		public:
			inline NotificationID()
			{
				m_code = 0;
			}

			inline NotificationID(const uint32_t version, const X_NOTIFY_AREA area, const uint32_t index)
			{
				m_reserved = 0;
				m_area = (uint32_t)area;
				m_index = index;
				m_version = version;
			}

			inline const uint32_t GetCode() const
			{
				return m_code;
			}

			inline const uint32_t GetVersion() const
			{
				return m_version;
			}

			inline const X_NOTIFY_AREA GetArea() const
			{
				return (X_NOTIFY_AREA)m_area;
			}

			inline const uint32_t GetIndex() const
			{
				return m_index;
			}

		private:
			union
			{
				struct
				{
					uint32_t m_index : 16;
					uint32_t m_version : 9;
					uint32_t m_area : 6;
					uint32_t m_reserved : 1;
				};

				uint32_t m_code;
			};
		};

		extern const NotificationID XN_SYS_UI;
		extern const NotificationID XN_SYS_SIGNINCHANGED;
		extern const NotificationID XN_SYS_STORAGEDEVICESCHANGED;
		extern const NotificationID XN_SYS_PROFILESETTINGCHANGED;
		extern const NotificationID XN_SYS_MUTELISTCHANGED;
		extern const NotificationID XN_SYS_INPUTDEVICESCHANGED;
		extern const NotificationID XN_SYS_INPUTDEVICECONFIGCHANGED;
		extern const NotificationID XN_SYS_PLAYTIMERNOTICE;
		extern const NotificationID XN_SYS_AVATARCHANGED;
		extern const NotificationID XN_SYS_NUIHARDWARESTATUSCHANGED;
		extern const NotificationID XN_SYS_NUIPAUSE;
		extern const NotificationID XN_SYS_NUIUIAPPROACH;
		extern const NotificationID XN_SYS_DEVICEREMAP;
		extern const NotificationID XN_SYS_NUIBINDINGCHANGED;
		extern const NotificationID XN_SYS_AUDIOLATENCYCHANGED;
		extern const NotificationID XN_SYS_NUICHATBINDINGCHANGED;
		extern const NotificationID XN_SYS_INPUTACTIVITYCHANGED;
		extern const NotificationID XN_LIVE_CONNECTIONCHANGED;
		extern const NotificationID XN_LIVE_INVITE_ACCEPTED;
		extern const NotificationID XN_LIVE_LINK_STATE_CHANGED;
		extern const NotificationID XN_LIVE_CONTENT_INSTALLED;
		extern const NotificationID XN_LIVE_MEMBERSHIP_PURCHASED;
		extern const NotificationID XN_LIVE_VOICECHAT_AWAY;
		extern const NotificationID XN_LIVE_PRESENCE_CHANGED;
		extern const NotificationID XN_FRIENDS_PRESENCE_CHANGED;
		extern const NotificationID XN_FRIENDS_FRIEND_ADDED;
		extern const NotificationID XN_FRIENDS_FRIEND_REMOVED;
		extern const NotificationID XN_CUSTOM_ACTIONPRESSED;
		extern const NotificationID XN_CUSTOM_GAMERCARD;
		extern const NotificationID XN_XMP_STATECHANGED;
		extern const NotificationID XN_XMP_PLAYBACKBEHAVIORCHANGED;
		extern const NotificationID XN_XMP_PLAYBACKCONTROLLERCHANGED;
		extern const NotificationID XN_PARTY_MEMBERS_CHANGED;		

	} // lib
} // xenon