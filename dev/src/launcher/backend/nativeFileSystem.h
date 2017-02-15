#pragma once

namespace native
{
	enum FileMode : uint8
	{
		FileMode_ReadOnly = 1,
		FileMode_ReadWrite = 2,
		FileMode_Append = 4,
	};

	enum FileAttributes : uint32
	{
		eFileAttribute_ReadOnly = 0x0001,
		eFileAttribute_Hidden = 0x0002,
		eFileAttribute_System = 0x0004,
		eFileAttribute_Directory = 0x0010,
		eFileAttribute_Archive = 0x0020,
		eFileAttribute_Device = 0x0040,
		eFileAttribute_Normal = 0x0080,
		eFileAttribute_Temporary = 0x0100,
		eFileAttribute_Compressed = 0x0800,
		eFileAttribute_Encrypted = 0x4000,
	};

	struct LAUNCHER_API FileInfo
	{
		FileInfo();

		uint64 m_creationTime;
		uint64 m_lastAccessTime;
		uint64 m_lastWriteTime;
		uint64 m_changeTime;
		uint64 m_allocationSize;
		uint64 m_fileLength;
		uint32 m_attributes;
	};

	class LAUNCHER_API IFile
	{
	public:
		virtual ~IFile();

		virtual bool GetOffset(uint64& outOffset) const = 0;
		virtual bool SetOffset(const uint64 offset) = 0;
		virtual bool Read(void* buffer, const uint32 size, const uint64 offset, uint32& outSize) = 0;
		virtual bool Write(void* buffer, const uint32 size, const uint64 offset, uint32& outSize) = 0;

		virtual bool GetInfo(FileInfo& outInfo) const = 0;
	};

	class LAUNCHER_API IFileSystem
	{
	public:
		virtual ~IFileSystem();

		/// open file
		virtual IFile* Open(const std::wstring& mountPath, const uint32 fileMode) = 0;

		// get file information
		virtual bool GetFileInfo(const std::wstring& mountPath, FileInfo& outInfo) const = 0;
	};

} // native