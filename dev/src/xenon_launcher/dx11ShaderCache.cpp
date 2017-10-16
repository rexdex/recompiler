#include "build.h"
#include "xenonGPUUtils.h"
#include "dx11MicrocodeShader.h"
#include "dx11ShaderCache.h"

CDX11ShaderCache::CDX11ShaderCache( ID3D11Device* dev )
	: m_device( dev )
{
}

void CDX11ShaderCache::SetDumpPath( const std::wstring& absPath )
{
	m_dumpPath = absPath;
}

CDX11ShaderCache::~CDX11ShaderCache()
{
	for ( auto it : m_pixelShaders )
		delete it.second;

	for ( auto it : m_vertexShaders )
		delete it.second;
}

CDX11VertexShader* CDX11ShaderCache::GetVertexShader( CDX11MicrocodeShader* sourceMicrocode, const CDX11VertexShader::DrawingContext& context )
{
	// no source
	if ( !sourceMicrocode )
		return nullptr;

	// search
	const Key key( sourceMicrocode->GetHash(), context.GetHash() );
	const auto it = m_vertexShaders.find( key );
	if ( it != m_vertexShaders.end() )
		return it->second;

	// create new
	CDX11VertexShader* vs = CDX11VertexShader::Compile( m_device, sourceMicrocode, context, m_dumpPath );
	m_vertexShaders[ key ] = vs;
	return vs;
}

CDX11PixelShader* CDX11ShaderCache::GetPixelShader( CDX11MicrocodeShader* sourceMicrocode, const CDX11PixelShader::DrawingContext& context )
{
	// no source
	if ( !sourceMicrocode )
		return nullptr;

	// search
	const Key key( sourceMicrocode->GetHash(), context.GetHash() );
	const auto it = m_pixelShaders.find( key );
	if ( it != m_pixelShaders.end() )
		return it->second;

	// create new
	CDX11PixelShader* ps = CDX11PixelShader::Compile( m_device, sourceMicrocode, context, m_dumpPath );
	m_pixelShaders[ key ] = ps;
	return ps;
}