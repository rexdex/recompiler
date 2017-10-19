#pragma once

namespace utils
{
	class LAUNCHER_API BitSet
	{
	public:
		BitSet(const uint32 numBits = 0);
		BitSet(const BitSet& other) = default;
		BitSet(BitSet&& other) = default;

		BitSet& operator=(const BitSet& other) = default;
		BitSet& operator=(BitSet&& other) = default;

		inline uint32 GetSize() const { return m_numBits; }

		void Resize(const uint32 numBits = 0);

		void ClearAll();
		void SetAll();

		inline bool IsSet(const uint32 index) const
		{
			const uint32 mask = GetBitMask(index);
			return 0 != (GetWord(index) & mask);
		}

		inline void Set(const uint32 index)
		{
			const uint32 mask = GetBitMask(index);
			GetWord(index) |= mask;
		}

		inline void Clear(const uint32 index)
		{
			const uint32 mask = GetBitMask(index);
			GetWord(index) &= ~mask;
		}

		inline void Toggle(const uint32 index, const bool isSet)
		{
			const uint32 mask = GetBitMask(index);
			if (isSet)
				GetWord(index) |= mask;
			else
				GetWord(index) &= ~mask;
		}

	private:		
		uint32 m_numBits;
		std::vector<uint32> m_bits;

		inline const uint32 GetBitMask(const uint32 index) const
		{
			return 1 << (index & 31);
		}

		inline const uint32& GetWord(const uint32 index) const
		{
			return m_bits.data()[index / 32];
		}


		inline uint32& GetWord(const uint32 index)
		{
			return m_bits.data()[index / 32];
		}

	};

} // utils