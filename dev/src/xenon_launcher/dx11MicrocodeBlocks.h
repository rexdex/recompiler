#pragma once

//----------------------------------------------------------------------------

namespace DX11Microcode
{
	class ExprNode;
	class Statement;

	/// Generalized block in the shader
	class Block
	{
	public:
		enum class EBlockType : uint8
		{
			EXEC,
			JUMP,
			CALL,
			END, // shader end
			RET, // function return
		};

		Block( uint32 address, Statement* preamble, Statement* code, ExprNode* condition );
		Block( ExprNode* condition, uint32 targetAddress, EBlockType type );
		~Block();

		inline const EBlockType GetType() const { return m_type; }

		inline const uint32	GetAddress() const { return m_address; }
		inline const uint32	GetTargetAddress() const { return m_targetAddrses; }

		inline ExprNode* GetCondition() const { return m_condition; }
		inline Statement* GetCode() const { return m_code; }
		inline Statement* GetPreamble() const { return m_preamble; }

		inline const std::vector< Block* >& GetSourceBlocks() const { return m_sources; }
		inline Block* GetTargetBlock() const { return m_target; }

		inline Block* GetContinuation() const { return m_continuation; }

		void ConnectTarget( Block* targetBlock ); // jump/call target
		void ConnectContinuation( Block* nextBlock ); // next block to execute (NULL only for END and RET)

		// emit control flow graph HLSL
		void EmitHLSL( class IHLSLWriter& writer ) const;

	private:
		EBlockType		m_type;				// block type

		uint32			m_address;			// generalized address (used to link blocks together)
		uint32			m_targetAddrses;	// target address - only for JUMP and CALL

		ExprNode*		m_condition;		// condition for this block of code
		Statement*		m_code;				// code for this block (executed inside conditional branch)
		Statement*		m_preamble;			// part of code executed outside the conditional branch

		std::vector< Block* >	m_sources;	// blocks jumping to this block
		Block*					m_target;	// resolved target block, only for JUMP and CALL

		Block*			m_continuation; // continuation block (branchles case, simply put - the next block to execute)
	};

	/// Master control flow structure
	class ControlFlowGraph
	{
	public:
		ControlFlowGraph();
		~ControlFlowGraph();

		// get the starting block of the whole shader
		inline Block* GetStartBlock() const { return m_blocks.front(); }

		// random block access
		inline const uint32 GetNumBlocks() const { return (uint32) m_blocks.size(); }
		inline const Block* GetBlock( const uint32 index ) const { return m_blocks[ index ]; }

		// compile
		static ControlFlowGraph* DecompileMicroCode( const void* code, const uint32 codeLength, XenonShaderType shaderType );

		// emit control flow graph HLSL
		void EmitHLSL( class IHLSLWriter& writer ) const;

		// get root block address (main entry point)
		uint32 GetEntryPointAddress() const;

	private:
		std::vector< Block* >		m_blocks;		// all blocks
		std::vector< Block* >		m_rootBlocks;	// root blocks (control flow targets - results of conditional jumps/calls)

		static void ExtractBlocks( const Block* block, std::vector< const Block* >& outAllBlocks, std::set< const Block* >& visitedBlocks );
	};

	/// Decompiled shader
	class Shader
	{
	public:
		Shader();
		~Shader();

		typedef ExprTextureFetch::TextureType TextureType;

		// reference to a texture
		struct TextureRef
		{
			TextureType		m_type;
			uint32			m_slot; // fetch slot
		};

		// get the control flow graph and the code
		inline ControlFlowGraph* GetCode() const { return m_cf; }

		// get number of vertex fetch instructions
		inline const uint32 GetNumVertexFetches() const { return (uint32) m_vertexFetches.size(); }
		inline const ExprVertexFetch* GetVertexFetch( const uint32 index ) const { return m_vertexFetches[ index ]; }

		// get number of register export instructions (shader outputs)
		inline const uint32 GetNumExportInstructions() const { return (uint32) m_exports.size(); }
		inline const ExprWriteExportReg* GetExportInstruction( const uint32 index ) const { return m_exports[ index ]; }

		// get number of used registers
		inline const uint32 GetNumUsedRegisters() const { return (uint32) m_usedRegisters.size(); }
		inline const uint32 GetUsedRegister( const uint32 index ) const { return m_usedRegisters[index]; }

		// get number of used interpolators
		inline const uint32 GetNumUsedInterpolators() const { return m_numUsedInterpolators; }

		/// Get mask for used texture fetch slots
		inline const uint32 GetTextureFetchSlotMask() const { return m_textureFetchSlotMask; }

		// get number of texture references
		inline const uint32 GetNumUsedTextures() const { return (uint32) m_usedTextures.size(); }
		inline const TextureRef& GetUsedTexture( const uint32 index ) const { return m_usedTextures[ index ]; }

		// compile
		static Shader* DecompileMicroCode( const void* code, const uint32 codeLength, XenonShaderType shaderType );

	private:
		// control flow graph (code container)
		ControlFlowGraph*		m_cf;

		// gathered FETCH instruction (all blocks)
		std::vector< const ExprVertexFetch* >		m_vertexFetches;

		// gathered export instructions (all blocks)
		std::vector< const ExprWriteExportReg* >	m_exports;

		// used registers
		std::vector< uint32 >						m_usedRegisters;
		uint32										m_numUsedInterpolators;

		// gathered and collapsed information about textures used by shader
		std::vector< TextureRef >					m_usedTextures;
		uint32										m_textureFetchSlotMask;
	};

} // DX11Microcode
