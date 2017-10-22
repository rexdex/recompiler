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
#include <istream>
#include <fstream>
#include <functional>
#include <algorithm>

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

class Commandline;
class IBinaryFileWriter;
class IBinaryFileReader;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned __int64 uint64;
typedef char int8;
typedef short int16;
typedef int int32;
typedef __int64 int64;

typedef uint64 TraceFrameID;

static const uint64 INVALID_TRACE_FRAME_ID = ~0ULL;

#include "output.h"

namespace image
{
	class Binary;
	class Section;
}

namespace code
{
	class IGenerator;
}

namespace trace
{
	class DataFile;
	class DataFrame;
	class MemoryHistory;
	class MemoryHistoryReader;
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