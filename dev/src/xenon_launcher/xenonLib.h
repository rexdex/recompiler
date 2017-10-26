#pragma once

#include "../host_core/runtimeSymbols.h"

namespace xenon
{

	//----

	// bytes are not swapper
	template< typename T >
	struct DataMemoryDirect
	{
		__forceinline static const T Read(const void* ptr)
		{
			return *(const T*)ptr;
		}

		__forceinline static void Write(void* ptr, const T value)
		{
			xenon::TagMemoryWrite(ptr, sizeof(T), "DataMemoryDirect");
			*(T*)ptr = value;
		}
	};

	// swap for 2-bytes
	template< typename T >
	struct DataMemorySwap2
	{
		static_assert(sizeof(T) == 2, "Size of template's type must be 2 bytes");

		union U
		{
			T t;
			uint16_t u;
		};

		__forceinline static const T Read(const void* ptr)
		{
			const auto u = _byteswap_ushort(*(const uint16_t*)ptr);
			return ((U*)&u)->t;
		}

		__forceinline static void Write(void* ptr, const T value)
		{
			*(uint16_t*)ptr = _byteswap_ushort(((const U*)&value)->u);
			xenon::TagMemoryWrite(ptr, sizeof(T), "DataMemoryDirect");
		}
	};

	// swap for 4-bytes
	template< typename T >
	struct DataMemorySwap4
	{
		static_assert(sizeof(T) == 4, "Size of template's type must be 4 bytes");

		union U
		{
			T t;
			uint32_t u;
		};

		__forceinline static const T Read(const void* ptr)
		{
			const auto u = _byteswap_ulong(*(const uint32_t*)ptr);
			return ((U*)&u)->t;
		}

		__forceinline static void Write(void* ptr, const T value)
		{
			*(uint32_t*)ptr = _byteswap_ulong(((const U*)&value)->u);
			xenon::TagMemoryWrite(ptr, sizeof(T), "DataMemoryDirect");
		}
	};

	// swap for 8-bytes
	template< typename T >
	struct DataMemorySwap8
	{
		static_assert(sizeof(T) == 8, "Size of template's type must be 8 bytes");

		union U
		{
			T t;
			uint64_t u;
		};

		__forceinline static const T Read(const void* ptr)
		{
			const auto u = _byteswap_uint64(*(const uint64_t*)ptr);
			return ((U*)&u)->t;
		}

		__forceinline static void Write(void* ptr, const T value)
		{
			*(uint64_t*)ptr = _byteswap_uint64(((const U*)&value)->u);
			xenon::TagMemoryWrite(ptr, sizeof(T), "DataMemoryDirect");
		}
	};

	//----

	// provides means to access type in memory
	template<typename T>
	struct DataMemoryAccess : public DataMemoryDirect<T> {};

	// normal types
	template<> struct DataMemoryAccess<wchar_t> : public DataMemorySwap2<wchar_t> {};
	template<> struct DataMemoryAccess<uint8_t> : public DataMemoryDirect<uint8_t> {};
	template<> struct DataMemoryAccess<uint16_t> : public DataMemorySwap2<uint16_t> {};
	template<> struct DataMemoryAccess<uint32_t> : public DataMemorySwap4<uint32_t> {};
	template<> struct DataMemoryAccess<uint64_t> : public DataMemorySwap8<uint64_t> {};
	template<> struct DataMemoryAccess<int8_t> : public DataMemoryDirect<int8_t> {};
	template<> struct DataMemoryAccess<int16_t> : public DataMemorySwap2<int16_t> {};
	template<> struct DataMemoryAccess<int32_t> : public DataMemorySwap4<int32_t> {};
	template<> struct DataMemoryAccess<int64_t> : public DataMemorySwap8<int64_t> {};
	template<> struct DataMemoryAccess<float> : public DataMemorySwap4<float> {};
	template<> struct DataMemoryAccess<double> : public DataMemorySwap8<double> {};

	//----

	// memory address, can be casted to any pointer
	struct MemoryAddress
	{
	public:
		inline MemoryAddress()
			: m_address(0)
		{}

		template< typename T >
		inline MemoryAddress(T* ptr)
			: m_address(0)
		{
			// when initializing from normal pointer we MUST make sure it fits in 32-bits
			const uint64_t address = (uint64_t)ptr;
			DEBUG_CHECK(address <= 0xFFFFFFFF);
			m_address = (uint32_t)address;
		}

		inline MemoryAddress(const uint64_t addr)
			: m_address((uint32_t)addr)
		{}

		inline MemoryAddress(nullptr_t)
			: m_address(0)
		{}

		inline const bool IsValid() const
		{
			return m_address != 0;
		}

		inline const uint32 GetAddressValue() const
		{
			return m_address;
		}

		inline void* GetNativePointer() const
		{
			return (void*)(uint64_t)m_address;
		}

		inline ptrdiff_t operator-(const MemoryAddress& other) const
		{
			return (ptrdiff_t)m_address - (ptrdiff_t)other.m_address;
		}

		inline MemoryAddress operator+(const int64_t index) const
		{
			return MemoryAddress((uint32)(m_address + index));
		}

		inline MemoryAddress operator-(const int64_t index) const
		{
			return MemoryAddress((uint32)(m_address - index));
		}

		inline MemoryAddress& operator+=(const int64_t index)
		{
			m_address = (uint32)(m_address + index);
			return *this;
		}

		inline MemoryAddress& operator-=(const int64_t index)
		{
			m_address = (uint32)(m_address - index);
			return *this;
		}

		inline const bool operator==(const MemoryAddress& other) const
		{
			return m_address == other.m_address;
		}

		inline const bool operator!=(const MemoryAddress& other) const
		{
			return m_address != other.m_address;
		}

		inline const bool operator<(const MemoryAddress& other) const
		{
			return m_address < other.m_address;
		}

		inline const bool operator>(const MemoryAddress& other) const
		{
			return m_address > other.m_address;
		}

		inline const bool operator<=(const MemoryAddress& other) const
		{
			return m_address <= other.m_address;
		}

		inline const bool operator>=(const MemoryAddress& other) const
		{
			return m_address >= other.m_address;
		}

	private:
		uint32_t m_address;
	};


	// native value wrapper
	template<typename T>
	struct DataInMemory
	{
		inline DataInMemory()
			: m_address(0)
		{}

		inline DataInMemory(const MemoryAddress address)
			: m_address(address)
		{}

		inline const MemoryAddress GetAddress() const
		{
			return m_address;
		}

		inline T Get() const
		{
			return DataMemoryAccess<T>::Read(m_address.GetNativePointer());
		}

		inline operator T() const
		{
			DEBUG_CHECK(m_address.IsValid());
			return DataMemoryAccess<T>::Read(m_address.GetNativePointer());
		}

		inline DataInMemory<T>& operator=(const T& value)
		{
			DEBUG_CHECK(m_address.IsValid());
			DataMemoryAccess<T>::Write(m_address.GetNativePointer(), value);
			return *this;
		}

	private:
		MemoryAddress m_address;
	};

	// wrapper for memory pointer of given type
	template<typename T>
	struct Pointer
	{
		inline Pointer()
		{}

		inline Pointer(const MemoryAddress address)
			: m_address(address)
		{}

		inline Pointer(T* ptr)
			: m_address(ptr)
		{}

		inline Pointer(nullptr_t)
		{}

		inline bool IsValid() const
		{
			return m_address.IsValid();
		}		

		inline const MemoryAddress GetAddress() const
		{
			return m_address;
		}

		inline T* GetNativePointer() const
		{
			return (T*)m_address.GetNativePointer();
		}

		inline DataInMemory<T> operator*() const
		{
			return DataInMemory<T>(m_address);
		}

		inline const T* operator->() const
		{
			return GetNativePointer();
		}

		inline T* operator->()
		{
			return GetNativePointer();
		}

		inline Pointer<T> operator+(const int64_t index) const
		{
			return Pointer<T>(m_address + sizeof(T)*index);
		}

		inline Pointer<T> operator-(const int64_t index) const
		{
			return Pointer<T>(m_address - sizeof(T)*index);
		}

		inline Pointer<T>& operator+=(const int64_t index)
		{
			m_address = m_address + sizeof(T)*index;
			return *this;
		}

		inline Pointer<T>& operator-=(const int64_t index)
		{
			m_address = m_address - sizeof(T)*index;
			return *this;
		}

		inline Pointer<T>& operator++()
		{
			m_address += sizeof(T);
			return *this;
		}

		inline Pointer<T>& operator--()
		{
			m_address -= sizeof(T);
			return *this;
		}

		inline Pointer<T> operator++(int)
		{
			Pointer<T> ret(*this);
			m_address += sizeof(T);
			return ret;
		}

		inline Pointer<T> operator--(int)
		{
			Pointer<T> ret(*this);
			m_address -= sizeof(T);
			return ret;
		}

		inline ptrdiff_t operator-(const Pointer<T>& other) const
		{
			return m_address - other.m_address;
		}

		inline DataInMemory<T> operator[](const uint32_t index) const
		{
			return DataInMemory<T>(m_address + index * sizeof(T));
		}

		inline const bool operator==(const Pointer<T>& other) const
		{
			return m_address == other.m_address;
		}

		inline const bool operator!=(const Pointer<T>& other) const
		{
			return m_address != other.m_address;
		}

		inline const bool operator<(const Pointer<T>& other) const
		{
			return m_address < other.m_address;
		}

		inline const bool operator>(const Pointer<T>& other) const
		{
			return m_address > other.m_address;
		}

		inline const bool operator<=(const Pointer<T>& other) const
		{
			return m_address <= other.m_address;
		}

		inline const bool operator>=(const Pointer<T>& other) const
		{
			return m_address >= other.m_address;
		}

	private:
		MemoryAddress m_address;
	};

	template<typename T>
	struct DataMemoryAccess<Pointer<T>> : public DataMemorySwap4<Pointer<T>> {};

	//----

	/// type inverted data
	template<typename T>
	struct Field
	{
		inline Field()
		{}

		inline Field(const T& initData)
		{
			DataMemoryAccess<T>::Write(&m_rawData, initData);
		}

		inline operator T() const
		{
			return DataMemoryAccess<T>::Read(&m_rawData);
		}

		inline const uint32_t GetAddress() const
		{
			return (uint32_t)this;
		}

		inline DataInMemory<T> AsData() const
		{
			return DataInMemory<T>(GetAddress());
		}

		template< typename W >
		inline Field<T>& operator=(const W& other)
		{
			const T value(other);
			DataMemoryAccess<T>::Write(&m_rawData, value);
			return *this;
		}

		template< typename W >
		inline bool operator==(const W& other) const
		{
			return DataMemoryAccess<T>::Read(&m_rawData) == other;
		}

		template< typename W >
		inline bool operator!=(const W& other) const
		{
			return DataMemoryAccess<T>::Read(&m_rawData) != other;
		}

		inline const T& GetNativeData() const
		{
			return m_rawData;
		}

		inline T& GetNativeData()
		{
			return m_rawData;
		}

	private:
		T m_rawData;
	};

} // xenon
