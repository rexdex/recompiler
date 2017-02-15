#include "build.h"
#include "xenonInplaceExecution.h"

#include "..\..\..\launcher\backend\runtimeCodeExecutor.h"
#include "..\..\..\launcher\backend\runtimeTraceWriter.h"

namespace xenon
{
	extern cpu::Reservation GReservation;

	InplaceExecution::InplaceExecution(Kernel* kernel, const InplaceExecutionParams& params)
		: m_code( &m_regs, &kernel->GetCode(), params.m_entryPoint )
		, m_memory(kernel, params.m_stackSize, params.m_threadId, params.m_entryPoint, params.m_cpu)
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
	}

	InplaceExecution::~InplaceExecution()
	{
	}

	
	int InplaceExecution::Execute(const bool trace)
	{
#if 0
		runtime::TraceWriter* traceFile = nullptr;
		if (trace)
		{
			static uint32 s_traceIndex = 0;

			wchar_t filePath[128];
			wsprintf(filePath, L"D:\\inplace_%08Xh_%d.trace", m_code.GetInstructionPointer(), s_traceIndex++);

			traceFile = runtime::TraceWriter::CreateTrace(*m_regs.GetDesc(), filePath);
		}

		if (traceFile != nullptr)
		{
			traceFile->AddFrame(m_code.GetInstructionPointer(), m_regs);

			while (m_code.Step())
				traceFile->AddFrame(m_code.GetInstructionPointer(), m_regs);
		}
		else
		{
			while (m_code.Step()) {};
		}

		delete traceFile;
#else
		while (m_code.Step()) {};
#endif
		return (int)m_regs.R3;
	}

} // xenon