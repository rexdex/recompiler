#pragma once

#define RETURN() return (uint32)regs.LR; // return from native function
#define RETURN_ARG(arg) { regs.R3 = arg; return (uint32)regs.LR; } 
#define RETURN_DEFAULT() { GLog.Warn( "Visited empty function '%s'", __FUNCTION__ ); regs.R3 = 0; return (uint32)regs.LR; } 

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

	// native value wrapper
	template<typename T>
	struct DataInMemory
	{
		inline DataInMemory()
			: m_address(0)
		{}

		inline DataInMemory(const uint32 address)
			: m_address(address)
		{}

		inline const uint32 GetAddress() const
		{
			return m_address;
		}

		inline T Get() const
		{
			return DataMemoryAccess<T>::Read((void*)m_address);
		}

		inline operator T() const
		{
			DEBUG_CHECK(m_address != 0);
			return DataMemoryAccess<T>::Read((void*)m_address);
		}

		inline DataInMemory<T>& operator=(const T& value)
		{
			DEBUG_CHECK(m_address != 0);
			DataMemoryAccess<T>::Write((void*)m_address, value);
			return *this;
		}

	private:
		uint32 m_address;
	};

	// wrapper for memory pointer of given type
	template<typename T>
	struct Pointer
	{
		inline Pointer()
			: m_address(0)
		{}

		inline Pointer(const uint64_t address)
			: m_address((uint32_t)address)
		{}

		inline Pointer(nullptr_t)
			: m_address(0)
		{}

		inline bool IsValid() const
		{
			return m_address != 0;
		}		

		inline const uint32 GetAddress() const
		{
			return m_address;
		}

		inline T* GetNativePointer() const
		{
			return (T*)GetAddress();
		}

		inline DataInMemory<T> operator*() const
		{
			return DataInMemory<T>(m_address);
		}

		inline const T* operator->() const
		{
			return (const T*)GetAddress();
		}

		inline T* operator->()
		{
			return (T*)GetAddress();
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
			return (ptrdiff_t)m_address - (ptrdiff_t)other.m_address;
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
		uint32 m_address;
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

	//----

	enum class FunctionArgumentType
	{
		Generic,
		FPU,
	};

	template<typename T>
	struct FunctionArgumentTypeSelector
	{};

	// type selector for different basic types
	template<> struct FunctionArgumentTypeSelector<uint8_t> { const FunctionArgumentType value = FunctionArgumentType::Generic; };
	template<> struct FunctionArgumentTypeSelector<uint16_t> { const FunctionArgumentType value = FunctionArgumentType::Generic; };
	template<> struct FunctionArgumentTypeSelector<uint32_t> { const FunctionArgumentType value = FunctionArgumentType::Generic; };
	template<> struct FunctionArgumentTypeSelector<uint64_t> { const FunctionArgumentType value = FunctionArgumentType::Generic; };
	template<> struct FunctionArgumentTypeSelector<int8_t> { const FunctionArgumentType value = FunctionArgumentType::Generic; };
	template<> struct FunctionArgumentTypeSelector<int16_t> { const FunctionArgumentType value = FunctionArgumentType::Generic; };
	template<> struct FunctionArgumentTypeSelector<int32_t> { const FunctionArgumentType value = FunctionArgumentType::Generic; };
	template<> struct FunctionArgumentTypeSelector<int64_t> { const FunctionArgumentType value = FunctionArgumentType::Generic; };
	template<> struct FunctionArgumentTypeSelector<float> { const FunctionArgumentType value = FunctionArgumentType::FPU; };
	template<> struct FunctionArgumentTypeSelector<double> { const FunctionArgumentType value = FunctionArgumentType::FPU; };

	// pointer
	template<typename T>
	struct FunctionArgumentTypeSelector<Pointer<T>> { const FunctionArgumentType value = FunctionArgumentType::Generic; };
	template<typename T>
	struct FunctionArgumentTypeSelector<DataInMemory<T>> { const FunctionArgumentType value = FunctionArgumentType::Generic; };

	// argument fetcher - non implemented version
	template<FunctionArgumentType T>
	class FunctionArgumentFetcher
	{
	public:
		template< typename T >
		static inline T Fetch(const cpu::CpuRegs& regs, const uint32_t index)
		{
			DEBUG_CHECK(!"Invalid argument type");
			return T(0);
		}
	};

	// argument fetcher - non implemented version
	template<>
	class FunctionArgumentFetcher<FunctionArgumentType::Generic>
	{
	public:
		template< typename T >
		static inline T Fetch(const cpu::CpuRegs& regs, const uint32_t index)
		{
			const auto value = &regs.R3[index];
			DEBUG_CHECK(!"Invalid argument type");
			return T(value);
		}
	};

	// argument fetcher - non implemented version
	template<>
	class FunctionArgumentFetcher<FunctionArgumentType::FPU>
	{
	public:
		template< typename T >
		static inline T Fetch(const cpu::CpuRegs& regs, const uint32_t index)
		{
			const auto value = &regs.FR0[index];
			DEBUG_CHECK(!"Invalid argument type");
			return T(value);
		}
	};

	// function ABI
	class FunctionInterface
	{
	public:
		FunctionInterface(const uint64 ip, const cpu::CpuRegs& regs);

		// fetch argument from the stack
		template< typename T >
		inline T GetArgument(const uint32_t index) const
		{
			return FunctionArgumentFetcher<FunctionArgumentTypeSelector<T>::value>::Fetch<T>(m_regs, index);
		}

		// get current instruction pointer
		inline const uint64 GetInstructionPointer() const
		{
			return m_ip;
		}

		// return from function

	private:
		const cpu::CpuRegs& m_regs;
		const uint64 m_ip;
	};

} // xenon

#define REGISTER(x) symbols.RegisterFunction(#x, (runtime::TBlockFunc) &Xbox_##x);
