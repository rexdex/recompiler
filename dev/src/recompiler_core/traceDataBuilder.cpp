#include "build.h"
#include "traceDataBuilder.h"
#include "decodingContext.h"
#include "decodingMemoryMap.h"
#include "decodingInstruction.h"
#include "decodingInstructionInfo.h"
#include <algorithm>

#pragma optimize("",off)

namespace trace
{

	DataBuilder::DataBuilder(const RawTraceReader& rawTrace, const TDecodingContextQuery& decodingContextQuery)
		: m_rawTrace(&rawTrace)
		, m_decodingContextQueryFunc(decodingContextQuery)
	{
		m_blob.push_back(0); // make sure the blob offset 0 will mean "invalid"
		m_callFrames.push_back(CallFrame());
	}

	void DataBuilder::StartContext(ILogOutput& log, const uint32 writerId, const uint32 threadId, const uint64 ip, const TraceFrameID seq, const char* name)
	{
		// create context entry
		auto& context = m_contexts.AllocAt(writerId);
		context.m_id = writerId;
		context.m_threadId = threadId;
		context.m_first.m_contextId = writerId;
		context.m_first.m_contextSeq = 0;
		context.m_first.m_seq = seq;
		context.m_first.m_ip = ip;
		context.m_first.m_time = 0;

		// set context name
		strcpy_s(context.m_name, name);

		// determine type
		if (threadId == 0)
			context.m_type = ContextType::IRQ;
		else if (threadId == 1)
			context.m_type = ContextType::APC;
		else
			context.m_type = ContextType::Thread;

		// create the compression context
		m_deltaContextData.AllocAt(writerId) = new DeltaContext();

		// create the call frame
		CallFrame callFrame;
		callFrame.m_enterLocation = context.m_first;
		callFrame.m_parentFrame = 0;
		const auto callEntryIndex = m_callFrames.push_back(callFrame);
		context.m_rootCallFrame = (uint32_t)callEntryIndex;

		// create the call stack context
		m_callstackBuilders.AllocAt(writerId) = new CallStackBuilder((uint32_t)callEntryIndex);

		// create the context builder
		m_codeTraceBuilders.AllocAt(writerId) = new CodeTraceBuilder();
	}

	void DataBuilder::EndContext(ILogOutput& log, const uint32 writerId, const uint64 ip, const TraceFrameID seq, const uint32 numFrames)
	{
		// end context
		auto& context = m_contexts[writerId];
		context.m_last.m_contextId = writerId;
		context.m_last.m_contextSeq = numFrames;
		context.m_last.m_seq = seq;
		context.m_last.m_ip = ip;
		context.m_last.m_time = 0;

		// end call stack by forcibly finishing all open functions
		auto* callstackBuilder = m_callstackBuilders[writerId];
		for (auto callEntryId : callstackBuilder->m_callFrames)
		{
			auto& entry = m_callFrames[callEntryId];
			entry.m_leaveLocation = context.m_last;
		}
		callstackBuilder->m_callFrames.clear();

		// emit the code trace data
		auto* codeTraceBuilder = m_codeTraceBuilders[writerId];
		if (codeTraceBuilder)
			EmitCodeTracePages(*codeTraceBuilder, context.m_firstCodePage, context.m_numCodePages);
	}

	void DataBuilder::ConsumeFrame(ILogOutput& log, const uint32 writerId, const TraceFrameID seq, const RawTraceFrame& frame)
	{
		// update context
		auto& context = m_contexts[writerId];

		// get the compression context
		auto* deltaContext = m_deltaContextData[writerId];

		// get the offset to data
		const auto dataOffset = m_blob.size();

		// write the blob header
		DataFile::BlobInfo blobInfo;
		blobInfo.m_ip = frame.m_ip;
		blobInfo.m_localSeq = deltaContext->m_localSeq;
		blobInfo.m_time = frame.m_timeStamp;
		WriteToBlob(&blobInfo, sizeof(blobInfo));

		// write the delta compressed data
		TraceFrameID baseFrameId = INVALID_TRACE_FRAME_ID;
		DeltaCompress(*deltaContext, frame, baseFrameId);

		// define entry
		auto& entry = m_entries.AllocAt(seq);
		entry.m_base = baseFrameId;
		entry.m_prevThread = deltaContext->m_prevSeq;
		entry.m_nextThread = INVALID_TRACE_FRAME_ID;
		entry.m_type = (uint8_t)FrameType::CpuInstruction;
		entry.m_offset = dataOffset;
		entry.m_context = writerId;

		// update the delta context
		if (deltaContext->m_prevSeq != INVALID_TRACE_FRAME_ID)
		{
			auto& prevEntry = m_entries[deltaContext->m_prevSeq];
			prevEntry.m_nextThread = seq;
		}

		// extract call structure
		auto* callstackBuilder = m_callstackBuilders[writerId];
		ExtractCallstackData(log, *callstackBuilder, frame, deltaContext->m_localSeq);

		// extract code horizontal trace
		auto* codeTraceBuilder = m_codeTraceBuilders[writerId];
		codeTraceBuilder->RegisterAddress(frame);

		// update local sequence number
		deltaContext->m_prevSeq = seq;
		deltaContext->m_localSeq += 1;
	}

	//--

	DataBuilder::DeltaContext::DeltaContext()
		: m_localSeq(0)
		, m_prevSeq(INVALID_TRACE_FRAME_ID)
	{}

	void DataBuilder::DeltaContext::RetireExpiredReferences()
	{
		while (!m_references.empty())
		{
			// still good ?
			if (m_localSeq < m_references.back().m_retireSeq)
				break;

			// remove the reference
			m_references.pop_back();
		}
	}

	void DataBuilder::WriteToBlob(const void* data, const uint32_t size)
	{
		const auto* writePtr = (const uint8_t*)data;
		for (uint32 i = 0; i < size; ++i)
			m_blob.push_back(writePtr[i]);
	}

	uint32_t DataBuilder::DeltaWrite(const uint8_t* referenceData, const uint8_t* currentData)
	{
		const auto pos = m_blob.size();

		uint8_t zeroReference[32];
		memset(zeroReference, 0, sizeof(zeroReference));

		uint16_t registersToSaveIndices[512];
		uint16_t numRegistersToSaveIndices = 0;

		// compare register by register, collect list of registers to save
		const auto numRegs = m_rawTrace->GetRegisters().size();
		for (uint32_t i = 0; i < numRegs; ++i)
		{
			const auto& regInfo = m_rawTrace->GetRegisters()[i];

			// compare the data against the reference
			if (nullptr != referenceData)
			{
				if (0 == memcmp(referenceData + regInfo.m_dataOffset, currentData + regInfo.m_dataOffset, regInfo.m_dataSize))
					continue; // register data is up to date
			}
			else
			{
				if (0 == memcmp(zeroReference, currentData + regInfo.m_dataOffset, regInfo.m_dataSize))
					continue; // register data is zeroed
			}

			// store in list
			registersToSaveIndices[numRegistersToSaveIndices] = i;
			numRegistersToSaveIndices += 1;
		}

		// save the number of registers
		WriteToBlob<uint8_t>((uint8_t)numRegistersToSaveIndices);

		// save the registers
		for (uint32_t i = 0; i < numRegistersToSaveIndices; ++i)
		{
			// save the register index
			const auto regIndex = registersToSaveIndices[i];
			WriteToBlob<uint8_t>((uint8_t)regIndex);

			// write register data
			const auto& regInfo = m_rawTrace->GetRegisters()[regIndex];
			const auto* regData = currentData + regInfo.m_dataOffset;
			WriteToBlob(regData, regInfo.m_dataSize);
		}

		// return number of bytes written to the data blob
		return (uint32_t)(m_blob.size() - pos);
	}

	void DataBuilder::DeltaCompress(DeltaContext& ctx, const RawTraceFrame& frame, TraceFrameID& outRefSeq)
	{
		// retire all references that are no longer useful
		ctx.RetireExpiredReferences();

		// get the reference data for the compression
		// this may be empty if there are no references on the stack, in this case we use zeros as reference
		outRefSeq = ctx.m_references.empty() ? INVALID_TRACE_FRAME_ID : ctx.m_references.back().m_seq;
		const uint8_t* refData = ctx.m_references.empty() ? nullptr : (const uint8_t*)ctx.m_references.back().m_data.data();

		// store the difference as blob data
		DeltaWrite(refData, (const uint8_t*)frame.m_data.data());

		// add a reference
		if (ctx.m_references.empty())
		{ 
			DeltaReference refData;
			refData.m_data = frame.m_data;
			refData.m_seq = frame.m_seq;
			refData.m_localSeq = ctx.m_localSeq;
			refData.m_retireSeq = ctx.m_localSeq + 1024;
			refData.m_descendSeq = ctx.m_localSeq + 64;
			ctx.m_references.push_back(refData);
		}
	}

	//--

	void DataBuilder::ReturnFromFunction(ILogOutput& log, CallStackBuilder& builder, const LocationInfo& locationOfReturnInstruction)
	{
		if (builder.m_callFrames.size() == 1)
		{
			log.Error("Trace: Return from function found at 0x%08llX (#%llu) but there's nowhere to return to. Ignoring.", locationOfReturnInstruction.m_ip, locationOfReturnInstruction.m_seq);
			return;
		}

		/*log.Log("Trace: CallStack[%u->%u]: Returning from function at 0x%08llX (#%llu)",
			builder.m_callFrames.size(), builder.m_callFrames.size()-1,
			locationOfReturnInstruction.m_ip, locationOfReturnInstruction.m_seq);*/

		auto& topFrame = m_callFrames[builder.m_callFrames.back()];
		topFrame.m_leaveLocation = locationOfReturnInstruction;;
		builder.m_callFrames.pop_back();
		builder.m_lastChildFrames.pop_back();
	}

	void DataBuilder::EnterToFunction(ILogOutput& log, CallStackBuilder& builder, const LocationInfo& locationOfFirstFunctionInstruction)
	{
		auto topFrameIndex = builder.m_callFrames.size() - 1;
		auto topFrameEntryIndex = builder.m_callFrames[topFrameIndex];
		auto& topFrame = m_callFrames[topFrameEntryIndex];

		// create the call frame
		CallFrame callFrame;
		callFrame.m_enterLocation = locationOfFirstFunctionInstruction;
		callFrame.m_parentFrame = topFrameEntryIndex;
		const auto callEntryIndex = m_callFrames.push_back(callFrame);
		builder.m_callFrames.push_back((uint32_t)callEntryIndex);
		builder.m_lastChildFrames.push_back(0);

		/*log.Log("Trace: CallStack[%u->%u]: Entering function at 0x%08llX (#%llu)", 
			builder.m_callFrames.size()-1, builder.m_callFrames.size(),
			locationOfFirstFunctionInstruction.m_ip, locationOfFirstFunctionInstruction.m_seq);*/

		// link as a children of parent
		if (builder.m_lastChildFrames[topFrameIndex] == 0)
		{
			topFrame.m_firstChildFrame = (uint32_t)callEntryIndex;
		}
		else
		{
			auto& lastChildFrame = m_callFrames[builder.m_lastChildFrames[topFrameIndex]];
			lastChildFrame.m_nextChildFrame = (uint32_t)callEntryIndex;
		}
		builder.m_lastChildFrames[topFrameIndex] = (uint32_t)callEntryIndex;
	}

	bool DataBuilder::ExtractCallstackData(ILogOutput& log, CallStackBuilder& builder, const RawTraceFrame& frame, const uint32_t contextSeq)
	{
		const auto codeAddress = frame.m_ip;

		// build location info
		LocationInfo locInfo;
		locInfo.m_contextId = frame.m_writerId;
		locInfo.m_contextSeq = contextSeq;
		locInfo.m_ip = frame.m_ip;
		locInfo.m_seq = frame.m_seq;
		locInfo.m_time = frame.m_timeStamp;

		// jump to an import function - sometimes happens
		auto* decodingContext = m_decodingContextQueryFunc(codeAddress);
		if (!decodingContext)
		{
			log.Error("CallStack: Instruction at %08llXh is outside range that can be decoded", codeAddress);
			return false;
		}

		// did the speculated return happened ?
		if (builder.m_speculatedReturnNotTakenAddress != 0)
		{
			// if current address is different than the speculated one for return NOT being taken than we returned from function
			if (codeAddress != builder.m_speculatedReturnNotTakenAddress)
			{
				ReturnFromFunction(log, builder, builder.m_speculatedLocation);
			}

		}
		else if (builder.m_speculatedCallNotTakenAddress != 0)
		{
			// if current address is different than the speculated one for a call than the call WAS taken
			if (codeAddress != builder.m_speculatedCallNotTakenAddress)
			{
				EnterToFunction(log, builder, locInfo);
			}
		}

		// get info about instruction
		auto memInfo = decodingContext->GetMemoryMap().GetMemoryInfo(codeAddress);
		if (memInfo.IsExecutable() && memInfo.GetInstructionFlags().IsImportFunction())
		{
			builder.m_speculatedLocation = LocationInfo();
			builder.m_speculatedCallNotTakenAddress = 0;
			builder.m_speculatedReturnNotTakenAddress = 0;

			ReturnFromFunction(log, builder, locInfo);
			return true;
		}

		// decode instruction
		decoding::Instruction op;
		const auto opSize = decodingContext->DecodeInstruction(ILogOutput::DevNull(), codeAddress, op, false);
		if (!opSize)
		{
			log.Error("CallStack: Instruction at %08llXh is invalid", codeAddress);
			return false;
		}

		// get additional info
		decoding::InstructionExtendedInfo info;
		if (!op.GetExtendedInfo(codeAddress, *decodingContext, info))
		{
			log.Error("CallStack: Instruction at %08llXh has no extended information", codeAddress);
			return false;
		}

		// invalid
		if ((info.m_codeFlags & decoding::InstructionExtendedInfo::eInstructionFlag_Call) &&
			(info.m_codeFlags & decoding::InstructionExtendedInfo::eInstructionFlag_Return))
		{
			log.Error("CallStack: Instruction at %08Xh is both a call and return", codeAddress);
			return false;
		}

		// reset speculation
		builder.m_speculatedLocation = LocationInfo();
		builder.m_speculatedCallNotTakenAddress = 0;
		builder.m_speculatedReturnNotTakenAddress = 0;

		// call or return - we are about to enter or leave a function
		if (info.m_codeFlags & decoding::InstructionExtendedInfo::eInstructionFlag_Call)
		{
			// if we don't take the call the new code address will be the current one + the size of the instruction
			builder.m_speculatedCallNotTakenAddress = codeAddress + opSize;
		}
		else if (info.m_codeFlags & decoding::InstructionExtendedInfo::eInstructionFlag_Return)
		{
			// if we don't take the return the new code address will be the current one + the size of the instruction
			builder.m_speculatedReturnNotTakenAddress = codeAddress + opSize;
			builder.m_speculatedLocation = locInfo;
		}

		// valid
		return true;
	}

	void DataBuilder::EmitCodeTracePages(const CodeTraceBuilder& codeTraceBuilder, uint32& outFirstCodePage, uint32& outNumCodePages)
	{
		// get pages from map, we need sorted pages later
		std::vector<const CodeTraceBuilderPage*> pages;
		for (auto it : codeTraceBuilder.m_pages)
			pages.push_back(it.second);

		// sort pages by base address
		std::sort(pages.begin(), pages.end(), [](const CodeTraceBuilderPage* a, const CodeTraceBuilderPage* b) { return a->m_baseMemoryAddress < b->m_baseMemoryAddress; });

		// pack pages
		outFirstCodePage = (uint32_t)m_codeTracePages.size();
		for (const auto* page : pages)
		{
			// initialize page
			CodeTracePage tracePage;
			memset(&tracePage, 0, sizeof(tracePage));
			tracePage.m_baseAddress = page->m_baseMemoryAddress;

			// export address chains
			for (uint32 i = 0; i < CodeTracePage::NUM_ADDRESSES_PER_PAGE; ++i)
			{
				const auto& seqChain = page->m_seqChain[i];
				if (!seqChain.empty())
				{
					// write to blob (as everything else :P)
					tracePage.m_dataOffsets[i] = m_blob.size();

					// write count
					WriteToBlob<uint32_t>((uint32_t)seqChain.size());

					// write elements
					auto sortedSeqChain = seqChain;
					std::sort(sortedSeqChain.begin(), sortedSeqChain.end());
					WriteToBlob(sortedSeqChain.data(), (uint32_t)(sortedSeqChain.size() * sizeof(sortedSeqChain[0])));
				}
			}

			// write page data
			m_codeTracePages.push_back(tracePage);
		}
		outNumCodePages = (uint32_t) m_codeTracePages.size() - outFirstCodePage;
	}	

	//--

} // trace

