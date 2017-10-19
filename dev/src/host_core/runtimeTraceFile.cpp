#include "build.h"
#include "runtimeTraceFile.h"
#include "runtimeTraceWriter.h"
#include "runtimeCPU.h"

#include "../recompiler_core/traceCommon.h"
#include "../xenon_launcher/xenonUtils.h"

namespace runtime
{
	///---

	TraceFile::TraceFile(const runtime::RegisterBankInfo& bankInfo, std::unique_ptr<std::ofstream>& outputFile)
		: m_writeFile(std::move(outputFile))
		, m_numRegsToWrite(0)
		, m_writeFailed(false)
		, m_writeRequestExit(false)
		, m_writePendingCount(0)
		, m_sequenceNumber(0)
	{
		// write header
		common::TraceFileHeader header;
		header.m_magic = common::TraceFileHeader::MAGIC;
		header.m_numRegisers = bankInfo.GetNumRootRegisters();
		strcpy_s(header.m_cpuName, bankInfo.GetCPUName().c_str());
		strcpy_s(header.m_platformName, bankInfo.GetPlatformName().c_str());
		WriteBlockSync(&header, sizeof(header));

		// prepare register write list, also, write the register information
		uint32_t fullWriteSize = 0;
		for (uint32 i = 0; i < bankInfo.GetNumRootRegisters(); ++i)
		{
			const auto* reg = bankInfo.GetRootRegister(i);

			common::TraceRegiserInfo regInfo;
			regInfo.m_bitSize = reg->GetBitCount();
			strcpy_s(regInfo.m_name, reg->GetName());
			WriteBlockSync(&regInfo, sizeof(regInfo));

			RegToWrite& writeInfo = m_registersToWrite[m_numRegsToWrite++];
			writeInfo.m_dataOffset = reg->GetDataOffset();
			writeInfo.m_size = reg->GetBitCount() / 8;
			fullWriteSize += writeInfo.m_size;
		}

		GLog.Log("Trace: Trace contains %d registers to write (%d bytes in full frame)", m_numRegsToWrite, fullWriteSize);

		// start writing thread
		m_writeThread.reset(new std::thread(&TraceFile::WriteThreadFunc, this));
	}

	TraceFile::~TraceFile()
	{
		// detach all writers, this prevents any more data being sent our way even if the objects are not deleted
		DetachWriters();

		// write all pending data
		Flush();

		// wait for the writing thread to finish
		m_writeRequestExit = true;
		m_writeLockCV.notify_one();
		m_writeThread->join();
		m_writeThread.reset();

		// free all temporary memory blocks
		utils::ClearPtr(m_freeBlocks);
	}

	void TraceFile::WriteThreadFunc()
	{
		GLog.Log("Trace: Write thread started");

		// periodically show info about trace writing progress
		const size_t logWarningEvery = 100 * 1024 * 1024; 
		size_t logNextWriteSize = logWarningEvery;

		while (1)
		{
			// wait for the signal
			std::unique_lock<std::mutex> lk(m_writeLock);
			m_writeLockCV.wait(lk);

			// exit requested
			if (m_writeRequestExit)
			{
				GLog.Log("Trace: Received exit request for the write thread");
				break;
			}

			// pop data to write, after that we can release the queue lock
			Block* block = nullptr;
			{
				std::lock_guard<std::mutex> lock(m_writeQueueLock);
				block = m_writeQueue.front();
				m_writeQueue.pop_front();
			}
			lk.unlock();

			// write the data to file
			// NOTE: we have exclusive access to file here
			if (block != nullptr)
			{
				m_writeFile->write((const char*)block->m_memory, block->m_size);

				// failed ?
				if (m_writeFile->fail())
				{
					GLog.Err("Trace: File write error. Check for disk space.");
					m_writeFailed = true;
				}

				// release block
				FreeBlock(block);
				--m_writePendingCount;

				// write info
				if ((uint64)m_writeFile->tellp() > logNextWriteSize)
				{
					GLog.Warn("Trace: Written %1.2f MB (%u entries)", (double)m_writeFile->tellp() / (1024.0*1024.0), (uint64)m_sequenceNumber);
					logNextWriteSize = logNextWriteSize + logWarningEvery;
				}
			}
		}

		GLog.Log("Trace: Write thread finished, pending writes=%u", (uint32)m_writePendingCount);
	}

	void TraceFile::Flush()
	{
		std::lock_guard<std::mutex> lock(m_writersLock);

		for (auto* it : m_writers)
			it->LocalFlush();

		// TODO: better way..
		for (;;)
		{
			if (0 == m_writePendingCount)
				break;

			// wait before rechecking to allow the pending writes to complete 
			std::this_thread::sleep_for(std::chrono::microseconds(50));
			m_writeLockCV.notify_all();
		}		
	}

	void TraceFile::WriteBlockSync(const void* data, const size_t size)
	{
		// we have failed writing already, skip more writes (disk is probably full...)
		if (m_writeFailed)
			return;

		// nothing to write
		if (!size)
			return;

		// write data
		m_writeFile->write((const char*)data, size);

		// failed ?
		if (m_writeFile->fail())
		{
			GLog.Err("Trace: File write error. Check for disk space.");
			m_writeFailed = true;
		}
	}

	void TraceFile::WriteBlockAsync(const void* data, const size_t size)
	{
		// we have failed writing already, skip more writes (disk is probably full...)
		if (m_writeFailed)
			return;

		// nothing to write
		if (!size)
			return;

		/*// allocate temp memory for the data
		auto* block = AllocBlock(size);
		memcpy(block->m_memory, data, size);

.		// send the block to the writer thread
		++m_writePendingCount;
		{
			std::unique_lock<std::mutex> lk(m_writeQueueLock);
			m_writeQueue.push_back(block);
		}
		m_writeLockCV.notify_one();		 */
		WriteBlockSync(data, size);
	}

	void TraceFile::DetachWriters()
	{
		std::lock_guard<std::mutex> lock(m_writersLock);

		for (auto* it : m_writers)
			it->Detach();
		m_writers.clear();
	}

	//--

	TraceFile::Block* TraceFile::AllocBlock(const size_t requiredSize)
	{
		DEBUG_CHECK(requiredSize <= MEMORY_BLOCK_SIZE);

		std::lock_guard<std::mutex> lock(m_freeBlockLock);

		if (m_freeBlocks.empty())
		{
			auto* block = new Block();
			block->m_memory = malloc(MEMORY_BLOCK_SIZE);
			block->m_size = requiredSize;
			return block;
		}
		else
		{
			auto* block = m_freeBlocks.back();
			m_freeBlocks.pop_back();
			return block;
		}
	}

	void TraceFile::FreeBlock(Block* block)
	{
		std::lock_guard<std::mutex> lock(m_freeBlockLock);
		m_freeBlocks.push_back(block);
	}

	//--

	TraceWriter* TraceFile::CreateThreadWriter(const uint32_t threadId)
	{
		char buf[16];
		sprintf_s(buf, "Thread%u", threadId);
		return new TraceWriter(this, threadId, m_sequenceNumber, buf);
	}

	TraceWriter* TraceFile::CreateInterruptWriter(const char* name)
	{
		return new TraceWriter(this, 0, m_sequenceNumber, name);
	}

	//--

	TraceFile* TraceFile::Create(const runtime::RegisterBankInfo& bankInfo, const std::wstring& outputFile)
	{
		// open the target file
		auto file = std::make_unique<std::ofstream>(outputFile, std::ios_base::out | std::ios_base::binary);
		if (!file || file->fail())
			return nullptr;

		// create the write
		return new TraceFile(bankInfo, file);
	}


} // runtime