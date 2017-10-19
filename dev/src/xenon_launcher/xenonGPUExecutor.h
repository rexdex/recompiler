#pragma once

#include "xenonGPURegisters.h"
#include "xenonGPUState.h"

class CXenonGPUCommandBufferReader;
class IXenonGPUDumpWriter;
class CXenonGPUTraceWriter;

/// GPU command executor, translates GPU commands from ring/command buffer (& registers) into state changes in the abstract layer
class CXenonGPUExecutor
{
public:
	CXenonGPUExecutor( IXenonGPUAbstractLayer* abstractionLayer, const launcher::Commandline& cmdLine );
	~CXenonGPUExecutor();

	/// Process command from command buffer
	void Execute( CXenonGPUCommandBufferReader& reader );	

	// set internal interrupt callback
	void SetInterruptCallbackAddr( const uint32 addr, const uint32 userData );

	/// Signal external VBlank
	void SignalVBlank();

	/// Read register value
	uint64 ReadRegister( const uint32 registerIndex );

	/// Request a trace dump of the next frame
	void RequestTraceDump();

private:	
	// gpu abstraction layer (owned externally)
	IXenonGPUAbstractLayer*		m_abstractLayer;

	// gpu trace dumper
	IXenonGPUDumpWriter*		m_traceDumpFile;
	std::atomic< bool >			m_traceDumpRequested;

	// gpu log
	CXenonGPUTraceWriter*		m_logWriter;

	// gpu internal register map
	CXenonGPURegisters				m_registers;
	CXenonGPUDirtyRegisterTracker	m_registerDirtyMask;

	// gpu state management
	CXenonGPUState				m_state;

	// tiled rendering packet mask
	uint64		m_tiledMask;
	uint64		m_tiledSelector;

	// internal swap counter
	std::atomic< uint32	>	m_swapCounter;
	std::atomic< uint32	>	m_vblankCounter;

	// interrupt
	uint32		m_interruptAddr;
	uint32		m_interruptUserData;

	// command buffer execution (pasted from AMD driver)
	void ExecutePrimaryBuffer( CXenonGPUCommandBufferReader& reader );
	bool ExecutePacket( CXenonGPUCommandBufferReader& reader );

	// packet types
	bool ExecutePacketType0( CXenonGPUCommandBufferReader& reader, const uint32 packetData );
	bool ExecutePacketType1( CXenonGPUCommandBufferReader& reader, const uint32 packetData );
	bool ExecutePacketType2( CXenonGPUCommandBufferReader& reader, const uint32 packetData );
	bool ExecutePacketType3( CXenonGPUCommandBufferReader& reader, const uint32 packetData );

	// Type3 packets
	bool ExecutePacketType3_ME_INIT( CXenonGPUCommandBufferReader& reader, const uint32 packetData, const uint32 count );
	bool ExecutePacketType3_NOP( CXenonGPUCommandBufferReader& reader, const uint32 packetData, const uint32 count );
	bool ExecutePacketType3_INTERRUPT( CXenonGPUCommandBufferReader& reader, const uint32 packetData, const uint32 count );
	bool ExecutePacketType3_INDIRECT_BUFFER( CXenonGPUCommandBufferReader& reader, const uint32 packetData, const uint32 count );
	bool ExecutePacketType3_WAIT_REG_MEM( CXenonGPUCommandBufferReader& reader, const uint32 packetData, const uint32 count );
	bool ExecutePacketType3_REG_RMW( CXenonGPUCommandBufferReader& reader, const uint32 packetData, const uint32 count );
	bool ExecutePacketType3_COND_WRITE( CXenonGPUCommandBufferReader& reader, const uint32 packetData, const uint32 count );
	bool ExecutePacketType3_EVENT_WRITE( CXenonGPUCommandBufferReader& reader, const uint32 packetData, const uint32 count );
	bool ExecutePacketType3_EVENT_WRITE_SHD( CXenonGPUCommandBufferReader& reader, const uint32 packetData, const uint32 count );
	bool ExecutePacketType3_EVENT_WRITE_EXT( CXenonGPUCommandBufferReader& reader, const uint32 packetData, const uint32 count );
	bool ExecutePacketType3_DRAW_INDX( CXenonGPUCommandBufferReader& reader, const uint32 packetData, const uint32 count );
	bool ExecutePacketType3_DRAW_INDX_2( CXenonGPUCommandBufferReader& reader, const uint32 packetData, const uint32 count );
	bool ExecutePacketType3_SET_CONSTANT( CXenonGPUCommandBufferReader& reader, const uint32 packetData, const uint32 count );
	bool ExecutePacketType3_SET_CONSTANT2( CXenonGPUCommandBufferReader& reader, const uint32 packetData, const uint32 count );
	bool ExecutePacketType3_LOAD_ALU_CONSTANT( CXenonGPUCommandBufferReader& reader, const uint32 packetData, const uint32 count );
	bool ExecutePacketType3_SET_SHADER_CONSTANTS( CXenonGPUCommandBufferReader& reader, const uint32 packetData, const uint32 count );
	bool ExecutePacketType3_IM_LOAD( CXenonGPUCommandBufferReader& reader, const uint32 packetData, const uint32 count );
	bool ExecutePacketType3_IM_LOAD_IMMEDIATE( CXenonGPUCommandBufferReader& reader, const uint32 packetData, const uint32 count );
	bool ExecutePacketType3_INVALIDATE_STATE( CXenonGPUCommandBufferReader& reader, const uint32 packetData, const uint32 count );

	// Hacks
	bool ExecutePacketType3_HACK_SWAP( CXenonGPUCommandBufferReader& reader, const uint32 packetData, const uint32 count );

	// reg access
	void WriteRegister( const uint32 registerIndex, const uint32 registerData );
	void WriteRegister( const XenonGPURegister registerIndex, const uint32 registerData );

	// make state coherent
	void MakeCoherent();

	// waiting state
	void BeingWait();
	void FinishWait();

	// dispatch processor interrupt
	void DispatchInterrupt( const uint32 source, const uint32 cpu );
};