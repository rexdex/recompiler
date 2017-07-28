#include "build.h"
#include "dx11AbstractLayer.h"
#include "dx11RenderingWindow.h"
#include "dx11SurfaceManager.h"
#include "dx11TextureManager.h"
#include "dx11GeometryDrawer.h"
#include "dx11StateCache.h"
#include "xenonGPURegisters.h"

#pragma optimize("",off)

//-----------------------------------------------------------------------------------------------

#pragma comment ( lib, "d3d11.lib" )
#pragma comment ( lib, "d3dcompiler.lib" )

//-----------------------------------------------------------------------------------------------

CDX11AbstractLayer::CDX11AbstractLayer()
	: m_device( nullptr )
	, m_mainContext( nullptr )
	, m_window( nullptr )
	, m_swapChain( nullptr )
	, m_backBufferView( nullptr )
	, m_canRender( false )
	, m_viewportDirty( false )
	, m_depthStencilStateDirty( false )
	, m_depthStencilStateStencilRef( 0 )
	, m_blendStateDirty( false )
	, m_rasterStateDirty( false )
	, m_primitiveRestartEnabled( false )
	, m_primitiveRestartIndex( 0xFFFF )
{
	memset( &m_viewport, 0, sizeof(m_viewport) );
	memset( &m_depthStencilState, 0, sizeof(m_depthStencilState) );
	memset( &m_blendState, 0, sizeof(m_blendState) );
	memset( &m_rasterState, 0, sizeof(m_rasterState) );

	m_blendState.AlphaToCoverageEnable = FALSE;
	m_blendState.IndependentBlendEnable = TRUE;

	m_blendStateColor[0] = 1.0f;
	m_blendStateColor[1] = 1.0f;
	m_blendStateColor[2] = 1.0f;
	m_blendStateColor[3] = 1.0f;
}

CDX11AbstractLayer::~CDX11AbstractLayer()
{
	// discard swapchain
	DiscardSwapchain();

	// destroy geometry drawer
	if ( m_drawer )
	{
		delete m_drawer;
		m_drawer = nullptr;
	}

	// destroy state cache
	if ( m_stateCache )
	{
		delete m_stateCache;
		m_stateCache = nullptr;
	}

	// destroy surface manager
	if ( m_surfaceManager )
	{
		delete m_surfaceManager;
		m_surfaceManager = nullptr;
	}

	// destroy texture manager
	if ( m_textureManager )
	{
		delete m_textureManager;
		m_textureManager = nullptr;
	}

	// destroy main context
	if ( m_mainContext )
	{
		m_mainContext->ClearState();
		m_mainContext->Flush();
		m_mainContext->Release();
		m_mainContext = NULL;
	}

	// destroy device
	if ( m_device )
	{
		m_device->Release();
		m_device = nullptr;
	}
}

bool CDX11AbstractLayer::Initialize()
{
	// create the native device
	if ( !CreateDevice() )
		return false;

	// create the runtime window
	m_window = new CDX11RenderingWindow();

	// create swap chain (initial one)
	if ( !CreateSwapchain() )
		return false;

	// create surface manager
	m_surfaceManager = new CDX11SurfaceManager( m_device, m_mainContext );

	// create texture manager
	m_textureManager = new CDX11TextureManager( m_device, m_mainContext );

	// create state cache
	m_stateCache = new CDX11StateCache( m_device, m_mainContext );

	// create geometry drawer
	m_drawer = new CDX11GeometryDrawer( m_device, m_mainContext );
	m_drawer->SetShaderDumpDirector( L"Q:/shaderdump/" );

	// create shader constants buffers
	m_vertexShaderConsts.Create( m_device );
	m_pixelShaderConsts.Create( m_device );
	m_booleanConsts.Create( m_device );

	// done
	GLog.Log( "DX11: Abstract layer initialized" );
	return true;
}

bool CDX11AbstractLayer::SetDisplayMode( const uint32 width, const uint32 height )
{
	m_canRender = false;

	GLog.Log( "DX11: Requesting mode change to %dx%d", width,height );
	return true;
}

void CDX11AbstractLayer::BeingFrame()
{
	// recreate swapchain
	if ( !m_canRender || !m_backBufferView )
	{
		if ( m_swapChain )
			DiscardSwapchain();

		m_canRender = CreateSwapchain();
	}

	// clean to debug color
	if ( m_canRender && m_backBufferView )
	{
		FLOAT debugColor[4] = { 0.2f, 1.0f, 0.2f, 1.0f };
		m_mainContext->ClearRenderTargetView( m_backBufferView, debugColor );
	}
}

void CDX11AbstractLayer::Swap( const CXenonGPUState::SwapState& ss )
{
	if ( !m_canRender )
		return;

	// get the texture at the
	auto* swapTexture = m_textureManager->FindTexture( ss.m_frontBufferBase );
	if ( 0 ) //swapTexture )
	{
		// we have a proper front buffer texture that we resolved scene into, copy it into the swapchain
		ID3D11Texture2D* srcTexture = (ID3D11Texture2D*) swapTexture->GetRuntimeTexture();

		// get the dimensions of the target area
		D3D11_TEXTURE2D_DESC frontBufferDesc;
		memset( &frontBufferDesc, 0, sizeof(frontBufferDesc) );
		m_backBuffer->GetDesc( &frontBufferDesc );

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
		m_mainContext->CopySubresourceRegion( 
			m_backBuffer, 0,  // front buffer mip0
			copyMinX, copyMinY, 0, // front buffer destination,
			srcTexture, 0, // mip0 of the color0 surface
			&destBox );		
	}
	else
	{
		// copy whatever is in the front buffer surface into the actual presentation back buffer
		// NOTE: emulated front buffer is actual back buffer in D3D11 Swapchain, don't get those confused
		m_surfaceManager->ResolveSwap( m_backBuffer );
	}

	// swap the frame
	if ( m_swapChain )
		m_swapChain->Present( 0, 0 );

	// reset surface management for the new frame
	m_surfaceManager->Reset();

	//,,leep(100);

	// reset geometry cache
	m_drawer->Reset();
}


#ifndef _DEBUG
#define _DEBUG
#endif

bool CDX11AbstractLayer::CreateDevice()
{
	// Setup flags
	DWORD deviceFlags = 0;
#ifdef _DEBUG
	deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	// Initialize DX11 device
	D3D_FEATURE_LEVEL featureLevel;
	ID3D11Device* device = nullptr;
	ID3D11DeviceContext* mainContext = nullptr;
	HRESULT hRet = D3D11CreateDevice( NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, deviceFlags, NULL, 0, D3D11_SDK_VERSION, &device, &featureLevel, &mainContext );
	if ( FAILED( hRet ) || !device )
	{
		GLog.Err( "DX11: Unable to create device, err = 0x%08X", hRet );
		return false;
	}

//#ifdef _DEBUG
	// Enable DirectX debugging
	ID3D11Debug* debugInterface = NULL;
	if( SUCCEEDED( device->QueryInterface( __uuidof(ID3D11Debug), (void**)&debugInterface ) ) )
	{
		ID3D11InfoQueue* infoQueue = NULL;
		if( SUCCEEDED( debugInterface->QueryInterface( __uuidof(ID3D11InfoQueue), (void**)&infoQueue ) ) )
		{
			// Disable DEVICE_DRAW_INDEX_BUFFER_TOO_SMALL warnings, that are a confirmed dx11 debug layer bug (fixed in 11.1)
			D3D11_MESSAGE_ID denied [] =
			{
				D3D11_MESSAGE_ID_DEVICE_DRAW_INDEX_BUFFER_TOO_SMALL,
				D3D11_MESSAGE_ID_DEVICE_DRAW_CONSTANT_BUFFER_TOO_SMALL,
				D3D11_MESSAGE_ID_DEVICE_DRAW_RENDERTARGETVIEW_NOT_SET,
			};
			D3D11_INFO_QUEUE_FILTER filter;
			memset( &filter, 0, sizeof(filter) );
			filter.DenyList.NumIDs = _countof(denied);
			filter.DenyList.pIDList = denied;
			infoQueue->AddStorageFilterEntries( &filter );

			//infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, true);
			infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
			infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
		}
	}
//#endif

	// store device
	m_device = device;
	m_mainContext = mainContext;

	// device initialized
	GLog.Log( "DX11: Native device initialized" );
	return true;
}

void CDX11AbstractLayer::DiscardSwapchain()
{
	// discard view
	if ( m_backBufferView != nullptr )
	{
		m_backBufferView->Release();
		m_backBufferView = nullptr;
	}

	// discard back buffer
	if ( m_backBuffer != nullptr )
	{
		m_backBuffer->Release();
		m_backBuffer = nullptr;
	}

	// discard back buffer
	if ( m_swapChain != nullptr )
	{
		m_swapChain->Release();
		m_swapChain = nullptr;
	}

	// stats
	GLog.Log( "DX11: Swapchain discarded" );
}

bool CDX11AbstractLayer::CreateSwapchain()
{
	// Get the GI device from 3D device
	IDXGIDevice * giDevice;
	HRESULT hr = m_device->QueryInterface(__uuidof(IDXGIDevice), (void **)&giDevice);
	if ( FAILED( hr ) )
	{
		GLog.Err( "DXGI: Failed to get adapter GIDevice, hRet: 0x%0X", (uint32)hr );
		return false;
	}

	// Get the adapter info
	IDXGIAdapter * giAdapter;
	hr = giDevice->GetParent(__uuidof(IDXGIAdapter), (void **)&giAdapter);
	if ( FAILED( hr ) )
	{
		GLog.Err( "DXGI: Failed to get adapter GIAdapter, hRet: 0x%0X", (uint32)hr );
		return false;
	}

	// Get the adapter factory
	IDXGIFactory * giFactory;
	hr = giAdapter->GetParent(__uuidof(IDXGIFactory), (void **)&giFactory);
	if ( FAILED( hr ) )
	{
		GLog.Err( "DXGI: Failed to get adapter GIFactory, hRet: 0x%0X", (uint32)hr );
		return false;
	}

	DXGI_SWAP_CHAIN_DESC scDesc;
	memset( &scDesc, 0, sizeof(scDesc) );
	scDesc.BufferCount = 1;
	scDesc.BufferDesc.Width = m_mode.m_width;
	scDesc.BufferDesc.Height = m_mode.m_height;
	scDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scDesc.BufferDesc.RefreshRate.Numerator = 60;
	scDesc.BufferDesc.RefreshRate.Denominator = 1;
	scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scDesc.OutputWindow = (HWND) m_window->GetWindowHandle();
	scDesc.SampleDesc.Count = 1;
	scDesc.SampleDesc.Quality = 0;
	scDesc.Windowed = true;

	// Fullscreen
	if ( m_mode.m_fullscreen )	
	{
		scDesc.Windowed = false;
		scDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	}

	// Create swapchain
	IDXGISwapChain* swapChain = nullptr;
	hr = giFactory->CreateSwapChain( m_device, &scDesc, &swapChain );
	if ( FAILED( hr ) )
	{
		GLog.Err( "DXGI: Failed to create swap chain, hRet: 0x%0X", (uint32)hr );
		return false;
	}

	// Get the back buffer texture
	ID3D11Texture2D* backBuffer = NULL;
	hr = swapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), (LPVOID*)&backBuffer );
	if ( FAILED( hr ) || backBuffer == NULL )
	{
		swapChain->Release();
		GLog.Err( "DXGI: Failed to get back buffer surface, hRet: 0x%0X", (uint32)hr );
		return false;
	}

	// Get render target view for the back buffer
	D3D11_RENDER_TARGET_VIEW_DESC viewDesc;
	memset( &viewDesc, 0, sizeof(viewDesc) );
	viewDesc.Texture2D.MipSlice = 0;
	viewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	viewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	ID3D11RenderTargetView* backBufferView = nullptr;
	hr = m_device->CreateRenderTargetView( backBuffer, &viewDesc, &backBufferView );
	if ( FAILED( hr ) || backBufferView == NULL )
	{
		swapChain->Release();
		backBuffer->Release();
		GLog.Err( "DXGI: Failed to get back buffer view, hRet: 0x%0X", (uint32)hr );
		return false;
	}

	// set stuff
	m_swapChain = swapChain;
	m_backBuffer = backBuffer;
	m_backBufferView = backBufferView;

	// Swapchain created
	GLog.Log( "DX11: Swapchain created, mode: %dx%d, fullscreen: %d", scDesc.BufferDesc.Width, scDesc.BufferDesc.Height, !scDesc.Windowed );
	return true;
}

void CDX11AbstractLayer::BindColorRenderTarget( const uint32 index, const XenonColorRenderTargetFormat format, const XenonMsaaSamples msaa, const uint32 base, const uint32 pitch )
{
	m_surfaceManager->BindColorRenderTarget( index, format, msaa, base, pitch );
}

void CDX11AbstractLayer::BindDepthStencil( const XenonDepthRenderTargetFormat format, const XenonMsaaSamples msaa, const uint32 base, const uint32 pitch )
{
	m_surfaceManager->BindDepthStencil( format, msaa, base, pitch );
}

void CDX11AbstractLayer::UnbindColorRenderTarget( const uint32 index )
{
	m_surfaceManager->UnbindColorRenderTarget( index );
}

void CDX11AbstractLayer::UnbindDepthStencil()
{
	m_surfaceManager->UnbindDepthStencil();
}

void CDX11AbstractLayer::SetColorRenderTargetWriteMask( const uint32 index, const bool enableRed, const bool enableGreen, const bool enableBlue, const bool enableAlpha )
{
	DEBUG_CHECK( index < 4 );

	const UINT8 value = 0 |
		(enableRed ? D3D11_COLOR_WRITE_ENABLE_RED : 0) |
		(enableGreen ? D3D11_COLOR_WRITE_ENABLE_GREEN : 0) |
		(enableBlue ? D3D11_COLOR_WRITE_ENABLE_BLUE : 0) |
		(enableAlpha ? D3D11_COLOR_WRITE_ENABLE_ALPHA : 0);

	auto& data = m_blendState.RenderTarget[index];
	if ( data.RenderTargetWriteMask != value )
	{
		data.RenderTargetWriteMask = value;
		m_blendStateDirty = true;
	}
}

void CDX11AbstractLayer::ClearColorRenderTarget( const uint32 index, const float* clearColor, const bool flushToEDRAM )
{
	m_surfaceManager->ClearColor( index, clearColor, flushToEDRAM );
}

void CDX11AbstractLayer::ClearDepthStencilRenderTarget( const float depthClear, const uint32 stencilClear, const bool flushToEDRAM )
{
	m_surfaceManager->ClearDepth( depthClear, stencilClear, flushToEDRAM );
}

bool CDX11AbstractLayer::RealizeSurfaceSetup( uint32& outMainWidth,  uint32& outMainHeight )
{
	if ( !m_surfaceManager->RealizeSetup( outMainWidth, outMainHeight ) )
		return false;

	m_drawer->SetPhysicalSize( outMainWidth, outMainHeight );
	return true;
}

bool CDX11AbstractLayer::ResolveColorRenderTarget( const uint32 srcIndex, const XenonColorRenderTargetFormat srcFormat, const uint32 srcBase, const XenonRect2D& srcRect, const uint32 destBase, const uint32 destLogicalWidth, const uint32 destLogicalHeight, const uint32 destBlockWidth, const uint32 destBlockHeight, const XenonTextureFormat destFormat, const XenonRect2D& destRect )
{
	// create/get the target texture proxy for a memory at given physical address
	// NOTE: this may reuse existing texture, also this may cause conflicts in memory management (waiting for DX12 I suppose)
	CDX11AbstractTexture* destTexture = m_textureManager->GetTexture( destBase, destLogicalWidth, destLogicalHeight, destFormat );
	if ( !destTexture )
		return false;

	// copy the EDRAM content into a runtime texture
	return m_surfaceManager->ResolveColor( srcIndex, srcFormat, srcBase, srcRect, destTexture, destRect );
}

bool CDX11AbstractLayer::ResolveDepthRenderTarget( const XenonDepthRenderTargetFormat srcFormat, const uint32 srcBase, const XenonRect2D& srcRect, const uint32 destBase, const uint32 destLogicalWidth, const uint32 destLogicalHeight, const uint32 destBlockWidth, const uint32 destBlockHeight, const XenonTextureFormat destFormat, const XenonRect2D& destRect )
{
	// create/get the target texture proxy for a memory at given physical address
	// NOTE: this may reuse existing texture, also this may cause conflicts in memory management (waiting for DX12 I suppose)
	CDX11AbstractTexture* destTexture = m_textureManager->GetTexture(destBase, destLogicalWidth, destLogicalHeight, destFormat);
	if (!destTexture)
		return false;

	// copy the EDRAM content into a runtime texture
	return m_surfaceManager->ResolveDepth(srcFormat, srcBase, srcRect, destTexture, destRect);
}

//---------------------------------------------------------------------------

void CDX11AbstractLayer::SetViewportVertexFormat( const bool xyDivied, const bool zDivied, const bool wInverted )
{
	m_drawer->SetViewportVertexFormat( xyDivied, zDivied, wInverted );
}

void CDX11AbstractLayer::SetViewportWindowScale( const bool bNormalizedXYCoordinates )
{
	m_drawer->SetViewportWindowScale( bNormalizedXYCoordinates );
}

void CDX11AbstractLayer::EnableScisor( const uint32 x, const uint32 y, const uint32 w, const uint32 h )
{
	D3D11_RECT scissorRect;
	scissorRect.left = x;
	scissorRect.top = y;
	scissorRect.right = x + w;
	scissorRect.bottom = y + h;
	m_mainContext->RSSetScissorRects( 1, &scissorRect );

	if ( m_rasterState.ScissorEnable != TRUE )
	{
		m_rasterState.ScissorEnable = TRUE;
		m_rasterStateDirty = true;
	}
}

void CDX11AbstractLayer::DisableScisor()
{
	if ( m_rasterState.ScissorEnable != FALSE )
	{
		m_rasterState.ScissorEnable = FALSE;
		m_rasterStateDirty = true;
	}
}

void CDX11AbstractLayer::SetViewportRange( const float x, const float y, const float w, const float h )
{
	m_viewport.TopLeftX = x;
	m_viewport.TopLeftY = y;
	m_viewport.Width = w;
	m_viewport.Height = h;
	m_viewportDirty = true;
}

void CDX11AbstractLayer::SetDepthRange( const float offset, const float scale )
{
	m_viewport.MinDepth = offset;
	m_viewport.MaxDepth = offset + scale;
	m_viewportDirty = true;
}

bool CDX11AbstractLayer::RealizeViewportSetup()
{
	// update viewport
	if ( m_viewportDirty )
	{
		m_mainContext->RSSetViewports( 1, &m_viewport );
		m_viewportDirty = false;
	}

	// state is valid
	return true;
}

//---------------------------------------------------------------------------

namespace Helper
{
	static D3D11_COMPARISON_FUNC ConvertCmpFunc( const XenonCmpFunc func )
	{
		switch ( func )
		{
		case XenonCmpFunc::Never: return D3D11_COMPARISON_NEVER;
		case XenonCmpFunc::Less: return D3D11_COMPARISON_LESS;
		case XenonCmpFunc::Equal: return D3D11_COMPARISON_EQUAL;
		case XenonCmpFunc::LessEqual: return D3D11_COMPARISON_LESS_EQUAL;
		case XenonCmpFunc::Greater: return D3D11_COMPARISON_GREATER;
		case XenonCmpFunc::NotEqual: return D3D11_COMPARISON_NOT_EQUAL;
		case XenonCmpFunc::GreaterEqual: return D3D11_COMPARISON_GREATER_EQUAL;
		case XenonCmpFunc::Always: return D3D11_COMPARISON_ALWAYS;
		}

		DEBUG_CHECK( !"Unmapped cmp function value" );
		return D3D11_COMPARISON_ALWAYS;
	}

	static D3D11_STENCIL_OP ConvertStencilOp( const XenonStencilOp op )
	{
		switch ( op )
		{
			case XenonStencilOp::Keep: return D3D11_STENCIL_OP_KEEP;
			case XenonStencilOp::Zero: return D3D11_STENCIL_OP_ZERO;
			case XenonStencilOp::Replace: return D3D11_STENCIL_OP_REPLACE;
			case XenonStencilOp::IncrWrap: return D3D11_STENCIL_OP_INCR;
			case XenonStencilOp::DecrWrap: return D3D11_STENCIL_OP_DECR;
			case XenonStencilOp::Invert: return D3D11_STENCIL_OP_INVERT;
			case XenonStencilOp::Incr: return D3D11_STENCIL_OP_INCR_SAT;
			case XenonStencilOp::Decr: return D3D11_STENCIL_OP_DECR_SAT;
		}

		DEBUG_CHECK( !"Unmapped stencil op value" );
		return D3D11_STENCIL_OP_KEEP;
	}

	static D3D11_BLEND_OP ConvertBlendOp( const XenonBlendOp op )
	{
		switch ( op )
		{
			case XenonBlendOp::Add: return D3D11_BLEND_OP_ADD;
			case XenonBlendOp::Subtract: return D3D11_BLEND_OP_SUBTRACT;
			case XenonBlendOp::Min: return D3D11_BLEND_OP_MIN;
			case XenonBlendOp::Max: return D3D11_BLEND_OP_MAX;
			case XenonBlendOp::ReverseSubtract: return D3D11_BLEND_OP_REV_SUBTRACT;
		}

		DEBUG_CHECK( !"Unmapped blend op value" );
		return D3D11_BLEND_OP_ADD;
	}

	static D3D11_BLEND ConvertBlendArg( const XenonBlendArg arg )
	{
		switch ( arg )
		{
			case XenonBlendArg::Zero: return D3D11_BLEND_ZERO;
			case XenonBlendArg::One: return D3D11_BLEND_ONE;
			case XenonBlendArg::SrcColor: return D3D11_BLEND_SRC_COLOR;
			case XenonBlendArg::OneMinusSrcColor: return D3D11_BLEND_INV_SRC_COLOR;
			case XenonBlendArg::SrcAlpha: return D3D11_BLEND_SRC_ALPHA;
			case XenonBlendArg::OneMinusSrcAlpha: return D3D11_BLEND_INV_SRC_ALPHA;
			case XenonBlendArg::DestColor: return D3D11_BLEND_DEST_COLOR;
			case XenonBlendArg::OneMinusDestColor: return D3D11_BLEND_INV_DEST_COLOR;
			case XenonBlendArg::DestAlpha: return D3D11_BLEND_DEST_ALPHA;
			case XenonBlendArg::OneMinusDestAlpha: return D3D11_BLEND_INV_DEST_ALPHA;
			case XenonBlendArg::ConstColor: return D3D11_BLEND_BLEND_FACTOR;
			case XenonBlendArg::OneMinusConstColor: return D3D11_BLEND_INV_BLEND_FACTOR;
			case XenonBlendArg::ConstAlpha: return D3D11_BLEND_BLEND_FACTOR;
			case XenonBlendArg::OneMinusConstAlpha: return D3D11_BLEND_INV_BLEND_FACTOR;
			case XenonBlendArg::SrcAlphaSaturate: return D3D11_BLEND_SRC_ALPHA_SAT;
		}

		DEBUG_CHECK( !"Unmapped blend op value" );
		return D3D11_BLEND_ZERO;
	}

	static D3D11_CULL_MODE ConvertCullMode( const XenonCullMode mode )
	{
		switch ( mode )
		{
			case XenonCullMode::None: return D3D11_CULL_NONE;
			case XenonCullMode::Back: return D3D11_CULL_BACK;
			case XenonCullMode::Front: return D3D11_CULL_FRONT;
		}

		DEBUG_CHECK( !"Unmapped cull mode " );
		return D3D11_CULL_NONE;
	}

	static D3D11_FILL_MODE ConvertFillMode( const XenonFillMode mode )
	{
		switch ( mode )
		{
			case XenonFillMode::Line: return D3D11_FILL_WIREFRAME;
			case XenonFillMode::Point: return D3D11_FILL_WIREFRAME;
			case XenonFillMode::Solid: return D3D11_FILL_SOLID;
		}

		DEBUG_CHECK( !"Unmapped fill mode " );
		return D3D11_FILL_SOLID;
	}
}

void CDX11AbstractLayer::SetDepthTest( const bool isEnabled )
{
	const BOOL value = isEnabled ? 1 : 0;
	if ( value != m_depthStencilState.DepthEnable )
	{
		m_depthStencilState.DepthEnable = value;
		m_depthStencilStateDirty = true;
	}
}

void CDX11AbstractLayer::SetDepthWrite( const bool isEnabled )
{
	const D3D11_DEPTH_WRITE_MASK value = isEnabled ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
	if ( value != m_depthStencilState.DepthWriteMask )
	{
		m_depthStencilState.DepthWriteMask = value;
		m_depthStencilStateDirty = true;
	}
}

void CDX11AbstractLayer::SetDepthFunc( const XenonCmpFunc func )
{
	const D3D11_COMPARISON_FUNC cmpFunc = Helper::ConvertCmpFunc( func );
	if ( cmpFunc != m_depthStencilState.DepthFunc )
	{
		m_depthStencilState.DepthFunc = cmpFunc;
		m_depthStencilStateDirty = true;
	}
}

void CDX11AbstractLayer::SetStencilTest( const bool isEnabled )
{
	const BOOL value = isEnabled ? 1 : 0;
	if ( value != m_depthStencilState.StencilEnable )
	{
		m_depthStencilState.StencilEnable = value;
		m_depthStencilStateDirty = true;
	}
}

void CDX11AbstractLayer::SetStencilWriteMask( const uint8 mask )
{
	const UINT8 value = mask;
	if ( value != m_depthStencilState.StencilWriteMask )
	{
		m_depthStencilState.StencilWriteMask = value;
		m_depthStencilStateDirty = true;
	}
}

void CDX11AbstractLayer::SetStencilReadMask( const uint8 mask )
{
	const UINT8 value = mask;
	if ( value != m_depthStencilState.StencilReadMask )
	{
		m_depthStencilState.StencilReadMask = value;
		m_depthStencilStateDirty = true;
	}
}

void CDX11AbstractLayer::SetStencilRef( const uint8 mask )
{
	if ( m_depthStencilStateStencilRef != mask )
	{
		m_depthStencilStateStencilRef = mask;
		m_depthStencilStateDirty = true;
	}
}

void CDX11AbstractLayer::SetStencilFunc( const bool front, const XenonCmpFunc func )
{
	const auto value = Helper::ConvertCmpFunc( func );
	auto& data = front ? m_depthStencilState.FrontFace : m_depthStencilState.BackFace;
	if ( value != data.StencilFunc )
	{
		data.StencilFunc = value;
		m_depthStencilStateDirty = true;
	}
}

void CDX11AbstractLayer::SetStencilOps( const bool front, const XenonStencilOp sfail, const XenonStencilOp dfail, const XenonStencilOp dpass )
{
	const auto valueSfail = Helper::ConvertStencilOp( sfail );
	const auto valueDfail = Helper::ConvertStencilOp( dfail );
	const auto valueDpass = Helper::ConvertStencilOp( dpass );
	auto& data = front ? m_depthStencilState.FrontFace : m_depthStencilState.BackFace;
	if ( valueSfail != data.StencilFailOp )
	{
		data.StencilFailOp = valueSfail;
		m_depthStencilStateDirty = true;
	}
	if ( valueDfail != data.StencilDepthFailOp )
	{
		data.StencilDepthFailOp = valueDfail;
		m_depthStencilStateDirty = true;
	}
	if ( valueDpass != data.StencilPassOp )
	{
		data.StencilPassOp = valueDpass;
		m_depthStencilStateDirty = true;
	}
}

bool CDX11AbstractLayer::RealizeDepthStencilState()
{
	// state is up to date
	if ( !m_depthStencilStateDirty )
		return true;

	// set via internal state cache
	if ( !m_stateCache->SetDepthStencil( m_depthStencilState, m_depthStencilStateStencilRef ) )
		return false;

	// valid state set, reset flag
	m_depthStencilStateDirty = false;
	return true;
}

//---------------------------------------------------------------------------

void CDX11AbstractLayer::SetBlend( const uint32 rtIndex, const bool isEnabled )
{
	DEBUG_CHECK( rtIndex < 4 );
	const BOOL value = isEnabled ? 1 : 0;

	auto& data = m_blendState.RenderTarget[rtIndex];
	if ( value != data.BlendEnable )
	{
		data.BlendEnable = value;
		m_blendStateDirty = true;
	}
}

void CDX11AbstractLayer::SetBlendOp( const uint32 rtIndex, const XenonBlendOp colorOp, const XenonBlendOp alphaOp )
{
	DEBUG_CHECK( rtIndex < 4 );

	const auto valueColor = Helper::ConvertBlendOp( colorOp );
	const auto valueAlpha = Helper::ConvertBlendOp( alphaOp );

	auto& data = m_blendState.RenderTarget[rtIndex];
	if ( valueColor != data.BlendOp || valueAlpha != data.BlendOpAlpha )
	{
		data.BlendOp = valueColor;
		data.BlendOpAlpha = valueAlpha;
		m_blendStateDirty = true;
	}
}

void CDX11AbstractLayer::SetBlendArg( const uint32 rtIndex, const XenonBlendArg colorSrc, const XenonBlendArg colorDest, const XenonBlendArg alphaSrc, const XenonBlendArg alphaDest )
{
	DEBUG_CHECK( rtIndex < 4 );

	const auto valueColorSrc = Helper::ConvertBlendArg( colorSrc );
	const auto valueColorDest = Helper::ConvertBlendArg( colorDest );
	const auto valueAlphaSrc = Helper::ConvertBlendArg( alphaSrc );
	const auto valueAlphaDest = Helper::ConvertBlendArg( alphaDest );

	auto& data = m_blendState.RenderTarget[rtIndex];
	if ( (valueColorSrc != data.SrcBlend) || (valueColorDest != data.DestBlend) ||
		(valueAlphaSrc != data.SrcBlendAlpha) || (valueAlphaDest != data.DestBlendAlpha) )
	{
		data.SrcBlend = valueColorSrc;
		data.DestBlend = valueColorDest;

		data.SrcBlendAlpha = valueAlphaSrc;
		data.DestBlendAlpha = valueAlphaDest;
		m_blendStateDirty = true;
	}
}

void CDX11AbstractLayer::SetBlendColor( const float r, const float g, const float b, const float a )
{
	if ( r != m_blendStateColor[0] )
	{
		m_blendStateColor[0] = r;
		m_blendStateDirty = true;
	}

	if ( g != m_blendStateColor[1] )
	{
		m_blendStateColor[1] = g;
		m_blendStateDirty = true;
	}

	if ( b != m_blendStateColor[2] )
	{
		m_blendStateColor[2] = b;
		m_blendStateDirty = true;
	}

	if ( a != m_blendStateColor[3] )
	{
		m_blendStateColor[3] = a;
		m_blendStateDirty = true;
	}
}

bool CDX11AbstractLayer::RealizeBlendState()
{
	// state is up to date
	if ( !m_blendStateDirty )
		return true;

	// set via internal state cache
	if ( !m_stateCache->SetBlendState( m_blendState, m_blendStateColor ) )
		return false;

	// valid state set, reset flag
	m_blendStateDirty = false;
	return true;
}

//---------------------------------------------------------------------------

void CDX11AbstractLayer::SetCullMode( const XenonCullMode cullMode )
{
	const auto value = Helper::ConvertCullMode( cullMode );
	if ( value != m_rasterState.CullMode )
	{
		switch (cullMode)
		{
			case XenonCullMode::None: m_rasterState.CullMode = D3D11_CULL_NONE; break;
			case XenonCullMode::Front: m_rasterState.CullMode = D3D11_CULL_FRONT; break;
			case XenonCullMode::Back: m_rasterState.CullMode = D3D11_CULL_BACK; break;
		}
		m_rasterStateDirty = true;
	}
}

void CDX11AbstractLayer::SetFillMode( const XenonFillMode fillMode )
{
	const auto value = Helper::ConvertFillMode( fillMode );
	if ( value != m_rasterState.FillMode )
	{
		m_rasterState.FillMode = value;
		m_rasterStateDirty = true;
	}
}

void CDX11AbstractLayer::SetFaceMode( const XenonFrontFace faceMode )
{
	const BOOL value = (faceMode == XenonFrontFace::CCW);
	if ( value != m_rasterState.FrontCounterClockwise )
	{
		m_rasterState.FrontCounterClockwise = value;
		m_rasterStateDirty = true;
	}
}

void CDX11AbstractLayer::SetPrimitiveRestart( const bool isEnabled )
{
	if ( m_primitiveRestartEnabled != isEnabled )
	{
		m_primitiveRestartEnabled = isEnabled;
		m_rasterStateDirty = true;
	}
}

void CDX11AbstractLayer::SetPrimitiveRestartIndex( const uint32 index )
{
	if ( m_primitiveRestartIndex != index )
	{
		m_primitiveRestartIndex = index;
		m_rasterStateDirty = true;
	}
}

bool CDX11AbstractLayer::RealizeRasterState()
{
	// state is up to date
	if ( !m_rasterStateDirty )
		return true;

	// if primitive restart is enabled we cannot render this geometry
	if ( m_primitiveRestartEnabled )
	{
		GLog.Err( "D3D: Primitive restart is enabled with index=0x%04X. This is not supported.", m_primitiveRestartIndex );
		return false;
	}

	// set via internal state cache
	if ( !m_stateCache->SetRasterState( m_rasterState ) )
		return false;

	// valid state set, reset flag
	m_rasterStateDirty = false;
	return true;
}

//---------------------------------------------------------------------------

void CDX11AbstractLayer::SetPixelShader( const void* microcode, const uint32 numWords )
{
	m_drawer->SetPixelShaderdCode( microcode, numWords );
}

void CDX11AbstractLayer::SetVertexShader( const void* microcode, const uint32 numWords )
{
	m_drawer->SetVertexShaderdCode( microcode, numWords );
}

void CDX11AbstractLayer::SetPixelShaderConsts( const uint32 firstVector, const uint32 numVectors, const float* values )
{
	float* dest = &m_pixelShaderConsts.Get().m_values[ firstVector*4 ];
	for ( uint32 i=0; i<numVectors; ++i, dest += 4, values += 4 )
	{
		dest[0] = values[0];
		dest[1] = values[1];
		dest[2] = values[2];
		dest[3] = values[3];
	}
}

void CDX11AbstractLayer::SetVertexShaderConsts( const uint32 firstVector, const uint32 numVectors, const float* values )
{
	float* dest = &m_vertexShaderConsts.Get().m_values[ firstVector*4 ];
	for ( uint32 i=0; i<numVectors; ++i, dest += 4, values += 4 )
	{
		dest[0] = values[0];
		dest[1] = values[1];
		dest[2] = values[2];
		dest[3] = values[3];
	}
}

void CDX11AbstractLayer::SetBooleanConstants( const uint32* boolConstants )
{
	m_booleanConsts.Get().m_values[0] = boolConstants[0];
	m_booleanConsts.Get().m_values[1] = boolConstants[1];
	m_booleanConsts.Get().m_values[2] = boolConstants[2];
	m_booleanConsts.Get().m_values[3] = boolConstants[3];
	m_booleanConsts.Get().m_values[4] = boolConstants[4];
	m_booleanConsts.Get().m_values[5] = boolConstants[5];
	m_booleanConsts.Get().m_values[6] = boolConstants[6];
	m_booleanConsts.Get().m_values[7] = boolConstants[7];
}

bool CDX11AbstractLayer::RealizeShaderConstants()
{
	m_mainContext->VSSetConstantBuffers( 0, 1, m_vertexShaderConsts.GetBufferForBinding( m_mainContext ) );
	m_mainContext->VSSetConstantBuffers( 1, 1, m_booleanConsts.GetBufferForBinding( m_mainContext ) );

	m_mainContext->PSSetConstantBuffers( 0, 1, m_pixelShaderConsts.GetBufferForBinding( m_mainContext ) );
	m_mainContext->PSSetConstantBuffers( 1, 1, m_booleanConsts.GetBufferForBinding( m_mainContext ) );

	return true;
}

//---------------------------------------------------------------------------

bool CDX11AbstractLayer::DrawGeometry( const CXenonGPURegisters& regs, IXenonGPUDumpWriter* traceDump, const CXenonGPUState::DrawIndexState& ds )
{
	// clear shader - HACK
	if ( ds.m_primitiveType == XenonPrimitiveType::PrimitiveRectangleList )
	{
		DEBUG_CHECK(ds.m_indexCount == 3);

		// get the vertex stream with clear color
		const uint32 fetchSlot = 0;
		const uint32 fetchRegBase = (uint32) XenonGPURegister::REG_SHADER_CONSTANT_FETCH_00_0 + (fetchSlot*2);
		const auto& fetchEntry = regs.GetStructAt<XenonGPUVertexFetchData>( (XenonGPURegister) fetchRegBase );

		// get the clear color - first vertex
		float clearColor[4];
		const uint32 srcMemoryAddress = GPlatform.GetMemory().TranslatePhysicalAddress( (fetchEntry.address<<2) + 0 );
		clearColor[0] = mem::loadAddr<float>( srcMemoryAddress + 12 + 0 );
		clearColor[1] = mem::loadAddr<float>( srcMemoryAddress + 12 + 4 );
		clearColor[2] = mem::loadAddr<float>( srcMemoryAddress + 12 + 8 );
		clearColor[3] = mem::loadAddr<float>( srcMemoryAddress + 12 + 12 );

		// get the Z to clear
		const auto clearZ = 1.0f;// mem::loadAddr<float>(srcMemoryAddress + 8);

		// compensate for packed color
		const float packMin = -32896.503f;
		const float packRange = 32896.503f;
		clearColor[0] = (clearColor[0] - packMin) / packRange;
		clearColor[1] = (clearColor[1] - packMin) / packRange;
		clearColor[2] = (clearColor[2] - packMin) / packRange;
		clearColor[3] = (clearColor[3] - packMin) / packRange;

		// pass to the edram - clear the EDRAM itself
		m_surfaceManager->ClearColor( 0, clearColor, true );
		m_surfaceManager->ClearDepth(clearZ, 0, true);
		return true;
	}

	// draw
	return m_drawer->Draw( regs, traceDump, ds );
}

//---------------------------------------------------------------------------

uint32 CDX11AbstractLayer::GetActiveTextureFetchSlotMask() const
{
	return m_drawer->GetActiveTextureFetchSlotMask();
}

void CDX11AbstractLayer::SetTexture( const uint32 fetchSlot, const XenonTextureInfo* texture )
{
	CDX11AbstractTexture* runtimeTexture = nullptr;

	// extract runtime texture from the GPU texture binding
	// since the actual DX11 textures are on GPU and our source data is on CPU we need to cache the results
	// also, the format may be very different therefore there may be data conversion between CPU->GPU texture format
	if ( texture && texture->m_address )
	{
		runtimeTexture = m_textureManager->GetTexture( texture );
		if ( !runtimeTexture )
			GLog.Err( "D3D: Failed to build runtime texture" );
	}

	// set the runtime texture on the drawer
	m_drawer->SetTexture( fetchSlot, runtimeTexture );
}

void CDX11AbstractLayer::SetSampler(const uint32 fetchSlot, const XenonSamplerInfo* sampler)
{
	static XenonSamplerInfo defaultSampler;
	m_drawer->SetSampler(fetchSlot, sampler ? *sampler : defaultSampler);
}

//---------------------------------------------------------------------------
