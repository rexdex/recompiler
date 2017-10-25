#include "build.h"
#include "xenonLib.h"  
#include "xenonFileSystem.h"
#include "xenonKernel.h"
#include "xenonPlatform.h"
#include "xenonBindings.h"

namespace xenon
{
	namespace lib
	{

		//---------------------------------------------------------------------------

		// http://processhacker.sourceforge.net/doc/ntioapi_8h.html
		static const uint32_t FILE_DIRECTORY_FILE = 0x00000001;
		static const uint32_t FILE_SEQUENTIAL_ONLY = 0x00000004;
		static const uint32_t FILE_SYNCHRONOUS_IO_ALERT = 0x00000010;
		static const uint32_t FILE_SYNCHRONOUS_IO_NONALERT = 0x00000020;
		static const uint32_t FILE_NON_DIRECTORY_FILE = 0x00000040;
		static const uint32_t FILE_RANDOM_ACCESS = 0x00000800;

		static uint32 CreateFileImpl(Pointer<uint32> handle_ptr, uint32 desired_access, Pointer<X_OBJECT_ATTRIBUTES> object_attributes_ptr, Pointer<X_FILE_IO_STATUS> io_status_block_ptr, Pointer<uint32> allocation_size_ptr, uint32 file_attributes, uint32 share_access, uint32 creation_disposition, uint32 creation_options)
		{
			const auto& attrs = *object_attributes_ptr.GetNativePointer();
			auto object_name = attrs.ObjectName.AsData().Get().GetNativePointer()->Duplicate();

			uint64 allocation_size = 0;
			if (allocation_size_ptr.IsValid())
				allocation_size = *allocation_size_ptr;

			xenon::FileSystemEntry* entry = NULL;
			xenon::FileSystemEntry* rootFile = NULL;
			if (attrs.RootDirectory != 0xFFFFFFFD && attrs.RootDirectory != 0)
			{
				rootFile = static_cast<xenon::FileSystemEntry*>(GPlatform.GetKernel().ResolveHandle(attrs.RootDirectory, xenon::KernelObjectType::FileSysEntry));
				DEBUG_CHECK(rootFile != nullptr);

				// resolve in context of parent entry
				std::string targetPath = rootFile->GetVirtualPath();
				targetPath += object_name;
				entry = GPlatform.GetFileSystem().Resolve(targetPath.c_str());
			}
			else
			{
				// Resolve the file using the virtual file system.
				entry = GPlatform.GetFileSystem().Resolve(object_name.c_str());
			}

			// determine file mode
			const auto mode = (desired_access & GENERIC_WRITE) ? xenon::FileMode_ReadWrite : xenon::FileMode_ReadOnly;
			X_STATUS result = X_STATUS_NO_SUCH_FILE;
			uint32 info = X_FILE_DOES_NOT_EXIST;

			// open the file
			auto* file = entry->Open(mode);
			if (file)
			{
				result = X_STATUS_SUCCESS;
				info = X_FILE_OPENED;

				if (handle_ptr.IsValid())
					*handle_ptr = file->GetHandle();
			}
			else
			{
				GLog.Err("IO: Failed to open file '%s', mode %d", object_name, mode);
				result = X_STATUS_NO_SUCH_FILE;
				info = X_FILE_DOES_NOT_EXIST;

				if (handle_ptr.IsValid())
					*handle_ptr = 0;
			}

			// io data
			if (io_status_block_ptr.IsValid())
			{
				io_status_block_ptr->Result = result;
				io_status_block_ptr->Info = info;
			}

			// cleanup
			return result;
		}

		uint64 __fastcall Xbox_NtCreateFile(uint64 ip, cpu::CpuRegs& regs)
		{
			MemoryAddress handle_ptr = (const uint32)regs.R3;
			uint32 desired_access = (const uint32)regs.R4;
			MemoryAddress object_attributes_ptr = (const uint32)regs.R5;
			MemoryAddress io_status_block_ptr = (const uint32)regs.R6;
			MemoryAddress allocation_size_ptr = (const uint32)regs.R7;
			uint32 file_attributes = (const uint32)regs.R8;
			uint32 share_access = (const uint32)regs.R9;
			uint32 creation_disposition = (const uint32)regs.R10;
			uint32 creation_options = 0;

			const auto result = CreateFileImpl(handle_ptr, desired_access, object_attributes_ptr, io_status_block_ptr, allocation_size_ptr, file_attributes, share_access, creation_disposition, creation_options);
			RETURN_ARG(result);
		}

		uint64 __fastcall Xbox_NtOpenFile(uint64 ip, cpu::CpuRegs& regs)
		{
			MemoryAddress handle_ptr = (const uint32)regs.R3;
			uint32 desired_access = (const uint32)regs.R4;
			MemoryAddress object_attributes_ptr = (const uint32)regs.R5;
			MemoryAddress io_status_block_ptr = (const uint32)regs.R6;
			uint32 options = (const uint32)regs.R7;

			MemoryAddress allocation_size_ptr = nullptr;
			uint32 file_attributes = 0;
			uint32 share_access = 0;
			uint32 creation_disposition = (uint32)XFileMode::Open;
			uint32 creation_options = options;

			const auto result = CreateFileImpl(handle_ptr, desired_access, object_attributes_ptr, io_status_block_ptr, allocation_size_ptr, file_attributes, share_access, creation_disposition, creation_options);
			RETURN_ARG(result);
		}

		uint64 __fastcall Xbox_NtQueryInformationFile(uint64 ip, cpu::CpuRegs& regs)
		{
			const auto file_handle = (const uint32)regs.R3;
			auto io_status_block_ptr = Pointer<X_FILE_IO_STATUS>(regs.R4);
			const auto file_info_address = (const uint32)(regs.R5);
			const auto length = (const uint32)regs.R6;
			const auto fileInfoType = (const uint32)regs.R7;

			uint32 info = 0;
			uint32 result = X_STATUS_UNSUCCESSFUL;

			if (file_handle != 0)
			{
				auto* file = static_cast<xenon::IFile*>(GPlatform.GetKernel().ResolveHandle(file_handle, xenon::KernelObjectType::FileHandle));
				if (file)
				{
					if (fileInfoType == XFileInternalInformation)
					{
						DEBUG_CHECK(length == 8);
						info = 8;

						if (file_info_address)
							*Pointer<uint64>(file_info_address) = 0;

						result = X_STATUS_SUCCESS;
					}
					else if (fileInfoType == XFilePositionInformation)
					{
						DEBUG_CHECK(length == 8);
						info = 8;

						uint64 fileOffset = 0;
						if (file->GetOffset(fileOffset))
						{
							if (file_info_address)
								*Pointer<uint64>(file_info_address) = fileOffset;

							result = X_STATUS_SUCCESS;
						}
					}
					else if (fileInfoType == XFileNetworkOpenInformation)
					{
						DEBUG_CHECK(length == 56);

						X_FILE_INFO fileInfo;
						memset(&fileInfo, 0, sizeof(fileInfo));
						if (file->GetInfo(fileInfo))
						{
							if (file_info_address)
								*Pointer<X_FILE_INFO>(file_info_address) = fileInfo;

							info = 56;
							result = X_STATUS_SUCCESS;
						}
					}
					else if (fileInfoType == XFileXctdCompressionInformation)
					{
						if (length == 4)
						{
							uint32 magic;
							uint32 read;
							file->Read(&magic, sizeof(magic), 0, read);

							if (read == sizeof(magic))
							{
								info = 4;

								const bool isCompressed = magic == _byteswap_ulong(0x0FF512ED);

								if (file_info_address)
									*Pointer<uint32>(file_info_address) = isCompressed ? 1 : 0;

								result = X_STATUS_SUCCESS;
							}
						}
					}
					else
					{
						GLog.Err("Unknown file type information %d", fileInfoType);
						result = X_STATUS_INFO_LENGTH_MISMATCH;
					}
				}
			}

			if (io_status_block_ptr.IsValid())
			{
				io_status_block_ptr->Result = result;
				io_status_block_ptr->Info = info;
			}

			RETURN_ARG(result);
		}

		uint64 __fastcall Xbox_NtWriteFile(uint64 ip, cpu::CpuRegs& regs)
		{
			const uint32 file_handle = (const uint32)regs.R3;
			const uint32 event_handle = (const uint32)regs.R4;
			const uint32 apc_routine_ptr = (const uint32)regs.R5;
			const uint32 apc_context = (const uint32)regs.R6;
			const uint32 io_status_block_ptr = (const uint32)regs.R7;
			const uint32 buffer = (const uint32)regs.R8;
			const uint32 buffer_length = (const uint32)regs.R9;
			const uint32 byte_offset_ptr = (const uint32)regs.R10;

			uint64 byte_offset = byte_offset_ptr ? cpu::mem::loadAddr<uint64>(byte_offset_ptr) : 0;
			GLog.Log("NtWriteFile(%08X, %08X, %08X, %08X, %08X, %08X, %d, %08X(%d))",
				file_handle, event_handle, apc_routine_ptr, apc_context,
				io_status_block_ptr, buffer, buffer_length, byte_offset_ptr,
				byte_offset);

			// Async not supported yet.
			DEBUG_CHECK(apc_routine_ptr == 0);

			X_STATUS result = X_STATUS_UNSUCCESSFUL;
			uint32 info = 0;

			// Grab event to signal.
			xenon::KernelEvent* eventObject = NULL;
			bool signal_event = false;
			if (event_handle)
			{
				eventObject = static_cast<xenon::KernelEvent*>(GPlatform.GetKernel().ResolveHandle(event_handle, xenon::KernelObjectType::Event));
				signal_event = (eventObject != nullptr);
				DEBUG_CHECK(eventObject != nullptr);
			}

			// Grab file
			auto* file = static_cast<xenon::IFile*>(GPlatform.GetKernel().ResolveHandle(file_handle, xenon::KernelObjectType::FileHandle));
			if (file)
			{
				// Reset event before we begin.
				if (eventObject)
				{
					eventObject->Reset();
				}

				const bool isSync = true;
				if (isSync)
				{
					// Synchronous request.
					if (!byte_offset_ptr || byte_offset == 0xFFFFFFFFfffffffe)
					{
						// FILE_USE_FILE_POINTER_POSITION
						byte_offset = -1;
					}

					// Read data
					uint32 bytes_written = 0;
					if (file->Write((void*)buffer, buffer_length, byte_offset, bytes_written))
					{
						info = (uint32)bytes_written;
						result = X_STATUS_SUCCESS;
						signal_event = true;
					}
				}
				else
				{
					result = X_STATUS_PENDING;
					info = 0;
				}
			}

			// io data
			if (io_status_block_ptr)
			{
				cpu::mem::storeAddr<uint32>(io_status_block_ptr + 0, result);
				cpu::mem::storeAddr<uint32>(io_status_block_ptr + 4, info);
				xenon::TagMemoryWrite(io_status_block_ptr, 8, "NtWriteFile");
			}

			// signal event
			if (eventObject)
			{
				if (signal_event)
					eventObject->Set(0, false);

				//eventObject->Release();
			}

			RETURN_ARG(result);
		}

		uint64 __fastcall Xbox_NtReadFile(uint64 ip, cpu::CpuRegs& regs)
		{
			const auto file_handle = (const uint32)regs.R3;
			const auto event_handle = (const uint32)regs.R4;
			const auto apc_routine_ptr = (const uint32)regs.R5;
			const auto apc_context = (const uint32)regs.R6;
			auto io_status_block_ptr = Pointer<X_FILE_IO_STATUS>(regs.R7);
			const uint32 buffer = (const uint32)regs.R8;
			const uint32 buffer_length = (const uint32)regs.R9;
			auto byte_offset_ptr = Pointer<uint32>(regs.R10);

			uint64 byte_offset = byte_offset_ptr.IsValid() ? (uint32)*byte_offset_ptr : 0;

			DEBUG_CHECK(apc_routine_ptr == 0);

			X_STATUS result = X_STATUS_UNSUCCESSFUL;
			uint32 info = 0;

			// Grab event to signal.
			xenon::KernelEvent* eventObject = NULL;
			bool signal_event = false;
			if (event_handle)
			{
				eventObject = static_cast<xenon::KernelEvent*>(GPlatform.GetKernel().ResolveHandle(event_handle, xenon::KernelObjectType::Event));
				signal_event = (eventObject != nullptr);
				DEBUG_CHECK(eventObject != nullptr);
			}

			// Grab file
			auto* file = static_cast<xenon::IFile*>(GPlatform.GetKernel().ResolveHandle(file_handle, xenon::KernelObjectType::FileHandle));
			if (file)
			{
				// Reset event before we begin.
				if (eventObject)
				{
					eventObject->Reset();
				}

				const bool isSync = true;
				if (isSync)
				{
					// Synchronous request.
					if (!byte_offset_ptr.IsValid() || byte_offset == 0xFFFFFFFFfffffffe)
					{
						// FILE_USE_FILE_POINTER_POSITION
						byte_offset = -1;
					}

					// Read data
					uint32 bytes_read = 0;
					if (file->Read((void*)buffer, buffer_length, byte_offset, bytes_read))
					{
						xenon::TagMemoryWrite(buffer, buffer_length, "NtReadFile('%s' at offset %u)", file->GetEntry()->GetVirtualPath(), byte_offset);
						info = (uint32)bytes_read;
						result = X_STATUS_SUCCESS;
						signal_event = true;
					}
				}
				else
				{
					result = X_STATUS_PENDING;
					info = 0;
				}
			}

			// io data
			if (io_status_block_ptr.IsValid())
			{
				io_status_block_ptr->Result = result;
				io_status_block_ptr->Info = info;
			}

			// signal event
			if (eventObject)
			{
				if (signal_event)
					eventObject->Set(0, false);

				//eventObject->Release();
			}

			RETURN_ARG(result);
		}

		uint64 __fastcall Xbox_NtQueryFullAttributesFile(uint64 ip, cpu::CpuRegs& regs)
		{
			auto object_attributes_ptr = Pointer<X_OBJECT_ATTRIBUTES>(regs.R3);
			auto file_info_ptr = Pointer<X_FILE_INFO>(regs.R4);

			const auto& attrs = *object_attributes_ptr.GetNativePointer();
			const auto object_name = attrs.ObjectName.AsData().Get().GetNativePointer()->Duplicate();

			// get root entry
			xenon::FileSystemEntry* entry = NULL;
			xenon::FileSystemEntry* rootFile = NULL;
			if (attrs.RootDirectory != 0xFFFFFFFD)
			{
				rootFile = static_cast<xenon::FileSystemEntry*>(GPlatform.GetKernel().ResolveHandle(attrs.RootDirectory, xenon::KernelObjectType::FileSysEntry));
				DEBUG_CHECK(rootFile != nullptr);

				// resolve in context of parent entry
				std::string targetPath = rootFile->GetVirtualPath();
				targetPath += object_name;
				entry = GPlatform.GetFileSystem().Resolve(targetPath.c_str());
			}
			else
			{
				// Resolve the file using the virtual file system.
				entry = GPlatform.GetFileSystem().Resolve(object_name.c_str());
			}

			// resolved ?
			XResult result = X_ERROR_NOT_FOUND;
			if (entry)
			{
				if (entry->GetInfo(*file_info_ptr.GetNativePointer()))
					result = X_ERROR_SUCCESS;
			}

			RETURN_ARG(result);
		}

		uint64 __fastcall Xbox_NtSetInformationFile(uint64 ip, cpu::CpuRegs& regs)
		{
			const auto file_handle = (const uint32)regs.R3;
			auto io_status_block_ptr = Pointer<X_FILE_IO_STATUS>(regs.R4);
			auto file_info_address = (const uint32)regs.R5;
			const auto length = (const uint32)regs.R6;
			const auto fileInfoType = (const uint32)regs.R7;

			X_STATUS result = X_STATUS_UNSUCCESSFUL;
			uint32 info = 0;

			auto* file = static_cast<xenon::IFile*>(GPlatform.GetKernel().ResolveHandle(file_handle, xenon::KernelObjectType::FileHandle));
			if (file)
			{
				if (fileInfoType == XFilePositionInformation)
				{
					DEBUG_CHECK(length == 8);
					info = 8;

					const uint64 offset = *Pointer<uint64>(file_info_address);
					if (file->SetOffset(offset))
					{
						result = X_STATUS_SUCCESS;
					}
				}
				else
				{
					GLog.Err("Unknown file type information %d", fileInfoType);
					result = X_STATUS_INFO_LENGTH_MISMATCH;
				}
			}

			if (io_status_block_ptr.IsValid())
			{
				io_status_block_ptr->Result = result;
				io_status_block_ptr->Info = info;
			}

			RETURN_ARG(result);
		}

		uint64 __fastcall Xbox_NtQueryVolumeInformationFile(uint64 ip, cpu::CpuRegs& regs)
		{
			RETURN_DEFAULT();
		}

		uint64 __fastcall Xbox_NtQueryDirectoryFile(uint64 ip, cpu::CpuRegs& regs)
		{
			RETURN_DEFAULT();
		}

		uint64 __fastcall Xbox_NtReadFileScatter(uint64 ip, cpu::CpuRegs& regs)
		{
			RETURN_DEFAULT();
		}

		uint64 __fastcall Xbox_NtFlushBuffersFile(uint64 ip, cpu::CpuRegs& regs)
		{
			RETURN_DEFAULT();
		}

		//---------------------------------------------------------------------------

		void RegisterXboxFiles(runtime::Symbols& symbols)
		{
			REGISTER(NtCreateFile);
			REGISTER(NtOpenFile);
			REGISTER(NtReadFile);
			REGISTER(NtWriteFile);
			REGISTER(NtSetInformationFile);
			REGISTER(NtQueryInformationFile);
			REGISTER(NtQueryVolumeInformationFile);
			REGISTER(NtQueryDirectoryFile);
			REGISTER(NtReadFileScatter);
			REGISTER(NtQueryFullAttributesFile);
			REGISTER(NtFlushBuffersFile);
		}

	} // lib
} // xenon