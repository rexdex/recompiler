#include "build.h"
#include "xenonLibNatives.h"
#include "xenonFileSystem.h"
#include "xenonFileSystemDevices.h"
#include "../host_core/launcherUtils.h"
#include "../host_core/launcherCommandline.h"

namespace xenon
{

	//----

	FileSystemEntry::FileSystemEntry(Kernel* kernel, const char* virtualPath, const wchar_t* physicalPath, class IFileSystemDevice* device)
		: IKernelObjectRefCounted(kernel, KernelObjectType::FileSysEntry, nullptr)
		, m_physicalPath(physicalPath)
		, m_virtualPath(virtualPath)
		, m_device(device)
	{
	}

	FileSystemEntry::~FileSystemEntry()
	{
	}

	IFile* FileSystemEntry::Open(const uint32 flags)
	{
		return m_device->Open(this, flags);
	}

	bool FileSystemEntry::GetInfo(lib::X_FILE_INFO& outInfo) const
	{
		return m_device->GetFileInfo(this, outInfo);
	}

	//----

	IFile::IFile(Kernel* kernel, const class FileSystemEntry* entry, class IFileSystemDevice* device)
		: IKernelObjectRefCounted(kernel, KernelObjectType::FileHandle, nullptr)
		, m_entry(entry)
		, m_device(device)
	{
	}

	IFile::~IFile()
	{
	}

	//----

	IFileSystemDevice::IFileSystemDevice()
	{
	}

	//----

	FileSystem::FileSystem(Kernel* kernel, native::IFileSystem* nativeFileSystem, const launcher::Commandline& commandline)
		: m_kernel(kernel)
	{
		// setup standard mappings
		Link("game:", "\\Device\\Cdrom0");
		Link("d:", "\\Device\\Cdrom0");
		Link("e:", "\\Device\\Harddisk1\\Partition1");
		Link("devkit:", "\\Device\\Harddisk1\\Partition1");

		// base path
		if (commandline.HasOption("fsroot"))
		{
			const auto nativePath = commandline.GetOptionValueW("fsroot");
			GLog.Log("IO: Root for file system set to '%ls'", nativePath.c_str());

			Mount(new FileSystemDevice_PathRedirection(kernel, nativeFileSystem, "\\Device\\Cdrom0", "DVD", nativePath));
			Mount(new FileSystemDevice_PathRedirection(kernel, nativeFileSystem, "\\Device\\Harddisk1\\Partition1", "DevKit", nativePath));
		}
		else
		{
			GLog.Warn("IO: Root for the file system not specified. Application will fail any IO access.");
		}
	}

	FileSystem::~FileSystem()
	{
		utils::ClearPtr(m_devices);
	}

	void FileSystem::Link(const char* symbolicPath, const char* path)
	{
		// update existing
		for (auto link : m_symLinks)
		{
			if (link.m_match == symbolicPath)
			{
				link.m_replace = path;
				GLog.Log("Symbolic link '%s' updated to '%s'", symbolicPath, path);
				return;
			}
		}

		// set new one
		SymLink link;
		link.m_match = symbolicPath;
		link.m_replace = path;
		m_symLinks.push_back(link);
		GLog.Log("Symbolic link '%s' set to '%s'", symbolicPath, path);
	}

	void FileSystem::Mount(IFileSystemDevice* device)
	{
		if (device != nullptr)
		{
			m_devices.push_back(device);
			GLog.Log("File system '%s' mounted at '%s'", device->GetName(), device->GetPrefix());
		}
	}

	FileSystemEntry* FileSystem::Resolve(const char* virtualPath)
	{
		// convert path to something safe
		char conformedPath[512];
		launcher::ConformPath(conformedPath, ARRAYSIZE(conformedPath), virtualPath);

		// use symbolic links
		for (auto it : m_symLinks)
		{
			const size_t len = it.m_match.length();
			if (0 == strncmp(conformedPath, it.m_match.c_str(), len))
			{
				std::string temp = it.m_replace;
				temp += (conformedPath + len);

				GLog.Log("Converted path '%s' -> '%s'", conformedPath, temp.c_str());
				strcpy_s(conformedPath, ARRAYSIZE(conformedPath), temp.c_str());
			}
		}

		// find in map
		FileSystemEntry* entry = nullptr;
		if (utils::Find(m_entries, std::string(conformedPath), entry))
			return entry;

		// find device to use
		const char* resolvedPath = nullptr;
		IFileSystemDevice* device = nullptr;
		for (auto it : m_devices)
		{
			const size_t len = strlen(it->GetPrefix());
			if (0 == strncmp(conformedPath, it->GetPrefix(), len))
			{
				resolvedPath = conformedPath + len;
				device = it;
				break;
			}
		}

		// no device found
		if (!device)
		{
			GLog.Warn("No file system device found for path '%s'", conformedPath);
			m_entries[conformedPath] = nullptr;
			return nullptr;
		}

		// log
		GLog.Log("Found device '%s' to service path '%s'", device->GetName(), resolvedPath);

		// translate to physical path
		std::wstring physicalResolvedPath;
		if (!device->Resolve(resolvedPath, physicalResolvedPath))
		{
			GLog.Err("Failed to resolve path '%s' using device '%s'", resolvedPath, device->GetName());
			m_entries[conformedPath] = nullptr;
			return nullptr;
		}

		// create file entry
		entry = new FileSystemEntry(m_kernel, conformedPath, physicalResolvedPath.c_str(), device);
		m_entries[conformedPath] = entry;
		return entry;
	}

	//----
	
} // xenon