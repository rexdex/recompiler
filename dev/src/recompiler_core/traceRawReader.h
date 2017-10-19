#pragma once

namespace trace
{
	//---------------------------------------------------------------------------

	class PagedFile;
	class PagedFileReader;

	//---------------------------------------------------------------------------

	// raw trace frame data
	struct RawTraceFrame
	{		
		uint32 m_writerId;
		uint32 m_threadId;
		uint64 m_timeStamp;
		uint64 m_ip;
		TraceFrameID m_seq;
		std::vector<uint8> m_data;

		inline RawTraceFrame()
			: m_writerId(0)
			, m_threadId(0)
			, m_timeStamp(0)
			, m_ip(0)
			, m_seq(0)
		{}
	};

	//---------------------------------------------------------------------------

	// trace visitor
	class RECOMPILER_API IRawTraceVisitor
	{
	public:
		virtual ~IRawTraceVisitor() {};

		// start trace context
		virtual void StartContext(ILogOutput& log, const uint32 writerId, const uint32 threadId, const uint64 ip, const TraceFrameID seq, const char* name) = 0;

		// end trace context, called with the index of last trace frame for that writer
		virtual void EndContext(ILogOutput& log, const uint32 writerId, const uint64 ip, const TraceFrameID seq, const uint32 numFrames) = 0;

		// consume frame
		// NOTE: called in increasing sequence order
		// NOTE: there may be gaps
		virtual void ConsumeFrame(ILogOutput& log, const uint32 writerId, const TraceFrameID seq, const RawTraceFrame& frame) = 0;
	};

	//---------------------------------------------------------------------------

	// reader for the raw trace data
	class RECOMPILER_API RawTraceReader
	{
	public:
		~RawTraceReader();

		//--

		struct RegInfo
		{
			std::string m_name;
			uint32 m_dataSize;
			uint32 m_dataOffset;
		};

		// get name of the CPU
		inline const std::string& GetCPUName() const { return m_cpuName; }

		// get name of the platform
		inline const std::string& GetPlatformName() const { return m_platformName; }

		// get name of the registers used in the file
		inline const std::vector<RegInfo>& GetRegisters() const { return m_registers; }

		// get size of memory block for each trace frame
		inline const uint32 GetFrameSize() const { return m_frameSize; }

		//--

		// scan the file with visitor, this extracts all the data from the file
		void Scan(ILogOutput& log, IRawTraceVisitor& vistor) const;

		//--

		// open the raw trace file
		static std::unique_ptr<RawTraceReader> Load(ILogOutput& log, const std::wstring& rawTraceFilePath);

	private:
		RawTraceReader(std::unique_ptr<std::ifstream>& f);

		void Read(void* data, const uint32_t size) const;
		void DecodeFrameData(const uint8* mask, uint8* refData, uint8* curData) const;

		struct Context
		{
			uint32 m_writerId;
			uint32 m_threadId;
			std::vector<uint8> m_refData;
			uint32 m_numEntries;

			TraceFrameID m_lastSeq;
			uint64 m_lastIp;

			inline Context(uint32 writerId, uint32 threadId, uint32 size)
				: m_writerId(writerId)
				, m_threadId(threadId)
				, m_lastSeq(0)
				, m_lastIp(0)
				, m_numEntries(0)
			{
				m_refData.resize(size, 0);
			}
		};

		std::unique_ptr<std::ifstream> m_file;
		std::vector<RegInfo> m_registers;

		uint32 m_frameSize;
		uint64 m_postHeaderOffset;
		uint64 m_fileSize;

		std::string m_cpuName;
		std::string m_platformName;

		//--
	};

	//---------------------------------------------------------------------------

} // trace