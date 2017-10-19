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

const char* DecompilationXenon::GetLauncherName() const
{
	return "xenon_launcher";
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

const uint32 DecompilationXenon::GetNumExtensions() const
{
	return 1;
}

const char* DecompilationXenon::GetExtension(const uint32 index) const
{
	switch (index)
	{
		case 0: return "xex";
	}

	return "";
}

const char* DecompilationXenon::GetExtensionName(const uint32 index) const
{
	switch (index)
	{
		case 0: return "Xbox360 Executable";
	}

	return "";
}

std::shared_ptr<image::Binary> DecompilationXenon::LoadImageFromFile(class ILogOutput& log, const platform::Definition* platform, const wchar_t* absolutePath) const
{
	std::shared_ptr<ImageBinaryXEX> dataLoader(new ImageBinaryXEX(absolutePath));
	if (!dataLoader->Load(log, platform))
		return nullptr;

	return dataLoader;
}

const uint32 DecompilationXenon::GetNumCPU() const
{
	return 1;
}

const platform::CPU* DecompilationXenon::GetCPU(const uint32 index) const
{
	if (0 == index)
		return &CPU_XenonPPC::GetInstance();

	return nullptr;
}

//---------------------------------------------------------------------------

extern "C" __declspec(dllexport) platform::DecompilationInterface* GetPlatformInterface()
{
	static DecompilationXenon theInstance;
	return &theInstance;
}

//---------------------------------------------------------------------------
