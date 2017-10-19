#include "build.h"

#include "image.h"

#include "platformCPU.h"

#include "decodingInstructionCache.h"
#include "decodingMemoryMap.h"
#include "decodingInstruction.h"

namespace decoding
{

	InstructionCache::InstructionCache( const image::Section* imageSection, class MemoryMap* memory, const platform::CPU* decodingCPU )
		: m_memoryMap( memory )
		, m_imageSection( imageSection )
		, m_cachedCPU( decodingCPU )
	{
		m_imageBaseAddress = imageSection->GetVirtualOffset();
		m_imageDataSize = imageSection->GetVirtualSize();
		m_imageDataPtr = (const uint8*)imageSection->GetImage()->GetMemory() + (m_imageBaseAddress - imageSection->GetImage()->GetBaseAddress());
	}

	InstructionCache::~InstructionCache()
	{
		Clear();
	}

	void InstructionCache::Clear()
	{
		m_decodedInstructions.clear();
		m_validatedInstructions.clear();
	}

	const uint32 InstructionCache::ValidateInstruction( ILogOutput& log, const uint64_t codeAddress, const bool cached /*= true*/ ) const
	{
		// is this a executable memory ?
		if ( !m_memoryMap->GetMemoryInfo( codeAddress ).IsExecutable() )
		{
			log.Error( "Decode: Unable to decode instruction from non-exectuable memory %08Xh", codeAddress );
			return 0;
		}

		// thunk ?
		if ( m_memoryMap->GetMemoryInfo( codeAddress ).GetInstructionFlags().IsThunk() )
		{
			m_validatedInstructions[ codeAddress ] = 4;
			return 4;
		}

		// find in the map
		if ( cached )
		{
			TValidatedInstructions::const_iterator it = m_validatedInstructions.find( codeAddress );
			if ( it != m_validatedInstructions.end() )
			{
				return (*it).second;
			}

			// clear cache if full
			if ( m_validatedInstructions.size() >= MAX_CACHED_VALIDATED_INSTRUCTIONS )
			{
				m_validatedInstructions.clear();
			}
		}

		// look up the CPU definition
		if ( !m_cachedCPU )
		{
			log.Error( "Decode: image::Binary is using undefined CPU" );
			return 0;
		}

		// build input stream
		const uint64 localOffset = codeAddress - m_imageBaseAddress;
		const uint8* memoryStart = m_imageDataPtr + localOffset;

		// decode instruction into the full form
		const uint32 size = m_cachedCPU->ValidateInstruction(log, memoryStart);

		// store in cache
		if ( cached )
			m_validatedInstructions[ codeAddress ] = size;

		return size;
	}

	const uint32 InstructionCache::DecodeInstruction( ILogOutput& log, const uint64_t codeAddress, Instruction& outInstruction, const bool cached /*= true*/ ) const
	{
		// is this a executable memory ?
		if ( !m_memoryMap->GetMemoryInfo( codeAddress ).IsExecutable() )
		{
			log.Error( "Decode: Unable to decode instruction from non-exectuable memory %08Xh", codeAddress );
			return 0;
		}

		// thunk ?
		if ( m_memoryMap->GetMemoryInfo( codeAddress ).GetInstructionFlags().IsThunk() )
			return 0;

		// find in the map
		if ( cached )
		{
			TDecodedInstructions::const_iterator it = m_decodedInstructions.find( codeAddress );
			if ( it != m_decodedInstructions.end() )
			{
				outInstruction = (*it).second;
				return outInstruction.GetCodeSize();
			}

			// clear cache if full
			if ( m_decodedInstructions.size() >= MAX_CACHED_DECODED_INSTRUCTIONS )
			{
				m_decodedInstructions.clear();
			}
		}

		// look up the CPU definition
		if ( !m_cachedCPU )
		{
			log.Error( "Decode: image::Binary is using undefined CPU" );
			return 0;
		}

		// build input stream
		const uint64 localOffset = codeAddress - m_imageBaseAddress;
		const uint8* memoryStart = m_imageDataPtr + localOffset;

		// decode instruction into the full form
		const uint32 size = m_cachedCPU->DecodeInstruction( log, memoryStart, outInstruction );
		if (cached && size)
		{
			m_decodedInstructions[ codeAddress ] = outInstruction;
		}

		// return decoded instruction
		return size;
	}

} // decoding