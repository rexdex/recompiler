#pragma once

namespace runtime
{
	class RegisterBank;
	class RegisterBankInfo;

	/// writes instruction + register trace into the file
	class LAUNCHER_API TraceWriter
	{
	public:
		~TraceWriter();

		// flush the trace file
		void Flush();

		// add next frame to the trace
		void AddFrame(const uint64 ip, const runtime::RegisterBank& regs);

		// create trace file, may fail
		static TraceWriter* CreateTrace(const runtime::RegisterBankInfo& bankInfo, const std::wstring& outputFile);

	private:
		static const uint32 MAX_REGS_TO_WRITE = 512;
		static const uint32 MAX_REG_DATA = 16;

		TraceWriter(const runtime::RegisterBankInfo& bankInfo, HANDLE hFileHandle);

		struct RegToWrite
		{
			uint16 m_dataOffset; // offset in register bank buffer
			uint16 m_size; // size of the data
		};

		uint8			m_prevData[MAX_REGS_TO_WRITE * MAX_REG_DATA]; // enough memory for all registers
		uint8			m_curData[MAX_REGS_TO_WRITE * MAX_REG_DATA]; // enough memory for all registers

		uint8			m_deltaWriteBuffer[MAX_REGS_TO_WRITE * MAX_REG_DATA]; // enough memory for all registers
		uint64			m_deltaWwriteRegMask[MAX_REGS_TO_WRITE / 64];
		uint32			m_deltaWriteSize;

		uint64			m_fullWriteRegMask[MAX_REGS_TO_WRITE / 64];
		uint32			m_fullWriteSize;

		RegToWrite		m_registersToWrite[MAX_REGS_TO_WRITE];
		uint32			m_numRegsToWrite;

		HANDLE			m_outputFile;

		uint32			m_frameIndex;
		uint32			m_numFramesPerPage;

		uint64			m_prevBlockSkipOffset;

		void CapureData(const runtime::RegisterBank& regs);
		void BuildDeltaData();

		void SetOffset(const uint64 offset);
		uint64 GetOffset() const;
		void Write(const void* data, const uint32 size);
	};
	
} // trace
