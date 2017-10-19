#include "build.h"
#include "xenonInplaceExecution.h"

#include "../host_core/runtimeCodeExecutor.h"
#include "../host_core/runtimeTraceWriter.h"
#include "../host_core/runtimeTraceFile.h"

namespace xenon
{
	extern cpu::Reservation GReservation;

	InplaceExecution::InplaceExecution(Kernel* kernel, const InplaceExecutionParams& params, const char* name)
		: m_code( &m_regs, &kernel->GetCode(), params.m_entryPoint )
		, m_memory(kernel, params.m_stackSize, params.m_threadId, params.m_entryPoint, params.m_cpu)
		, m_name(name)
	{
		// setup initial register state
		m_regs.R1 = (uint64)m_memory.GetStack().GetTop();
		m_regs.R3 = params.m_args[0];
		m_regs.R4 = params.m_args[1];
		m_regs.R5 = params.m_args[2];
		m_regs.R6 = params.m_args[3];
		m_regs.R7 = params.m_args[4];
		m_regs.R8 = params.m_args[5];
		m_regs.R13 = m_memory.GetPRCAddr(); // always at r13
		m_regs.RES = &GReservation;
		m_regs.INT = &GPlatform.GetInterruptTable();
		m_regs.IO = &GPlatform.GetIOTable();
	}

	InplaceExecution::~InplaceExecution()
	{
	}
	
	int InplaceExecution::Execute()
	{
		auto* traceFile = GPlatform.GetTraceFile();
		auto* traceWriter = traceFile ? traceFile->CreateInterruptWriter(m_name) : nullptr;

		if (traceWriter != nullptr)
		{
			traceWriter->AddFrame(m_code.GetInstructionPointer(), m_regs);

			while (m_code.RunTraced(*traceWriter))
			{}				
		}
		else
		{
			while (m_code.RunPure())
			{}
		}

		delete traceWriter;

		return (int)m_regs.R3;
	}

} // xenon