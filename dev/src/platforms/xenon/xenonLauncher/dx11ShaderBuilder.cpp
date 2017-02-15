#include "build.h"
#include "xenonGPUUtils.h"
#include "dx11ShaderBuilder.h"
#include "dx11MicrocodeShader.h"
#include "dx11MicrocodeNodes.h"
#include "dx11MicrocodeHLSLWriter.h"
#include "dx11ShaderHLSLWriter.h"

#include "shaders/psCommon.fx.h"
#include "shaders/vsCommon.fx.h"
#include "shaders/instrCommon.fx.h"

//#pragma optimize( "", off )

CDX11ShaderHLSLGenerator::CDX11ShaderHLSLGenerator()
{
}

namespace Helper
{
	// NOTE: all formated strings are _by definition_ (we are free to use any naming convention and we choose to use the following)
	class CDX11ShaderHLSLGeneratorGlue : public DX11Microcode::IHLSLWriter
	{
	public:
		CDX11ShaderHLSLGeneratorGlue( CDX11ShaderHLSLWriter& writer, const bool isPixelShader )
			: m_writer( &writer )
			, m_isPixelShader( isPixelShader )
			, m_varIndex( 0 )
			, m_hasJumps( false )
		{
		}

		virtual CodeChunk FetchVertex( CodeChunk src, const DX11Microcode::ExprVertexFetch& fetchInstr ) override final
		{
			DEBUG_CHECK( !m_isPixelShader );

			// get fetch function name
			const char* fetchFuncName = "";
			uint32 fetchDataSize = 0;
			switch ( fetchInstr.GetFormat() )
			{
				case DX11Microcode::ExprVertexFetch::EFetchFormat::FMT_8: fetchDataSize = 8; fetchFuncName = "FetchVertex_8"; break;
				case DX11Microcode::ExprVertexFetch::EFetchFormat::FMT_8_8_8_8: fetchDataSize = 8; fetchFuncName = "FetchVertex_8_8_8_8"; break;
				case DX11Microcode::ExprVertexFetch::EFetchFormat::FMT_2_10_10_10: fetchDataSize = 16; fetchFuncName = "FetchVertex_2_10_10_10"; break; // expands to 16 16 16 16 during fetch (hack)
				case DX11Microcode::ExprVertexFetch::EFetchFormat::FMT_8_8: fetchDataSize = 8; fetchFuncName = "FetchVertex_8_8"; break;
				case DX11Microcode::ExprVertexFetch::EFetchFormat::FMT_16: fetchDataSize = 16; fetchFuncName = "FetchVertex_16"; break;
				case DX11Microcode::ExprVertexFetch::EFetchFormat::FMT_16_16: fetchDataSize = 16; fetchFuncName = "FetchVertex_16_16"; break;
				case DX11Microcode::ExprVertexFetch::EFetchFormat::FMT_16_16_16_16: fetchDataSize = 16; fetchFuncName = "FetchVertex_16_16_16_16"; break;
				case DX11Microcode::ExprVertexFetch::EFetchFormat::FMT_32: fetchDataSize = 32; fetchFuncName = "FetchVertex_32"; break;
				case DX11Microcode::ExprVertexFetch::EFetchFormat::FMT_32_32: fetchDataSize = 32; fetchFuncName = "FetchVertex_32_32"; break;
				case DX11Microcode::ExprVertexFetch::EFetchFormat::FMT_32_32_32_32: fetchDataSize = 32; fetchFuncName = "FetchVertex_32_32_32_32"; break;
				case DX11Microcode::ExprVertexFetch::EFetchFormat::FMT_32_FLOAT: fetchDataSize = 32; fetchFuncName = "FetchVertex_32"; break;
				case DX11Microcode::ExprVertexFetch::EFetchFormat::FMT_32_32_FLOAT: fetchDataSize = 32; fetchFuncName = "FetchVertex_32_32"; break;
				case DX11Microcode::ExprVertexFetch::EFetchFormat::FMT_32_32_32_32_FLOAT: fetchDataSize = 32; fetchFuncName = "FetchVertex_32_32_32_32"; break;
				case DX11Microcode::ExprVertexFetch::EFetchFormat::FMT_32_32_32_FLOAT: fetchDataSize = 32; fetchFuncName = "FetchVertex_32_32_32"; break;
			}
			DEBUG_CHECK( fetchFuncName != "" );

			// get expand function name (how to convert data just fetched to usuable format)
			const char* fetchConvFuncName = "";
			if ( fetchInstr.IsFloat() )
			{
				fetchConvFuncName = "asfloat";
			}
			else if ( fetchInstr.IsNormalized() )
			{
				if ( fetchInstr.IsSigned() )
				{
					switch ( fetchDataSize )
					{
						case 8: fetchConvFuncName = "NormalizeSigned8"; break;
						case 16: fetchConvFuncName = "NormalizeSigned16"; break;
						case 32: fetchConvFuncName = "NormalizeSigned32"; break;
						default: DEBUG_CHECK( !"Unknown fetch data size" );
					}
				}
				else
				{
					switch ( fetchDataSize )
					{
						case 8: fetchConvFuncName = "NormalizeUnsigned8"; break;
						case 16: fetchConvFuncName = "NormalizeUnsigned16"; break;
						case 32: fetchConvFuncName = "NormalizeUnsigned32"; break;
						default: DEBUG_CHECK( !"Unknown fetch data size" );
					}
				}
			}
			else
			{
				if ( fetchInstr.IsSigned() )
				{
					switch ( fetchDataSize )
					{
						case 8: fetchConvFuncName = "UnnormalizedSigned8"; break;
						case 16: fetchConvFuncName = "UnnormalizedSigned16"; break;
						case 32: fetchConvFuncName = "UnnormalizedSigned32"; break;
						default: DEBUG_CHECK( !"Unknown fetch data size" );
					}
				}
				else
				{
					switch ( fetchDataSize )
					{
						case 8: fetchConvFuncName = "UnnormalizedUnsigned8"; break; // just a cast
						case 16: fetchConvFuncName = "UnnormalizedUnsigned16"; break; // just a cast
						case 32: fetchConvFuncName = "UnnormalizedUnsigned32"; break; // just a cast
						default: DEBUG_CHECK( !"Unknown fetch data size" );
					}
				}
			}

			CodeChunk ret;
			// ConvFunc( FetchFunc( VertexBuffer_SlotId, Offset, Stride, VertexIndex ) );
			ret.Appendf( "%hs( %hs( VertexBuffer_%d, %d, %d, (uint)(%hs).x ) )", 				
				fetchConvFuncName, fetchFuncName, 
				fetchInstr.GetSlot(), fetchInstr.GetOffset(), fetchInstr.GetStride(), 
				src.c_str() );
			return ret;
		}

		virtual CodeChunk FetchTexture( CodeChunk src, const DX11Microcode::ExprTextureFetch& fetchInstr ) override final
		{
			CodeChunk ret;

			const auto slot = fetchInstr.GetSlot();
			if ( fetchInstr.GetTextureType() == DX11Microcode::ExprTextureFetch::TextureType::Type1D )
			{
				ret.Appendf( "Texture%d.Sample( Sampler%d, float4( (%hs).x, 0.0f, 0.0f, 1.0f ) )", slot, slot, src.c_str() );
			}
			else if ( fetchInstr.GetTextureType() == DX11Microcode::ExprTextureFetch::TextureType::Type2D )
			{
				ret.Appendf( "Texture%d.Sample( Sampler%d, float4( (%hs).xy, 0.0f, 1.0f ) )", slot, slot, src.c_str() );
			}
			else if ( fetchInstr.GetTextureType() == DX11Microcode::ExprTextureFetch::TextureType::Type2DArray )
			{
				ret.Appendf( "Texture%d.Sample( Sampler%d, float4( (%hs).xyz, 1.0f ) )", slot, slot, src.c_str() );
			}
			else if ( fetchInstr.GetTextureType() == DX11Microcode::ExprTextureFetch::TextureType::Type3D )
			{
				ret.Appendf( "Texture%d.Sample( Sampler%d, float4( (%hs).xyz, 1.0f ) )", slot, slot, src.c_str() );
			}
			else if ( fetchInstr.GetTextureType() == DX11Microcode::ExprTextureFetch::TextureType::TypeCube )
			{
				ret.Appendf( "Texture%d.Sample( Sampler%d, float4( (%hs).xyz, 1.0f ) )", slot, slot, src.c_str() );
			}

			return ret;			
		}

		virtual CodeChunk GetExportDest( const DX11Microcode::ExprWriteExportReg::EExportReg reg ) override final
		{
			if ( m_isPixelShader )
			{
				// must match crap in GeneratePixelShader
				switch ( reg )
				{
					case DX11Microcode::ExprWriteExportReg::EExportReg::COLOR0: return CodeChunk( "ret.out_COLOR0");
					case DX11Microcode::ExprWriteExportReg::EExportReg::COLOR1: return CodeChunk( "ret.out_COLOR1");
					case DX11Microcode::ExprWriteExportReg::EExportReg::COLOR2: return CodeChunk( "ret.out_COLOR2");
					case DX11Microcode::ExprWriteExportReg::EExportReg::COLOR3: return CodeChunk( "ret.out_COLOR3");
				}

				DEBUG_CHECK( !"Unknown export register type" );
			}
			else
			{
				// must match crap in GenerateVertexShader
				switch ( reg )
				{
					case DX11Microcode::ExprWriteExportReg::EExportReg::POSITION: return CodeChunk( "ret.out_POSITION");
					case DX11Microcode::ExprWriteExportReg::EExportReg::POINTSIZE: return CodeChunk( "ret.out_POINTSIZE");
					case DX11Microcode::ExprWriteExportReg::EExportReg::INTERP0: return CodeChunk( "ret.out_INTERP0");
					case DX11Microcode::ExprWriteExportReg::EExportReg::INTERP1: return CodeChunk( "ret.out_INTERP1");
					case DX11Microcode::ExprWriteExportReg::EExportReg::INTERP2: return CodeChunk( "ret.out_INTERP2");
					case DX11Microcode::ExprWriteExportReg::EExportReg::INTERP3: return CodeChunk( "ret.out_INTERP3");
					case DX11Microcode::ExprWriteExportReg::EExportReg::INTERP4: return CodeChunk( "ret.out_INTERP4");
					case DX11Microcode::ExprWriteExportReg::EExportReg::INTERP5: return CodeChunk( "ret.out_INTERP5");
					case DX11Microcode::ExprWriteExportReg::EExportReg::INTERP6: return CodeChunk( "ret.out_INTERP6");
					case DX11Microcode::ExprWriteExportReg::EExportReg::INTERP7: return CodeChunk( "ret.out_INTERP7");
					case DX11Microcode::ExprWriteExportReg::EExportReg::INTERP8: return CodeChunk( "ret.out_INTERP8");
				}

				DEBUG_CHECK( !"Unknown export register type" );
			}

			return CodeChunk();
		}

		virtual CodeChunk GetReg( uint32 regIndex ) override final
		{
			DEBUG_CHECK( regIndex < 256 );

			CodeChunk ret;
			ret.Appendf( "regs.R%d", regIndex ); // by convention
			return ret;
		}

		virtual CodeChunk GetBoolVal( const uint32 boolRegIndex ) override final
		{
			DEBUG_CHECK( boolRegIndex < 256 );

			CodeChunk ret;
			ret.Appendf( "GetBoolConst(%d)", boolRegIndex ); // by convention, actual implementation in psCommon, vsCommon
			return ret;
		}

		virtual CodeChunk GetFloatVal( const uint32 floatRegIndex ) override final
		{
			DEBUG_CHECK( floatRegIndex < 256 );

			CodeChunk ret;
			ret.Appendf( "GetFloatConst(%d)", floatRegIndex ); // by convention, must match stuff in psCommon, vsCommon
			return ret;
		}

		virtual CodeChunk GetFloatValRelative( const uint32 floatRegOffset ) override final
		{
			DEBUG_CHECK( floatRegOffset < 256 );

			CodeChunk ret;
			ret.Appendf( "GetFloatConst(regs.A0 + %d)", floatRegOffset ); // by convention, must match stuff in psCommon, vsCommon
			return ret;
		}

		virtual CodeChunk GetPredicate() override final
		{
			return CodeChunk( "regs.PRED" ); // by convention, must match stuff in psCommon, vsCommon
		}

		virtual void SetPredicate( CodeChunk newValue ) override final
		{
			m_writer->Appendf( "SetPredicate( regs.PRED, (%s) );\n", newValue.c_str() ); // by convention, must match stuff in psCommon, vsCommon
		}

		virtual void PushPredicate( CodeChunk newValue ) override final
		{
		}

		virtual void PopPredicate() override final
		{
		}

		virtual CodeChunk AllocLocalVector( CodeChunk initCode ) override final
		{
			CodeChunk ret;
			ret.Appendf( "local%d", m_varIndex );
			m_varIndex += 1;

			m_writer->Appendf( "float4 %hs = %hs;\n", ret.c_str(), initCode.c_str() );
			return ret;
		}

		virtual CodeChunk AllocLocalScalar( CodeChunk initCode ) override final
		{
			CodeChunk ret;
			ret.Appendf( "local%d", m_varIndex );
			m_varIndex += 1;

			m_writer->Appendf( "float %hs = %hs;\n", ret.c_str(), initCode.c_str() );
			return ret;
		}

		virtual CodeChunk AllocLocalBool( CodeChunk initCode ) override final
		{
			CodeChunk ret;
			ret.Appendf( "local%d", m_varIndex );
			m_varIndex += 1;

			m_writer->Appendf( "bool %hs = %hs;\n", ret.c_str(), initCode.c_str() );
			return ret;
		}

		virtual void BeingCondition( CodeChunk condition ) override final
		{
			m_writer->Appendf( "if ( %hs )\n", condition.c_str() );
			m_writer->Append( "{\n" );
			m_writer->Indent( 1 );
		}

		virtual void EndCondition() override final
		{
			m_writer->Indent( -1 );
			m_writer->Append( "}\n" );
		}

		virtual void Assign( CodeChunk dest, CodeChunk src ) override final
		{
			m_writer->Appendf( "%hs = %hs;\n", dest.c_str(), src.c_str() );
		}

		virtual void BeginControlFlow( const uint32 address, const bool bHasJumps, const bool bHasCalls, const bool bIsCalled ) override final
		{
			m_writer->Appendf( "uint microcode_%d( INPUT input, inout OUTPUT ret, inout REGS regs )\n", address );
			m_writer->Append( "{\n ");
			m_writer->Indent( 1 );

			if ( bHasJumps )
			{
				m_writer->Appendf( "uint pc = %d;\n ", address );
				m_writer->Append(  "while ( pc != 0xFFFF )\n ");
				m_writer->Append(  "{\n ");
				m_writer->Indent( 1 );
				m_writer->Append(  "[call]\n" );
				m_writer->Append(  "switch ( pc )\n ");
				m_writer->Append(  "{\n ");
				m_writer->Indent( 1 );
			}

			m_hasJumps = bHasJumps;
		}

		virtual void EndControlFlow() override final
		{
			if ( m_hasJumps )
			{
				m_writer->Indent( -1 );
				m_writer->Append(  "}\n");

				m_writer->Indent( -1 );
				m_writer->Append(  "}\n");
			}

			m_writer->Append(  "return 0xFFFF;\n ");

			m_writer->Indent( -1 );
			m_writer->Append(  "}\n\n");
		}

		virtual void BeginBlockWithAddress( const uint32 address ) override final
		{
			if ( m_hasJumps )
			{
				m_writer->Appendf(  "case %d:\n", address );
				m_writer->Append(   "{\n");
				m_writer->Indent( 1 );
			}
		}

		virtual void EndBlockWithAddress() override final 
		{
			if ( m_hasJumps )
			{
				m_writer->Indent( -1 );
				m_writer->Append(  "}\n ");
			}
		}

		virtual void ControlFlowReturn( const uint32 targetAddress ) override final
		{
			m_writer->Appendf( "return %d; // RET\n", targetAddress );
		}

		virtual void ControlFlowEnd() override final
		{
			m_writer->Append( "return 0xFFFF; // END\n" );
		}

		virtual void ControlFlowCall( const uint32 targetAddress ) override final
		{
			if ( m_hasJumps )
			{
				m_writer->Appendf( "pc = microcode_%d( input, ret, regs ); \\ CALL\n", targetAddress );
				m_writer->Append(  "break;\n" );
			}
			else
			{
				m_writer->Appendf( "if ( 0xFFFF == microcode_%d( input, ret, regs ) ) return 0xFFFF; \\ CALL\n", targetAddress );
			}
		}

		virtual void ControlFlowJump( const uint32 targetAddress ) override final
		{
			DEBUG_CHECK( m_hasJumps );
			m_writer->Appendf( "pc = %d; \\ JUMP\n", targetAddress );
			m_writer->Append(  "break;\n" );
		}

	private:
		CDX11ShaderHLSLWriter*		m_writer;
		bool						m_isPixelShader;
		uint32						m_varIndex;

		bool						m_hasJumps;
	};
}

void CDX11ShaderHLSLGenerator::GenerateRegsStructure( class CDX11ShaderHLSLWriter& writer, const class CDX11MicrocodeShader& microcode )
{
	writer.Append( "// Registers\n" );
	writer.Append( "struct REGS\n" );
	writer.Append( "{\n" );
	writer.Append( "  int A0;\n ");
	writer.Append( "  bool PRED;\n ");

	// initialize used regs
	std::vector< uint32 > usedRegs;
	microcode.GetUsedRegisters( usedRegs );
	for ( uint32 i=0; i<usedRegs.size(); ++i )
	{
		const uint32 regIndex = usedRegs[i];
		writer.Appendf( "  float4 R%d;\n", regIndex );
	}
	writer.Append( "};\n\n" );
}

void CDX11ShaderHLSLGenerator::GeneratePixelShaderStructures( class CDX11ShaderHLSLWriter& writer, const Context& context, const class CDX11MicrocodeShader& microcode )
{
	DEBUG_CHECK( microcode.IsPixelShader() );

	// emit input structure, TODO: we should have rules about generating this
	{
		writer.Append( "// Pixel shader inputs\n" );
		writer.Append( "struct INPUT\n" );
		writer.Append( "{\n" );
		writer.Append( "  float4 Position : SV_Position;\n" );
		for ( uint32 i=0; i<context.m_numIncomingInputs; ++i )
		{
			writer.Appendf( "  float4 Interp%d : TEXCOORD%d;\n", i, i );
		}
		writer.Append( "};\n\n" );
	}

	// emit output structure
	{
		writer.Append( "// Pixel shader outputs\n" );
		writer.Append( "struct OUTPUT\n" );
		writer.Append( "{\n" );
		{
			std::vector< CDX11MicrocodeShader::OutputInfo > outputs;
			microcode.GetOutputs( outputs );

			for ( const auto& it : outputs )
			{
				writer.Appendf( "   %hs %hs : %hs;\n", it.m_type.c_str(), it.m_name.c_str(), it.m_semantic.c_str() );
			}
		}
		writer.Append( "};\n\n" );
	}

	// emit textures
	{
		std::vector< CDX11MicrocodeShader::TextureInfo > textures;
		microcode.GetUsedTextures( textures );

		for ( const auto& it : textures )
		{
			writer.Append( "// Texture sampler\n" );
			writer.Appendf( "sampler Sampler%d : register( s%d );\n", it.m_fetchSlot, it.m_runtimeSlot );

			if ( it.m_type == CDX11MicrocodeShader::ETextureType::TYPE_1D )
			{
				writer.Appendf( "texture1D Texture%d : register( t%d );\n", it.m_fetchSlot, it.m_runtimeSlot );
			}
			else if ( it.m_type == CDX11MicrocodeShader::ETextureType::TYPE_Array1D )
			{
				writer.Appendf( "texture1DArray Texture%d : register( t%d );\n", it.m_fetchSlot, it.m_runtimeSlot );
			}
			else if ( it.m_type == CDX11MicrocodeShader::ETextureType::TYPE_2D )
			{
				writer.Appendf( "texture2D Texture%d : register( t%d );\n", it.m_fetchSlot, it.m_runtimeSlot );
			}
			else if ( it.m_type == CDX11MicrocodeShader::ETextureType::TYPE_Array2D )
			{
				writer.Appendf( "texture2DArray Texture%d : register( t%d );\n", it.m_fetchSlot, it.m_runtimeSlot );
			}
			else if ( it.m_type == CDX11MicrocodeShader::ETextureType::TYPE_3D )
			{
				writer.Appendf( "texture3D Texture%d : register( t%d );\n", it.m_fetchSlot, it.m_runtimeSlot );
			}
			else if ( it.m_type == CDX11MicrocodeShader::ETextureType::TYPE_CUBE )
			{
				writer.Appendf( "textureCUBE Texture%d : register( t%d );\n", it.m_fetchSlot, it.m_runtimeSlot );
			}
			else
			{
				DEBUG_CHECK( !"Unknown teture format" );
			}
			writer.Append( "\n" );
		}
	}
}

void CDX11ShaderHLSLGenerator::GenerateVertexShaderStructures( class CDX11ShaderHLSLWriter& writer, const Context& context, const class CDX11MicrocodeShader& microcode )
{
	DEBUG_CHECK( !microcode.IsPixelShader() );

	// emit fetch input
	{
		std::vector< CDX11MicrocodeShader::FetchInfo > fetches;
		microcode.GetVertexFetches( fetches );

		// gather information about used data
		// NOTE: EVERYTHING is mapped to uint buffers, even floats (we just use asfloat to reinterpret the data)
		// get the list of unique slots, each will have a buffer bounded to it
		std::vector< uint32 > buffers;
		for ( const auto& it : fetches )
		{
			const uint32 slotId = it.m_rawSlot;
			if ( std::find( buffers.begin(), buffers.end(), slotId ) == buffers.end() )
			{
				buffers.push_back( slotId );
			}
		}

		// generate "vertex buffer" bindings
		if ( !buffers.empty() )
		{
			writer.Append( "// Vertex buffers\n" );
			for ( uint32 i=0; i<buffers.size(); ++i )
			{
				writer.Appendf( "Buffer<uint> VertexBuffer_%d : register( t%d );\n", buffers[i], 4+i );
			}
			writer.Append( "\n" );
		}

		// emit the "fetch data" structure that simulates the normal vertex format
		writer.Append( "// Vertex shader inputs (simulated fetch)\n" );
		writer.Append( "struct INPUT\n" );
		writer.Append( "{\n" );
		writer.Append( "  uint VertexID;\n" );
		writer.Append( "  uint InstanceID;\n" );
		writer.Append( "};\n\n" );
	}

	// emit output structure
	{
		writer.Append( "// Vertex shader outputs\n" );
		writer.Append( "struct OUTPUT\n" );
		writer.Append( "{\n" );
		{
			std::vector< CDX11MicrocodeShader::OutputInfo > outputs;
			microcode.GetOutputs( outputs );

			// HACK: output SV_Position first
			for ( const auto& it : outputs )
			{
				if ( 0 != strcmp( it.m_semantic.c_str(), "SV_Position" ) )
					continue;

				writer.Appendf( "  %hs %hs : %hs;\n", it.m_type.c_str(), it.m_name.c_str(), it.m_semantic.c_str() );
			}

			// HACK: output SV_Position first
			for ( const auto& it : outputs )
			{
				if ( 0 == strcmp( it.m_semantic.c_str(), "SV_Position" ) )
					continue;

				writer.Appendf( "  %hs %hs : %hs;\n", it.m_type.c_str(), it.m_name.c_str(), it.m_semantic.c_str() );
			}
		}
		writer.Append( "};\n\n" );
	}
}

bool CDX11ShaderHLSLGenerator::GenerateHLSL( const class CDX11MicrocodeShader& microcode, const Context& context, std::string& outCode )
{
	CDX11ShaderHLSLWriter writer;

	// emit input/output structures
	if ( microcode.IsPixelShader() )
		GeneratePixelShaderStructures( writer, context, microcode );
	else
		GenerateVertexShaderStructures( writer, context, microcode );

	// emit common stuff
	writer.Append( microcode.IsPixelShader() ? PixelShaderCommonTxt : VertexShaderCommonTxt );
	writer.Append( InstructionsCommonTxt );

	// generate the registers
	GenerateRegsStructure( writer, microcode );
	
	// emit the microcode structure
	{
		Helper::CDX11ShaderHLSLGeneratorGlue glueWriter( writer, microcode.IsPixelShader() );
		microcode.EmitHLSL( glueWriter );
	}

	// extra shit
	/*if ( microcode.GetHash() == 0x714759C9 )
	//if ( !microcode.IsPixelShader() )
	{
		writer.Append("uint microcode_300( INPUT input, inout OUTPUT ret, inout REGS regs )  \n");
		writer.Append("{  \n");
		writer.Append("	regs.R2.xyz = float4(asfloat( FetchVertex_32_32_32( VertexBuffer_95, 0, 4, (uint)(regs.R0).x ) )).xyz;  \n");
		writer.Append("	regs.R2.w = 1.0f;  \n");
		writer.Append("	regs.R0.xyzw = float4(NormalizeUnsigned8( FetchVertex_8_8_8_8( VertexBuffer_95, 3, 4, (uint)(regs.R0).x ) )).zyxw;  \n");
		writer.Append("	regs.R1.xyzw = float4(MULv((((regs.R2).wwww)),(((GetFloatConst(3)).xwzy)))).xyzw;  \n");
		writer.Append("	regs.R1.xyzw = float4(MULLADDv((((regs.R2).zzzz)),(((GetFloatConst(2)).xyzw)),(regs.R1))).xyzw;  \n");
		writer.Append("	regs.R1.xyzw = float4(MULLADDv((((regs.R2).yyyy)),(((GetFloatConst(1)).xyzw)),(((regs.R1).xyzw)))).xyzw;  \n");
		writer.Append("	ret.out_POSITION.xyzw = float4(MULLADDv((((regs.R2).xxxx)),(GetFloatConst(0)),(((regs.R1).xyzw)))).xyzw;  \n");
		writer.Append("	ret.out_INTERP0.xyzw = float4(max((regs.R0),(regs.R0))).xyzw;  \n");
		writer.Append("	return 0xFFFF; // END  \n");
		writer.Append("	return 0xFFFF;  \n");
		writer.Append("}  \n");

	}*/

	// emit the main function
	if ( microcode.IsPixelShader() )
	{
		// Pixel shader gets the data from vertex shader through the classic interpolators
		// On Xenon the initial values from interpolators are preloaded into the R0-R7 registers.
		// The data is being output using normal SV_TargetN stuff.
		// Other special cases are the Depth and CoverageMask
		writer.Append( "void main( INPUT input, out OUTPUT ret )\n" );
		writer.Append( "{\n" );
		writer.Append( "  ret = (OUTPUT)0;\n" );
		writer.Append( "  REGS regs = (REGS)0;\n" );
		writer.Append( "  \n" );

		// assign interpolators
		std::vector< uint32 > usedRegs;
		microcode.GetUsedRegisters( usedRegs );

		// retain rest of the inputs with the values of shader contants
		for ( uint32 i=0; i<usedRegs.size(); ++i )
		{
			const uint32 regIndex = usedRegs[i];
			if ( regIndex >= context.m_numIncomingInputs )
			{
				writer.Appendf( "  regs.R%d = GetFloatConst(%d);\n", regIndex, regIndex );
			}
			else
			{
				writer.Appendf( "  regs.R%d = input.Interp%d;\n", regIndex, regIndex );
			}
		}
	}
	else
	{
		// vertex shader gets the data in a little bit tricky way: we don't use IA at all, 
		// everything is fetched from SRV buffers that are directly addressed simulating the way vfetch works on Xenon
		// the VS output is mapped to TEXCOORD interpolator stages hoping that it will work :)
		// the only special cases are the SV_Position (exported via oPosition)	
		writer.Append( "void main( uint vindex : SV_VertexID, uint iindex : SV_InstanceID, out OUTPUT ret )\n" );
		writer.Append( "{\n" );
		writer.Append( "  ret = (OUTPUT)0;\n" );
		writer.Append( "  INPUT input = (INPUT)0;\n" );
		writer.Append( "  uint indexedVertexIndex = GetVertexIndex( vindex );\n" );
		writer.Append( "  input.VertexID = indexedVertexIndex;\n" );
		writer.Append( "  input.InstanceID = (float)iindex;\n" );
		writer.Append( "  \n" );
		writer.Append( "  REGS regs = (REGS)0;\n" );
		writer.Append( "  regs.R0 = input.VertexID;\n" );
		writer.Append( "  regs.R1 = iindex;\n" );
		writer.Append( "  \n" );
	}

	// execute root microcode block (at address 0)
	writer.Append(  "  // execute root microcode block;\n" );
	//writer.Appendf( "  microcode_%d( input, ret, regs );\n", (microcode.GetHash() == 0x714759C9) ? 300 : microcode.GetEntryPointAddress() );
	writer.Appendf( "  microcode_%d( input, ret, regs );\n", microcode.GetEntryPointAddress() );
	//writer.Appendf( "  microcode_%d( input, ret, regs );\n", 300 );

	// vertex shader finale
	if ( !microcode.IsPixelShader() )
	{
		writer.Append( "ret.out_POSITION = PostTransformVertex( vindex, ret.out_POSITION );\n" );
	}
	else
	{
		//writer.Append( "ret.out_COLOR0 = float4(1,0,1,1);\n" );
	}

	// end
	writer.Append(  "}\n" );

	// extract code
	outCode = writer.c_str();
	return true;
}
