#include "build.h"

#include "../recompiler_core/internalUtils.h"
#include "../recompiler_core/decodingEnvironment.h"
#include "../recompiler_core/codeGenerator.h"
#include "../recompiler_core/decodingContext.h"
#include "../recompiler_core/platformDefinition.h"
#include "../recompiler_core/platformDecompilation.h"
#include "../recompiler_core/platformLibrary.h"
#include "../recompiler_core/externalApp.h"

///--

class ExternalAppRunner : public code::IGeneratorRemoteExecutor
{
public:
	ExternalAppRunner(ILogOutput& log)
		: m_parentLog(&log)
	{}

	virtual const uint32 RunExecutable(ILogOutput& log, const std::wstring& executablePath, const std::wstring& executableArguments) override final
	{
		m_parentLog->Log("Executor: Running '%ls %ls'", executablePath.c_str(), executableArguments.c_str());
		return RunApplication(log, executablePath, executableArguments);
	}

private:
	ILogOutput* m_parentLog;
};

///--

const int RunRecompiler(const Commandline& cmdLine, ILogOutput& log)
{
	// load the image
	const auto imagePath = cmdLine.GetOptionValueW("in");
	if (imagePath.empty())
	{
		log.Error("Recompiler: Input path to source rpi image (-in) not specified");
		return -2;
	}

	// get the output directory name where stuff will be output
	const auto outputDirPath = cmdLine.GetOptionValueW("out");
	if (outputDirPath.empty())
	{
		log.Error("Recompiler: Output path to directory with compiled stuff (-out) not specified");
		return -2;
	}

	// get the code generator name
	const auto codeGeneratorName = cmdLine.GetOptionValueA("generator");
	if (codeGeneratorName.empty())
	{
		log.Error("Recompiler: Output path to directory with compiled stuff (-out) not specified");
		return -2;
	}

	// load the image
	const auto env = decoding::Environment::Load(log, imagePath);
	if (!env)
	{
		log.Error("Recompiler: Decoding environment failed to load from '%ls'", imagePath.c_str());
		return -2;
	}

	// create the code generator
	const auto gen = code::IGenerator::CreateGenerator(log, cmdLine);
	if (!gen)
		return -2;

	// create the temp path
	const auto tempPath = outputDirPath + L"temp/";
	const auto outPath = outputDirPath + L"code.bin";

	// generate the temp code
	auto* decompilation = env->GetDecodingContext()->GetPlatform()->GetDecompilationInterface();
	if (!decompilation->ExportCode(log, *env->GetDecodingContext(), cmdLine, *gen))
	{
		log.Error("Recompiler: Failed to export code for recompilation");
		return -2;
	}

	// finalize
	ExternalAppRunner appRunner(log);
	if (!gen->CompileModule(appRunner, tempPath, outPath))
	{
		log.Error("Recompiler: Failed to compile recompiled code");
		return -2;
	}

	// done
	return 0;
}

///--

class ConsoleLogOutput : public ILogOutput
{
public:
	ConsoleLogOutput(const bool verbose)
		: m_verbose(verbose)
	{}

	virtual void DoLog(const LogLevel level, const char* buffer) override final
	{
		if (!m_verbose && level == LogLevel::Info)
			return;

		if (level == LogLevel::Info)
		{
			fprintf(stdout, "%hs\n", buffer);
			fflush(stdout);
		}
		else
		{
			fprintf(stderr, "%hs\n", buffer);
			fflush(stderr);
		}
	}

private:
	bool m_verbose;
};

int main(int argc, const char** argv)
{
	// display the help
	if (argc < 2 || argv[1] == "--help" || argv[1] == "/?")
	{
		fprintf(stdout, "Recompiler\n");
		fprintf(stdout, "(C) 2013-2017 by Rex Dex\n");
		fprintf(stdout, "\n");
		fprintf(stdout, "Usage:\n");
		fprintf(stdout, "  recompiler_api -command=commandName [options]\n");
		fprintf(stdout, "\n");
		fprintf(stdout, "Commands:\n");
		fprintf(stdout, "  decompile -platform=<platform> -in=<image> -out=<path> [options]\n");
		fprintf(stdout, "  recompile -in=<image> -out=<path> [options]\n");
		return -1;
	}

	// parse options
	Commandline cmdLine(argv, argc);

	// create log
#ifdef _DEBUG
	const bool verbose = true;
#else
	const bool verbose = cmdLine.HasOption("verbose");
#endif
	ConsoleLogOutput log(verbose);

	// initialize platform library
	if (!platform::Library::GetInstance().Initialize(log))
	{
		log.Error("No decompilation platforms found");
		return -2;
	}

	// get the command name
	const auto commandName = cmdLine.GetOptionValueA("command");
	if (commandName.empty())
	{
		log.Error("No command specified");
		return -1;
	}

	// run commands
	if (commandName == "recompile")
	{
		return RunRecompiler(cmdLine, log);
	}
	else
	{
		log.Error("Command '%hs' was not recognized", commandName.c_str());
		return -2;
	}
}
;