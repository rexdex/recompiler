#include "build.h"
#include "traceDataBuilder.h"
#include "decodingContext.h"
#include "decodingMemoryMap.h"
#include "decodingInstruction.h"
#include "decodingInstructionInfo.h"
#include <algorithm>
#include "platformCPU.h"
#include "traceUtils.h"


namespace trace
{

	DataBuilder::DataBuilder(const RawTraceReader& rawTrace, const TDecodingContextQuery& decodingContextQuery)
		: m_rawTrace(&rawTrace)
		, m_decodingContextQueryFunc(decodingContextQuery)
		, m_memoryTraceBuilder(new MemoryTraceBuilder())
		, m_firstSeq(INVALID_TRACE_FRAME_ID)
		, m_lastSeq(INVALID_TRACE_FRAME_ID)
	{
		m_blob.push_back(0); // make sure the blob offset 0 will mean "invalid"
		m_callFrames.push_back(CallFrame());
	}

	void DataBuilder::FlushData()
	{
		EmitMemoryTracePages();
	}

	static inline bool ComparePtr(const uint8* a, const uint8* b, const uint32 size)
	{
		const auto end = a + size;
		while (a < end)
			if (*a++ != *b++)
				return false;

		return true;
	}

	static inline void CopyPtr(void* writePtr, const void* readPtr, uint32 size)
	{
		auto* writerPtrX = (uint8_t*)writePtr;
		auto* readPtrX = (const uint8_t*)readPtr;
		const auto end = readPtrX + size;
		while (readPtrX < end)
			*writerPtrX++ = *readPtrX++;
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
		context.m_rootCallFrame = 0;

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
		if (callstackBuilder != nullptr)
		{
			context.m_rootCallFrame = callstackBuilder->m_rootCallFrame;

			for (auto callEntryId : callstackBuilder->m_callFrames)
			{
				auto& entry = m_callFrames[callEntryId];
				entry.m_leaveLocation = context.m_last;
			}
			callstackBuilder->m_callFrames.clear();
		}

		// emit the code trace data
		auto* codeTraceBuilder = m_codeTraceBuilders[writerId];
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

		// update ranges
		if (m_firstSeq == INVALID_TRACE_FRAME_ID)
		{
			m_firstSeq = seq;
			m_lastSeq = seq;
		}
		else
		{ 
			m_lastSeq = std::max(seq, m_lastSeq);
		}

		// write the blob header
		DataFile::BlobInfo blobInfo;
		blobInfo.m_ip = frame.m_ip;
		blobInfo.m_localSeq = deltaContext->m_localSeq;
		blobInfo.m_time = frame.m_timeStamp;
		const auto blobOffset = WriteToBlob(&blobInfo, sizeof(blobInfo));

		// define entry
		auto& entry = m_entries.AllocAt(seq);
		entry.m_base = INVALID_TRACE_FRAME_ID;
		entry.m_prevThread = deltaContext->m_prevSeq;
		entry.m_nextThread = INVALID_TRACE_FRAME_ID;
		entry.m_offset = dataOffset;
		entry.m_context = writerId;
		entry.m_type = frame.m_type;

		// update the delta context
		if (deltaContext->m_prevSeq != INVALID_TRACE_FRAME_ID)
		{
			auto& prevEntry = m_entries[deltaContext->m_prevSeq];
			prevEntry.m_nextThread = seq;
			deltaContext->m_prevSeq = INVALID_TRACE_FRAME_ID;
		}

		// cpu frame
		if (frame.m_type == (uint8)FrameType::CpuInstruction)
		{
			// update previous memory writes
			for (auto* ptr : deltaContext->m_unresolvedIPAddresses)
				*ptr = blobInfo.m_ip;
			deltaContext->m_unresolvedIPAddresses.clear();
			deltaContext->m_lastValidResolvedIP = frame.m_ip;

			// write the delta compressed data
			TraceFrameID baseFrameId = INVALID_TRACE_FRAME_ID;
			DeltaCompress(*deltaContext, frame, baseFrameId);

			// set entry type
			entry.m_base = baseFrameId;

			// create callstack builder on first instruction
			if (context.m_rootCallFrame == 0)
			{
				// create the call frame
				CallFrame callFrame;
				callFrame.m_enterLocation.m_contextId = context.m_id;
				callFrame.m_enterLocation.m_contextSeq = blobInfo.m_localSeq;
				callFrame.m_enterLocation.m_time = frame.m_timeStamp;
				callFrame.m_enterLocation.m_ip = frame.m_ip;
				callFrame.m_enterLocation.m_seq = frame.m_seq;
				callFrame.m_parentFrame = 0;
				const auto callEntryIndex = m_callFrames.push_back(callFrame);
				context.m_rootCallFrame = (uint32_t)callEntryIndex;

				// create the call stack context
				m_callstackBuilders.AllocAt(writerId) = new CallStackBuilder((uint32_t)callEntryIndex);
			}

			// extract call structure
			auto* callstackBuilder = m_callstackBuilders[writerId];
			ExtractCallstackData(log, *callstackBuilder, frame, deltaContext->m_localSeq);

			// extract code horizontal trace
			auto* codeTraceBuilder = m_codeTraceBuilders[writerId];
			codeTraceBuilder->RegisterAddress(frame);

			// extract memory writes from memory writing instructions :)
			ExtractMemoryWritesFromInstructions(log, frame);
		}
		// external memory write
		else if (frame.m_type == (uint8)FrameType::ExternalMemoryWrite)
		{
			// try to assign a matching IP to the memory write
			auto* writtenBlobInfo = (DataFile::BlobInfo*)&m_blob[blobOffset];
			if (deltaContext->m_lastValidResolvedIP)
			{
				// update with the IP of the previous instruction (this is more useful)
				writtenBlobInfo->m_ip = deltaContext->m_lastValidResolvedIP;
			}
			else
			{
				deltaContext->m_unresolvedIPAddresses.push_back(&writtenBlobInfo->m_ip);
			}

			// write the text length
			const auto stringLength = (uint8_t)(frame.m_desc.length());
			WriteToBlob(stringLength);
			WriteToBlob(frame.m_desc.c_str(), stringLength);

			// write the address
			WriteToBlob(frame.m_address);
			WriteToBlob((uint32)frame.m_data.size());

			// create memory access info
			for (uint32_t i = 0; i < frame.m_data.size(); ++i)
				m_memoryTraceBuilder->RegisterWrite(seq, frame.m_address + i, frame.m_data[i]);
		}

		// update local sequence number
		deltaContext->m_prevSeq = seq;
		deltaContext->m_localSeq += 1;
	}

	//--

	DataBuilder::DeltaContext::DeltaContext()
		: m_localSeq(0)
		, m_prevSeq(INVALID_TRACE_FRAME_ID)
		, m_lastValidResolvedIP(0)
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

	uint64_t DataBuilder::WriteToBlob(const void* data, const uint32_t size)
	{
		uint64_t offset = m_blob.size();
		const auto* writePtr = (const uint8_t*)data;
		for (uint32 i = 0; i < size; ++i)
			m_blob.push_back(writePtr[i]);
		return offset;
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
		const auto* regInfo = &m_rawTrace->GetRegisters()[0];
		for (uint32_t i = 0; i < numRegs; ++i, ++regInfo)
		{
			// compare the data against the reference
			if (nullptr != referenceData)
			{
				if (ComparePtr(referenceData + regInfo->m_dataOffset, currentData + regInfo->m_dataOffset, regInfo->m_dataSize))
					continue; // register data is up to date
			}
			else
			{
				if (ComparePtr(zeroReference, currentData + regInfo->m_dataOffset, regInfo->m_dataSize))
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
			log.Error("Trace: Return from function found at 0x%08llX (#%llu) but there's nowhere to return to. Creating fake one.", locationOfReturnInstruction.m_ip, locationOfReturnInstruction.m_seq);

			auto curTopFrameIndex = builder.m_callFrames.front();
			auto& curTopFrame = m_callFrames[curTopFrameIndex];

			// create a new call frame
			CallFrame newTopFrame;
			newTopFrame.m_enterLocation = curTopFrame.m_enterLocation;
			newTopFrame.m_functionStart = curTopFrame.m_functionStart;
			newTopFrame.m_parentFrame = 0;
			newTopFrame.m_nextChildFrame = 0;
			newTopFrame.m_firstChildFrame = curTopFrameIndex;
			const auto newToFrameIndex = m_callFrames.push_back(newTopFrame);

			// link
			curTopFrame.m_parentFrame = (uint32_t)newToFrameIndex;
			builder.m_callFrames.insert(builder.m_callFrames.begin(), (uint32_t)newToFrameIndex);
			builder.m_lastChildFrames.insert(builder.m_lastChildFrames.begin(), (uint32_t)curTopFrameIndex);

			// make the new top frame
			builder.m_rootCallFrame = (uint32_t)newToFrameIndex;
		}

		log.Log("Trace: CallStack[%u->%u]: Returning from function at 0x%08llX (#%llu)",
			builder.m_callFrames.size(), builder.m_callFrames.size()-1,
			locationOfReturnInstruction.m_ip, locationOfReturnInstruction.m_seq);

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

		log.Log("Trace: CallStack[%u->%u]: Entering function at 0x%08llX (#%llu)", 
			builder.m_callFrames.size()-1, builder.m_callFrames.size(),
			locationOfFirstFunctionInstruction.m_ip, locationOfFirstFunctionInstruction.m_seq);

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

		// decode instruction
		decoding::Instruction op;
		const auto opSize = decodingContext->DecodeInstruction(ILogOutput::DevNull(), codeAddress, op, false);
		if (!opSize)
		{
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

	inline const void Swap2(uint8* ptr)
	{
		std::swap(ptr[0], ptr[1]);
	}

	inline const void Swap4(uint8* ptr)
	{
		std::swap(ptr[0], ptr[3]);
		std::swap(ptr[1], ptr[2]);
	}

	inline const void Swap8(uint8* ptr)
	{
		std::swap(ptr[0], ptr[7]);
		std::swap(ptr[1], ptr[6]);
		std::swap(ptr[2], ptr[5]);
		std::swap(ptr[3], ptr[4]);
	}

#pragma optimize("",off)
	bool DataBuilder::ExtractMemoryWritesFromInstructions(ILogOutput& log, const RawTraceFrame& frame)
	{
		const auto seq = frame.m_seq;
		const auto codeAddress = frame.m_ip;

		// jump to an import function - sometimes happens
		auto* decodingContext = m_decodingContextQueryFunc(codeAddress);
		if (!decodingContext)
		{
			log.Error("MemoryTrace: Instruction at %08llXh is outside range that can be decoded", codeAddress);
			return false;
		}

		// decode instruction
		decoding::Instruction op;
		const auto opSize = decodingContext->DecodeInstruction(ILogOutput::DevNull(), codeAddress, op, false);
		if (!opSize)
		{
			log.Error("MemoryTrace: Instruction at %08llXh is invalid", codeAddress);
			return false;
		}

		// get additional info
		decoding::InstructionExtendedInfo info;
		if (!op.GetExtendedInfo(codeAddress, *decodingContext, info))
		{
			log.Error("MemoryTrace: Instruction at %08llXh has no extended information", codeAddress);
			return false;
		}

		// we are interested only in memory-writing instructions
		if (0 != (info.m_memoryFlags & decoding::InstructionExtendedInfo::eMemoryFlags_Write))
		{
			// compute where the memory was written
			const auto dataFetch = [&frame](const platform::CPURegister* reg, void* outData)
			{
				const auto ofs = reg->GetTraceDataOffset();
				CopyPtr(outData, (char*)frame.m_data.data() + ofs, reg->GetBitSize() / 8);
				return true;
			};

			uint64_t memoryWriteAddress = 0;
			if (info.ComputeMemoryAddress(dataFetch, memoryWriteAddress))
			{
				// get the register that with the value that is being written
				const platform::CPURegister* reg0 = op.GetArg0().m_reg;

				// get the written value
				uint8_t fullData[64];
				dataFetch(reg0, fullData);

				// HACK FOR DOUBLES
				if (reg0->GetType() == platform::CPURegisterType::FloatingPoint && reg0->GetBitSize() == 64 && info.m_memorySize == 4)
				{
					const auto dbl = *(const double*)&fullData;
					*(float*)&fullData = (float)dbl;
				}

				// Endianess hack
				switch (reg0->GetBitSize()/8)
				{
					case 2:
						Swap2(fullData);
						break;

					case 4:
						Swap4(fullData);
						break;

					case 8:
						Swap8(fullData);
						break;

					case 16:
						Swap4(fullData+0);
						Swap4(fullData+4);
						Swap4(fullData+8);
						Swap4(fullData+16);
						break;
				}

				// compute the data to write
				uint8_t writeOffset = 0;
				uint8_t regOffset = 0;
				uint8_t writeCount = info.m_memorySize;

				// adjustments for LVX/RVX writes
				if (info.m_memoryWriteMode == decoding::InstructionExtendedInfo::eMemoryWriteMode_LeftAligned)
				{
					const auto unalignedOffset = (uint8_t)(memoryWriteAddress % info.m_memorySize);

					regOffset = 0;
					writeOffset = unalignedOffset;
					writeCount = info.m_memorySize - writeOffset;
				}
				else if (info.m_memoryWriteMode == decoding::InstructionExtendedInfo::eMemoryWriteMode_RightAligned)
				{
					const auto unalignedOffset = 16 - (uint8_t)(memoryWriteAddress % info.m_memorySize);

					regOffset = unalignedOffset;
					writeOffset = 0;
					writeCount = info.m_memorySize - unalignedOffset;
				}

				// write it into the trace, byte at a time
				for (uint32_t i = 0; i < writeCount; ++i)
					m_memoryTraceBuilder->RegisterWrite(frame.m_seq, memoryWriteAddress + i + writeOffset, fullData[i + regOffset]);
			}
		}

		// no major errors
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

	void DataBuilder::EmitMemoryTracePages()
	{
		// get pages from map, we need sorted pages later
		std::vector<const MemoryTraceBuilderPage*> pages;
		for (auto it : m_memoryTraceBuilder->m_pages)
			pages.push_back(it.second);

		// sort pages by base address
		std::sort(pages.begin(), pages.end(), [](const MemoryTraceBuilderPage* a, const MemoryTraceBuilderPage* b) { return a->m_baseMemoryAddress < b->m_baseMemoryAddress; });

		// pack pages
		for (const auto* page : pages)
		{
			// initialize page
			MemoryTracePage tracePage;
			memset(&tracePage, 0, sizeof(tracePage));
			tracePage.m_baseAddress = page->m_baseMemoryAddress;

			// export address chains
			for (uint32 i = 0; i < MemoryTracePage::NUM_ADDRESSES_PER_PAGE; ++i)
			{
				const auto address = page->m_baseMemoryAddress + i;
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
					for (const auto& entry : sortedSeqChain)
					{
						WriteToBlob(entry.m_seq);
						WriteToBlob(entry.m_data);
					}
				}
			}

			// write page data
			m_memoryTracePages.push_back(tracePage);
		}
	}

	//--

} // trace

