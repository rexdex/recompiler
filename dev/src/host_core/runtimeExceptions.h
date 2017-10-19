#pragma once

#include <exception>

#define USE_EXCEPTIONS

namespace runtime
{
	//------------

	class RegisterBank;

	//------------

	// base runtime exception
	class Exception : public std::exception
	{
	public:
		inline Exception(const uint64_t ip, const char* message)
			: std::exception(message)
			, m_ip(ip)
		{}

		// get location of the
		inline const uint64_t GetInstructionPointer() const { return m_ip; }

	private:
		const uint64_t m_ip;
	};

	//------------

	// process finished/killed
	class TerminateProcessException : public Exception
	{
	public:
		inline TerminateProcessException(const uint64_t ip, const uint64_t exitCode)
			: Exception(ip, "TerminateProcess")
			, m_exitCode(exitCode)
		{}

		// get the exit code
		inline const uint64_t GetExitCode() const { return m_exitCode; }

	private:
		const uint64_t m_exitCode;
	};

	//------------

	// thread finished/killed
	class TerminateThreadException : public Exception
	{
	public:
		inline TerminateThreadException(const uint64_t ip, const uint64_t threadId, const uint64_t exitCode)
			: Exception(ip, "TerminateThread")
			, m_threadId(threadId)
			, m_exitCode(exitCode)
		{}

		// get thread id
		inline const uint64_t GetThreadId() const { return m_threadId; }

		// get the exit code
		inline const uint64_t GetExitCode() const { return m_exitCode; }

	private:
		const uint64_t m_threadId;
		const uint64_t m_exitCode;
	};

	//------------

	// unhandled trap/IRQ etc
	class TrapException : public Exception
	{
	public:
		inline TrapException(const uint64_t ip, const uint64_t trapType)
			: Exception(ip, "UnhandledTrap")
			, m_trap(trapType)
		{}

		// get trap type
		inline const uint64_t GetTrapIndex() const { return m_trap; }

	private:
		const uint64_t m_trap;
	};

	// invalid instruction address
	// we don't have code precompiled for this address yet it was reached
	class InvalidAddressException : public Exception
	{
	public:
		inline InvalidAddressException(const uint64_t ip, const uint64_t blockStartIp)
			: Exception(ip, "InvalidExecutionAddress")
			, m_blockStart(blockStartIp)
		{}

		// get start of the block
		inline const uint64_t GetBlockStart() const { return m_blockStart; }

	private:
		const uint64_t m_blockStart;
	};

	// invalid instruction reached
	// called when runtime state for some reason does not represent a valid instruction
	class InvalidInstructionException : public Exception
	{
	public:
		inline InvalidInstructionException(const uint64_t ip)
			: Exception(ip, "InvalidInstruction")
		{}
	};

	// unimplemented import function was called
	class UnimplementedImportCalledException : public Exception
	{
	public:
		inline UnimplementedImportCalledException(const uint64_t ip, const char* name)
			: Exception(ip, "UnimplementedImport")
			, m_functionName(name)
		{}

		// get the name of the function
		inline const char* GetFunctionName() const { return m_functionName; }

	private:
		const char* m_functionName;
	};

	// unimplemented read of global memory via non-standard interface
	class UnhandledGlobalReadException : public Exception
	{
	public:
		inline UnhandledGlobalReadException(const uint64_t ip, const uint64_t adress, const uint64_t size)
			: Exception(ip, "UnhandledGlobalRead")
			, m_address(adress)
			, m_size(size)
		{}

		// get address of the read
		inline const uint64_t GetAddress() const { return m_address; }

		// get size of the read
		inline const uint64_t GetSize() const { return m_size; }

	private:
		const uint64_t m_address;
		const uint64_t m_size;
	};

	// unimplemented write of global memory via non-standard interface
	class UnhandledGlobalWriteException : public Exception
	{
	public:
		inline UnhandledGlobalWriteException(const uint64_t ip, const uint64_t adress, const uint64_t size)
			: Exception(ip, "UnhandledGlobalWrite")
			, m_address(adress)
			, m_size(size)
		{}

		// get address of the write
		inline const uint64_t GetAddress() const { return m_address; }

		// get size of the write
		inline const uint64_t GetSize() const { return m_size; }

	private:
		const uint64_t m_address;
		const uint64_t m_size;
	};

	// unimplemented read from port
	class UnhandledPortReadException : public Exception
	{
	public:
		inline UnhandledPortReadException::UnhandledPortReadException(const uint64_t ip, const uint16_t port, const uint64_t size)
			: Exception(ip, "UnhandledPortRead")
			, m_port(port)
			, m_size(size)
		{}

		// get port index
		inline const uint16_t GetPort() const { return m_port; }

		// get size of the read
		inline const uint64_t GetSize() const { return m_size; }

	private:
		const uint16_t m_port;
		const uint64_t m_size;
	};

	// unimplemented write to port
	class UnhandledPortWriteException : public Exception
	{
	public:
		inline UnhandledPortWriteException::UnhandledPortWriteException(const uint64_t ip, const uint16_t port, const uint64_t size)
			: Exception(ip, "UnhandledPortWrite")
			, m_port(port)
			, m_size(size)
		{}

		// get port index
		inline const uint16_t GetPort() const { return m_port; }

		// get size of the write
		inline const uint64_t GetSize() const { return m_size; }

	private:
		const uint16_t m_port;
		const uint64_t m_size;
	};

	//------------

	static inline void InvalidAddress(const uint64_t ip, const uint64_t blockStartIP)
	{
#ifdef USE_EXCEPTIONS
		throw InvalidAddressException(ip, blockStartIP);
#else
		abort();
#endif
	}

	static inline void InvalidInstruction(const uint64_t ip)
	{
#ifdef USE_EXCEPTIONS
		throw InvalidInstructionException(ip);
#else
		abort();
#endif
	}

	static inline void UnimplementedImportFunction(const uint64 ip, const char* name)
	{
#ifdef USE_EXCEPTIONS
		throw UnimplementedImportCalledException(ip, name);
#else
		abort();
#endif
	}

	static void UnhandledGlobalRead(const uint64_t ip, const uint64_t address, const uint64_t size, void* outPtr)
	{
#ifdef USE_EXCEPTIONS
		throw UnhandledGlobalReadException(ip, address, size);
#else
		abort();
#endif
	}

	static void UnhandledGlobalWrite(const uint64_t ip, const uint64_t address, const uint64_t size, const void* ptr)
	{
#ifdef USE_EXCEPTIONS
		throw UnhandledGlobalWriteException(ip, address, size);
#else
		abort();
#endif
	}

	static void UnhandledPortRead(const uint64_t ip, const uint16_t portIndex, const uint64_t size, void* outPtr)
	{
#ifdef USE_EXCEPTIONS
		throw UnhandledPortReadException(ip, portIndex, size);
#else
		abort();
#endif
	}

	static void UnhandledPortWrite(const uint64_t ip, const uint16_t portIndex, const uint64_t size, const void* ptr)
	{
#ifdef USE_EXCEPTIONS
		throw UnhandledPortWriteException(ip, portIndex, size);
#else
		abort();
#endif
	}

	static void UnhandledInterruptCall(const uint64_t ip, const uint32_t index, RegisterBank& regs)
	{
#ifdef USE_EXCEPTIONS
		throw TrapException(ip, index);
#else
		abort();
#endif
	}

	//------------	

} // runtime