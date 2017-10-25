#pragma once

#include "xenonKernel.h"

namespace native
{
	class IFileSystem;
}

namespace xenon
{

	class FileSystemEntry;
	class IFileSystemDevice;

	///  file system mode
	enum EFileMode
	{
		FileMode_ReadOnly = 1,
		FileMode_ReadWrite = 2,
		FileMode_Append = 4,
	};

	//-----

	///  file interface
	class IFile : public IKernelObjectRefCounted
	{
	public:
		IFile( Kernel* kernel, const FileSystemEntry* entry, IFileSystemDevice* device);
		virtual ~IFile();

		inline const FileSystemEntry* GetEntry() const { return m_entry; }
		inline IFileSystemDevice* GetDevice() const { return m_device; }

		// file interface
		virtual bool GetOffset(uint64& outOffset) const = 0;
		virtual bool SetOffset(const uint64 offset) = 0;
		virtual bool Read(void* buffer, const uint32 size, const uint64 offset, uint32& outSize) = 0;
		virtual bool Write(void* buffer, const uint32 size, const uint64 offset, uint32& outSize) = 0;
		virtual bool GetInfo(lib::X_FILE_INFO& outInfo) const = 0;

	private:
		const FileSystemEntry*	m_entry;
		IFileSystemDevice*		m_device;
	};

	//-----

	///  file handle
	class FileSystemEntry : public IKernelObjectRefCounted
	{
	public:
		FileSystemEntry(Kernel* kernel, const char* virtualPah, const wchar_t* physicalPath, class IFileSystemDevice* device);
		virtual ~FileSystemEntry();

		inline const wchar_t* GetPhysicalPath() const { return m_physicalPath.c_str(); }
		inline const char* GetVirtualPath() const { return m_virtualPath.c_str(); }
		inline IFileSystemDevice* GetDevice() const { return m_device; }

		// open access
		IFile* Open(const uint32 flags);
		bool GetInfo(lib::X_FILE_INFO& outInfo) const;

	private:
		std::wstring				m_physicalPath;
		std::string					m_virtualPath;
		class IFileSystemDevice*	m_device;
	};

	///----

	///  fake file system device
	class IFileSystemDevice
	{
	public:
		IFileSystemDevice();
		virtual ~IFileSystemDevice() {};

		virtual const char* GetName() const = 0;
		virtual const char* GetPrefix() const = 0;

		virtual bool Resolve(const char* path, std::wstring& outPath) = 0;
		virtual IFile* Open(const class FileSystemEntry* enty, const uint32 fileMode) = 0;
		virtual bool GetFileInfo(const class FileSystemEntry* enty, lib::X_FILE_INFO& outInfo) const = 0;
	};

	///----

	///  fake file system
	class FileSystem
	{
	public:
		FileSystem( Kernel* kernel, native::IFileSystem* nativeFileSystem, const launcher::Commandline& commandline);
		virtual ~FileSystem();

		// resolve file entry for given virtual path
		FileSystemEntry* Resolve(const char* virtualPath);

	private:
		struct SymLink
		{
			std::string		m_match;
			std::string		m_replace;
		};

		Kernel*				m_kernel;

		typedef std::vector< SymLink >						TSymLinks;
		TSymLinks			m_symLinks;

		typedef std::vector< IFileSystemDevice* >		TDevices;
		TDevices			m_devices;

		typedef std::map< std::string, FileSystemEntry* >		TEntries;
		TEntries			m_entries;

		// register symbolic link
		void Link(const char* symbolicPath, const char* path);

		// mount device
		void Mount(IFileSystemDevice* device);
	};

} // xenon