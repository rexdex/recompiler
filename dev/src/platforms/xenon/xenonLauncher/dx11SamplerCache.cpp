#include "build.h"
#include "xenonGPUUtils.h"
#include "dx11SamplerCache.h"

//----------------

CDX11SamplerCache::SamplerInfo::SamplerInfo()
{
}

uint32 CDX11SamplerCache::SamplerInfo::GetHash() const
{
	return 0;
}

//----------------

CDX11SamplerCache::CDX11SamplerCache( ID3D11Device* device )
	: m_device( device )
{
}

CDX11SamplerCache::~CDX11SamplerCache()
{
	Clear();
}

void CDX11SamplerCache::Clear()
{
	for ( const auto& it : m_samplers )
	{
		if ( it.second )
		{
			it.second->Release();
		}
	}
}

ID3D11SamplerState* CDX11SamplerCache::GetSamplerState( const SamplerInfo& info )
{
	// find in cache
	const auto it = m_samplers.find( info.GetHash() );
	if ( it != m_samplers.end() )
		return it->second;

	// create new
	auto* sampler = CreateSamplerState( m_device, info );
	m_samplers[ info.GetHash() ] = sampler;
	return sampler;
}

ID3D11SamplerState* CDX11SamplerCache::CreateSamplerState( ID3D11Device* dev, const SamplerInfo& info )
{
	// setup sampler state from generic description
	D3D11_SAMPLER_DESC desc;
	memset( &desc, 0, sizeof(desc) );
	desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	desc.MinLOD = 0;
	desc.MaxLOD = 16;

	// create DX11 sampler state object
	ID3D11SamplerState* samplerState = nullptr;
	HRESULT hRet = dev->CreateSamplerState( &desc, &samplerState );
	DEBUG_CHECK( SUCCEEDED(hRet) && samplerState );
	return samplerState;
}