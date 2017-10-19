#include "build.h"
#include "utils.h"
#include "../recompiler_core/platformCPU.h"

namespace Parse
{

	bool ParseUrlKeyword(const char*& stream, const char* match)
	{
		const char* txt = stream;

		while (*txt && *txt <= ' ')
			++txt;

		const uint32 len = strlen(match);
		if (0 == strncmp(txt, match, len))
		{
			stream = txt + len;
			return true;
		}

		return false;
	}

	bool ParseUrlPart(const char*& stream, char* outData, const uint32 outDataMax )
	{
		const char* txt = stream;

		while (*txt && *txt <= ' ')
			++txt;

		uint32 writePos = 0;

		while (*txt)
		{
			if (*txt == ';' || *txt == ',' || *txt == '?' || *txt == '&')
			{
				txt += 1;
				break;
			}

			if (writePos >= (outDataMax-1))
				return false;

			outData[writePos++] = *txt++;
		}

		outData[writePos] = 0;
		stream = txt;
		return true;
	}

	bool ParseUrlInteger(const char*& stream, int& outInteger)
	{
		const char* txt = stream;

		while (*txt && *txt <= ' ')
			++txt;

		char buf[32];
		if ( !ParseUrlPart(txt, buf, 32) )
			return false;

		if (1 != sscanf_s(buf, "%d", &outInteger))
			return false;

		stream = txt;
		return true;
	}

	bool ParseUrlAddress(const char*& stream, uint32& outAddress)
	{
		const char* txt = stream;

		while (*txt && *txt <= ' ')
			++txt;

		char buf[32];
		if ( !ParseUrlPart(txt, buf, 32) )
			return false;

		const uint32 len = strlen(buf);
		if ( !len )
			return false;

		if (buf[len-1] == 'h')
		{
			buf[len-1] = 0;

			if (1 != sscanf_s(buf, "%X", &outAddress))
				return false;
		}
		else if (buf[0] == '0' && buf[1] == 'x')
		{
			if (1 != sscanf_s(buf+2, "%X", &outAddress))
				return false;
		}
		else
		{
			if (1 != sscanf_s(buf, "%u", &outAddress))
				return false;
		}

		stream = txt;
		return true;
	}	

} // Parse

//---------------------------------------------------------------------------

void ExtractDirW( const wchar_t* filePath, std::wstring& outDirectory )
{
	// find the file division
	const wchar_t* filePart = wcsrchr( filePath, '\\' );
	const wchar_t* filePart2 = wcsrchr( filePath, '/' );
	filePart = (filePart > filePart2) ? filePart : filePart2;

	// get the directory
	if ( NULL != filePart )
	{
		outDirectory = std::wstring(filePath, filePart-filePath);
		outDirectory += L"\\";
	}
	else
	{
		outDirectory = L"\\";
	}
}

//---------------------------------------------------------------------------
