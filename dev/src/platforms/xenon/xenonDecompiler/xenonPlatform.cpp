#include "xenonPlatform.h"
#include "xenonImageLoader.h"
#include "xenonCPU.h"

//---------------------------------------------------------------------------

DecompilationXenon::DecompilationXenon()
{	
}

DecompilationXenon::~DecompilationXenon()
{
}

void DecompilationXenon::Release()
{
	// nothing
}

const char* DecompilationXenon::GetName() const
{
	return "Xbox360";
}

const char* DecompilationXenon::GetExportLibrary() const
{
	return "Recompiler.Xenon.Platform.exports";
}

static std::wstring MakeIncludeDirectory()
{
	wchar_t modulePath[512];
	modulePath[0] = 0;
	if (!GetModuleFileNameW(GetModuleHandle(NULL), modulePath, ARRAYSIZE(modulePath)))
		return std::wstring();

	wchar_t* str = wcsrchr(modulePath, '\\');
	if (!str)
		return L"";
	*str = 0;

	wcscat_s(modulePath, L"\\..\\..\\dev\\src\\platforms\\xenon\\xenonLauncher\\");
	return modulePath;
}

const wchar_t* DecompilationXenon::GetIncludeDirectory() const
{
	static std::wstring includeDir = MakeIncludeDirectory();
	return includeDir.c_str();
}

const uint32 DecompilationXenon::GetNumExtensions() const
{
	return 1;
}

const char* DecompilationXenon::GetExtension( const uint32 index ) const
{
	switch ( index )
	{
		case 0: return "xex";
	}

	return "";
}

const char* DecompilationXenon::GetExtensionName( const uint32 index ) const
{
	switch ( index )
	{
		case 0: return "Xbox360 Executable";
	}

	return "";
}

image::Binary* DecompilationXenon::LoadImageFromFile( class ILogOutput& log, const platform::Definition* platform, const wchar_t* absolutePath ) const
{
	ImageBinaryXEX* dataLoader = new ImageBinaryXEX( absolutePath );
	if ( !dataLoader->Load( log, platform ) )
	{
		delete dataLoader;
		return NULL;
	}

	return dataLoader;
}

const uint32 DecompilationXenon::GetNumCPU() const
{
	return 1;
}

const platform::CPU* DecompilationXenon::GetCPU( const uint32 index ) const
{
	if (0 == index)
		return &CPU_XenonPPC::GetInstance();

	return nullptr;
}

//---------------------------------------------------------------------------

extern "C" void* __stdcall CreateInterface( int version )
{
	if ( version == 1 )
	{
		static DecompilationXenon theInstance;
		return &theInstance;
	}

	return nullptr;
}

//---------------------------------------------------------------------------
