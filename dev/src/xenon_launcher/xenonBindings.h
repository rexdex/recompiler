#pragma once

#include "xenonLib.h"
#include "../host_core/runtimeSymbols.h"

namespace xenon
{
	namespace lib
	{
		namespace binding
		{

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
			template<> struct FunctionArgumentTypeSelector<uint8_t> { static const FunctionArgumentType value = FunctionArgumentType::Generic; };
			template<> struct FunctionArgumentTypeSelector<uint16_t> { static const FunctionArgumentType value = FunctionArgumentType::Generic; };
			template<> struct FunctionArgumentTypeSelector<uint32_t> { static const FunctionArgumentType value = FunctionArgumentType::Generic; };
			template<> struct FunctionArgumentTypeSelector<uint64_t> { static const FunctionArgumentType value = FunctionArgumentType::Generic; };
			template<> struct FunctionArgumentTypeSelector<int8_t> { static const FunctionArgumentType value = FunctionArgumentType::Generic; };
			template<> struct FunctionArgumentTypeSelector<int16_t> { static const FunctionArgumentType value = FunctionArgumentType::Generic; };
			template<> struct FunctionArgumentTypeSelector<int32_t> { static const FunctionArgumentType value = FunctionArgumentType::Generic; };
			template<> struct FunctionArgumentTypeSelector<int64_t> { static const FunctionArgumentType value = FunctionArgumentType::Generic; };
			template<> struct FunctionArgumentTypeSelector<float> { static const FunctionArgumentType value = FunctionArgumentType::FPU; };
			template<> struct FunctionArgumentTypeSelector<double> { static const FunctionArgumentType value = FunctionArgumentType::FPU; };
			template<> struct FunctionArgumentTypeSelector<MemoryAddress> { static const FunctionArgumentType value = FunctionArgumentType::Generic; };
			template<> struct FunctionArgumentTypeSelector<char> { static const FunctionArgumentType value = FunctionArgumentType::Generic; };
			template<> struct FunctionArgumentTypeSelector<wchar_t> { static const FunctionArgumentType value = FunctionArgumentType::Generic; };

			// pointer
			template<typename T>
			struct FunctionArgumentTypeSelector<Pointer<T>> { static const FunctionArgumentType value = FunctionArgumentType::Generic; };
			template<typename T>
			struct FunctionArgumentTypeSelector<DataInMemory<T>> { static const FunctionArgumentType value = FunctionArgumentType::Generic; };

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
					const auto value = (&regs.R3)[index];
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
					const auto value = (&regs.FR0)[index];
					return T(value);
				}
			};

			// interface for logging system calls
			class IFunctionCallLogger
			{
			public:
				virtual ~IFunctionCallLogger();

				enum class DataType
				{
					Int8,
					Int16,
					Int32,
					Int64,
					Uint8,
					Uint16,
					Uint32,
					Uint64,
					Address,
					AnsiString,
					UniString,
					Pointer8,
					Pointer16,
					Pointer32,
					Pointer64,
					PointerGeneric,
					Float,
					Double,
					Generic,
				};

				virtual void OnBeginFunction(const char* name) = 0;
				virtual void OnArgument(const DataType type, const void* data, const uint32 size) = 0;
				virtual void OnReturnValue(const DataType type, const void* data, const uint32 size) = 0;
				virtual void OnEndFunction(const uint64 returnPoint) = 0;

				//--

				template< typename T > struct TypeResolver { static const auto value = DataType::Generic; };
				template<> struct TypeResolver<uint8_t> { static const auto value = DataType::Uint8; };
				template<> struct TypeResolver<uint16_t> { static const auto value = DataType::Uint16; };
				template<> struct TypeResolver<uint32_t> { static const auto value = DataType::Uint32; };
				template<> struct TypeResolver<uint64_t> { static const auto value = DataType::Uint64; };
				template<> struct TypeResolver<int8_t> { static const auto value = DataType::Int8; };
				template<> struct TypeResolver<int16_t> { static const auto value = DataType::Int16; };
				template<> struct TypeResolver<int32_t> { static const auto value = DataType::Int32; };
				template<> struct TypeResolver<int64_t> { static const auto value = DataType::Int64; };
				template<> struct TypeResolver<float> { static const auto value = DataType::Float; };
				template<> struct TypeResolver<double> { static const auto value = DataType::Double; };
				template<> struct TypeResolver<char> { static const auto value = DataType::Uint8; };
				template<> struct TypeResolver<wchar_t> { static const auto value = DataType::Uint16; };

				template<> struct TypeResolver<Pointer<uint8_t>> { static const auto value = DataType::Pointer8; };
				template<> struct TypeResolver<Pointer<uint16_t>> { static const auto value = DataType::Pointer16; };
				template<> struct TypeResolver<Pointer<uint32_t>> { static const auto value = DataType::Pointer32; };
				template<> struct TypeResolver<Pointer<uint64_t>> { static const auto value = DataType::Pointer64; };
				template<> struct TypeResolver<Pointer<char>> { static const auto value = DataType::AnsiString; };
				template<> struct TypeResolver<Pointer<wchar_t>> { static const auto value = DataType::UniString; };

				template<typename T>
				struct TypeResolver<Pointer<T>> { static const auto value = DataType::PointerGeneric; };

				template< typename T >
				inline void OnArgument(const T& data)
				{
					const auto type = TypeResolver<T>::value;
					return OnArgument(type, &data, sizeof(data));
				}

				template< typename T >
				inline void OnReturnValue(const T& data)
				{
					const auto type = TypeResolver<T>::value;
					return OnReturnValue(type, &data, sizeof(data));
				}
			};

			// function calling ABI
			// NOTE: this is the "calling convention" used by xenon that we implement in order to retrieve arguments
			class FunctionInterface
			{
			public:
				FunctionInterface(const uint64 ip, runtime::RegisterBank& regs, const char* name);

				// bind call logger
				static void BindFunctionLogger(IFunctionCallLogger* logger);

				// fetch argument from the stack
				template< typename T >
				__forceinline T GetArgument()
				{
					uint32_t index = 0;

					if (FunctionArgumentTypeSelector<T>::value == FunctionArgumentType::Generic)
						index = m_nextGenericArgument++;
					else if (FunctionArgumentTypeSelector<T>::value == FunctionArgumentType::FPU)
						index = m_nextGenericArgument++;
					
					// TODO: even on PowerPC the arguments eventually are being passed on stack, implement this

					if (st_Logger)
					{
						auto arg = FunctionArgumentFetcher<FunctionArgumentTypeSelector<T>::value>::Fetch<T>(m_regs, index);
						st_Logger->OnArgument(arg);
						return arg;
					}
					else
					{
						return FunctionArgumentFetcher<FunctionArgumentTypeSelector<T>::value>::Fetch<T>(m_regs, index);
					}
				}

				// place return value
				template< typename T >
				__forceinline void PlaceReturn(const T& val)
				{
					if (st_Logger)
						st_Logger->OnReturnValue(val);

					if (FunctionArgumentTypeSelector<T>::value == FunctionArgumentType::Generic)
					{
						DEBUG_CHECK(sizeof(val) <= 8);

						if (sizeof(T) == 1)
							m_regs.R3 = (uint8_t&)val;
						else if (sizeof(T) == 2)
							m_regs.R3 = (uint16_t&)val;
						else if (sizeof(T) == 4)
							m_regs.R3 = (uint32_t&)val;
						else if (sizeof(T) == 8)
							m_regs.R3 = (uint32_t&)val;
					}
					else if (FunctionArgumentTypeSelector<T>::value == FunctionArgumentType::FPU)
					{
						DEBUG_CHECK(sizeof(val) == 8 || sizeof(val) == 4);

						if (sizeof(T) == 4)
							m_regs.FR0 = (float&)val;
						else if (sizeof(T) == 8)
							m_regs.FR0 = (double&)val;
					}
				}

				//--

				// get current instruction pointer
				__forceinline const uint64 GetInstructionPointer() const
				{
					return m_ip;
				}

				// return from function
				__forceinline const uint64 GetReturnAddress() const
				{
					if (st_Logger)
						st_Logger->OnEndFunction((uint32)m_regs.LR);
					return (uint32)m_regs.LR;
				}

				//--

			private:
				cpu::CpuRegs& m_regs;
				const uint64 m_ip;
				uint8_t m_nextGenericArgument;
				uint8_t m_nextFloatingArgument;

				static IFunctionCallLogger* st_Logger;
			};

			// helper class that can build a function call proxy for static (global) functions
			// for compatibility the context parameter is left in the function but it's always NULL
			// code borrowed from Inferno Engine (tm) Script Binding
			struct StaticFunctionProxy
			{
				//---------------------------------------------------------------------
				//-- unimplemented function
				//---------------------------------------------------------------------

				static inline runtime::TSystemFunction CreateUnimplementedFunction(const char* name)
				{
					return [name](const uint64_t ip, runtime::RegisterBank& regs) -> uint64
					{
						GLog.Err("Runtime: Called unimplemented function '%hs'", name);
						return regs.ReturnFromFunction();
					};
				}

				//---------------------------------------------------------------------
				//-- old style proxy
				//---------------------------------------------------------------------

				typedef uint64(__fastcall *TNativeBlockFunc)(const uint64_t ip, cpu::CpuRegs& regs);

				static inline runtime::TSystemFunction CreateProxy(const char* name, TNativeBlockFunc func)
				{
					return [func, name](const uint64_t ip, runtime::RegisterBank& regs) -> uint64
					{
						return (*func)(ip, (cpu::CpuRegs&)regs);
					};
				}

				//---------------------------------------------------------------------
				//-- 0 arguments, no return value
				//---------------------------------------------------------------------

				static inline runtime::TSystemFunction CreateProxy(const char* name, void(*func)())
				{
					return [func, name](const uint64_t ip, runtime::RegisterBank& regs) -> uint64
					{
						FunctionInterface cc(ip, regs, name);
						(*func)();
						return cc.GetReturnAddress();
					};
				}

				//---------------------------------------------------------------------
				//-- 1 argument, no return value
				//---------------------------------------------------------------------

				template< typename F1 >
				static inline runtime::TSystemFunction CreateProxy(const char* name, void(*func)(F1))
				{
					typedef std::remove_cv<std::remove_reference<F1>::type>::type SafeF1;

					return [func, name](const uint64_t ip, runtime::RegisterBank& regs) -> uint64
					{
						FunctionInterface cc(ip, regs, name);
						SafeF1 f1 = cc.GetArgument<F1>();
						((*func)(f1));
						return cc.GetReturnAddress();
					};
				}

				//---------------------------------------------------------------------
				//-- 2 arguments, no return value
				//---------------------------------------------------------------------

				template< typename F1, typename F2 >
				static inline runtime::TSystemFunction CreateProxy(const char* name, void(*func)(F1, F2))
				{
					typedef std::remove_cv<std::remove_reference<F1>::type>::type SafeF1;
					typedef std::remove_cv<std::remove_reference<F2>::type>::type SafeF2;

					return [func, name](const uint64_t ip, runtime::RegisterBank& regs) -> uint64
					{
						FunctionInterface cc(ip, regs, name);
						SafeF1 f1 = cc.GetArgument<F1>();
						SafeF2 f2 = cc.GetArgument<F2>();
						((*func)(f1, f2));
						return cc.GetReturnAddress();
					};
				}

				//---------------------------------------------------------------------
				//-- 3 arguments, no return value
				//---------------------------------------------------------------------

				template< typename F1, typename F2, typename F3>
				static inline runtime::TSystemFunction CreateProxy(const char* name, void(*func)(F1, F2, F3))
				{
					typedef std::remove_cv<std::remove_reference<F1>::type>::type SafeF1;
					typedef std::remove_cv<std::remove_reference<F2>::type>::type SafeF2;
					typedef std::remove_cv<std::remove_reference<F3>::type>::type SafeF3;

					return [func, name](const uint64_t ip, runtime::RegisterBank& regs) -> uint64
					{
						FunctionInterface cc(ip, regs, name);
						SafeF1 f1 = cc.GetArgument<F1>();
						SafeF2 f2 = cc.GetArgument<F2>();
						SafeF3 f3 = cc.GetArgument<F3>();
						((*func)(f1, f2, f3));
						return cc.GetReturnAddress();
					};
				}

				//---------------------------------------------------------------------
				//-- 4 arguments, no return value
				//---------------------------------------------------------------------

				template< typename F1, typename F2, typename F3, typename F4>
				static inline runtime::TSystemFunction CreateProxy(const char* name, void(*func)(F1, F2, F3, F4))
				{
					typedef std::remove_cv<std::remove_reference<F1>::type>::type SafeF1;
					typedef std::remove_cv<std::remove_reference<F2>::type>::type SafeF2;
					typedef std::remove_cv<std::remove_reference<F3>::type>::type SafeF3;
					typedef std::remove_cv<std::remove_reference<F4>::type>::type SafeF4;

					return [func, name](const uint64_t ip, runtime::RegisterBank& regs) -> uint64
					{
						FunctionInterface cc(ip, regs, name);
						SafeF1 f1 = cc.GetArgument<F1>();
						SafeF2 f2 = cc.GetArgument<F2>();
						SafeF3 f3 = cc.GetArgument<F3>();
						SafeF4 f4 = cc.GetArgument<F4>();
						((*func)(f1, f2, f3, f4));
						return cc.GetReturnAddress();
					};
				}

				//---------------------------------------------------------------------
				//-- 5 arguments, no return value
				//---------------------------------------------------------------------

				template< typename F1, typename F2, typename F3, typename F4, typename F5>
				static inline runtime::TSystemFunction CreateProxy(const char* name, void(*func)(F1, F2, F3, F4, F5))
				{
					typedef std::remove_cv<std::remove_reference<F1>::type>::type SafeF1;
					typedef std::remove_cv<std::remove_reference<F2>::type>::type SafeF2;
					typedef std::remove_cv<std::remove_reference<F3>::type>::type SafeF3;
					typedef std::remove_cv<std::remove_reference<F4>::type>::type SafeF4;
					typedef std::remove_cv<std::remove_reference<F5>::type>::type SafeF5;

					return [func, name](const uint64_t ip, runtime::RegisterBank& regs) -> uint64
					{
						FunctionInterface cc(ip, regs, name);
						SafeF1 f1 = cc.GetArgument<F1>();
						SafeF2 f2 = cc.GetArgument<F2>();
						SafeF3 f3 = cc.GetArgument<F3>();
						SafeF4 f4 = cc.GetArgument<F4>();
						SafeF5 f5 = cc.GetArgument<F5>();
						((*func)(f1, f2, f3, f4, f5));
						return cc.GetReturnAddress();
					};
				}

				//---------------------------------------------------------------------
				//-- 6 arguments, no return value
				//---------------------------------------------------------------------

				template< typename F1, typename F2, typename F3, typename F4, typename F5, typename F6>
				static inline runtime::TSystemFunction CreateProxy(const char* name, void(*func)(F1, F2, F3, F4, F5, F6))
				{
					typedef std::remove_cv<std::remove_reference<F1>::type>::type SafeF1;
					typedef std::remove_cv<std::remove_reference<F2>::type>::type SafeF2;
					typedef std::remove_cv<std::remove_reference<F3>::type>::type SafeF3;
					typedef std::remove_cv<std::remove_reference<F4>::type>::type SafeF4;
					typedef std::remove_cv<std::remove_reference<F5>::type>::type SafeF5;
					typedef std::remove_cv<std::remove_reference<F6>::type>::type SafeF6;

					return [func, name](const uint64_t ip, runtime::RegisterBank& regs) -> uint64
					{
						FunctionInterface cc(ip, regs, name);
						SafeF1 f1 = cc.GetArgument<F1>();
						SafeF2 f2 = cc.GetArgument<F2>();
						SafeF3 f3 = cc.GetArgument<F3>();
						SafeF4 f4 = cc.GetArgument<F4>();
						SafeF5 f5 = cc.GetArgument<F5>();
						SafeF6 f6 = cc.GetArgument<F6>();
						((*func)(f1, f2, f3, f4, f5, f6));
						return cc.GetReturnAddress();
					};
				}

				//---------------------------------------------------------------------
				//-- 7 arguments, no return value
				//---------------------------------------------------------------------

				template< typename F1, typename F2, typename F3, typename F4, typename F5, typename F6, typename F7>
				static inline runtime::TSystemFunction CreateProxy(const char* name, void(*func)(F1, F2, F3, F4, F5, F6, F7))
				{
					typedef std::remove_cv<std::remove_reference<F1>::type>::type SafeF1;
					typedef std::remove_cv<std::remove_reference<F2>::type>::type SafeF2;
					typedef std::remove_cv<std::remove_reference<F3>::type>::type SafeF3;
					typedef std::remove_cv<std::remove_reference<F4>::type>::type SafeF4;
					typedef std::remove_cv<std::remove_reference<F5>::type>::type SafeF5;
					typedef std::remove_cv<std::remove_reference<F6>::type>::type SafeF6;
					typedef std::remove_cv<std::remove_reference<F7>::type>::type SafeF7;

					return [func, name](const uint64_t ip, runtime::RegisterBank& regs) -> uint64
					{
						FunctionInterface cc(ip, regs, name);
						SafeF1 f1 = cc.GetArgument<F1>();
						SafeF2 f2 = cc.GetArgument<F2>();
						SafeF3 f3 = cc.GetArgument<F3>();
						SafeF4 f4 = cc.GetArgument<F4>();
						SafeF5 f5 = cc.GetArgument<F5>();
						SafeF6 f6 = cc.GetArgument<F6>();
						SafeF7 f7 = cc.GetArgument<F7>();
						((*func)(f1, f2, f3, f4, f5, f6, f7));
						return cc.GetReturnAddress();
					};
				}

				//---------------------------------------------------------------------
				//-- 8 arguments, no return value
				//---------------------------------------------------------------------

				template< typename F1, typename F2, typename F3, typename F4, typename F5, typename F6, typename F7, typename F8 >
				static inline runtime::TSystemFunction CreateProxy(const char* name, void(*func)(F1, F2, F3, F4, F5, F6, F7, F8))
				{
					typedef std::remove_cv<std::remove_reference<F1>::type>::type SafeF1;
					typedef std::remove_cv<std::remove_reference<F2>::type>::type SafeF2;
					typedef std::remove_cv<std::remove_reference<F3>::type>::type SafeF3;
					typedef std::remove_cv<std::remove_reference<F4>::type>::type SafeF4;
					typedef std::remove_cv<std::remove_reference<F5>::type>::type SafeF5;
					typedef std::remove_cv<std::remove_reference<F6>::type>::type SafeF6;
					typedef std::remove_cv<std::remove_reference<F7>::type>::type SafeF7;
					typedef std::remove_cv<std::remove_reference<F8>::type>::type SafeF8;

					return [func, name](const uint64_t ip, runtime::RegisterBank& regs) -> uint64
					{
						FunctionInterface cc(ip, regs, name);
						SafeF1 f1 = cc.GetArgument<F1>();
						SafeF2 f2 = cc.GetArgument<F2>();
						SafeF3 f3 = cc.GetArgument<F3>();
						SafeF4 f4 = cc.GetArgument<F4>();
						SafeF5 f5 = cc.GetArgument<F5>();
						SafeF6 f6 = cc.GetArgument<F6>();
						SafeF7 f7 = cc.GetArgument<F7>();
						SafeF8 f8 = cc.GetArgument<F8>();
						((*func)(f1, f2, f3, f4, f5, f6, f7, f8));
						return cc.GetReturnAddress();
					};
				}

				//---------------------------------------------------------------------
				//-- 0 arguments
				//---------------------------------------------------------------------

				template< typename Ret >
				static inline runtime::TSystemFunction CreateProxy(const char* name, Ret(*func)())
				{
					return [func, name](const uint64_t ip, runtime::RegisterBank& regs) -> uint64
					{
						FunctionInterface cc(ip, regs, name);
						cc.PlaceReturn<Ret>((*func)());
						return cc.GetReturnAddress();
					};
				}

				//---------------------------------------------------------------------
				//-- 1 argument
				//---------------------------------------------------------------------

				template< typename Ret, typename F1 >
				static inline runtime::TSystemFunction CreateProxy(const char* name, Ret(*func)(F1))
				{
					typedef std::remove_cv<std::remove_reference<F1>::type>::type SafeF1;

					return [func, name](const uint64_t ip, runtime::RegisterBank& regs) -> uint64
					{
						FunctionInterface cc(ip, regs, name);
						SafeF1 f1 = cc.GetArgument<F1>();
						cc.PlaceReturn<Ret>((*func)(f1));
						return cc.GetReturnAddress();
					};
				}

				//---------------------------------------------------------------------
				//-- 2 arguments
				//---------------------------------------------------------------------

				template< typename Ret, typename F1, typename F2 >
				static inline runtime::TSystemFunction CreateProxy(const char* name, Ret(*func)(F1, F2))
				{
					typedef std::remove_cv<std::remove_reference<F1>::type>::type SafeF1;
					typedef std::remove_cv<std::remove_reference<F2>::type>::type SafeF2;

					return [func, name](const uint64_t ip, runtime::RegisterBank& regs) -> uint64
					{
						FunctionInterface cc(ip, regs, name);
						SafeF1 f1 = cc.GetArgument<F1>();
						SafeF2 f2 = cc.GetArgument<F2>();
						cc.PlaceReturn<Ret>((*func)(f1, f2));
						return cc.GetReturnAddress();
					};
				}

				//---------------------------------------------------------------------
				//-- 3 arguments
				//---------------------------------------------------------------------

				template< typename Ret, typename F1, typename F2, typename F3>
				static inline runtime::TSystemFunction CreateProxy(const char* name, Ret(*func)(F1, F2, F3))
				{
					typedef std::remove_cv<std::remove_reference<F1>::type>::type SafeF1;
					typedef std::remove_cv<std::remove_reference<F2>::type>::type SafeF2;
					typedef std::remove_cv<std::remove_reference<F3>::type>::type SafeF3;

					return [func, name](const uint64_t ip, runtime::RegisterBank& regs) -> uint64
					{
						FunctionInterface cc(ip, regs, name);
						SafeF1 f1 = cc.GetArgument<F1>();
						SafeF2 f2 = cc.GetArgument<F2>();
						SafeF3 f3 = cc.GetArgument<F3>();
						cc.PlaceReturn<Ret>((*func)(f1, f2, f3));
						return cc.GetReturnAddress();
					};
				}

				//---------------------------------------------------------------------
				//-- 4 arguments
				//---------------------------------------------------------------------

				template< typename Ret, typename F1, typename F2, typename F3, typename F4>
				static inline runtime::TSystemFunction CreateProxy(const char* name, Ret(*func)(F1, F2, F3, F4))
				{
					typedef std::remove_cv<std::remove_reference<F1>::type>::type SafeF1;
					typedef std::remove_cv<std::remove_reference<F2>::type>::type SafeF2;
					typedef std::remove_cv<std::remove_reference<F3>::type>::type SafeF3;
					typedef std::remove_cv<std::remove_reference<F4>::type>::type SafeF4;

					return [func, name](const uint64_t ip, runtime::RegisterBank& regs) -> uint64
					{
						FunctionInterface cc(ip, regs, name);
						SafeF1 f1 = cc.GetArgument<F1>();
						SafeF2 f2 = cc.GetArgument<F2>();
						SafeF3 f3 = cc.GetArgument<F3>();
						SafeF4 f4 = cc.GetArgument<F4>();
						cc.PlaceReturn<Ret>((*func)(f1, f2, f3, f4));
						return cc.GetReturnAddress();
					};
				}

				//---------------------------------------------------------------------
				//-- 5 arguments
				//---------------------------------------------------------------------

				template< typename Ret, typename F1, typename F2, typename F3, typename F4, typename F5>
				static inline runtime::TSystemFunction CreateProxy(const char* name, Ret(*func)(F1, F2, F3, F4, F5))
				{
					typedef std::remove_cv<std::remove_reference<F1>::type>::type SafeF1;
					typedef std::remove_cv<std::remove_reference<F2>::type>::type SafeF2;
					typedef std::remove_cv<std::remove_reference<F3>::type>::type SafeF3;
					typedef std::remove_cv<std::remove_reference<F4>::type>::type SafeF4;
					typedef std::remove_cv<std::remove_reference<F5>::type>::type SafeF5;

					return [func, name](const uint64_t ip, runtime::RegisterBank& regs) -> uint64
					{
						FunctionInterface cc(ip, regs, name);
						SafeF1 f1 = cc.GetArgument<F1>();
						SafeF2 f2 = cc.GetArgument<F2>();
						SafeF3 f3 = cc.GetArgument<F3>();
						SafeF4 f4 = cc.GetArgument<F4>();
						SafeF5 f5 = cc.GetArgument<F5>();
						cc.PlaceReturn<Ret>((*func)(f1, f2, f3, f4, f5));
						return cc.GetReturnAddress();
					};
				}

				//---------------------------------------------------------------------
				//-- 6 arguments
				//---------------------------------------------------------------------

				template< typename Ret, typename F1, typename F2, typename F3, typename F4, typename F5, typename F6>
				static inline runtime::TSystemFunction CreateProxy(const char* name, Ret(*func)(F1, F2, F3, F4, F5, F6))
				{
					typedef std::remove_cv<std::remove_reference<F1>::type>::type SafeF1;
					typedef std::remove_cv<std::remove_reference<F2>::type>::type SafeF2;
					typedef std::remove_cv<std::remove_reference<F3>::type>::type SafeF3;
					typedef std::remove_cv<std::remove_reference<F4>::type>::type SafeF4;
					typedef std::remove_cv<std::remove_reference<F5>::type>::type SafeF5;
					typedef std::remove_cv<std::remove_reference<F6>::type>::type SafeF6;

					return [func, name](const uint64_t ip, runtime::RegisterBank& regs) -> uint64
					{
						FunctionInterface cc(ip, regs, name);
						SafeF1 f1 = cc.GetArgument<F1>();
						SafeF2 f2 = cc.GetArgument<F2>();
						SafeF3 f3 = cc.GetArgument<F3>();
						SafeF4 f4 = cc.GetArgument<F4>();
						SafeF5 f5 = cc.GetArgument<F5>();
						SafeF6 f6 = cc.GetArgument<F6>();
						cc.PlaceReturn<Ret>((*func)(f1, f2, f3, f4, f5, f6));
						return cc.GetReturnAddress();
					};
				}

				//---------------------------------------------------------------------
				//-- 7 arguments
				//---------------------------------------------------------------------

				template< typename Ret, typename F1, typename F2, typename F3, typename F4, typename F5, typename F6, typename F7>
				static inline runtime::TSystemFunction CreateProxy(const char* name, Ret(*func)(F1, F2, F3, F4, F5, F6, F7))
				{
					typedef std::remove_cv<std::remove_reference<F1>::type>::type SafeF1;
					typedef std::remove_cv<std::remove_reference<F2>::type>::type SafeF2;
					typedef std::remove_cv<std::remove_reference<F3>::type>::type SafeF3;
					typedef std::remove_cv<std::remove_reference<F4>::type>::type SafeF4;
					typedef std::remove_cv<std::remove_reference<F5>::type>::type SafeF5;
					typedef std::remove_cv<std::remove_reference<F6>::type>::type SafeF6;
					typedef std::remove_cv<std::remove_reference<F7>::type>::type SafeF7;

					return [func, name](const uint64_t ip, runtime::RegisterBank& regs) -> uint64
					{
						FunctionInterface cc(ip, regs, name);
						SafeF1 f1 = cc.GetArgument<F1>();
						SafeF2 f2 = cc.GetArgument<F2>();
						SafeF3 f3 = cc.GetArgument<F3>();
						SafeF4 f4 = cc.GetArgument<F4>();
						SafeF5 f5 = cc.GetArgument<F5>();
						SafeF6 f6 = cc.GetArgument<F6>();
						SafeF7 f7 = cc.GetArgument<F7>();
						cc.PlaceReturn<Ret>((*func)(f1, f2, f3, f4, f5, f6, f7));
						return cc.GetReturnAddress();
					};
				}

				//---------------------------------------------------------------------
				//-- 8 arguments
				//---------------------------------------------------------------------

				template< typename Ret, typename F1, typename F2, typename F3, typename F4, typename F5, typename F6, typename F7, typename F8 >
				static inline runtime::TSystemFunction CreateProxy(const char* name, Ret(*func)(F1, F2, F3, F4, F5, F6, F7, F8))
				{
					typedef std::remove_cv<std::remove_reference<F1>::type>::type SafeF1;
					typedef std::remove_cv<std::remove_reference<F2>::type>::type SafeF2;
					typedef std::remove_cv<std::remove_reference<F3>::type>::type SafeF3;
					typedef std::remove_cv<std::remove_reference<F4>::type>::type SafeF4;
					typedef std::remove_cv<std::remove_reference<F5>::type>::type SafeF5;
					typedef std::remove_cv<std::remove_reference<F6>::type>::type SafeF6;
					typedef std::remove_cv<std::remove_reference<F7>::type>::type SafeF7;
					typedef std::remove_cv<std::remove_reference<F8>::type>::type SafeF8;

					return [func, name](const uint64_t ip, runtime::RegisterBank& regs) -> uint64
					{
						FunctionInterface cc(ip, regs, name);
						SafeF1 f1 = cc.GetArgument<F1>();
						SafeF2 f2 = cc.GetArgument<F2>();
						SafeF3 f3 = cc.GetArgument<F3>();
						SafeF4 f4 = cc.GetArgument<F4>();
						SafeF5 f5 = cc.GetArgument<F5>();
						SafeF6 f6 = cc.GetArgument<F6>();
						SafeF7 f7 = cc.GetArgument<F7>();
						SafeF8 f8 = cc.GetArgument<F8>();
						cc.PlaceReturn<Ret>((*func)(f1, f2, f3, f4, f5, f6, f7, f8));
						return cc.GetReturnAddress();
					};
				}
			};

			//---

			// text based logger
			class ITextFunctionTraceLogger : public IFunctionCallLogger
			{
			public:
				ITextFunctionTraceLogger();

				// print text to buffer
				virtual void Print(const char* mainInfo, const char* arguments) = 0;

			private:
				struct FunctionInfo
				{
					uint32_t m_thead;
					uint64_t m_callStart;
					uint64_t m_callDuration;
					const char* m_name;
					char m_text[2048];
					uint16_t m_numArgs;
					char* m_textWrite;
					bool m_return;

					void Appendf(const char* txt, ...);
				};

				static __declspec(thread) FunctionInfo* st_functionInfos;
				static uint64_t st_timeBaseLine;

				void PrintData(FunctionInfo* ptr, const DataType type, const void* data, const uint32 size);

				virtual void OnBeginFunction(const char* name) override;
				virtual void OnArgument(const DataType type, const void* data, const uint32 size) override;
				virtual void OnReturnValue(const DataType type, const void* data, const uint32 size) override;
				virtual void OnEndFunction(const uint64 returnPoint) override;
			};

			//---

			// screen based log
			class StdOutFunctionCallLog : public lib::binding::ITextFunctionTraceLogger
			{
			public:
				virtual void Print(const char* mainInfo, const char* arguments) override final;
			};

			//---

			// file based log
			class FileFunctionCallLog : public lib::binding::ITextFunctionTraceLogger
			{
			public:
				FileFunctionCallLog(const std::wstring& filePath);
				virtual void Print(const char* mainInfo, const char* arguments) override final;

			private:
				std::fstream m_file;
				std::mutex m_lock;
			};

			//---


		} // binding
	} // lib
} // xenon

#define REGISTER(x) symbols.RegisterFunction(#x, xenon::lib::binding::StaticFunctionProxy::CreateProxy(#x, &Xbox_##x));
#define NOT_IMPLEMENTED(x) symbols.RegisterFunction(#x, xenon::lib::binding::StaticFunctionProxy::CreateUnimplementedFunction(#x));