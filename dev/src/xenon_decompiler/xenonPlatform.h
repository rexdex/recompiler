#pragma once

//---------------------------------------------------------------------------

#include "../recompiler_core/build.h"
#include "../recompiler_core/platformDecompilation.h"

#include "xexformat.h"

//---------------------------------------------------------------------------

/// Platform decompilation interface for Xenon
class DecompilationXenon : public platform::DecompilationInterface
{
public:
	DecompilationXenon();
	virtual ~DecompilationXenon();

	//! platform::DecompilationInterface interface
	virtual void Release() override final;

	virtual const char* GetName() const override final;
	virtual const char* GetLauncherName() const override final;

	virtual const uint32 GetNumExtensions() const override final;
	virtual const char* GetExtension(const uint32 index) const override final;
	virtual const char* GetExtensionName(const uint32 index) const override final;

	virtual const uint32 GetNumCPU() const override final;
	virtual const platform::CPU* GetCPU(const uint32 index) const final;

	virtual std::shared_ptr<image::Binary> LoadImageFromFile(class ILogOutput& log, const platform::Definition* platform, const wchar_t* absolutePath) const override final;

	virtual bool DecodeImage(ILogOutput& log, decoding::Context& context) const final;
	virtual bool ExportCode(ILogOutput& log, decoding::Context& decodingContext, const Commandline& settings, class code::IGenerator& codeGen) const final;
};

//---------------------------------------------------------------------------
