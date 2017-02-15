#pragma once

//---------------------------------------------------------------------------

#include "../../../framework/backend/build.h"

#include "xexformat.h"

#include <Windows.h>
#include "../../../framework/backend/image.h"

//---------------------------------------------------------------------------

template < class T >
static inline void SafeRelease( T*& ptr )
{
	if ( nullptr != ptr )
	{
		ptr->Release();
		ptr = nullptr;
	}
}

template < class T >
static inline void SafeAddRef( T*& ptr )
{
	if ( nullptr != ptr )
	{
		ptr->AddRef();
	}
}

template < class T >
static inline T SafeArray( const std::vector<T>& ptr, const int index, T defaultValue = T(0) )
{
	if ( index < 0 || index >= (int)ptr.size() )
	{
		return defaultValue;
	}

	return ptr[index];
}

template < class T >
static inline void ReleaseVector( std::vector<T*>& table )
{
	for ( std::vector<T*>::const_iterator it = table.begin();
		it != table.end(); ++it )
	{
		(*it)->Release();
	}

	table.clear();
}

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

static std::string UnixTimeToString( const uint32 t )
{
	FILETIME ft;
	UnixTimeToFileTime( t, &ft );
	return FileTimeToString( &ft );
}

static void ConcatParam( std::string& txt, const char* name )
{
	if ( txt.empty() )
	{
		txt += ", ";
	}

	txt += name;
}

static const char* VersionString( int major, int minor )
{
	static char s[16];
	sprintf_s( s, 16, "%d.%d", major, minor );
	return s;
}

static inline void RemoveEndingSlash( wchar_t* str )
{
	size_t len = wcslen( str );
	if ( len > 0 )
	{
		wchar_t end = str[ len-1 ];
		if ( end == '/' || end == '\\' ) 
		{
			str[ len-1 ] = 0;
		}
	}
}

static inline void RemoveFileName( wchar_t* str )
{
	wchar_t* end = wcsrchr( str, '\\' );
	if ( !end ) end = wcsrchr( str, '/' );
	if ( end ) *end = 0;
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
