#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>

#include "xenonGPUConstants.h"
#include "xenonGPUUtils.h"
#include "dx11Utils.h"

class CDX11SurfaceCache;
class CDX11SurfaceMemory;

class CDX11AbstractTexture;
class CDX11AbstractRenderTarget;
class CDX11AbstractDepthStencil;

/// Manager for EDRAM surfaces
class CDX11SurfaceManager
{
public:
	CDX11SurfaceManager( ID3D11Device* dev, ID3D11DeviceContext* context );
	~CDX11SurfaceManager();

	// Reset (clear memory, etc)
	void Reset();

	// RT&DS interface
	void BindColorRenderTarget( const uint32 index, const XenonColorRenderTargetFormat format, const XenonMsaaSamples msaa, const uint32 base, const uint32 pitch );
	void BindDepthStencil( const XenonDepthRenderTargetFormat format, const XenonMsaaSamples msaa, const uint32 base, const uint32 pitch );	
	void UnbindColorRenderTarget( const uint32 index );
	void UnbindDepthStencil();

	/// Realize setup - this will actually allocate the resource and prepare for rendering
	bool RealizeSetup( uint32& outMainWidth,  uint32& outMainHeight );

	/// Clear
	void ClearColor( const uint32 index, const float* clearColor, const bool flushToEDRAM );
	void ClearDepth( const float depthClear, const uint32 stencilClear, const bool flushToEDRAM );

	/// Copy out data from EDRAM
	bool ResolveColor(const uint32 srcIndex, const XenonColorRenderTargetFormat srcFormat, const uint32 srcBase, const XenonRect2D& srcRect, CDX11AbstractTexture* destTexture, const XenonRect2D& destRect);

	/// Copy out the depth data from EDRAM
	bool ResolveDepth(const XenonDepthRenderTargetFormat srcFormat, const uint32 srcBase, const XenonRect2D& srcRect, CDX11AbstractTexture* destTexture, const XenonRect2D& destRect);

	/// Swap
	void ResolveSwap( ID3D11Texture2D* frontBuffer );

private:
	// context & device
	ID3D11DeviceContext*		m_context;
	ID3D11Device*				m_device;

	/// EDRAM render target surface
	struct EDRAMRenderTarget
	{
		XenonColorRenderTargetFormat		m_format;
		XenonMsaaSamples					m_msaa;
		uint32								m_base;
		uint32								m_pitch;

		// check if we can prevent flushing of this surface
		bool IsMatching( CDX11AbstractRenderTarget* rt ) const;

		inline bool IsValid() const
		{
			return (m_pitch != 0);
		}
	};

	/// EDRAM depth stencil surface
	struct EDRAMDepthStencil
	{
		XenonDepthRenderTargetFormat		m_format;
		XenonMsaaSamples					m_msaa;
		uint32								m_base;
		uint32								m_pitch;

		// check if we can prevent flushing of this surface
		bool IsMatching( CDX11AbstractDepthStencil* ds ) const;

		inline bool IsValid() const
		{
			return (m_pitch != 0);
		}
	};

	/// EDRAM setup
	struct EDRAMSetup
	{
		EDRAMRenderTarget		m_color[4];
		EDRAMDepthStencil		m_depth;

		EDRAMSetup();
		void Dump();
	};

	/// EDRAM runtime setup
	struct EDRAMRuntimeSetup
	{
		EDRAMSetup					m_desc;
		CDX11AbstractRenderTarget*	m_surfaceColor[4];
		CDX11AbstractDepthStencil*	m_surfaceDepth;

		EDRAMRuntimeSetup();
	};

	/// Surface copy params
#pragma pack(push,4)
	struct Settings
	{
		int32 m_copySrcOffsetX;
		int32 m_copySrcOffsetY;
		int32 m_copyDestOffsetX;
		int32 m_copyDestOffsetY;
		int32 m_copyDestSizeX;
		int32 m_copyDestSizeY;

		Settings()
			: m_copySrcOffsetX(0)
			, m_copySrcOffsetY(0)
			, m_copyDestOffsetX(0)
			, m_copyDestOffsetY(0)
			, m_copyDestSizeX(0)
			, m_copyDestSizeY(0)
		{};
	};
#pragma pack(pop)

	static_assert(sizeof(Settings) == 6 * 4, "Mismatched CPU <-> GPU size");

	// constant buffer
	TDX11ConstantBuffer< Settings >	m_settings;

	/// Surface cache
	CDX11SurfaceCache*		m_cache;

	/// EDRAM memory emulator
	CDX11SurfaceMemory*		m_memory;

	/// Pending setup
	EDRAMSetup				m_incomingSetup;

	/// Last realized EDRAM setup
	EDRAMRuntimeSetup		m_runtimeSetup;

	/// Copy shaders
	CDX11ComputeShader		m_copyTextureCSFloat;
	CDX11ComputeShader		m_copyTextureCSUnorm;
	CDX11ComputeShader		m_copyTextureCSUint;
};
