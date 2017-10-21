#pragma once

#include <mutex>
#include <iosfwd>

namespace runtime
{

	class TraceFile;

	/// writes instruction + register trace into the file
	class LAUNCHER_API TraceWriter
	{
	public:
		TraceWriter(TraceFile* owner, const uint32_t threadId, std::atomic<uint32_t>& sequenceNumber, const char* name);
		~TraceWriter();

		// flush any partial blocks
		void LocalFlush();

		// detach from writer, prevents any more writing
		void Detach();

		// add next frame to the trace
		void AddFrame(const uint64 ip, const runtime::RegisterBank& regs);

		// add memory write
		void AddMemoryWrite(const uint64 addr, const uint32 size, const void* data, const char* writerText);

	private:
		static const uint32 LOCAL_WRITE_BUFFER_SIZE = 64 * 1024;
		static const uint32 GUARD_AREA_SIZE = 4 * 1024;
		static const uint32 MAX_REGS_TO_WRITE = 512;
		static const uint32 MAX_REG_DATA = 16;

		TraceFile* m_owner;
		uint32_t m_threadId;
		uint32_t m_writerId;

		std::string m_name;
		uint64 m_lastValidIp;

		std::atomic<uint32_t>* m_sequenceNumber;

		uint8 m_prevData[MAX_REGS_TO_WRITE * MAX_REG_DATA]; // enough memory for all registers

		uint8 m_deltaWriteBuffer[MAX_REGS_TO_WRITE * MAX_REG_DATA]; // enough memory for all registers
		uint64 m_deltaWriteRegMask[MAX_REGS_TO_WRITE / 64];

		uint8 m_localWriteBuffer[LOCAL_WRITE_BUFFER_SIZE];
		uint32 m_localWriteBufferPos;
		uint64 m_localWriteStartFrameIndex;

		uint32 m_frameIndex;

		void WriteBlockHeader();
		void WriteDeltaFrame(const uint64_t ip, const runtime::RegisterBank& regs);

		inline void LocalWrite(const void* data, const uint32 size)
		{
			DEBUG_CHECK(m_localWriteBufferPos + size < LOCAL_WRITE_BUFFER_SIZE);
			memcpy(m_localWriteBuffer + m_localWriteBufferPos, data, size);
			m_localWriteBufferPos += size;
		}

		inline void* LocalWrite(const uint32 size)
		{
			DEBUG_CHECK(m_localWriteBufferPos + size < LOCAL_WRITE_BUFFER_SIZE);
			auto* ptr = m_localWriteBuffer + m_localWriteBufferPos;
			m_localWriteBufferPos += size;
			return ptr;
		}

		template<typename T>
		inline T* LocalWrite()
		{
			auto* ptr = LocalWrite(sizeof(T));
			return new (ptr) T();
		}
	};
	
} // trace

