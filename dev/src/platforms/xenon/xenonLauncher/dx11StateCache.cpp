#include "build.h"
#include "dx11StateCache.h"

//#pragma optimize( "",off)

CDX11StateCache::CDX11StateCache( ID3D11Device* dev, ID3D11DeviceContext* context )
	: m_device( dev )
	, m_mainContext( context )
{
}

CDX11StateCache::~CDX11StateCache()
{
	PurgeCache();
}

void CDX11StateCache::PurgeCache()
{
	m_dsStates.Clear();
	m_blendStates.Clear();
	m_rasterStates.Clear();
}

bool CDX11StateCache::SetDepthStencil( const D3D11_DEPTH_STENCIL_DESC& dsState, const uint32 stencilRef )
{
	// compute input hash
	const TDSStates::THash hash = TDSStates::ComputeHash( dsState );

	// use existing data
	{
		auto* ds = m_dsStates.Get( hash );
		if ( ds )
		{
			m_mainContext->OMSetDepthStencilState( ds, stencilRef );
			return true;
		}
	}

	// create new state
	ID3D11DepthStencilState* ds = nullptr;
	HRESULT hRet = m_device->CreateDepthStencilState( &dsState, &ds );
	if ( FAILED(hRet) || !ds )
	{
		GLog.Err( "D3D: Failed to create DepthStencil state from provided input, error = 0x%08X", hRet );
		return false;
	}

	// store in hash
	m_dsStates.Insert( hash, ds );

	// bind
	m_mainContext->OMSetDepthStencilState( ds, stencilRef );
	return true;
}

bool CDX11StateCache::SetBlendState( const D3D11_BLEND_DESC& blendState, const float blendFactors[4] )
{
	// compute input hash
	const TBlendStates::THash hash = TBlendStates::ComputeHash( blendState );

	// use existing data
	{
		auto* bs = m_blendStates.Get( hash );
		if ( bs )
		{
			m_mainContext->OMSetBlendState( bs, blendFactors, 0xFFFFFFFF );
			return true;
		}
	}

	// create new state
	ID3D11BlendState* bs = nullptr;
	HRESULT hRet = m_device->CreateBlendState( &blendState, &bs );
	if ( FAILED(hRet) || !bs )
	{
		GLog.Err( "D3D: Failed to create Blend state from provided input, error = 0x%08X", hRet );
		return false;
	}

	// store in hash
	m_blendStates.Insert( hash, bs );

	// bind
	m_mainContext->OMSetBlendState( bs, blendFactors, 0xFFFFFFFF );
	return true;
}

bool CDX11StateCache::SetRasterState( const D3D11_RASTERIZER_DESC& rasterState )
{
	// compute input hash
	const TRasterStates::THash hash = TRasterStates::ComputeHash( rasterState );

	// use existing data
	{
		auto* rs = m_rasterStates.Get( hash );
		if ( rs )
		{
			m_mainContext->RSSetState( rs );
			return true;
		}
	}

	// create new state
	ID3D11RasterizerState* rs = nullptr;
	HRESULT hRet = m_device->CreateRasterizerState( &rasterState, &rs );
	if ( FAILED(hRet) || !rs )
	{
		GLog.Err( "D3D: Failed to create Rasterizer state from provided input, error = 0x%08X", hRet );
		return false;
	}

	// store in hash
	m_rasterStates.Insert( hash, rs );

	// bind
	m_mainContext->RSSetState( rs );
	return true;
}
