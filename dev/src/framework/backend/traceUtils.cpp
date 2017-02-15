#include "build.h"
#include "traceData.h"
#include "traceUtils.h"
#include "platformCPU.h"

namespace trace
{

	int64 GetRegisterValueInteger(const platform::CPURegister* reg, const DataFrame& frame, const bool signExtend/*= true*/)
	{
		uint64 bitSize = reg->GetBitSize();
		uint64 bitOffset = reg->GetBitOffset();

		const auto* rootReg = reg;
		while (rootReg->GetParent())
		{
			bitOffset += rootReg->GetParent()->GetBitOffset();
			rootReg = rootReg->GetParent();
		}

		// get data
		const auto* rawData = frame.GetRegData(rootReg);
		if (!rawData)
			return 0;

		// register is to big to be represented as single value
		if (bitSize > 64)
			return 0;

		// apply bit offset
		rawData += (bitOffset / 8);
		bitOffset &= 7;

		// compute bit mask
		const uint64 bitMask = (bitSize == 64) ? ~0ULL : ((1ULL << bitSize) - 1);
		uint64 unsignedVal = *(const uint64 *)rawData & bitMask; // TODO: potential AV risk		

		// sign extend to 64 bits if required
		if (signExtend && (bitSize < 64))
		{
			const uint64 bitSign = (1ULL << (bitSize - 1));
			if (unsignedVal)
			{
				const uint64 signExtendBits = ~((1ULL << bitSize) - 1); // 0x00000100 -> 0x000000FF -> 0xFFFFFF00
				unsignedVal |= signExtendBits;
			}
		}

		return (int64&)unsignedVal;
	}

	double GetRegisterValueFloat(const platform::CPURegister* reg, const DataFrame& frame)
	{
		uint64 bitSize = reg->GetBitSize();
		uint64 bitOffset = reg->GetBitOffset();

		const auto* rootReg = reg;
		while (rootReg->GetParent())
		{
			bitOffset += rootReg->GetParent()->GetBitOffset();
			rootReg = rootReg->GetParent();
		}

		// invalid bit shift for a floating point value
		if ((bitOffset & 7) != 0)
			return 0.0f;

		// get data
		const auto* rawData = frame.GetRegData(rootReg) + (bitOffset / 8);
		if (!rawData)
			return 0.0f;

		// interpret data based on reg size 4 (float) and 8 (double) values are supported
		// TODO: add half
		if (bitSize == 32)
		{
			return *(const float*)(rawData);
		}
		else if (bitSize == 64)
		{
			return *(const double*)(rawData);
		}
		else
		{
			return 0.0f;
		}
	}

	static const RegDisplayFormat GetBestFormat(const platform::CPURegister* reg)
	{
		if (reg->GetType() == platform::EInstructionRegisterType::Wide)
			return RegDisplayFormat::Hex; // we can use forced mode to show it 
		else if (reg->GetType() == platform::EInstructionRegisterType::FloatingPoint)
			return RegDisplayFormat::FloatingPoint;

		return RegDisplayFormat::Signed;
	}

	static void PrependString(std::string& str, const char* txt, ...)
	{
		char buf[512];
		va_list args;

		va_start(args, txt);
		vsprintf(buf, txt, args);
		va_end(args);

		str = std::string(buf) + str; // EHHH
	}

	static void PrependStringWithSeparator(std::string& str, const char* txt, ...)
	{
		char buf[512];
		va_list args;

		va_start(args, txt);
		vsprintf(buf, txt, args);
		va_end(args);

		if (!str.empty())
			str = std::string("; ") + str;

		str = std::string(buf);
	}

	std::string GetRegisterValueText(const platform::CPURegister* reg, const DataFrame& frame, const RegDisplayFormat requestedFormat /*= RegDisplayFormat::Auto*/)
	{
		// change format if required
		auto format = (requestedFormat == RegDisplayFormat::Auto) ? GetBestFormat(reg) : requestedFormat;

		// get the data placement
		uint64 bitSize = reg->GetBitSize();
		uint64 bitOffset = reg->GetBitOffset();
		const auto* rootReg = reg;
		while (rootReg->GetParent())
		{
			bitOffset += rootReg->GetParent()->GetBitOffset();
			rootReg = rootReg->GetParent();
		}

		// get data
		const auto* rawData = frame.GetRegData(rootReg);
		if (!rawData)
			return std::string();

		// apply bit offset
		const auto numBytes = bitSize / 8;
		rawData += (bitOffset / 8);
		bitOffset &= 7;

		std::string str;

		// format shit, direct modes are simple
		const uint64 bitMask = (bitSize == 64) ? ~0ULL : ((1ULL << bitSize) - 1);
		if (format == RegDisplayFormat::Unsigned)
		{
			const auto val = (*(const uint64*)rawData) & bitMask;

			PrependString(str, "%llu", val);
		}
		else if (format == RegDisplayFormat::Signed)
		{
			auto unsignedVal = (*(const uint64*)rawData) & bitMask;

			const uint64 bitSign = (1ULL << (bitSize - 1));
			if (unsignedVal && (bitSize < 64))
			{
				const uint64 signExtendBits = ~((1ULL << bitSize) - 1); // 0x00000100 -> 0x000000FF -> 0xFFFFFF00
				unsignedVal |= signExtendBits;
			}

			PrependString(str, "%lld", (int64*)unsignedVal);
		}
		else if (format == RegDisplayFormat::FloatingPoint && (bitSize == 32))
		{
			const auto val = *(const float*)rawData;
			PrependString(str, "%f", (double)val);
		}
		else if (format == RegDisplayFormat::FloatingPoint && (bitSize == 64))
		{
			const auto val = *(const double*)rawData;
			PrependString(str, "%f", val);
		}
		else if (format == RegDisplayFormat::Hex)
		{
			for (auto i = 0; i < numBytes; ++i )
				PrependString(str, "%02X", (uint8)rawData[i]);
		}
		else if (format == RegDisplayFormat::ASCII)
		{
			for (auto i = 0; i < numBytes; ++i)
			{
				const auto b = (uint8)rawData[i];
				if (b == 13)
				{
					PrependString(str, "\\n");
				}
				else if (b == 10)
				{
					PrependString(str, "\\r");
				}
				else  if (b == '\t')
				{
					PrependString(str, "\\t");
				}
				else  if (b < 32 || b > 127)
				{
					PrependString(str, "\\x%d", b);
				}
				else
				{
					PrependString(str, "%c", b);
				}
			}
		}
		else if (format == RegDisplayFormat::Comp_Uint8)
		{
			for (auto i = 0; i < numBytes; ++i)
				PrependStringWithSeparator(str, "%u", *(const uint8*)(rawData + i));
		}
		else if (format == RegDisplayFormat::Comp_Uint16)
		{
			for (auto i = 0; i < numBytes; i += 2)
				PrependStringWithSeparator(str, "%u", *(const uint16*)(rawData + i));
		}
		else if (format == RegDisplayFormat::Comp_Uint32)
		{
			for (auto i = 0; i < numBytes; i += 4)
				PrependStringWithSeparator(str, "%u", *(const uint32*)(rawData + i));
		}
		else if (format == RegDisplayFormat::Comp_Uint64)
		{
			for (auto i = 0; i < numBytes; i += 8)
				PrependStringWithSeparator(str, "%llu", *(const uint64*)(rawData + i));
		}
		else if (format == RegDisplayFormat::Comp_Int8)
		{
			for (auto i = 0; i < numBytes; ++i)
				PrependStringWithSeparator(str, "%d", *(const int8*)(rawData + i));
		}
		else if (format == RegDisplayFormat::Comp_Int16)
		{
			for (auto i = 0; i < numBytes; i += 2)
				PrependStringWithSeparator(str, "%d", *(const int16*)(rawData + i));
		}
		else if (format == RegDisplayFormat::Comp_Int32)
		{
			for (auto i = 0; i < numBytes; i += 4)
				PrependStringWithSeparator(str, "%d", *(const int32*)(rawData + i));
		}
		else if (format == RegDisplayFormat::Comp_Int64)
		{
			for (auto i = 0; i < numBytes; i += 8)
				PrependStringWithSeparator(str, "%lld", *(const int64*)(rawData + i));
		}
		else if (format == RegDisplayFormat::Comp_Float16)
		{
		}
		else if (format == RegDisplayFormat::Comp_Float32)
		{
			for (auto i = 0; i < numBytes; i += 4)
				PrependStringWithSeparator(str, "%f", *(const float*)(rawData + i));
		}
		else if (format == RegDisplayFormat::Comp_Float64)
		{
			for (auto i = 0; i < numBytes; i += 4)
				PrependStringWithSeparator(str, "%f", *(const double*)(rawData + i));
		}

		return str;
	}

} // trace