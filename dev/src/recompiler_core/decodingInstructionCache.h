#pragma once

#include "decodingInstruction.h"

namespace image
{
	class Section;
}

namespace platform
{
	class CPU;
}

namespace decoding
{
	class MemoryMap;
	class Instruction;

	/// Quick lookup so we don't decode the instructions over and over
	class RECOMPILER_API InstructionCache
	{
	public:
		InstructionCache( const image::Section* section, class MemoryMap* memory, const platform::CPU* decodingCPU );
		~InstructionCache();

		// get owning section
		inline const image::Section* GetImageSection() const { return m_imageSection; }

		// clear the cache
		void Clear();

		// validate location, returns instruction size or 0 if invalid
		const uint32 ValidateInstruction( ILogOutput& log, const uint32 codeAddress, const bool cached = true ) const;

		// fully decode the (assumed valid) instruction at given address
		const uint32 DecodeInstruction( ILogOutput& log, const uint32 codeAddress, Instruction& outInstruction, const bool cached = true ) const;

	public:
		inline const platform::CPU* GetCPU() const { return m_cachedCPU; }

	private:
		const static uint32 MAX_CACHED_DECODED_INSTRUCTIONS		= 8192;
		const static uint32 MAX_CACHED_VALIDATED_INSTRUCTIONS	= 65535;

		// image section that is governed by this instruction cache
		const image::Section*	m_imageSection;

		// image memory range
		uint64 m_imageBaseAddress;
		uint64 m_imageDataSize;
		const uint8* m_imageDataPtr;

		// cpu name
		const platform::CPU*	m_cachedCPU;

		// reference to the memory map
		class MemoryMap*		m_memoryMap;

		// fully decoded instructions
		typedef std::unordered_map< uint32, Instruction >	TDecodedInstructions;
		mutable TDecodedInstructions	m_decodedInstructions;

		// validation data - size of the instruction at given location
		typedef std::unordered_map< uint32, uint32 >			TValidatedInstructions;
		mutable TValidatedInstructions	m_validatedInstructions;
	};

} // decoding
