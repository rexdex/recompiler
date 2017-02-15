#pragma once
#include "..\backend\nativeFileSystem.h"

namespace win
{

	class File : public native::IFile
	{
	public:
		File(HANDLE hFile);
		virtual ~File();

		virtual bool GetOffset(uint64& outOffset) const override final;
		virtual bool SetOffset(const uint64 offset) override final;
		virtual bool Read(void* buffer, const uint32 size, const uint64 offset, uint32& outSize) override final;
		virtual bool Write(void* buffer, const uint32 size, const uint64 offset, uint32& outSize) override final;
		virtual bool GetInfo(native::FileInfo& outInfo) const override final;

	private:
		HANDLE	m_hFile;
	};

	class FileSystem : public native::IFileSystem
	{
	public:
		FileSystem();
		virtual ~FileSystem();

		virtual native::IFile* Open(const std::wstring& mountPath, const uint32 fileMode) override final;
		virtual bool GetFileInfo(const std::wstring& mountPath, native::FileInfo& outInfo) const override final;
	};

} // win