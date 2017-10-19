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
		DataBuilder(const RawTraceReader& rawTrace);

		//--

		utils::big_vector<DataFile::Entry> m_entries;
		utils::big_vector<Context> m_contexts;
		utils::big_vector<uint8_t> m_blob;

	private:
		virtual void StartContext(const uint32 writerId, const uint32 threadId, const uint64 ip, const TraceFrameID seq, const char* name) override final;
		virtual void EndContext(const uint32 writerId, const uint64 ip, const TraceFrameID seq, const uint32 numFrames) override final;
		virtual void ConsumeFrame(const uint32 writerId, const TraceFrameID seq, const RawTraceFrame& frame) override final;

		const RawTraceReader* m_rawTrace;

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

		utils::big_vector<DeltaContext*> m_deltaContextData;

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
	};

} // trace