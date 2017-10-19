#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>

#include "xenonGpuAbstractLayer.h"
#include "xenonGPUConstants.h"
#include "dx11Utils.h"

class CDX11RenderingWindow;
class CDX11SurfaceManager;
class CDX11GeometryDrawer;
class CDX11TextureManager;
class CDX11StateCache;
class CXenonGPURegisters;

class CDX11AbstractLayer : public IXenonGPUAbstractLayer
{
public:
	CDX11AbstractLayer();
	~CDX11AbstractLayer();

	// interface
	virtual bool Initialize() override final;
	virtual bool SetDisplayMode( const uint32 width, const uint32 height ) override final;
	virtual void BeingFrame() override final;
	virtual void Swap( const CXenonGPUState::SwapState& ss ) override final;

	// debugging
	virtual void BeginEvent(const char* name) override final;
	virtual void EndEvent() override final;

	// RT&DS interface
	virtual void BindColorRenderTarget( const uint32 index, const XenonColorRenderTargetFormat format, const XenonMsaaSamples msaa, const uint32 base, const uint32 pitch ) override final;
	virtual void BindDepthStencil( const XenonDepthRenderTargetFormat format, const XenonMsaaSamples msaa, const uint32 base, const uint32 pitch ) override final;	
	virtual void UnbindColorRenderTarget( const uint32 index ) override final;
	virtual void UnbindDepthStencil() override final;
	virtual void SetColorRenderTargetWriteMask( const uint32 index, const bool enableRed, const bool enableGreen, const bool enableBlue, const bool enableAlpha ) override final;
	virtual void ClearColorRenderTarget( const uint32 index, const float* clearColor, const bool flushToEDRAM ) override final;
	virtual void ClearDepthStencilRenderTarget( const float depthClear, const uint32 stencilClear, const bool flushToEDRAM ) override final;
	virtual bool ResolveColorRenderTarget( const uint32 srcIndex, const XenonColorRenderTargetFormat srcFormat, const uint32 srcBase, const XenonRect2D& srcRect, const uint32 destBase, const uint32 destLogicalWidth, const uint32 destLogicalHeight, const uint32 destBlockWidth, const uint32 destBlockHeight, const XenonTextureFormat destFormat, const XenonRect2D& destRect ) override final;
	virtual bool ResolveDepthRenderTarget( const XenonDepthRenderTargetFormat srcFormat, const uint32 srcBase, const XenonRect2D& srcRect, const uint32 destBase, const uint32 destLogicalWidth, const uint32 destLogicalHeight, const uint32 destBlockWidth, const uint32 destBlockHeight, const XenonTextureFormat destFormat, const XenonRect2D& destRect ) override final;
	virtual bool RealizeSurfaceSetup( uint32& outMainWidth,  uint32& outMainHeight ) override final;

	// Viewport interface
	virtual void SetViewportVertexFormat( const bool xyDivied, const bool zDivied, const bool wNotInverted ) override final;
	virtual void SetViewportWindowScale( const bool bNormalizedXYCoordinates ) override final;
	virtual void EnableScisor( const uint32 x, const uint32 y, const uint32 w, const uint32 h ) override final;
	virtual void DisableScisor() override final;
	virtual void SetViewportRange( const float x, const float y, const float w, const float h ) override final;
	virtual void SetDepthRange( const float offset, const float scale ) override final;
	virtual bool RealizeViewportSetup() override final;

	// Depth/stencil state
	virtual void SetDepthTest( const bool isEnabled ) override final;
	virtual void SetDepthWrite( const bool isEnabled ) override final;
	virtual void SetDepthFunc( const XenonCmpFunc func ) override final;
	virtual void SetStencilTest( const bool isEnabled ) override final;
	virtual void SetStencilWriteMask( const uint8 mask ) override final;
	virtual void SetStencilReadMask( const uint8 mask ) override final;
	virtual void SetStencilRef( const uint8 mask ) override final;
	virtual void SetStencilFunc( const bool front, const XenonCmpFunc func ) override final;
	virtual void SetStencilOps( const bool front, const XenonStencilOp sfail, const XenonStencilOp dfail, const XenonStencilOp dpass ) override final;
	virtual bool RealizeDepthStencilState() override final;

	// Blending
	virtual void SetBlend( const uint32 rtIndex, const bool isEnabled ) override final;
	virtual void SetBlendOp( const uint32 rtIndex, const XenonBlendOp colorOp, const XenonBlendOp alphaOp ) override final;
	virtual void SetBlendArg( const uint32 rtIndex, const XenonBlendArg colorSrc, const XenonBlendArg colorDest, const XenonBlendArg alphaSrc, const XenonBlendArg alphaDest ) override final;
	virtual void SetBlendColor( const float r, const float g, const float b, const float a ) override final;
	virtual bool RealizeBlendState() override final;

	// Raster
	virtual void SetCullMode( const XenonCullMode cullMode ) override final;
	virtual void SetFillMode( const XenonFillMode fillMode ) override final;
	virtual void SetFaceMode( const XenonFrontFace faceMode ) override final;
	virtual void SetPrimitiveRestart( const bool isEnabled ) override final;
	virtual void SetPrimitiveRestartIndex( const uint32 index ) override final;
	virtual bool RealizeRasterState() override final;
	
	// Shader interface
	virtual void SetPixelShader( const void* microcode, const uint32 numWords ) override final;
	virtual void SetVertexShader( const void* microcode, const uint32 numWords ) override final;
	virtual void SetPixelShaderConsts( const uint32 firstVector, const uint32 numVectors, const float* values ) override final;
	virtual void SetVertexShaderConsts( const uint32 firstVector, const uint32 numVectors, const float* values ) override final;
	virtual void SetBooleanConstants( const uint32* boolConstants ) override final;
	virtual bool RealizeShaderConstants() override final;

	// Geometry interface
	virtual bool DrawGeometry( const class CXenonGPURegisters& regs, IXenonGPUDumpWriter* traceDump, const CXenonGPUState::DrawIndexState& ds ) override final;

	// Textures
	virtual uint32 GetActiveTextureFetchSlotMask() const override final;
	virtual void SetTexture( const uint32 fetchSlot, const XenonTextureInfo* texture ) override final;
	virtual void SetSampler(const uint32 fetchSlot, const XenonSamplerInfo* sampler) override final;

private:
	// DX11 device, always valid
	ID3D11Device*				m_device;
	ID3D11DeviceContext*		m_mainContext;

	// debug interface
	ID3DUserDefinedAnnotation*	m_eventDevice;

	// rendering swapchain, valid if the mode is valid
	IDXGISwapChain*				m_swapChain;
	ID3D11Texture2D*			m_backBuffer;	
	ID3D11RenderTargetView*		m_backBufferView;

	// rendering window, always valid
	CDX11RenderingWindow*		m_window;	

	// EDRAM manager (aka. rendering surface manager)
	CDX11SurfaceManager*		m_surfaceManager;

	// Texture manager (aka. copy pasting pseudo physical memory into DX11 textures)
	CDX11TextureManager*		m_textureManager;

	// geometry drawer
	CDX11GeometryDrawer*		m_drawer;

	// internal state cache
	CDX11StateCache*			m_stateCache;

	struct ModeInfo
	{
		uint32					m_width;
		uint32					m_height;
		bool					m_fullscreen;

		ModeInfo()
			: m_width( 1280 )
			, m_height( 720 )
			, m_fullscreen( false )
		{}
	};

	// current rendering mode
	ModeInfo					m_mode;

	// current viewport settings
	D3D11_VIEWPORT				m_viewport;
	bool						m_viewportDirty;

	// current depth/stencil state
	D3D11_DEPTH_STENCIL_DESC	m_depthStencilState;
	uint32						m_depthStencilStateStencilRef;
	bool						m_depthStencilStateDirty;

	// current blend state
	D3D11_BLEND_DESC			m_blendState;
	float						m_blendStateColor[4];
	bool						m_blendStateDirty;

	// current rasterizer state
	D3D11_RASTERIZER_DESC		m_rasterState;
	bool						m_rasterStateDirty;

	// primitive restart state
	bool						m_primitiveRestartEnabled;
	uint32						m_primitiveRestartIndex;

	struct ShaderFloatConsts
	{
		float  m_values[256*4];

		inline ShaderFloatConsts()
		{
			memset( m_values, 0, sizeof(m_values) );
		}
	};

	struct ShaderBoolConsts
	{
		uint32 m_values[8];

		inline ShaderBoolConsts()
		{
			memset( m_values, 0, sizeof(m_values) );
		}
	};

	// shader constants
	TDX11ConstantBuffer< ShaderFloatConsts >	m_pixelShaderConsts;
	TDX11ConstantBuffer< ShaderFloatConsts >	m_vertexShaderConsts;
	TDX11ConstantBuffer< ShaderBoolConsts >		m_booleanConsts;

	// can we render this frame ?
	volatile bool				m_canRender;

	// create rendering deivce
	bool CreateDevice();

	// discard current swapchain
	void DiscardSwapchain();

	// create swapchain 
	bool CreateSwapchain();
};