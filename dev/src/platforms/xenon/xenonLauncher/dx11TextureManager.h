#pragma once

#include <Windows.h>
#include <d3d11.h>

#include "xenonGPUConstants.h"
#include "xenonGPUAbstractLayer.h"
#include "xenonGPUTextures.h"

//---------------------------------------------------------------------------

class CDX11StagingTexture;
class CDX11StagingTextureCache;

//---------------------------------------------------------------------------

/// Actual DX11 surface implementation
class CDX11AbstractSurface : public IXenonGPUAbstractSurface
{
public:
	/// IXenonGPUAbstractSurface interface
	virtual XenonTextureFormat GetFormat() const override final;
	virtual uint32 GetWidth() const override final;
	virtual uint32 GetHeight() const override final;
	virtual uint32 GetDepth() const override final;
	virtual uint32 GetRowPitch() const override final;
	virtual uint32 GetSlicePitch() const override final;

	/// Get assigned memory address
	virtual uint32 GetSourceMemoryAddress() const override final;

	// Get the raw view for compute shaders
	inline ID3D11UnorderedAccessView* GetRawViewUint() const { return m_viewUint; }
	inline ID3D11UnorderedAccessView* GetRawViewFloat() const { return m_viewFloat; }

	// Upload texture from CPU data (may recompress)
	void Upload( const void* srcTextureData, void* destData, uint32 destRowPitch, uint32 destSlicePitch ) const;

protected:
	CDX11AbstractSurface();
	virtual ~CDX11AbstractSurface();

	uint32					m_width;
	uint32					m_height;
	uint32					m_depth;

	XenonTextureFormat		m_sourceFormat;
	XenonGPUEndianFormat	m_sourceEndianess;
	uint32					m_sourceWidth;
	uint32					m_sourceHeight;
	uint32					m_sourceBlockWidth;
	uint32					m_sourceBlockHeight;
	uint32					m_sourceBlockSize;
	uint32					m_sourceRowPitch;
	uint32					m_sourceSlicePitch;
	uint32					m_sourceMemoryOffset; // offset vs base (may depend on tiling)
	uint32					m_sourcePackedTileOffsetX;
	uint32					m_sourcePackedTileOffsetY;
	bool					m_sourceIsTiled;
	bool					m_isBlockCompressed;

	DXGI_FORMAT				m_runtimeFormat;
	DXGI_FORMAT				m_viewFormat; // UAV for direct copy, NOTE: may not exist

	ID3D11UnorderedAccessView*	m_viewUint; // for writing into the surface - used for some formats during Resolve
	ID3D11UnorderedAccessView*	m_viewFloat; // for writing into the surface - used for some formats during Resolve

	friend class CDX11AbstractTexture;
};

//---------------------------------------------------------------------------

/// Texture wrapper
class CDX11AbstractTexture : public IXenonGPUAbstractTexture
{
public:
	// IXenonGPUAbstractTexture interface
	virtual uint32 GetBaseAddress() const override final;
	virtual XenonTextureFormat GetFormat() const override final;
	virtual XenonTextureType GetType() const override final;
	virtual uint32 GetBaseWidth() const override final;
	virtual uint32 GetBaseHeight() const override final;
	virtual uint32 GetBaseDepth() const override final;
	virtual uint32 GetNumMipLevels() const override final;
	virtual uint32 GetNumArraySlices() const  override final;
	virtual IXenonGPUAbstractSurface* GetSurface( const uint32 slice, const uint32 mip ) override final;

	// get runtime texture format
	inline DXGI_FORMAT GetRuntimeFormat() const { return m_runtimeFormat; }

	// get runtime texture
	inline ID3D11Resource* GetRuntimeTexture() const { return m_runtimeTexture; }

	// get general resource view
	inline ID3D11ShaderResourceView* GetView() const { return m_view; }

	// Create new texture based on the XenonTextureInfo
	// NOTE: the XenonTextureInfo is usually created from fetch structure
	static CDX11AbstractTexture* Create( ID3D11Device* device, CDX11StagingTextureCache* stagingCache, const struct XenonTextureInfo* textureInfo );
	virtual ~CDX11AbstractTexture(); // NOTE: textures are purged if they are not used for a while

	// Ensure that texture is up to date
	void EnsureUpToDate();

protected:
	CDX11AbstractTexture();

	XenonTextureInfo			m_sourceInfo;
	XenonTextureType			m_sourceType;
	uint32						m_sourceMips;
	uint32						m_sourceArraySlices;

	DXGI_FORMAT					m_runtimeFormat;
	ID3D11Resource*				m_runtimeTexture;
	bool						m_runtimeUAVSupported;

	DXGI_FORMAT					m_viewFormat;
	ID3D11ShaderResourceView*	m_view; // for shaders only

	bool						m_initialDirty;

	std::vector< CDX11AbstractSurface* >	m_surfaces; // per slices, per mip
	std::vector< CDX11StagingTexture* >		m_stagingBuffers; // per mip

	// resource creation
	bool CreateResources( ID3D11Device* device );

	// surface creation
	bool CreateSurfaces( ID3D11Device* device );

	// staging data creation
	bool CreateStagingBuffers( CDX11StagingTextureCache* stagingDataCache );

	// should texture be updated ?
	bool ShouldBeUpdated() const;

	// update texture (forced)
	void Update();

	// mapping
	static bool MapFormat( XenonTextureFormat sourceFormat, DXGI_FORMAT& runtimeFormat, DXGI_FORMAT& viewFormat );
	static bool IsUAVFormat( XenonTextureFormat sourceFormat );
	static bool IsBlockCompressedFormat( XenonTextureFormat sourceFormat );
};

//---------------------------------------------------------------------------

/// Texture manager
class CDX11TextureManager
{
public:
	CDX11TextureManager( ID3D11Device* device, ID3D11DeviceContext* context );
	~CDX11TextureManager();

	/// How to evict textures
	enum class EvictionPolicy
	{
		All,			// evict ALL textures (they will be recreated and reuploaded)
		Unused,			// evict textures not used for few frames
	};

	/// Evict unused textures
	void EvictTextures( const EvictionPolicy policy );

	/// Find texture with given base address (not created if missing)
	CDX11AbstractTexture* FindTexture( const uint32 baseAddress );

	/// Get/create texture matching given settings (RT only), one mip, no slices
	/// NOTE: this is ONLY for render targets/other textures NOT coming from samplers
	CDX11AbstractTexture* GetTexture( const uint32 baseAddress, const uint32 width, const uint32 height, const XenonTextureFormat format );

	/// Get/create texture matching given runtime texture setup
	/// NOTE: texture will not be re-uploaded unless the memory region is marked as changed
	CDX11AbstractTexture* GetTexture( const XenonTextureInfo* textureInfo );

private:
	typedef std::vector< CDX11AbstractTexture* >		TTextures;

	// internal lock
	typedef std::mutex					TLock;
	typedef std::lock_guard<TLock>		TScopeLock;
	TLock			m_lock;
	
	// all created (and managed) textures
	TTextures				m_textures;

	// device
	ID3D11Device*			m_device;
	ID3D11DeviceContext*	m_context;

	// texture staging buffer
	CDX11StagingTextureCache*	m_stagingCache;

	// internal timing (used to evict textures)
	uint32					m_frameIndex;
};

//---------------------------------------------------------------------------
