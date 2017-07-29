#include "build.h"
#include "xenonLibUtils.h" 
#include "xenonFileSystem.h"
#include "xenonKernel.h"
#include "xenonPlatform.h"

//---------------------------------------------------------------------------

uint64 __fastcall XboxFiles_NtCreateFile( uint64 ip, cpu::CpuRegs& regs )
{
	uint32 handle_ptr = (const uint32) regs.R3;
	uint32 desired_access = (const uint32) regs.R4;
	uint32 object_attributes_ptr = (const uint32) regs.R5;
	uint32 io_status_block_ptr = (const uint32) regs.R6;
	uint32 allocation_size_ptr = (const uint32) regs.R7;
	uint32 file_attributes = (const uint32) regs.R8;
	uint32 share_access = (const uint32) regs.R9;
	uint32 creation_disposition = (const uint32) regs.R10;

	xnative::X_OBJECT_ATTRIBUTES attrs( nullptr, object_attributes_ptr );
	char* object_name = attrs.ObjectName.Duplicate();

	GLog.Log("NtCreateFile(%08X, %08X, %08X(%s), %08X, %08X, %08X, %d, %d)",
         handle_ptr, desired_access, object_attributes_ptr,
         !object_name ? "(null)" : object_name, io_status_block_ptr,
         allocation_size_ptr, file_attributes, share_access,
         creation_disposition);

	uint64 allocation_size = 0;  // is this correct???
	if (allocation_size_ptr != 0)
	{
		allocation_size = mem::loadAddr<uint64>(allocation_size_ptr);
	}

	xenon::FileSystemEntry* entry = NULL;
	xenon::FileSystemEntry* rootFile = NULL;
	if ( attrs.RootDirectory != 0xFFFFFFFD && attrs.RootDirectory != 0 )
	{
	    rootFile = static_cast< xenon::FileSystemEntry* >( GPlatform.GetKernel().ResolveHandle( attrs.RootDirectory, xenon::KernelObjectType::FileSysEntry ) );
		DEBUG_CHECK( rootFile != nullptr );

		// resolve in context of parent entry
		std::string targetPath = rootFile->GetVirtualPath();
		targetPath += object_name;
		entry = GPlatform.GetFileSystem().Resolve( targetPath.c_str() );
	}
	else
	{
		// Resolve the file using the virtual file system.
		entry = GPlatform.GetFileSystem().Resolve( object_name );
	}

	// determine file mode
	const auto mode = (desired_access & GENERIC_WRITE) ? xenon::FileMode_ReadWrite : xenon::FileMode_ReadOnly;
	xnative::X_STATUS result = xnative::X_STATUS_NO_SUCH_FILE;
	uint32 info = xnative::X_FILE_DOES_NOT_EXIST;

	// open the file
	auto* file = entry->Open( mode );
	if (file)
	{
		result = xnative::X_STATUS_SUCCESS;
		info = xnative::X_FILE_OPENED;

		if ( handle_ptr )
		{
			mem::storeAddr<uint32>( handle_ptr, file->GetHandle() );
		}
	}
	else
	{
		GLog.Err( "IO: Failed to open file '%s', mode %d", object_name, mode );
	    result = xnative::X_STATUS_NO_SUCH_FILE;
	    info = xnative::X_FILE_DOES_NOT_EXIST;

		if (handle_ptr)
		{
			mem::storeAddr<uint32>(handle_ptr, 0 );
		}
	}

	// io data
	if (io_status_block_ptr)
	{
		mem::storeAddr<uint32>( io_status_block_ptr+0, result );
		mem::storeAddr<uint32>( io_status_block_ptr+4, info );
	}

	// cleanup
	free( object_name );
	RETURN_ARG( result );
}

uint64 __fastcall XboxFiles_NtOpenFile( uint64 ip, cpu::CpuRegs& regs )
{
	RETURN_DEFAULT();
}

uint64 __fastcall XboxFiles_NtQueryInformationFile( uint64 ip, cpu::CpuRegs& regs )
{
	const uint32 file_handle = (const uint32) regs.R3;
	const uint32 io_status_block_ptr = (const uint32) regs.R4;
	const uint32 file_info_ptr = (const uint32) regs.R5;
	const uint32 length = (const uint32) regs.R6;
	const uint32 fileInfoType = (const uint32) regs.R7;
	GLog.Log("NtQueryInformationFile(%08X, %08X, %08X, %08X, %08X)", file_handle, io_status_block_ptr, file_info_ptr, length, fileInfoType );

	uint32 info = 0;
	uint32 result = xnative::X_STATUS_UNSUCCESSFUL;

	if (file_handle != 0)
	{
		auto* file = static_cast<xenon::IFile*>(GPlatform.GetKernel().ResolveHandle(file_handle, xenon::KernelObjectType::FileHandle));
		if (file)
		{
			if (fileInfoType == xnative::XFileInternalInformation)
			{
				// Internal unique file pointer. Not sure why anyone would want this.
				DEBUG_CHECK(length == 8);
				info = 8;
				mem::storeAddr<uint64>(file_info_ptr, 0);
				result = xnative::X_STATUS_SUCCESS;
			}
			else if (fileInfoType == xnative::XFilePositionInformation)
			{
				DEBUG_CHECK(length == 8);
				info = 8;

				uint64 fileOffset = 0;
				if (file->GetOffset(fileOffset))
				{
					mem::storeAddr<uint64>(file_info_ptr, fileOffset);
					result = xnative::X_STATUS_SUCCESS;
				}
			}
			else if (fileInfoType == xnative::XFileNetworkOpenInformation)
			{
				DEBUG_CHECK(length == 56);

				xnative::X_FILE_INFO fileInfo;
				memset(&fileInfo, 0, sizeof(fileInfo));
				if (file->GetInfo(fileInfo))
				{
					info = 56;
					fileInfo.Write(nullptr, file_info_ptr);
					result = xnative::X_STATUS_SUCCESS;
				}
			}
			else if (fileInfoType == xnative::XFileXctdCompressionInformation)
			{
				// Read timeout.
				if (length == 4)
				{
					uint32 magic;
					uint32 read;
					file->Read(&magic, sizeof(magic), 0, read);
					if (read == sizeof(magic))
					{
						info = 4;
						mem::storeAddr<uint32>(file_info_ptr, magic == _byteswap_ulong(0x0FF512ED));
						result = xnative::X_STATUS_SUCCESS;
					}
				}
			}
			else
			{
				GLog.Err("Unknown file type information %d", fileInfoType);
				result = xnative::X_STATUS_INFO_LENGTH_MISMATCH;
			}
		}
	}

	if (io_status_block_ptr)
	{
		mem::storeAddr<uint32>( io_status_block_ptr+0, result );
		mem::storeAddr<uint32>( io_status_block_ptr+4, info );
	}

	RETURN_ARG(result);
}

uint64 __fastcall XboxFiles_NtWriteFile( uint64 ip, cpu::CpuRegs& regs )
{
	const uint32 file_handle = (const uint32) regs.R3;
	const uint32 event_handle = (const uint32) regs.R4;
	const uint32 apc_routine_ptr = (const uint32) regs.R5;
	const uint32 apc_context = (const uint32) regs.R6;
	const uint32 io_status_block_ptr = (const uint32) regs.R7;
	const uint32 buffer = (const uint32) regs.R8;
	const uint32 buffer_length = (const uint32) regs.R9;
	const uint32 byte_offset_ptr = (const uint32) regs.R10;

	uint64 byte_offset = byte_offset_ptr ? mem::loadAddr<uint64>(byte_offset_ptr) : 0;
	GLog.Log("NtWriteFile(%08X, %08X, %08X, %08X, %08X, %08X, %d, %08X(%d))",
         file_handle, event_handle, apc_routine_ptr, apc_context,
         io_status_block_ptr, buffer, buffer_length, byte_offset_ptr,
         byte_offset);

	// Async not supported yet.
	DEBUG_CHECK(apc_routine_ptr == 0);

	xnative::X_STATUS result = xnative::X_STATUS_UNSUCCESSFUL;
	uint32 info = 0;

	// Grab event to signal.
	xenon::KernelEvent* eventObject = NULL;
	bool signal_event = false;
	if (event_handle)
	{
		eventObject = static_cast< xenon::KernelEvent* >( GPlatform.GetKernel().ResolveHandle( event_handle, xenon::KernelObjectType::Event ) );
		signal_event = (eventObject != nullptr);
		DEBUG_CHECK( eventObject != nullptr );
	}

	// Grab file
	auto* file = static_cast< xenon::IFile* >( GPlatform.GetKernel().ResolveHandle( file_handle, xenon::KernelObjectType::FileHandle) );
	if ( file )
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
			if ( file->Write( (void*) buffer, buffer_length, byte_offset, bytes_written ) )
			{
		        info = (uint32)bytes_written;
				result = xnative::X_STATUS_SUCCESS;
				signal_event = true;
			}
		}
		else
		{
			result = xnative::X_STATUS_PENDING;
			info  = 0;
		}
	}

	// io data
	if (io_status_block_ptr)
	{
		mem::storeAddr<uint32>( io_status_block_ptr+0, result );
		mem::storeAddr<uint32>( io_status_block_ptr+4, info );
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

uint64 __fastcall XboxFiles_NtReadFile( uint64 ip, cpu::CpuRegs& regs )
{
	const uint32 file_handle = (const uint32) regs.R3;
	const uint32 event_handle = (const uint32) regs.R4;
	const uint32 apc_routine_ptr = (const uint32) regs.R5;
	const uint32 apc_context = (const uint32) regs.R6;
	const uint32 io_status_block_ptr = (const uint32) regs.R7;
	const uint32 buffer = (const uint32) regs.R8;
	const uint32 buffer_length = (const uint32) regs.R9;
	const uint32 byte_offset_ptr = (const uint32) regs.R10;

	uint64 byte_offset = byte_offset_ptr ? mem::loadAddr<uint64>(byte_offset_ptr) : 0;
	GLog.Log("NtReadFile(%08X, %08X, %08X, %08X, %08X, %08X, %d, %08X(%d))",
         file_handle, event_handle, apc_routine_ptr, apc_context,
         io_status_block_ptr, buffer, buffer_length, byte_offset_ptr,
         byte_offset);

	// Async not supported yet.
	DEBUG_CHECK(apc_routine_ptr == 0);

	xnative::X_STATUS result = xnative::X_STATUS_UNSUCCESSFUL;
	uint32 info = 0;

	// Grab event to signal.
	xenon::KernelEvent* eventObject = NULL;
	bool signal_event = false;
	if (event_handle)
	{
		eventObject = static_cast< xenon::KernelEvent* >( GPlatform.GetKernel().ResolveHandle( event_handle, xenon::KernelObjectType::Event) );
		signal_event = (eventObject != nullptr);
		DEBUG_CHECK( eventObject != nullptr );
	}

	// Grab file
	auto* file = static_cast< xenon::IFile* >( GPlatform.GetKernel().ResolveHandle( file_handle, xenon::KernelObjectType::FileHandle) );
	if ( file )
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
			uint32 bytes_read = 0;
			if ( file->Read( (void*) buffer, buffer_length, byte_offset, bytes_read ) )
			{
		        info = (uint32)bytes_read;
				result = xnative::X_STATUS_SUCCESS;
				signal_event = true;
			}
		}
		else
		{
			result = xnative::X_STATUS_PENDING;
			info  = 0;
		}
	}

	// io data
	if (io_status_block_ptr)
	{
		mem::storeAddr<uint32>( io_status_block_ptr+0, result );
		mem::storeAddr<uint32>( io_status_block_ptr+4, info );
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

uint64 __fastcall XboxFiles_NtQueryFullAttributesFile( uint64 ip, cpu::CpuRegs& regs )
{
	const uint32 object_attributes_ptr = (const uint32) regs.R3;
	const uint32 file_info_ptr = (const uint32) regs.R4;

	xnative::X_OBJECT_ATTRIBUTES attrs( nullptr, object_attributes_ptr );
	char* object_name = attrs.ObjectName.Duplicate();
	GLog.Log("NtQueryFullAttributesFile(%08X(%s), %08X)", object_attributes_ptr, !object_name ? "(null)" : object_name, file_info_ptr);

	// get root entry
	xenon::FileSystemEntry* entry = NULL;
	xenon::FileSystemEntry* rootFile = NULL;
	if ( attrs.RootDirectory != 0xFFFFFFFD )
	{
	    rootFile = static_cast< xenon::FileSystemEntry* >( GPlatform.GetKernel().ResolveHandle( attrs.RootDirectory, xenon::KernelObjectType::FileSysEntry) );
		DEBUG_CHECK( rootFile != nullptr );

		// resolve in context of parent entry
		std::string targetPath = rootFile->GetVirtualPath();
		targetPath += object_name;
		entry = GPlatform.GetFileSystem().Resolve( targetPath.c_str() );
	}
	else
	{
		// Resolve the file using the virtual file system.
		entry = GPlatform.GetFileSystem().Resolve( object_name );
	}

	// resolved ?
	xnative::XResult result = xnative::X_ERROR_NOT_FOUND;
	if (entry)
	{
	    // Found.
	    xnative::X_FILE_INFO file_info;
		if ( entry->GetInfo( file_info ) )
		{
			file_info.Write(nullptr, file_info_ptr);
			result = xnative::X_ERROR_SUCCESS;
		}
	}

	free(object_name);
	RETURN_ARG(result);
}

uint64 __fastcall XboxFiles_NtSetInformationFile( uint64 ip, cpu::CpuRegs& regs )
{
	const uint32 file_handle = (const uint32) regs.R3;
	const uint32 io_status_block_ptr = (const uint32) regs.R4;
	const uint32 file_info_ptr = (const uint32) regs.R5;
	const uint32 length = (const uint32) regs.R6;
	const uint32 fileInfoType = (const uint32) regs.R7;
	GLog.Log("NtSetInformationFile(%08X, %08X, %08X, %08X, %08X)", file_handle, io_status_block_ptr, file_info_ptr, length, fileInfoType);

	xnative::X_STATUS result = xnative::X_STATUS_UNSUCCESSFUL;
	uint32 info = 0;

	auto* file = static_cast< xenon::IFile* >( GPlatform.GetKernel().ResolveHandle( file_handle, xenon::KernelObjectType::FileHandle ) );
	if ( file )
	{
		if ( fileInfoType == xnative::XFilePositionInformation )
		{
			DEBUG_CHECK(length == 8);
			info = 8;

			const uint64 offset = mem::loadAddr<uint64>( file_info_ptr );
			if ( file->SetOffset( offset ) )
			{
				result = xnative::X_STATUS_SUCCESS;
			}
		}
		else
		{
			GLog.Err( "Unknown file type information %d", fileInfoType );
			result = xnative::X_STATUS_INFO_LENGTH_MISMATCH;
		}
	}

	if (io_status_block_ptr)
	{
		mem::storeAddr<uint32>( io_status_block_ptr+0, result );
		mem::storeAddr<uint32>( io_status_block_ptr+4, info );
	}

	RETURN_ARG(result);
}

uint64 __fastcall XboxFiles_NtQueryVolumeInformationFile( uint64 ip, cpu::CpuRegs& regs )
{
	RETURN_DEFAULT();
}

uint64 __fastcall XboxFiles_NtQueryDirectoryFile( uint64 ip, cpu::CpuRegs& regs )
{
	RETURN_DEFAULT();
}

uint64 __fastcall XboxFiles_NtReadFileScatter( uint64 ip, cpu::CpuRegs& regs )
{
	RETURN_DEFAULT();
}

uint64 __fastcall XboxFiles_NtFlushBuffersFile( uint64 ip, cpu::CpuRegs& regs )
{
	RETURN_DEFAULT();
}

//---------------------------------------------------------------------------

void RegisterXboxFiles(runtime::Symbols& symbols)
{
	#define REGISTER(x) symbols.RegisterFunction(#x, (runtime::TBlockFunc) &XboxFiles_##x);
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
	#undef REGISTER
}