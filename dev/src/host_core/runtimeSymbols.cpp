#include "build.h"
#include "runtimeSymbols.h"
#include "runtimeRegisterBank.h"
#include "runtimeExceptions.h"

namespace runtime
{

	Symbols::Symbols()
	{
		m_defaultMemReaderFunc = &UnhandledGlobalRead;
		m_defaultMemWriterFunc = &UnhandledGlobalWrite;
		m_defaultPortReaderFunc = &UnhandledPortRead;
		m_defaultPortWriterFunc = &UnhandledPortWrite;
	}

	void Symbols::RegisterSymbol(const char* name, const void* ptr)
	{
		const uint64 address = (const uint64)ptr;
		if (address > 0xFFFFFFFF)
		{
			GLog.Err("Symbol '%s' has address extending default 32-bit range", name);
			DEBUG_CHECK(address <= 0xFFFFFFFF);
			return;
		}

		auto it = m_symbols.find(name);
		if (it != m_symbols.end())
		{
			GLog.Err("Symbol '%s' is already registered as import symbol.", name);
			return;
		}

		SymbolInfo info;
		info.m_addressValue = (uint32)address;
		info.m_name = name;
		m_symbols[name] = info;
	}

	void Symbols::RegisterFunction(const char* name, TBlockFunc functionPtr)
	{
		auto it = m_functions.find(name);
		if (it != m_functions.end())
		{
			GLog.Err("Function '%s' is already registered as import symbol.", name);
			return;
		}

		// define function
		FunctionInfo info;
		info.m_functionCode = functionPtr ? functionPtr : &Symbols::MissingImportFunction;
		info.m_name = name;
		m_functions[name] = info;
	}

	void Symbols::RegisterInterrupt(const uint32 index, TInterruptFunc functionPtr)
	{
		DEBUG_CHECK(functionPtr != nullptr);
		DEBUG_CHECK(index <= 255);
		m_interrupts.Add((uint8)index, functionPtr);
	}

	void Symbols::RegisterPortIO(const uint32 portIndex, TGlobalPortReadFunc portReadFunc, TGlobalPortWriteFunc portWriteFunc)
	{
		DEBUG_CHECK(portIndex <= 65535);

		if (portReadFunc != nullptr)
			m_portReaders.Add((uint16)portIndex, portReadFunc);

		if (portWriteFunc != nullptr)
			m_portWriters.Add((uint16)portIndex, portWriteFunc);
	}

	void Symbols::RegisterMemoryIO(const uint64 memoryAddress, TGlobalMemReadFunc memReadFunc, TGlobalMemWriteFunc memWriteFunc)
	{
		if (memReadFunc != nullptr)
			m_memReaders.Add(memoryAddress, memReadFunc);

		if (memWriteFunc != nullptr)
			m_memWriters.Add(memoryAddress, memWriteFunc);
	}

	void Symbols::SetDefaultMemoryIO(TGlobalMemReadFunc memReadFunc, TGlobalMemWriteFunc memWriteFunc)
	{
		if ( memReadFunc )
			m_defaultMemReaderFunc = memReadFunc;
	
		if (memWriteFunc)
			m_defaultMemWriterFunc = memWriteFunc;
	}

	void Symbols::SetDefaultPortIO(TGlobalPortReadFunc portReadFunc, TGlobalPortWriteFunc portWriteFunc)
	{
		if (portReadFunc)
			m_defaultPortReaderFunc = portReadFunc;

		if (portWriteFunc)
			m_defaultPortWriterFunc = portWriteFunc;
	}

	const uint64 Symbols::FindSymbolAddress(const char* name) const
	{
		// direct symbol
		auto it = m_symbols.find(name);
		if (it != m_symbols.end())
			return (*it).second.m_addressValue;

		// not found
		return 0;
	}

	runtime::TBlockFunc Symbols::FindFunctionCode(const char* name) const
	{
		// function symbol
		auto jt = m_functions.find(name);
		if (jt != m_functions.end())
			return (*jt).second.m_functionCode;

		// not found
		return 0;
	}

	TInterruptFunc Symbols::FindInterruptCallback(const uint32 intterruptIndex) const
	{
		return m_interrupts.Find((uint8)intterruptIndex);
	}

	TGlobalPortReadFunc Symbols::FindPortIOReader(const uint32 portIndex) const
	{
		auto ret = m_portReaders.Find((uint16)portIndex);
		return ret ? ret : m_defaultPortReaderFunc;
	}

	TGlobalPortWriteFunc Symbols::FindPortIOWriter(const uint32 portIndex) const
	{
		auto ret = m_portWriters.Find((uint16)portIndex);
		return ret ? ret : m_defaultPortWriterFunc;
	}

	TGlobalMemReadFunc Symbols::FindMemoryIOReader(const uint64 memoryAddress) const
	{
		auto ret = m_memReaders.Find(memoryAddress);
		return ret ? ret : m_defaultMemReaderFunc;
	}

	TGlobalMemWriteFunc Symbols::FindMemoryIOWriter(const uint64 memoryAddress) const
	{
		auto ret = m_memWriters.Find(memoryAddress);
		return ret ? ret : m_defaultMemWriterFunc;
	}

	uint64 __fastcall Symbols::MissingImportFunction(uint64 ip, RegisterBank& regs)
	{
		GLog.Err("Called missing import function at %06Xh", ip);
		return regs.ReturnFromFunction();
	}

} // runtime