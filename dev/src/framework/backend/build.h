#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <assert.h>

#ifdef _LIB
	#define RECOMPILER_API
#else
	#pragma warning (disable: 4251) // class '' needs to have dll-interface to be used by clients of class ''

	#ifdef RECOMPILER_EXPORTS
		#define RECOMPILER_API __declspec(dllexport)
	#else
		#define RECOMPILER_API __declspec(dllimport)
	#endif
#endif

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned __int64 uint64;
typedef char int8;
typedef short int16;
typedef int int32;
typedef __int64 int64;

#include "output.h"

namespace image
{
	class Binary;
	class Section;
}

namespace code
{
	class Generator;
}

namespace trace
{
	class Data;
	class DataFrame;
	class DataReader;
	class MemoryHistory;
	class MemoryHistoryReader;
	class Registers;
	class CallTree;
}

namespace timemachine
{
	class Trace;
	class Entry;
}

namespace decoding
{
	class Registers;
	class Instruction;
	class InstructionCache;
	class InstructionExtendedInfo;
	class Context;
	class Environment;

	/// Code generation mode
	enum class CompilationMode
	{
		Debug,      // allow debugging
		Release,	// no debugging
		Final,		// final

		MAX,
	};

	// Compiler
	enum class CompilationPlatform
	{
		VS2015,     // visual studio
		XCode,		// apple XCode

		MAX,
	};
}

namespace platform
{
	class CPU;
	class CPURegister;
	class CPUInstruction;
	class Definition;
	class Library;
	class ExportLibrary;
}

namespace xml
{
	class Reader;
}