#include "build.h"
#include "xenonLibNatives.h"
#include "xenonFileSystem.h"
#include "xenonFileSystemDevices.h"
#include "xenonFileSystemFiles.h"
#include "../host_core/launcherUtils.h"
#include "../host_core/nativeFileSystem.h"

namespace xenon
{

	//-----------

	FileSystemDevice_PathRedirection::FileSystemDevice_PathRedirection(Kernel* kernel, native::IFileSystem* nativeFileSystem, const char* prefix, const char* name, const std::wstring& realBasePath)
		: IFileSystemDevice()
		, m_name(name)
		, m_prefix(prefix)
		, m_basePath(realBasePath)
		, m_nativeFileSystem(nativeFileSystem)
		, m_kernel(kernel)
	{
	}

	FileSystemDevice_PathRedirection::~FileSystemDevice_PathRedirection()
	{
	}

	bool FileSystemDevice_PathRedirection::Resolve(const char* path, std::wstring& outPath)
	{
		outPath = m_basePath;
		outPath += launcher::AnsiToUnicode(path);
		GLog.Log("IO: Resolved path '%hs' to absolute '%ls'", path, outPath.c_str());
		return true;
	}

	bool FileSystemDevice_PathRedirection::GetFileInfo(const class FileSystemEntry* entry, lib::X_FILE_INFO& outInfo) const
	{
		native::FileInfo info;
		if (!m_nativeFileSystem->GetFileInfo(entry->GetPhysicalPath(), info))
			return false;

		memset(&outInfo, 0, sizeof(outInfo));
		outInfo.FileLength = info.m_fileLength;
		outInfo.CreationTime = info.m_creationTime;
		outInfo.LastAccessTime = info.m_lastAccessTime;
		outInfo.LastWriteTime = info.m_lastWriteTime;
		return true;
	}

	IFile* FileSystemDevice_PathRedirection::Open(const class FileSystemEntry* entry, const uint32 fileMode)
	{
		uint32 nativeFileMode = 0;

		if (fileMode & xenon::FileMode_Append)
			nativeFileMode |= native::FileMode_Append;
		if (fileMode & xenon::FileMode_ReadOnly)
			nativeFileMode |= native::FileMode_ReadOnly;
		if (fileMode & xenon::FileMode_ReadWrite)
			nativeFileMode |= native::FileMode_ReadWrite;

		auto* nativeFile = m_nativeFileSystem->Open(entry->GetPhysicalPath(), nativeFileMode);
		if (!nativeFile)
			return nullptr;

		// create file wrapper
		return new FileNative( m_kernel, entry, this, nativeFile);
	}

	//-----------

} // xenon