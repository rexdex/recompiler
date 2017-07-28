#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>

#include "xenonGPUConstants.h"
#include "dx11Utils.h"

/// EDRAM emulator (10MB or 2048 of 80x16x4 (5120 bytes) tiles)
/// The EDRAM operation is simulated by making possible to copy surface into/from EDRAM memory 
/// thus simulating aliasing and format change as it was happening on Xenon
class CDX11SurfaceMemory
{
public:
	CDX11SurfaceMemory( ID3D11Device* dev, ID3D11DeviceContext* mainContext );
	~CDX11SurfaceMemory();

	// reset EDRAM content (clear to random noise in debug, zeros in release)
	void Reset();

	// copy surface content into EDRAM
	// EDRAM placement is taken from the surface
	void CopyIntoEDRAM( class CDX11AbstractRenderTarget* rt );
	void CopyIntoEDRAM( class CDX11AbstractDepthStencil* ds );

	// copy surface content from EDRAM
	// EDRAM placement is taken from the surface
	void CopyFromEDRAM( class CDX11AbstractRenderTarget* rt );
	void CopyFromEDRAM( class CDX11AbstractDepthStencil* ds );

	// resolve from EDRAM into a surface of a texture
	// EDRAM placement is taken from the surface
	// Target placement in target surface is given directly
	// Format conversion is hardcoded
	void Resolve( const XenonColorRenderTargetFormat srcFormat, const uint32 srcBase, const uint32 srcPitch, const uint32 srcX, const uint32 srcY, const uint32 destX, const uint32 destY, const uint32 width, const uint32 height, class CDX11AbstractSurface* destSurf );

	// clear EDRAM range with given color
	void ClearInEDRAM( const XenonColorRenderTargetFormat format, const uint32 edramBase, const uint32 edramPitch, const uint32 height, const float* clearColor );

private:
	static const uint32 NUM_EDRAM_TILES = 2048;

	static const uint32 EDRAM_TILE_X = 80; // no MSAA mode
	static const uint32 EDRAM_TILE_Y = 16; // no MSAA mode
	static const uint32 EDRAM_BPP = 4; // no MSAA mode

	// generalized 32-bit format info
	static const uint32 GENERIC_FORMAT_8_8_8_8 = 1;
	static const uint32 GENERIC_FORMAT_8_8_8_8_GAMMA = 2;
	static const uint32 GENERIC_FORMAT_10_10_10_2 = 3;
	static const uint32 GENERIC_FORMAT_10_10_10_2_GAMMA = 4;
	static const uint32 GENERIC_FORMAT_16_16 = 5;
	static const uint32 GENERIC_FORMAT_16_16_FLOAT = 6;
	
	// DX11 device, always valid
	ID3D11Device*				m_device;
	ID3D11DeviceContext*		m_mainContext;

	// simulated 10MB of EDRAM
	ID3D11Buffer*				m_edramMemory;
	ID3D11UnorderedAccessView*	m_edramView;	

	// CPU access proxy
	ID3D11Buffer*				m_edramCopy;

	// internal EDRAM shader
	CDX11ComputeShader			m_fillRandom;

	// EDRAM upload/download for 32-bit formats
	CDX11ComputeShader			m_download32;
	CDX11ComputeShader			m_upload32;
	CDX11ComputeShader			m_clear32;

	// general settings
	#pragma pack(push,4)
	struct Settings
	{
		// flat 
		uint32		m_flatOffset;
		uint32		m_flatSize;

		// format
		uint32		m_edramFormat;			// generic shader format enum
		uint32		m_copySurfaceFormat;	// generic shader format enum

		// pixel dispatch
		uint32		m_pixelDispatchWidth;
		uint32		m_pixelDispatchHeight;
		uint32		m_pixelDispatcBlocksX;
		uint32		m_pixelDispatcBlocksY;

		// EDRAM surface binding
		uint32		m_edramBaseAddr;
		uint32		m_edramPitch;
		uint32		m_edramOffsetX; // in pixels, offsets BEFORE EDRAM memory offset calculation
		uint32		m_edramOffsetY; // in pixels, offsets BEFORE EDRAM memory offset calculation

		// copy settings
		uint32		m_copySurfaceTargetX; // in pixels
		uint32		m_copySurfaceTargetY; // in pixels
		uint32		m_copySurfaceTotalWidth; // in pixels
		uint32		m_copySurfaceTotalHeight; // in pixels
		uint32		m_copySurfaceWidth; // width of the copy region
		uint32		m_copySurfaceHeight; // height of the copy region
		uint32		m_padding0;
		uint32		m_padding1;

		// clear settings
		float		m_clearColorR;
		float		m_clearColorG;
		float		m_clearColorB;
		float		m_clearColorA;
	};
	#pragma pack(pop)

	static_assert( sizeof(Settings) == 24*4, "Mismatched CPU <-> GPU size" );

	// constant buffer
	TDX11ConstantBuffer< Settings >		m_settings;

	// load shaders
	bool LoadShaders();

	// dispatch shader with flat addressing, NOTE: memory addresses are within EDRAM
	void DispatchFlat( const CDX11ComputeShader& shader, const uint32 memoryOffset, const uint32 memorySize );

	// dispatch shader for given rect of pixels
	void DispatchPixels( const CDX11ComputeShader& shader, const uint32 width, const uint32 height, ID3D11UnorderedAccessView* additionalUAV );

	// dispatch shader for given rect of pixels with a read only input
	void DispatchPixels( const CDX11ComputeShader& shader, const uint32 width, const uint32 height, ID3D11ShaderResourceView* shaderView );

	// EDRAM format translation for upload/download shaders
	static const uint32 TranslateToGenericFormat_EDRAM( const XenonColorRenderTargetFormat format );
	static const uint32 TranslateToGenericFormat_EDRAM( const XenonDepthRenderTargetFormat format );

	// Surface format translation for upload/download shaders
	static const uint32 TranslateToGenericFormat_Surface( const XenonColorRenderTargetFormat format );
	static const uint32 TranslateToGenericFormat_Surface( const XenonDepthRenderTargetFormat format );
	static const uint32 TranslateToGenericFormat_Surface( const XenonTextureFormat format );
};