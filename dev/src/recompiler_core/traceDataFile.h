#pragma once
#include <functional>

namespace trace
{

	class RawTraceReader;

	// representation of trace point in various units
	struct LocationInfo
	{
		TraceFrameID m_seq; // global sequence number
		uint32 m_contextId; // context (thread/IRQ, etc)
		uint32 m_contextSeq; // sequence number in context
		uint64 m_ip; // instruction pointer
		uint64 m_time; // time stamp

		inline LocationInfo()
			: m_seq(0)
			, m_ip(0)
			, m_time(0)
			, m_contextId(0)
			, m_contextSeq(0)
		{}
	};

	// type of thread execution
	enum class ContextType : uint32_t
	{
		Thread=0,
		IRQ=1,
		APC=2,
	};

	// type of frame
	enum class FrameType : uint8_t
	{
		Invalid=0,
		CpuInstruction=1,
		ExternalMemoryWrite=2,
	};

	// context of code execution, usually a thread, sometimes
	struct Context
	{
		// general info
		ContextType m_type;		// type of the execution context, mostly THREAD, sometimes IRQ or APC
		uint32 m_id;			// unique ID of this context
		uint32 m_threadId;		// in case of thread this is the thread ID
		char m_name[32];		// user-given name
		LocationInfo m_first;	// first executed object
		LocationInfo m_last;	// last executed object

		//--

		// callstack for this execution context
		uint32 m_rootCallFrame;	

		//--

		// lateral code trace pages
		uint32 m_firstCodePage;
		uint32 m_numCodePages;

		inline Context()
			: m_type(ContextType::Thread)
			, m_id(0)
			, m_threadId(0)
			, m_rootCallFrame(0)
			, m_firstCodePage(0)
			, m_numCodePages(0)
		{
			memset(m_name, 0, sizeof(m_name));
		}
	};
	
	// navigation data
	struct NavigationData
	{
		TraceFrameID m_prevInContext;
		TraceFrameID m_nextInContext;
		uint32 m_context;

		inline NavigationData()
			: m_prevInContext(INVALID_TRACE_FRAME_ID)
			, m_nextInContext(INVALID_TRACE_FRAME_ID)
			, m_context(0)
		{}
	};

	// call frame (executed function)
	struct CallFrame
	{
		uint64_t		m_functionStart;		// address of function entered
		LocationInfo	m_enterLocation;		// where was the scope entered
		LocationInfo	m_leaveLocation;		// where was the scope left
		uint32_t		m_parentFrame;			// 0 if root
		uint32_t		m_firstChildFrame;		// first child frame index (0 means no child calls)
		uint32_t		m_nextChildFrame;		// next child frame (0 marks end of the list)

		inline CallFrame()
			: m_functionStart(0)
			, m_parentFrame(0)
			, m_firstChildFrame(0)
			, m_nextChildFrame(0)
		{}
	};

	// code trace page
	struct CodeTracePage
	{
		static const uint32_t NUM_ADDRESSES_PER_PAGE = 4096; // normal page
		uint64_t m_baseAddress; // base address of data
		uint64_t m_dataOffsets[NUM_ADDRESSES_PER_PAGE]; // offsets to entry lists in data blob
	};

	// decoded frame
	class RECOMPILER_API DataFrame
	{
	public:
		DataFrame(const LocationInfo& locInfo, const FrameType& type, const NavigationData& navigation, std::vector<uint8>& data);

		// get the type of the frame
		inline const FrameType GetType() const { return m_type; }

		// get frame location info
		inline const LocationInfo& GetLocationInfo() const { return m_location; }

		// get navigation info
		inline const NavigationData& GetNavigationInfo() const { return m_navigation; }

		// get code address (IP) of the frame
		inline const uint64 GetAddress() const { return m_location.m_ip; }

		// get sequience index of the frame
		inline const TraceFrameID GetIndex() const { return m_location.m_seq; }

		// get raw data buffer
		inline const void* GetRawData() const { return m_data.data(); }

		// get untyped data for given register
		// NOTE: register MUST be in the trace
		const uint8* GetRegData(const platform::CPURegister* reg) const;

	private:
		FrameType m_type; // trace entry data
		LocationInfo m_location; // where is that frame
		NavigationData m_navigation; // navigation info
		std::vector<uint8> m_data; // collapsed data for ALL registers or the data for the memory write
	};

	// trace data file, provides the low-level access to the trace data
	class RECOMPILER_API DataFile
	{
	public:
		~DataFile();

		//--

		// get total number of frames 
		inline const uint64 GetNumDataFrames() const { return m_entries.size(); }

		// get list of trace contexts in the file
		typedef std::vector<Context> TContextList;
		inline const TContextList& GetContextList() const { return m_contexts; }

		// get the associated CPU
		inline const platform::CPU* GetCPU() const { return m_cpu; }

		// get register mapping (all used registers)
		typedef std::vector<const platform::CPURegister*> TRegisterList;
		inline const TRegisterList& GetRegisters() const { return m_registers; }

		// get call frames
		typedef std::vector<CallFrame> TCallFrames;
		inline const TCallFrames& GetCallFrames() const { return m_callFrames; }

		// get code pages
		typedef std::vector<CodeTracePage> TCodePages;
		inline const TCodePages& GetCodeTracePages() const { return m_codeTracePages; }

		//--

		// purge frame cache
		// NOTE: this should be called outside the main loop
		void PurgeCache();

		// manager frame cache, purges the cache if it's to big
		// NOTE: this should be called outside the main loop
		void ManageCache();

		// decode frame
		const DataFrame& GetFrame(const TraceFrameID id);

		//--

		// get code trace page (or null if not found) for given trace frame
		const CodeTracePage* GetCodeTracePage(const TraceFrameID id) const;

		// get code trace page (or null if not found) for given address
		const CodeTracePage* GetCodeTracePage(const uint32_t contextId, const uint64_t address) const;

		// in given code page enumerate entries before and after given sequence number
		const bool GetCodeTracePageHistogram(const CodeTracePage& page, const TraceFrameID seq, const TraceFrameID minSeq, const TraceFrameID maxSeq, const uint64_t address, uint32& outBefore, uint32& outAfter) const;

		//--

		// get the inner most (with no more children) entry from call stack trace for given sequence ID
		// this is the function we are in bascialy
		const bool GetInnerMostCallFunction(const TraceFrameID seq, TraceFrameID& outFunctionEntrySeq, TraceFrameID& outFunctionLeaveSeq);

		//--

		// save to file, can fail if we don't have enough disk space
		const bool Save(ILogOutput& log, const std::wstring& filePath) const;

		// build trace data from raw trace
		typedef std::function<decoding::Context*(const uint64_t ip)> TDecodingContextQuery;
		static std::unique_ptr<DataFile> Build(ILogOutput& log, const platform::CPU& cpuInfo, const RawTraceReader& rawTrace, const TDecodingContextQuery& decodingContextQuery);

		// load raw trace data from file
		static std::unique_ptr<DataFile> Load(ILogOutput& log, const platform::CPU& cpuInfo, const std::wstring& filePath);

	private:
		DataFile(const platform::CPU* cpuInfo);

		static const uint32_t MAGIC = 'XTRC';
		static const uint32_t NUM_CHUNKS = 6;

		static const uint32_t CHUNK_CONTEXTS = 0;
		static const uint32_t CHUNK_ENTRIES = 1;
		static const uint32_t CHUNK_DATA_BLOB = 2;
		static const uint32_t CHUNK_CALL_FRAMES = 3;
		static const uint32_t CHUNK_CODE_TRACE = 4;
		static const uint32_t CHUNK_MEMORY_TRACE = 5;

		struct FileChunk
		{
			uint64 m_dataOffset;
			uint64 m_dataSize;
		};

		struct FileRegister
		{
			char m_name[16];
			uint32_t m_dataSize;
		};

		struct FileHeader
		{
			uint32 m_magic;
			uint32 m_numRegisters;
			FileChunk m_chunks[NUM_CHUNKS];
		};

		// trace entry
#pragma pack(push, 1)
		struct Entry
		{
			TraceFrameID m_base; // 0 for master
			uint32 m_type : 8; // type of the frame
			uint32 m_context : 24; // id of the context
			uint64 m_offset; // offset to data
			TraceFrameID m_prevThread; // previous entry in this thread
			TraceFrameID m_nextThread; // next entry in this thread

			inline Entry()
				: m_base(0)
				, m_context(0)
				, m_offset(0)
				, m_prevThread(0)
				, m_nextThread(0)
			{}
		};

		// data blob header, followed by packed register data
		struct BlobInfo
		{
			uint32 m_localSeq; // sequence number in context
			uint64 m_ip; // instruction pointer
			uint64 m_time; // timestamp

			inline BlobInfo()
				: m_localSeq(0)
				, m_ip(0)
				, m_time(0)
			{}
		};
#pragma pack(pop)
		
		// for each trace sequence number this points to the data in the trace buffer
		std::vector<Entry> m_entries;

		// packed trace data
		std::vector<uint8_t> m_dataBlob;

		// trace context table (threads and IRQs/APC/etc)
		std::vector<Context> m_contexts;

		// call frames
		std::vector<CallFrame> m_callFrames;

		// code pages
		std::vector<CodeTracePage> m_codeTracePages;

		//--

		// get the CPU this trace is for
		const platform::CPU* m_cpu;

		// get the register map (trace registers to CPU registers)
		TRegisterList m_registers;		

		// size of the data frame
		uint32 m_dataFrameSize;

		//--

		// empty data frame
		DataFrame* m_emptyFrame;

		// cached frames
		typedef std::unordered_map<TraceFrameID, DataFrame*> TFrameCache;
		TFrameCache m_cache;

		//--

		DataFrame* CompileFrame(const TraceFrameID id);

		void PostLoad();

		friend class DataBuilder;
	};

} // trace