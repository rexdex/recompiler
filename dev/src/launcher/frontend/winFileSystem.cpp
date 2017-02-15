#include "build.h"
#include "winFileSystem.h"

namespace win
{

	File::File(HANDLE hFile)
		: m_hFile(hFile)
	{}

	File::~File()
	{
		CloseHandle(m_hFile);
		m_hFile = NULL;
	}

	bool File::GetOffset(uint64& outOffset) const
	{
		LARGE_INTEGER offsetToMove;
		offsetToMove.QuadPart = 0;
		LARGE_INTEGER newPointer;
		newPointer.QuadPart = 0;
		if (!SetFilePointerEx(m_hFile, offsetToMove, &newPointer, FILE_CURRENT))
			return false;

		outOffset = newPointer.QuadPart;
		return true;
	}

	bool File::SetOffset(const uint64 offset)
	{
		LARGE_INTEGER offsetToMove;
		offsetToMove.QuadPart = offset;
		if (!SetFilePointerEx(m_hFile, offsetToMove, NULL, FILE_BEGIN))
			return false;

		return true;
	}

	bool File::Read(void* buffer, const uint32 size, const uint64 offset, uint32& outSize)
	{
		DWORD numRead = 0;
		if (!ReadFile(m_hFile, buffer, size, &numRead, NULL))
			return false;

		outSize = numRead;
		return true;
	}

	bool File::Write(void* buffer, const uint32 size, const uint64 offset, uint32& outSize)
	{
		DWORD numWritten = 0;
		if (!WriteFile(m_hFile, buffer, size, &numWritten, NULL))
			return false;

		outSize = numWritten;
		return true;
	}

	bool File::GetInfo(native::FileInfo& outInfo) const
	{
		if (!GetFileSizeEx(m_hFile, (LARGE_INTEGER*)&outInfo.m_fileLength))
			return false;
		if (!GetFileTime(m_hFile, (FILETIME*)&outInfo.m_creationTime, (FILETIME*)&outInfo.m_lastAccessTime, (FILETIME*)&outInfo.m_lastWriteTime))
			return false;
		return true;
	}

	//---

	FileSystem::FileSystem()
	{
	}

	FileSystem::~FileSystem()
	{
	}

	native::IFile* FileSystem::Open(const std::wstring& mountPath, const uint32 fileMode)
	{
		// create real file
		DWORD shareMode = 0;
		DWORD createMode = 0;
		DWORD accessMode = 0;
		DWORD attributes = 0;
		if (fileMode & native::FileMode_ReadOnly)
		{
			attributes = FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_NORMAL;
			createMode = OPEN_EXISTING;
			accessMode = GENERIC_READ;
			shareMode = FILE_SHARE_READ;
		}
		else if (fileMode & native::FileMode_ReadWrite)
		{
			if (fileMode & native::FileMode_Append)
			{
				createMode = OPEN_ALWAYS;
			}
			else
			{
				createMode = CREATE_ALWAYS;
			}

			attributes = FILE_ATTRIBUTE_NORMAL;
			accessMode = GENERIC_WRITE | GENERIC_READ;
			shareMode = 0;
		}

		// create the file handle

		HANDLE hFile = ::CreateFile(mountPath.c_str(), accessMode, shareMode, NULL, createMode, FILE_ATTRIBUTE_ARCHIVE, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
			return nullptr;

		// create file wrapper
		return new File(hFile);
	}

	bool FileSystem::GetFileInfo(const std::wstring& mountPath, native::FileInfo& outInfo) const
	{
		// create the file handle
		HANDLE hFile = ::CreateFile(mountPath.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
			return false;

		// get stuff
		ZeroMemory(&outInfo, sizeof(outInfo));
		GetFileSizeEx(hFile, (LARGE_INTEGER*)&outInfo.m_fileLength);
		GetFileTime(hFile, (FILETIME*)&outInfo.m_creationTime, (FILETIME*)&outInfo.m_lastAccessTime, (FILETIME*)&outInfo.m_lastWriteTime);
		outInfo.m_attributes = GetFileAttributesW(mountPath.c_str());

		// done
		CloseHandle(hFile);
		return true;
	}

} // win