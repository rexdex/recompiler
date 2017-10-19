#pragma once

namespace platform
{
	class CPURegister;
}

namespace trace
{
	class DataFrame;

	//---------------------------------------------------------------------------

	// how should we interpret the bit patterns
	enum class RegDisplayFormat
	{
		Auto = 0,		// auto select, based on the register type (signed/float only)

		Unsigned,		// interpret bit pattern as unsigned number
		Signed,			// interpret bit pattern as signed number
		FloatingPoint,	// interpret bit pattern as floating point number (4 and 8 bytes regs only)
		Hex,			// hexadecimal representation of the bit pattern (always works)
		ASCII,			// ASCII text representation (good for characters in text)

		// component interpretation

		Comp_Uint8,		// display register value as bytes
		Comp_Uint16,	// display register value as words
		Comp_Uint32,	// display register value as double words
		Comp_Uint64,	// display register value as unsigned 64 bit values

		Comp_Int8,		// display register value as bytes
		Comp_Int16,		// display register value as words
		Comp_Int32,		// display register value as double words
		Comp_Int64,		// display register value as unsigned 64 bit values

		Comp_Float16,	// display as half precision floating point numbers
		Comp_Float32,	// display as single precision floating point numbers
		Comp_Float64,	// display as double precision floating point numbers
	};

	// get register data as int64 
	// respects the parent relation, strips the bits
	extern RECOMPILER_API int64 GetRegisterValueInteger(const platform::CPURegister* reg, const DataFrame& frame, const bool signExtend = true);

	// get register data as floating point value
	extern RECOMPILER_API double GetRegisterValueFloat(const platform::CPURegister* reg, const DataFrame& frame);

	// get printable register value
	extern RECOMPILER_API std::string GetRegisterValueText(const platform::CPURegister* reg, const DataFrame& frame, const RegDisplayFormat format = RegDisplayFormat::Auto);

	//---------------------------------------------------------------------------

} // trace