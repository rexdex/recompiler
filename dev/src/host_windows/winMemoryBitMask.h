#pragma once

namespace win
{
	template< uint32 MAX_ENTRIES >
	struct BitMask
	{
		uint32		m_bits[(MAX_ENTRIES + 31) / 32];

		inline BitMask()
		{
			memset(&m_bits, 0, sizeof(m_bits));
		}

		inline bool IsSet(const uint32 index) const
		{
			const uint32 mask = 1 << (index & 31);
			return 0 != (m_bits[index / 32] & mask);
		}

		inline void Set(const uint32 index)
		{
			const uint32 mask = 1 << (index & 31);
			m_bits[index / 32] |= mask;
		}

		inline void Clear(const uint32 index)
		{
			const uint32 mask = 1 << (index & 31);
			m_bits[index / 32] &= ~mask;
		}

		inline void Toggle(const uint32 index, const bool isSet)
		{
			const uint32 mask = 1 << (index & 31);
			if (isSet)
				m_bits[index / 32] |= mask;
			else
				m_bits[index / 32] &= ~mask;
		}
	};

} // win