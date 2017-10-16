#pragma once

namespace decoding
{
	class Context;
}

namespace trace
{
	class Data;
	class DataReader;

	/// CallTree (all calls)
	class RECOMPILER_API CallTree
	{
	public:
		struct SFrame
		{
			uint32			m_functionStart;		// address of function entered
			uint32			m_traceEnter;
			uint32			m_traceLeave;			// 0 if no leave
			uint32			m_parentFrame;			// 0 if root
			uint32			m_firstChildFrame;		// first child frame index (0 means no child calls)
			uint32			m_nextChildFrame;		// next child frame (0 marks end of the list)

			inline SFrame()
				: m_functionStart(0)
				, m_traceEnter(0)
				, m_traceLeave(0)
				, m_parentFrame(0)
				, m_firstChildFrame(0)
				, m_nextChildFrame(0)
			{}
		};

		class IStackWalker
		{
		public:
			virtual ~IStackWalker() {};
			virtual void OnEnterFunction(const uint32 address, const uint32 trace) = 0;
			virtual void OnLeaveFunction(const uint32 address, const uint32 trace) = 0;
		};

		class IStackBuilderProgress
		{
		public:
			virtual ~IStackBuilderProgress() {};
			virtual void OnProgress(const uint32 position, const uint32 total) = 0;
		};

		~CallTree();

		// Get number of frames (total)
		const uint32 GetNumFrames() const;

		// Get root frame
		const uint32 GetRootFrame() const;

		// Get frame information
		const SFrame& GetFrame(const uint32 index) const;

		// Walk the stack starting from given frame, can be recursive or not, returns number of frames visited
		const uint32 WalkStack(const uint32 startIndex, IStackWalker& walker, const uint32 recursionDepth) const;

		// Save to file
		const bool Save(class ILogOutput& log, const wchar_t* filePath);

		// Load from file
		static CallTree* OpenTraceCallstack(class ILogOutput& log, const wchar_t* filePath);

		// Build from trace data
		static CallTree* BuildTraceCalltack(class ILogOutput& log, decoding::Context& context, const Data& data, const uint32 startEntry, IStackBuilderProgress& progress);

	private:
		CallTree();

		typedef std::vector< SFrame >		TFrames;

		// internal stack frame walker
		static bool WalkStackFrames(class ILogOutput& log, uint32 &entryIndex, decoding::Context& context, DataReader& reader, uint32& outEntryIndex, TFrames& frames, const uint32 expectedReturnAddress, IStackBuilderProgress& progress);

		TFrames		m_frames;
	};

} // trace