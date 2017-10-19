#pragma once

namespace Parse
{
	extern bool ParseUrlKeyword(const char*& stream, const char* match);
	extern bool ParseUrlPart(const char*& stream, char* outData, const uint32 outDataMax );
	extern bool ParseUrlInteger(const char*& stream, int& outInteger);
	extern bool ParseUrlAddress(const char*& stream, uint32& outAddress);
} // Parse

//---------------------------------------------------------------------------

extern void ExtractDirW( const wchar_t* filePath, std::wstring& outDirectory );

//---------------------------------------------------------------------------
