#include "build.h"
#include "projectTraceData.h"
#include "progressDialog.h"
#include "project.h"

#include "../recompiler_core/traceCalltree.h"
#include "../recompiler_core/traceMemoryHistory.h"
#include "../recompiler_core/timemachine.h"
#include "../recompiler_core/decodingEnvironment.h"
#include "../recompiler_core/decodingMemoryMap.h"
#include "../recompiler_core/decodingContext.h"
#include "../recompiler_core/decodingInstructionInfo.h"
#include "../recompiler_core/decodingInstruction.h"
#include "../recompiler_core/platformCPU.h"
#include "../recompiler_core/platformDefinition.h"
#include "../recompiler_core/traceMemoryHistoryBuilder.h"
#include "../recompiler_core/traceUtils.h"
#include "eventDispatcher.h"

namespace tools
{
	ProjectTraceData::ProjectTraceData()
		: m_data(NULL)
		, m_reader(NULL)
		, m_memReader(NULL)
		, m_callstack(NULL)
	{
	}

	ProjectTraceData::~ProjectTraceData()
	{
		if (m_reader)
		{
			delete m_reader;
			m_reader = NULL;
		}

		if (m_memReader)
		{
			delete m_memReader;
			m_memReader = NULL;
		}

		if (m_callstack)
		{
			delete m_callstack;
			m_callstack = NULL;
		}

		if (m_data)
		{
			delete m_data;
			m_data = NULL;
		}
	}

	const uint32 ProjectTraceData::GetCurrentEntryIndex() const
	{
		return m_currentEntry;
	}

	const uint32 ProjectTraceData::GetNumEntries() const
	{
		return m_data->GetNumDataFrames();
	}

	const uint32 ProjectTraceData::GetCurrentAddress() const
	{
		return (uint32)m_currentEntryAddress;
	}

	const trace::DataFrame& ProjectTraceData::GetCurrentFrame() const
	{
		return m_reader->GetFrame(m_currentEntry);
	}

	const trace::DataFrame& ProjectTraceData::GetNextFrame() const
	{
		if (m_currentEntry <= (m_reader->GetNumFrames() - 1))
			return m_reader->GetFrame(m_currentEntry + 1);
		else
			return m_reader->GetFrame(m_currentEntry);
	}

	const trace::Registers& ProjectTraceData::GetRegisters() const
	{
		return m_data->GetRegisters();
	}

	const trace::CallTree* ProjectTraceData::GetCallstack() const
	{
		return m_callstack;
	}

	class timemachine::Trace* ProjectTraceData::CreateTimeMachineTrace(const uint32 traceFrameIndex)
	{
		return nullptr;// timemachine::Trace::CreateTimeMachine(m_project->GetEnv().GetDecodingContext(), m_data, traceFrameIndex);
	}

	bool ProjectTraceData::MoveToEntry(const uint32 entryIndex)
	{
		if (entryIndex < m_data->GetNumDataFrames())
		{
			if (entryIndex != m_currentEntry)
			{
				m_currentEntryAddress = m_reader->GetFrame(entryIndex).GetAddress();
				m_currentEntry = entryIndex;

				EventDispatcher::GetInstance().PostEvent("TraceIndexChanged", entryIndex);
			}

			return true;
		}

		return false;
	}

	bool ProjectTraceData::Reset()
	{
		MoveToEntry(0);
		return true;
	}

	int ProjectTraceData::GetNextBreakEntry() const
	{
		// memory map
		const auto& memMap = *(decoding::MemoryMap*)nullptr;// ::M < em m_project->GetEnv().GetDecodingContext()->GetMemoryMap();

		// scan and look for breakpoints
		uint32 entryIndex = m_currentEntry + 1;
		const uint32 lastEntry = m_data->GetNumDataFrames();
		while (entryIndex < lastEntry)
		{
			// get decoded frame
			const trace::DataFrame& frame = m_reader->GetFrame(entryIndex);
			const uint32 address = frame.GetAddress();

			// get local offset
			if (memMap.GetMemoryInfo(address).HasBreakpoint())
			{
				return entryIndex;
			}

			// advance
			++entryIndex;
		}

		// no breakpoint found
		return -1;
	}

	bool ProjectTraceData::FindBackwardMemoryAccess(const uint32 startEntry, const uint32 memoryAddress, const uint32 size, const uint8 actionMask, uint32& outEntryIndex, uint32& outCodeAddress) const
	{
		// create local reader
		trace::DataReader* tempReader = m_data->CreateReader();
		if (!tempReader)
			return false;

		// end of memory range we are testing
		const uint32 memoryAddressEnd = memoryAddress + size;

		// while not at the end
		uint32 entryIndex = startEntry - 1;
		decoding::Context* context = nullptr;// m_project->GetEnv().GetDecodingContext();
		while (entryIndex > 0)
		{
			// stats		
			//wxTheApp->GetLogWindow().SetTaskProgress(startEntry - entryIndex, startEntry);

			// get decoded frame
			const uint32 thisEntryIndex = entryIndex--;
			const trace::DataFrame& frame = tempReader->GetFrame(thisEntryIndex);
			const uint32 codeAddress = frame.GetAddress();

			// decode instruction
			decoding::Instruction op;
			if (!context->DecodeInstruction(ILogOutput::DevNull(), codeAddress, op, false))
				continue;

			// get additional info
			decoding::InstructionExtendedInfo info;
			if (!op.GetExtendedInfo(codeAddress, *context, info))
				continue;

			// not a memory op
			uint8 memFlags = 0;
			if (info.m_memoryFlags & decoding::InstructionExtendedInfo::eMemoryFlags_Read) memFlags |= 1;
			if (info.m_memoryFlags & decoding::InstructionExtendedInfo::eMemoryFlags_Write) memFlags |= 2;
			if (!(memFlags & actionMask))
				continue;

			// compute memory range for this instruction
			uint64 currentMemoryAddress = 0;
			if (!info.ComputeMemoryAddress(frame, currentMemoryAddress))
				continue;

			// address in range ?
			uint32 currentMemoryAddressEnd = currentMemoryAddress + info.m_memorySize;
			if ((currentMemoryAddressEnd > memoryAddress) && (currentMemoryAddress < memoryAddressEnd))
			{
				{
					char commandName[256];
					char* stream = commandName;
					op.GenerateText(codeAddress, stream, stream + sizeof(commandName));
				//	wxTheApp->GetLogWindow().Log("Trace: Instruction accessing memory: '%s'", commandName);
				}

				outCodeAddress = codeAddress;
				outEntryIndex = thisEntryIndex;
				delete tempReader;
				return true;
			}
		}

		// nothing found
		delete tempReader;
		return false;
	}

	bool ProjectTraceData::FindFirstMemoryAccess(const uint32 startEntry, const uint32 memoryAddress, const uint32 size, const uint8 actionMask, uint32& outEntryIndex, uint32& outCodeAddress) const
	{
		// create local reader
		trace::DataReader* tempReader = m_data->CreateReader();
		if (!tempReader)
			return false;

		// end of memory range we are testing
		const uint32 memoryAddressEnd = memoryAddress + size;

		// while not at the end
		uint32 entryIndex = 0;
		const uint32 lastEntry = m_data->GetNumDataFrames();
		decoding::Context* context = nullptr;//m_project->GetEnv().GetDecodingContext();
		while (entryIndex < startEntry)
		{
			// stats		
			//wxTheApp->GetLogWindow().SetTaskProgress(entryIndex, startEntry);

			// get decoded frame
			const uint32 thisEntryIndex = entryIndex++;
			const trace::DataFrame& frame = tempReader->GetFrame(thisEntryIndex);
			const uint32 codeAddress = frame.GetAddress();

			// decode instruction
			decoding::Instruction op;
			if (!context->DecodeInstruction(ILogOutput::DevNull(), codeAddress, op, false))
				continue;

			// get additional info
			decoding::InstructionExtendedInfo info;
			if (!op.GetExtendedInfo(codeAddress, *context, info))
				continue;

			// not a memory op
			uint8 memFlags = 0;
			if (info.m_memoryFlags & decoding::InstructionExtendedInfo::eMemoryFlags_Read) memFlags |= 1;
			if (info.m_memoryFlags & decoding::InstructionExtendedInfo::eMemoryFlags_Write) memFlags |= 2;
			if (!(memFlags & actionMask))
				continue;

			// compute memory range for this instruction
			uint64 currentMemoryAddress = 0;
			if (!info.ComputeMemoryAddress(frame, currentMemoryAddress))
				continue;

			// address in range ?
			uint32 currentMemoryAddressEnd = currentMemoryAddress + info.m_memorySize;
			if ((currentMemoryAddressEnd > memoryAddress) && (currentMemoryAddress < memoryAddressEnd))
			{
				{
					char commandName[256];
					char* stream = commandName;
					op.GenerateText(codeAddress, stream, stream + sizeof(commandName));
				}

				outCodeAddress = codeAddress;
				outEntryIndex = thisEntryIndex;
				delete tempReader;
				return true;
			}
		}

		// nothing found
		delete tempReader;
		return false;
	}

	bool ProjectTraceData::FindAllMemoryAccess(const uint32 memoryAddress, const uint32 size, const uint8 actionMask, std::vector< ProjectTraceMemoryAccessEntry >& outEntries) const
	{
		//CScopedTask task(wxTheApp->GetLogWindow(), "Scanning...");
		//wxTheApp->GetLogWindow().SetTaskName("Scanning for memory access...");

		// create local reader
		trace::DataReader* tempReader = m_data->CreateReader();
		if (!tempReader)
			return false;

		// end of memory range we are testing
		const uint32 memoryAddressEnd = memoryAddress + size;

		// while not at the end
		uint32 entryIndex = 0;
		const uint32 lastEntry = m_data->GetNumDataFrames();
		decoding::Context* context = nullptr;//m_project->GetEnv().GetDecodingContext();
		while (entryIndex < lastEntry)
		{
			// stats		
			wxTheApp->GetLogWindow().SetTaskProgress(entryIndex, lastEntry);

			// get decoded frame
			const uint32 thisEntryIndex = entryIndex++;
			const trace::DataFrame& frame = tempReader->GetFrame(thisEntryIndex);
			const uint32 codeAddress = frame.GetAddress();

			// decode instruction
			decoding::Instruction op;
			if (!context->DecodeInstruction(ILogOutput::DevNull(), codeAddress, op, false))
				continue;

			// get additional info
			decoding::InstructionExtendedInfo info;
			if (!op.GetExtendedInfo(codeAddress, *context, info))
				continue;

			// not a memory op
			uint8 memFlags = 0;
			if (info.m_memoryFlags & decoding::InstructionExtendedInfo::eMemoryFlags_Read) memFlags |= 1;
			if (info.m_memoryFlags & decoding::InstructionExtendedInfo::eMemoryFlags_Write) memFlags |= 2;
			if (!(memFlags & actionMask))
				continue;

			// compute memory range for this instruction
			uint64 currentMemoryAddress = 0;
			if (!info.ComputeMemoryAddress(frame, currentMemoryAddress))
				continue;

			// address in range ?
			uint32 currentMemoryAddressEnd = currentMemoryAddress + info.m_memorySize;
			if ((currentMemoryAddressEnd > memoryAddress) && (currentMemoryAddress < memoryAddressEnd))
			{
				ProjectTraceMemoryAccessEntry entryInfo;
				entryInfo.m_address = codeAddress;
				entryInfo.m_trace = thisEntryIndex;
				entryInfo.m_size = info.m_memorySize;
				entryInfo.m_type = memFlags;

				// NULL data
				memset(&entryInfo.m_data, 0, sizeof(entryInfo.m_data));

				// write to memory
				if (memFlags & 2)
				{
					// get the source register
					const platform::CPURegister* reg0 = op.GetArg0().m_reg;
					entryInfo.m_data = trace::GetRegisterValueText(reg0, frame, trace::RegDisplayFormat::Hex);
				}

				outEntries.push_back(entryInfo);
			}
		}

		delete tempReader;
		return true;
	}

	bool ProjectTraceData::FindForwardMemoryAccess(const uint32 startEntry, const uint32 memoryAddress, const uint32 size, const uint8 actionMask, uint32& outEntryIndex, uint32& outCodeAddress) const
	{
		//wxTheApp->GetLogWindow().SetTaskName("Scanning for memory access...");

		// create local reader
		trace::DataReader* tempReader = m_data->CreateReader();
		if (!tempReader)
			return false;

		// end of memory range we are testing
		const uint32 memoryAddressEnd = memoryAddress + size;

		// while not at the end
		uint32 entryIndex = startEntry + 1;
		const uint32 lastEntry = m_data->GetNumDataFrames();
		decoding::Context* context = nullptr;// wxTheFrame->GetProject()->GetEnv().GetDecodingContext();
		while (entryIndex < lastEntry)
		{
			// stats		
			//wxTheApp->GetLogWindow().SetTaskProgress(entryIndex - startEntry, lastEntry - startEntry);

			// get decoded frame
			const uint32 thisEntryIndex = entryIndex++;
			const trace::DataFrame& frame = tempReader->GetFrame(thisEntryIndex);
			const uint32 codeAddress = frame.GetAddress();

			// decode instruction
			decoding::Instruction op;
			if (!context->DecodeInstruction(ILogOutput::DevNull(), codeAddress, op, false))
				continue;

			// get additional info
			decoding::InstructionExtendedInfo info;
			if (!op.GetExtendedInfo(codeAddress, *context, info))
				continue;

			// not a memory op
			uint8 memFlags = 0;
			if (info.m_memoryFlags & decoding::InstructionExtendedInfo::eMemoryFlags_Read) memFlags |= 1;
			if (info.m_memoryFlags & decoding::InstructionExtendedInfo::eMemoryFlags_Write) memFlags |= 2;
			if (!(memFlags & actionMask))
				continue;

			// compute memory range for this instruction
			uint64 currentMemoryAddress = 0;
			if (!info.ComputeMemoryAddress(frame, currentMemoryAddress))
				continue;

			// address in range ?
			uint32 currentMemoryAddressEnd = currentMemoryAddress + info.m_memorySize;
			if ((currentMemoryAddressEnd > memoryAddress) && (currentMemoryAddress < memoryAddressEnd))
			{
				{
					char commandName[256];
					char* stream = commandName;
					op.GenerateText(codeAddress, stream, stream + sizeof(commandName));
					wxTheApp->GetLogWindow().Log("Trace: Instruction accessing memory: '%s'", commandName);
				}

				outCodeAddress = codeAddress;
				outEntryIndex = thisEntryIndex;
				delete tempReader;
				return true;
			}
		}

		// nothing found
		delete tempReader;
		return false;
	}

	bool ProjectTraceData::FindForwardRegisterAccess(const uint32 startEntry, const char* regName, const uint8 actionMask, uint32& outEntryIndex, uint32& outCodeAddress) const
	{
		// find the image section
		decoding::Context* context = nullptr;// wxTheFrame->GetProject()->GetEnv().GetDecodingContext();
		const platform::CPU* cpu = context->GetPlatform()->GetCPU(0); // todo: per section CPU
		if (!cpu)
			return false;

		// find the register
		const platform::CPURegister* reg = cpu->FindRegister(regName);
		if (!reg)
		{
			wxTheApp->GetLogWindow().Log("No register '%s'!", regName);
			return false;
		}

		// create local reader
		trace::DataReader* tempReader = m_data->CreateReader();
		if (!tempReader)
			return false;

		// while not at the end
		uint32 entryIndex = startEntry + 1;
		const uint32 lastEntry = m_data->GetNumDataFrames();
		while (entryIndex < lastEntry)
		{
			// stats		
			//wxTheApp->GetLogWindow().SetTaskProgress(entryIndex - startEntry, lastEntry - startEntry);

			// get decoded frame
			const uint32 thisEntryIndex = entryIndex++;
			const trace::DataFrame& frame = tempReader->GetFrame(thisEntryIndex);
			const uint32 codeAddress = frame.GetAddress();

			// decode instruction
			decoding::Instruction op;
			if (!context->DecodeInstruction(ILogOutput::DevNull(), codeAddress, op, false))
				continue;

			// get additional info
			decoding::InstructionExtendedInfo info;
			if (!op.GetExtendedInfo(codeAddress, *context, info))
				continue;

			// are we using register as dependency
			bool regUsed = false;
			if (actionMask & 1)
			{
				for (uint32 i = 0; i < info.m_registersDependenciesCount; ++i)
				{
					if (info.m_registersDependencies[i] == reg)
					{
						regUsed = true;
						break;
					}
				}
			}

			// are we writing to register
			if (actionMask & 2)
			{
				for (uint32 i = 0; i < info.m_registersModifiedCount; ++i)
				{
					if (info.m_registersModified[i] == reg)
					{
						regUsed = true;
						break;
					}
				}
			}

			// address in range ?
			if (regUsed)
			{
				{
					char commandName[256];
					char* stream = commandName;
					op.GenerateText(codeAddress, stream, stream + sizeof(commandName));
					wxTheApp->GetLogWindow().Log("Trace: Instruction accessing register: '%s'", commandName);
				}

				outCodeAddress = codeAddress;
				outEntryIndex = thisEntryIndex;
				delete tempReader;
				return true;
			}
		}

		// nothing found
		delete tempReader;
		return false;
	}

	bool ProjectTraceData::FindBackwardRegisterAccess(const uint32 startEntry, const char* regName, const uint8 actionMask, uint32& outEntryIndex, uint32& outCodeAddress) const
	{
		// find the image section
		decoding::Context* context = nullptr;//wxTheFrame->GetProject()->GetEnv().GetDecodingContext();
		const platform::CPU* cpu = context->GetPlatform()->GetCPU(0); // todo: per section CPU
		if (!cpu)
			return false;

		// find the register
		const platform::CPURegister* reg = cpu->FindRegister(regName);
		if (!reg)
		{
			wxTheApp->GetLogWindow().Log("No register '%s'!", regName);
			return false;
		}

		// create local reader
		trace::DataReader* tempReader = m_data->CreateReader();
		if (!tempReader)
			return false;

		// while not at the end
		uint32 entryIndex = startEntry - 1;
		const uint32 lastEntry = m_data->GetNumDataFrames();
		while (entryIndex > 0)
		{
			// stats		
			//wxTheApp->GetLogWindow().SetTaskProgress(startEntry - entryIndex, startEntry);

			// get decoded frame
			const uint32 thisEntryIndex = entryIndex--;
			const trace::DataFrame& frame = tempReader->GetFrame(thisEntryIndex);
			const uint32 codeAddress = frame.GetAddress();

			// decode instruction
			decoding::Instruction op;
			if (!context->DecodeInstruction(ILogOutput::DevNull(), codeAddress, op, false))
				continue;

			// get additional info
			decoding::InstructionExtendedInfo info;
			if (!op.GetExtendedInfo(codeAddress, *context, info))
				continue;

			// are we using register as dependency
			bool regUsed = false;
			if (actionMask & 1)
			{
				for (uint32 i = 0; i < info.m_registersDependenciesCount; ++i)
				{
					if (info.m_registersDependencies[i] == reg)
					{
						regUsed = true;
						break;
					}
				}
			}

			// are we writing to register
			if (actionMask & 2)
			{
				for (uint32 i = 0; i < info.m_registersModifiedCount; ++i)
				{
					if (info.m_registersModified[i] == reg)
					{
						regUsed = true;
						break;
					}
				}
			}

			// address in range ?
			if (regUsed)
			{
				{
					char commandName[256];
					char* stream = commandName;
					op.GenerateText(codeAddress, stream, stream + sizeof(commandName));
					wxTheApp->GetLogWindow().Log("Trace: Instruction accessing register: '%s'", commandName);
				}

				outCodeAddress = codeAddress;
				outEntryIndex = thisEntryIndex;
				delete tempReader;
				return true;
			}
		}

		// nothing found
		delete tempReader;
		return false;
	}

	int ProjectTraceData::GetStepBackEntry() const
	{
		if (m_currentEntry > 0)
		{
			return m_currentEntry - 1;
		}

		return -1;
	}

	int ProjectTraceData::GetStepIntoEntry() const
	{
		// advance code to next instruction
		const uint32 lastEntry = m_data->GetNumDataFrames();
		if (m_currentEntry < lastEntry)
		{
			return m_currentEntry + 1;
		}

		return -1;
	}

	int ProjectTraceData::GetStepOverEntry() const
	{
		return -1;
	}

	int ProjectTraceData::GetStepOutEntry() const
	{
		return -1;
	}

	void ProjectTraceData::BatchVisit(const uint32 firstEntry, const uint32 lastEntry, trace::IBatchVisitor& dataInterface) const
	{
		m_data->BatchVisit(firstEntry, lastEntry, dataInterface);
	}

	void ProjectTraceData::BatchVisitAll(trace::IBatchVisitor& dataInterface) const
	{
		if (m_data->GetNumDataFrames())
			m_data->BatchVisit(0, m_data->GetNumDataFrames() - 1, dataInterface);
	}

	bool ProjectTraceData::BuildMemoryMap()
	{
		// delete current memory map data
		if (m_memReader)
		{
			delete m_memReader;
			m_memReader = NULL;
		}

		// delete current memory map
		if (m_memory)
		{
			delete m_memory;
			m_memory = NULL;
		}

		// mem file trace
		std::wstring memTracePath(m_filePath);
		memTracePath += L".mem";

		// try to build the memory trace file
		/*trace::MemoryHistoryBuilder builder;
		if (!builder.BuildFile(wxTheApp->GetLogWindow(), *m_project->GetEnv().GetDecodingContext(), *m_data, memTracePath.c_str()))
			return false;*/

		// open the memory file	
		m_memory = trace::MemoryHistory::OpenFile(wxTheApp->GetLogWindow(), memTracePath.c_str());
		if (!m_memory)
			return false;

		// create memory reader
		m_memReader = m_memory->CreateReader();
		if (!m_memReader)
		{
			delete m_memory;
			return false;
		}

		return true;
	}

	bool ProjectTraceData::BuildCallstack(ILogOutput& log)
	{
		// no decoding context
/*		if (!wxTheFrame->GetProject()->GetEnv().GetDecodingContext())
		{
			log.Error("Callstack: No decoding context");
			return false;
		}*/

		// delete current callstack data
		if (m_callstack)
		{
			delete m_callstack;
			m_callstack = nullptr;
		}

		// try to build the memory trace file
		trace::CallTree* callstack = nullptr;// trace::CallTree::BuildTraceCalltack(log, *wxTheFrame->GetProject()->GetEnv().GetDecodingContext(), *m_data, 0, progressDisplay);
		if (!callstack)
		{
			log.Error("Callstack: Failed to build callstack from trace data");
			return false;
		}

		// store the callstack
		m_callstack = callstack;

		// mem file trace
		std::wstring callstackTracePath(m_filePath);
		callstackTracePath += L".call";

		// save it
		if (!m_callstack->Save(log, callstackTracePath.c_str()))
		{
			log.Warn("Callstack: Failed to save callstack data. Results will not be persistent.");
		}

		// valid
		return true;
	}

	bool ProjectTraceData::GetMemoryHistory(const uint32 address, const uint32 size, std::vector<ProjectTraceMemoryHistoryInfo>& outHistory) const
	{
		if (!m_memReader)
			return false;

		// do it for each byte
		for (uint32 i = 0; i < size; ++i)
		{
			const uint32 byteAddress = address + i;
			const uint16 byteMask = (uint16)(1 << i);

			// get memory history
			std::vector<trace::MemoryHistoryReader::History> localHistory;
			if (!m_memReader->GetHistory(byteAddress, 0, UINT_MAX, localHistory))
				continue;

			// process results
			for (uint32 j = 0; j < localHistory.size(); ++j)
			{
				const trace::MemoryHistoryReader::History& memHistory = localHistory[j];

				// update entries
				bool updated = false;
				for (uint32 k = 0; k < outHistory.size(); ++k)
				{
					if (outHistory[k].m_entry == memHistory.m_entry)
					{
						outHistory[k].m_validMask |= byteMask;
						outHistory[k].m_opMask |= memHistory.m_op ? byteMask : 0;
						outHistory[k].m_data[i] = memHistory.m_data;
						updated = true;
					}
				}

				// create new entry
				if (!updated)
				{
					ProjectTraceMemoryHistoryInfo newEntry;
					memset(&newEntry, 0, sizeof(newEntry));
					newEntry.m_entry = memHistory.m_entry;
					newEntry.m_validMask = byteMask;
					newEntry.m_opMask = memHistory.m_op ? byteMask : 0;
					newEntry.m_data[i] = memHistory.m_data;
					outHistory.push_back(newEntry);
				}
			}
		}

		// sort the results

		// done
		return !outHistory.empty();
	}

	bool ProjectTraceData::GetRegisterHistory(const uint32 testCodeAddress, const char* regName, const int startTrace, const int endTrace, std::vector< ProjectTraceAddressHistoryInfo >& outHistory) const
	{
		// find the image section
		decoding::Context* context = nullptr;//auto* context = m_project->GetEnv().GetDecodingContext();
		const platform::CPU* cpu = context->GetPlatform()->GetCPU(0); // todo: per section CPU
		if (!cpu)
			return false;

		// find the register
		const platform::CPURegister* reg = cpu->FindRegister(regName);
		if (!reg)
		{
			wxTheApp->GetLogWindow().Error("No register '%s' in cpu '%s'!", regName, cpu->GetName().c_str());
			return false;
		}

		// create local reader
		trace::DataReader* tempReader = m_data->CreateReader();
		if (!tempReader)
			return false;

		// while not at the end
		uint32 endEntryIndex = (endTrace < startTrace) ? m_data->GetNumDataFrames() : endTrace;
		for (uint32 entryIndex = startTrace; entryIndex < endEntryIndex; ++entryIndex)
		{
			// stats		
			wxTheApp->GetLogWindow().SetTaskProgress(entryIndex - startTrace, endEntryIndex - startTrace);

			// get decoded frame
			const trace::DataFrame& frame = tempReader->GetFrame(entryIndex);
			const uint32 codeAddress = frame.GetAddress();
			if (codeAddress != testCodeAddress)
				continue;

			// dump data
			ProjectTraceAddressHistoryInfo entry;
			entry.m_traceEntry = entryIndex;
			entry.m_register = reg;
			entry.m_value = trace::GetRegisterValueText(reg, frame);
			outHistory.push_back(entry);
		}

		// data extracted
		delete tempReader;
		return true;
	}

	ProjectTraceData* ProjectTraceData::LoadFromFile(ILogOutput& log, Project* project, const std::wstring& filePath)
	{
		// load raw data
		trace::Data* data = trace::Data::OpenTraceFile(wxTheApp->GetLogWindow(), filePath);
		if (!data)
			return NULL;

		// create project wrapper
		ProjectTraceData* projectData = new ProjectTraceData();
		projectData->m_project = project;
		projectData->m_data = data;
		projectData->m_reader = data->CreateReader();
		projectData->m_memReader = NULL;
		projectData->m_callstack = NULL;
		projectData->m_filePath = filePath;
		projectData->Reset();

		// load memory map (if there)
		{
			// assemble file path
			std::wstring memTracePath(filePath);
			memTracePath += L".mem";

			// load the mem map if present
			trace::MemoryHistory* memData = trace::MemoryHistory::OpenFile(log, memTracePath.c_str());
			if (memData)
			{
				projectData->m_memReader = memData->CreateReader();
				log.Log("Trace: Found and loaded memory map");
			}
		}

		// load call stack data
		{
			// assemble file path
			std::wstring callstackTracePath(filePath);
			callstackTracePath += L".call";

			// laod callstack data if there
			trace::CallTree* callstack = trace::CallTree::OpenTraceCallstack(log, callstackTracePath.c_str());
			if (callstack)
			{
				projectData->m_callstack = callstack;
				log.Log("Trace: Found and loaded callstack data");
			}
		}

		// done
		return projectData;
	}

} // tools