#include "build.h"
#include "internalUtils.h"

#include "image.h"

#include "platformDefinition.h"

#include "decodingContext.h"
#include "decodingInstructionCache.h"
#include "decodingMemoryMap.h"
#include "decodingCommentMap.h"
#include "decodingNameMap.h"
#include "decodingAddressMap.h"

namespace decoding
{

	//---------------------------------------------------------------------------

	Context::Context(const std::shared_ptr<image::Binary>& module, const platform::Definition* platformDefinition)
		: m_baseOffsetRVA(module->GetBaseAddress())
		, m_entryPointRVA(module->GetEntryAddress())
		, m_platform(platformDefinition)
		, m_currentSection(nullptr)
		, m_currentInstructionCache(nullptr)
	{
		m_memoryMap = new MemoryMap();
		m_commentMap = new CommentMap(m_memoryMap);
		m_addressMap = new AddressMap(m_memoryMap);
		m_nameMap = new NameMap(m_memoryMap);
	}

	Context::~Context()
	{
		delete m_commentMap;
		m_commentMap = nullptr;

		delete m_addressMap;
		m_addressMap = nullptr;

		delete m_nameMap;
		m_nameMap = nullptr;

		delete m_memoryMap;
		m_memoryMap = nullptr;

		DeleteVector(m_codeSectionInstructionCaches);
	}

	void Context::ClearCachedData()
	{
		for (auto* cache : m_codeSectionInstructionCaches)
			cache->Clear();
	}

	Context* Context::Create(ILogOutput& log, const std::shared_ptr<image::Binary>& module, const platform::Definition* platformDefinition)
	{
		std::auto_ptr< Context > ctx(new Context(module, platformDefinition));
		ctx->m_image = module;

		// initialize the memory map
		if (!ctx->m_memoryMap->Initialize(log, *module))
			return nullptr;

		// find the code section
		const uint32 numSections = module->GetNumSections();
		for (uint32 i = 0; i < numSections; ++i)
		{
			const image::Section* section = module->GetSection(i);
			if (section && section->CanExecute())
			{
				// add to list of code sections
				ctx->m_codeSections.push_back(section);

				// find the cpu in the platform
				const auto cpuName = section->GetCPUName();
				const auto* cpuDefinition = platformDefinition->FindCPU(cpuName.c_str());
				if (nullptr == cpuDefinition)
				{
					log.Error("Decoding: Image section '%hs' is using unknown cpu '%hs'. This section will not be decoded.",
						section->GetName().c_str(), cpuName.c_str());
				}
				else
				{
					// create instruction cache for this executable section
					auto* instructionCache = new InstructionCache(section, ctx->m_memoryMap, cpuDefinition);
					ctx->m_codeSectionInstructionCaches.push_back(instructionCache);
				}
			}
		}

		// no code section found
		if (ctx->m_codeSections.empty())
		{
			log.Error("Decoding: No executable sections found in image '%ls'", module->GetPath().c_str());
			return nullptr;
		}

		// return created context
		return ctx.release();
	}

	const uint32 Context::ValidateInstruction(ILogOutput& log, const uint64_t codeAddress, const bool cached /*= true*/)
	{
		auto* cache = PrepareInstructionCache(codeAddress);
		if (cache != nullptr)
		{
			// decode the instruction (but do not get the full representation here)
			const uint32 size = cache->ValidateInstruction(log, codeAddress, cached);
			if (size > 0)
			{
				m_memoryMap->SetMemoryBlockLength(log, codeAddress, size);
				m_memoryMap->SetMemoryBlockType(log, codeAddress, (uint32)MemoryFlag::Executable, (uint32)MemoryFlag::GenericData);
				m_memoryMap->SetMemoryBlockSubType(log, codeAddress, (uint32)InstructionFlag::Valid, 0);
				return size;
			}
		}
		else
		{
			log.Error("Decoding: No CPU assigned to section at %08Xh", codeAddress);
		}

		// nothing decoded
		return 0;
	}

	const uint32 Context::DecodeInstruction(ILogOutput& log, const uint64_t codeAddress, Instruction& outInstruction, const bool cached /*= true*/)
	{
		auto* cache = PrepareInstructionCache(codeAddress);
		if (cache != nullptr)
		{
			// decode the instruction (but do not get the full representation here)
			const uint32 size = cache->DecodeInstruction(log, codeAddress, outInstruction, cached);
			if (size > 0)
			{
				m_memoryMap->SetMemoryBlockLength(log, codeAddress, size);
				m_memoryMap->SetMemoryBlockType(log, codeAddress, (uint32)MemoryFlag::Executable, (uint32)MemoryFlag::GenericData);
				m_memoryMap->SetMemoryBlockSubType(log, codeAddress, (uint32)InstructionFlag::Valid, 0);
				return size;
			}
		}
		else
		{
			log.Error("Decoding: No CPU assigned to section at %08Xh", codeAddress);
		}

		// nothing decoded;
		return 0;
	}

	const bool Context::GetFunctionName(const uint32 codeAddress, std::string& outFunctionName, uint32& outFunctionStart) const
	{
		uint32 addr = codeAddress;
		while (m_memoryMap->GetMemoryInfo(addr).IsExecutable())
		{
			// function start ?
			if (m_memoryMap->GetMemoryInfo(addr).GetInstructionFlags().IsFunctionStart())
			{
				outFunctionStart = addr;
				outFunctionName = m_nameMap->GetName(addr);
				return true;
			}

			// HACK!
			addr -= 4;
		}

		// invalid address
		return false;
	}

	InstructionCache* Context::PrepareInstructionCache(const uint64_t codeAddress) const
	{
		const auto base = m_image->GetBaseAddress();

		// use the cached instruction cache if still valid
		if (m_currentSection)
		{
			const auto sectionStart = m_currentSection->GetVirtualOffset() + base;
			const auto sectionEnd = sectionStart + m_currentSection->GetVirtualSize();

			if (codeAddress >= sectionStart && codeAddress < sectionEnd)
				return m_currentInstructionCache;
		}

		// find matching instruction cache, if a match is found update the cached pointers
		for (auto* cache : m_codeSectionInstructionCaches)
		{
			const auto* section = cache->GetImageSection();

			const auto sectionStart = section->GetVirtualOffset() + base;
			const auto sectionEnd = sectionStart + section->GetVirtualSize();

			if (codeAddress >= sectionStart && codeAddress < sectionEnd)
			{
				m_currentSection = section;
				m_currentInstructionCache = cache;
				return cache;
			}
		}

		// no section matches the address
		return nullptr;
	}

	//---------------------------------------------------------------------------

} // decoding