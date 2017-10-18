#include "build.h"
#include "bitset.h"

namespace utils
{

	BitSet::BitSet(const uint32 numBits /*= 0*/)
		: m_numBits(0)
	{
		Resize(numBits);
	}

	void BitSet::Resize(const uint32 numBits /*= 0*/)
	{
		if (m_numBits != numBits)
		{
			const auto oldBitCount = m_numBits;
			const auto numWords = (numBits + 31) / 32;
			m_bits.resize(numWords, 0);

			const auto maxBit = std::min<uint32>(numBits, (oldBitCount & 31) + 31);
			for (uint32 i = oldBitCount; i < maxBit; ++i)
				Clear(i);
		}
	}

	void BitSet::ClearAll()
	{
		memset(m_bits.data(), 0x00, m_bits.size() * sizeof(uint32));
	}

	void BitSet::SetAll()
	{
		memset(m_bits.data(), 0xFF, m_bits.size() * sizeof(uint32));
	}

} // utils
