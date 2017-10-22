#pragma once

#include <mutex>
#include <iosfwd>
#include <atomic>

namespace runtime
{
	class TraceWriter;
	class RegisterBank;
	class RegisterBankInfo;

	/// trace file
	class LAUNCHER_API TraceFile
	{
	public:
		~TraceFile();

		// flush all pending writes
		void Flush();

		// create a writer for a thread
		TraceWriter* CreateThreadWriter(const uint32_t threadId);

		// create a writer for short code (interrupt handler)
		TraceWriter* CreateInterruptWriter(const char* name);

		//---

		// is the trace collection paused ?
		inline const bool IsPaused() const { return m_paused; }

		// pause trace collection
		void Pause();

		// resume trace collection
		void Resume();

		//---

		// create trace file at given location, requires CPU register bank for the CPU you will be storing
		static TraceFile* Create(const runtime::RegisterBankInfo& bankInfo, const std::wstring& outputFile, const uint64 traceTriggerAddress);

	private:
		TraceFile(const runtime::RegisterBankInfo& bankInfo, std::unique_ptr<std::ofstream>& outputFile, const uint64 traceTriggerAddress);

		static const uint32 MAX_REGS_TO_WRITE = 512;
		static const uint32 MEMORY_BLOCK_SIZE = 64 * 1024;

		struct RegToWrite
		{
			uint16 m_dataOffset; // offset in register bank buffer
			uint16 m_size; // size of the data
		};

		RegToWrite m_registersToWrite[MAX_REGS_TO_WRITE];
		uint32 m_numRegsToWrite;

		uint64 m_traceTriggerAddress;

		std::atomic<uint32_t> m_sequenceNumber;
		std::atomic<bool> m_paused;

		//---------

		// block of memory
		struct Block
		{
			void* m_memory;
			size_t m_size;
		};

		// memory blocks, can be reused
		std::vector<Block*> m_freeBlocks;
		std::mutex m_freeBlockLock;

		// allocate block of memory
		Block* AllocBlock(const size_t requiredSize);

		// free block of memory
		void FreeBlock(Block* block);

		//---------

		// list of registered trace writers
		std::vector<TraceWriter*> m_writers;
		std::mutex m_writersLock;

		//---------

		// writing related stuff
		std::unique_ptr<std::ofstream> m_writeFile;
		std::deque<Block*> m_writeQueue;
		std::mutex m_writeQueueLock;
		std::atomic<uint32_t> m_writePendingCount;
		std::condition_variable m_writeLockCV;
		std::mutex m_writeLock;
		std::atomic<bool> m_writeFailed;
		std::atomic<bool> m_writeRequestExit;
		std::unique_ptr<std::thread> m_writeThread;

		void WriteBlockAsync(Block* block);
		void WriteBlockAsync(const void* data, const size_t size);
		void WriteBlockSync(const void* data, const size_t size);
		void DetachWriters();
		void WriteThreadFunc();

		//---

		friend class TraceWriter;
	};

} // runtime