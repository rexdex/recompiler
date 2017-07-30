#include "build.h"
#include "xenonGPUUtils.h"
#include "dx11FetchLayout.h"
#include "dx11MicrocodeShader.h"

//#pragma optimize("",off)

CDX11FetchLayout::CDX11FetchLayout()
	: m_numStreams( 0 )
{
}

CDX11FetchLayout::~CDX11FetchLayout()
{
}

CDX11FetchLayout* CDX11FetchLayout::ExtractFromMicrocode( CDX11MicrocodeShader* sourceMicrocode )
{
	// no microcode
	if ( !sourceMicrocode )
		return nullptr;	

	// fetch layout can only be build from a vertex shader
	if ( sourceMicrocode->IsPixelShader() )
		return nullptr;

	// get original vertex data fetches in the shader
	std::vector< CDX11MicrocodeShader::FetchInfo > vertexFetches;
	sourceMicrocode->GetVertexFetches( vertexFetches );

	// output fetch list
	std::auto_ptr< CDX11FetchLayout > ret( new CDX11FetchLayout() );

	// the assumption: each "slot" maps to a single input vertex stream, within each slot we may have different actual elements being fetched
	// this kind of mimics the vfetch_full + vfetch_mini combo
	for ( const auto& vfetch : vertexFetches )
	{
		// find existing slot
		StreamDesc* stream = nullptr;
		for ( uint32 i=0; i<ret->m_numStreams; ++i )
		{
			if ( ret->m_streams[i].m_slot == vfetch.m_rawSlot )
			{
				stream = &ret->m_streams[i];
				break;
			}
		}

		// new stream needed
		if ( !stream )
		{
			if ( ret->m_numStreams == MAX_STREAMS )
			{
				GLog.Err( "D3D: Fetch layout has more than %d streams. WTF?", MAX_STREAMS );
				return nullptr;
			}

			DEBUG_CHECK( vfetch.m_stride != 0 ); // first time a slot is defined the stride must be given (vfetch_full)
			stream = &ret->m_streams[ ret->m_numStreams++ ];
			memset( stream, 0, sizeof(StreamDesc) );

			stream->m_slot = vfetch.m_rawSlot;
			stream->m_stride = vfetch.m_stride;
			stream->m_numElems = 0;
		}

		// the data strides in the same slots MUST match (vfetch_full and matching vfetch_mini)
		if ( vfetch.m_stride )
		{
			DEBUG_CHECK( vfetch.m_stride == stream->m_stride );
			if ( vfetch.m_stride != stream->m_stride )
			{
				GLog.Err( "D3D: Incompatible strides in vertex format %d != %d", vfetch.m_stride, stream->m_stride );
				return nullptr;
			}
		}

		// add the decoding element - there cannot be more than 8 per stream (hardware limit - we have only 8 dwords in the biggest vfetch_full)
		DEBUG_CHECK( stream->m_numElems <= MAX_ELEMS );
		ElementDesc* elem = &stream->m_elems[ stream->m_numElems++ ];
		elem->m_format = ConvertFetchFormat( vfetch.m_format );
		elem->m_offset = vfetch.m_rawOffset;
	}	

	// return created layout
	return ret.release();
}

const CDX11FetchLayout::EFormat CDX11FetchLayout::ConvertFetchFormat( const CDX11MicrocodeShader::EFetchFormat shaderFetchFormat )
{
	switch ( shaderFetchFormat )
	{
		case CDX11MicrocodeShader::EFetchFormat::FMT_8: return EFormat::FMT_8;
		case CDX11MicrocodeShader::EFetchFormat::FMT_8_8_8_8: return EFormat::FMT_8_8_8_8;
		case CDX11MicrocodeShader::EFetchFormat::FMT_2_10_10_10: return EFormat::FMT_2_10_10_10;
		case CDX11MicrocodeShader::EFetchFormat::FMT_8_8: return EFormat::FMT_8_8;
		case CDX11MicrocodeShader::EFetchFormat::FMT_16: return EFormat::FMT_16;
		case CDX11MicrocodeShader::EFetchFormat::FMT_16_16: return EFormat::FMT_16_16;
		case CDX11MicrocodeShader::EFetchFormat::FMT_16_16_16_16: return EFormat::FMT_16_16_16_16;
		case CDX11MicrocodeShader::EFetchFormat::FMT_16_16_FLOAT: return EFormat::FMT_16_16_FLOAT;
		case CDX11MicrocodeShader::EFetchFormat::FMT_16_16_16_16_FLOAT: return EFormat::FMT_16_16_16_16_FLOAT;
		case CDX11MicrocodeShader::EFetchFormat::FMT_32: return EFormat::FMT_32;
		case CDX11MicrocodeShader::EFetchFormat::FMT_32_32: return EFormat::FMT_32_32;
		case CDX11MicrocodeShader::EFetchFormat::FMT_32_32_32_32: return EFormat::FMT_32_32_32_32;
		case CDX11MicrocodeShader::EFetchFormat::FMT_32_FLOAT: return EFormat::FMT_32_FLOAT;
		case CDX11MicrocodeShader::EFetchFormat::FMT_32_32_FLOAT: return EFormat::FMT_32_32_FLOAT;
		case CDX11MicrocodeShader::EFetchFormat::FMT_32_32_32_32_FLOAT: return EFormat::FMT_32_32_32_32_FLOAT;
		case CDX11MicrocodeShader::EFetchFormat::FMT_32_32_32_FLOAT: return EFormat::FMT_32_32_32_FLOAT;
	}

	DEBUG_CHECK( "Unknown shader fetch format" );
	return EFormat::FMT_UNKNOWN;
}