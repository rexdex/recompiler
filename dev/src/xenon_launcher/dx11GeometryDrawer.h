#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>

#include "xenonGpuAbstractLayer.h"
#include "xenonGPUConstants.h"
#include "xenonGPUState.h"
#include "dx11Utils.h"

class CDX11BufferRing;
class CDX11MicrocodeCache;
class CDX11MicrocodeShader;
class CDX11ShaderCache;
class CDX11GeometryBuffer;
class CDX11SamplerCache;
class CDX11AbstractTexture;
class CXenonGPURegisters;

/// Geometry drawer, handles input assembly and shader dispatching
class CDX11GeometryDrawer
{
public:
	CDX11GeometryDrawer( ID3D11Device* dev, ID3D11DeviceContext* context );
	~CDX11GeometryDrawer();

	// Reset geometry cache
	void Reset();

	// Setup shader dumping directory
	void SetShaderDumpDirector( const std::wstring& dumpDir );

	// Setup outgoing format of vertices from vertex shader
	// NOTE: in real hardware we can only output POSH vertices so we have to convert whatever is outputed from the simulated shader, this tells us how
	void SetViewportVertexFormat( const bool xyDivied, const bool zDivied, const bool wNotInverted );
	void SetViewportWindowScale( const bool bNormalizedXYCoordinates );

	// Setup physical surface size
	void SetPhysicalSize( const uint32 width, const uint32 height );

	// Set pixel shader micro code
	void SetPixelShaderdCode( const void* data, const uint32 numWords );

	// Set vertex shader micro code
	void SetVertexShaderdCode( const void* data, const uint32 numWords );

	// Get active fetch slots (based on the active pixel/vertex shader combo)
	const uint32 GetActiveTextureFetchSlotMask() const;

	// Set the runtime texture (actual D3D resource) to a given fetch slot
	void SetTexture( const uint32 fetchSlot, class CDX11AbstractTexture* runtimeTexture );

	// Set the sampler to given fetch slot
	void SetSampler(const uint32 fetchSlot, const XenonSamplerInfo& samplerInfo);

	//--

	// Draw prepared geometry
	bool Draw( const class CXenonGPURegisters& regs, class IXenonGPUDumpWriter* traceDump, const struct CXenonGPUState::DrawIndexState& ds );

	//--

	// Draw clear quad
	bool DrawClearQuad(const float minX, const float minY, const float maxX, const float maxY, const float depth, const float r, const float g, const float b, const float a);

private:
	// DX11 device, always valid
	ID3D11Device*				m_device;
	ID3D11DeviceContext*		m_mainContext;

	// Captured shader data
	struct ShaderData
	{
		// type of shader
		XenonShaderType			m_type;

		// cached microcode
		bool					m_changed;
		CDX11MicrocodeShader*	m_microcode;

		ShaderData();

		// set new data, returns false if data is up to date
		bool SetData( CDX11MicrocodeCache* cache, const void* data, const uint32 numWords );
	};

	// Vertex viewport state
#pragma pack(push,4)
	struct VertexViewportState
	{
		uint32			m_xyDivided;
		uint32			m_zDivided;
		uint32			m_wNotInverted;
		uint32			padding01;

		uint32			m_normalizedCoordinates;
		uint32			padding02;
		uint32			padding03;
		uint32			padding04;

		uint32			m_indexMode; // 0-non indexed, 1-indexed
		uint32			m_baseVertex; // added to each vertex index BEFORE fetching from the vertex buffer
		uint32			padding05;
		uint32			padding06;

		float			m_physicalWidth;
		float			m_physicalHeight;
		float			m_physicalInvWidth;
		float			m_physicalInvHeight;
	};
#pragma pack(pop)

	// shader constants for the viewport/vertex shader post-processing
	TDX11ConstantBuffer< VertexViewportState >	m_vertexViewportState;

	// default resources
	ID3D11Texture1D*			m_defaultTexture1D;
	ID3D11ShaderResourceView*	m_defaultTexture1DView;
	ID3D11Texture2D*			m_defaultTexture2D;
	ID3D11ShaderResourceView*	m_defaultTexture2DView;
	ID3D11Texture2D*			m_defaultTexture2DArray;
	ID3D11ShaderResourceView*	m_defaultTexture2DArrayView;
	ID3D11Texture3D*			m_defaultTexture3D;
	ID3D11ShaderResourceView*	m_defaultTexture3DView;
	ID3D11Texture2D*			m_defaultTextureCube;
	ID3D11ShaderResourceView*	m_defaultTextureCubeView;

	// microcode copy
	ShaderData			m_pixelShader;
	ShaderData			m_vertexShader;
	bool				m_shaderDirty;

	// geometry buffer & cache
	CDX11GeometryBuffer*	m_geometryBuffer;

	// cache for decompiled microcode shaders
	CDX11MicrocodeCache*	m_microcodeCache;

	// cache for renderable shaders
	CDX11ShaderCache*		m_shaderCache;

	// cache for samplers
	CDX11SamplerCache*		m_samplerCache;

	// shader active texture fetch slots
	uint32					m_shaderTextureFetchSlots;

	// active runtime textures for given fetch slots
	static const uint32 MAX_TEXTURE_FETCH_SLOTS = 32;
	CDX11AbstractTexture*	m_textures[ MAX_TEXTURE_FETCH_SLOTS ];

	// samplers
	ID3D11SamplerState*		m_samplers[MAX_TEXTURE_FETCH_SLOTS];

	// shader dump directory
	std::wstring	m_shaderDumpDirectory;

	// specialized geometry shaders for geometry generation
	CDX11GeometryShader m_geometryShaderRectList;

	// prepare pipeline stages
	bool RealizeShaders();	
	bool RealizeVertexBuffers( const class CXenonGPURegisters& regs, class IXenonGPUDumpWriter* traceDump, const class CDX11FetchLayout* layout );
	bool RealizeIndexBuffer( struct CXenonGPUState::DrawIndexState& ds );

	// manage default texture resources
	void CreateDefaultTextures();
	void ReleaseDefaultTextres();
	void CreateDefaultSamplers();
	
	// create the specialized geometry shaders
	void CreateGeometryShaders();

	// texture fetch slot mask update (from both shaders)
	void RefreshTextureFetchSlotMask();

	// state translation functions
	static bool TranslatePrimitiveType( const XenonPrimitiveType primitiveType, D3D11_PRIMITIVE& outPrimitive, D3D11_PRIMITIVE_TOPOLOGY& outTopology );
};
