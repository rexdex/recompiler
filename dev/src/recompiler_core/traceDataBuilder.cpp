#include "build.h"
#include "traceDataBuilder.h"

//#pragma optimize("",off)

namespace trace
{

	DataBuilder::DataBuilder(const RawTraceReader& rawTrace)
		: m_rawTrace(&rawTrace)
	{
		m_blob.push_back(0); // make sure the blob offset 0 will mean "invalid"
	}

	void DataBuilder::StartContext(const uint32 writerId, const uint32 threadId, const uint64 ip, const TraceFrameID seq, const char* name)
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
	}

	void DataBuilder::EndContext(const uint32 writerId, const uint64 ip, const TraceFrameID seq, const uint32 numFrames)
	{
		auto& context = m_contexts[writerId];
		context.m_last.m_contextId = writerId;
		context.m_last.m_contextSeq = numFrames;
		context.m_last.m_seq = seq;
		context.m_last.m_ip = ip;
		context.m_last.m_time = 0;
	}

	void DataBuilder::ConsumeFrame(const uint32 writerId, const TraceFrameID seq, const RawTraceFrame& frame)
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

} // trace

