#include "build.h"
#include "xenonLibNatives.h"
#include "xenonFileSystem.h"
#include "xenonFileSystemFiles.h"

#include "..\..\..\launcher\backend\nativeFileSystem.h"

namespace xenon
{

	//-----

	FileNative::FileNative(Kernel* kernel, const class FileSystemEntry* entry, class IFileSystemDevice* device, native::IFile* nativeFile)
		: IFile(kernel, entry, device)
		, m_nativeFile(nativeFile)
	{
	}

	FileNative::~FileNative()
	{
		delete m_nativeFile;
		m_nativeFile = nullptr;
	}

	bool FileNative::GetOffset(uint64& outOffset) const
	{
		return m_nativeFile->GetOffset(outOffset);
	}

	bool FileNative::SetOffset(const uint64 offset)
	{
		return m_nativeFile->SetOffset(offset);
	}

	bool FileNative::Read(void* buffer, const uint32 size, const uint64 offset, uint32& outSize)
	{
		return m_nativeFile->Read(buffer, size, offset, outSize);
	}

	bool FileNative::Write(void* buffer, const uint32 size, const uint64 offset, uint32& outSize)
	{
		return m_nativeFile->Write(buffer, size, offset, outSize);
	}

	bool FileNative::GetInfo(xnative::X_FILE_INFO& outInfo) const
	{
		native::FileInfo info;
		if (!m_nativeFile->GetInfo(info))
			return false;

		memset(&outInfo, 0, sizeof(outInfo));
		outInfo.FileLength = info.m_fileLength;
		outInfo.CreationTime = info.m_creationTime;
		outInfo.LastAccessTime = info.m_lastAccessTime;
		outInfo.LastWriteTime = info.m_lastWriteTime;
		return true;
	}

} // xenon