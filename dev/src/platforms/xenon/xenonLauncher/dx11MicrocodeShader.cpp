#include "build.h"
#include "xenonGPUConstants.h"
#include "dx11MicrocodeNodes.h"
#include "dx11MicrocodeBlocks.h"
#include "dx11MicrocodeShader.h"
#include <algorithm>

CDX11MicrocodeShader::CDX11MicrocodeShader()
	: m_decompiledShader( nullptr )
	, m_bIsPixelShader( false )
	, m_microcodeHash( 0 )
{
}

CDX11MicrocodeShader::~CDX11MicrocodeShader()
{
	// delete shader data
	if ( m_decompiledShader )
	{
		delete m_decompiledShader;
		m_decompiledShader = nullptr;
	}
}

CDX11MicrocodeShader* CDX11MicrocodeShader::ExtractPixelShader( const void* shaderCode, const uint32 shaderCodeSize, const uint64 microcodeHash )
{
	// decompile as pixel shader
	DX11Microcode::Shader* shader = DX11Microcode::Shader::DecompileMicroCode( shaderCode, shaderCodeSize, XenonShaderType::ShaderPixel );
	if ( !shader )
		return nullptr;

	// create storage
	CDX11MicrocodeShader* ret = new CDX11MicrocodeShader();
	ret->m_bIsPixelShader = true;
	ret->m_microcodeHash = microcodeHash;
	ret->m_decompiledShader = shader;
	return ret;
}

CDX11MicrocodeShader* CDX11MicrocodeShader::ExtractVertexShader( const void* shaderCode, const uint32 shaderCodeSize, const uint64 microcodeHash )
{
	// decompile as vertex shader
	DX11Microcode::Shader* shader = DX11Microcode::Shader::DecompileMicroCode( shaderCode, shaderCodeSize, XenonShaderType::ShaderVertex );
	if ( !shader )
		return nullptr;

	// create storage
	CDX11MicrocodeShader* ret = new CDX11MicrocodeShader();
	ret->m_bIsPixelShader = false;
	ret->m_microcodeHash = microcodeHash;
	ret->m_decompiledShader = shader;
	return ret;
}

uint32 CDX11MicrocodeShader::GetFetchFormatChannels( const EFetchFormat format )
{
	switch ( format )
	{
		case EFetchFormat::FMT_1_REVERSE: return 0;
		case EFetchFormat::FMT_8: return 1;
		case EFetchFormat::FMT_8_8_8_8: return 4;
		case EFetchFormat::FMT_2_10_10_10: return 4;
		case EFetchFormat::FMT_8_8: return 2;
		case EFetchFormat::FMT_16: return 1;
		case EFetchFormat::FMT_16_16: return 2;
		case EFetchFormat::FMT_16_16_16_16: return 4;
		case EFetchFormat::FMT_16_16_FLOAT: return 2;
		case EFetchFormat::FMT_16_16_16_16_FLOAT: return 4;
		case EFetchFormat::FMT_32: return 1;
		case EFetchFormat::FMT_32_32: return 2;
		case EFetchFormat::FMT_32_32_32_32: return 4;
		case EFetchFormat::FMT_32_FLOAT: return 1;
		case EFetchFormat::FMT_32_32_FLOAT: return 2;
		case EFetchFormat::FMT_32_32_32_32_FLOAT: return 4;
		case EFetchFormat::FMT_32_32_32_FLOAT: return 3;
	}

	return 0;
}

const char* CDX11MicrocodeShader::GetFetchFormatName( const EFetchFormat format )
{
	switch ( format )
	{
		case EFetchFormat::FMT_1_REVERSE: return "FMT_1_REVERSE";
		case EFetchFormat::FMT_8: return "FMT_8";
		case EFetchFormat::FMT_8_8_8_8: return "FMT_8_8_8_8";
		case EFetchFormat::FMT_2_10_10_10: return "FMT_2_10_10_10";
		case EFetchFormat::FMT_8_8: return "FMT_8_8";
		case EFetchFormat::FMT_16: return "FMT_16";
		case EFetchFormat::FMT_16_16: return "FMT_16_16";
		case EFetchFormat::FMT_16_16_16_16: return "FMT_16_16_16_16";
		case EFetchFormat::FMT_16_16_FLOAT: return "FMT_16_16_FLOAT";
		case EFetchFormat::FMT_16_16_16_16_FLOAT: return "FMT_16_16_16_16_FLOAT";
		case EFetchFormat::FMT_32: return "FMT_32";
		case EFetchFormat::FMT_32_32: return "FMT_32_32";
		case EFetchFormat::FMT_32_32_32_32: return "FMT_32_32_32_32";
		case EFetchFormat::FMT_32_FLOAT: return "FMT_32_FLOAT";
		case EFetchFormat::FMT_32_32_FLOAT: return "FMT_32_32_FLOAT";
		case EFetchFormat::FMT_32_32_32_32_FLOAT: return "FMT_32_32_32_32_FLOAT";
		case EFetchFormat::FMT_32_32_32_FLOAT: return "FMT_32_32_32_FLOAT";
	}

	return 0;
}

namespace Helper
{
	static bool TranslateTextureType( const DX11Microcode::ExprTextureFetch::TextureType nativeType, CDX11MicrocodeShader::ETextureType& outType )
	{
		switch ( nativeType )
		{
			case DX11Microcode::ExprTextureFetch::TextureType::Type1D: outType = CDX11MicrocodeShader::ETextureType::TYPE_1D; return true;
			case DX11Microcode::ExprTextureFetch::TextureType::Type2D: outType = CDX11MicrocodeShader::ETextureType::TYPE_2D; return true;
			case DX11Microcode::ExprTextureFetch::TextureType::Type2DArray: outType = CDX11MicrocodeShader::ETextureType::TYPE_Array2D; return true;
			case DX11Microcode::ExprTextureFetch::TextureType::Type3D: outType = CDX11MicrocodeShader::ETextureType::TYPE_3D; return true;
			case DX11Microcode::ExprTextureFetch::TextureType::TypeCube: outType = CDX11MicrocodeShader::ETextureType::TYPE_CUBE; return true;
		}

		return false;
	}

	static bool TranslateFormat( const CXenonGPUMicrocodeTransformer::EFetchFormat nativeFormat, CDX11MicrocodeShader::EFetchFormat& outFormat )
	{
		switch ( nativeFormat )
		{
			case CXenonGPUMicrocodeTransformer::EFetchFormat::FMT_1_REVERSE: outFormat = CDX11MicrocodeShader::EFetchFormat::FMT_1_REVERSE; return true;
			case CXenonGPUMicrocodeTransformer::EFetchFormat::FMT_8: outFormat = CDX11MicrocodeShader::EFetchFormat::FMT_8; return true;
			case CXenonGPUMicrocodeTransformer::EFetchFormat::FMT_8_8_8_8: outFormat = CDX11MicrocodeShader::EFetchFormat::FMT_8_8_8_8; return true;
			case CXenonGPUMicrocodeTransformer::EFetchFormat::FMT_2_10_10_10: outFormat = CDX11MicrocodeShader::EFetchFormat::FMT_2_10_10_10; return true;
			case CXenonGPUMicrocodeTransformer::EFetchFormat::FMT_8_8: outFormat = CDX11MicrocodeShader::EFetchFormat::FMT_8_8; return true;
			case CXenonGPUMicrocodeTransformer::EFetchFormat::FMT_16: outFormat = CDX11MicrocodeShader::EFetchFormat::FMT_16; return true;
			case CXenonGPUMicrocodeTransformer::EFetchFormat::FMT_16_16: outFormat = CDX11MicrocodeShader::EFetchFormat::FMT_16_16; return true;
			case CXenonGPUMicrocodeTransformer::EFetchFormat::FMT_16_16_16_16: outFormat = CDX11MicrocodeShader::EFetchFormat::FMT_16_16_16_16; return true;
			case CXenonGPUMicrocodeTransformer::EFetchFormat::FMT_16_16_FLOAT: outFormat = CDX11MicrocodeShader::EFetchFormat::FMT_16_16_FLOAT; return true;
			case CXenonGPUMicrocodeTransformer::EFetchFormat::FMT_16_16_16_16_FLOAT: outFormat = CDX11MicrocodeShader::EFetchFormat::FMT_16_16_16_16_FLOAT; return true;
			case CXenonGPUMicrocodeTransformer::EFetchFormat::FMT_32: outFormat = CDX11MicrocodeShader::EFetchFormat::FMT_32; return true;
			case CXenonGPUMicrocodeTransformer::EFetchFormat::FMT_32_32: outFormat = CDX11MicrocodeShader::EFetchFormat::FMT_32_32; return true;
			case CXenonGPUMicrocodeTransformer::EFetchFormat::FMT_32_32_32_32: outFormat = CDX11MicrocodeShader::EFetchFormat::FMT_32_32_32_32; return true;
			case CXenonGPUMicrocodeTransformer::EFetchFormat::FMT_32_FLOAT: outFormat = CDX11MicrocodeShader::EFetchFormat::FMT_32_FLOAT; return true;
			case CXenonGPUMicrocodeTransformer::EFetchFormat::FMT_32_32_FLOAT: outFormat = CDX11MicrocodeShader::EFetchFormat::FMT_32_32_FLOAT; return true;
			case CXenonGPUMicrocodeTransformer::EFetchFormat::FMT_32_32_32_32_FLOAT: outFormat = CDX11MicrocodeShader::EFetchFormat::FMT_32_32_32_32_FLOAT; return true;
			case CXenonGPUMicrocodeTransformer::EFetchFormat::FMT_32_32_32_FLOAT: outFormat = CDX11MicrocodeShader::EFetchFormat::FMT_32_32_32_FLOAT; return true;
		}

		// unknown format
		return false;
	}
}

void CDX11MicrocodeShader::GetVertexFetches( std::vector< FetchInfo >& fetches ) const
{
	const uint32 numFetches = m_decompiledShader->GetNumVertexFetches();
	fetches.reserve( numFetches );
	for ( uint32 i=0; i<numFetches; ++i )
	{
		// get the fetch instruction
		const auto* instr = m_decompiledShader->GetVertexFetch( i );

		// convert type
		EFetchFormat fetchFormat;
		if ( !Helper::TranslateFormat( instr->GetFormat(), fetchFormat ) )
			continue;

		// extract information
		FetchInfo info;
		info.m_format = fetchFormat;
		info.m_normalized = instr->IsNormalized(); 
		info.m_signed = instr->IsSigned();
		info.m_float = instr->IsFloat();
		info.m_rawSlot = instr->GetSlot();
		info.m_rawOffset = instr->GetOffset();
		info.m_stride = instr->GetStride();
		info.m_layoutSlot = (uint16) fetches.size();
		info.m_channels = GetFetchFormatChannels( fetchFormat );
		fetches.push_back( info );
	}
}

void CDX11MicrocodeShader::GetUsedTextures( std::vector< TextureInfo >& outTextures ) const
{
	const uint32 numTextures = m_decompiledShader->GetNumUsedTextures();
	outTextures.reserve( numTextures );
	for ( uint32 i=0; i<numTextures; ++i )
	{
		// get the texture information
		const auto& texInfo = m_decompiledShader->GetUsedTexture( i );

		// convert texture type
		ETextureType textureType;
		if ( !Helper::TranslateTextureType( texInfo.m_type, textureType ) )
			continue;

		// extract other information
		TextureInfo info;
		info.m_runtimeSlot = i;
		info.m_fetchSlot = texInfo.m_slot;
		info.m_type = textureType;
		outTextures.push_back( info );
	}
}

void CDX11MicrocodeShader::GetOutputs( std::vector< OutputInfo >& outputs ) const
{
	const uint32 numExports = m_decompiledShader->GetNumExportInstructions();
	outputs.reserve( numExports );
	for ( uint32 i=0; i<numExports; ++i )
	{
		// get the export instruction
		const auto* instr = m_decompiledShader->GetExportInstruction( i );

		// all exports are handles via float4 type
		const char* exportType = "float4";
		const char* exportName = instr->GetExporRegName( instr->GetExportReg() );
		const char* exportSemantic = instr->GetExporSemanticName( instr->GetExportReg() ); 
		const int exportIndex = instr->GetExportSemanticIndex( instr->GetExportReg() );

		// check if already exists
		bool exists = false;
		for ( const auto& ret : outputs )
		{
			if ( 0 == strcmp( ret.m_name.c_str(), exportName ) )
			{
				exists = true;
				break;
			}
		}

		// add if not already added
		if ( !exists )
			outputs.emplace_back( exportType, exportName, exportSemantic, exportIndex );
	}

	// make sure output exports order is always deterministic
	std::sort( outputs.begin(), outputs.end(), []( const OutputInfo& a, const OutputInfo& b ) { return a.m_index < b.m_index; } );
}

void CDX11MicrocodeShader::GetUsedRegisters( std::vector< uint32 >& outUsedRegs ) const
{
	outUsedRegs.resize( m_decompiledShader->GetNumUsedRegisters() );
	for ( uint32 i=0; i<m_decompiledShader->GetNumUsedRegisters(); ++i )
	{
		outUsedRegs[i] = m_decompiledShader->GetUsedRegister(i);
	}
}

uint32 CDX11MicrocodeShader::GetEntryPointAddress() const
{
	return m_decompiledShader->GetCode()->GetEntryPointAddress();
}

uint32 CDX11MicrocodeShader::GetNumUsedInterpolators() const
{
	return m_decompiledShader->GetNumUsedInterpolators();
}

uint32 CDX11MicrocodeShader::GetTextureFetchSlotMask() const
{
	return m_decompiledShader->GetTextureFetchSlotMask();
}

void CDX11MicrocodeShader::DumpShader( const std::wstring& absoluteFilePath ) const
{
	FILE* f = NULL;
	_wfopen_s( &f, absoluteFilePath.c_str(), L"w" );
	if ( !f )
		return;

	if ( m_decompiledShader->GetNumExportInstructions() > 0 )
	{
		fprintf( f, "%d export instructions\n", m_decompiledShader->GetNumExportInstructions() );
		for ( uint32 i=0; i<m_decompiledShader->GetNumExportInstructions(); ++i )
		{
			const auto* instr = m_decompiledShader->GetExportInstruction(i);

			fprintf( f, "[%d]: Exported %s\n", 
				i, instr->GetExporRegName( instr->GetExportReg() ) );
		}
		fprintf( f, "\n" );
	}

	if ( m_decompiledShader->GetNumVertexFetches() > 0 )
	{
		fprintf( f, "%d vertex fetches\n", m_decompiledShader->GetNumVertexFetches() );
		for ( uint32 i=0; i<m_decompiledShader->GetNumVertexFetches(); ++i )
		{
			const auto* instr = m_decompiledShader->GetVertexFetch(i);

			fprintf( f, "[%d]: Fetch (slot: %d, offset: %d) format: %s\n", 
				i, instr->GetSlot(), instr->GetOffset(), instr->GetFormatName( instr->GetFormat() ) );
		}
		fprintf( f, "\n" );
	}

	if ( m_decompiledShader->GetNumUsedRegisters() > 0 )
	{
		fprintf( f, "%d registers used\n", m_decompiledShader->GetNumUsedRegisters() );
		for ( uint32 i=0; i<m_decompiledShader->GetNumUsedRegisters(); ++i )
		{
			fprintf( f, "[%d]: Register %d\n", 
				i, m_decompiledShader->GetUsedRegister(i) );
		}
		fprintf( f, "\n" );
	}

	if ( m_decompiledShader->GetCode() )
	{
		const uint32 numBlocks = m_decompiledShader->GetCode()->GetNumBlocks();
		fprintf( f, "%d blocks\n\n", numBlocks );

		for ( uint32 i=0; i<numBlocks; ++i )
		{
			const auto* b = m_decompiledShader->GetCode()->GetBlock(i);

			if ( b->GetType() == DX11Microcode::Block::EBlockType::EXEC )
			{
				fprintf( f, "Block %d, EXEC, address: %d\n\n", i, b->GetAddress() );
			}
			else if ( b->GetType() == DX11Microcode::Block::EBlockType::JUMP )
			{
				fprintf( f, "Block %d, JUMP, target address: %d\n\n", i, b->GetTargetAddress() );
			}
			else if ( b->GetType() == DX11Microcode::Block::EBlockType::CALL )
			{
				fprintf( f, "Block %d, CALL, target address: %d\n\n", i, b->GetTargetAddress() );
			}
			else if ( b->GetType() == DX11Microcode::Block::EBlockType::RET )
			{
				fprintf( f, "Block %d, RET, target address: %d\n\n", i, b->GetTargetAddress() );
			}
			else if ( b->GetType() == DX11Microcode::Block::EBlockType::END )
			{
				fprintf( f, "Block %d, END OF SHADER\n\n", i );
			}

			if ( b->GetContinuation() != nullptr )
			{
				int otherBlockIndex = -1;
				for ( uint32 j=0; j<numBlocks; ++j )
				{
					const auto* ob = m_decompiledShader->GetCode()->GetBlock(j);
					if ( ob == b->GetContinuation() )
					{
						otherBlockIndex = j;
						break;
					}
				}

				fprintf( f, "Continuation:\n" );
				fprintf( f, "\tBlock %d\n", otherBlockIndex );
			}

			if ( b->GetCondition() )
			{
				fprintf( f, "Condition:\n" );
				fprintf( f, "\t%s\n", b->GetCondition()->ToString().c_str() );
				fprintf( f, "\n" );
			}

			if ( b->GetCode() )
			{
				fprintf( f, "Code:\n" );
				fprintf( f, "%s\n", b->GetCode()->ToString().c_str() );
			}			

			fprintf( f, "\n------------------------------------------------\n\n");
		}
	}

	fclose(f);
}

void CDX11MicrocodeShader::EmitHLSL( class DX11Microcode::IHLSLWriter& writer ) const
{
	m_decompiledShader->GetCode()->EmitHLSL( writer );
}