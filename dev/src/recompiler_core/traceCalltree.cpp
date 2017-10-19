#include "build.h"

#if 0
#include "traceData.h"
#include "traceCalltree.h"
#include "internalFile.h"
#include "decodingContext.h"
#include "decodingMemoryMap.h"
#include "decodingNameMap.h"
#include "decodingInstruction.h"
#include "decodingInstructionInfo.h"
#include "platformDefinition.h"
#include "platformCPU.h"

namespace trace
{

	//---

	static inline IBinaryFileWriter &operator<<(IBinaryFileWriter& file, const CallTree::SFrame& frame)
	{
		file << frame.m_functionStart;
		file << frame.m_traceEnter;
		file << frame.m_traceLeave;
		file << frame.m_firstChildFrame;
		file << frame.m_nextChildFrame;
		file << frame.m_parentFrame;
		return file;
	}

	static inline IBinaryFileReader &operator >> (IBinaryFileReader& file, CallTree::SFrame& frame)
	{
		file >> frame.m_functionStart;
		file >> frame.m_traceEnter;
		file >> frame.m_traceLeave;
		file >> frame.m_firstChildFrame;
		file >> frame.m_nextChildFrame;
		file >> frame.m_parentFrame;
		return file;
	}

	//---

	CallTree::CallTree()
	{
	}

	CallTree::~CallTree()
	{
	}

	const uint32 CallTree::GetNumFrames() const
	{
		return (uint32)m_frames.size();
	}

	const uint32 CallTree::GetRootFrame() const
	{
		return 0;
	}

	const CallTree::SFrame& CallTree::GetFrame(const uint32 index) const
	{
		return m_frames[index];
	}

	const uint32 CallTree::WalkStack(const uint32 startIndex, IStackWalker& walker, const uint32 recursionDepth) const
	{
		uint32 ret = 0;

		if (startIndex < m_frames.size())
		{
			const SFrame& frame = m_frames[startIndex];

			// enter function
			walker.OnEnterFunction(frame.m_functionStart, frame.m_traceEnter);

			// recurse to child function calls
			if (recursionDepth > 0)
			{
				uint32 child = frame.m_firstChildFrame;
				while (child)
				{
					ret += WalkStack(child, walker, recursionDepth - 1);
					child = m_frames[child].m_nextChildFrame;
				}
			}

			// leave function
			walker.OnLeaveFunction(frame.m_functionStart, frame.m_traceLeave);
			ret += 1;
		}

		// return total number of functions visited
		return ret;
	}

	const bool CallTree::Save(class ILogOutput& log, const wchar_t* filePath)
	{
		// open file
		auto writer = CreateFileWriter(filePath, true);
		if (!writer.get())
			return false;

		// write data
		{
			FileChunk chunk(*writer.get(), "TraceCallstack");
			*writer << m_frames;
		}

		// saved
		return true;
	}

	CallTree* CallTree::OpenTraceCallstack(class ILogOutput& log, const wchar_t* filePath)
	{
		// open file
		auto reader = CreateFileReader(filePath);
		if (!reader.get())
		{
			log.Error("Callstack: Unable to open file %ls", filePath);
			return nullptr;
		}

		std::auto_ptr< CallTree > ret(new CallTree());

		// load data
		FileChunk chunk(*reader.get(), "TraceCallstack");
		if (!chunk)
		{
			log.Error("Callstack: Unable to load callstack from file %ls", filePath);
			return nullptr;
		}

		// load frames
		*reader >> ret->m_frames;

		// done
		return ret.release();
	}

	bool CallTree::WalkStackFrames(class ILogOutput& log, uint32 &entryIndex, decoding::Context& context, trace::DataReader& reader, uint32& outEntryIndex, TFrames& frames, const uint32 expectedReturnAddress, IStackBuilderProgress& progress)
	{
		// allocate frame
		outEntryIndex = (uint32)frames.size();
		frames.push_back(SFrame());

		// get current code address
		const trace::DataFrame& startFrame = reader.GetFrame(entryIndex);
		const uint32 startCodeAddress = startFrame.GetAddress();

		// setup entry point
		frames[outEntryIndex].m_traceEnter = entryIndex;
		frames[outEntryIndex].m_functionStart = startCodeAddress;
		frames[outEntryIndex].m_firstChildFrame = 0;
		uint32 lastChildFrame = 0;

		// imported function
		auto startInfo = context.GetMemoryMap().GetMemoryInfo(startCodeAddress);
		if (startInfo.IsExecutable() && startInfo.GetInstructionFlags().IsImportFunction())
		{
			frames[outEntryIndex].m_traceLeave = entryIndex;
			entryIndex += 1;
			return true;
		}

		// update stack progress
		progress.OnProgress(entryIndex, reader.GetNumFrames());

		// walk the trace info
		while (entryIndex < reader.GetNumFrames())
		{
			const uint32 thisEntryIndex = entryIndex++;
			const trace::DataFrame& frame = reader.GetFrame(thisEntryIndex);
			const uint32 codeAddress = frame.GetAddress();

			// validate trace data
			if (entryIndex + 3 < reader.GetNumFrames())
			{
				if (reader.GetFrame(entryIndex).GetAddress() == codeAddress)
				{
					log.Error("CallStack: Corrupted trace data at %08Xh, TR=#%05u", codeAddress, thisEntryIndex);
					return false;
				}
			}

			// jump to an import function - sometimes happens
			auto memInfo = context.GetMemoryMap().GetMemoryInfo(codeAddress);
			if (memInfo.IsExecutable() && memInfo.GetInstructionFlags().IsImportFunction())
			{
				const char* functionName = context.GetNameMap().GetName(codeAddress);
				//log.Warn( "Callstack: Jump to an import function %s, TR=#%05d", functionName, thisEntryIndex );
				frames[outEntryIndex].m_traceLeave = entryIndex;
				return true; // we always exit
			}

			// decode instruction
			decoding::Instruction op;
			if (!context.DecodeInstruction(ILogOutput::DevNull(), codeAddress, op, false))
			{
				log.Error("CallStack: Instruction at %08Xh, TR=#%05u is invalid", codeAddress, thisEntryIndex);
				return false;
			}

			// get additional info
			decoding::InstructionExtendedInfo info;
			if (!op.GetExtendedInfo(codeAddress, context, info))
			{
				log.Error("CallStack: Instruction at %08Xh, TR=#%05u has no extended information", codeAddress, thisEntryIndex);
				return false;
			}

			// invalid
			if ((info.m_codeFlags & decoding::InstructionExtendedInfo::eInstructionFlag_Call) &&
				(info.m_codeFlags & decoding::InstructionExtendedInfo::eInstructionFlag_Return))
			{
				log.Error("CallStack: Instruction at %08Xh is both a call and return", codeAddress);
				return false;
			}

			// conditional instruction
			if (info.m_codeFlags & decoding::InstructionExtendedInfo::eInstructionFlag_Conditional)
			{
				// is taken ?
				const uint32 nextCodeAddress = reader.GetFrame(entryIndex).GetAddress();
				if (nextCodeAddress == codeAddress + 4)
				{
					// call/ret was not executed
					continue;
				}
			}

			// call
			if (info.m_codeFlags & decoding::InstructionExtendedInfo::eInstructionFlag_Call)
			{
				// get the address to call
				uint64 callAddress;
				if (!info.ComputeBranchTargetAddress(frame, callAddress))
				{
					log.Error("CallStack: Failed to compute call addresss at %08Xh, TR=#%05u", codeAddress, thisEntryIndex);
					return false;
				}

				// we expect to return to our next instruction
				const uint32 nextCodeAddr = codeAddress + 4;

				// recurse
				uint32 createdFrame = 0;
				const bool ret = WalkStackFrames(log, entryIndex, context, reader, createdFrame, frames, nextCodeAddr, progress);

				// link
				if (lastChildFrame)
				{
					frames[createdFrame].m_parentFrame = outEntryIndex;
					frames[createdFrame].m_nextChildFrame = 0;

					frames[lastChildFrame].m_nextChildFrame = createdFrame;
					lastChildFrame = createdFrame;
				}
				else
				{
					frames[createdFrame].m_parentFrame = outEntryIndex;
					frames[createdFrame].m_nextChildFrame = 0;

					frames[outEntryIndex].m_firstChildFrame = createdFrame;
					lastChildFrame = createdFrame;
				}

				if (!ret)
				{
					log.Error("CallStack: Error building callstack at %08Xh TR=#%05u",
						frames[createdFrame].m_functionStart, frames[createdFrame].m_traceEnter);

					frames[outEntryIndex].m_traceLeave = entryIndex - 1;
					return false;
				}

				if (entryIndex + 1 < reader.GetNumFrames())
				{
					// did we return to the proper address ?
					const uint32 returnedAddress = reader.GetFrame(entryIndex).GetAddress();
					if (nextCodeAddr && (nextCodeAddr != returnedAddress))
					{
						log.Error("CallStack: Function call at %08Xh returned with address %08Xh instead of %08Xh, TR=#%05u",
							frames[createdFrame].m_functionStart, returnedAddress, nextCodeAddr, entryIndex);
						log.Error("CallStack: In function %08Xh TR=#%05u",
							frames[createdFrame].m_functionStart, frames[createdFrame].m_traceEnter);

						frames[outEntryIndex].m_traceLeave = entryIndex - 1;
						return false;
					}
				}
			}

			// return
			else if (info.m_codeFlags & decoding::InstructionExtendedInfo::eInstructionFlag_Return)
			{
				break;
			}
		}

		// setup leave address
		frames[outEntryIndex].m_traceLeave = entryIndex - 1;
		return true;
	}

	CallTree* CallTree::BuildTraceCalltack(class ILogOutput& log, decoding::Context& context, const Data& data, const uint32 startEntry, IStackBuilderProgress& progress)
	{
		std::auto_ptr<trace::DataReader> tempReader(data.CreateReader());
		if (!tempReader.get())
		{
			log.Error("Callstack: failed to create trace data reader");
			return false;
		}

		// stack pointer
		const auto* cpu = context.GetPlatform()->GetCPU(0); // HACK!
		const platform::CPURegister* stackReg = cpu->FindRegister("R1");

		// read current frame
		const trace::DataFrame& startFrame = tempReader->GetFrame(startEntry);
		const uint32 codeAddress = startFrame.GetAddress();

		// output data
		std::auto_ptr< CallTree > ret(new CallTree());
		ret->m_frames.reserve(1 << 20); // 1M frames

										// walk the stack
		uint32 walkingEntryIndex = startEntry;
		uint32 rootEntryIndex = 0;
		if (!WalkStackFrames(log, walkingEntryIndex, context, *tempReader.get(), rootEntryIndex, ret->m_frames, 0, progress))
		{
			log.Error("Callstack: There was an unrecoverable error in trace data - callstack may be corrupted");
			//return nullptr;
		}

		// return created data
		return ret.release();
	}

} // trace

#endif