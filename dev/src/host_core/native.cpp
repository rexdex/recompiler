#include "build.h"
#include "nativeFileSystem.h"
#include "nativeKernel.h"
#include "nativeMemory.h"
#include "native.h"

namespace native
{
	//---

	IFile::~IFile()
	{}

	FileInfo::FileInfo()
		: m_creationTime(0)
		, m_lastAccessTime(0)
		, m_lastWriteTime(0)
		, m_changeTime(0)
		, m_allocationSize(0)
		, m_fileLength(0)
		, m_attributes(0)
	{}

	IFileSystem::~IFileSystem()
	{
	}

	//---

	IKernelObject::~IKernelObject()
	{}

	IRunnable::~IRunnable()
	{}

	ICriticalSection::~ICriticalSection()
	{}

	IEvent::~IEvent()
	{}

	ISemaphore::~ISemaphore()
	{}

	IThread::~IThread()
	{}

	IKernel::~IKernel()
	{}

	//---

	IMemory::~IMemory()
	{}

	//---

	Systems::Systems()
	{
		m_fileSystem = nullptr;
		m_memory = nullptr;
		m_kernel = nullptr;
	}

	//---

} // native

#if 0


IFile* FileSystemDevice_PathRedirection::Open(const class FileSystemEntry* entry, const uint32 fileMode)
{
	
}

bool FileSystemDevice_PathRedirection::GetFileInfo(const class FileSystemEntry* entry, xnative::X_FILE_INFO& outInfo) const
{
	
}

#endif