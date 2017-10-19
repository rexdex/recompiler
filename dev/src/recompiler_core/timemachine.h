#pragma once

class ILogOutput;

namespace timemachine
{
	class Entry;
	class Trace;

	/// trace - a collection of entries
	class RECOMPILER_API Trace
	{
	public:
		~Trace();

		// create the time machine trace entry
		static Trace* CreateTimeMachine(decoding::Context* context, trace::DataFile* traceData, const TraceFrameID traceFrameIndex);

		// get entry for given trace frame index
		const Entry* GetTraceEntry(ILogOutput& log, const TraceFrameID traceFrameIndex);

		//---

		// get the raw reader of the trace data
		inline trace::DataFile* GetTraceReader() const { return m_traceReader; }

		// get root entry
		inline const Entry* GetRoot() const { return m_rootEntry; }

		// get the parent decoding context
		inline decoding::Context& GetDecodingContext() const { return *m_context; }

	private:
		Trace();

		typedef std::vector< Entry* > TEntries;
		typedef std::map< TraceFrameID, Entry* > TEntriesMap;

		decoding::Context*		m_context;

		trace::DataFile*		m_traceReader;
		Entry*					m_rootEntry;
		TEntries				m_entries;
		TEntriesMap				m_entriesMap;
	};

	/// time machine entry - abstracted trace entry with "knowledge"
	class RECOMPILER_API Entry
	{
	public:
		class AbstractSource;
		class AbstractDependency;

		// abstract dependency
		class AbstractDependency
		{
		public:
			virtual ~AbstractDependency() {};
			virtual const Entry* GetEntry() const = 0;
			virtual std::string GetCaption() const = 0; // short visual description "r1"
			virtual std::string GetValue(Trace* trace) const = 0; // short visual value
			virtual void Resolve(ILogOutput& log, Trace* trace, std::vector< const AbstractSource* >& outDependencies) const = 0; // resolve the trace index that is a dependency
		};

		// abstract data source
		class AbstractSource
		{
		public:
			virtual ~AbstractSource() {};
			virtual const Entry* GetEntry() const = 0;
			virtual std::string GetCaption() const = 0; // short visual description "r1"
			virtual std::string GetValue(Trace* trace) const = 0; // short visual value
			virtual bool IsSourceFor(const AbstractDependency* dependency) const = 0;
			virtual void Resolve(ILogOutput& log, Trace* trace, std::vector< const AbstractDependency* >& outDependencies) const = 0; // resolve the trace index that is a dependency
		};

		~Entry();

		// create trace entry, may fail
		static Entry* CreateEntry(class ILogOutput& log, decoding::Context* context, Trace* trace, class trace::DataFile* reader, const TraceFrameID traceEntryIndex);

		inline decoding::Context& GetDecodingContext() const { return *m_context; }
		inline Trace& GetTrace() const { return *m_trace; }

		inline const std::string& GetDisplay() const { return m_display; }

		inline const TraceFrameID GetTraceIndex() const { return m_traceIndex; }
		inline const uint64_t GetCodeAddres() const { return m_codeAddress; }

		inline uint32 GetNumDependencies() const { return (uint32)m_abstarctDeps.size(); }
		inline const AbstractDependency* GetDependency(const uint32 index) const { return m_abstarctDeps[index]; }

		inline uint32 GetNumSources() const { return (uint32)m_abstractSources.size(); }
		inline const AbstractSource* GetSource(const uint32 index) const { return m_abstractSources[index]; }

	private:
		Entry(decoding::Context* context, Trace* trace, const TraceFrameID traceIndex);

		typedef std::vector< AbstractDependency* >		TAbstractDependencies;
		typedef std::vector< AbstractSource* >			TAbstractSources;

		decoding::Context*			m_context;
		Trace*						m_trace;

		TraceFrameID				m_traceIndex;		 // decoding trace frame index
		uint64						m_codeAddress;		 // instruction address
		std::string					m_display;			 // instruction code (mostly)

		TAbstractDependencies		m_abstarctDeps;		 // discovered dependencies (NOT RESOLVED yet)
		TAbstractSources			m_abstractSources;	 // discovered modified data (NOT RESOLVED yet)
	};

} // timemachine