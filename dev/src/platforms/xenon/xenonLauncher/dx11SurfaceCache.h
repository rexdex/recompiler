#pragma once

#include <Windows.h>
#include <d3d11.h>

#include "xenonGPUConstants.h"
#include "xenonGPUAbstractLayer.h"

/// NOTE: All render targets are in "EDRAM"

/// DX11 render target
class CDX11AbstractRenderTarget : public IXenonGPUAbstractRenderTarget
{
public:
	virtual ~CDX11AbstractRenderTarget();

	/// IXenonGPUAbstractRenderTarget interface
	virtual XenonColorRenderTargetFormat GetFormat() const override final;
	virtual XenonMsaaSamples GetMSAA() const override final;
	virtual uint32 GetMemoryPitch() const override final;
	virtual int32 GetEDRAMPlacement() const override final;
	virtual void Clear( const float* clearColor ) override final;

	// Get physical texture width
	inline const uint32 GetPhysicalWidth() const { return m_physicalWidth; }
	inline const uint32 GetPhysicalHeight() const { return m_physicalHeight; }

	/// Bind runtime context that will be using this surface (needed for clear and resolve)
	void BindRuntimeContext( ID3D11DeviceContext* context );

	/// EDRAM placement
	void SetEDRAMPlacement( const int32 placement );

	/// translate format
	static DXGI_FORMAT TranslateFormatUnorm( XenonColorRenderTargetFormat format );
	static DXGI_FORMAT TranslateFormatUint( XenonColorRenderTargetFormat format );
	static DXGI_FORMAT TranslateTyplessFormat( XenonColorRenderTargetFormat format );
	static uint32 GetFormatPixelSize( XenonColorRenderTargetFormat format );
	static uint32 GetFormatPixelSizeWithMSAA( XenonColorRenderTargetFormat format, XenonMsaaSamples msaa );
	static uint32 GetMSSASampleCount( XenonMsaaSamples msaa );

	/// Create RT surface
	static CDX11AbstractRenderTarget* Create( ID3D11Device* device, XenonColorRenderTargetFormat format, XenonMsaaSamples msaa, const uint32 pitch );

	/// Get render target view
	inline ID3D11RenderTargetView* GetBindableView() const { return m_rtv; }

	// Get the raw view for compute shaders
	inline ID3D11UnorderedAccessView* GetRawView() const { return m_uav; }

	// Get the read only view
	inline ID3D11ShaderResourceView* GetROViewUint() const { return m_srvUint; }
	inline ID3D11ShaderResourceView* GetROViewUnorm() const { return m_srvUnorm; }

	/// Get texture data
	inline ID3D11Texture2D* GetTexture() const { return m_texture; }

private:
	CDX11AbstractRenderTarget();

	ID3D11Texture2D*				m_texture;
	ID3D11ShaderResourceView*		m_srvUint; // NOT USED DIRECTLY - used for copying the RT into proper texture (Resolve)
	ID3D11ShaderResourceView*		m_srvUnorm; // NOT USED DIRECTLY - used for copying the RT into proper texture (Resolve)
	ID3D11RenderTargetView*			m_rtv;
	ID3D11UnorderedAccessView*		m_uav; // TYPELESS!

	XenonColorRenderTargetFormat	m_sourceFormat; // source format
	XenonMsaaSamples				m_sourceMSAA;
	uint32							m_sourcePitch;

	DXGI_FORMAT						m_actualFormat;

	int32							m_edramPlacement;

	uint32							m_physicalWidth;
	uint32							m_physicalHeight;

	ID3D11DeviceContext*			m_runtimeContext;
};

/// DX11 depth stencil
class CDX11AbstractDepthStencil : public IXenonGPUAbstractDepthStencil
{
public:
	virtual ~CDX11AbstractDepthStencil();

	/// IXenonGPUAbstractDepthStencilinterface
	virtual XenonDepthRenderTargetFormat GetFormat() const override final;
	virtual XenonMsaaSamples GetMSAA() const override final;
	virtual uint32 GetMemoryPitch() const override final;
	virtual int32 GetEDRAMPlacement() const override final;
	virtual void Clear( const bool clearDepth, const bool clearStencil, const float depthValue, const uint32 stencilValue ) override final;

	// Get physical texture width
	inline const uint32 GetPhysicalWidth() const { return m_physicalWidth; }
	inline const uint32 GetPhysicalHeight() const { return m_physicalWidth; }

	/// Bind runtime context that will be using this surface (needed for clear and resolve)
	void BindRuntimeContext( ID3D11DeviceContext* context );

	/// EDRAM placement
	void SetEDRAMPlacement( const int32 placement );

	/// translate format
	static DXGI_FORMAT TranslateFormat( XenonDepthRenderTargetFormat format );
	static uint32 GetFormatPixelSize( XenonDepthRenderTargetFormat format );
	static uint32 GetFormatPixelSizeWithMSAA( XenonDepthRenderTargetFormat format, XenonMsaaSamples msaa );
	static uint32 GetMSSASampleCount( XenonMsaaSamples msaa );

	/// Create RT surface
	static CDX11AbstractDepthStencil* Create( ID3D11Device* device, XenonDepthRenderTargetFormat format, XenonMsaaSamples msaa, const uint32 pitch );

	/// Get depth stencil view
	inline ID3D11DepthStencilView* GetBindableView() const { return m_dsv; }

private:
	CDX11AbstractDepthStencil();
	
	ID3D11Texture2D*				m_texture;
	ID3D11ShaderResourceView*		m_srv; // NOT USED DIRECTLY - used for copying the RT into proper texture (Resolve)
	ID3D11DepthStencilView*			m_dsv;

	XenonDepthRenderTargetFormat	m_sourceFormat; // source format
	XenonMsaaSamples				m_sourceMSAA;
	uint32							m_sourcePitch;

	DXGI_FORMAT						m_actualFormat;

	int32							m_edramPlacement;

	uint32							m_physicalWidth;
	uint32							m_physicalHeight;

	ID3D11DeviceContext*			m_runtimeContext;
};

/// Cache for surfaces - used to allocate resources
class CDX11SurfaceCache
{
public:
	CDX11SurfaceCache( ID3D11Device* dev );
	~CDX11SurfaceCache();

	/// Get usable render target matching specified parameters
	CDX11AbstractRenderTarget* GetRenderTarget( XenonColorRenderTargetFormat format, XenonMsaaSamples msaa, const uint32 pitch, const uint32 placement );

	/// Get usable depth stencil surface matching specified parameters
	CDX11AbstractDepthStencil* GetDepthStencil( XenonDepthRenderTargetFormat format, XenonMsaaSamples msaa, const uint32 pitch, const uint32 placement );

	/// Remove all RT & DS from EDRAM
	void FreeEDRAM();

	/// Purge all unused resources
	void PurgeUnusedResources();

private:
	ID3D11Device*			m_device;

	// all RT
	std::vector< CDX11AbstractRenderTarget* >		m_renderTargets;
	std::vector< CDX11AbstractDepthStencil* >		m_depthStencil;

	// Find matching RT that is not assigned into EDRAM, returns NULL if nothing free found
	CDX11AbstractRenderTarget* FindMatchingRT( XenonColorRenderTargetFormat format, XenonMsaaSamples msaa, const uint32 pitch );

	// Find matching DS that is not assigned into EDRAM, returns NULL if nothing free found
	CDX11AbstractDepthStencil* FindMatchingDS( XenonDepthRenderTargetFormat format, XenonMsaaSamples msaa, const uint32 pitch );
};