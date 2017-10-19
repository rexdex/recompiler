#pragma once

//---------------------------------------------------------------------------

#include "../recompiler_core/build.h"

#include "xexformat.h"

#include <Windows.h>
#include "../recompiler_core/image.h"

//---------------------------------------------------------------------------

static void UnixTimeToFileTime( const uint32 t, LPFILETIME pft )
{
	LONGLONG ll = Int32x32To64(t, 10000000) + 116444736000000000;
	pft->dwLowDateTime = (DWORD)ll;
	pft->dwHighDateTime = ll >> 32;
}

static std::string FileTimeToString( const LPFILETIME pft )
{
	FILETIME localFileTime;
	if ( FileTimeToLocalFileTime( pft, &localFileTime ) )
	{
		SYSTEMTIME st;
		if ( FileTimeToSystemTime( &localFileTime, &st ) )
		{
			char temp[256], temp2[ 256 ];

			GetDateFormatA( LOCALE_USER_DEFAULT, DATE_LONGDATE, &st, NULL, temp, 255 );
			GetTimeFormatA( LOCALE_USER_DEFAULT, 0, &st, NULL, temp2, 255 );

			std::string ret = temp;
			ret += " ";
			ret += temp2;

			return ret;
		}
	}

	return "N/A";
}

static std::string UnixTimeToString(const uint32 t)
{
	FILETIME ft;
	UnixTimeToFileTime(t, &ft);
	return FileTimeToString(&ft);
}

//---------------------------------------------------------------------------

/// image::Binary builder
class ImageBinaryXEX : public image::Binary
{
public:
	ImageBinaryXEX( const wchar_t* path );

	//! Load from file, returns the loaded image
	bool Load( ILogOutput& log, const class platform::Definition* platform );

private:
	// release internal data
	void ReleaseInternalData();

	// set the version information
	void SetVersionInfo( const XEXVersion& version, const XEXVersion& minVersion );

	// try to load symbols for the module
	bool LoadSymbols( ILogOutput& log );

	// resolve imports
	void ResolveImports( ILogOutput& log );

	// Load XEX headers
	bool LoadHeaders( ILogOutput& log, ImageByteReaderXEX& data );

	// Load image data
	bool LoadImageData( ILogOutput& log, ImageByteReaderXEX& data );

	// Load image data for uncompressed image
	bool LoadImageDataUncompressed( ILogOutput& log, ImageByteReaderXEX& data );

	// Load image data for normal image
	bool LoadImageDataNormal( ILogOutput& log, ImageByteReaderXEX& data );

	// Load image data for image with basic compression 
	bool LoadImageDataBasic( ILogOutput& log, ImageByteReaderXEX& data );

	// Load the PE image and PE sections
	bool LoadPEImage( ILogOutput& log, const uint8* fileData, const uint32 fileDataSize );

	// Load symbols from map file
	bool LoadSymbols( ILogOutput& log, FILE* mapFile );

	// Patch image imports and setup the jump table
	bool PatchImports( ILogOutput& log, const class platform::Definition* platform );

	// Decrypt memory buffer
	bool DecryptBuffer( const uint8* key, const uint8* inputData, const uint32 inputSize, uint8* outputData, const uint32 outputSize );

	// Create image section
	image::Section* CreateSection( const COFFSection& section );

private:
	// file header
	XEXImageData			m_xexData;
	PEOptHeader				m_peHeader;

	// library names
	std::vector< std::string >		m_libNames;

	// version info for the import module
	XEXVersion				m_version;
	XEXVersion				m_minimalVersion;
};

//---------------------------------------------------------------------------
