#pragma once
#include <mutex>
#include <atomic>

namespace platform
{
	class Definition;
	class CPU;
	class CPURegister;
}

namespace trace
{
	class PagedFile;
	class PagedFileReader;

	class DataReader;
	class DataFramePool;

	//---------------------------------------------------------------------------

	/// Visitor for linear access
	class RECOMPILER_API IBatchVisitor
	{
	public:
		virtual ~IBatchVisitor() {};

		//! Process data frame, return false to stop processing
		virtual bool ProcessFrame(const DataFrame& frame) = 0;
	};

	//---------------------------------------------------------------------------

	/// Decoding set of registers
	class RECOMPILER_API Registers
	{
	public:
		Registers();

		// build from register table
		void Build( const std::vector< const platform::CPURegister* >& traceRegisters );
	
		// find index in the trace file for given platform register
		int FindTraceRegisterIndex(const platform::CPURegister* reg) const;

		// get number of mapped registers
		inline const uint32 GetNumTraceRegisters() const { return (uint32)m_traceRegisters.size(); }

		// get register list
		typedef std::vector< const platform::CPURegister* >		TRegistersArray;
		inline const TRegistersArray& GetTraceRegisters() const { return m_traceRegisters; }

		// get sizes (in bytes) of the trace register data, allows to read the data
		typedef std::vector< uint8  >		TRegistersSizeArray;
		inline const TRegistersSizeArray& GetTraceRegisterSizes() const { return m_traceRegistersSizes;  }

	private:
		TRegistersArray		m_traceRegisters;
		TRegistersSizeArray m_traceRegistersSizes;
	};

	//---------------------------------------------------------------------------

	/// Decoding registry data
	class RECOMPILER_API DataFrame
	{
	public:
		DataFrame();
		~DataFrame();

		// get trace frame index
		inline const uint32 GetIndex() const { return m_index; }

		// get instruction address
		inline const uint32 GetAddress() const { return m_address; }

		// get timing value
		inline const uint64 GetClock() const { return m_clock; }

		// get size of the data buffer
		inline const uint32 GetDataSize() const { return m_dataSize; }

		// get data buffer
		inline const uint8* GetData() const { return m_data; }

		// get register value
		inline uint8* GetData() { return m_data; }

		// bind data buffer
		void BindBuffer(uint8* data, const uint32 dataSize);

		// copy values from other frame
		void CopyValues(const DataFrame& frame);

		// empty frame
		static DataFrame& EMPTY();

		//---

		// get untyped data for given register
		// NOTE: register MUST be in the trace
		const uint8* GetRegData(const platform::CPURegister* reg) const;

		//--

		uint32			m_index; // frame index in the trace
		uint32			m_address; // instruction address
		uint64			m_clock; // timing value

	private:
		uint32			m_dataSize;
		uint8*			m_data;	// raw values, requires register to be interpreted

		DataFrame(const DataFrame& other) {};
		DataFrame& operator==(const DataFrame& other) { return *this; }
	};

	//---------------------------------------------------------------------------

	/// Trace data block - base frame + all deltas
	class RECOMPILER_API DataFramePage
	{
	public:
		// get page index (firstFrame / numFramesPerPage)
		inline const uint32 GetPageIndex() const { return m_pageIndex; }

		// get first trace frame covered by this block
		inline const uint32 GetFirstFrame() const { return m_firstFrame; }

		// get number of frames in this block
		inline const uint32 GetNumFrames() const { return m_numFrames; }

		// is given frame index in range of this page ?
		bool ContainsFrame(const uint32 frameIndex) const;

		// get data for a trace frame, NOTE: GLOBAL FRAME INDEX is used here
		const DataFrame& GetFrame(const uint32 frameIndex) const;

		// release the frame block, if it's not used by anybody it may be recycled
		void Release();

		// add additional reference to block
		void AddRef();

	private:
		DataFramePage(Data* owner, const uint32 maxFrames, const uint32 dataSizePerFrame);
		~DataFramePage();

		uint32		m_firstFrame;
		uint32		m_numFrames;
		uint32		m_pageIndex;

		uint32			m_maxFrames;
		DataFrame*		m_frames; // always the same size

		uint32			m_maxData;
		uint8*			m_data; // one big buffer for data for all frames

		uint32		m_lruCacheToken;

		Data* m_owner;
		std::atomic< int > m_refCount;

		inline void Bind(const uint32 pageIndex, const uint32 firstFrame, const uint32 numFrames)
		{
			m_lruCacheToken = 0;
			m_firstFrame = firstFrame;
			m_numFrames = numFrames;
			m_pageIndex = pageIndex;
		}

		friend class Data;
	};

	//---------------------------------------------------------------------------

	/// Decoding trace data (abstracted)
	class RECOMPILER_API Data
	{
	public:
		Data();
		~Data();

		// get the platform
		inline const platform::Definition* GetPlatform() const { return m_platform; }

		// get the CPU used for tracing
		inline const platform::CPU* GetCPU() const { return m_cpu; }

		// get number of data frames in file
		inline const uint32 GetNumDataFrames() const { return m_numFrames; }

		// get number of data frames per page
		inline const uint32 GetNumDataFramesPerPage() const { return m_numFramesPerPage; }

		// get register mapping (used to display the data)
		inline const Registers& GetRegisters() const { return m_registers; }

		//----

		// visit range of data frames, forward/backward, end point are INCLUSIVE
		// batcher can stop the operation
		void BatchVisit( const uint32 firstFrame, const uint32 lastFrame, IBatchVisitor& visitor );

		// get from cache or build a new one
		// returns a refcoutned object
		DataFramePage* GetPage(const uint32 pageIndex);

		// create a simple reader, reader has less contention than this class and retains the cached data
		DataReader* CreateReader() const;

	public:
		// open trace data file
		static Data* OpenTraceFile( class ILogOutput& log, const std::wstring& filePath );

	private:
		static const uint32 MAX_FREE_PAGES = 4096; // max active pages, limited only by the amount of memory in the system

		typedef std::vector< uint64 > TFileOffsetTable;
		typedef std::vector< const platform::CPURegister* > TRegisterTable;

		// traced CPU definition
		const platform::Definition*	m_platform;
		const platform::CPU* m_cpu;

		// file and it's reader
		PagedFile* m_file;
		PagedFileReader* m_fileReader;

		// registers in the trace
		Registers m_registers;

		// offsets to full events (actual offset saved every N-th events)
		TFileOffsetTable m_pageOffsets; // file offset to each full frame data
		uint32 m_numFramesPerPage; // how often the full frame occurs
		uint32 m_numFrames; // total frames in the trace dump

		// data sizgin
		uint32 m_maxDataPerFrame; // if all registers are full

		// free pages for reuse, still indexed by the page index
		typedef std::map< uint32, DataFramePage* >  TFreePages;
		TFreePages	m_freePages;
		uint32 m_freePagesCacheIndex;

		// active pages, indexed by the page index
		typedef std::map< uint32, DataFramePage* >  TActivePages;
		TActivePages	m_activePages;

		// lock for the free and active pages
		std::mutex m_lock;


		// load page data from disk, uncached
		DataFramePage* PreparePage_NoLock(const uint32 pageIndex);

		// get an empty page holder
		DataFramePage* AllocPage_NoLock();

		// release reference to page, when it gets to zero the page is released back in to the free list
		void ReleasePageRef(DataFramePage* page);

		friend class DataReader;
		friend class DataFramePage;
	};

	//---------------------------------------------------------------------------

	/// Trace data viewer, access within the data block is free and does not require synchronization
	class RECOMPILER_API DataReader
	{
	public:
		DataReader(Data* data);
		~DataReader();

		// get number of frames
		const uint32 GetNumFrames() const;

		// read the frame
		const DataFrame& GetFrame(const uint32 frameIndex);

		// get original data
		const Data& GetData() const;

	private:
		static const uint32 MAX_CACHED_PAGES = 4;

		// data
		Data*	m_data;

		// cached pages
		typedef std::vector< DataFramePage* >  TPages;
		TPages	m_pages;

		// request a page for given frame
		DataFramePage* RequestPage(const uint32 frameIndex);
	};

	//---------------------------------------------------------------------------

} // trace