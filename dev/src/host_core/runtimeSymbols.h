#pragma once

#include "launcherBase.h"
#include "runtimeImageInfo.h"

namespace runtime
{
	/// lookup table
	template< typename Num, typename Func >
	class FastLookupTable
	{
	public:
		inline FastLookupTable()
			: m_numEntries(0)
		{}

		inline void Add(Num addr, Func func)
		{
			for (uint32 i = 0; i < m_numEntries; ++i)
			{
				DEBUG_CHECK(m_entries[i].addr != addr);
			}

			DEBUG_CHECK(m_numEntries < MAX_ENTRIES);
			m_entries[m_numEntries].func = func;
			m_entries[m_numEntries].addr = addr;
			m_numEntries += 1;
		}

		inline Func Find(Num addr) const
		{
			if (m_hot.addr == addr)
				return m_hot.func;

			for (uint32 i = 0; i < m_numEntries; ++i)
			{
				if (m_entries[i].addr == addr)
				{
					m_hot = m_entries[i];
					return m_hot.func;
				}
			}

			return nullptr;
		}

	private:
		static const uint32 MAX_ENTRIES = 64;

		struct Entry
		{
			Func func;
			Num addr;
		};

		mutable Entry m_hot;

		Entry m_entries[MAX_ENTRIES];
		uint32 m_numEntries;
	};

	/// native system function
	typedef std::function<uint64(const uint64_t ip, RegisterBank& regs) > TSystemFunction;

	/// Symbol/Interrup/Port mapping
	class LAUNCHER_API Symbols
	{
	public:
		Symbols();

		// register symbol, address should be known, address must be in the basic 32-bit address range
		void RegisterSymbol(const char* name, const void* address);

		// register function, calling address will be auto assigned
		void RegisterFunction(const char* name, const TSystemFunction& function);

		// register interrupts callback
		void RegisterInterrupt(const uint32 intterruptIndex, TInterruptFunc functionPtr);

		// register port IO
		void RegisterPortIO(const uint32 portIndex, TGlobalPortReadFunc portReadFunc, TGlobalPortWriteFunc portWriteFunc);

		// register memory IO (very special case)
		void RegisterMemoryIO(const uint64 memoryAddress, TGlobalMemReadFunc memReadFunc, TGlobalMemWriteFunc memWriteFunc);

		// set default memory IO functions, returned if specific function was not found
		void SetDefaultMemoryIO(TGlobalMemReadFunc memReadFunc, TGlobalMemWriteFunc memWriteFunc);

		// set default port IO functions, returned if specific function was not found
		void SetDefaultPortIO(TGlobalPortReadFunc portReadFunc, TGlobalPortWriteFunc portWriteFunc);

		// lookup symbol address, returns 0 if not found
		const uint64 FindSymbolAddress(const char* name) const;

		// lookup function address, returns nullptr if not found
		const TSystemFunction* FindFunction(const char* name) const;

		// lookup interrupt callback, returns nullptr if interrupt was not registered
		TInterruptFunc FindInterruptCallback(const uint32 intterruptIndex) const;

		// lookup port IO reading callbacks, returns nullptr if port was not registered
		TGlobalPortReadFunc FindPortIOReader(const uint32 portIndex) const;

		// lookup port IO writing callbacks, returns nullptr if port was not registered
		TGlobalPortWriteFunc FindPortIOWriter(const uint32 portIndex) const;

		// lookup memory IO reading callbacks, returns nullptr if memory address was not registered
		TGlobalMemReadFunc FindMemoryIOReader(const uint64 memoryAddress) const;

		// lookup memory IO writing callbacks, returns nullptr if memory address was not registered
		TGlobalMemWriteFunc FindMemoryIOWriter(const uint64 memoryAddress) const;

	private:
		static uint64 __fastcall MissingImportFunction(uint64 ip, RegisterBank& regs);

		// general symbol information
		struct SymbolInfo
		{
			const char* m_name;
			uint64 m_addressValue;

			inline SymbolInfo()
				: m_name("")
				, m_addressValue(0)
			{}
		};

		// general function information
		struct FunctionInfo
		{
			const char* m_name;
			TSystemFunction m_functionCode;

			inline FunctionInfo()
				: m_name("")
				, m_functionCode(NULL)
			{}
		};

		// registered data symbols
		typedef std::map< std::string, SymbolInfo > TSymbols;
		TSymbols m_symbols;

		// registered global functions
		typedef std::map< std::string, FunctionInfo >	TFunctions;
		TFunctions m_functions;

		// lookup tables
		typedef FastLookupTable< uint8, TInterruptFunc > TInterrupts;
		typedef FastLookupTable< uint64, TGlobalMemReadFunc > TMemReads;
		typedef FastLookupTable< uint64, TGlobalMemWriteFunc > TMemWriters;
		typedef FastLookupTable< uint16, TGlobalPortReadFunc> TPortReads;
		typedef FastLookupTable< uint16, TGlobalPortWriteFunc> TPortWrites;
		TInterrupts	m_interrupts;
		TMemReads m_memReaders;
		TMemWriters m_memWriters;
		TPortReads m_portReaders;
		TPortWrites m_portWriters;

		// default IO functions
		TGlobalMemReadFunc m_defaultMemReaderFunc;
		TGlobalMemWriteFunc m_defaultMemWriterFunc;
		TGlobalPortReadFunc m_defaultPortReaderFunc;
		TGlobalPortWriteFunc m_defaultPortWriterFunc;
	};

} // runtime