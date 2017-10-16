#include "build.h"
#include "dx11Staging.h"

//---------------------------------------------------------------------------

CDX11StagingTexture::CDX11StagingTexture()
	: m_texture( nullptr )
	, m_width( 0 )
	, m_height( 0 )
	, m_dims( 0 )
	, m_depth( 0 )
	, m_format( DXGI_FORMAT_UNKNOWN )
	, m_locked( false )
	, m_device( nullptr )
	, m_deviceContext( nullptr )
{
}

CDX11StagingTexture::~CDX11StagingTexture()
{
	DEBUG_CHECK( !m_locked );

	if ( m_texture )
	{
		m_texture->Release();
		m_texture = nullptr;
	}
}

void* CDX11StagingTexture::Lock( uint32& outRowPitch, uint32& outSlicePitch )
{
	// make sure texture is NOT alraedy locked
	DEBUG_CHECK( !m_locked );
	if ( m_locked )
		return nullptr;

	// lock data
	D3D11_MAPPED_SUBRESOURCE data;
	HRESULT hRet = m_deviceContext->Map( m_texture, 0, D3D11_MAP_WRITE, 0, &data );
	DEBUG_CHECK( SUCCEEDED(hRet) && data.pData );
	if ( !data.pData )
		return nullptr;

	// enter the locked mode
	m_locked = true;

	// return locked memory layout
	outRowPitch = data.RowPitch;
	outSlicePitch = data.DepthPitch;
	return data.pData;
}

void CDX11StagingTexture::Flush( ID3D11Resource* target, const uint32 targetSlice )
{
	// must be locked
	if ( !m_locked )
		return;

	// unlock
	m_deviceContext->Unmap( m_texture, 0 );
	m_locked = false;

	// setup source region
	D3D11_BOX region;
	region.left = 0;
	region.right = m_width;
	region.top = 0;
	region.bottom = m_height;
	region.front = 0;
	region.back = m_depth;

	// copy to target
	m_deviceContext->CopySubresourceRegion( target, targetSlice, 0, 0, 0, m_texture, 0, &region );
}

CDX11StagingTexture*CDX11StagingTexture::Create( ID3D11Device* dev, ID3D11DeviceContext* context, const uint32 dims, DXGI_FORMAT format, const uint32 width, const uint32 height, const uint32 depth )
{
	// invalid parameters
	if ( !dev || !context )
		return nullptr;

	// create resource
	ID3D11Resource* texture = nullptr;
	if ( dims == 1 )
	{
		DEBUG_CHECK( height == 1 );
		DEBUG_CHECK( depth == 1 );

		D3D11_TEXTURE1D_DESC desc;
		memset( &desc, 0, sizeof(desc) );
		desc.Format = format;
		desc.ArraySize = 1;
		desc.BindFlags = 0; // not bindable
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.MipLevels = 1;
		desc.MiscFlags = 0;
		desc.Usage = D3D11_USAGE_STAGING;
		desc.Width = width;
		
		HRESULT hRet = dev->CreateTexture1D( &desc, NULL, (ID3D11Texture1D**) &texture );
		DEBUG_CHECK( SUCCEEDED(hRet) && (texture != nullptr) );
	}
	else if ( dims == 2 )
	{
		DEBUG_CHECK( depth == 1 );

		D3D11_TEXTURE2D_DESC desc;
		memset( &desc, 0, sizeof(desc) );
		desc.Format = format;
		desc.ArraySize = 1;
		desc.BindFlags = 0; // not bindable
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.MipLevels = 1;
		desc.MiscFlags = 0;
		desc.Usage = D3D11_USAGE_STAGING;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Width = width;
		desc.Height = height;

		HRESULT hRet = dev->CreateTexture2D( &desc, NULL, (ID3D11Texture2D**) &texture );
		DEBUG_CHECK( SUCCEEDED(hRet) && (texture != nullptr) );
	}
	else if ( dims == 3 )
	{
		D3D11_TEXTURE3D_DESC desc;
		memset( &desc, 0, sizeof(desc) );
		desc.Format = format;
		desc.BindFlags = 0; // not bindable
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.MipLevels = 1;
		desc.MiscFlags = 0;
		desc.Usage = D3D11_USAGE_STAGING;
		desc.Width = width;
		desc.Height = height;
		desc.Depth = depth;

		HRESULT hRet = dev->CreateTexture3D( &desc, NULL, (ID3D11Texture3D**) &texture );
		DEBUG_CHECK( SUCCEEDED(hRet) && (texture != nullptr) );
	}

	// no texture created
	if ( !texture )
		return nullptr;

	// create wrapper
	CDX11StagingTexture* ret = new CDX11StagingTexture();
	ret->m_device = dev;
	ret->m_deviceContext = context;
	ret->m_format = format;
	ret->m_width = width;
	ret->m_height = height;
	ret->m_depth = depth;
	ret->m_texture = texture;
	return ret;
}

//---------------------------------------------------------------------------

CDX11StagingTextureCache::CDX11StagingTextureCache( ID3D11Device* dev, ID3D11DeviceContext* context )
	: m_device( dev )
	, m_deviceContext( context )
{
}

CDX11StagingTextureCache::~CDX11StagingTextureCache()
{
	FreeResources();
}

CDX11StagingTexture* CDX11StagingTextureCache::GetStagingTexture( const uint32 dims, DXGI_FORMAT format, const uint32 width, const uint32 height, const uint32 depth )
{
	TScopeLock lock( m_lock );

	// look for existing texture
	// format and size MUST match perfectly
	for ( auto* ptr : m_textures )
	{
		if ( ptr->GetDimmensions() == dims && ptr->GetWidth() == width && ptr->GetHeight() == height && ptr->GetDepth() == depth && ptr->GetFormat() == format )
		{
			return ptr;
		}
	}

	// create new staging texture
	auto* texture = CDX11StagingTexture::Create( m_device, m_deviceContext, dims, format, width, height, depth );
	if ( !texture )
	{
		GLog.Err( "D3D: Failed to create staging texture, dim %d, size %dx%dx%d, format %d", dims, width, height, depth, format );
		return nullptr;
	}

	// add to registry
	GLog.Log( "D3D: Created staging texture, dim %d, size %dx%dx%d, format %d", dims, width, height, depth, format );
	m_textures.push_back( texture );
	return texture;
}

void CDX11StagingTextureCache::FreeResources()
{
	TScopeLock lock( m_lock );

	for ( auto* ptr : m_textures )
		delete ptr;

	m_textures.clear();
}

//---------------------------------------------------------------------------
