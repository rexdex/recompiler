#include "build.h"
#include "dx11MicrocodeNodes.h"
#include "dx11MicrocodeBlocks.h"
#include "dx11MicrocodeBlocksTranslator.h"
#include "dx11MicrocodeHLSLWriter.h"
#include <algorithm>

//#pragma optimize( "", off )

using namespace DX11Microcode;

//-----------------------------------------------------------------------------

Block::Block( uint32 address, Statement* preamble, Statement* code, ExprNode* condition )
	: m_address( address )
	, m_targetAddrses( 0 )
	, m_target( nullptr )
	, m_type( EBlockType::EXEC )
	, m_continuation( nullptr )
{
	m_condition = condition ? condition->CopyWithType< ExprNode >() : nullptr;
	m_code = code ? code->CopyWithType< Statement >() : nullptr;
	m_preamble = preamble ? preamble->CopyWithType< Statement >() : nullptr;
}

Block::Block( ExprNode* condition, uint32 targetAddress, EBlockType type )
	: m_address( 0 )
	, m_targetAddrses( targetAddress )
	, m_target( nullptr )
	, m_type( type )
	, m_code( nullptr )
	, m_continuation( nullptr )
	, m_preamble( nullptr )
{
	m_condition = condition ? condition->CopyWithType< ExprNode >() : nullptr;
}

Block::~Block()
{
	if ( m_code )
	{
		m_code->Release();
		m_code = nullptr;
	}

	if ( m_condition )
	{
		m_condition->Release();
		m_condition = nullptr;
	}

	if ( m_preamble )
	{
		m_preamble->Release();
		m_preamble = nullptr;
	}
}

void Block::ConnectTarget( Block* targetBlock )
{
	DEBUG_CHECK( m_targetAddrses != 0 );
	DEBUG_CHECK( m_target == nullptr );
	DEBUG_CHECK( targetBlock != this )
	DEBUG_CHECK( targetBlock != nullptr );

	targetBlock->m_sources.push_back( this );
	m_target = targetBlock;
}

void Block::ConnectContinuation( Block* nextBlock )
{
	DEBUG_CHECK( m_continuation == nullptr );
	DEBUG_CHECK( nextBlock != this );
	DEBUG_CHECK( nextBlock != nullptr );
	m_continuation = nextBlock;
}

void Block::EmitHLSL( class IHLSLWriter& writer ) const
{
	// emit preamble (outside the conditional block)
	if ( m_preamble )
		m_preamble->EmitHLSL( writer );

	// each block can have condition
	if ( m_condition )
		writer.BeingCondition( m_condition->EmitHLSL( writer ) );

	// emit the block content
	switch ( m_type )
	{
		case Block::EBlockType::CALL:
		{
			DEBUG_CHECK( m_target != nullptr );
			writer.ControlFlowCall( m_target->GetAddress() );
			break;
		}

		case Block::EBlockType::JUMP:
		{
			DEBUG_CHECK( m_target != nullptr );
			writer.ControlFlowJump( m_target->GetAddress() );
			break;
		}

		case Block::EBlockType::RET:
		{
			DEBUG_CHECK( m_target != nullptr );
			writer.ControlFlowReturn( m_target->GetAddress() );
			break;
		}

		case Block::EBlockType::END:
		{
			writer.ControlFlowEnd();
			break;
		}

		case Block::EBlockType::EXEC:
		{
			DEBUG_CHECK( m_code != nullptr );
			m_code->EmitHLSL( writer );
			break;
		}
	}		

	// each block can have condition
	if ( m_condition )
		writer.EndCondition();
}

//-----------------------------------------------------------------------------

ControlFlowGraph::ControlFlowGraph()
{
}

ControlFlowGraph::~ControlFlowGraph()
{
	for ( auto* ptr : m_blocks )
		delete ptr;
}


ControlFlowGraph* ControlFlowGraph::DecompileMicroCode( const void* code, const uint32 codeLength, XenonShaderType shaderType )
{
	// count number of words
	if ( codeLength & 3 )
		return nullptr;

	// setup translator
	CXenonGPUMicrocodeTransformer transformer( shaderType );

	// create translation helper
	BlockTranslator blockTranslator;
	transformer.TransformShader( blockTranslator, (const uint32*)code, codeLength / sizeof(uint32) );

	// no blocks extracted
	if ( blockTranslator.GetNumCreatedBlocks() == 0 )
		return nullptr;

	// extract control flow blocks
	ControlFlowGraph* ret = new ControlFlowGraph();
	ret->m_blocks.reserve( blockTranslator.GetNumCreatedBlocks() );

	// copy all blocks
	const uint32 numBlocks = blockTranslator.GetNumCreatedBlocks();
	for ( uint32 i=0; i<numBlocks; ++i )
	{
		ret->m_blocks.push_back( blockTranslator.GetCreatedBlock(i) );
	}

	// setup automatic continuations
	for ( uint32 i=0; i<numBlocks-1; ++i )
	{
		Block* block = blockTranslator.GetCreatedBlock(i);
		Block* nextBlock = blockTranslator.GetCreatedBlock(i+1);

		// can we continue into the next block ?
		const auto type = block->GetType();
		// TODO: this logic is broken 
		if ( type != Block::EBlockType::END && type != Block::EBlockType::RET )
		{
			// TODO: detect unconditional jumps/calls
			if ( type == Block::EBlockType::JUMP || type == Block::EBlockType::CALL )
			{
				/*if ( block->IsUnconditional() )
					continue;*/
			}

			// set next block that will execute
			block->ConnectContinuation( nextBlock );
		}
	}		

	// connect control flow blocks - if this fails it means the extracted shader is fishy
	std::map< uint32, Block* > blocksWithAddresses;
	for ( uint32 i=0; i<numBlocks; ++i )
	{
		Block* block = blockTranslator.GetCreatedBlock(i);
		if ( block->GetType() == Block::EBlockType::EXEC )
		{
			if ( blocksWithAddresses[ block->GetAddress() ] != nullptr )
			{
				GLog.Err( "Microcode: Two blocks with the same microcode address %d", block->GetAddress() );
				return nullptr;
			}

			// add to list
			blocksWithAddresses[ block->GetAddress() ] = block;
		}
	}

	// connect jumps and calls, generate the list of root blocks (main + each call)
	std::set< Block* > rootBlocks;
	for ( uint32 i=0; i<numBlocks; ++i )
	{
		Block* block = blockTranslator.GetCreatedBlock(i);
		if ( block->GetType() == Block::EBlockType::JUMP )
		{
			DEBUG_CHECK( block->GetTargetAddress() != 0 );

			Block* targetBlock = blocksWithAddresses[ block->GetTargetAddress() ];
			if ( targetBlock == nullptr )
			{
				GLog.Err( "Microcode: No target block with address %d for jump.", block->GetTargetAddress() );
				return nullptr;
			}

			block->ConnectTarget( targetBlock );
		}
		else if ( block->GetType() == Block::EBlockType::CALL )
		{
			DEBUG_CHECK( block->GetTargetAddress() != 0 );

			Block* targetBlock = blocksWithAddresses[ block->GetTargetAddress() ];
			if ( targetBlock == nullptr )
			{
				GLog.Err( "Microcode: No target block with address %d for call.", block->GetTargetAddress() );
				return nullptr;
			}

			block->ConnectTarget( targetBlock );
			rootBlocks.insert( targetBlock );
		}
	}

	// store the roots, the starting block is the implicit root
	ret->m_rootBlocks.push_back( ret->m_blocks[0] );

	// store additional roots
	for ( Block* functionRoot: rootBlocks )
		ret->m_rootBlocks.push_back( functionRoot );

	// done, valid control flow graph extracted
	return ret;
}

void ControlFlowGraph::ExtractBlocks( const Block* block , std::vector< const Block* >& outAllBlocks, std::set< const Block* >& visitedBlocks )
{
	if ( !block  )
		return;

	if ( visitedBlocks.find( block ) != visitedBlocks.end() )
		return;

	visitedBlocks.insert( block );
	outAllBlocks.push_back( block );

	if ( block->GetType() == Block::EBlockType::JUMP )
		ExtractBlocks( block->GetTargetBlock(), outAllBlocks, visitedBlocks );

	if ( block->GetContinuation() != nullptr )
		ExtractBlocks( block->GetContinuation(), outAllBlocks, visitedBlocks );
}

uint32 ControlFlowGraph::GetEntryPointAddress() const
{
	DEBUG_CHECK( !m_rootBlocks.empty() );
	return m_rootBlocks[0]->GetAddress();
}

void ControlFlowGraph::EmitHLSL( class IHLSLWriter& writer ) const
{
	// emit stuff for each root group
	for ( Block* rootBlock : m_rootBlocks )
	{
		// the root block MUST be the EXEC block
		DEBUG_CHECK( rootBlock->GetType() == Block::EBlockType::EXEC );
		const uint32 rootAddress = rootBlock->GetAddress();

		// extract all blocks ever used from that root (note: calls are not included)
		std::vector< const Block* > allUsedBlocks;
		{
			std::set< const Block* > visitedBlocks;
			ExtractBlocks( rootBlock, allUsedBlocks, visitedBlocks );
		}

		// do we have jumps and calls here ?
		bool hasJumps = false;
		bool hasCalls = false;
		for ( const Block* block : allUsedBlocks )
		{
			if ( block->GetType() == Block::EBlockType::JUMP )
			{
				hasJumps = true;
			}
			else if ( block->GetType() == Block::EBlockType::CALL )
			{
				hasCalls = true;
			}
		}

		// are we a call target ?
		bool isCalled = !rootBlock->GetSourceBlocks().empty();

		// start the control flow region		
		writer.BeginControlFlow( rootAddress, hasJumps, hasCalls, isCalled );

		// extract all blocks that have addresses - they will be the seeds of the jump logic
		std::vector< const Block* > allAddressableBlocks;
		allAddressableBlocks.reserve( allUsedBlocks.size() );
		for ( const Block* block : allUsedBlocks )
		{
			if ( block->GetType() == Block::EBlockType::EXEC )
			{
				// NOTE: we NEVER jump to this block so technically, it does not have a distinct address that is used anywhere so we can ignore it for the control flow
				// NOTE: the root block is always addressable although it may be the ONLY addressable block
				if ( block->GetSourceBlocks().empty() && (block != rootBlock) )
					continue;

				allAddressableBlocks.push_back( block );
			}
		}

		// sort addressable blocks by address
		std::sort( allAddressableBlocks.begin(), allAddressableBlocks.end(), []( const Block* a, const Block* b) { return a->GetAddress() < b->GetAddress(); } );

		// emit addressable chain for each addressable block
		// following blocks are added using continuation
		for ( uint32 j=0; j<allAddressableBlocks.size(); ++j )
		{
			const Block* baseBlock = allAddressableBlocks[j];
			writer.BeginBlockWithAddress( baseBlock->GetAddress() );

			const Block* nextBaseBlock = nullptr;
			if ( j < (allAddressableBlocks.size() - 1) )
				nextBaseBlock = allAddressableBlocks[j+1];

			const Block* block = baseBlock;
			while ( block )
			{
				// don't allow to enter into next addressable region
				if ( nextBaseBlock && block->GetAddress() == nextBaseBlock->GetAddress() )
					break;

				// emit block into the HLSL
				block->EmitHLSL( writer );

				// follow block chain
				block = block->GetContinuation();
			}

			// end block chain
			writer.EndBlockWithAddress();
		}

		// end the control flow region
		writer.EndControlFlow();
	}
}

//-----------------------------------------------------------------------------

Shader::Shader()
	: m_cf( nullptr )
	, m_numUsedInterpolators( 0 )
{
}

Shader::~Shader()
{
	// delete code
	if ( m_cf )
	{
		delete m_cf;
		m_cf = nullptr;
	}
}

namespace Helper
{
	void VisitAll( const Block* b, Statement::IVisitor& v )
	{
		if ( b->GetCondition() )
			v.OnConditionPush( b->GetCondition() );

		if ( b->GetCode() )
			b->GetCode()->Visit( v );

		if ( b->GetCondition() )
			v.OnConditionPop();
	}

	void VisitAll( const ControlFlowGraph* cf, Statement::IVisitor& v )
	{
		for ( uint32 i=0; i<cf->GetNumBlocks(); ++i )
		{
			const Block* b = cf->GetBlock(i);
			VisitAll( b, v );
		}
	}

	class AllExpressionVisitor : public Statement::IVisitor
	{
	public:
		AllExpressionVisitor( ExprNode::IVisitor* exprVisitor )
			: m_exprVisitor( exprVisitor )
		{}

		virtual void OnWrite( const ExprNode* dest, const ExprNode* src, const ESwizzle* mask ) override final
		{
			dest->Visit( *m_exprVisitor );
			src->Visit( *m_exprVisitor );
		}

		virtual void OnConditionPush( const ExprNode* condition ) override final
		{
			condition->Visit( *m_exprVisitor );
		}

		virtual void OnConditionPop() override final
		{
		}

	private:
		ExprNode::IVisitor*		m_exprVisitor;
	};

	class GlobalInstructionExtractor : public ExprNode::IVisitor
	{
	public:
		virtual void OnExprStart( const ExprNode* n ) override final
		{
			if ( n->GetType() == EExprType::VFETCH )
			{
				m_vfetch.push_back( static_cast< const ExprVertexFetch* >(n) );
			}
			else if ( n->GetType() == EExprType::TFETCH )
			{
				m_tfetch.push_back( static_cast< const ExprTextureFetch* >(n) );
			}
			else if ( n->GetType() == EExprType::EXPORT )
			{
				m_exports.push_back( static_cast< const ExprWriteExportReg* >(n) );
			}
			else
			{
				const int regIndex = n->GetRegisterIndex();
				if ( regIndex != -1 )
				{
					m_usedRegisters.insert( (uint32) regIndex );
				}
			}
		}

		virtual void OnExprEnd( const ExprNode* n ) override final {}

		std::vector< const ExprVertexFetch* >		m_vfetch;
		std::vector< const ExprTextureFetch* >		m_tfetch;
		std::vector< const ExprWriteExportReg* >	m_exports;
		std::set< uint32 >							m_usedRegisters;
	};

}

Shader* Shader::DecompileMicroCode( const void* code, const uint32 codeLength, XenonShaderType shaderType )
{
	// decompile the control flow structure with the instructions
	ControlFlowGraph* cf = ControlFlowGraph::DecompileMicroCode( code, codeLength, shaderType );
	if ( !cf )
		return nullptr;

	// create output shader
	Shader* ret = new Shader();
	ret->m_cf = cf;

	// extract instructions from all statements in all control flow blocks 
	Helper::GlobalInstructionExtractor instructionExtractor;
	Helper::VisitAll( cf, Helper::AllExpressionVisitor( &instructionExtractor ) );

	// transfer
	ret->m_vertexFetches = std::move( instructionExtractor.m_vfetch );
	ret->m_exports = std::move( instructionExtractor.m_exports );

	// generate list of unique texture reference
	ret->m_textureFetchSlotMask = 0;
	for ( const auto& tfetch : instructionExtractor.m_tfetch )
	{
		// find in existing list
		bool found = false;
		for ( const auto& usedTexture : ret->m_usedTextures )
		{
			// already there ? (if so, make sure we are consistent about texture type here)
			if ( usedTexture.m_slot == tfetch->GetSlot() )
			{
				DEBUG_CHECK( usedTexture.m_type == tfetch->GetTextureType() );
				found = true;
				break;
			}			
		}

		// new texture
		if ( !found )
		{
			// add new info
			TextureRef ref;
			ref.m_slot = tfetch->GetSlot();
			ref.m_type = tfetch->GetTextureType();
			ret->m_usedTextures.push_back( ref );

			// merge the fetch slot mask
			ret->m_textureFetchSlotMask |= (1 << ref.m_slot);
		}
	}

	// extract used registers
	ret->m_usedRegisters.reserve( instructionExtractor.m_usedRegisters.size() );
	for ( const uint32 regIndex : instructionExtractor.m_usedRegisters )
		ret->m_usedRegisters.push_back( regIndex );
	std::sort( 	ret->m_usedRegisters.begin(), ret->m_usedRegisters.end() );

	// scan exports
	ret->m_numUsedInterpolators = 0;
	for ( const auto& exp : ret->m_exports )
	{
		const int index = exp->GetExportInterpolatorIndex( exp->GetExportReg() );
		if ( index >= 0 )
		{
			ret->m_numUsedInterpolators = max( (uint32)index+1, ret->m_numUsedInterpolators );
		}
	}

	// done
	return ret;
}

//-----------------------------------------------------------------------------
