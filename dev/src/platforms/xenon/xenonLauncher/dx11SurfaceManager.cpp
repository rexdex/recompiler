#include "build.h"
#include "dx11SurfaceManager.h"
#include "dx11SurfaceCache.h"
#include "dx11SurfaceMemory.h"
#include "dx11TextureManager.h"
#include "xenonGPUUtils.h"

//#pragma optimize("",off)

//-------------------------------------------------------

bool CDX11SurfaceManager::EDRAMRenderTarget::IsMatching( CDX11AbstractRenderTarget* rt ) const
{
	// invalid surface, everything matches that (by convention)
	if ( !rt )
		return true;

	// not assigned to EDRAM
	if ( rt->GetEDRAMPlacement() < 0 )
		return false;

	// check match
	if ( rt->GetFormat() != m_format )
		return false;
	if ( rt->GetMSAA() != m_msaa )
		return false;
	if ( rt->GetMemoryPitch() != m_pitch )
		return false;
	if ( rt->GetEDRAMPlacement() != m_base )
		return false;

	// matching
	return true;
}

bool CDX11SurfaceManager::EDRAMDepthStencil::IsMatching( CDX11AbstractDepthStencil* ds ) const
{
	// invalid surface, everything matches that (by convention)
	if ( !ds )
		return true;

	// not assigned to EDRAM
	if ( ds->GetEDRAMPlacement() < 0 )
		return false;

	// check match
	if ( ds->GetFormat() != m_format )
		return false;
	if ( ds->GetMSAA() != m_msaa )
		return false;
	if ( ds->GetMemoryPitch() != m_pitch )
		return false;
	if ( ds->GetEDRAMPlacement() != m_base )
		return false;

	// matching
	return true;
}

CDX11SurfaceManager::EDRAMSetup::EDRAMSetup()
{
	memset( this, 0 ,sizeof(EDRAMSetup) );
}

void CDX11SurfaceManager::EDRAMSetup::Dump()
{
	if ( m_depth.IsValid() )
	{
		GLog.Log( "EDRAM:  Depth: %hs, %hs, pitch:%d, base: %d", 
			XenonGPUGetDepthRenderTargetFormatName( m_depth.m_format ),
			XenonGPUGetMSAAName( m_depth.m_msaa ),
			m_depth.m_pitch, m_depth.m_base );
	}

	for ( uint32 i=0; i<4; ++i )
	{
		const auto& color = m_color[i];
		if ( color.IsValid() )
		{
			GLog.Log( "EDRAM: Color%d: %hs, %hs, pitch:%d, base: %d", 
				i,
				XenonGPUGetColorRenderTargetFormatName( color.m_format ),
				XenonGPUGetMSAAName( color.m_msaa ),
				color.m_pitch, color.m_base );
		}
	}
}

//-------------------------------------------------------

CDX11SurfaceManager::EDRAMRuntimeSetup::EDRAMRuntimeSetup()
{
	m_surfaceColor[0] = nullptr;
	m_surfaceColor[1] = nullptr;
	m_surfaceColor[2] = nullptr;
	m_surfaceColor[3] = nullptr;
	m_surfaceDepth = nullptr;
}

//-------------------------------------------------------

CDX11SurfaceManager::CDX11SurfaceManager( ID3D11Device* dev, ID3D11DeviceContext* context )
	: m_context( context )
	, m_device( dev )
{
	// create surface cache
	m_cache = new CDX11SurfaceCache( dev );

	// create memory emulator
	m_memory = new CDX11SurfaceMemory( dev, context );
}

CDX11SurfaceManager::~CDX11SurfaceManager()
{
	// release memory emulator
	if ( m_memory )
	{
		delete m_memory;
		m_memory = nullptr;
	}

	// release surface cache
	if ( m_cache )
	{
		delete m_cache;
		m_cache = nullptr;
	}
}

void CDX11SurfaceManager::BindColorRenderTarget( const uint32 index, const XenonColorRenderTargetFormat format, const XenonMsaaSamples msaa, const uint32 base, const uint32 pitch )
{
	DEBUG_CHECK( index < 4 );
	DEBUG_CHECK( pitch != 0 );

	auto& data = m_incomingSetup.m_color[ index ];
	data.m_format = format;
	data.m_msaa = msaa;
	data.m_base = base;
	data.m_pitch = pitch;
}

void CDX11SurfaceManager::BindDepthStencil( const XenonDepthRenderTargetFormat format, const XenonMsaaSamples msaa, const uint32 base, const uint32 pitch )
{
	DEBUG_CHECK( pitch != 0 );

	auto& data = m_incomingSetup.m_depth;
	data.m_format = format;
	data.m_msaa = msaa;
	data.m_base = base;
	data.m_pitch = pitch;
}

void CDX11SurfaceManager::UnbindColorRenderTarget( const uint32 index )
{
	DEBUG_CHECK( index < 4 );

	auto& data = m_incomingSetup.m_color[ index ];
	data.m_pitch = 0; // unbinds the memory
}

void CDX11SurfaceManager::UnbindDepthStencil()
{
	auto& data = m_incomingSetup.m_depth;
	data.m_pitch = 0; // unbinds the memory
}

void CDX11SurfaceManager::Reset()
{
	// clear EDRAM memory (for debug purposes mostly)
	m_memory->Reset();
}

bool CDX11SurfaceManager::RealizeSetup( uint32& outMainWidth,  uint32& outMainHeight )
{
	// dump the incoming setup
	m_incomingSetup.Dump();

	// detach rendering surfaces
	m_context->OMSetRenderTargets( 0, NULL, NULL );

	// we must ensure the EDRAM data is consistent:
	// find surfaces that are currently bound but are not in the incoming setup
	{
		// can we reuse the memory without flushing ?
		if ( !m_incomingSetup.m_depth.IsMatching( m_runtimeSetup.m_surfaceDepth ) )
		{
			auto* ds = m_runtimeSetup.m_surfaceDepth;
			m_runtimeSetup.m_surfaceDepth = nullptr;

			// flush part of this surface that was used directly into the EDRAM memory
			m_memory->CopyIntoEDRAM( ds );

			// release the surface EDRAM binding (this allows it to be reused for something else)
			ds->SetEDRAMPlacement( -1 );
		}

		// flush or keep color surfaces
		for ( uint32 rtIndex=0; rtIndex<4; ++rtIndex )
		{
			// can we reuse the memory without flushing ?
			if ( !m_incomingSetup.m_color[rtIndex].IsMatching( m_runtimeSetup.m_surfaceColor[rtIndex] ) )
			{
				auto* rt = m_runtimeSetup.m_surfaceColor[rtIndex];
				m_runtimeSetup.m_surfaceColor[rtIndex] = nullptr;

				// flush part of this surface that was used directly into the EDRAM memory
				DEBUG_CHECK( rt->GetEDRAMPlacement() >= 0 );
				m_memory->CopyIntoEDRAM( rt );

				// release the surface EDRAM binding (this allows it to be reused for something else)
				rt->SetEDRAMPlacement( -1 );
			}
		}
	}

	// make the actual setup the incoming setup (we may still have render targets that are not yet allocated)
	m_runtimeSetup.m_desc = m_incomingSetup;

	// allocate render targets for new setup and signal to copy existing EDRAM content into the surfaces (unless it's cleared right away)
	{
		// allocate depth surface first
		const auto& setup = m_incomingSetup.m_depth;
		if ( setup.IsValid() )
		{
			// allocate depth/stencil surface if it's not already allocated
			if ( m_runtimeSetup.m_surfaceDepth == nullptr )
			{
				// allocate from surface cache
				m_runtimeSetup.m_surfaceDepth = m_cache->GetDepthStencil( setup.m_format, setup.m_msaa, setup.m_pitch, setup.m_base );
				DEBUG_CHECK( m_runtimeSetup.m_surfaceDepth != nullptr );

				// copy content of this surface FROM what ever is in the EDRAM
				// NOTE: in MANY cases this is not necessary
				// NOTE: we only copy the part we will be using
				m_memory->CopyFromEDRAM( m_runtimeSetup.m_surfaceDepth );
			}
			else
			{
				// it should be placed at the right address
				DEBUG_CHECK( m_runtimeSetup.m_surfaceDepth->GetEDRAMPlacement() == setup.m_base );
			}

			// we must match
			DEBUG_CHECK( setup.IsMatching( m_runtimeSetup.m_surfaceDepth ) );
		}
		else
		{
			// we should not have any surface bound
			DEBUG_CHECK( m_runtimeSetup.m_surfaceDepth == nullptr );
		}

		// allocate color surfaces
		for ( uint32 rtIndex=0; rtIndex<4; ++rtIndex )
		{
			const auto& setup = m_incomingSetup.m_color[ rtIndex ];
			auto& surfaceRef = m_runtimeSetup.m_surfaceColor[rtIndex];

			if ( setup.IsValid() )
			{
				// allocate depth/stencil surface if it's not already allocated
				if ( surfaceRef == nullptr )
				{
					// allocate from surface cache
					surfaceRef = m_cache->GetRenderTarget( setup.m_format, setup.m_msaa, setup.m_pitch, setup.m_base );
					DEBUG_CHECK( surfaceRef != nullptr );

					// copy content of this surface FROM what ever is in the edram
					// NOTE: in MANY cases this is not necessary
					// NOTE: we only copy the part we will be using
					m_memory->CopyFromEDRAM( surfaceRef );
				}
				else
				{
					// it should be placed at the right address
					DEBUG_CHECK( surfaceRef->GetEDRAMPlacement() == setup.m_base );
				}

				// we must match
				DEBUG_CHECK( setup.IsMatching( surfaceRef ) );
			}
			else
			{
				// we should not have any surface bound
				DEBUG_CHECK( surfaceRef == nullptr );
			}
		}
	}

	// set the actual rendering
	{
		ID3D11RenderTargetView* rtViews[4] = { NULL, NULL, NULL, NULL };
		ID3D11DepthStencilView* dsView = NULL;

		// extract depth stencil
		{
			auto* ds = m_runtimeSetup.m_surfaceDepth;
			if ( ds )
			{
				ds->BindRuntimeContext( m_context );
				dsView = ds->GetBindableView();
			}
		}

		// extract color views
		for ( uint32 i=0; i<4; ++i )
		{
			auto* rt = m_runtimeSetup.m_surfaceColor[i];
			if ( rt )
			{
				rt->BindRuntimeContext( m_context );
				rtViews[i] = rt->GetBindableView();
			}
		}

		// set on the actual device
		m_context->OMSetRenderTargets( ARRAYSIZE(rtViews), rtViews, dsView );

		// hack
		if ( dsView )
			m_context->ClearDepthStencilView( dsView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0 );
	}

	// export the main color surface width
	if ( m_runtimeSetup.m_surfaceColor[0] != NULL )
	{
		outMainWidth = m_runtimeSetup.m_surfaceColor[0]->GetPhysicalWidth();
		outMainHeight = m_runtimeSetup.m_surfaceColor[0]->GetPhysicalHeight();
		return true;
	}
	else if ( m_runtimeSetup.m_surfaceDepth != NULL )
	{
		outMainWidth = m_runtimeSetup.m_surfaceDepth->GetPhysicalWidth();
		outMainHeight = m_runtimeSetup.m_surfaceDepth->GetPhysicalHeight();
		return true;
	}
	else
	{
		// nothing to render to
		return false;
	}
}

bool CDX11SurfaceManager::ResolveColor( const uint32 srcIndex, const XenonColorRenderTargetFormat srcFormat, const uint32 srcBase, const XenonRect2D& srcRect, CDX11AbstractTexture* destTexture, const XenonRect2D& destRect )
{
	// destiation texture MUST be given
	DEBUG_CHECK( destTexture != nullptr );
	if ( !destTexture )
		return false;

	// check RT index
	DEBUG_CHECK( srcIndex < 4 );
	if ( srcIndex >= 4 )
		return false;

	// clear the surface if bounded
	auto* rt = m_runtimeSetup.m_surfaceColor[srcIndex];
	if ( rt == nullptr )
	{
		GLog.Err( "EDRAM: No color surface at RT%d to copy to EDRAM", srcIndex );
		return false;
	}

	// rt surface is NOT bound to EDRAM
	if ( rt->GetEDRAMPlacement() == -1 )
	{
		GLog.Err( "EDRAM: Color surface at RT%d not bound to EDRAM", srcIndex );
		return false;
	}

	// lie - different BASE for the edram surface
	DEBUG_CHECK( rt->GetEDRAMPlacement() == srcBase );

	// validate destSize <= surfaceSize
	DEBUG_CHECK( srcRect.x >= 0 );
	DEBUG_CHECK( srcRect.y >= 0 );
	DEBUG_CHECK( srcRect.w <= destRect.w );
	DEBUG_CHECK( srcRect.h <= destRect.h );

	// setup looks right, resolve into the FIRST surface of the texture
	auto* destSurf = static_cast< CDX11AbstractSurface* >( destTexture->GetSurface(0,0) );
	DEBUG_CHECK( destSurf != nullptr );
	if ( !destSurf )
		return false;

	// last size check
	DEBUG_CHECK( destRect.x + destRect.w <= (int)destSurf->GetWidth() );
	DEBUG_CHECK( destRect.y + destRect.h <= (int)destSurf->GetHeight() );

	// resolve using the memory manager (it has all the means to do so)
	m_memory->Resolve( srcFormat, srcBase, rt->GetMemoryPitch(), srcRect.x, srcRect.y, destRect.x, destRect.y, destRect.w, destRect.h, destSurf );
	return true;
}

void CDX11SurfaceManager::ClearColor( const uint32 index, const float* clearColor, const bool flushToEDRAM )
{
	DEBUG_CHECK( index < 4 );

	// clear the surface if bounded
	auto* rt = m_runtimeSetup.m_surfaceColor[index];
	if ( nullptr != rt )
	{
		// clear the surface		
		rt->Clear( clearColor );

		// copy the FULL surface to EDRAM
		if ( flushToEDRAM )
		{
			m_memory->CopyIntoEDRAM( rt );
		}
	}
}

void CDX11SurfaceManager::ClearEDRAM( const float* clearColor )
{
	// hacks for special "shaders" that are used to clear the EDRAM

	const auto& color0 = m_runtimeSetup.m_desc.m_color[0];
	if ( color0.m_msaa == XenonMsaaSamples::MSSA4X )
	{
		const auto base = color0.m_base;
		const auto pitch = color0.m_pitch;
		const auto height = (1024*1024*10) / color0.m_pitch; // TODO: remove the depth surface from this!
		m_memory->ClearInEDRAM( color0.m_format, base, pitch, height, clearColor );
	}
}

void CDX11SurfaceManager::ClearDepth( const float depthClear, const uint32 stencilClear, const bool flushToEDRAM )
{
	// clear the surface if bounded
	auto* rt = m_runtimeSetup.m_surfaceDepth;
	if ( nullptr != rt )
	{
		// clear the surface
		rt->Clear( true, true, depthClear, stencilClear );

		// copy the FULL surface to EDRAM
		if ( flushToEDRAM )
		{
			m_memory->CopyIntoEDRAM( rt );
		}
	}
}

void CDX11SurfaceManager::ResolveSwap( ID3D11Texture2D* frontBuffer )
{
	DEBUG_CHECK( frontBuffer != nullptr );

	// for now, copy current color0 RT
	auto* color0 = m_runtimeSetup.m_surfaceColor[0];
	ID3D11Texture2D* srcTexture = color0 ? color0->GetTexture() : nullptr;
	if ( !srcTexture )
		return;

	// get the dimensions of the target area
	D3D11_TEXTURE2D_DESC frontBufferDesc;
	memset( &frontBufferDesc, 0, sizeof(frontBufferDesc) );
	frontBuffer->GetDesc( &frontBufferDesc );

	// get maximum dimensions of the copy
	const uint32 maxWidth = frontBufferDesc.Width;
	const uint32 maxHeight = frontBufferDesc.Height;

	// calculate the amount of data to copy
	const uint32 copyMinX = 0;
	const uint32 copyMinY = 0;
	const uint32 copyMaxX = maxWidth;
	const uint32 copyMaxY = maxHeight;

	// setup destination rect
	D3D11_BOX destBox;
	destBox.left = copyMinX;
	destBox.top = copyMinY;
	destBox.front = 0;
	destBox.right = copyMaxX;
	destBox.bottom = copyMaxY;
	destBox.back = 1;

	// copy!
	m_context->CopySubresourceRegion( 
		frontBuffer, 0,  // front buffer mip0
		copyMinX, copyMinY, 0, // front buffer destination,
		srcTexture, 0, // mip0 of the color0 surface
		&destBox );		
}
