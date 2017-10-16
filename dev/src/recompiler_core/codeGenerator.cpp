#include "build.h"
#include "codeGenerator.h"
#include "codeGeneratorMSVC.h"
#include "internalUtils.h"

namespace code
{

	IGenerator::~IGenerator()
	{}

	void IGenerator::ListGenerators(std::vector<std::string>& outCodeGenerators)
	{
		outCodeGenerators.push_back("cpp_msvc");
		outCodeGenerators.push_back("cpp_clang"); // TODO
		outCodeGenerators.push_back("llvm"); // TODO
	}

	std::shared_ptr<IGenerator> IGenerator::CreateGenerator(ILogOutput& log, const Commandline& parameters)
	{
		// generator parameter is required
		const auto generatorName = parameters.GetOptionValueA("generator");
		if (generatorName.empty())
		{
			log.Error("CodeGen: Missing generator name");
			return nullptr;
		}

		// create the generator
		std::shared_ptr<IGenerator> gen;
		if (generatorName == "cpp_msvc")
		{
			gen = std::make_shared<msvc::Generator>(log, parameters);
		}
		else
		{
			log.Error("CodeGen: Unknown code generator '%hs'", generatorName.c_str());
			return nullptr;
		}

		return gen;
	}

} // code