#include "build.h"
#include "internalUtils.h"

#include "platformCPU.h"
#include "platformLibrary.h"

#include "decodingEnvironment.h"
#include "decodingInstruction.h"
#include "decodingInstructionInfo.h"
#include "decodingContext.h"

#include "traceData.h"
#include "traceUtils.h"

#include "timemachine.h"

namespace timemachine
{

	//----

	Trace::Trace()
		: m_rootEntry( nullptr )
		, m_traceReader( nullptr )
	{
	}

	Trace::~Trace()
	{
		DeleteVector( m_entries );	
		m_rootEntry = nullptr;

		if ( m_traceReader )
		{
			delete m_traceReader;
			m_traceReader = nullptr;
		}
	}

	const Entry* Trace::GetTraceEntry( ILogOutput& log, const uint32 traceFrameIndex )
	{
		// find existing
		auto it = m_entriesMap.find( traceFrameIndex );
		if ( it != m_entriesMap.end() )
			return it->second;

		// create new entry
		Entry* newEntry = Entry::CreateEntry( log, m_context, this, m_traceReader, traceFrameIndex );
		if ( !newEntry )
			return nullptr;

		// add to map
		m_entries.push_back( newEntry );
		m_entriesMap[ traceFrameIndex ] = newEntry;
		return newEntry;
	}

	Trace* Trace::CreateTimeMachine( decoding::Context* context, const trace::Data* traceData, const uint32 traceFrameIndex )
	{
		// no or invalid trace data
		if ( !traceData || traceFrameIndex >= traceData->GetNumDataFrames() )
			return nullptr;

		// create reader
		trace::DataReader* traceReader = traceData->CreateReader();
		if ( !traceReader )
			return nullptr;

		// create data holder
		std::auto_ptr<Trace> timeMachineTrace( new Trace() );
		timeMachineTrace->m_traceReader = traceReader;
		timeMachineTrace->m_rootEntry = Entry::CreateEntry( ILogOutput::DevNull(), context, timeMachineTrace.get(), traceReader, traceFrameIndex );
		if ( !timeMachineTrace->m_rootEntry )
			return nullptr;

		// add as first entry
		timeMachineTrace->m_entries.push_back( timeMachineTrace->m_rootEntry );
		timeMachineTrace->m_entriesMap[ traceFrameIndex ] = timeMachineTrace->m_rootEntry;
		return timeMachineTrace.release();
	}

	//----

	/// Dependency on previous memory location
	class DependencyMemoryLoad : public Entry::AbstractDependency
	{
	public:
		DependencyMemoryLoad( decoding::Context* context, const int32 traceFrame, const Entry* entry, const uint64 addr, const uint32 size )
			: m_traceFrame( traceFrame )
			, m_memoryAddress( addr )
			, m_memorySize( size )
			, m_entry( entry )
			, m_context( context )
		{}

		virtual const Entry* GetEntry() const override
		{
			return m_entry;
		}

		virtual std::string GetCaption() const override
		{
			char buf[512];
			sprintf_s( buf, ARRAYSIZE(buf), "[0x%08llX]", m_memoryAddress );
			return buf;
		}

		virtual std::string GetValue( Trace* trace ) const override
		{
			/*// get the trace frame shit
			const trace::DataFrame& frame = trace->GetTraceReader()->GetFrame( m_traceFrame );
			const uint32 codeAddress = frame.GetAddress();

			// decode instruction
			decoding::Instruction op;
			auto* context = decoding::Environment::GetInstance().GetDecodingContext();
			if (!context->GetInstructionCache().DecodeInstruction(ILogOutput::DevNull(), codeAddress, op, false))
				return "";

			// get additional info
			InstructionExtendedInfo info;
			if (!op.GetExtendedInfo(codeAddress, *context, info))
				return "";

			// the shit loaded into the memory is in the target register
			if ( info.m_registersModifiedCount == 0 )
				return "";
			const auto* reg = info.m_registersModified[0];

			// get mapped register index
			const int regIndex = trace->GetTraceReader()->GetData().GetRegisterMapping().FindRegisterIndex(reg);
			if (regIndex == -1)
				return "";

			// get the value on the input (in this frame)
			const auto& regValue = frame.GetValue( regIndex );

			// convert to text
			char text[128];
			GetRegisterValueTextRaw( regValue, reg, text, ARRAYSIZE(text) );
			return text;*/
			return "";
		}

		inline const bool MatchesMemoryAccess( const uint64 addr, const uint32 size ) const
		{
			const uint64 addrEnd = addr + size;
			if ( addr < (m_memoryAddress + m_memorySize) && (m_memoryAddress < addrEnd) )
			{
				return true;
			}

			return false;
		}

		virtual void Resolve( ILogOutput& log, Trace* trace, std::vector< const Entry::AbstractSource* >& outDependencies ) const override
		{
			// end of memory range we are testing
			const uint64 memoryAddressEnd = m_memorySize + m_memoryAddress;

			// address mask (marks bytes that have already been written to)
			uint32 memoryMask = (1 << m_memorySize) - 1;

			// while not at the end
			uint32 entryIndex = m_traceFrame - 1;
			while (entryIndex > 0)
			{
				// task progress
				log.SetTaskProgress( m_traceFrame-entryIndex, m_traceFrame );

				// no more bytes to check
				if ( memoryMask == 0 )
					break;

				// lookup canceled
				if ( log.IsTaskCanceled() )
					break;

				// get decoded frame
				const uint32 thisEntryIndex = entryIndex--;
				const trace::DataFrame& frame = trace->GetTraceReader()->GetFrame(thisEntryIndex);
				const uint32 codeAddress = frame.GetAddress();

				// decode instruction
				decoding::Instruction op;
				if (!m_context->DecodeInstruction(ILogOutput::DevNull(), codeAddress, op, false))
					continue;

				// get additional info
				decoding::InstructionExtendedInfo info;
				if (!op.GetExtendedInfo(codeAddress, *m_context, info))
					continue;

				// we are looking for a write to that location
				uint8 memFlags = 0;
				if ( !(info.m_memoryFlags & decoding::InstructionExtendedInfo::eMemoryFlags_Write ) )
					continue;

				// compute memory range for this instruction
				uint64 currentMemoryAddress = 0;
				if (!info.ComputeMemoryAddress( frame, currentMemoryAddress))
					continue;

				// address in range ?
				uint64 currentMemoryAddressEnd = currentMemoryAddress + info.m_memorySize;
				if ((currentMemoryAddressEnd > m_memoryAddress) && (currentMemoryAddress < memoryAddressEnd))
				{
					// find the source
					const Entry* otherEntry = trace->GetTraceEntry( log, thisEntryIndex );
					if ( otherEntry )
					{
						// find matching source
						const uint32 numSources = otherEntry->GetNumSources();
						for ( uint32 j=0; j<numSources; ++j )
						{
							const Entry::AbstractSource* source = otherEntry->GetSource(j);
							if ( source->IsSourceFor( this ) )
							{
								// we've found trace entry touching given address
								outDependencies.push_back( source );

								// clear the memory bytes that were set
								uint32 numBytesSet = 0;
								for ( uint32 k=0; k<info.m_memorySize; ++k )
								{
									uint64 addr = currentMemoryAddress + k;
									if ( addr >= m_memoryAddress )
									{
										uint64 byteIndex = addr - m_memoryAddress;
										if ( byteIndex < m_memorySize )
										{
											const uint32 mask = 1 << byteIndex;
											if ( memoryMask & mask )
											{
												memoryMask &= ~mask;
												numBytesSet += 1;
											}
										}
									}
								}

								if ( numBytesSet )
								{
									log.Log( "TimeMachine: Mem at 0x%08X, trace #%05u: 0x%08X, size %u changed %u bytes", 
										codeAddress, thisEntryIndex, currentMemoryAddress, info.m_memorySize, numBytesSet );
								}
							}
						}
					}
				}
			}
		}

	private:
		uint32				m_traceFrame;
		uint64				m_memoryAddress;
		uint32				m_memorySize;
		const Entry*		m_entry;
		decoding::Context*	m_context;
	};

	//----

	/// Dependency on previous register value
	class DependencyResisterValue : public Entry::AbstractDependency
	{
	public:
		DependencyResisterValue( decoding::Context* context, const int32 traceFrame, const Entry* entry, const platform::CPURegister* reg )
			: m_traceFrame( traceFrame )
			, m_entry( entry )
			, m_reg( reg ) 
			, m_context( context )
		{}
	
		virtual const Entry* GetEntry() const override
		{
			return m_entry;
		}

		virtual std::string GetCaption() const override
		{
			return m_reg->GetName();
		}

		virtual std::string GetValue( Trace* trace ) const override
		{
			// get the trace frame shit
			const trace::DataFrame& frame = trace->GetTraceReader()->GetFrame( m_traceFrame );
			const uint32 codeAddress = frame.GetAddress();

			// get the value on the input (in this frame)
			return trace::GetRegisterValueText(m_reg, frame);
		}

		inline const platform::CPURegister* GetRegister() const
		{
			return m_reg;
		}

		virtual void Resolve( ILogOutput& log, Trace* trace, std::vector< const Entry::AbstractSource* >& outDependencies ) const override
		{
			// while not at the end
			uint32 entryIndex = m_traceFrame - 1;
			bool found = false;
			while (entryIndex > 0 && !found)
			{
				// task progress
				log.SetTaskProgress( m_traceFrame-entryIndex, m_traceFrame );

				// lookup canceled
				if ( log.IsTaskCanceled() )
					break;

				// get decoded frame
				const uint32 thisEntryIndex = entryIndex--;
				const trace::DataFrame& frame = trace->GetTraceReader()->GetFrame(thisEntryIndex);
				const uint32 codeAddress = frame.GetAddress();

				// decode instruction
				decoding::Instruction op;
				if (!m_context->DecodeInstruction(ILogOutput::DevNull(), codeAddress, op, false))
					continue;

				// get additional info
				decoding::InstructionExtendedInfo info;
				if (!op.GetExtendedInfo(codeAddress, *m_context, info))
					continue;

				// we are looking for a instruction that WROTE to that location
				for ( uint32 i=0; i<info.m_registersModifiedCount; ++i )
				{
					if ( info.m_registersModified[i] == m_reg )
					{
						// find the source
						const Entry* otherEntry = trace->GetTraceEntry( log, thisEntryIndex );
						if ( otherEntry )
						{
							// find matching source
							const uint32 numSources = otherEntry->GetNumSources();
							for ( uint32 j=0; j<numSources; ++j )
							{
								const Entry::AbstractSource* source = otherEntry->GetSource(j);
								if ( source->IsSourceFor( this ) )
								{
									// we've found trace entry touching given address
									outDependencies.push_back( source );
									found = true;
								}
							}

							// debug log
							if ( !found )
							{
								log.Log( "TimeMachine: almost a source at #%05u", thisEntryIndex );
							}
						}
					}
				}
			}
		}

	private:
		uint32							m_traceFrame;
		const platform::CPURegister*	m_reg;
		decoding::Context*				m_context;
		const Entry*					m_entry;
	};

	//----

	class SourceMemoryWriter : public Entry::AbstractSource
	{
	public:
		SourceMemoryWriter( decoding::Context* context, const uint32 traceFrameIndex, const Entry* entry, const uint64 memoryAddr, const uint32 memorySize )
			: m_traceFrame( traceFrameIndex )
			, m_memoryAddress( memoryAddr )
			, m_memorySize( memorySize )
			, m_entry( entry )
			, m_context( context )
		{}

		virtual const Entry* GetEntry() const override
		{
			return m_entry;
		}

		virtual std::string GetCaption() const
		{
			char buf[512];
			sprintf_s( buf, ARRAYSIZE(buf), "[0x%08llX]", m_memoryAddress );
			return buf;
		}

		virtual std::string GetValue( Trace* trace ) const override
		{
			/*// get the trace frame shit
			const trace::DataFrame& frame = trace->GetTraceReader()->GetFrame( m_traceFrame );
			const uint32 codeAddress = frame.GetAddress();

			// decode instruction
			decoding::Instruction op;
			auto* context = decoding::Environment::GetInstance().GetDecodingContext();
			if (!context->GetInstructionCache().DecodeInstruction(ILogOutput::DevNull(), codeAddress, op, false))
				return "";

			// get additional info
			InstructionExtendedInfo info;
			if (!op.GetExtendedInfo(codeAddress, *context, info))
				return "";

			// the shit loaded into the memory is in the source register
			if ( info.m_registersDependencies == 0 )
				return "";
			const auto* reg = info.m_registersDependencies[0];

			// get mapped register index
			const int regIndex = trace->GetTraceReader()->GetData().GetRegisterMapping().FindRegisterIndex(reg);
			if (regIndex == -1)
				return "";

			// get the NEXT TRACE FRAME to see what is the value of the register
			const trace::DataFrame& nextFrame = trace->GetTraceReader()->GetFrame( m_traceFrame+1 );
			const auto& regValue = nextFrame.GetValue( regIndex );

			// convert to text
			char text[128];
			GetRegisterValueTextRaw( regValue, reg, text, ARRAYSIZE(text) );
			return text;*/
			return "";
		}

		virtual bool IsSourceFor( const Entry::AbstractDependency* dependency ) const
		{
			const DependencyMemoryLoad* load = dynamic_cast< const DependencyMemoryLoad* >( dependency );
			return load && load->MatchesMemoryAccess( m_memoryAddress, m_memorySize );
		}

		virtual void Resolve( ILogOutput& log, Trace* trace, std::vector< const Entry::AbstractDependency* >& outDependencies ) const override
		{
			// nothing for now
		}

	private:
		uint32				m_traceFrame;
		uint64				m_memoryAddress;
		uint32				m_memorySize;
		decoding::Context*	m_context;
		const Entry*		m_entry; 
	};

	//----

	class SourceRegisterWrite : public Entry::AbstractSource
	{
	public:
		SourceRegisterWrite( decoding::Context* context, const uint32 traceFrameIndex, const Entry* entry, const platform::CPURegister* reg )
			: m_traceFrame( traceFrameIndex )
			, m_entry( entry )
			, m_reg( reg )
			, m_context( context )
		{}

		virtual const Entry* GetEntry() const override
		{
			return m_entry;
		}

		virtual std::string GetCaption() const
		{
			return m_reg->GetName();
		}

		virtual std::string GetValue( Trace* trace ) const override
		{
			// get the trace frame shit
			const trace::DataFrame& frame = trace->GetTraceReader()->GetFrame( m_traceFrame );
			const uint32 codeAddress = frame.GetAddress();

			// get the NEXT TRACE FRAME to see what is the value of the register
			const trace::DataFrame& nextFrame = trace->GetTraceReader()->GetFrame( m_traceFrame+1 );
			return trace::GetRegisterValueText(m_reg, nextFrame);
		}

		virtual bool IsSourceFor( const Entry::AbstractDependency* dependency ) const
		{
			const DependencyResisterValue* regAccess = dynamic_cast< const DependencyResisterValue* >( dependency );
			return regAccess && (regAccess->GetRegister() == m_reg);
		}

		virtual void Resolve( ILogOutput& log, Trace* trace, std::vector< const Entry::AbstractDependency* >& outDependencies ) const override
		{
			// nothing for now
		}

	private:
		uint32							m_traceFrame;
		const Entry*					m_entry;
		const platform::CPURegister*	m_reg;
		decoding::Context*				m_context;
	};

	//----

	Entry::Entry( decoding::Context* context, Trace* trace, const uint32 traceIndex )
		: m_traceIndex( traceIndex )
		, m_codeAddress( 0 )
		, m_display("???")
		, m_context( context )
		, m_trace( trace )
	{
	}

	Entry::~Entry()
	{	
	}

	Entry* Entry::CreateEntry( class ILogOutput& log, decoding::Context* context, Trace* trace, class trace::DataReader* reader, const int32 traceEntryIndex )
	{
		// get data from trace reader
		const auto& frame = reader->GetFrame( traceEntryIndex  );
		if ( !frame.GetAddress() )
			return nullptr;

		// extract basic information
		const auto codeAddress = frame.GetAddress();

		// get display value - decoded instruction
		decoding::Instruction instruction;
		if ( 0 == context->DecodeInstruction( log, codeAddress, instruction ) )
			return nullptr;

		// get instruction name
		char name[512];
		char* writer = &name[0];
		instruction.GenerateText( codeAddress, writer, writer + ARRAYSIZE(name) );

		// get extended instruction information
		decoding::InstructionExtendedInfo info;
		if ( !instruction.GetExtendedInfo( codeAddress, *context, info ) )
			return nullptr;

		// create output entry
		Entry* entry = new Entry( context, trace, traceEntryIndex );
		entry->m_display = name;
		entry->m_codeAddress = codeAddress;

		// memory access
		if ( info.m_memoryFlags & decoding::InstructionExtendedInfo::eMemoryFlags_Read )
		{
			// calculate memory address being accessed
			uint64 computedMemoryAddress = 0;
			if ( info.ComputeMemoryAddress( frame, computedMemoryAddress ) )
			{
				entry->m_abstarctDeps.push_back( new DependencyMemoryLoad( context, traceEntryIndex, entry, computedMemoryAddress, info.m_memorySize ) );
			}
		}
		else if ( info.m_memoryFlags & decoding::InstructionExtendedInfo::eMemoryFlags_Write )
		{
			// calculate memory address being accessed
			uint64 computedMemoryAddress = 0;
			if ( info.ComputeMemoryAddress( frame, computedMemoryAddress ) )
			{
				entry->m_abstractSources.push_back( new SourceMemoryWriter( context, traceEntryIndex, entry, computedMemoryAddress, info.m_memorySize ) );
			}
		}

		// register dependency
		for ( uint32 i=0; i<info.m_registersDependenciesCount; ++i )
		{
			const auto* reg = info.m_registersDependencies[i];
			entry->m_abstarctDeps.push_back( new DependencyResisterValue( context, traceEntryIndex, entry, reg ) );
		}

		// register sources
		for ( uint32 i=0; i<info.m_registersModifiedCount; ++i )
		{
			const auto* reg = info.m_registersModified[i];
			entry->m_abstractSources.push_back( new SourceRegisterWrite( context, traceEntryIndex, entry, reg ) );
		}

		// return created entry
		return entry;
	}

	//-----

} // timemachine