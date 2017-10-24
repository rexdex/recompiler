#include "build.h"
#include "dx11SurfaceMemory.h"
#include "dx11SurfaceCache.h"
#include "xenonGPUUtils.h"
#include "dx11TextureManager.h"

//#pragma optimize("",off)

//-----------------------------------

CDX11SurfaceMemory::CDX11SurfaceMemory(ID3D11Device* dev, ID3D11DeviceContext* mainContext)
	: m_device(dev)
	, m_mainContext(mainContext)
	, m_edramMemory(nullptr)
	, m_edramView(nullptr)
	, m_fillRandom("FillRandom")
	, m_download32("Download32")
	, m_upload32("Upload32")
	, m_clear32("Clear32")
{
	// calculate size of edram memory
	const uint32 edramMemorySize = NUM_EDRAM_TILES * EDRAM_TILE_X * EDRAM_TILE_Y * EDRAM_BPP;
	GLog.Err("EDRAM: Calculated memory size = %d", edramMemorySize);

	// create EDRAM memory buffer
	{
		D3D11_BUFFER_DESC bufferDesc;
		memset(&bufferDesc, 0, sizeof(bufferDesc));
		bufferDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
		bufferDesc.ByteWidth = edramMemorySize;
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.MiscFlags = 0;//D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
		bufferDesc.StructureByteStride = 0;
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;

		// create
		HRESULT hRet = m_device->CreateBuffer(&bufferDesc, NULL, &m_edramMemory);
		if (!m_edramMemory || FAILED(hRet))
		{
			GLog.Err("EDRAM: Failed to create EDRAM memory buffer. Error = 0x%08X", hRet);
		}
	}

	// CPU access buffer
	{
		D3D11_BUFFER_DESC bufferDesc;
		memset(&bufferDesc, 0, sizeof(bufferDesc));
		bufferDesc.BindFlags = 0;
		bufferDesc.ByteWidth = edramMemorySize;
		bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
		bufferDesc.MiscFlags = 0;//D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
		bufferDesc.StructureByteStride = 0;
		bufferDesc.Usage = D3D11_USAGE_STAGING;

		// create
		HRESULT hRet = m_device->CreateBuffer(&bufferDesc, NULL, &m_edramCopy);
		if (!m_edramMemory || FAILED(hRet))
		{
			GLog.Err("EDRAM: Failed to create EDRAM copy buffer. Error = 0x%08X", hRet);
		}
	}

	// Clear EDRAM to default gray color
	{
		// write data to edram copy
		D3D11_MAPPED_SUBRESOURCE resData;
		m_mainContext->Map(m_edramCopy, 0, D3D11_MAP_WRITE, 0, &resData);
		memset(resData.pData, 0x77, edramMemorySize);
		m_mainContext->Unmap(m_edramCopy, 0);

		// copy to resource
		m_mainContext->CopyResource(m_edramMemory, m_edramCopy);
	}

	// create resource view
	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC viewDesc;
		memset(&viewDesc, 0, sizeof(viewDesc));
		viewDesc.Format = DXGI_FORMAT_R32_UINT; // DXGI_FORMAT_R32_TYPELESS
		viewDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		viewDesc.Buffer.Flags = 0;//D3D11_BUFFER_UAV_FLAG_RAW;
		viewDesc.Buffer.FirstElement = 0;
		viewDesc.Buffer.NumElements = edramMemorySize / 4;
		HRESULT hRet = m_device->CreateUnorderedAccessView(m_edramMemory, &viewDesc, &m_edramView);
		if (!m_edramView || FAILED(hRet))
		{
			m_edramMemory->Release();
			m_edramMemory = nullptr;

			GLog.Err("EDRAM: Failed to create EDRAM view buffer. Error = 0x%08X", hRet);
		}
	}

	// create settings buffer
	m_settings.Create(dev);

	// load compute shaders
	LoadShaders();

	// Reset the buffer
	Reset();
}

CDX11SurfaceMemory::~CDX11SurfaceMemory()
{
	// release view
	if (m_edramView)
	{
		m_edramView->Release();
		m_edramView = nullptr;
	}

	// release memory
	if (m_edramMemory)
	{
		m_edramMemory->Release();
		m_edramMemory = nullptr;
	}
}

void CDX11SurfaceMemory::Reset()
{
	// clear the EDRAM with random values
	DispatchFlat(m_fillRandom, 0, NUM_EDRAM_TILES * EDRAM_TILE_X * EDRAM_TILE_Y * EDRAM_BPP);
}

const uint32 CDX11SurfaceMemory::TranslateToGenericFormat_EDRAM(const XenonColorRenderTargetFormat format)
{
	switch (format)
	{
		case XenonColorRenderTargetFormat::Format_8_8_8_8: return GENERIC_FORMAT_8_8_8_8;
		case XenonColorRenderTargetFormat::Format_8_8_8_8_GAMMA: return GENERIC_FORMAT_8_8_8_8_GAMMA;
		case XenonColorRenderTargetFormat::Format_2_10_10_10: return GENERIC_FORMAT_10_10_10_2;
		case XenonColorRenderTargetFormat::Format_2_10_10_10_unknown: return GENERIC_FORMAT_10_10_10_2_GAMMA;
		case XenonColorRenderTargetFormat::Format_16_16: return GENERIC_FORMAT_16_16;
	}

	GLog.Err("Unsupported EDRAM format: %u", format);
	DEBUG_CHECK(!"Unsupported render target format for EDRAM");
	return 0;
}

const uint32 CDX11SurfaceMemory::TranslateToGenericFormat_EDRAM(const XenonDepthRenderTargetFormat format)
{
	DEBUG_CHECK(!"Unsupported depth stencil format for EDRAM");
	return 0;
}

const uint32 CDX11SurfaceMemory::TranslateToGenericFormat_Surface(const XenonColorRenderTargetFormat format)
{
	switch (format)
	{
		case XenonColorRenderTargetFormat::Format_8_8_8_8: return GENERIC_FORMAT_8_8_8_8;
		case XenonColorRenderTargetFormat::Format_8_8_8_8_GAMMA: return GENERIC_FORMAT_8_8_8_8_GAMMA;
		case XenonColorRenderTargetFormat::Format_2_10_10_10: return GENERIC_FORMAT_10_10_10_2;
		case XenonColorRenderTargetFormat::Format_2_10_10_10_unknown: return GENERIC_FORMAT_10_10_10_2_GAMMA;
		case XenonColorRenderTargetFormat::Format_16_16: return GENERIC_FORMAT_16_16;
	}

	GLog.Err("Unsupported EDRAM format: %u", format);
	DEBUG_CHECK(!"Unsupported render target format for surface");
	return 0;
}

const uint32 CDX11SurfaceMemory::TranslateToGenericFormat_Surface(const XenonDepthRenderTargetFormat format)
{
	DEBUG_CHECK(!"Unsupported depth stenicil format for surface");
	return 0;
}

const uint32 CDX11SurfaceMemory::TranslateToGenericFormat_Surface(const XenonTextureFormat format)
{
	switch (format)
	{
		case XenonTextureFormat::Format_8_8_8_8: return GENERIC_FORMAT_8_8_8_8;
		case XenonTextureFormat::Format_2_10_10_10: return GENERIC_FORMAT_10_10_10_2;
		case XenonTextureFormat::Format_16_16: return GENERIC_FORMAT_16_16;
		case XenonTextureFormat::Format_16_16_FLOAT: return GENERIC_FORMAT_16_16_FLOAT;
	}

	GLog.Err("Unsupported EDRAM format: %u", format);
	DEBUG_CHECK(!"Unsupported texture format for surface");
	return 0;
}

void CDX11SurfaceMemory::CopyIntoEDRAM(class CDX11AbstractRenderTarget* rt)
{
	// unable to handle buffers that are NOT MSAA1
	if (rt->GetMSAA() != XenonMsaaSamples::MSSA1X)
	{
		//GLog.Warn( "Non MSAA1 render target: [%dx%d], EDRAM 0x%04X, Format %hs", rt->GetPhysicalWidth(), rt->GetPhysicalHeight(), rt->GetEDRAMPlacement(), XenonGPUGetColorRenderTargetFormatName( rt->GetFormat() ) );
		return;
	}

	// get surface information
	D3D11_TEXTURE2D_DESC texDesc;
	memset(&texDesc, 0, sizeof(texDesc));
	rt->GetTexture()->GetDesc(&texDesc);

	// number of EDRAM blocks left - that's how much we need to copy
	const uint32 maxBlocks = 1024 * 10;
	const uint32 tailingBlocks = maxBlocks - rt->GetEDRAMPlacement();
	const uint32 tailingHeight = (tailingBlocks * 1024) / rt->GetMemoryPitch();

	// determine copy placement
	const uint32 copyX = 0;
	const uint32 copyY = 0;
	const uint32 copyWidth = std::min<uint32>(texDesc.Width, rt->GetMemoryPitch());
	const uint32 copyHeight = std::min<uint32>(texDesc.Height, tailingHeight);

	// configure EDRAM placement
	m_settings.Get().m_edramBaseAddr = rt->GetEDRAMPlacement();
	m_settings.Get().m_edramPitch = rt->GetMemoryPitch(); // in pixels
	m_settings.Get().m_edramOffsetX = 0; // no offset in simple copy
	m_settings.Get().m_edramOffsetY = 0;
	m_settings.Get().m_edramFormat = TranslateToGenericFormat_EDRAM(rt->GetFormat());

	// configure the target surface
	m_settings.Get().m_copySurfaceTargetX = copyX;
	m_settings.Get().m_copySurfaceTargetY = copyY;
	m_settings.Get().m_copySurfaceTotalWidth = texDesc.Width;
	m_settings.Get().m_copySurfaceTotalHeight = texDesc.Height;
	m_settings.Get().m_copySurfaceWidth = copyWidth;
	m_settings.Get().m_copySurfaceHeight = copyHeight;
	m_settings.Get().m_copySurfaceFormat = TranslateToGenericFormat_Surface(rt->GetFormat());

	// dispatch the copy from EDRAM into the texture
	// TODO: add support for 64-bit EDRAM formats
	//DispatchPixels( m_upload32, copyWidth, copyHeight, rt->GetROViewUint() );
}

void CDX11SurfaceMemory::ClearInEDRAM(const XenonColorRenderTargetFormat format, const uint32 edramBase, const uint32 edramPitch, const uint32 height, const float* clearColor)
{
	// configure EDRAM placement
	m_settings.Get().m_edramBaseAddr = edramBase;
	m_settings.Get().m_edramPitch = edramPitch;
	m_settings.Get().m_edramOffsetX = 0; // no offset in simple copy
	m_settings.Get().m_edramOffsetY = 0;
	m_settings.Get().m_edramFormat = TranslateToGenericFormat_EDRAM(format);

	// configure the target surface
	m_settings.Get().m_copySurfaceTargetX = 0;
	m_settings.Get().m_copySurfaceTargetY = 0;
	m_settings.Get().m_copySurfaceTotalWidth = 0;
	m_settings.Get().m_copySurfaceTotalHeight = 0;
	m_settings.Get().m_copySurfaceWidth = 0;
	m_settings.Get().m_copySurfaceHeight = 0;
	m_settings.Get().m_copySurfaceFormat = 0;

	// setup the clear
	m_settings.Get().m_clearColorR = clearColor[0];
	m_settings.Get().m_clearColorG = clearColor[1];
	m_settings.Get().m_clearColorB = clearColor[2];
	m_settings.Get().m_clearColorA = clearColor[3];

	// dispatch the copy from EDRAM into the texture
	// TODO: add support for 64-bit EDRAM formats
	DispatchPixels(m_clear32, edramPitch, height, (ID3D11ShaderResourceView*)nullptr);
}

void CDX11SurfaceMemory::CopyFromEDRAM(class CDX11AbstractRenderTarget* rt)
{
	// unable to handle buffers that are NOT MSAA1
	if (rt->GetMSAA() != XenonMsaaSamples::MSSA1X)
	{
		//GLog.Warn( "Non MSAA1 render target: [%dx%d], EDRAM 0x%04X, Format %hs", rt->GetPhysicalWidth(), rt->GetPhysicalHeight(), rt->GetEDRAMPlacement(), XenonGPUGetColorRenderTargetFormatName( rt->GetFormat() ) );
		return;
	}

	// the "not copied from EDRAM" color
	FLOAT color[4] = { 0.6f, 0.2f, 0.2f, 1.0f };
	m_mainContext->ClearRenderTargetView(rt->GetBindableView(), color);

	// get surface information
	D3D11_TEXTURE2D_DESC texDesc;
	memset(&texDesc, 0, sizeof(texDesc));
	rt->GetTexture()->GetDesc(&texDesc);

	// number of EDRAM blocks left - that's how much we need to copy
	const uint32 maxBlocks = 1024 * 10;
	const uint32 tailingBlocks = maxBlocks - rt->GetEDRAMPlacement();
	const uint32 tailingHeight = (tailingBlocks * 1024) / rt->GetMemoryPitch();

	// determine copy placement
	const uint32 copyX = 0;
	const uint32 copyY = 0;
	const uint32 copyWidth = std::min<uint32>(texDesc.Width, rt->GetMemoryPitch());
	const uint32 copyHeight = std::min<uint32>(texDesc.Height, tailingHeight);

	// configure EDRAM placement
	m_settings.Get().m_edramBaseAddr = rt->GetEDRAMPlacement();
	m_settings.Get().m_edramPitch = rt->GetMemoryPitch(); // in pixels
	m_settings.Get().m_edramOffsetX = 0; // no offset in simple copy
	m_settings.Get().m_edramOffsetY = 0;
	m_settings.Get().m_edramFormat = TranslateToGenericFormat_EDRAM(rt->GetFormat());

	// configure the target surface
	m_settings.Get().m_copySurfaceTargetX = copyX;
	m_settings.Get().m_copySurfaceTargetY = copyY;
	m_settings.Get().m_copySurfaceTotalWidth = texDesc.Width;
	m_settings.Get().m_copySurfaceTotalHeight = texDesc.Height;
	m_settings.Get().m_copySurfaceWidth = copyWidth;
	m_settings.Get().m_copySurfaceHeight = copyHeight;
	m_settings.Get().m_copySurfaceFormat = TranslateToGenericFormat_Surface(rt->GetFormat());

	// limit copiable size
	uint32 maxCopyWidth = copyWidth;
	maxCopyWidth = std::min<uint32>(maxCopyWidth, texDesc.Width - copyX);
	uint32 maxCopyHeight = copyHeight;
	maxCopyHeight = std::min<uint32>(maxCopyHeight, texDesc.Height - copyY);

	// dispatch the copy from EDRAM into the texture
	// TODO: add support for 64-bit formats
	//	DispatchPixels( m_download32, maxCopyWidth, maxCopyHeight, rt->GetRawView() );
}

void CDX11SurfaceMemory::CopyIntoEDRAM(class CDX11AbstractDepthStencil* ds)
{
	// copy current depth buffer content into texture accessible by compute shader
	//ds->TransferToEDRAMCopy();

	// number of EDRAM blocks left - that's how much we need to copy
	const uint32 maxBlocks = 1024 * 10;
	const uint32 tailingBlocks = maxBlocks - ds->GetEDRAMPlacement();
	const uint32 tailingHeight = (tailingBlocks * 1024) / ds->GetMemoryPitch();

	// determine copy placement
	const uint32 copyX = 0;
	const uint32 copyY = 0;
	const uint32 copyWidth = std::min<uint32>(ds->GetPhysicalWidth(), ds->GetMemoryPitch());
	const uint32 copyHeight = std::min<uint32>(ds->GetPhysicalHeight(), tailingHeight);

	// configure EDRAM placement
	m_settings.Get().m_edramBaseAddr = ds->GetEDRAMPlacement();
	m_settings.Get().m_edramPitch = ds->GetMemoryPitch(); // in pixels
	m_settings.Get().m_edramOffsetX = 0; // no offset in simple copy
	m_settings.Get().m_edramOffsetY = 0;
	m_settings.Get().m_edramFormat = TranslateToGenericFormat_EDRAM(ds->GetFormat());

	// configure the target surface
	m_settings.Get().m_copySurfaceTargetX = copyX;
	m_settings.Get().m_copySurfaceTargetY = copyY;
	m_settings.Get().m_copySurfaceTotalWidth = ds->GetPhysicalWidth();
	m_settings.Get().m_copySurfaceTotalHeight = ds->GetPhysicalHeight();
	m_settings.Get().m_copySurfaceWidth = copyWidth;
	m_settings.Get().m_copySurfaceHeight = copyHeight;
	m_settings.Get().m_copySurfaceFormat = TranslateToGenericFormat_Surface(ds->GetFormat());

	// dispatch the copy from EDRAM into the texture
	// TODO: add support for 64-bit EDRAM formats
	//DispatchPixels(m_upload32, copyWidth, copyHeight, ds->GetRawView());
}

void CDX11SurfaceMemory::CopyFromEDRAM(class CDX11AbstractDepthStencil* ds)
{
}

void CDX11SurfaceMemory::Resolve(const XenonColorRenderTargetFormat srcFormat, const uint32 srcBase, const uint32 srcPitch, const uint32 srcX, const uint32 srcY, const uint32 destX, const uint32 destY, const uint32 width, const uint32 height, class CDX11AbstractSurface* destSurf)
{
	// argument check
	DEBUG_CHECK(destSurf != nullptr);

	// configure EDRAM placement
	m_settings.Get().m_edramBaseAddr = srcBase;
	m_settings.Get().m_edramPitch = srcPitch;
	m_settings.Get().m_edramOffsetX = srcX;
	m_settings.Get().m_edramOffsetY = srcY;
	m_settings.Get().m_edramFormat = TranslateToGenericFormat_EDRAM(srcFormat);

	// configure the target surface
	m_settings.Get().m_copySurfaceTargetX = destX;
	m_settings.Get().m_copySurfaceTargetY = destY;
	m_settings.Get().m_copySurfaceTotalWidth = destSurf->GetWidth();
	m_settings.Get().m_copySurfaceTotalHeight = destSurf->GetHeight();
	m_settings.Get().m_copySurfaceWidth = width;
	m_settings.Get().m_copySurfaceHeight = height;
	m_settings.Get().m_copySurfaceFormat = TranslateToGenericFormat_Surface(destSurf->GetFormat());

	// limit copiable size
	uint32 maxCopyWidth = width;
	maxCopyWidth = std::min<uint32>(maxCopyWidth, destSurf->GetWidth() - destX);
	uint32 maxCopyHeight = height;
	maxCopyHeight = std::min<uint32>(maxCopyHeight, destSurf->GetHeight() - destY);

	// dispatch the copy from EDRAM into the texture
	// TODO: add support for floating point formats
	// TODO: add support for 64-bit formats
	//	DispatchPixels( m_download32, width, height, destSurf->GetRawView() );	
}

bool CDX11SurfaceMemory::LoadShaders()
{
#define char uint8

	{
#include "shaders/edramClear.cs.fx.h"
		if (!m_fillRandom.Load(m_device, ShaderData, ARRAYSIZE(ShaderData)))
			return false;
	}

	{
#include "shaders/edramDownload32.cs.fx.h"
		if (!m_download32.Load(m_device, ShaderData, ARRAYSIZE(ShaderData)))
			return false;
	}

	{
#include "shaders/edramUpload32.cs.fx.h"
		if (!m_upload32.Load(m_device, ShaderData, ARRAYSIZE(ShaderData)))
			return false;
	}

	{
#include "shaders/edramClear32.fx.h"
		if (!m_clear32.Load(m_device, ShaderData, ARRAYSIZE(ShaderData)))
			return false;
	}

#undef char

	GLog.Log("EDRAM: All shaders loaded");
	return true;
}

void CDX11SurfaceMemory::DispatchFlat(const CDX11ComputeShader& shader, const uint32 memoryOffset, const uint32 memorySize)
{
	// unset render targets
	ID3D11RenderTargetView* rtv[] = { nullptr, nullptr, nullptr, nullptr };
	m_mainContext->OMSetRenderTargets(4, rtv, nullptr);

	// configure
	m_settings.Get().m_flatOffset = memoryOffset;
	m_settings.Get().m_flatSize = memorySize;

	// calculate number of blocks
	const uint32 numPixels = memorySize / 4;
	const uint32 numBlocks = numPixels / 1024;

	// setup
	m_mainContext->CSSetShader(shader.GetShader(), NULL, 0);
	m_mainContext->CSSetConstantBuffers(0, 1, m_settings.GetBufferForBinding(m_mainContext));
	m_mainContext->CSSetUnorderedAccessViews(0, 1, &m_edramView, NULL);
	m_mainContext->Dispatch(numBlocks, 1, 1);

	// reset binding
	ID3D11UnorderedAccessView* nullView = NULL;
	m_mainContext->CSSetUnorderedAccessViews(0, 1, &nullView, NULL);
}

void CDX11SurfaceMemory::DispatchPixels(const CDX11ComputeShader& shader, const uint32 width, const uint32 height, ID3D11UnorderedAccessView* additionalUAV)
{
	DEBUG_CHECK(additionalUAV != nullptr);

	// unset render targets
	ID3D11RenderTargetView* rtv[] = { nullptr, nullptr, nullptr, nullptr };
	m_mainContext->OMSetRenderTargets(4, rtv, nullptr);

	// calculate number of blocks
	const uint32 numBlocksX = width / 8;
	const uint32 numBlocksY = height / 8;

	// configure
	m_settings.Get().m_pixelDispatchWidth = width;
	m_settings.Get().m_pixelDispatchHeight = height;
	m_settings.Get().m_pixelDispatcBlocksX = numBlocksX;
	m_settings.Get().m_pixelDispatcBlocksY = numBlocksY;

	// UAVs
	ID3D11UnorderedAccessView* views[2] =
	{
		m_edramView,
		additionalUAV,
	};

	// setup
	m_mainContext->CSSetShader(shader.GetShader(), NULL, 0);
	m_mainContext->CSSetConstantBuffers(0, 1, m_settings.GetBufferForBinding(m_mainContext));
	m_mainContext->CSSetUnorderedAccessViews(0, ARRAYSIZE(views), views, NULL);
	m_mainContext->Dispatch(numBlocksX, numBlocksY, 1);

	// reset binding
	ID3D11UnorderedAccessView* nullView = NULL;
	m_mainContext->CSSetUnorderedAccessViews(0, 1, &nullView, NULL);
	m_mainContext->CSSetUnorderedAccessViews(1, 1, &nullView, NULL);
}

void CDX11SurfaceMemory::DispatchPixels(const CDX11ComputeShader& shader, const uint32 width, const uint32 height, ID3D11ShaderResourceView* shaderView)
{
	DEBUG_CHECK(shaderView != nullptr);

	// unset render targets
	ID3D11RenderTargetView* rtv[] = { nullptr, nullptr, nullptr, nullptr };
	m_mainContext->OMSetRenderTargets(4, rtv, nullptr);

	// calculate number of blocks
	const uint32 numBlocksX = width / 8;
	const uint32 numBlocksY = height / 8;

	// configure
	m_settings.Get().m_pixelDispatchWidth = width;
	m_settings.Get().m_pixelDispatchHeight = height;
	m_settings.Get().m_pixelDispatcBlocksX = numBlocksX;
	m_settings.Get().m_pixelDispatcBlocksY = numBlocksY;

	// UAVs
	ID3D11UnorderedAccessView* views[1] =
	{
		m_edramView,
	};

	// SRVs
	ID3D11ShaderResourceView* roViews[1] =
	{
		shaderView,
	};

	// setup
	m_mainContext->CSSetShader(shader.GetShader(), NULL, 0);
	m_mainContext->CSSetConstantBuffers(0, 1, m_settings.GetBufferForBinding(m_mainContext));
	m_mainContext->CSSetUnorderedAccessViews(0, ARRAYSIZE(views), views, NULL);
	m_mainContext->CSSetShaderResources(0, ARRAYSIZE(roViews), roViews);
	m_mainContext->Dispatch(numBlocksX, numBlocksY, 1);

	// reset binding
	ID3D11UnorderedAccessView* nullView = NULL;
	ID3D11ShaderResourceView* nullViewRO = NULL;
	m_mainContext->CSSetUnorderedAccessViews(0, 1, &nullView, NULL);
	m_mainContext->CSSetShaderResources(0, 1, &nullViewRO);
}
