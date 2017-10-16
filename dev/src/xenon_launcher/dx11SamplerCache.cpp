#include "build.h"
#include "xenonGPUUtils.h"
#include "xenonGPUTextures.h"
#include "dx11SamplerCache.h"

//----------------

CDX11SamplerCache::CDX11SamplerCache(ID3D11Device* device)
	: m_device(device)
{
}

CDX11SamplerCache::~CDX11SamplerCache()
{
	Clear();
}

void CDX11SamplerCache::Clear()
{
	for (const auto& it : m_samplers)
	{
		if (it.second)
		{
			it.second->Release();
		}
	}
}

ID3D11SamplerState* CDX11SamplerCache::GetSamplerState(const XenonSamplerInfo& info)
{
	// find in cache
	const auto it = m_samplers.find(info.GetHash());
	if (it != m_samplers.end())
		return it->second;

	// create new
	auto* sampler = CreateSamplerState(m_device, info);
	m_samplers[info.GetHash()] = sampler;
	return sampler;
}

static const D3D11_TEXTURE_ADDRESS_MODE TranslateAddressMode(const XenonClampMode mode)
{
	switch (mode)
	{
		case XenonClampMode::Repeat: return D3D11_TEXTURE_ADDRESS_WRAP;
		case XenonClampMode::MirroredRepeat: return D3D11_TEXTURE_ADDRESS_MIRROR;
		case XenonClampMode::ClampToEdge: return D3D11_TEXTURE_ADDRESS_CLAMP;
		case XenonClampMode::ClampToHalfway: return D3D11_TEXTURE_ADDRESS_CLAMP;
		case XenonClampMode::MirrorClampToHalfway: return D3D11_TEXTURE_ADDRESS_MIRROR_ONCE;
		case XenonClampMode::MirrorClampToEdge: return D3D11_TEXTURE_ADDRESS_MIRROR_ONCE;
		case XenonClampMode::MirrorClampToBorder: return D3D11_TEXTURE_ADDRESS_MIRROR_ONCE;
		case XenonClampMode::ClampToBorder: return D3D11_TEXTURE_ADDRESS_BORDER;
	}

	DEBUG_CHECK(!"Unsupported clamp mode");
	return D3D11_TEXTURE_ADDRESS_WRAP;
}

static const D3D11_FILTER TranslateFilterMode(const XenonTextureFilter minMode, const XenonTextureFilter magMode, const XenonTextureFilter mipMode)
{
	if (mipMode == XenonTextureFilter::Point)
	{
		if (magMode == XenonTextureFilter::Point)
		{
			if (minMode == XenonTextureFilter::Point)
				return D3D11_FILTER_MIN_MAG_MIP_POINT;
			else
				return D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
		}
		else
		{
			if (minMode == XenonTextureFilter::Point)
				return D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR;
			else
				return D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
		}
	}
	else
	{
		if (magMode == XenonTextureFilter::Point)
		{
			if (minMode == XenonTextureFilter::Point)
				return D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
			else
				return D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
		}
		else
		{
			if (minMode == XenonTextureFilter::Point)
				return D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR;
			else
				return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		}
	}
}

static const uint32 TranslateAnisotropy(const XenonAnisoFilter filter)
{
	switch (filter)
	{
		case XenonAnisoFilter::Disabled: return 1;
		case XenonAnisoFilter::Max_1_1: return 1;
		case XenonAnisoFilter::Max_2_1: return 2;
		case XenonAnisoFilter::Max_4_1: return 4;
		case XenonAnisoFilter::Max_8_1: return 8;
		case XenonAnisoFilter::Max_16_1: return 16;
	}

	return 1;
}


ID3D11SamplerState* CDX11SamplerCache::CreateSamplerState(ID3D11Device* dev, const XenonSamplerInfo& info)
{
	// setup sampler state from generic description
	D3D11_SAMPLER_DESC desc;
	memset(&desc, 0, sizeof(desc));
	desc.AddressU = TranslateAddressMode(info.m_clampU);
	desc.AddressV = TranslateAddressMode(info.m_clampV);
	desc.AddressW = TranslateAddressMode(info.m_clampW);
	desc.Filter = TranslateFilterMode(info.m_minFilter, info.m_magFilter, info.m_mipFilter);
	desc.MinLOD = 0;
	desc.MaxLOD = 16;
	desc.MaxAnisotropy = TranslateAnisotropy(info.m_anisoFilter);
	desc.MipLODBias = info.m_lodBias;

	// create DX11 sampler state object
	ID3D11SamplerState* samplerState = nullptr;
	HRESULT hRet = dev->CreateSamplerState(&desc, &samplerState);
	DEBUG_CHECK(SUCCEEDED(hRet) && samplerState);
	return samplerState;
}
