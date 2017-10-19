#pragma once

#include "traceRawReader.h"
#include "traceDataFile.h"
#include "bigArray.h"

namespace trace
{
	
	/// builder of the trace data
	class DataBuilder : public IRawTraceVisitor
	{
	public:
		typedef std::function<decoding::Context*(const uint64_t ip)> TDecodingContextQuery;
		DataBuilder(const RawTraceReader& rawTrace, const TDecodingContextQuery& decodingContextQuery);

		//--

		utils::big_vector<DataFile::Entry> m_entries;
		utils::big_vector<Context> m_contexts;
		utils::big_vector<uint8_t> m_blob;
		utils::big_vector<CallFrame> m_callFrames;

	private:
		virtual void StartContext(ILogOutput& log, const uint32 writerId, const uint32 threadId, const uint64 ip, const TraceFrameID seq, const char* name) override final;
		virtual void EndContext(ILogOutput& log, const uint32 writerId, const uint64 ip, const TraceFrameID seq, const uint32 numFrames) override final;
		virtual void ConsumeFrame(ILogOutput& log, const uint32 writerId, const TraceFrameID seq, const RawTraceFrame& frame) override final;

		const RawTraceReader* m_rawTrace; // source data

		TDecodingContextQuery m_decodingContextQueryFunc;

		struct DeltaReference
		{
			TraceFrameID m_seq;
			std::vector<uint8_t> m_data;

			uint32_t m_localSeq; // where was the reference established
			uint32_t m_descendSeq; // for how long we use this reference before creating a child
			uint32_t m_retireSeq; // for how long we can keep this reference alive

			inline DeltaReference()
				: m_seq(0)
				, m_localSeq(0)
				, m_descendSeq(0)
				, m_retireSeq(0)
			{}
		};

		struct DeltaContext
		{
			std::vector<DeltaReference> m_references;
			uint32_t m_localSeq;
			TraceFrameID m_prevSeq;

			DeltaContext();

			// retire all reference frames that are to old for given sequence number
			void RetireExpiredReferences();
		};

		struct CallStackBuilder
		{
			std::vector<uint32_t> m_callFrames;
			std::vector<uint32_t> m_lastChildFrames;

			LocationInfo m_speculatedLocation;

			uint64_t m_speculatedReturnNotTakenAddress;
			uint64_t m_speculatedCallNotTakenAddress;

			inline CallStackBuilder(uint32_t rootEntry)
			{
				m_speculatedReturnNotTakenAddress = 0;
				m_speculatedCallNotTakenAddress = 0;
				m_callFrames.push_back(rootEntry);
				m_lastChildFrames.push_back(0);
			}
		};

		utils::big_vector<DeltaContext*> m_deltaContextData;
		utils::big_vector<CallStackBuilder*> m_callstackBuilders;

		//--

		// delta compress the trace frame using hierarchy of reference frames
		void DeltaCompress(DeltaContext& ctx, const RawTraceFrame& frame, TraceFrameID& outRefSeq);

		// do the delta compression, write the delta stream between two data buffers, returns size of written data
		uint32_t DeltaWrite(const uint8_t* referenceData, const uint8_t* currentData);

		// write to blob
		void WriteToBlob(const void* data, const uint32_t size);

		// write to blob, typed
		template< typename T>
		inline void WriteToBlob(const T& data)
		{
			WriteToBlob(&data, sizeof(data));
		}

		//--

		// return from top function in stack builder
		void ReturnFromFunction(ILogOutput& log, CallStackBuilder& builder, const LocationInfo& locationOfReturnInstruction);

		// enter a new function scope
		void EnterToFunction(ILogOutput& log, CallStackBuilder& builder, const LocationInfo& locationOfFirstFunctionInstruction);

		// process frame and extract call stack
		bool ExtractCallstackData(ILogOutput& log, CallStackBuilder& builder, const RawTraceFrame& frame, const uint32_t contextSeq);

	};

} // trace