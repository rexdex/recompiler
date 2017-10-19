#pragma once

#include "xenonKernel.h"
#include "xenonFileSystem.h"

namespace native
{
	class IFileSystem;
}

namespace xenon
{

	//----------

	class FileSystemDevice_PathRedirection : public IFileSystemDevice
	{
	public:
		FileSystemDevice_PathRedirection( Kernel* kernel, native::IFileSystem* nativeFileSystem, const char* prefix, const char* name, const std::wstring& realBasePath);
		virtual ~FileSystemDevice_PathRedirection();

		virtual const char* GetName() const override { return m_name.c_str(); }
		virtual const char* GetPrefix() const override { return m_prefix.c_str(); }

		virtual bool Resolve(const char* path, std::wstring& outPath) override;
		virtual IFile* Open(const class FileSystemEntry* enty, const uint32 fileMode) override;
		virtual bool GetFileInfo(const class FileSystemEntry* enty, xnative::X_FILE_INFO& outInfo) const override;

	private:
		Kernel*				m_kernel;

		std::wstring		m_basePath;
		std::string			m_prefix;
		std::string			m_name;

		native::IFileSystem* m_nativeFileSystem;
	};

	//----------

} // xenon