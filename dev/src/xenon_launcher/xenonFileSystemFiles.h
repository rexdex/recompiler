#pragma once

#include "xenonKernel.h"
#include "xenonFileSystem.h"

namespace native
{
	class IFile;
}

namespace xenon
{

	//----------

	class FileNative : public IFile
	{
	public:
		FileNative(Kernel* kernel, const class FileSystemEntry* entry, class IFileSystemDevice* device, native::IFile* nativeFile);
		virtual ~FileNative();

		// file interface
		virtual bool GetOffset(uint64& outOffset) const override;
		virtual bool SetOffset(const uint64 offset) override;
		virtual bool Read(void* buffer, const uint32 size, const uint64 offset, uint32& outSize)  override;
		virtual bool Write(void* buffer, const uint32 size, const uint64 offset, uint32& outSize)  override;
		virtual bool GetInfo(lib::X_FILE_INFO& outInfo) const override;

	private:
		native::IFile*	m_nativeFile;
	};

	//----------

} // xenon