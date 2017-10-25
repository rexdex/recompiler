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

		static X_STATUS CreateFileImpl(Pointer<uint32> outHandlePtr, uint32 acesssFlags, Pointer<X_OBJECT_ATTRIBUTES> attributesPtr, Pointer<X_FILE_IO_STATUS> outStatusBlockPtr, Pointer<uint32> sizePtr, uint32 fileAttributes, uint32 shaderAccessFlags, uint32 creationMode, uint32 creationOptions)
		{
			const auto& attrs = *attributesPtr.GetNativePointer();
			auto object_name = attrs.ObjectName.AsData().Get().GetNativePointer()->Duplicate();

			uint64 allocation_size = 0;
			if (sizePtr.IsValid())
				allocation_size = *sizePtr;

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
			const auto mode = (acesssFlags & GENERIC_WRITE) ? xenon::FileMode_ReadWrite : xenon::FileMode_ReadOnly;
			X_STATUS result = X_STATUS_NO_SUCH_FILE;
			uint32 info = X_FILE_DOES_NOT_EXIST;

			// open the file
			auto* file = entry->Open(mode);
			if (file)
			{
				result = X_STATUS_SUCCESS;
				info = X_FILE_OPENED;

				if (outHandlePtr.IsValid())
					*outHandlePtr = file->GetHandle();
			}
			else
			{
				GLog.Err("IO: Failed to open file '%s', mode %d", object_name, mode);
				result = X_STATUS_NO_SUCH_FILE;
				info = X_FILE_DOES_NOT_EXIST;

				if (outHandlePtr.IsValid())
					*outHandlePtr = 0;
			}

			// io data
			if (outStatusBlockPtr.IsValid())
			{
				outStatusBlockPtr->Result = result;
				outStatusBlockPtr->Info = info;
			}

			return result;
		}

		X_STATUS Xbox_NtCreateFile(Pointer<uint32> outHandlePtr, uint32 desiredAccess,
			Pointer<X_OBJECT_ATTRIBUTES> objectAttributesPtr, Pointer<X_FILE_IO_STATUS> outStatusBlockPtr,
			Pointer<uint32> sizePtr, uint32 fileAttributes, uint32 shaderAccessFlags, uint32 creationMode)
		{
			return CreateFileImpl(outHandlePtr, desiredAccess, objectAttributesPtr, outStatusBlockPtr, sizePtr, fileAttributes, shaderAccessFlags, creationMode, 0);
		}

		X_STATUS Xbox_NtOpenFile(Pointer<uint32> outHandlePtr, uint32 desiredAccess,
			Pointer<X_OBJECT_ATTRIBUTES> objectAttributesPtr, Pointer<X_FILE_IO_STATUS> outStatusBlockPtr, uint32_t creationOptions)
		{
			return CreateFileImpl(outHandlePtr, desiredAccess, objectAttributesPtr, outStatusBlockPtr, nullptr, 0, 0, 0, creationOptions);
		}

		static X_STATUS ReturnFromIOFunction(Pointer<X_FILE_IO_STATUS> outStatusBlockPtr, const uint32_t info, const X_STATUS result)
		{
			if (outStatusBlockPtr.IsValid())
			{
				outStatusBlockPtr->Info = info;
				outStatusBlockPtr->Result = result;
			}
			return result;
		}

		X_STATUS Xbox_NtQueryInformationFile(uint32_t fileHandlePtr, Pointer<X_FILE_IO_STATUS> outStatusBlockPtr, MemoryAddress fileInfoPtr, uint32 fileInfoSize, uint32 fileInfoType)
		{
			if (!fileHandlePtr)
				return ReturnFromIOFunction(outStatusBlockPtr, 0, X_STATUS_UNSUCCESSFUL);

			// resolve file object
			auto* file = static_cast<xenon::IFile*>(GPlatform.GetKernel().ResolveHandle(fileHandlePtr, xenon::KernelObjectType::FileHandle));
			if (!file)
				return ReturnFromIOFunction(outStatusBlockPtr, 0, X_STATUS_UNSUCCESSFUL);

			// get info
			if (fileInfoType == XFileInternalInformation)
			{
				DEBUG_CHECK(fileInfoSize == 8);

				if (fileInfoPtr.IsValid())
					*Pointer<uint64>(fileInfoPtr) = 0;

				return ReturnFromIOFunction(outStatusBlockPtr, fileInfoSize, X_STATUS_SUCCESS);
			} 
			else if (fileInfoType == XFilePositionInformation)
			{
				DEBUG_CHECK(fileInfoSize == 8);

				uint64 fileOffset = 0;
				if (!file->GetOffset(fileOffset))
					return ReturnFromIOFunction(outStatusBlockPtr, 0, X_STATUS_UNSUCCESSFUL);

				if (fileInfoPtr.IsValid())
					*Pointer<uint64>(fileInfoPtr) = fileOffset;

				return ReturnFromIOFunction(outStatusBlockPtr, fileInfoSize, X_STATUS_SUCCESS);
			}
			else if (fileInfoType == XFileNetworkOpenInformation)
			{
				DEBUG_CHECK(fileInfoSize == 56);

				X_FILE_INFO fileInfo;
				memset(&fileInfo, 0, sizeof(fileInfo));
				if (!file->GetInfo(fileInfo))
					return ReturnFromIOFunction(outStatusBlockPtr, 0, X_STATUS_UNSUCCESSFUL);

				if (fileInfoPtr.IsValid())
					*Pointer<X_FILE_INFO>(fileInfoPtr) = fileInfo;

				return ReturnFromIOFunction(outStatusBlockPtr, fileInfoSize, X_STATUS_SUCCESS);
			}
			else if (fileInfoType == XFileXctdCompressionInformation)
			{
				DEBUG_CHECK(fileInfoSize == 4);

				uint32 magic = 0;
				uint32 read = 0;
				file->Read(&magic, sizeof(magic), 0, read);
				if (read == sizeof(magic))
				{
					const bool isCompressed = (magic == _byteswap_ulong(0x0FF512ED));

					if (fileInfoPtr.IsValid())
						*Pointer<uint32_t>(fileInfoPtr) = isCompressed ? 1 : 0;

					return ReturnFromIOFunction(outStatusBlockPtr, fileInfoSize, X_STATUS_SUCCESS);
				}
			}

			// unknown info
			GLog.Err("Unknown file type information %d", fileInfoType);
			return ReturnFromIOFunction(outStatusBlockPtr, 0, X_STATUS_UNSUCCESSFUL);
		}

		X_STATUS Xbox_NtWriteFile(uint32 fileHandle, uint32 eventHandle, MemoryAddress apcRoutineAddress, uint32 apcContext, 
			Pointer<X_FILE_IO_STATUS> outStatusBlockPtr, MemoryAddress bufferAddress, uint32 bufferLength, Pointer<uint64> byteOffsetPtr)
		{
			// TODO: async IO
			DEBUG_CHECK(!apcRoutineAddress.IsValid());

			// IO may require event to signal
			xenon::KernelEvent* eventObject = NULL;
			if (eventHandle != 0)
			{
				eventObject = static_cast<xenon::KernelEvent*>(GPlatform.GetKernel().ResolveHandle(eventHandle, xenon::KernelObjectType::Event));
				if (!eventObject)
					ReturnFromIOFunction(outStatusBlockPtr, 0, X_STATUS_INVALID_HANDLE);
			}

			// Resolve file handle to file object
			auto* file = static_cast<xenon::IFile*>(GPlatform.GetKernel().ResolveHandle(fileHandle, xenon::KernelObjectType::FileHandle));
			if (!file)
				return ReturnFromIOFunction(outStatusBlockPtr, 0, X_STATUS_INVALID_HANDLE);

			// Reset event before we begin.
			if (eventObject)
				eventObject->Reset();

			// Load data
			// TODO: async IO
			X_STATUS result = 0;
			const bool isSync = true;
			if (isSync)
			{
				uint64 writeOffset = -1;
				if (byteOffsetPtr.IsValid())
				{
					const auto byteOffset = (uint64)*byteOffsetPtr;
					if (byteOffset != 0xFFFFFFFFfffffffeULL)
						writeOffset = byteOffset;
				}

				// write data to file at given offset
				uint32 numBytesWritten = 0;
				if (!file->Write(bufferAddress.GetNativePointer(), bufferLength, writeOffset, numBytesWritten))
				{
					result = ReturnFromIOFunction(outStatusBlockPtr, 0, X_STATUS_UNSUCCESSFUL);
				}
				else
				{
					result = ReturnFromIOFunction(outStatusBlockPtr, numBytesWritten, X_STATUS_SUCCESS);
				}
			}
			else
			{
				result = ReturnFromIOFunction(outStatusBlockPtr, 0, X_STATUS_PENDING);
			}

			// signal event
			if (eventObject)
				eventObject->Set(0, false);

			// propagate result
			return result;
		}

		X_STATUS Xbox_NtReadFile(uint32_t fileHandle, uint32_t eventHandle, MemoryAddress apcRoutineAddress, uint32 apcContext,
			Pointer<X_FILE_IO_STATUS> outStatusBlockPtr, MemoryAddress bufferAddress, uint32 bufferLength, Pointer<uint64> byteOffsetPtr)
		{
			// TODO: async IO
			DEBUG_CHECK(!apcRoutineAddress.IsValid());

			// IO may require event to signal
			xenon::KernelEvent* eventObject = NULL;
			if (eventHandle != 0)
			{
				eventObject = static_cast<xenon::KernelEvent*>(GPlatform.GetKernel().ResolveHandle(eventHandle, xenon::KernelObjectType::Event));
				if (!eventObject)
					ReturnFromIOFunction(outStatusBlockPtr, 0, X_STATUS_INVALID_HANDLE);
			}

			// Resolve file handle to file object
			auto* file = static_cast<xenon::IFile*>(GPlatform.GetKernel().ResolveHandle(fileHandle, xenon::KernelObjectType::FileHandle));
			if (!file)
				return ReturnFromIOFunction(outStatusBlockPtr, 0, X_STATUS_INVALID_HANDLE);

			// Reset event before we begin.
			if (eventObject)
				eventObject->Reset();

			// Load data
			// TODO: async IO
			X_STATUS result = 0;
			const bool isSync = true;
			if (isSync)
			{
				uint64 readOffset = -1;
				if (byteOffsetPtr.IsValid())
				{
					const auto byteOffset = (uint64)*byteOffsetPtr;
					if (byteOffset != 0xFFFFFFFFfffffffeULL)
						readOffset = byteOffset;
				}

				// read data to file from given offset
				uint32 numBytesWritten = 0;
				if (!file->Read(bufferAddress.GetNativePointer(), bufferLength, readOffset, numBytesWritten))
				{
					xenon::TagMemoryWrite(bufferAddress.GetNativePointer(), bufferLength, "NtReadFile('%s' at offset %u)", file->GetEntry()->GetVirtualPath(), readOffset);
					result = ReturnFromIOFunction(outStatusBlockPtr, 0, X_STATUS_UNSUCCESSFUL);
				}
				else
				{
					result = ReturnFromIOFunction(outStatusBlockPtr, numBytesWritten, X_STATUS_SUCCESS);
				}
			}
			else
			{
				result = ReturnFromIOFunction(outStatusBlockPtr, 0, X_STATUS_PENDING);
			}

			// signal event
			if (eventObject)
				eventObject->Set(0, false);

			// propagate result
			return result;
		}

		X_STATUS Xbox_NtQueryFullAttributesFile(Pointer<X_OBJECT_ATTRIBUTES> attributesPtr, Pointer<X_FILE_INFO> fileInfoPtr)
		{
			// get name of the file/directory
			const auto fileName = attributesPtr->ObjectName.AsData().Get().GetNativePointer()->Duplicate();

			// get root entry
			xenon::FileSystemEntry* entry = NULL;
			xenon::FileSystemEntry* rootFile = NULL;
			if (attributesPtr->RootDirectory != 0xFFFFFFFD)
			{
				// find directory entry
				rootFile = static_cast<xenon::FileSystemEntry*>(GPlatform.GetKernel().ResolveHandle(attributesPtr->RootDirectory, xenon::KernelObjectType::FileSysEntry));
				DEBUG_CHECK(rootFile != nullptr);

				// resolve in context of parent entry
				std::string targetPath = rootFile->GetVirtualPath();
				targetPath += fileName;
				entry = GPlatform.GetFileSystem().Resolve(targetPath.c_str());
			}
			else
			{
				// Resolve the file using the virtual file system.
				entry = GPlatform.GetFileSystem().Resolve(fileName.c_str());
			}

			// entry not found
			if (!entry)
				return X_STATUS_NO_SUCH_FILE;

			// get file informations
			if (!entry->GetInfo(*fileInfoPtr.GetNativePointer()))
				return X_STATUS_NO_SUCH_FILE;

			// info retrieved
			return X_STATUS_SUCCESS;
		}

		X_STATUS Xbox_NtSetInformationFile(const uint32 fileHandle, Pointer<X_FILE_IO_STATUS> outStatusBlockPtr, MemoryAddress fileInfoAddress, uint32 fileInfoSize, uint32 fileInfoType)
		{
			// resolve file handle
			auto* file = static_cast<xenon::IFile*>(GPlatform.GetKernel().ResolveHandle(fileHandle, xenon::KernelObjectType::FileHandle));
			if (!file)
				return ReturnFromIOFunction(outStatusBlockPtr, 0, X_STATUS_UNSUCCESSFUL);

			// extract info
			if (fileInfoType == XFilePositionInformation)
			{
				DEBUG_CHECK(fileInfoSize == 8);

				const uint64 offset = *Pointer<uint64>(fileInfoAddress);
				if (!file->SetOffset(offset))
					return ReturnFromIOFunction(outStatusBlockPtr, 0, X_STATUS_INFO_LENGTH_MISMATCH);
			}
			else
			{
				GLog.Err("Unknown file type information %d", fileInfoType);
				return ReturnFromIOFunction(outStatusBlockPtr, 0, X_STATUS_NOT_IMPLEMENTED);
			}

			return X_STATUS_SUCCESS;
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
			REGISTER(NtQueryFullAttributesFile);

			NOT_IMPLEMENTED(NtReadFileScatter);
			NOT_IMPLEMENTED(NtFlushBuffersFile)
			NOT_IMPLEMENTED(NtQueryVolumeInformationFile);
			NOT_IMPLEMENTED(NtQueryDirectoryFile);
		}

	} // lib
} // xenon