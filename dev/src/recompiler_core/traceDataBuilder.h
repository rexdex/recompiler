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

		void FlushData();

		TraceFrameID m_firstSeq;
		TraceFrameID m_lastSeq;

		utils::big_vector<DataFile::Entry> m_entries;
		utils::big_vector<Context> m_contexts;
		utils::big_vector<uint8_t> m_blob;
		utils::big_vector<CallFrame> m_callFrames;
		utils::big_vector<CodeTracePage> m_codeTracePages;
		utils::big_vector<MemoryTracePage> m_memoryTracePages;

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

			std::vector<uint64_t*> m_unresolvedIPAddresses;
			uint64_t m_lastValidResolvedIP;

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
			uint32_t m_rootCallFrame;

			inline CallStackBuilder(uint32_t rootEntry)
			{
				m_rootCallFrame = rootEntry;
				m_speculatedReturnNotTakenAddress = 0;
				m_speculatedCallNotTakenAddress = 0;
				m_callFrames.push_back(rootEntry);
				m_lastChildFrames.push_back(0);
			}
		};

		struct CodeTraceBuilderPage
		{
			static const uint32_t NUM_ADDRESSES_PER_PAGE = trace::CodeTracePage::NUM_ADDRESSES_PER_PAGE;

			uint64_t m_baseMemoryAddress;
			std::vector<TraceFrameID> m_seqChain[NUM_ADDRESSES_PER_PAGE];

			inline CodeTraceBuilderPage(uint64_t baseMemoryAddress)
				: m_baseMemoryAddress(baseMemoryAddress)
			{}

			inline void RegisterAddress(const RawTraceFrame& rawFrame)
			{
				const auto offset = rawFrame.m_ip - m_baseMemoryAddress;
				m_seqChain[offset].push_back(rawFrame.m_seq);
			}
		};

		struct CodeTraceBuilder
		{
			std::unordered_map<uint64_t, CodeTraceBuilderPage*> m_pages;

			inline ~CodeTraceBuilder()
			{
				for (auto it : m_pages)
					delete it.second;
			}

			inline CodeTraceBuilderPage* GetPage(const uint64_t address)
			{
				// get top part of the address
				const auto pageIndex = address / CodeTraceBuilderPage::NUM_ADDRESSES_PER_PAGE;

				// find existing page with the data
				const auto it = m_pages.find(pageIndex);
				if (it != m_pages.end())
					return (*it).second;

				// create new page
				auto* page = new CodeTraceBuilderPage(pageIndex * CodeTraceBuilderPage::NUM_ADDRESSES_PER_PAGE);
				m_pages[pageIndex] = page;
				return page;
			}

			inline void RegisterAddress(const RawTraceFrame& rawFrame)
			{
				if (rawFrame.m_ip)
				{
					auto* page = GetPage(rawFrame.m_ip);
					page->RegisterAddress(rawFrame);
				}
			}
		};

		struct MemoryWriteInfo
		{
			TraceFrameID m_seq;
			uint8_t m_data;

			const bool operator==(const MemoryWriteInfo& other) const
			{
				return m_seq == other.m_seq;
			}

			const bool operator<(const MemoryWriteInfo& info) const
			{
				return m_seq < info.m_seq;
			}
		};

		struct MemoryTraceBuilderPage
		{
			static const uint32_t NUM_ADDRESSES_PER_PAGE = trace::CodeTracePage::NUM_ADDRESSES_PER_PAGE;

			uint64_t m_baseMemoryAddress;
			std::vector<MemoryWriteInfo> m_seqChain[NUM_ADDRESSES_PER_PAGE];

			inline MemoryTraceBuilderPage(uint64_t baseMemoryAddress)
				: m_baseMemoryAddress(baseMemoryAddress)
			{}

			inline void RegisterWrite(const TraceFrameID seq, const uint64 address, const uint8_t value)
			{
				const auto offset = address - m_baseMemoryAddress;
				m_seqChain[offset].push_back(MemoryWriteInfo{ seq, value });
			}
		};

		struct MemoryTraceBuilder
		{
			std::unordered_map<uint64_t, MemoryTraceBuilderPage*> m_pages;

			inline ~MemoryTraceBuilder()
			{
				for (auto it : m_pages)
					delete it.second;
			}

			inline MemoryTraceBuilderPage* GetPage(const uint64_t address)
			{
				// get top part of the address
				const auto pageIndex = address / CodeTraceBuilderPage::NUM_ADDRESSES_PER_PAGE;

				// find existing page with the data
				const auto it = m_pages.find(pageIndex);
				if (it != m_pages.end())
					return (*it).second;

				// create new page
				auto* page = new MemoryTraceBuilderPage(pageIndex * MemoryTraceBuilderPage::NUM_ADDRESSES_PER_PAGE);
				m_pages[pageIndex] = page;
				return page;
			}

			inline void RegisterWrite(const TraceFrameID seq, const uint64 address, const uint8_t value)
			{
				auto* page = GetPage(address);
				page->RegisterWrite(seq, address, value);
			}
		};

		utils::big_vector<DeltaContext*> m_deltaContextData;
		utils::big_vector<CallStackBuilder*> m_callstackBuilders;
		utils::big_vector<CodeTraceBuilder*> m_codeTraceBuilders;
		MemoryTraceBuilder* m_memoryTraceBuilder;

		//--

		// delta compress the trace frame using hierarchy of reference frames
		void DeltaCompress(DeltaContext& ctx, const RawTraceFrame& frame, TraceFrameID& outRefSeq);

		// do the delta compression, write the delta stream between two data buffers, returns size of written data
		uint32_t DeltaWrite(const uint8_t* referenceData, const uint8_t* currentData);

		// write to blob
		uint64_t WriteToBlob(const void* data, const uint32_t size);

		// write to blob, typed
		template< typename T>
		inline uint64_t WriteToBlob(const T& data)
		{
			return WriteToBlob(&data, sizeof(data));
		}

		//--

		// return from top function in stack builder
		void ReturnFromFunction(ILogOutput& log, CallStackBuilder& builder, const LocationInfo& locationOfReturnInstruction);

		// enter a new function scope
		void EnterToFunction(ILogOutput& log, CallStackBuilder& builder, const LocationInfo& locationOfFirstFunctionInstruction);

		// process frame and extract call stack
		bool ExtractCallstackData(ILogOutput& log, CallStackBuilder& builder, const RawTraceFrame& frame, const uint32_t contextSeq);

		// given an instruction extract memory write data if it was a memory writing instruction
		bool ExtractMemoryWritesFromInstructions(ILogOutput& log, const RawTraceFrame& frame);

		//---

		// extract built code trace pages
		void EmitCodeTracePages(const CodeTraceBuilder& codeTraceBuilder, uint32& outFirstCodePage, uint32& outNumCodePages);

		// emit memory write access
		void EmitMemoryTracePages();
	};

} // trace