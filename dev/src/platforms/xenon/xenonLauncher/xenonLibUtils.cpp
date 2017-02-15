#include "build.h"
#include "xenonLibUtils.h" 

//---------------------------------------------------------------------------

uint64 __fastcall XboxUtils__vsnprintf( uint64 ip, cpu::CpuRegs& regs )
{
	char* targetBuf = GetPointer<char>(regs.R3);
	uint32 targetBufSize = (uint32)regs.R4;
	const char* formatStr = GetPointer<const char>(regs.R5);

	{
		// format the output data
		const char* cur = formatStr;
		const char* end = formatStr + strlen(formatStr);

		// output string
		char* write = targetBuf;
		char* writeEnd = targetBuf + targetBufSize-1;

		// format values
		const TReg* curReg = &regs.R6;
		while ( cur < end )
		{
			const char ch = *cur++;

			if (ch == '%')
			{
				// skip the numbers (for now)
				while (*cur >= '0' && *cur <= '9' )
					++cur;

				// get the code				
				const char code = *cur++;
				switch ( code )
				{
					// just %
					case '%':
					{
						Append(write, writeEnd, '%');
						break;
					}

					// address
					case 'x':
					case 'X':
					{
						const uint64 val = *curReg;
						AppendHex(write, writeEnd, val);
						++curReg;
						break;
					}
				}
			}
			else
			{
				Append(write,writeEnd,ch);
			}
		}

		// return number of characters written
		const uint32 numWritten = (uint32)(write - targetBuf);
		regs.R3 = numWritten;

		// end of string
		Append(write, writeEnd,0);
	}

	// done
	return (uint32)regs.LR;
}

uint64 __fastcall XboxUtils_sprintf( uint64 ip, cpu::CpuRegs& regs )
{
	char* targetBuf = GetPointer<char>(regs.R3);
	const uint32 targetBufSize = 1024;
	const char* formatStr = GetPointer<const char>(regs.R4);

	{
		// format the output data
		const char* cur = formatStr;
		const char* end = formatStr + strlen(formatStr);

		// output string
		char* write = targetBuf;
		char* writeEnd = targetBuf + targetBufSize-1;

		// format values
		const TReg* curReg = &regs.R6;
		while ( cur < end )
		{
			const char ch = *cur++;

			if (ch == '%')
			{
				// skip the numbers (for now)
				while (*cur >= '0' && *cur <= '9' )
					++cur;

				// get the code				
				const char code = *cur++;
				switch ( code )
				{
					// just %
					case '%':
					{
						Append(write, writeEnd, '%');
						break;
					}

					// address
					case 'x':
					case 'X':
					{
						const uint64 val = *curReg;
						AppendHex(write, writeEnd, val);
						++curReg;
						break;
					}
				}
			}
			else
			{
				Append(write,writeEnd,ch);
			}
		}

		// return number of characters written
		const uint32 numWritten = (uint32)(write - targetBuf);
		regs.R3 = numWritten;

		// end of string
		Append(write, writeEnd,0);
	}

	// done
	return (uint32)regs.LR;
}

uint64 __fastcall XboxUtils_RtlUnicodeToMultiByteN( uint64 ip, cpu::CpuRegs& regs )
{
	RETURN_DEFAULT();
}

uint64 __fastcall XboxUtils_RtlMultiByteToUnicodeN( uint64 ip, cpu::CpuRegs& regs )
{
	RETURN_DEFAULT();
}

//---------------------------------------------------------------------------

void RegisterXboxUtils(runtime::Symbols& symbols)
{
	#define REGISTER(x) symbols.RegisterFunction(#x, (runtime::TBlockFunc) &XboxUtils_##x);
	REGISTER(_vsnprintf)
	REGISTER(sprintf)
	REGISTER(RtlUnicodeToMultiByteN);
	REGISTER(RtlMultiByteToUnicodeN);
	#undef REGISTER
}

//---------------------------------------------------------------------------
