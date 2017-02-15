#include "xenonImageLoader.h"
#include "rijndael-alg-fst.h"
#include "mspack.h"
#include "lzx.h"
#include <assert.h>

#include "../../../framework/backend/platformDefinition.h"
#include "../../../framework/backend/platformExports.h"

//---------------------------------------------------------------------------

ImageBinaryXEX::ImageBinaryXEX( const wchar_t* path )
{
	// set path
	m_path = path;

	// reset version
	m_version.value = 0;
	m_minimalVersion.value = 0;

	// reset XEX data
	memset( &m_xexData, 0, sizeof(m_xexData) );
}

bool ImageBinaryXEX::Load( ILogOutput& log, const class platform::Definition* platform )
{
	// importing image
	CScopedTask task( log, "Loading XEX file" );

	// open the file
	FILE* f = nullptr;
	_wfopen_s( &f, m_path.c_str(), L"rb" );
	if ( nullptr == f )
	{
		log.Error( "Unable to open file '%ls'", m_path.c_str() );
		return false;
	}

	// get file size
	fseek( f, 0, SEEK_END );
	const uint32 fileSize = ftell( f );
	fseek( f, 0, SEEK_SET );

	// load data
	uint8* fileData = new uint8[ fileSize ];
	{
		log.SetTaskName( "Loading XEX file '%ls'...", m_path.c_str() );
		const uint32 bufferSize = 4096;
		for (uint32 i=0; i<fileSize; i += bufferSize)
		{
			log.SetTaskProgress( i, fileSize );
			const int bufferReadSize = ((fileSize - i) > bufferSize) ? bufferSize : fileSize - i;
			fread( fileData + i, bufferReadSize, 1, f );
		}
		fclose( f );
	}

	// load headers
	log.SetTaskName( "Parsing headers..." );
	ImageByteReaderXEX reader( fileData, fileSize );
	if ( !LoadHeaders( log, reader ) )
	{
		delete [] fileData;
		return false;
	}

	// load, decompress and decrypt image data
	log.SetTaskName( "Decompressing image..." );
	if ( !LoadImageData( log, reader ) )
	{
		delete [] fileData;
		return false;
	}

	// load the embedded PE image from memory image
	log.SetTaskName( "Loading PE image..." );
	if ( !LoadPEImage( log, fileData + m_xexData.header.exe_offset, fileSize - m_xexData.header.exe_offset) ) 
	{
		free( (void*)m_memoryData );
		delete [] fileData;
		m_memoryData = nullptr;
		m_memorySize = 0;
		return false;
	}

	// the file data buffer can be freed
	delete [] fileData;

	// finally, patch the imports
	log.SetTaskName( "Patching imports..." );
	if ( !PatchImports( log, platform ) )
	{
		free( (void*)m_memoryData );
		m_memoryData = nullptr;
		m_memorySize = 0;
		return false;
	}

	// try to load associated map file
	{
		const wchar_t* ext = wcsrchr( m_path.c_str(), '.' );
		if ( ext )
		{
			std::wstring tempPath( m_path.c_str(), ext );
			tempPath += L".map";

			FILE* f = nullptr;
			_wfopen_s( &f, tempPath.c_str(), L"r" );
			if ( nullptr != f )
			{
				log.Log( "XEX: Found map file at '%ls'. Trying to load symbols.", tempPath.c_str() );
				LoadSymbols( log, f );
				fclose( f );
			}
		}
	}

	// loaded
	return true;
}

void EndianSwap( uint32& ofs )
{
	uint8* ptr = (uint8*)&ofs;
	std::swap( ptr[0], ptr[3] );
	std::swap( ptr[1], ptr[2] );
}

bool ImageBinaryXEX::LoadHeaders( ILogOutput& log, ImageByteReaderXEX& reader )
{
	// load the XEX header	 
	XEXImageData& imageData = m_xexData;
	XEXHeader& header = m_xexData.header;
	if ( !reader.Read( log, &header, sizeof(header) ) ) return false;

	// swap and validate
	header.Swap();
	if ( !header.Validate( log ) ) return false;

	// process local headers
	for ( uint32 n=0; n<header.header_count; n++)
	{
		// load the header
		XEXOptionalHeader optionalHeader;
		if ( !optionalHeader.Read( log, reader ) ) return false;

		// extract the length
		bool add = true;
	    switch ( optionalHeader.key & 0xFF )
		{
			// just the data
			case 0x00:
			case 0x01:
			{
				optionalHeader.value = optionalHeader.offset;
				optionalHeader.offset = 0;
				break;
			}

			// data
		    case 0xFF:
			{
				optionalHeader.length = *(uint32*)(reader.GetData() + optionalHeader.offset);
				optionalHeader.offset += 4;
				Swap32( &optionalHeader.length );

				// to big ?
				if ( optionalHeader.length + optionalHeader.offset >= reader.GetSize() )
				{
					log.Warn( "Optional header %i (0x%X) crosses file boundary. Will not be read.", n, optionalHeader.key );
					add = false;
				}

				break;
			}

			// small data
			default:
			{
				optionalHeader.length = (optionalHeader.key & 0xFF) * 4;

				// to big ?
				if ( optionalHeader.length + optionalHeader.offset >= reader.GetSize() )
				{
					log.Warn( "Optional header %i (0x%X) crosses file boundary. Will not be read.", n, optionalHeader.key );
					add = false;
				}

				break;
			}
		}

		// store local optional header
		if ( add )
		{
			m_xexData.optional_headers.push_back( optionalHeader );
		}
    }

	// process the optional headers
	for ( uint32 i=0; i<m_xexData.optional_headers.size(); ++i )
	{
		const XEXOptionalHeader& opt = m_xexData.optional_headers[i];

		// go to the header offset
		if ( opt.length > 0 && opt.offset != 0 )
		{
			if ( !reader.Seek( log, opt.offset ) )
			{
				continue;
			}
		}

		// process the optional headers
		switch ( opt.key )
		{
			// System flags
			case XEX_HEADER_SYSTEM_FLAGS:
			{
				imageData.system_flags = (XEXSystemFlags) opt.value;
				break;
			}

			// Resource info repository
			case XEX_HEADER_RESOURCE_INFO:
			{
				// get the count
				const uint32 count = (opt.length - 4) / 16;
				m_xexData.resources.resize( count );

				// move to file position
		        for ( uint32 n=0; n<count; ++n )
				{
					// load the resource entry
					XEXResourceInfo& resInfo = m_xexData.resources[i];
					if ( !reader.Read( log, &resInfo, sizeof(resInfo) ) ) return false;
				}

				break;
			}

			// Execution info
			case XEX_HEADER_EXECUTION_INFO:
			{
				if ( !imageData.execution_info.Read( log, reader ) ) return false;
				break;
			}

			// Game ratings
			case XEX_HEADER_GAME_RATINGS:
			{
				break;
			}

			// TLS info
			case XEX_HEADER_TLS_INFO:
			{
				if ( !imageData.tls_info.Read( log, reader ) ) return false;
				break;
			}

			// Base address
			case XEX_HEADER_IMAGE_BASE_ADDRESS:
			{
				imageData.exe_address = opt.value;
				log.Log( "XEX: Found base addrses: 0x%08X", imageData.exe_address );
				break;
			}

			// Entry point
		    case XEX_HEADER_ENTRY_POINT:
			{
				imageData.exe_entry_point = opt.value;
				log.Log( "XEX: Found entry point: 0x%08X", imageData.exe_entry_point );
				break;
			}

			// Default stack size
			case XEX_HEADER_DEFAULT_STACK_SIZE:
			{
				imageData.exe_stack_size = opt.value;
				break;
			}

			// Default heap size
			case XEX_HEADER_DEFAULT_HEAP_SIZE:
			{
				imageData.exe_heap_size = opt.value;
				break;
			}

			// File format information
			case XEX_HEADER_FILE_FORMAT_INFO:
			{
				// load the encryption type
				XEXEncryptionHeader encHeader;
				if ( !encHeader.Read( log, reader ) ) return false;

				// setup header info
				imageData.file_format_info.encryption_type = (XEXEncryptionType) encHeader.encryption_type;
				imageData.file_format_info.compression_type = (XEXCompressionType) encHeader.compression_type;

				// load compression blocks
				switch ( encHeader.compression_type )
				{
					case XEX_COMPRESSION_NONE:
					{
						log.Log( "XEX: image::Binary is using no compression" );
						break;
					}

					case XEX_COMPRESSION_DELTA:
					{
						log.Warn( "XEX: image::Binary is using unsupported delta compression" );
						break;
					}

					case XEX_COMPRESSION_BASIC:
					{
						// get the block count
						const uint32 block_count = (opt.length - 8) / 8;
						imageData.file_format_info.basic_blocks.resize( block_count );

						// load the basic compression blocks
						for ( uint32 i=0; i<block_count; ++i )
						{
							XEXFileBasicCompressionBlock& block = imageData.file_format_info.basic_blocks[i];

							if ( !reader.Read( log, &block, sizeof(block) ) ) return false;
							Swap32( &block.data_size );
							Swap32( &block.zero_size );
						}

						log.Log( "XEX: image::Binary is using basic compression with %d blocks", block_count );
						break;
					}

					case XEX_COMPRESSION_NORMAL:
					{
						XEXFileNormalCompressionInfo& normal_info = imageData.file_format_info.normal;
						if ( !normal_info.Read( log, reader ) ) return false;

						log.Log( "XEX: image::Binary is using normal compression with block size = %d", normal_info.block_size );
						break;
					}
				}

				// encryption type
				if ( encHeader.encryption_type != XEX_ENCRYPTION_NONE )
				{
					log.Warn( "XEX: image::Binary is encrypted" );
				}

				// opt header
				break;
			}

			// Import libraries - very important piece
			case XEX_HEADER_IMPORT_LIBRARIES:
			{
				// Load the header data
				XEXImportLibraryBlockHeader blockHeader;
				if ( !blockHeader.Read( log, reader ) ) return false;

				// get the string data
				const char* string_table = (const char*)reader.GetData() + reader.GetOffset();
				reader.Seek( log, reader.GetOffset() + blockHeader.string_table_size );

				// load the imports
			    for ( uint32 m=0; m<blockHeader.count; m++)
				{
					XEXImportLibraryHeader header;
					if ( !header.Read( log, reader ) ) return false;

					// get the library name
					const char* name = "Unknown";
					const uint16 name_index = header.name_index & 0xFF;
					for ( uint32 i=0, j=0; i<blockHeader.string_table_size; )
					{
						assert(j <= 0xFF);
						if ( j == name_index )
						{
							name = string_table + i;
							break;
						}

						if (string_table[i] == 0)
						{
							i++;
							if ( i % 4 )
							{
								i += 4 - (i % 4);
							}
							j++;
						}
						else
						{
							i++;
						}
					}

					// save the import lib name
					if ( name[0] )
					{
						log.Log( "Found import library: '%s'", name );
						m_libNames.push_back( name );
					}

					// load the records
					for ( uint32 i=0; i<header.record_count; ++i )
					{
						// load the record entry
						uint32 recordEntry = 0;
						if ( !reader.Read( log, &recordEntry, sizeof(recordEntry) ) ) return false;
						Swap32( &recordEntry );

						// add to the global record entry list
						m_xexData.import_records.push_back( recordEntry );
					}
				}

				// done
				break;
			}
		}
	}

	// load the loader info
	{
		// go to the certificate region
		if ( !reader.Seek( log, header.certificate_offset ) ) return false;

		// load the loader info
		XEXLoaderInfo& li = m_xexData.loader_info;
		if ( !li.Read( log, reader ) ) return false;

		// print some stats
		log.Log( "XEX: image::Binary size: 0x%X", li.image_size );
	}

	// load the sections
	{
		// go to the section region
		if ( !reader.Seek( log, header.certificate_offset + 0x180 ) ) return false;

		// get the section count
		uint32 sectionCount = 0;
		if ( !reader.Read( log, &sectionCount, sizeof(sectionCount) ) ) return false;
		Swap32( &sectionCount );

		// load the sections
		m_xexData.sections.resize( sectionCount );
		for ( uint32 i=0; i<sectionCount; ++i )
		{
			XEXSection& section = m_xexData.sections[i];
			if ( !reader.Read( log, &section, sizeof(XEXSection) ) ) return false;
			Swap32( &section.info.value );
		}
	}

	// decrypt the XEX key
	{
		// Key for retail executables
		const static uint8 xe_xex2_retail_key[16] =
		{
			0x20, 0xB1, 0x85, 0xA5, 0x9D, 0x28, 0xFD, 0xC3,
			0x40, 0x58, 0x3F, 0xBB, 0x08, 0x96, 0xBF, 0x91
		};

		// Key for devkit executables
		const static uint8 xe_xex2_devkit_key[16] =
		{
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		};

		// Guess key based on file info.
		const uint8* keyToUse = xe_xex2_devkit_key;
		if ( m_xexData.execution_info.title_id != 0 )
		{
			log.Log( "XEX: Found TitleID 0x%X", m_xexData.execution_info.title_id );
			//if ( m_xexData.system_flags 
			keyToUse = xe_xex2_retail_key;
		}

		// Decrypt the header and session key
		uint32 buffer[ 4 * (MAXNR + 1) ];
		int32 nr = rijndaelKeySetupDec(buffer, keyToUse, 128);
		rijndaelDecrypt(buffer, nr, m_xexData.loader_info.file_key, m_xexData.session_key );

		// stats
		{
			const uint32* keys = (const uint32*) &m_xexData.loader_info.file_key;
			log.Log( "XEX: Decrypted file key: %08X-%08X-%08X-%08X", keys[0], keys[1], keys[2], keys[3] );

			const uint32* skeys = (const uint32*) &m_xexData.session_key;
			log.Log( "XEX: Decrypted session key: %08X-%08X-%08X-%08X", skeys[0], skeys[1], skeys[2], skeys[3] );
		}
	}

	// headers loaded
	return true;
}

bool ImageBinaryXEX::LoadImageDataUncompressed( ILogOutput& log, ImageByteReaderXEX& data )
{
	// The EXE image memory is just the XEX memory - exe offset
	const uint32 memorySize = data.GetSize() - m_xexData.header.exe_offset;

	// sanity check
	const uint32 maxImageSize = 128 << 20;
	if ( memorySize >= maxImageSize )
	{
		log.Error( "Computed image size is to big (%X), the exe offset = 0x%X", 
			memorySize, m_xexData.header.exe_offset );

		return false;
	}

	// Allocate in-place the XEX memory.
	uint8* memory = (uint8*) malloc( memorySize );
	if ( nullptr == memory )
	{
		log.Error( "Failed to allocate image memory (size = 0x%X)", memorySize );
		return false;
	}

	// Decrypt the image
	const uint8* sourceData = data.GetData() + m_xexData.header.exe_offset;
	if ( !DecryptBuffer( m_xexData.session_key, sourceData, memorySize, memory, memorySize ) ) return false;

	// done
	m_memoryData = memory;
	m_memorySize = memorySize;
	return true;
}

namespace MSPack
{
	struct mspack_memory_file
	{
		mspack_system sys;
		void *buffer;
		uint32 buffer_size;
		uint32 offset;
	};

	mspack_memory_file* mspack_memory_open(mspack_system *sys, void *buffer, const uint32 buffer_size )
	{
		mspack_memory_file *memfile = (mspack_memory_file *)calloc(1, sizeof(mspack_memory_file));
		if (!memfile) return NULL;
		memfile->buffer = buffer;
		memfile->buffer_size = (off_t)buffer_size;
		memfile->offset = 0;
		return memfile;
	}

	void mspack_memory_close( mspack_memory_file *file )
	{
		mspack_memory_file *memfile = (mspack_memory_file *)file;
		free(memfile);
	}

	int mspack_memory_read(struct mspack_file *file, void *buffer, int chars)
	{
		mspack_memory_file *memfile = (mspack_memory_file *)file;
		const off_t remaining = memfile->buffer_size - memfile->offset;
		const off_t total = min(static_cast<off_t>(chars), remaining);
		memcpy(buffer, (uint8*)memfile->buffer + memfile->offset, total);
		memfile->offset += total;
		return (int)total;
	}

	int mspack_memory_write(struct mspack_file *file, void *buffer, int chars)
	{
		mspack_memory_file *memfile = (mspack_memory_file *)file;
		const off_t remaining = memfile->buffer_size - memfile->offset;
		const off_t total = min(static_cast<off_t>(chars), remaining);
		memcpy((uint8*)memfile->buffer + memfile->offset, buffer, total);
		memfile->offset += total;
		return (int)total;
	}

	void *mspack_memory_alloc(struct mspack_system *sys, size_t chars)
	{
		return calloc(chars, 1);
	}

	void mspack_memory_free(void *ptr)
	{
		free(ptr);
	}

	void mspack_memory_copy(void *src, void *dest, size_t chars)
	{
		memcpy(dest, src, chars);
	}

	mspack_system *mspack_memory_sys_create()
	{
		mspack_system *sys = (mspack_system *)calloc(1, sizeof(mspack_system));
		if (!sys) return NULL;
		sys->read = mspack_memory_read;
		sys->write = mspack_memory_write;
		sys->alloc = mspack_memory_alloc;
		sys->free = mspack_memory_free;
		sys->copy = mspack_memory_copy;
		return sys;
	}

	void mspack_memory_sys_destroy(mspack_system *sys)
	{
		free(sys);
	}
} // MSPack


bool ImageBinaryXEX::LoadImageDataNormal( ILogOutput& log, ImageByteReaderXEX& data )
{
	bool sucesss = false;

	// Image source
	const uint32 sourceSize = (uint32)(data.GetSize() - m_xexData.header.exe_offset);
	const uint8* sourceBuffer = data.GetData() + m_xexData.header.exe_offset;

	// Get data
	uint32 imageSize = sourceSize;
	const uint8* imageData = sourceBuffer;
	if ( m_xexData.file_format_info.encryption_type == XEX_ENCRYPTION_NORMAL )
	{
		// allocate new buffer and decode it
		imageData = (const uint8*) malloc( sourceSize );
		memset( (void*)imageData, 0, sourceSize );
		DecryptBuffer( m_xexData.session_key, sourceBuffer, sourceSize, (uint8*) imageData, imageSize );
		log.Log( "Decoded the encrypted XEX image using session key" );
	}

	// Allocate deblocked data
	uint8* compressData = (uint8*) malloc( imageSize );

	// Compute buffer size
	uint32 compressedSize = 0;
	uint32 uncompressedSize = 0;
	{
		const uint8* p = imageData;
		uint8* d = compressData;

		uint32 blockSize = m_xexData.file_format_info.normal.block_size;
		while ( blockSize )
		{
			const uint8 *pnext = p + blockSize;
			const uint32 nextSize = _byteswap_ulong( *(uint32*)p );
			p += 4;
			p += 20;  // skip 20b hash

			while (true)
			{
				const uint32 chunkSize = (p[0] << 8) | p[1];
				p += 2;
				if ( !chunkSize)
					break;	

				memcpy(d, p, chunkSize);
				p += chunkSize;
				d += chunkSize;

				uncompressedSize += 0x8000;
			}

			p = pnext;
			blockSize = nextSize;
		}

		compressedSize = (uint32)(d - compressData);
	}

	// Report sizes
	log.Log( "Uncompressed image size: %d", uncompressedSize );
	log.Log( "Compressed image size: %d", compressedSize );
		
	// Allocate in-place the XEX memory
	uint8* uncompressedImage = (uint8*) malloc( uncompressedSize );
	if ( !uncompressedImage)
	{
		log.Error( "Unable to allocate memory for uncompressed image" );
		goto Cleanup;
	}

	// Setup decompressor and decompress.
	mspack_system* sys = MSPack::mspack_memory_sys_create();
	if ( !sys )
	{
		log.Error( "Unable to allocate decompression helper" );
		goto Cleanup;
	}

	// create source file
	MSPack::mspack_memory_file* lzxsrc = MSPack::mspack_memory_open( sys, compressData, compressedSize);
	if ( !lzxsrc )
	{
		log.Error( "Unable to allocate input decompression file" );
		goto Cleanup;
	}

	// create destination file
	MSPack::mspack_memory_file* lzxdst = MSPack::mspack_memory_open( sys, uncompressedImage, uncompressedSize);
	if ( !lzxdst )
	{
		log.Error( "Unable to allocate output decompression file" );
		goto Cleanup;
	}

	// initialize decompression
	lzxd_stream* lzxd = lzxd_init( sys, (mspack_file*) lzxsrc, (mspack_file *)lzxdst, m_xexData.file_format_info.normal.window_bits, 0, 32768, m_xexData.loader_info.image_size );
	if ( !lzxd )
	{
		log.Error( "Unable to initialize decompression helper" );
		goto Cleanup;
	}

	// decompress
	const int ret = lzxd_decompress( lzxd, (uint32) m_xexData.loader_info.image_size );
	if ( ret != 0 )
	{
		log.Error( "Unable to decompression image data: %d", ret );
		goto Cleanup;
	}

	// done
	log.Log( "Image data decompressed" );
	sucesss = true;

	// set image data
	m_memoryData = uncompressedImage;
	m_memorySize = uncompressedSize;
	uncompressedImage = nullptr;

Cleanup:

	if ( lzxd )
	{
		lzxd_free(lzxd);
		lzxd = NULL;
	}

	if (lzxsrc)
	{
		MSPack::mspack_memory_close(lzxsrc);
		lzxsrc = NULL;
	}

	if (lzxdst)
	{
		MSPack::mspack_memory_close(lzxdst);
		lzxdst = NULL;
	}

	if (sys)
	{
		MSPack::mspack_memory_sys_destroy(sys);
		sys = NULL;
	}

	free(compressData);
	free(uncompressedImage);

	if (imageData != sourceBuffer) 
		free((void *)imageData);

	return sucesss;
}

bool ImageBinaryXEX::LoadImageDataBasic( ILogOutput& log, ImageByteReaderXEX& data )
{
	// calculate the uncompressed size
	uint32 memorySize = 0;
	const uint32 blockCount = (uint32)m_xexData.file_format_info.basic_blocks.size();
	for ( uint32 i=0; i<blockCount; ++i )
	{
		const XEXFileBasicCompressionBlock& block = m_xexData.file_format_info.basic_blocks[i];
		memorySize += block.data_size + block.zero_size;
	}

	// source data
	const uint32 sourceSize = (uint32)(data.GetSize() - m_xexData.header.exe_offset);
	const uint8* sourceBuffer = data.GetData() + m_xexData.header.exe_offset;
	
	// sanity check
	const uint32 maxImageSize = 128 << 20;
	if ( memorySize >= maxImageSize )
	{
		log.Error( "Computed image size is to big (%X), the exe offset = 0x%X", 
			memorySize, m_xexData.header.exe_offset );

		return false;
	}

	// Allocate in-place the XEX memory.
	uint8* memory = (uint8*) malloc( memorySize );
	if ( nullptr == memory )
	{
		log.Error( "Failed to allocate image memory (size = 0x%X)", memorySize );
		return false;
	}

	// The decryption state is global for all blocks
	uint32 rk[4 * (MAXNR + 1)];
	uint8 ivec[16] = {0};
	int Nr = rijndaelKeySetupDec(rk, m_xexData.session_key, 128);

	// Destination memory pointers
	uint8* destMemory = memory;
	memset( memory, 0, memorySize );

	// Copy/Decrypt blocks
	for ( uint32 n=0; n<blockCount; n++)
	{
		// get the size of actual data and the zeros
		const XEXFileBasicCompressionBlock& block = m_xexData.file_format_info.basic_blocks[n];
		const uint32 data_size = block.data_size;
		const uint32 zero_size = block.zero_size;

		// decompress/copy data
		const XEXEncryptionType encType = m_xexData.file_format_info.encryption_type;
		switch ( encType )
		{
			// no encryption, copy data
			case XEX_ENCRYPTION_NONE:
			{
				memcpy( destMemory, sourceBuffer, data_size );
				break;
			}

			// AES
			case XEX_ENCRYPTION_NORMAL:
			{
				const uint8 *ct = sourceBuffer;
				uint8* pt = destMemory;
				for ( uint32 n=0; n<data_size; n += 16, ct += 16, pt += 16)
				{
					// Decrypt 16 uint8_ts from input -> output.
					rijndaelDecrypt(rk, Nr, ct, pt);

					// XOR with previous
					for ( uint32 i = 0; i < 16; i++ )
					{
						pt[i] ^= ivec[i];
						ivec[i] = ct[i];
					}
				}

				break;
			}
        }

		// go to next block
		sourceBuffer += data_size;
		destMemory += data_size + zero_size;
	}

	// check if all source data was consumed
	const uint32 consumed = (uint32)(sourceBuffer - data.GetData());
	if ( consumed > data.GetSize() )
	{
		log.Error( "XEX: To much source data was consumed by block decompression (%d > %d)", consumed, data.GetSize() );
		free( memory );
		return false;
	}
	else if ( consumed < data.GetSize() )
	{
		log.Warn( "XEX: %d bytes of data was not consumed in block decompression (out of %d)", data.GetSize() - consumed, data.GetSize() );
	}

	// check if all data was outputed
	const uint32 numOutputed = (uint32)(destMemory - memory);
	if ( numOutputed > memorySize )
	{
		log.Error( "XEX: To much data was outputed in block decompression (%d > %d)", numOutputed, memorySize );
		free( memory );
		return false;
	}
	else if ( numOutputed < memorySize )
	{
		log.Warn( "XEX: %d bytes of data was not outputed in block decompression (out of %d)", memorySize - numOutputed, memorySize );
	}

	// loaded
	m_memoryData = memory;
	m_memorySize = memorySize;
	log.Log( "XEX: Decompressed %d bytes from %d disk bytes", memorySize, sourceSize );
	return true;
}

bool ImageBinaryXEX::LoadImageData( ILogOutput& log, ImageByteReaderXEX& data )
{
	// decompress and decrypt
	const XEXCompressionType compType = m_xexData.file_format_info.compression_type;
	switch ( compType )
	{
		case XEX_COMPRESSION_NONE:
		{
			log.Log( "XEX: image::Binary is not compressed" );
			return LoadImageDataUncompressed( log, data );
		}

		case XEX_COMPRESSION_BASIC:
		{
			log.Log( "XEX: image::Binary is using basic compression (zero blocks)" );
			return LoadImageDataBasic( log, data );
		}

		case XEX_COMPRESSION_NORMAL:
		{
			log.Log( "XEX: image::Binary is using normal compression" );
			return LoadImageDataNormal( log, data );
		}
	}

	// unsupported compression
	log.Error( "Image '%ls' is using unsupported compression mode %d and cannot be loaded", GetPath().c_str(), compType );
	return false; 
}

bool ImageBinaryXEX::LoadPEImage( ILogOutput& log, const uint8* fileData, const uint32 fileDataSize )
{
	ImageByteReaderXEX loader( m_memoryData, m_memorySize );

	// LOAD DOS header
	DOSHeader dosHeader;
	if ( !loader.Read( log, &dosHeader, sizeof(dosHeader) ) ) return false;
	if ( !dosHeader.Validate( log ) ) return false;

	// Move to the new header
	if ( !loader.Seek( log, dosHeader.e_lfanew ) ) return false;

	// Verify NT signature (PE\0\0).
	uint32 peSignature;
	if ( !loader.Read( log, &peSignature, sizeof(peSignature) ) ) return false;
	if ( peSignature != 0x00004550 )
	{
		log.Error( "PE: Missing PE signature" );
		return false;
	}

	// Load the file header
	COFFHeader coffHeader;
	if ( !loader.Read( log, &coffHeader, sizeof(COFFHeader) ) ) return false;

	// Verify matches an Xbox PE
	if ( coffHeader.Machine != 0x01F2 )
	{
		log.Error( "PE: Machine type does not match Xbox360 (found 0x%X)", coffHeader.Machine );
		return false;
	}

	// Should be 32-bit
	if ( (coffHeader.Characteristics & 0x0100) == 0 )
	{
		log.Error( "PE: Only 32-bit images are supported" );
		return false;
	}

	// Verify the expected size.
	if ( coffHeader.SizeOfOptionalHeader != 224 )
	{
		log.Error( "PE: Invalid size of optional header (got %d)", coffHeader.SizeOfOptionalHeader );
		return false;
	}

	// Read the optional header information
	PEOptHeader optHeader;
	if ( !loader.Read( log, &optHeader, sizeof(optHeader) ) ) return false;

	// Veriry the optional header is valid (32bit)
	if ( optHeader.signature != 0x10b )
	{
		log.Error( "PE: Invalid signature of optional header (got 0x%0X)", optHeader.signature );
		return false;
	}

	// Verify subsystem
	if ( optHeader.Subsystem != IMAGE_SUBSYSTEM_XBOX )
	{
		log.Error( "PE: Invalid subsystem (got %d)", optHeader.Subsystem );
		return false;
	}

	// Store the PE header
	m_peHeader = optHeader;

	// extend the virtual memory size
	uint32 extendedMemorySize = 0;

	// Read the sections
	const uint32 numSections = coffHeader.NumberOfSections;
	for ( uint32 i=0; i<numSections; ++i )
	{
		// load section description
		COFFSection section;
		if ( !loader.Read( log, &section, sizeof(section) ) ) return false;

		// get section name
		char sectionName[sizeof(section.Name) + 1];
		memcpy( sectionName, section.Name, sizeof(section.Name) );
		sectionName[ sizeof(section.Name) ] = 0;

		// exend the memory size
		const uint32 lastMemoryAddress = section.VirtualAddress + section.VirtualSize;
		if ( lastMemoryAddress > extendedMemorySize )
		{
			extendedMemorySize = lastMemoryAddress;
		}

		// create section info
		m_sections.push_back( CreateSection( section ) );

		// section info
		log.Log( "PE: Section '%s': physical (%d,%d), virtual (%d, %d)", 
			m_sections.back()->GetName(),
			section.PointerToRawData,
			section.SizeOfRawData,
			section.VirtualAddress,
			section.VirtualSize );
	}

	// extend image size
	if ( extendedMemorySize > m_memorySize )
	{
		// we have extended image data
		log.Warn( "PE: Image sections extend beyond virtual memory range loaded from file (%06Xh > %06Xh). Extending by %d bytes.",
			extendedMemorySize, m_memorySize,
			extendedMemorySize - m_memorySize );

		// resize image data
		const uint32 oldMemorySize = m_memorySize;
		uint8* newMemoryData = new uint8[ extendedMemorySize ];
		memset( newMemoryData, 0, extendedMemorySize );
		memcpy( newMemoryData, m_memoryData, m_memorySize );
		delete [] m_memoryData;
		m_memorySize = extendedMemorySize;
		m_memoryData = newMemoryData;

		// copy extra sectiomn
		for ( uint32 i=0; i<m_sections.size(); ++i )
		{
			const image::Section* section = m_sections[i];

			// no physical data
			if ( !section->GetPhysicalSize() )
				continue;

			// check physical size
			if ( section->GetPhysicalSize() + section->GetPhysicalOffset() > fileDataSize )
			{
				log.Warn( "PE: Section '%hs' lies outside any phyisical data we have %d (size %d)",
					section->GetName().c_str(), section->GetPhysicalSize(), section->GetPhysicalOffset() );

				continue;
			}

			// valid section offset ?
			if ( section->GetVirtualAddress() >= oldMemorySize )
			{
				log.Log( "PE: Copying section '%hs' directly from raw file from offset %d (size %d)",
					section->GetName().c_str(), section->GetPhysicalSize(), section->GetPhysicalOffset() );

				uint32 sizeToCopy = section->GetPhysicalSize();
				if ( section->GetVirtualSize() < sizeToCopy )
					sizeToCopy = section->GetVirtualSize();

				memcpy( newMemoryData + section->GetVirtualAddress(),
					fileData + section->GetPhysicalOffset(),
					sizeToCopy );
			}
		}
	}


	// Set the base address and entry point
	m_baseAddress = m_xexData.exe_address;
	m_entryAddress = m_xexData.exe_entry_point;

	// image::Binary data loaded
	return true;
}

bool ImageBinaryXEX::PatchImports( ILogOutput& log, const class platform::Definition* platform )
{
	// process the records to get the actuall imports
	// The number of records does not correspond to the number of imports!
	// Each record points at either a location in text or data - dereferencing the
	// pointer will yield a value that & 0xFFFF = the import ordinal,
	// >> 16 & 0xFF = import library index, and >> 24 & 0xFF = 0 if a variable
	// (just get address) or 1 if a thunk (needs rewrite).
	for ( uint32 i=0; i<m_xexData.import_records.size(); ++i )
	{
		const uint32 tableAddress = m_xexData.import_records[i];

		// stats
		log.SetTaskProgress( i, (uint32)m_xexData.import_records.size() );

		// get relative address
		const uint32 memOffset = tableAddress - GetBaseAddress();
		if ( memOffset > m_memorySize )
		{
			log.Error( "XEX: invalid import record offset: 0x%X", memOffset );
			return false;
		}

		// get the value in the memory
		uint32 value = *(const uint32*)( m_memoryData + memOffset );
		Swap32( &value );

		// Get record type
		const uint8 type = (value & 0xFF000000) >> 24;
		const uint8 libIndex = (value & 0x00FF0000) >> 16;
		
		// Import symbols
		if ( type == 0 )
		{
			if ( libIndex >= m_libNames.size() )
			{
				log.Error( "XEX: invalid import type 0 record lib index (%d, max:%d)", libIndex, m_libNames.size() );
				continue;
			}

			// get the export info
			const uint32 importOrdinal = (value & 0xFFFF);
			const std::string importLibName = m_libNames[ libIndex ];
			const uint32 importAddress = (m_xexData.import_records[i]);

			// find the function name
			std::string exportName;
			const uint32 exportLibraryVersion = 0;
			const auto* image = platform->GetExportLibrary() ? platform->GetExportLibrary()->FindImageByName( exportLibraryVersion, importLibName.c_str() ) : nullptr;
			if ( image )
			{
				// find the export
				const auto* exportInfo = image->FindExportByOrdinal( importOrdinal );
				if ( nullptr != exportInfo )
				{
					exportName = exportInfo->GetName();
				}
				else
				{
					log.Warn( "XEX: reference to unknown import 0x%X in '%s'", importOrdinal, importLibName.c_str() );
				}
			}
			else
			{
				log.Warn( "XEX: reference to unknown image '%s'", importLibName.c_str() );
			}

			// autogenerate import name if not known
			if ( exportName.empty() )
			{
				char autoExportName[ 256 ];
				sprintf_s( autoExportName, 256, "Export%d", importOrdinal );
				exportName = autoExportName;
			}

			// create import
			image::Import* im = new image::Import( this, exportName.c_str(), importLibName.c_str(), tableAddress, importAddress, image::Import::eImportType_Data );
			log.Log( "XEX: Import symbol '%s', table=%06Xh", exportName.c_str(), importAddress );
			m_imports.push_back( im );
		}

		// Import functions
		else if ( type == 1 )
		{
			if ( libIndex >= m_libNames.size() )
			{
				log.Error( "XEX: invalid import type 1 record lib index (%d, max:%d)", libIndex, m_libNames.size() );
				continue;
			}

			// get the export info
			const uint32 importOrdinal = (value & 0xFFFF);
			const std::string importLibName = m_libNames[ libIndex ];
			const uint32 importAddress = (m_xexData.import_records[i]);

			// find the function name
			std::string exportName;
			const uint32 exportLibraryVersion = 0;
			const auto* image = platform->GetExportLibrary() ? platform->GetExportLibrary()->FindImageByName( exportLibraryVersion, importLibName.c_str() ) : nullptr;
			if ( image )
			{
				// find the export
				const auto* exportInfo = image->FindExportByOrdinal( importOrdinal );
				if ( nullptr != exportInfo )
				{
					exportName = exportInfo->GetName();
				}
				else
				{
					log.Warn( "XEX: reference to unknown import 0x%X in '%s'", importOrdinal, importLibName.c_str() );
				}
			}
			else
			{
				log.Warn( "XEX: reference to unknown image '%s'", importLibName.c_str() );
			}

			// autogenerate import name if not known
			if ( exportName.empty() )
			{
				char autoExportName[ 256 ];
				sprintf_s( autoExportName, 256, "Export%d", importOrdinal );
				exportName = autoExportName;
			}

			// create import
			image::Import* im = new image::Import( this, exportName.c_str(), importLibName.c_str(), tableAddress, importAddress, image::Import::eImportType_Function );
			log.Log( "XEX: Import func '%s', table=%06Xh, entry=%06Xh", exportName.c_str(), tableAddress, importAddress );
			m_imports.push_back( im );
		}
	}

	// Done
	return true;
}

bool ImageBinaryXEX::DecryptBuffer( const uint8* key, const uint8* inputData, const uint32 inputSize, uint8* outputData, const uint32 outputSize )
{
	// no compression, just copy
	if ( m_xexData.file_format_info.encryption_type == XEX_ENCRYPTION_NONE )
	{
		if ( inputSize != outputSize )
		{
			return false;
		}

		memcpy( outputData, inputData, inputSize );
		return true;
	}

	// AES encryption
	uint32 rk[4 * (MAXNR + 1)];
	uint8 ivec[16] = {0};

	int32 nr = rijndaelKeySetupDec(rk, key, 128);
	const uint8* ct = inputData;
	uint8* pt = outputData;

	for ( uint32 n=0; n<inputSize; n += 16, ct += 16, pt += 16 )
	{
	    // Decrypt 16 uint8_ts from input -> output.
	    rijndaelDecrypt(rk, nr, ct, pt);

		// XOR with previous
		for ( uint32 i=0; i<16; i++)
		{
			pt[i] ^= ivec[i];
			ivec[i] = ct[i];
		}
	}

	return true;
}

image::Section* ImageBinaryXEX::CreateSection( const COFFSection& section )
{
	// copy name
	char localName[ sizeof(section.Name)+1 ];
	memcpy( localName, section.Name, sizeof(section.Name) );
	localName[sizeof(section.Name)] = 0;

	// create section info
	return new image::Section( 
		this, 
		localName,
		section.VirtualAddress,
		section.VirtualSize,
		section.PointerToRawData,
		section.SizeOfRawData,
		0 != (section.Flags & IMAGE_SCN_MEM_READ),
		0 != (section.Flags & IMAGE_SCN_MEM_WRITE),
		0 != (section.Flags & IMAGE_SCN_MEM_EXECUTE),
		"ppc" );
};

inline static bool ParseKeyword( const char*& stream, const char* match )
{
	const char* txt = stream;

	// eat white spaces
	while ( *txt <= ' ' )
	{
		if ( !*txt ) return false;
		++txt;
	}

	// matched ?
	const size_t len = strlen( match );
	if ( 0 == _strnicmp( txt, match, len ) )
	{
		stream = txt + len;
		return true;
	}

	// not matched
	return false;
}

inline static bool ParseText( const char*& stream, std::string& outText )
{
	const char* txt = stream;

	// eat white spaces
	while ( *txt <= ' ' )
	{
		if ( !*txt ) return false;
		++txt;
	}

	// parse untill another space (or end of line) is found
	while ( *txt > ' ' )
	{
		char temp[2] = {*txt,0};
		outText += temp;
		++txt;
	}

	// done
	stream = txt;
	return true;
}

inline static bool ParseHexValue( const char* start, uint32& outValue )
{
	// decode number
	const char* txt = start + strlen(start) - 1;
	uint32 ret = 0;
	uint32 b = 1;
	while ( txt >= start )
	{
		const char ch = *txt--;

		// get char value
		uint32 c = 0;
		if ( ch == 'A' || ch == 'a' ) c = 10;
		else if ( ch == 'B' || ch == 'b' ) c = 11;
		else if ( ch == 'C' || ch == 'c' ) c = 12;
		else if ( ch == 'D' || ch == 'd' ) c = 13;
		else if ( ch == 'E' || ch == 'e' ) c = 14;
		else if ( ch == 'F' || ch == 'f' ) c = 15;
		else if ( ch == 'F' || ch == 'f' ) c = 15;
		else if ( ch == 'F' || ch == 'f' ) c = 15;
		else if ( ch >= '0' && ch <= '9' ) c = (ch - '0');
		else 
		{
			return false;
		}

		// accumulate
		ret += b * c;
		b <<= 4;
	}

	// done
	outValue = ret;
	return true;
}

bool ImageBinaryXEX::LoadSymbols( ILogOutput& log, FILE* mapFile )
{
	log.SetTaskName( "Loading symbols..." );

	// get file size
	fseek(mapFile, 0, SEEK_END);
	auto fileSize = (uint32)ftell(mapFile);
	fseek(mapFile, 0, SEEK_SET);

	// parse the lines
	char line[512];
	bool hasSymbolTable = false;
	while (fgets(line, sizeof(line), mapFile) != NULL)
	{
		if ( NULL != strstr( line, "Address" ) )
		{
			if ( NULL != strstr( line, "Publics by Value" ) )
			{
				if ( NULL != strstr( line, "Rva+Base" ) )
				{
					hasSymbolTable = true;
					break;
				}
			}
		}
	}

	// scan symbols
	if ( hasSymbolTable )
	{
		while (fgets(line, sizeof(line), mapFile) != NULL)
		{
			std::string seg, name, addr, lib;
			bool isFunction = false;
			bool isInline = false;

			// update progress
			const auto pos = (uint32)ftell(mapFile);
			log.SetTaskProgress( pos, fileSize );

			// parse symbol info
			const char* stream = line;
			if ( ParseText( stream, seg ) )
			{
				if ( ParseText( stream, name ) )
				{
					if ( ParseText( stream, addr ) )
					{
						isFunction = ParseKeyword( stream, "f" );
						isInline = ParseKeyword( stream, "i" );

						if ( ParseText( stream, lib ) )
						{
							// symbol type
							image::Symbol::ESymbolType type = image::Symbol::eSymbolType_Variable;
							if ( isFunction ) type = image::Symbol::eSymbolType_Function;

							// parse the address
							uint32 addrValue = 0;
							if ( ParseHexValue( addr.c_str(), addrValue ) )
							{
								// create the symbol information
								const auto index = (uint32)m_symbols.size();
								image::Symbol* symbol = new image::Symbol( this, index, type, addrValue, name.c_str(), lib.c_str() );
								m_symbols.push_back( symbol );
							}
						}
					}
				}
			}
		}
	}

	// loaded
	log.Log( "Loaded %d symbols from map file", m_symbols.size() );
	return true;
}

//---------------------------------------------------------------------------

