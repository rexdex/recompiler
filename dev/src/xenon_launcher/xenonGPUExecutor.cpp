/* Copyright (c) 2002,2007-2009, Code Aurora Forum. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of Code Aurora nor
*       the names of its contributors may be used to endorse or promote
*       products derived from this software without specific prior written
*       permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
* NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
* OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/

#include "build.h"
#include "xenonLibNatives.h"
#include "xenonGPUCommandBuffer.h"
#include "xenonGPUExecutor.h"
#include "xenonGPUOpcodes.h"
#include "xenonGPUUtils.h"
#include "xenonGPUDumpWriter.h"
#include "xenonGPUTraceWriter.h"

#include "dx11AbstractLayer.h"

//-----------

CXenonGPUExecutor::CXenonGPUExecutor(IXenonGPUAbstractLayer* abstractionLayer, const launcher::Commandline& cmdLine)
	: m_abstractLayer(abstractionLayer)
	, m_tiledMask(0xFFFFFFFFull)
	, m_tiledSelector(0xFFFFFFFFull)
	, m_swapCounter(0)
	, m_vblankCounter(0)
	, m_interruptAddr(0)
	, m_interruptUserData(0)
	, m_traceDumpRequested(false)
	, m_traceDumpFile(nullptr)
	, m_logWriter(nullptr)
{
	/*if (cmdLine.HasOption("gpulog"))
	{
		std::wstring gpuLogFile = cmdLine.GetOptionValueW("gpulog");
		GLog.Log("GPU: Logging gpu activity to '%ls'", gpuLogFile.c_str());

		m_logWriter = CXenonGPUTraceWriter::Create(gpuLogFile);
	}*/
}

CXenonGPUExecutor::~CXenonGPUExecutor()
{
	if (m_logWriter)
	{
		delete m_logWriter;
		m_logWriter = nullptr;
	}

	if (m_traceDumpFile)
	{
		m_traceDumpFile->Close();
		m_traceDumpFile = nullptr;
	}
}


class CXenonGPUFileTraceBlock
{
public:
	CXenonGPUFileTraceBlock(IXenonGPUDumpWriter*& fileWriter, const char* tag)
		: m_trace(&fileWriter)
		, m_blockId(0)
	{
		if (m_trace && *m_trace)
			(*m_trace)->BeginBlock(tag, m_blockId);
	}

	~CXenonGPUFileTraceBlock()
	{
		if (m_trace && *m_trace)
			(*m_trace)->EndBlock(m_blockId);
	}

private:
	IXenonGPUDumpWriter**		m_trace;
	uint32						m_blockId;
};

class CXenonGPULogBlock
{
public:
	CXenonGPULogBlock(CXenonGPUTraceWriter* fileWriter, const char* tag)
		: m_fileWriter(fileWriter)
		, m_tag(tag)
	{
		if (m_fileWriter)
		{
			m_fileWriter->Writef("Block '%hs'", m_tag);
			m_fileWriter->Indent(1);
		}
	}

	~CXenonGPULogBlock()
	{
		if (m_fileWriter)
		{
			m_fileWriter->Indent(-1);
			m_fileWriter->Writef("EndBlock '%hs'", m_tag);
		}
	}

private:
	CXenonGPUTraceWriter*	m_fileWriter;
	const char*				m_tag;
};

void CXenonGPUExecutor::Execute(CXenonGPUCommandBufferReader& reader)
{
	ExecutePrimaryBuffer(reader);
}

void CXenonGPUExecutor::SetInterruptCallbackAddr(const uint32 addr, const uint32 userData)
{
	m_interruptAddr = addr;
	m_interruptUserData = userData;
}

bool GSuppressGPUInterrupts = false;

void CXenonGPUExecutor::RequestTraceDump()
{
	m_traceDumpRequested = true;
}

void CXenonGPUExecutor::SignalVBlank()
{
	if (GSuppressGPUInterrupts)
		return;

	m_vblankCounter += 1;

	DispatchInterrupt(0, 2);
}

uint64 CXenonGPUExecutor::ReadRegister(const uint32 registerIndex)
{
	switch (registerIndex)
	{
		case 0x3C00:  // ?
			return 0x08100748;

		case 0x3C04:  // ?
			return 0x0000200E;

		case 0x6530:  // Scanline?
			return 0x000002D0;

		case 0x6544:  // ? vblank pending?
			return 1;

		case 0x6584:  // Screen res - 1280x720
			return 0x050002D0;
	}

	return m_registers[registerIndex].m_dword;
}

void CXenonGPUExecutor::ExecutePrimaryBuffer(CXenonGPUCommandBufferReader& reader)
{
	CXenonGPUFileTraceBlock block(m_traceDumpFile, "PrimaryBuffer");
	CXenonGPULogBlock block2(m_logWriter, "PrimaryBuffer");

	while (reader.CanRead())
	{
		ExecutePacket(reader);
	}
}

bool CXenonGPUExecutor::ExecutePacket(CXenonGPUCommandBufferReader& reader)
{
	const uint32 packetData = reader.Read();
	const uint32_t packetType = packetData >> 30;

	// empty packet data
	if (packetData == 0)
		return true;

	// process different types of packets
	switch (packetType)
	{
		case 0x00: return ExecutePacketType0(reader, packetData);
		case 0x01: return ExecutePacketType1(reader, packetData);
		case 0x02: return ExecutePacketType2(reader, packetData);
		case 0x03: return ExecutePacketType3(reader, packetData);
	}

	// invalid packet type
	return false;
}

bool CXenonGPUExecutor::ExecutePacketType0(CXenonGPUCommandBufferReader& reader, const uint32 packetData)
{
	CXenonGPULogBlock block2(m_logWriter, "Packet0");

	// Type-0 packet.
	// Write count registers in sequence to the registers starting at
	// (base_index << 2).

	const uint32 regCount = ((packetData >> 16) & 0x3FFF) + 1;
	const uint32 baseIndex = (packetData & 0x7FFF);
	const uint32 writeOneReg = (packetData >> 15) & 0x1;

	// log info
	if (m_logWriter)
	{
		m_logWriter->Writef("RegCount = %d", regCount);
		m_logWriter->Writef("BaseIndex = %d", baseIndex);
		m_logWriter->Writef("WriteOneReg = %d", writeOneReg);
	}

	// trace
	if (m_traceDumpFile)
	{
		auto* wordsData = (uint32*)alloca(sizeof(uint32) * regCount);
		reader.GetBatch(regCount, wordsData);

		m_traceDumpFile->Packet(packetData, wordsData, regCount);
	}

	for (uint32 i = 0; i<regCount; i++)
	{
		const uint32 regData = reader.Read();
		const uint32 regIndex = writeOneReg ? baseIndex : baseIndex + i;
		WriteRegister(regIndex, regData);
	}

	// valid
	return true;
}

bool CXenonGPUExecutor::ExecutePacketType1(CXenonGPUCommandBufferReader& reader, const uint32 packetData)
{
	CXenonGPULogBlock block2(m_logWriter, "Packet1");

	// Type-1 packet.
	// Two registers of data

	const uint32 regIndexA = packetData & 0x7FF;
	const uint32 regIndexB = (packetData >> 11) & 0x7FF;
	const uint32 regDataA = reader.Read();
	const uint32 regDataB = reader.Read();

	// log info
	if (m_logWriter)
	{
		m_logWriter->Writef("RegIndexA = %d", regIndexA);
		m_logWriter->Writef("RegIndexB = %d", regIndexB);
		m_logWriter->Writef("RegDataA = %08Xh (%d)", regDataA, regDataA);
		m_logWriter->Writef("RegDataB = %08Xh (%d)", regDataB, regDataB);
	}

	// trace
	if (m_traceDumpFile)
	{
		uint32 wordsData[2];
		reader.GetBatch(2, wordsData);

		m_traceDumpFile->Packet(packetData, wordsData, 2);
	}

	WriteRegister(regIndexA, regDataA);
	WriteRegister(regIndexB, regDataB);

	return true;
}

bool CXenonGPUExecutor::ExecutePacketType2(CXenonGPUCommandBufferReader& reader, const uint32 packetData)
{
	CXenonGPULogBlock block2(m_logWriter, "Packet2");

	// nop
	return true;
}

class CFullReadCheck
{
public:
	CFullReadCheck(const CXenonGPUCommandBufferReader& reader, const uint32 count)
		: m_reader(&reader)
		//, m_expectedOffset( reader.GetReadOffset() + count )
	{
	}

	~CFullReadCheck()
	{
		//DEBUG_CHECK( m_expectedOffset == m_reader->GetReadOffset() );
	}

private:
	const CXenonGPUCommandBufferReader*		m_reader;
	//const uint32							m_expectedOffset;
};

bool CXenonGPUExecutor::ExecutePacketType3(CXenonGPUCommandBufferReader& reader, const uint32 packetData)
{
	CXenonGPULogBlock block2(m_logWriter, "Packet3");

	// Type-3 packet.
	const uint32 opcode = (packetData >> 8) & 0x7F;
	const uint32 count = ((packetData >> 16) & 0x3FFF) + 1;

	// log
	if (m_logWriter)
	{
		m_logWriter->Writef("OpCode = %d", opcode);
		m_logWriter->Writef("Count = %d", count);
	}

	// trace
	if (m_traceDumpFile && opcode != PM4_INDIRECT_BUFFER)
	{
		auto* wordsData = (uint32*)alloca(sizeof(uint32) * count);
		reader.GetBatch(count, wordsData);

		m_traceDumpFile->Packet(packetData, wordsData, count);
	}

	// in predicated mode we may need to skip this command if the tile mask does not match tile selector
	if (packetData & 1)
	{
		// skip if we are NOT the target
		const bool shouldSkip = (m_tiledMask & m_tiledSelector) == 0;
		if (shouldSkip)
		{
			reader.Advance(count);
			return true;
		}
	}

	// check read validity
	const CFullReadCheck readerCheck(reader, count);

	// process opcode
	switch (opcode)
	{
		case PM4_ME_INIT:
			return ExecutePacketType3_ME_INIT(reader, packetData, count);

		case PM4_NOP:
			return ExecutePacketType3_NOP(reader, packetData, count);

		case PM4_INTERRUPT:
			return ExecutePacketType3_INTERRUPT(reader, packetData, count);

		case PM4_HACK_SWAP:
			return ExecutePacketType3_HACK_SWAP(reader, packetData, count);

		case PM4_INDIRECT_BUFFER:
			return ExecutePacketType3_INDIRECT_BUFFER(reader, packetData, count);

		case PM4_WAIT_REG_MEM:
			return ExecutePacketType3_WAIT_REG_MEM(reader, packetData, count);

		case PM4_REG_RMW:
			return ExecutePacketType3_REG_RMW(reader, packetData, count);

		case PM4_COND_WRITE:
			return ExecutePacketType3_COND_WRITE(reader, packetData, count);

		case PM4_EVENT_WRITE:
			return ExecutePacketType3_EVENT_WRITE(reader, packetData, count);

		case PM4_EVENT_WRITE_SHD:
			return ExecutePacketType3_EVENT_WRITE_SHD(reader, packetData, count);

		case PM4_EVENT_WRITE_EXT:
			return ExecutePacketType3_EVENT_WRITE_EXT(reader, packetData, count);

		case PM4_DRAW_INDX:
			return ExecutePacketType3_DRAW_INDX(reader, packetData, count);

		case PM4_DRAW_INDX_2:
			return ExecutePacketType3_DRAW_INDX_2(reader, packetData, count);

		case PM4_SET_CONSTANT:
			return ExecutePacketType3_SET_CONSTANT(reader, packetData, count);

		case PM4_SET_CONSTANT2:
			return ExecutePacketType3_SET_CONSTANT2(reader, packetData, count);

		case PM4_LOAD_ALU_CONSTANT:
			return ExecutePacketType3_LOAD_ALU_CONSTANT(reader, packetData, count);

		case PM4_SET_SHADER_CONSTANTS:
			return ExecutePacketType3_SET_SHADER_CONSTANTS(reader, packetData, count);

		case PM4_IM_LOAD:
			return ExecutePacketType3_IM_LOAD(reader, packetData, count);

		case PM4_IM_LOAD_IMMEDIATE:
			return ExecutePacketType3_IM_LOAD_IMMEDIATE(reader, packetData, count);

		case PM4_INVALIDATE_STATE:
			return ExecutePacketType3_INVALIDATE_STATE(reader, packetData, count);

			// tiled rendering bit mask
		case PM4_SET_BIN_MASK_LO:
		{
			const uint32 loMask = reader.Read();
			m_tiledMask = (m_tiledMask & 0xFFFFFFFF00000000ull) | loMask;
			return true;
		}

		case PM4_SET_BIN_MASK_HI:
		{
			const uint32 hiMask = reader.Read();
			m_tiledMask = (m_tiledMask & 0xFFFFFFFFull) | (static_cast<uint64>(hiMask) << 32);
			return true;
		}

		case PM4_SET_BIN_SELECT_LO:
		{
			const uint32 loSelect = reader.Read();
			m_tiledSelector = (m_tiledSelector & 0xFFFFFFFF00000000ull) | loSelect;
			return true;
		}

		case PM4_SET_BIN_SELECT_HI:
		{
			const uint32 hiSelect = reader.Read();
			m_tiledSelector = (m_tiledSelector & 0xFFFFFFFFull) | (static_cast<uint64>(hiSelect) << 32);
			return true;
		}

		// Ignored packets - useful if breaking on the default handler below.
		case 0x50:  // 0xC0015000 usually 2 words, 0xFFFFFFFF / 0x00000000
		case 0x51:  // 0xC0015100 usually 2 words, 0xFFFFFFFF / 0xFFFFFFFF
		{
			reader.Advance(count);
			break;
		}
	}

	// ERROR
	GLog.Err("GPU: Unknown Type3 opcode: %d, packet data: 0x%08X, count: %d", opcode, packetData, count);
	reader.Advance(count);
	return false;
}

bool CXenonGPUExecutor::ExecutePacketType3_ME_INIT(CXenonGPUCommandBufferReader& reader, const uint32 packetData, const uint32 count)
{
	CXenonGPULogBlock block(m_logWriter, "ME_INIT");

	// micro-engine initialization
	// nothing to do in here
	reader.Advance(count);
	return true;
}

bool CXenonGPUExecutor::ExecutePacketType3_NOP(CXenonGPUCommandBufferReader& reader, const uint32 packetData, const uint32 count)
{
	CXenonGPULogBlock block(m_logWriter, "NOP");
	reader.Advance(count);
	return true;
}

bool CXenonGPUExecutor::ExecutePacketType3_INTERRUPT(CXenonGPUCommandBufferReader& reader, const uint32 packetData, const uint32 count)
{
	CXenonGPULogBlock block(m_logWriter, "INTERRUPT");

	const uint32 cpuMask = reader.Read();

	if (m_logWriter)
		m_logWriter->Writef("CPU Mask = %d", cpuMask);

	for (uint32 n = 0; n < 6; n++)
	{
		if (cpuMask & (1 << n))
		{
			DispatchInterrupt(1, n);
		}
	}

	return true;
}

bool CXenonGPUExecutor::ExecutePacketType3_HACK_SWAP(CXenonGPUCommandBufferReader& reader, const uint32 packetData, const uint32 count)
{
	CXenonGPULogBlock block(m_logWriter, "SWAP");

	// read front buffer stuff
	DEBUG_CHECK(count >= 3);
	const uint32 frontBufferPtr = reader.Read() & 0x3FFFFFFF;
	const uint32 frontBufferWidth = reader.Read();
	const uint32 frontBufferHeight = reader.Read();

	// consume the rest
	reader.Advance(count - 3);

	// count frames
	m_swapCounter += 1;

	// end of current capture
	if (m_traceDumpFile)
	{
		m_traceDumpFile->Close();
		m_traceDumpFile = nullptr;

		GLog.Log("GPU: Trace dump finished");
	}

	// start new capture
	if (m_traceDumpRequested)
	{
		GLog.Log("GPU: Trace dump requested");

		m_traceDumpRequested = false;
		m_traceDumpFile = IXenonGPUDumpWriter::Create();
		if (m_traceDumpFile)
			GLog.Log("GPU: Trace dump started");
	}

	// setup swap command
	CXenonGPUState::SwapState ss;
	ss.m_frontBufferBase = frontBufferPtr;
	ss.m_frontBufferWidth = frontBufferWidth;
	ss.m_frontBufferHeight = frontBufferHeight;
	return m_state.IssueSwap(m_abstractLayer, m_traceDumpFile, m_registers, ss);
}

bool CXenonGPUExecutor::ExecutePacketType3_INDIRECT_BUFFER(CXenonGPUCommandBufferReader& reader, const uint32 packetData, const uint32 count)
{
	CXenonGPULogBlock block(m_logWriter, "INDIRECT_BUFFER");
	CXenonGPUFileTraceBlock block2(m_traceDumpFile, "IndirectBuffer");

	// indirect buffer dispatch
	const uint32 listAddr = XenonCPUAddrToGPUAddr(reader.Read());
	const uint32 listLength = reader.Read();

	// log
	if (m_logWriter)
	{
		m_logWriter->Writef("ListAddr = %08Xh", listAddr);
		m_logWriter->Writef("ListLength = %d", listLength);
	}

	// create indirect reader
	const uint32* memBase = (const uint32*)GPlatform.GetMemory().TranslatePhysicalAddress(listAddr);
	CXenonGPUCommandBufferReader indirectReader(memBase, listLength, 0, listLength);

	// execute indirect commands
	while (indirectReader.CanRead())
	{
		ExecutePacket(indirectReader);
	}

	// done
	return true;
}

bool CXenonGPUExecutor::ExecutePacketType3_WAIT_REG_MEM(CXenonGPUCommandBufferReader& reader, const uint32 packetData, const uint32 count)
{
	CXenonGPULogBlock block(m_logWriter, "WAIT_REG_MEM");

	// wait until a register or memory location is a specific value
	const uint32 waitInfo = reader.Read();
	const uint32 pollRegAddr = reader.Read();
	const uint32 ref = reader.Read();
	const uint32 mask = reader.Read();
	const uint32 wait = reader.Read();

	// log
	if (m_logWriter)
	{
		m_logWriter->Writef("WaitInfo = %08Xh", waitInfo);
		m_logWriter->Writef("PoolRegAddr = %d", pollRegAddr);
		m_logWriter->Writef("Ref = %08Xh", ref);
		m_logWriter->Writef("Mask = %08Xh", mask);
		m_logWriter->Writef("Wait = %08Xh", wait);
	}

	bool matched = false;
	do
	{
		uint32 value;
		if (waitInfo & 0x10)
		{
			// Log in trace
			if (m_traceDumpFile)
			{
				const uint32 memoryAddress = GPlatform.GetMemory().TranslatePhysicalAddress(pollRegAddr & ~0x3);
				m_traceDumpFile->MemoryAccessRead(memoryAddress, sizeof(uint32), "WaitForMemory");
			}

			// Memory.
			value = XenonGPULoadPhysicalAddWithFormat(pollRegAddr);

			// log
			if (m_logWriter)
			{
				const uint32 memoryAddress = GPlatform.GetMemory().TranslatePhysicalAddress(pollRegAddr & ~0x3);
				m_logWriter->Writef("MemRead[%08Xh] = %08Xh", memoryAddress, value);
			}
		}
		else
		{
			// Register.
			DEBUG_CHECK(pollRegAddr < m_registers.NUM_REGISTERS);
			value = m_registers[pollRegAddr].m_dword;

			// Sync
			if (pollRegAddr == (uint32)XenonGPURegister::REG_COHER_STATUS_HOST)
			{
				MakeCoherent();
				value = m_registers[pollRegAddr].m_dword;
			}

			// log
			if (m_logWriter)
				m_logWriter->Writef("Reg[%08Xh] = %08Xh", pollRegAddr, value);
		}

		switch (waitInfo & 0x7)
		{
			case 0x0:  // Never.
				matched = false;
				break;

			case 0x1:  // Less than reference.
				matched = (value & mask) < ref;
				break;

			case 0x2:  // Less than or equal to reference.
				matched = (value & mask) <= ref;
				break;

			case 0x3:  // Equal to reference.
				matched = (value & mask) == ref;
				break;

			case 0x4:  // Not equal to reference.
				matched = (value & mask) != ref;
				break;

			case 0x5:  // Greater than or equal to reference.
				matched = (value & mask) >= ref;
				break;

			case 0x6:  // Greater than reference.
				matched = (value & mask) > ref;
				break;

			case 0x7:  // Always
				matched = true;
				break;
		}

		if (!matched)
		{
			// Wait.
			if (wait >= 0x100)
			{
				BeingWait();
				Sleep(wait / 0x100);
				MemoryBarrier();
				FinishWait();
			}
			else
			{
				// just yield
				SwitchToThread();
			}
		}
	} while (!matched);
	return true;
}

bool CXenonGPUExecutor::ExecutePacketType3_REG_RMW(CXenonGPUCommandBufferReader& reader, const uint32 packetData, const uint32 count)
{
	CXenonGPULogBlock block(m_logWriter, "REG_RMW");

	// read/modify/write for register
	const uint32 rmwSetup = reader.Read();
	const uint32 andMask = reader.Read();
	const uint32 orMask = reader.Read();

	// log
	if (m_logWriter)
	{
		m_logWriter->Writef("RMWSetup = %08Xh", rmwSetup);
		m_logWriter->Writef("AndMask = %d", andMask);
		m_logWriter->Writef("OrMask = %08Xh", orMask);
	}

	// read value
	const uint32 regIndex = (rmwSetup & 0x1FFF);
	uint32 value = m_registers[regIndex].m_dword;
	const uint32 oldValue = value;

	// OR value (with reg or immediate value)
	if ((rmwSetup >> 30) & 0x1)
	{
		// | reg
		const uint32 orValue = m_registers[orMask & 0x1FFF].m_dword;
		value |= orValue;
	}
	else
	{
		// | imm
		value |= orMask;
	}

	// AND value (with reg or immediate value)
	if ((rmwSetup >> 31) & 0x1)
	{
		// & reg
		const uint32 andValue = m_registers[andMask & 0x1FFF].m_dword;
		value &= andValue;
	}
	else
	{
		// & imm
		value &= andMask;
	}

	// write new value
	WriteRegister(regIndex, value);
	return true;
}

bool CXenonGPUExecutor::ExecutePacketType3_COND_WRITE(CXenonGPUCommandBufferReader& reader, const uint32 packetData, const uint32 count)
{
	CXenonGPULogBlock block(m_logWriter, "COND_WRITE");

	// conditional write to memory or register
	const uint32 waitInfo = reader.Read();
	const uint32 pollRegAddr = reader.Read();
	const uint32 ref = reader.Read();
	const uint32 mask = reader.Read();
	const uint32 writeRegAddr = reader.Read();
	const uint32 writeData = reader.Read();

	// log
	if (m_logWriter)
	{
		m_logWriter->Writef("WaitInfo = %08Xh", waitInfo);
		m_logWriter->Writef("PollRegAddr = %d", pollRegAddr);
		m_logWriter->Writef("Ref = %08Xh", ref);
		m_logWriter->Writef("Mask = %08Xh", mask);
		m_logWriter->Writef("WriteRegAddr = %08Xh", writeRegAddr);
		m_logWriter->Writef("WriteData = %08Xh", writeData);
	}

	// read input value
	uint32 value;
	if (waitInfo & 0x10)
	{
		// get swap format (swap)
		const auto format = static_cast< XenonGPUEndianFormat >(pollRegAddr & 0x3);

		// get actual memory address
		const uint32 addr = GPlatform.GetMemory().TranslatePhysicalAddress(XenonGPUAddrToCPUAddr(pollRegAddr & ~3));

		// Log in trace
		if (m_traceDumpFile)
		{
			const uint32 memoryAddress = GPlatform.GetMemory().TranslatePhysicalAddress(pollRegAddr & ~0x3);
			m_traceDumpFile->MemoryAccessRead(memoryAddress, sizeof(uint32), "ConditionalWrite");
		}

		// load data (raw)
		const uint32 rawValue = *(const uint32*)addr;

		// swap value
		value = XenonGPUSwap32(rawValue, format);

		// log the read
		if (m_logWriter)
			m_logWriter->Writef("MemRead[%08Xh] = %08Xh", addr, value);
	}
	else
	{
		DEBUG_CHECK(pollRegAddr < m_registers.NUM_REGISTERS);
		value = m_registers[pollRegAddr].m_dword;

		// log the read
		if (m_logWriter)
			m_logWriter->Writef("Reg[%08Xh] = %08Xh", pollRegAddr, value);
	}

	// comapre
	bool matched = false;
	switch (waitInfo & 0x7)
	{
		case 0x0:  // Never.
			matched = false;
			break;

		case 0x1:  // Less than reference.
			matched = (value & mask) < ref;
			break;

		case 0x2:  // Less than or equal to reference.
			matched = (value & mask) <= ref;
			break;

		case 0x3:  // Equal to reference.
			matched = (value & mask) == ref;
			break;

		case 0x4:  // Not equal to reference.
			matched = (value & mask) != ref;
			break;

		case 0x5:  // Greater than or equal to reference.
			matched = (value & mask) >= ref;
			break;

		case 0x6:  // Greater than reference.
			matched = (value & mask) > ref;
			break;

		case 0x7:  // Always
			matched = true;
			break;
	}

	// write result
	if (matched)
	{
		// Write.
		if (waitInfo & 0x100)
		{
			// write address
			const uint32 addr = GPlatform.GetMemory().TranslatePhysicalAddress(writeRegAddr & ~3);

			// convert value
			const auto format = static_cast< XenonGPUEndianFormat >(writeRegAddr & 0x3);
			const uint32 rawValue = XenonGPUSwap32(writeData, format);

			// write it
			*(uint32*)addr = rawValue;

			// log the read
			if (m_logWriter)
				m_logWriter->Writef("MemWrite[%08Xh] = %08Xh", addr, writeData);

			// Log in trace
			if (m_traceDumpFile)
			{
				const uint32 memoryAddress = GPlatform.GetMemory().TranslatePhysicalAddress(pollRegAddr & ~0x3);
				m_traceDumpFile->MemoryAccessWrite(memoryAddress, sizeof(uint32), "ConditionalWrite");
			}
		}
		else
		{
			// Register.
			WriteRegister(writeRegAddr, writeData);
		}
	}

	return true;
}

bool CXenonGPUExecutor::ExecutePacketType3_EVENT_WRITE(CXenonGPUCommandBufferReader& reader, const uint32 packetData, const uint32 count)
{
	CXenonGPULogBlock block(m_logWriter, "EVENT_WRITE");

	// generate an event that creates a write to memory when completed
	const uint32 initiator = reader.Read();

	// Writeback initiator
	WriteRegister(XenonGPURegister::REG_VGT_EVENT_INITIATOR, initiator & 0x3F);

	if (count == 1)
	{
		// Just an event flag? Where does this write?
	}
	else
	{
		// Write to an address
		DEBUG_CHECK(!"Unsupported write");
		reader.Advance(count - 1);
	}

	return true;
}

bool CXenonGPUExecutor::ExecutePacketType3_EVENT_WRITE_SHD(CXenonGPUCommandBufferReader& reader, const uint32 packetData, const uint32 count)
{
	CXenonGPULogBlock block(m_logWriter, "EVENT_WRITE_SHD");

	// generate a VS|PS_done event
	const uint32 initiator = reader.Read();
	const uint32 address = reader.Read();
	const uint32 value = reader.Read();

	// log
	if (m_logWriter)
	{
		m_logWriter->Writef("Initiator = %d", initiator);
		m_logWriter->Writef("Address = %08Xh", address);
		m_logWriter->Writef("Value = %08Xh", value);
	}

	// setup writeback initiator
	WriteRegister(XenonGPURegister::REG_VGT_EVENT_INITIATOR, initiator & 0x3F);

	// data to write
	uint32 writeValue;
	if ((initiator >> 31) & 0x1)
	{
		// Write frame counter
		writeValue = m_swapCounter;
	}
	else
	{
		// Write specific value
		writeValue = value;
	}

	// get value to store
	const XenonGPUEndianFormat format = static_cast< XenonGPUEndianFormat >(address & 0x3);
	const uint32 rawValue = XenonGPUSwap32(writeValue, format);

	// compute write address
	const uint32 writeAddr = GPlatform.GetMemory().TranslatePhysicalAddress(address & ~0x3);

	// store it
	*(uint32*)writeAddr = rawValue;

	// log the write
	if (m_logWriter)
		m_logWriter->Writef("MemWrite[%08Xh] = %08Xh", writeAddr, writeValue);

	// Log in trace
	if (m_traceDumpFile)
	{
		const uint32 memoryAddress = GPlatform.GetMemory().TranslatePhysicalAddress(address & ~0x3);
		m_traceDumpFile->MemoryAccessWrite(memoryAddress, sizeof(uint32), "EventWrite");
	}

	return true;
}

bool CXenonGPUExecutor::ExecutePacketType3_EVENT_WRITE_EXT(CXenonGPUCommandBufferReader& reader, const uint32 packetData, const uint32 count)
{
	// generate a screen extent event
	auto initiator = reader.Read();
	auto address = reader.Read();

	// writeback initiator
	WriteRegister(XenonGPURegister::REG_VGT_EVENT_INITIATOR, initiator & 0x3F);
	auto endianness = static_cast<XenonGPUEndianFormat>(address & 0x3);
	address &= ~0x3;

	// Let us hope we can fake this.
	uint16 extents[] = {
		0 >> 3,     // min x
		2560 >> 3,  // max x
		0 >> 3,     // min y
		2560 >> 3,  // max y
		0,          // min z
		1,          // max z
	};

	DEBUG_CHECK(endianness == XenonGPUEndianFormat::Format8in16);

	// compute write address
	const uint32 writeAddr = GPlatform.GetMemory().TranslatePhysicalAddress(address & ~0x3);
	cpu::mem::storeAddr(writeAddr + 0, extents[0]);
	cpu::mem::storeAddr(writeAddr + 2, extents[1]);
	cpu::mem::storeAddr(writeAddr + 4, extents[2]);
	cpu::mem::storeAddr(writeAddr + 6, extents[3]);
	cpu::mem::storeAddr(writeAddr + 8, extents[4]);
	cpu::mem::storeAddr(writeAddr + 10, extents[5]);
	return true;
}

bool CXenonGPUExecutor::ExecutePacketType3_DRAW_INDX(CXenonGPUCommandBufferReader& reader, const uint32 packetData, const uint32 count)
{
	CXenonGPULogBlock block(m_logWriter, "DRAW_INDX");

	// initiate fetch of index buffer and draw
	// dword0 = viz query info
	const uint32 dword0 = reader.Read();
	const uint32 dword1 = reader.Read();

	const uint32 indexCount = dword1 >> 16;
	const XenonPrimitiveType primitiveType = static_cast<XenonPrimitiveType>(dword1 & 0x3F);
	const uint32 source = (dword1 >> 6) & 0x3;

	if (source == 0x0)
	{
		// extract information
		const bool is32Bit = (dword1 >> 11) & 0x1;
		const uint32 indexAddr = reader.Read();
		const uint32 indexSize = reader.Read();

		// setup index state
		const auto indexFormat = is32Bit ? XenonIndexFormat::Index32 : XenonIndexFormat::Index16;
		const auto indexEndianess = static_cast< XenonGPUEndianFormat >(indexSize >> 30);

		// Log in trace
		if (m_traceDumpFile)
		{
			const uint32 memorySize = indexCount * (is32Bit ? 4 : 2);
			const uint32 memoryAddress = GPlatform.GetMemory().TranslatePhysicalAddress(indexAddr);
			m_traceDumpFile->MemoryAccessRead(memoryAddress, memorySize, "IndexBuffer");
		}

		// setup draw state
		CXenonGPUState::DrawIndexState ds;
		ds.m_indexData = (const void*)GPlatform.GetMemory().TranslatePhysicalAddress(indexAddr);;
		ds.m_indexFormat = indexFormat;
		ds.m_indexEndianess = indexEndianess;
		ds.m_indexCount = indexCount;
		ds.m_baseVertexIndex = m_registers[XenonGPURegister::REG_VGT_INDX_OFFSET].m_dword;
		ds.m_primitiveType = primitiveType;

		// issue draw
		if (!m_state.IssueDraw(m_abstractLayer, m_traceDumpFile, m_registers, m_registerDirtyMask, ds))
			return false;
	}
	else if (source == 0x2)
	{
		// Setup auto draw
		CXenonGPUState::DrawIndexState ds;
		ds.m_primitiveType = primitiveType;
		ds.m_indexCount = indexCount;
		ds.m_indexData = nullptr;
		ds.m_indexFormat = XenonIndexFormat::Index16;
		ds.m_indexEndianess = XenonGPUEndianFormat::FormatUnspecified;
		ds.m_baseVertexIndex = 0;//m_registers[ XenonGPURegister::REG_VGT_INDX_OFFSET ].m_dword;

		// issue draw
		if (!m_state.IssueDraw(m_abstractLayer, m_traceDumpFile, m_registers, m_registerDirtyMask, ds))
			return false;
	}
	else
	{
		// Unknown source select
		DEBUG_CHECK(!"Unknown source for DRAW_INDX");
		return false;
	}

	// cleanup register dirty mask so we can track registers that changed till next draw
	m_registerDirtyMask.ClearAll();
	return true;
}

bool CXenonGPUExecutor::ExecutePacketType3_DRAW_INDX_2(CXenonGPUCommandBufferReader& reader, const uint32 packetData, const uint32 count)
{
	CXenonGPULogBlock block(m_logWriter, "DRAW_INDX_2");

	// draw using supplied indices in packet
	const uint32 dword0 = reader.Read();
	const uint32 indexCount = dword0 >> 16;

	const XenonPrimitiveType primitiveType = static_cast< XenonPrimitiveType >(dword0 & 0x3F);

	const uint32 source = (dword0 >> 6) & 0x3;
	DEBUG_CHECK(source == 0x2);

	// setup draw state
	CXenonGPUState::DrawIndexState ds;
	ds.m_indexCount = indexCount;
	ds.m_primitiveType = primitiveType;
	ds.m_indexData = nullptr;
	ds.m_indexFormat = XenonIndexFormat::Index16;
	ds.m_indexEndianess = XenonGPUEndianFormat::FormatUnspecified;
	ds.m_baseVertexIndex = 0;

	// send to drawing
	if (!m_state.IssueDraw(m_abstractLayer, m_traceDumpFile, m_registers, m_registerDirtyMask, ds))
		return false;

	// cleanup register dirty mask so we can track registers that changed till next draw
	m_registerDirtyMask.ClearAll();
	return true;
}

bool CXenonGPUExecutor::ExecutePacketType3_SET_CONSTANT(CXenonGPUCommandBufferReader& reader, const uint32 packetData, const uint32 count)
{
	CXenonGPULogBlock block(m_logWriter, "SET_CONSTANT");

	// load constant into chip and to memory
	// PM4_REG(reg) ((0x4 << 16) | (GSL_HAL_SUBBLOCK_OFFSET(reg)))
	//                                     reg - 0x2000
	const uint32 offsetType = reader.Read();
	const uint32 index = offsetType & 0x7FF;
	const uint32 type = (offsetType >> 16) & 0xFF;

	if (m_logWriter)
	{
		m_logWriter->Writef("Index = %d", index);
		m_logWriter->Writef("Type = %d", type);
	}

	uint32 baseIndex = index;
	switch (type)
	{
		case 0:  // ALU
			baseIndex += 0x4000;
			break;

		case 1:  // FETCH
			baseIndex += 0x4800;
			break;

		case 2:  // BOOL
			baseIndex += 0x4900;
			break;

		case 3:  // LOOP
			baseIndex += 0x4908;
			break;

		case 4:  // REGISTERS
			baseIndex += 0x2000;
			break;

		default:
			DEBUG_CHECK(!"Invalid type for SET_CONSTANT");
			reader.Advance(count - 1);
			return true;
	}

	// set data
	for (uint32 n = 0; n<count - 1; n++)
	{
		const uint32 data = reader.Read();
		WriteRegister(index + n, data);
	}

	return true;
}

bool CXenonGPUExecutor::ExecutePacketType3_SET_CONSTANT2(CXenonGPUCommandBufferReader& reader, const uint32 packetData, const uint32 count)
{
	CXenonGPULogBlock block(m_logWriter, "SET_CONSTANT2");

	const uint32 offsetType = reader.Read();
	const uint32 index = offsetType & 0xFFFF;

	for (uint32 n = 0; n<count - 1; n++)
	{
		const uint32 data = reader.Read();
		WriteRegister(index + n, data);
	}

	return true;
}

bool CXenonGPUExecutor::ExecutePacketType3_LOAD_ALU_CONSTANT(CXenonGPUCommandBufferReader& reader, const uint32 packetData, const uint32 count)
{
	CXenonGPULogBlock block(m_logWriter, "LOAD_ALU_CONSTANT");

	const uint32 address = reader.Read() & 0x3FFFFFFF;
	const uint32 offset_type = reader.Read();
	uint32 index = offset_type & 0x7FF;
	const uint32 size_dwords = reader.Read() & 0xFFF;
	const uint32 type = (offset_type >> 16) & 0xFF;

	switch (type)
	{
		case 0:  // ALU
			index += 0x4000;
			break;
		case 1:  // FETCH
			index += 0x4800;
			break;
		case 2:  // BOOL
			index += 0x4900;
			break;
		case 3:  // LOOP
			index += 0x4908;
			break;
		case 4:  // REGISTERS
			index += 0x2000;
			break;
		default:
			DEBUG_CHECK(!"Invalid range");
			return true;
	}

	// Log in trace
	if (m_traceDumpFile)
	{
		const uint32 memoryAddress = GPlatform.GetMemory().TranslatePhysicalAddress(address);
		m_traceDumpFile->MemoryAccessRead(memoryAddress, size_dwords * 4, "ALUConstants");
	}

	for (uint32 n = 0; n < size_dwords; n++, index++)
	{
		const auto data = _byteswap_ulong(*(const uint32*)GPlatform.GetMemory().TranslatePhysicalAddress(address + n * 4));
		WriteRegister(index, data);
	}

	return true;
}

bool CXenonGPUExecutor::ExecutePacketType3_SET_SHADER_CONSTANTS(CXenonGPUCommandBufferReader& reader, const uint32 packetData, const uint32 count)
{
	CXenonGPULogBlock block(m_logWriter, "SET_SHADER_CONSTANTS");

	const uint32 offsetType = reader.Read();
	const uint32 index = offsetType & 0xFFFF;
	for (uint32 n = 0; n <count - 1; n++)
	{
		const uint32 data = reader.Read();
		WriteRegister(index + n, data);
	}

	return true;
}

bool CXenonGPUExecutor::ExecutePacketType3_IM_LOAD(CXenonGPUCommandBufferReader& reader, const uint32 packetData, const uint32 count)
{
	CXenonGPULogBlock block(m_logWriter, "IM_LOAD");

	// load sequencer instruction memory (pointer-based)
	const uint32 addrType = reader.Read();
	const XenonShaderType shaderType = static_cast<XenonShaderType>(addrType & 0x3);
	const uint32 addr = addrType & ~0x3;
	const uint32 startSize = reader.Read();
	const uint32 start = startSize >> 16;
	const uint32 sizeDwords = startSize & 0xFFFF;  // dwords
	DEBUG_CHECK(start == 0);

	const uint32 actualMemAddr = GPlatform.GetMemory().TranslatePhysicalAddress(addr);

	// Log in trace
	if (m_traceDumpFile)
	{
		m_traceDumpFile->MemoryAccessRead(actualMemAddr, sizeDwords * 4, shaderType == XenonShaderType::ShaderPixel ? "PixelShader" : "VertexShader");
	}

	if (shaderType == XenonShaderType::ShaderPixel)
	{
		const void* actualMem = (const void*)actualMemAddr;
		m_abstractLayer->SetPixelShader(actualMem, sizeDwords);
	}
	else if (shaderType == XenonShaderType::ShaderVertex)
	{
		const void* actualMem = (const void*)actualMemAddr;
		m_abstractLayer->SetVertexShader(actualMem, sizeDwords);
	}

	return true;
}

bool CXenonGPUExecutor::ExecutePacketType3_IM_LOAD_IMMEDIATE(CXenonGPUCommandBufferReader& reader, const uint32 packetData, const uint32 count)
{
	CXenonGPULogBlock block(m_logWriter, "IM_LOAD_IMMEDIATE");

	// load sequencer instruction memory (code embedded in packet)
	const uint32 dword0 = reader.Read();
	const uint32 dword1 = reader.Read();

	const XenonShaderType shaderType = static_cast<XenonShaderType>(dword0);

	const uint32 startSize = dword1;
	const uint32 start = startSize >> 16;
	const uint32 sizeDwords = startSize & 0xFFFF;  // dwords

	DEBUG_CHECK(start == 0);

	// Log in trace
	if (m_traceDumpFile)
	{
		auto* actualMem = (uint32*)alloca(sizeof(uint32) * sizeDwords);
		reader.GetBatch(sizeDwords, actualMem);

		m_traceDumpFile->MemoryAccessRead((uint32)actualMem, sizeDwords * 4, shaderType == XenonShaderType::ShaderPixel ? "PixelShader" : "VertexShader");
	}

	if (shaderType == XenonShaderType::ShaderPixel)
	{
		auto* actualMem = (uint32*)alloca(sizeof(uint32) * sizeDwords);
		reader.GetBatch(sizeDwords, actualMem);

		m_abstractLayer->SetPixelShader(actualMem, sizeDwords);
	}
	else if (shaderType == XenonShaderType::ShaderVertex)
	{
		auto* actualMem = (uint32*)alloca(sizeof(uint32) * sizeDwords);
		reader.GetBatch(sizeDwords, actualMem);

		m_abstractLayer->SetVertexShader(actualMem, sizeDwords);
	}

	reader.Advance(sizeDwords);
	return true;
}

bool CXenonGPUExecutor::ExecutePacketType3_INVALIDATE_STATE(CXenonGPUCommandBufferReader& reader, const uint32 packetData, const uint32 count)
{
	const uint32 stateMask = reader.Read();
	//m_state.Invalidate( stateMask );
	return true;
}

void CXenonGPUExecutor::WriteRegister(const XenonGPURegister registerIndex, const uint32 registerData)
{
	WriteRegister((const uint32)registerIndex, registerData);
}

void CXenonGPUExecutor::WriteRegister(const uint32 registerIndex, const uint32 registerData)
{
	if (registerIndex >= m_registers.NUM_REGISTERS)
	{
		GLog.Err("GPU: Trying to write to non existing register 0x%08X", registerIndex);
		return;
	}

	// Direct write
	if (registerData != m_registers[registerIndex].m_dword)
	{
		m_registers[registerIndex].m_dword = registerData;
		m_registerDirtyMask.Set(registerIndex);
	}

	// Log
	if (m_logWriter)
		m_logWriter->Writef("Register[%04Xh] = %08Xh (%d)", registerIndex, registerData, registerData);

	// Trace
	const auto& info = m_registers.GetInfo(registerIndex);

	// COHER - looks like they are needed to synchronize memory
	if (registerIndex == (uint32)XenonGPURegister::REG_COHER_STATUS_HOST)
	{
		m_registers[registerIndex].m_dword |= 0x80000000ul;
		m_registerDirtyMask.Set(registerIndex);
	}

	// Scratch register writeback
	if (registerIndex >= (uint32)XenonGPURegister::REG_SCRATCH_REG0 &&
		registerIndex <= (uint32)XenonGPURegister::REG_SCRATCH_REG7)
	{
		const uint32 scratchRegIndex = registerIndex - (uint32)XenonGPURegister::REG_SCRATCH_REG0;

		const uint32 scrachMemMask = m_registers[XenonGPURegister::REG_SCRATCH_UMSK].m_dword;
		if ((1 << scratchRegIndex) & scrachMemMask)
		{
			// Writing is enabled
			const uint32 scratchAddr = m_registers[XenonGPURegister::REG_SCRATCH_ADDR].m_dword;
			const uint32 memAddr = scratchAddr + (scratchRegIndex * 4);

			// write
			const uint32 writeAddr = GPlatform.GetMemory().TranslatePhysicalAddress(memAddr & ~0x3);
			cpu::mem::storeAddr< uint32 >(writeAddr, registerData);

			// Add to trace
			if (m_traceDumpFile)
				m_traceDumpFile->MemoryAccessWrite(GPlatform.GetMemory().TranslatePhysicalAddress(memAddr & ~0x3), sizeof(uint32), "RegisterWriteBack");

			// Add to log
			if (m_logWriter)
				m_logWriter->Writef("GPU WRITE at %08Xh with %08Xh (%d)", writeAddr, registerData, registerData);
		}
	}
}

void CXenonGPUExecutor::MakeCoherent()
{
	// http://amd-dev.wpengine.netdna-cdn.com/wordpress/media/2013/10/R6xx_R7xx_3D.pdf
	// http://cgit.freedesktop.org/xorg/driver/xf86-video-radeonhd/tree/src/r6xx_accel.c?id=3f8b6eccd9dba116cc4801e7f80ce21a879c67d2#n454

	const uint32 statusHost = m_registers[XenonGPURegister::REG_COHER_STATUS_HOST].m_dword;
	const uint32 baseHost = m_registers[XenonGPURegister::REG_COHER_BASE_HOST].m_dword;
	const uint32 sizeHost = m_registers[XenonGPURegister::REG_COHER_SIZE_HOST].m_dword;

	if (!(statusHost & 0x80000000ul))
		return;

	const uint32 newStatusHost = statusHost & ~0x80000000ul;
	m_registers[XenonGPURegister::REG_COHER_STATUS_HOST].m_dword = newStatusHost;
	m_registerDirtyMask.Set((uint32)XenonGPURegister::REG_COHER_STATUS_HOST);

	// Log
	if (m_logWriter)
		m_logWriter->Writef("MakeCoherent()");

	// TODO:
	//m_scratch.ClearCache();
}

void CXenonGPUExecutor::BeingWait()
{
	// not implemented
}

void CXenonGPUExecutor::FinishWait()
{
	// not implemented
}

void CXenonGPUExecutor::DispatchInterrupt(const uint32 source, const uint32 cpu)
{
	if (!m_interruptAddr)
		return;

	uint32 cpuIndex = cpu;
	if (cpuIndex == 0xFFFFFFFF)
		cpuIndex = 2;

	// Log
	if (m_logWriter)
		m_logWriter->Writef("Dispatching CPU interrupt at %08Xh (source: %08Xh, data: %08Xh)", m_interruptAddr, source, m_interruptUserData);

	uint64 args[2] = { source, m_interruptUserData };
	const bool trace = (source == 1);
	GPlatform.GetKernel().ExecuteInterrupt(cpuIndex, m_interruptAddr, args, ARRAYSIZE(args), trace);
}
