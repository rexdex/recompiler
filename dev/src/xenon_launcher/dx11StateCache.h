#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>

#include <map>

#include "xenonGPUUtils.h"

/// State cache
template< typename T >
class TDX11StateMap
{
public:
	typedef uint32 THash;

	TDX11StateMap()
	{
	}

	~TDX11StateMap()
	{
		Clear();
	}

	inline void Clear()
	{
		for ( auto it : m_map )
		{
			if ( it.second )
				it.second->Release();
		}

		m_map.clear();
	}

	inline T* Get( const THash hash )
	{
		const auto it = m_map.find( hash );
		if ( it != m_map.end() )
			return it->second;

		return nullptr;
	}

	inline void Insert( const THash hash, T* ptr )
	{
		m_map[ hash ] = ptr;
	}

	// Compute object hash
	template< typename D >
	static THash ComputeHash( const D& data )
	{
		return (THash) XenonGPUCalcCRC( &data, sizeof(data) );
	}

private:	
	std::map< THash, T* >		m_map;
};

/// DX11 internal state cache
class CDX11StateCache
{
public:
	CDX11StateCache( ID3D11Device* dev, ID3D11DeviceContext* context );
	~CDX11StateCache();

	/// Flush all cached objects
	void PurgeCache();

	/// Set depth/stencil state
	bool SetDepthStencil( const D3D11_DEPTH_STENCIL_DESC& dsState, const uint32 stencilRef );

	/// Set blend state
	bool SetBlendState( const D3D11_BLEND_DESC& blendState, const float blendFactors[4] );

	/// Set rasterizer state
	bool SetRasterState( const D3D11_RASTERIZER_DESC& rasterState );

private:
	// DX11 device, always valid
	ID3D11Device*				m_device;
	ID3D11DeviceContext*		m_mainContext;

	// Cached objects
	typedef TDX11StateMap< ID3D11DepthStencilState >	 TDSStates;
	TDSStates					m_dsStates;

	typedef TDX11StateMap< ID3D11BlendState >			TBlendStates;
	TBlendStates				m_blendStates;

	typedef TDX11StateMap< ID3D11RasterizerState >		TRasterStates;
	TRasterStates				m_rasterStates;
};