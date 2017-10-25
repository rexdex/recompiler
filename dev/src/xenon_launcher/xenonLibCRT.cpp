#include "build.h"
#include "xenonLib.h"  
#include "xenonLibNatives.h" 
#include "xenonBindings.h"
#include "xenonMemory.h"

namespace xenon
{
	namespace lib
	{

		namespace sprintf_helper
		{
			template<typename T, typename W>
			static inline void Append(T& stream, T streamEnd, W value)
			{
				*stream = (char)value;

				if (streamEnd != nullptr)
					stream = (stream < streamEnd) ? stream + 1 : stream;
			}

			template<typename T, typename W>
			static inline void AppendString(T& stream, T streamEnd, W value)
			{
				while (*value)
				{
					Append(stream, streamEnd, *value);
					++value;
				}
			}

			template<typename T>
			static inline void AppendHex(T& stream, T streamEnd, const uint64 value)
			{
				char buf[64];
				sprintf_s(buf, "%llX", value);
				AppendString(stream, streamEnd, buf);
			}

			template<typename T>
			static inline void AppendSigned(T& stream, T streamEnd, const int64_t value)
			{
				char buf[64];
				sprintf_s(buf, "%lld", value);
				AppendString(stream, streamEnd, buf);
			}

			template<typename T>
			static inline void AppendUnsigned(T& stream, T streamEnd, const uint64_t value)
			{
				char buf[64];
				sprintf_s(buf, "%llu", value);
				AppendString(stream, streamEnd, buf);
			}

			template<typename WritePtr, typename StringPtr>
			uint64_t FormatString(WritePtr write, WritePtr writeEnd, StringPtr formatStr, binding::FunctionInterface& cc)
			{
				auto cur = formatStr;
				auto writeStart = write;
				const auto end = formatStr + strlen(formatStr.GetNativePointer());
				while (cur < end)
				{
					const char ch = *cur++;

					if (ch == '%')
					{
						// skip the numbers (for now)
						while (*cur >= '0' && *cur <= '9' || *cur == '.')
							++cur;

						// the 'l' modified
						bool hasLModfier = false;
						while (*cur == 'l')
						{
							hasLModfier++;
							++cur;
						}

						// get the code
						const char code = *cur++;
						switch (code)
						{
							// just %
							case '%':
							{
								Append(write, writeEnd, (char)'%');
								break;
							}

							// hex
							case 'x':
							case 'X':
							{
								const auto val = cc.GetArgument<uint64_t>();
								AppendHex(write, writeEnd, val);
								break;
							}

							// unsigned
							case 'u':
							{
								const auto val = hasLModfier ? cc.GetArgument<uint64_t>() : cc.GetArgument<uint32_t>();
								AppendUnsigned(write, writeEnd, val);
								break;
							}

							// signed
							case 'i':
							case 'd':
							{
								const auto val = hasLModfier ? cc.GetArgument<int32_t>() : cc.GetArgument<int64_t>();
								AppendSigned(write, writeEnd, val);
								break;
							}

							// string
							case 's':
							{
								if (hasLModfier)
								{
									const auto str = cc.GetArgument<Pointer<wchar_t>>();
									AppendString(write, writeEnd, str);
								}
								else
								{
									const auto str = cc.GetArgument<Pointer<char>>();
									AppendString(write, writeEnd, str);
								}
								break;
							}

							// floating point value
							case 'f':
							{
								const uint64 val = cc.GetArgument<uint64_t>();
								AppendHex(write, writeEnd, val);
								break;
							}
						}
					}
					else
					{
						Append(write, writeEnd, ch);
					}
				}

				// write the end char
				Append(write, writeEnd, 0);

				// return number of characters written
				const auto numWritten = write - writeStart;
				return numWritten;

			}

		} // sprintf_helper

		uint64 __fastcall Xbox__vsnprintf(uint64 ip, cpu::CpuRegs& regs)
		{
			binding::FunctionInterface cc(ip, regs);

			const auto targetBuf = cc.GetArgument<Pointer<char>>();
			const auto targetBufSize = cc.GetArgument<uint32>();
			const auto formatStr = cc.GetArgument<Pointer<char>>();

			const auto ret = sprintf_helper::FormatString(targetBuf, targetBuf + targetBufSize, formatStr, cc);
			cc.PlaceReturn(ret);
			return cc.GetReturnAddress();
		}

		uint64 __fastcall Xbox__snprintf(uint64 ip, cpu::CpuRegs& regs)
		{
			binding::FunctionInterface cc(ip, regs);

			const auto targetBuf = cc.GetArgument<Pointer<char>>();
			const auto targetBufSize = cc.GetArgument<uint32>();
			const auto formatStr = cc.GetArgument<Pointer<char>>();

			const auto ret = sprintf_helper::FormatString(targetBuf, targetBuf + targetBufSize, formatStr, cc);
			cc.PlaceReturn(ret);
			return cc.GetReturnAddress();
		}

		uint64 __fastcall Xbox_sprintf(uint64 ip, cpu::CpuRegs& regs)
		{
			binding::FunctionInterface cc(ip, regs);

			const auto targetBuf = cc.GetArgument<Pointer<char>>();
			const auto formatStr = cc.GetArgument<Pointer<char>>();

			const auto ret = sprintf_helper::FormatString(targetBuf, Pointer<char>(nullptr), formatStr, cc);
			cc.PlaceReturn(ret);
			return cc.GetReturnAddress();
		}
		
		uint64 __fastcall Xbox_DbgPrint(uint64 ip, cpu::CpuRegs& regs)
		{
			binding::FunctionInterface cc(ip, regs);

			const auto formatStr = cc.GetArgument<Pointer<char>>();

			char targetBuf[1024];

			const auto ret = sprintf_helper::FormatString(targetBuf, targetBuf + sizeof(targetBuf), formatStr, cc);
			GLog.Log("DbgPrint: %s", targetBuf);

			cc.PlaceReturn(0);
			return cc.GetReturnAddress();
		}

		//-----------------------------------------

		X_STATUS Xbox_RtlUnicodeToMultiByteN(Pointer<char> targetBuf, uint32 targetLength, Pointer<uint32> writtenPtr, Pointer<wchar_t> sourcePtr, uint32 sourceLengthBytes)
		{
			auto copyLength = sourceLengthBytes >> 1;
			copyLength = (copyLength < targetLength) ? copyLength : targetLength;

			for (uint32 i = 0; i < copyLength; i++)
			{
				auto c = sourcePtr[i];
				targetBuf[i] = c < 255 ? (uint8)c : '?';
			}

			targetBuf[copyLength] = 0;

			if (writtenPtr.IsValid())
				*writtenPtr = copyLength;

			return X_STATUS_SUCCESS;
		}

		X_STATUS Xbox_RtlMultiByteToUnicodeN(Pointer<wchar_t> targetBuf, uint32 targetLength, Pointer<uint32> writtenPtr, Pointer<char> sourcePtr, uint32 sourceLengthBytes)
		{
			auto copyLength = sourceLengthBytes;
			copyLength = (copyLength < targetLength) ? copyLength : targetLength;

			for (uint32 i = 0; i < copyLength; i++)
				targetBuf[i] = sourcePtr[i];
			targetBuf[copyLength] = 0;

			if (writtenPtr.IsValid())
				*writtenPtr = copyLength;

			return X_STATUS_SUCCESS;
		}

		template< typename T >
		static inline uint32_t StrLen(Pointer<T> str)
		{
			if (!str.IsValid())
				return 0;

			const auto strStart = str;
			while (*str != 0)
				++str;
			return (uint32_t)(str - strStart);
		}

		void Xbox_RtlInitAnsiString(Pointer<X_ANSI_STRING> ansiString, Pointer<char> srcString)
		{
			auto len = StrLen(srcString);
			if (len >= 65534)
				len = 65534;

			ansiString.GetNativePointer()->Length = (uint16_t)len;
			ansiString.GetNativePointer()->MaximumLength = (uint16_t)(len + 1);
			ansiString.GetNativePointer()->BufferPtr = srcString.GetAddress();
		}


		void Xbox_RtlInitUnicodeString(Pointer<X_UNICODE_STRING> ansiString, Pointer<wchar_t> srcString)
		{
			auto len = StrLen(srcString);
			if (len >= 65534)
				len = 65534;

			ansiString.GetNativePointer()->Length = (uint16_t)len;
			ansiString.GetNativePointer()->MaximumLength = (uint16_t)(len + 1);
			ansiString.GetNativePointer()->BufferPtr = srcString.GetAddress();
		}

		uint32_t Xbox_RtlImageXexHeaderField()
		{
			return 0;
		}

		uint32_t Xbox_XexGetProcedureAddress(uint32_t module, Pointer<char> functionName)
		{
			GLog.Warn("Runtime: Lookup for '%hs' in modele 0x%08X", functionName.GetNativePointer(), module);
			return 0;
		}

		uint32_t Xbox_RtlCompareMemoryUlong(Pointer<uint32> data, const uint32_t size, const uint32_t pattern)
		{
			uint32_t ret = 0;

			for (uint32_t i = 0; i < (size / 4); ++i)
				if (data[i] == pattern)
					ret += 1;

			return ret;
		}

		void Xbox_RtlRaiseException(MemoryAddress exceptionRecord)
		{
			GLog.Warn("RaiseException called!");
		}

		void Xbox_RtlFreeAnsiString(Pointer<X_ANSI_STRING> str)
		{
			// free memory allocated for string
			auto memoryAddres = str->BufferPtr.AsData().GetAddress().GetNativePointer();
			GPlatform.GetMemory().FreeSmallBlock(memoryAddres);

			// reset string
			str->BufferPtr = 0;
			str->Length = 0;
			str->MaximumLength = 0;
		}

		X_STATUS Xbox_RtlUnicodeStringToAnsiString(Pointer<X_ANSI_STRING> outStr, Pointer<X_UNICODE_STRING> inStr, uint32_t allocateMemory)
		{
			outStr->Length = 0;
			outStr->MaximumLength = 0;

			const auto length = inStr->Length;
			if (length == 0)
				return X_STATUS_SUCCESS;
			
			const auto memoryNeeded = sizeof(char) * outStr->MaximumLength;
			if (memoryNeeded > 1025 * 1024)
				return X_STATUS_MEMORY_NOT_ALLOCATED;

			if (allocateMemory)
				outStr->BufferPtr = MemoryAddress((uint64_t)GPlatform.GetMemory().AllocateSmallBlock((uint32_t)memoryNeeded));
			else
				outStr->BufferPtr = inStr->BufferPtr.AsData().GetAddress();

			if (!outStr->BufferPtr.AsData().Get().IsValid())
				return X_STATUS_MEMORY_NOT_ALLOCATED;

			for (uint32_t i = 0; i <= length; ++i)
			{
				const auto ch = inStr->BufferPtr.AsData().Get()[i];
				outStr->BufferPtr.AsData().Get()[i] = (ch > 127) ? '?' : (char)ch;
			}

			return X_STATUS_SUCCESS;
		}

		// copied from http://msdn.microsoft.com/en-us/library/ff552263
		void Xbox_RtlFillMemoryUlong(Pointer<uint32> destPtr, uint32 size, uint32 pattern)
		{
			const auto count = size >> 2;
			for (uint32 i = 0; i < count; ++i)
				destPtr[i] = pattern;
		}

		X_STATUS Xbox_XexLoadImage(Pointer<char> modulePath, uint32_t flags, uint32_t loadVersion, Pointer<uint32> outHandle)
		{
			GLog.Warn("Runtime: Request to load image '%hs'", modulePath.GetNativePointer());
			return X_STATUS_ACCESS_DENIED;
		}

		X_STATUS Xbox_XexUnloadImage(uint32_t moduleHandle)
		{
			return X_STATUS_SUCCESS;
		}

		//---------------------------------------------------------------------------

		void RegisterXboxCRT(runtime::Symbols& symbols)
		{
			REGISTER(_vsnprintf)
			REGISTER(sprintf)
			REGISTER(_snprintf)
			REGISTER(DbgPrint);
			REGISTER(RtlUnicodeToMultiByteN);
			REGISTER(RtlMultiByteToUnicodeN);
			REGISTER(RtlInitUnicodeString);
			REGISTER(RtlInitAnsiString);
			REGISTER(RtlFreeAnsiString);
			REGISTER(RtlUnicodeStringToAnsiString);
			REGISTER(RtlFillMemoryUlong);
			REGISTER(RtlImageXexHeaderField);
			REGISTER(RtlCompareMemoryUlong);
			REGISTER(RtlRaiseException);
			REGISTER(XexGetProcedureAddress);
			REGISTER(XexLoadImage);
			REGISTER(XexUnloadImage);
		}

		//---------------------------------------------------------------------------


	} // lib
} // xenon