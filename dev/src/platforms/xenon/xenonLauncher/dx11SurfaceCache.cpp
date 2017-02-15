#include "build.h"
#include "dx11AbstractLayer.h"
#include "dx11SurfaceCache.h"

//#pragma optimize("",off)

//-----------------------------------------------------------------------------

CDX11AbstractRenderTarget::CDX11AbstractRenderTarget()
	: m_texture( nullptr )
	, m_srvUint( nullptr )
	, m_srvUnorm( nullptr )
	, m_rtv( nullptr )
	, m_uav( nullptr )
	, m_edramPlacement( -1 )
	, m_sourceFormat( (XenonColorRenderTargetFormat) -1 )
	, m_sourceMSAA( (XenonMsaaSamples)-1 )
	, m_sourcePitch( 0 )
	, m_actualFormat( DXGI_FORMAT_UNKNOWN )
	, m_runtimeContext( nullptr )
{
}

CDX11AbstractRenderTarget::~CDX11AbstractRenderTarget()
{
	/// release shader view
	if ( m_srvUint )
	{
		m_srvUint->Release();
		m_srvUint = nullptr;
	}

	/// release shader view
	if ( m_srvUnorm )
	{
		m_srvUnorm->Release();
		m_srvUnorm = nullptr;
	}

	// release unordered access view
	if ( m_uav )
	{
		m_uav->Release();
		m_uav = nullptr;
	}

	/// relase render target view
	if ( m_rtv )
	{
		m_rtv->Release();
		m_rtv = nullptr;
	}

	/// release the resource itself
	if ( m_texture )
	{
		m_texture->Release();
		m_texture = nullptr;
	}
}

void CDX11AbstractRenderTarget::BindRuntimeContext( ID3D11DeviceContext* context )
{
	m_runtimeContext = context;
}

XenonColorRenderTargetFormat CDX11AbstractRenderTarget::GetFormat() const
{
	return m_sourceFormat;
}

XenonMsaaSamples CDX11AbstractRenderTarget::GetMSAA() const
{
	return m_sourceMSAA;
}

uint32 CDX11AbstractRenderTarget::GetMemoryPitch() const
{
	return m_sourcePitch;
}

int32 CDX11AbstractRenderTarget::GetEDRAMPlacement() const
{
	return m_edramPlacement;
}

void CDX11AbstractRenderTarget::SetEDRAMPlacement( const int32 placement )
{
	m_edramPlacement = placement;
}

void CDX11AbstractRenderTarget::Clear( const float* clearColor )
{
	DEBUG_CHECK( m_runtimeContext != nullptr );
	DEBUG_CHECK( m_rtv != nullptr );

	m_runtimeContext->ClearRenderTargetView( m_rtv, clearColor );
}

DXGI_FORMAT CDX11AbstractRenderTarget::TranslateFormatUnorm( XenonColorRenderTargetFormat format )
{
	switch ( format )
	{
		case XenonColorRenderTargetFormat::Format_8_8_8_8: return DXGI_FORMAT_R8G8B8A8_UNORM;
		case XenonColorRenderTargetFormat::Format_8_8_8_8_GAMMA: return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		case XenonColorRenderTargetFormat::Format_2_10_10_10: return DXGI_FORMAT_R10G10B10A2_UNORM;
		case XenonColorRenderTargetFormat::Format_16_16: return DXGI_FORMAT_R16G16_UNORM;
		case XenonColorRenderTargetFormat::Format_16_16_16_16: return DXGI_FORMAT_R16G16B16A16_UNORM;
		case XenonColorRenderTargetFormat::Format_16_16_FLOAT: return DXGI_FORMAT_R16G16_FLOAT;
		case XenonColorRenderTargetFormat::Format_16_16_16_16_FLOAT: return DXGI_FORMAT_R16G16B16A16_FLOAT;
		case XenonColorRenderTargetFormat::Format_32_FLOAT: return DXGI_FORMAT_R32_FLOAT;
		case XenonColorRenderTargetFormat::Format_32_32_FLOAT: return DXGI_FORMAT_R32G32_FLOAT;
	}

	//case XenonColorRenderTargetFormat::Format_2_10_10_10_FLOAT
	//case XenonColorRenderTargetFormat::Format_2_10_10_10_unknown
	//case XenonColorRenderTargetFormat::Format_2_10_10_10_FLOAT_unknown

	DEBUG_CHECK( "Untranslatable RT format" );
	return DXGI_FORMAT_UNKNOWN;
}

DXGI_FORMAT CDX11AbstractRenderTarget::TranslateFormatUint( XenonColorRenderTargetFormat format )
{
	switch ( format )
	{
		case XenonColorRenderTargetFormat::Format_8_8_8_8: return DXGI_FORMAT_R8G8B8A8_UINT;
		case XenonColorRenderTargetFormat::Format_8_8_8_8_GAMMA: return DXGI_FORMAT_R8G8B8A8_UINT;
		case XenonColorRenderTargetFormat::Format_2_10_10_10: return DXGI_FORMAT_R10G10B10A2_UINT;
		case XenonColorRenderTargetFormat::Format_16_16: return DXGI_FORMAT_R16G16_UINT;
		case XenonColorRenderTargetFormat::Format_16_16_16_16: return DXGI_FORMAT_R16G16B16A16_UINT;
	}

	//case XenonColorRenderTargetFormat::Format_2_10_10_10_FLOAT
	//case XenonColorRenderTargetFormat::Format_2_10_10_10_unknown
	//case XenonColorRenderTargetFormat::Format_2_10_10_10_FLOAT_unknown
	//case XenonColorRenderTargetFormat::Format_16_16_FLOAT: return DXGI_FORMAT_R16G16_UINT;
	//case XenonColorRenderTargetFormat::Format_16_16_16_16_FLOAT: return DXGI_FORMAT_R16G16B16A16_UINT;
	//case XenonColorRenderTargetFormat::Format_32_FLOAT: return DXGI_FORMAT_R32_UINT;
	//case XenonColorRenderTargetFormat::Format_32_32_FLOAT: return DXGI_FORMAT_R32G32_UINT;

	DEBUG_CHECK( "Untranslatable RT format" );
	return DXGI_FORMAT_UNKNOWN;
}

DXGI_FORMAT CDX11AbstractRenderTarget::TranslateTyplessFormat( XenonColorRenderTargetFormat format )
{
	switch ( format )
	{
		case XenonColorRenderTargetFormat::Format_8_8_8_8: return DXGI_FORMAT_R8G8B8A8_TYPELESS;
		case XenonColorRenderTargetFormat::Format_8_8_8_8_GAMMA: return DXGI_FORMAT_R8G8B8A8_TYPELESS;
		case XenonColorRenderTargetFormat::Format_2_10_10_10: return DXGI_FORMAT_R10G10B10A2_TYPELESS;
		case XenonColorRenderTargetFormat::Format_16_16: return DXGI_FORMAT_R16G16_TYPELESS;
		case XenonColorRenderTargetFormat::Format_16_16_16_16: return DXGI_FORMAT_R16G16B16A16_TYPELESS;
		case XenonColorRenderTargetFormat::Format_16_16_FLOAT: return DXGI_FORMAT_R16G16_TYPELESS;
		case XenonColorRenderTargetFormat::Format_16_16_16_16_FLOAT: return DXGI_FORMAT_R16G16B16A16_TYPELESS;
		case XenonColorRenderTargetFormat::Format_32_FLOAT: return DXGI_FORMAT_R32_TYPELESS;
		case XenonColorRenderTargetFormat::Format_32_32_FLOAT: return DXGI_FORMAT_R32G32_TYPELESS;
	}

	//case XenonColorRenderTargetFormat::Format_2_10_10_10_FLOAT
	//case XenonColorRenderTargetFormat::Format_2_10_10_10_unknown
	//case XenonColorRenderTargetFormat::Format_2_10_10_10_FLOAT_unknown

	DEBUG_CHECK( "Untranslatable RT format" );
	return DXGI_FORMAT_UNKNOWN;
}

uint32 CDX11AbstractRenderTarget::GetFormatPixelSize( XenonColorRenderTargetFormat format )
{
	switch ( format )
	{
		case XenonColorRenderTargetFormat::Format_8_8_8_8: return 4;
		case XenonColorRenderTargetFormat::Format_8_8_8_8_GAMMA: return 4;
		case XenonColorRenderTargetFormat::Format_2_10_10_10: return 4;
		case XenonColorRenderTargetFormat::Format_16_16: return 4;
		case XenonColorRenderTargetFormat::Format_16_16_16_16: return 8;
		case XenonColorRenderTargetFormat::Format_16_16_FLOAT: return 4;
		case XenonColorRenderTargetFormat::Format_16_16_16_16_FLOAT: return 8;
		case XenonColorRenderTargetFormat::Format_32_FLOAT: return 4;
		case XenonColorRenderTargetFormat::Format_32_32_FLOAT: return 8;
		case XenonColorRenderTargetFormat::Format_2_10_10_10_FLOAT: return 4;
		case XenonColorRenderTargetFormat::Format_2_10_10_10_unknown: return 4;
		case XenonColorRenderTargetFormat::Format_2_10_10_10_FLOAT_unknown: return 4;
	}

	DEBUG_CHECK( "Untranslatable RT format" );
	return 0;
}

uint32 CDX11AbstractRenderTarget::GetMSSASampleCount( XenonMsaaSamples msaa )
{
	switch ( msaa )
	{
		case XenonMsaaSamples::MSSA1X: return 1;
		case XenonMsaaSamples::MSSA2X: return 2;
		case XenonMsaaSamples::MSSA4X: return 4;
	}

	DEBUG_CHECK( "Untranslatable MSAA format" );
	return 0;

}

uint32 CDX11AbstractRenderTarget::GetFormatPixelSizeWithMSAA( XenonColorRenderTargetFormat format, XenonMsaaSamples msaa )
{
	const uint32 basePixelSize = GetFormatPixelSize( format );
	const uint32 samples = GetMSSASampleCount( msaa );
	return basePixelSize * samples;
}

CDX11AbstractRenderTarget* CDX11AbstractRenderTarget::Create( ID3D11Device* device, XenonColorRenderTargetFormat format, XenonMsaaSamples msaa, const uint32 pitch )
{
	// get the format
	const DXGI_FORMAT fullFormatUnorm = TranslateFormatUnorm( format );
	const DXGI_FORMAT fullFormatUint = TranslateFormatUint( format );
	const DXGI_FORMAT typlessFormat = TranslateTyplessFormat( format );

	// setup texture desc
	D3D11_TEXTURE2D_DESC desc;
	memset( &desc, 0, sizeof(desc) );
	desc.ArraySize = 1;
	desc.Format = typlessFormat;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET | D3D11_BIND_UNORDERED_ACCESS;
	desc.CPUAccessFlags = 0;
	desc.MipLevels = 1; // this is a raw render target
	desc.Usage = D3D11_USAGE_DEFAULT;

	// compute width 
	const uint32 pixelSize = GetFormatPixelSizeWithMSAA( format, msaa );
	const uint32 expectedWidth = pitch;//  ;
	desc.Width = expectedWidth;

	// compute maximum height
	const uint32 maxEDRAMSize = 10*1024*1024;
	const uint32 maxHeight = maxEDRAMSize / (pixelSize * pitch);
	desc.Height = maxHeight;

	// MSAA
	desc.SampleDesc.Count = 1;//GetMSSASampleCount( msaa );
	desc.SampleDesc.Quality = 0;
	
	// create render target
	ID3D11Texture2D* tex = nullptr;
	HRESULT hRet = device->CreateTexture2D( &desc, NULL, &tex );
	if ( !tex || FAILED(hRet) )
	{
		GLog.Err( "D3D: Failed to create RT texture: 0x%X, width:%d, height:%d", hRet, desc.Width, desc.Height );
		return nullptr;
	}
	
	// create render target view
	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
	memset( &rtvDesc, 0, sizeof(rtvDesc) );
	rtvDesc.Format = fullFormatUnorm;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	ID3D11RenderTargetView* rtv = nullptr;
	hRet = device->CreateRenderTargetView( tex, &rtvDesc, &rtv );
	if ( !rtv || FAILED(hRet) )
	{
		tex->Release();
		GLog.Err( "D3D: Failed to create RTV for RTV texture: 0x%X, width:%d, height:%d", hRet, desc.Width, desc.Height );
		return nullptr;
	}

	// create shader resource view for normalized read
	ID3D11ShaderResourceView* srvUnorm = nullptr;
	if ( fullFormatUnorm != DXGI_FORMAT_UNKNOWN )
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		memset( &srvDesc, 0, sizeof(srvDesc) );
		srvDesc.Format = fullFormatUnorm;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		hRet = device->CreateShaderResourceView( tex, &srvDesc, &srvUnorm );
		if ( !srvUnorm || FAILED(hRet) )
		{
			rtv->Release();
			tex->Release();
			GLog.Err( "D3D: Failed to create SRV for RT texture: 0x%X, width:%d, height:%d", hRet, desc.Width, desc.Height );
			return nullptr;
		}
	}

	// create shader resource view for integer read
	ID3D11ShaderResourceView* srvUint = nullptr;
	if ( fullFormatUint != DXGI_FORMAT_UNKNOWN )
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		memset( &srvDesc, 0, sizeof(srvDesc) );
		srvDesc.Format = fullFormatUint;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		hRet = device->CreateShaderResourceView( tex, &srvDesc, &srvUint );
		if ( !srvUint || FAILED(hRet) )
		{
			if ( srvUnorm ) srvUnorm->Release();
			rtv->Release();
			tex->Release();
			GLog.Err( "D3D: Failed to create SRV for RT texture: 0x%X, width:%d, height:%d", hRet, desc.Width, desc.Height );
			return nullptr;
		}
	}

	// create unordered access view
	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	memset( &uavDesc, 0, sizeof(uavDesc) );
	uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UINT;	//desc.Format; // the UAV must match the format of the surface, we have a separate compute shader for each format though
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;
	ID3D11UnorderedAccessView* uav = nullptr;
	hRet = device->CreateUnorderedAccessView( tex, &uavDesc, &uav );
	if ( !uav || FAILED(hRet) )
	{
		if ( srvUnorm ) srvUnorm->Release();
		if ( srvUint ) srvUint->Release();
		rtv->Release();
		tex->Release();
		GLog.Err( "D3D: Failed to create UAV for RT texture: 0x%X, width:%d, height:%d", hRet, desc.Width, desc.Height );
		return nullptr;
	}

	// build final RT wrapper
	CDX11AbstractRenderTarget* ret = new CDX11AbstractRenderTarget();
	ret->m_texture = tex;
	ret->m_srvUnorm = srvUnorm;
	ret->m_srvUint = srvUint;
	ret->m_rtv = rtv;
	ret->m_uav = uav;
	ret->m_actualFormat = desc.Format;
	ret->m_sourceFormat = format;
	ret->m_sourceMSAA = msaa;
	ret->m_sourcePitch = pitch;
	ret->m_edramPlacement = -1;
	ret->m_physicalWidth = desc.Width;
	ret->m_physicalHeight = desc.Height;
	return ret;
}

//-----------------------------------------------------------------------------

CDX11AbstractDepthStencil::CDX11AbstractDepthStencil()
	: m_texture( nullptr )
	, m_srv( nullptr )
	, m_dsv( nullptr )
	, m_edramPlacement( -1 )
	, m_sourceFormat( (XenonDepthRenderTargetFormat) -1 )
	, m_sourceMSAA( (XenonMsaaSamples)-1 )
	, m_sourcePitch( 0 )
	, m_actualFormat( DXGI_FORMAT_UNKNOWN )
	, m_runtimeContext( NULL )
{
}

CDX11AbstractDepthStencil::~CDX11AbstractDepthStencil()
{
	/// release shader view
	if ( m_srv )
	{
		m_srv->Release();
		m_srv = nullptr;
	}

	/// release depth stencil view
	if ( m_dsv )
	{
		m_dsv->Release();
		m_dsv = nullptr;
	}

	/// release the resource itself
	if ( m_texture )
	{
		m_texture->Release();
		m_texture = nullptr;
	}
}

void CDX11AbstractDepthStencil::BindRuntimeContext( ID3D11DeviceContext* context )
{
	m_runtimeContext = context;
}

void CDX11AbstractDepthStencil::Clear( const bool clearDepth, const bool clearStencil, const float depthValue, const uint32 stencilValue )
{
	DEBUG_CHECK( m_runtimeContext != nullptr );
	DEBUG_CHECK( m_dsv != nullptr );
	DEBUG_CHECK( clearDepth || clearStencil );

	DWORD flags = 0;
	flags |= clearDepth ? D3D11_CLEAR_DEPTH : 0;
	flags |= clearStencil ? D3D11_CLEAR_STENCIL : 0;
	
	m_runtimeContext->ClearDepthStencilView( m_dsv, flags, depthValue, stencilValue );
}

XenonDepthRenderTargetFormat CDX11AbstractDepthStencil::GetFormat() const
{
	return m_sourceFormat;
}

XenonMsaaSamples CDX11AbstractDepthStencil::GetMSAA() const
{
	return m_sourceMSAA;
}

uint32 CDX11AbstractDepthStencil::GetMemoryPitch() const
{
	return m_sourcePitch;
}

int32 CDX11AbstractDepthStencil::GetEDRAMPlacement() const
{
	return m_edramPlacement;
}

void CDX11AbstractDepthStencil::SetEDRAMPlacement( const int32 placement )
{
	m_edramPlacement = placement;
}

DXGI_FORMAT CDX11AbstractDepthStencil::TranslateFormat( XenonDepthRenderTargetFormat format )
{
	switch ( format )
	{
		case XenonDepthRenderTargetFormat::Format_D24FS8: return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
		case XenonDepthRenderTargetFormat::Format_D24S8: return DXGI_FORMAT_D24_UNORM_S8_UINT;
	}

	DEBUG_CHECK( "Untranslatable DS format" );
	return DXGI_FORMAT_UNKNOWN;
}

uint32 CDX11AbstractDepthStencil::GetFormatPixelSize( XenonDepthRenderTargetFormat format )
{
	switch ( format )
	{
		case XenonDepthRenderTargetFormat::Format_D24FS8: return 4;
		case XenonDepthRenderTargetFormat::Format_D24S8: return 4;
	}

	DEBUG_CHECK( "Untranslatable RT format" );
	return 0;
}

uint32 CDX11AbstractDepthStencil::GetMSSASampleCount( XenonMsaaSamples msaa )
{
	switch ( msaa )
	{
		case XenonMsaaSamples::MSSA1X: return 1;
		case XenonMsaaSamples::MSSA2X: return 2;
		case XenonMsaaSamples::MSSA4X: return 4;
	}

	DEBUG_CHECK( "Untranslatable MSAA format" );
	return 0;
}

uint32 CDX11AbstractDepthStencil::GetFormatPixelSizeWithMSAA( XenonDepthRenderTargetFormat format, XenonMsaaSamples msaa )
{
	const uint32 basePixelSize = GetFormatPixelSize( format );
	const uint32 samples = GetMSSASampleCount( msaa );
	return basePixelSize * samples;
}

CDX11AbstractDepthStencil* CDX11AbstractDepthStencil::Create( ID3D11Device* device, XenonDepthRenderTargetFormat format, XenonMsaaSamples msaa, const uint32 pitch )
{
	// setup texture desc
	D3D11_TEXTURE2D_DESC desc;
	memset( &desc, 0, sizeof(desc) );
	desc.ArraySize = 1;
	desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	desc.CPUAccessFlags = 0;
	desc.Format = TranslateFormat( format );
	desc.MipLevels = 1; // this is a raw render target
	desc.Usage = D3D11_USAGE_DEFAULT;

	// compute width 
	const uint32 pixelSize = GetFormatPixelSizeWithMSAA( format, msaa );
	const uint32 expectedWidth = pitch;//  ;
	desc.Width = expectedWidth;

	// compute maximum height
	const uint32 maxEDRAMSize = 10*1024*1024;
	const uint32 maxHeight = maxEDRAMSize / (pixelSize * pitch);
	desc.Height = maxHeight;

	// MSAA
	desc.SampleDesc.Count = 1;//GetMSSASampleCount( msaa );
	desc.SampleDesc.Quality = 0;

	// create render target
	ID3D11Texture2D* tex = nullptr;
	HRESULT hRet = device->CreateTexture2D( &desc, NULL, &tex );
	if ( !tex || FAILED(hRet) )
	{
		GLog.Err( "D3D: Failed to create RT texture: 0x%X, width:%d, height:%d", hRet, desc.Width, desc.Height );
		return nullptr;
	}

	// create depth stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	memset( &dsvDesc, 0, sizeof(dsvDesc) );
	dsvDesc.Format = desc.Format;

	if ( desc.SampleDesc.Count == 1 )
	{
		dsvDesc.Texture2D.MipSlice = 0;
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	}
	else
	{
		//dsvDesc.Texture2DMS.MipSlice = 0;
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
	}
	
	ID3D11DepthStencilView* dsv = nullptr;
	hRet = device->CreateDepthStencilView( tex, &dsvDesc, &dsv );
	if ( !dsv || FAILED(hRet) )
	{
		tex->Release();
		GLog.Err( "D3D: Failed to create DSV for DS texture: 0x%X, width:%d, height:%d", hRet, desc.Width, desc.Height );
		return nullptr;
	}

/*	// create shader resource view
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	memset( &srvDesc, 0, sizeof(srvDesc) );
	srvDesc.Format = desc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 0;
	ID3D11ShaderResourceView* srv = nullptr;
	hRet = device->CreateShaderResourceView( tex, &srvDesc, &srv );
	if ( !srv || FAILED(hRet) )
	{
		rtv->Release();
		tex->Release();
		GLog.Err( "D3D: Failed to create SRV for RTV texture: 0x%X, width:%d, height:%d", hRet, desc.Width, desc.Height );
		return nullptr;
	}*/

	// build final RT wrapper
	CDX11AbstractDepthStencil* ret = new CDX11AbstractDepthStencil();
	ret->m_texture = tex;
	ret->m_srv = nullptr;
	ret->m_dsv = dsv;
	ret->m_actualFormat = desc.Format;
	ret->m_sourceFormat = format;
	ret->m_sourceMSAA = msaa;
	ret->m_sourcePitch = pitch;
	ret->m_edramPlacement = -1;
	ret->m_physicalWidth = desc.Width;
	ret->m_physicalHeight = desc.Height;
	return ret;
}

//-----------------------------------------------------------------------------

CDX11SurfaceCache::CDX11SurfaceCache( ID3D11Device* dev )
	: m_device( dev )
{
}

CDX11SurfaceCache::~CDX11SurfaceCache()
{
	FreeEDRAM();
	PurgeUnusedResources();
}

CDX11AbstractRenderTarget* CDX11SurfaceCache::GetRenderTarget( XenonColorRenderTargetFormat format, XenonMsaaSamples msaa, const uint32 pitch, const uint32 edramPlacement )
{
	// find matching unused surface
	CDX11AbstractRenderTarget* ret = FindMatchingRT( format, msaa, pitch );
	if ( ret != nullptr )
	{
		ret->SetEDRAMPlacement( edramPlacement );
		return ret;
	}

	// create new surface
	ret = CDX11AbstractRenderTarget::Create( m_device, format, msaa, pitch );
	if ( !ret )
		return nullptr;

	// register and bind
	m_renderTargets.push_back( ret );
	ret->SetEDRAMPlacement( edramPlacement );
	return ret;
}

CDX11AbstractDepthStencil* CDX11SurfaceCache::GetDepthStencil( XenonDepthRenderTargetFormat format, XenonMsaaSamples msaa, const uint32 pitch, const uint32 edramPlacement )
{
	// find matching unused surface
	CDX11AbstractDepthStencil* ret = FindMatchingDS( format, msaa, pitch );
	if ( ret != nullptr )
	{
		ret->SetEDRAMPlacement( edramPlacement );
		return ret;
	}

	// create new surface
	ret = CDX11AbstractDepthStencil::Create( m_device, format, msaa, pitch );
	if ( !ret )
		return nullptr;

	// register and bind
	m_depthStencil.push_back( ret );
	ret->SetEDRAMPlacement( edramPlacement );
	return ret;
}

void CDX11SurfaceCache::FreeEDRAM()
{
	// reset placement on all RTs
	for ( auto* ptr : m_renderTargets )
		ptr->SetEDRAMPlacement( -1 );

	// reset placement on all DSs
	for ( auto* ptr : m_depthStencil )
		ptr->SetEDRAMPlacement( -1 );
}

void CDX11SurfaceCache::PurgeUnusedResources()
{
	// purge render targets
	{
		auto oldList = std::move( m_renderTargets );
		for ( auto* ptr : oldList )
		{
			if ( ptr->GetEDRAMPlacement() == -1 )
			{
				// not used, delete
				delete ptr;
			}
			else
			{
				// still in used
				m_renderTargets.push_back( ptr );
			}
		}
	}

	// purge depth stencil
	{
		auto oldList = std::move( m_depthStencil );
		for ( auto* ptr : oldList )
		{
			if ( ptr->GetEDRAMPlacement() == -1 )
			{
				// not used, delete
				delete ptr;
			}
			else
			{
				// still in used
				m_depthStencil.push_back( ptr );
			}
		}
	}
}

CDX11AbstractRenderTarget* CDX11SurfaceCache::FindMatchingRT( XenonColorRenderTargetFormat format, XenonMsaaSamples msaa, const uint32 pitch )
{
	// find unused RT matching requested structure
	for ( CDX11AbstractRenderTarget* rt : m_renderTargets )
	{
		// matches the format ?
		if ( rt->GetFormat() == format && rt->GetMemoryPitch() == pitch && rt->GetMSAA() == msaa )
		{
			// is free ?
			if ( rt->GetEDRAMPlacement() == -1 )
			{
				return rt;
			}
		}
	}

	// nothing found
	return nullptr;
}

CDX11AbstractDepthStencil* CDX11SurfaceCache::FindMatchingDS( XenonDepthRenderTargetFormat format, XenonMsaaSamples msaa, const uint32 pitch )
{
	// find unused RT matching requested structure
	for ( CDX11AbstractDepthStencil* rt : m_depthStencil )
	{
		// matches the format ?
		if ( rt->GetFormat() == format && rt->GetMemoryPitch() == pitch && rt->GetMSAA() == msaa )
		{
			// is free ?
			if ( rt->GetEDRAMPlacement() == -1 )
			{
				return rt;
			}
		}
	}

	// nothing found
	return nullptr;
}

//-----------------------------------------------------------------------------
