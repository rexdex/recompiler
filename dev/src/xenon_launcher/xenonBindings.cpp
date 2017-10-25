#include "build.h"
#include "xenonLib.h"
#include "xenonBindings.h"

namespace xenon
{
	namespace lib
	{
		namespace binding
		{

			//---------------------------------------------------------------------------

			IFunctionCallLogger* FunctionInterface::st_Logger = nullptr;

			FunctionInterface::FunctionInterface(const uint64 ip, runtime::RegisterBank& regs, const char* name)
				: m_ip(ip)
				, m_regs((cpu::CpuRegs&)regs)
				, m_nextGenericArgument(0)
				, m_nextFloatingArgument(0)
			{
				if (st_Logger)
					st_Logger->OnBeginFunction(name);
			}

			void FunctionInterface::BindFunctionLogger(IFunctionCallLogger* logger)
			{
				st_Logger = logger;
			}

			//---------------------------------------------------------------------------

			IFunctionCallLogger::~IFunctionCallLogger()
			{
			}

			//---------------------------------------------------------------------------

			
			__declspec(thread) ITextFunctionTraceLogger::FunctionInfo* ITextFunctionTraceLogger::st_functionInfos = nullptr;
			uint64_t ITextFunctionTraceLogger::st_timeBaseLine = 0;

			ITextFunctionTraceLogger::ITextFunctionTraceLogger()
			{
				QueryPerformanceCounter((LARGE_INTEGER*)&st_timeBaseLine);
			}

			void ITextFunctionTraceLogger::FunctionInfo::Appendf(const char* txt, ...)
			{
				va_list args;

				const auto* textEnd = m_text + sizeof(m_text);
				const auto lengthLeft = (textEnd - m_textWrite) - 1;

				va_start(args, txt);
				const auto len = vsprintf_s(m_textWrite, lengthLeft, txt, args);
				va_end(args);

				if (len > 0)
					m_textWrite += len;
			}

			void ITextFunctionTraceLogger::OnBeginFunction(const char* name)
			{
				auto* ptr = st_functionInfos;
				if (!st_functionInfos)
				{
					ptr = new FunctionInfo();
					st_functionInfos = ptr;
				}

				uint64_t curTime;
				QueryPerformanceCounter((LARGE_INTEGER*)&curTime);
			
				ptr->m_thead = GetCurrentThreadId();
				ptr->m_callStart = curTime;
				ptr->m_callDuration = 0;
				ptr->m_name = name;
				ptr->m_text[0] = '(';
				ptr->m_text[1] = 0;
				ptr->m_numArgs = 0;
				ptr->m_textWrite = ptr->m_text + 1;
				ptr->m_return = false;
			}

			void ITextFunctionTraceLogger::PrintData(FunctionInfo* ptr, const DataType type, const void* data, const uint32 size)
			{
				switch (type)
				{
					case DataType::Int8:
					{
						DEBUG_CHECK(size == 1);
						ptr->Appendf("i8: %d", *(int8_t*)data);
						break;
					}

					case DataType::Int16:
					{
						DEBUG_CHECK(size == 2);
						ptr->Appendf("i16: %d", *(int16_t*)data);
						break;
					}

					case DataType::Int32:
					{
						DEBUG_CHECK(size == 4);
						ptr->Appendf("i32: %d", *(int32_t*)data);
						break;
					}

					case DataType::Int64:
					{
						DEBUG_CHECK(size == 8);
						ptr->Appendf("i64: %d", *(int64_t*)data);
						break;
					}

					case DataType::Uint8:
					{
						DEBUG_CHECK(size == 1);
						ptr->Appendf("u8: %u (0x%02X)", *(uint8_t*)data, *(uint8_t*)data);
						break;
					}

					case DataType::Uint16:
					{
						DEBUG_CHECK(size == 2);
						ptr->Appendf("u16: %u (0x%04X)", *(uint16_t*)data, *(uint16_t*)data);
						break;
					}

					case DataType::Uint32:
					{
						DEBUG_CHECK(size == 4);
						ptr->Appendf("u32: %u (0x%08X)", *(uint32_t*)data, *(uint32_t*)data);
						break;
					}

					case DataType::Uint64:
					{
						DEBUG_CHECK(size == 8);
						ptr->Appendf("u64: %u (0x%08llX)", *(uint64_t*)data, *(uint64_t*)data);
						break;
					}

					case DataType::Address:
					{
						DEBUG_CHECK(size == 4);
						ptr->Appendf("a: 0x%08X", ((MemoryAddress*)data)->GetAddressValue());
						break;
					}

					case DataType::AnsiString:
					{
						DEBUG_CHECK(size == 4);
						const auto p = *(Pointer<char>*)data;
						ptr->Appendf("s: '%hs'", p.GetNativePointer());
						break;
					}

					case DataType::UniString:
					{
						DEBUG_CHECK(size == 4);
						const auto p = *(Pointer<wchar_t>*)data;
						ptr->Appendf("s: '%ls'", p.GetNativePointer());
						break;
					}

					case DataType::Pointer8:
					{
						DEBUG_CHECK(size == 4);
						auto p = *(Pointer<uint8_t>*)data;
						ptr->Appendf("p8: 0x%08X", p.GetAddress().GetAddressValue());
						if (p.IsValid())
						{
							ptr->Appendf(" [");
							PrintData(ptr, DataType::Uint8, p.GetNativePointer(), 1);
							ptr->Appendf("]");
						}
						break;
					}

					case DataType::Pointer16:
					{
						DEBUG_CHECK(size == 4);
						auto p = *(Pointer<uint16_t>*)data;
						ptr->Appendf("p8: 0x%08X", p.GetAddress().GetAddressValue());
						if (p.IsValid())
						{
							ptr->Appendf(" [");
							PrintData(ptr, DataType::Uint16, p.GetNativePointer(), 2);
							ptr->Appendf("]");
						}
						break;
					}

					case DataType::Pointer32:
					{
						DEBUG_CHECK(size == 4);
						auto p = *(Pointer<uint32_t>*)data;
						ptr->Appendf("p8: 0x%08X", p.GetAddress().GetAddressValue());
						if (p.IsValid())
						{
							ptr->Appendf(" [");
							PrintData(ptr, DataType::Uint32, p.GetNativePointer(), 4);
							ptr->Appendf("]");
						}
						break;
					}

					case DataType::Pointer64:
					{
						DEBUG_CHECK(size == 4);
						auto p = *(Pointer<uint64_t>*)data;
						ptr->Appendf("p8: 0x%08X", p.GetAddress().GetAddressValue());
						if (p.IsValid())
						{
							ptr->Appendf(" [");
							PrintData(ptr, DataType::Uint64, p.GetNativePointer(), 8);
							ptr->Appendf("]");
						}
						break;
					}

					case DataType::PointerGeneric:
					{
						DEBUG_CHECK(size == 4);
						auto p = *(Pointer<uint8_t>*)data;
						ptr->Appendf("p: 0x%08X", p.GetAddress().GetAddressValue());
						if (p.IsValid())
						{
							ptr->Appendf(" [");
							PrintData(ptr, DataType::Generic, p.GetNativePointer(), size);
							ptr->Appendf("]");
						}
						break;
					}

					case DataType::Float:
					{
						DEBUG_CHECK(size == 4);
						ptr->Appendf("f32: %f", *(float*)data);
						break;
					}

					case DataType::Double:
					{
						DEBUG_CHECK(size == 8);
						ptr->Appendf("f64: %f", *(double*)data);
						break;
					}

					case DataType::Generic:
					{
						const auto dataPtr = (const uint8_t*)data;

						ptr->Appendf("len: %u {", size);
						for (uint32 i = 0; i < size; ++i)
						{
							if (i > 0)
								ptr->Appendf(",");
							ptr->Appendf("%02X", dataPtr[i]);
						}
						ptr->Appendf("}");
						break;
					}
				}
			}

			void ITextFunctionTraceLogger::OnArgument(const DataType type, const void* data, const uint32 size)
			{
				auto* ptr = st_functionInfos;

				if (ptr->m_numArgs > 0)
					ptr->Appendf(", ");
				++ptr->m_numArgs;

				PrintData(ptr, type, data, size);
			}

			void ITextFunctionTraceLogger::OnReturnValue(const DataType type, const void* data, const uint32 size)
			{
				auto* ptr = st_functionInfos;

				ptr->m_return = true;
				ptr->Appendf("), returned: ");
				PrintData(ptr, type, data, size);
			}

			void ITextFunctionTraceLogger::OnEndFunction(const uint64 returnPoint)
			{
				auto* ptr = st_functionInfos;

				if (!ptr->m_return)
					ptr->Appendf(")");

				uint64_t endTimeQ;
				QueryPerformanceCounter((LARGE_INTEGER*)&endTimeQ);

				uint64_t freq;
				QueryPerformanceFrequency((LARGE_INTEGER*)&freq);

				const auto startTime = (double)(ptr->m_callStart - st_timeBaseLine) / (double)freq;
				const auto endTime = (double)(endTimeQ - st_timeBaseLine) / (double)freq;

				const auto startTimeMs = (uint64_t)(startTime * 1000.0);
				const auto durationTimnMs = (uint64_t)((endTime - startTime) * 1000000.0);

				char buf[512];
				sprintf_s(buf, "[%05u][%07llums][+%06lluus][0x%08llX] %s", ptr->m_thead, startTimeMs, durationTimnMs, returnPoint, ptr->m_name);
				Print(buf, ptr->m_text);
			}

			//---------------------------------------------------------------------------

			void StdOutFunctionCallLog::Print(const char* mainInfo, const char* arguments)
			{
				GLog.Log("CallLog: %hs%hs", mainInfo, arguments);
			}

			//---

			FileFunctionCallLog::FileFunctionCallLog(const std::wstring& filePath)
				: m_file(filePath, std::ios::out)
			{}

			void FileFunctionCallLog::Print(const char* mainInfo, const char* arguments)
			{
				m_lock.lock();
				m_file << mainInfo << arguments << std::endl;
				m_lock.unlock();
			}

			//---

		} // binding
	} // lib
} // xenon
