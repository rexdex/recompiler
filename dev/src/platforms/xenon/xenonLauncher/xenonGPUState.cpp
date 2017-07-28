#include "build.h"
#include "xenonLibNatives.h"
#include "xenonGPUUtils.h"
#include "xenonGPUState.h"
#include "xenonGPUAbstractLayer.h"
#include "xenonGPURegisters.h"
#include <xutility>
#include "xenonGPUTextures.h"
#include "xenonGPUDumpWriter.h"

#pragma optimize("",off)

namespace Helper
{
	static inline bool UpdateRegister( const CXenonGPURegisters& regs, const XenonGPURegister reg, uint32& value )
	{
		// value is different
		if ( regs[reg].m_dword != value )
		{
			value = regs[reg].m_dword;
			return true;
		}

		// value is the same
		return false;
	}

	static inline bool UpdateRegister( const CXenonGPURegisters& regs, const XenonGPURegister reg, float& value )
	{
		// value is different
		if ( regs[reg].m_float != value )
		{
			value = regs[reg].m_float;
			return true;
		}

		// value is the same
		return false;
	}

	static inline XenonTextureFormat RTFormatToTextureFormat( XenonColorRenderTargetFormat format )
	{
		switch ( format )
		{
			case XenonColorRenderTargetFormat::Format_8_8_8_8: return XenonTextureFormat::Format_8_8_8_8;
			case XenonColorRenderTargetFormat::Format_8_8_8_8_GAMMA: return XenonTextureFormat::Format_8_8_8_8;
			case XenonColorRenderTargetFormat::Format_2_10_10_10: return XenonTextureFormat::Format_2_10_10_10;
			case XenonColorRenderTargetFormat::Format_2_10_10_10_FLOAT: return XenonTextureFormat::Format_2_10_10_10_FLOAT;
			case XenonColorRenderTargetFormat::Format_16_16: return XenonTextureFormat::Format_16_16;
			case XenonColorRenderTargetFormat::Format_16_16_16_16: return XenonTextureFormat::Format_16_16_16_16;
			//case XenonColorRenderTargetFormat::Format_16_16_FLOAT: return XenonTextureFormat::Format_16_16_FLOAT;
			//case XenonColorRenderTargetFormat::Format_16_16_16_16_FLOAT: return XenonTextureFormat::Format_16_16_16_16_FLOAT;
			case XenonColorRenderTargetFormat::Format_2_10_10_10_unknown: return XenonTextureFormat::Format_2_10_10_10;
			case XenonColorRenderTargetFormat::Format_2_10_10_10_FLOAT_unknown: return XenonTextureFormat::Format_2_10_10_10_FLOAT;
			case XenonColorRenderTargetFormat::Format_32_FLOAT: return XenonTextureFormat::Format_32_FLOAT;
			case XenonColorRenderTargetFormat::Format_32_32_FLOAT: return XenonTextureFormat::Format_32_32_FLOAT;
		}

		GLog.Err("Unsupported texture format: %u", format);
		DEBUG_CHECK(!"Unsupported color format");
		return XenonTextureFormat::Format_Unknown;
	}

	static inline XenonTextureFormat ColorFormatToTextureFormat( XenonColorFormat format )
	{
		switch ( format )
		{
			case XenonColorFormat::Format_8: return XenonTextureFormat::Format_8;
			case XenonColorFormat::Format_8_8_8_8: return XenonTextureFormat::Format_8_8_8_8;
			case XenonColorFormat::Format_2_10_10_10: return XenonTextureFormat::Format_2_10_10_10;
			case XenonColorFormat::Format_32_FLOAT: return XenonTextureFormat::Format_32_FLOAT;
			case XenonColorFormat::Format_16_16: return XenonTextureFormat::Format_16_16;
			case XenonColorFormat::Format_16: return XenonTextureFormat::Format_16;
		}

		GLog.Err("Unsupported texture format: %u", format);
		DEBUG_CHECK( !"Unsupported color format" );
		return XenonTextureFormat::Format_Unknown;
	}

	static inline XenonTextureFormat DepthFormatToTextureFormat(XenonDepthRenderTargetFormat format)
	{
		switch (format)
		{
			case XenonDepthRenderTargetFormat::Format_D24S8: return XenonTextureFormat::Format_24_8;
			case XenonDepthRenderTargetFormat::Format_D24FS8: return XenonTextureFormat::Format_24_8_FLOAT;
		}

		GLog.Err("Unsupported texture format: %u", format);
		DEBUG_CHECK(!"Unsupported color format");
		return XenonTextureFormat::Format_Unknown;
	}

} // Helper

CXenonGPUState::CXenonGPUState()
{
	m_physicalRenderHeight = 0;
	m_physicalRenderWidth = 0;
}

bool CXenonGPUState::IssueDraw( IXenonGPUAbstractLayer* abstractLayer, IXenonGPUDumpWriter* traceDump, const CXenonGPURegisters& regs, const CXenonGPUDirtyRegisterTracker& dirtyRegs, const DrawIndexState& ds )
{
	// global state
	const XenonModeControl enableMode = (XenonModeControl)( regs[XenonGPURegister::REG_RB_MODECONTROL].m_dword & 0x7 );
	if ( enableMode == XenonModeControl::Ignore )
	{
		GLog.Log( "Ignore draw!" );
		return true;
	}
	else if ( enableMode == XenonModeControl::Copy )
	{
		return IssueCopy( abstractLayer, traceDump, regs );
	}

	// clear BEFORE resetting states
	if (ds.m_primitiveType == XenonPrimitiveType::PrimitiveRectangleList)
		return abstractLayer->DrawGeometry(regs, traceDump, ds);

	// prepare drawing conditions
	if ( !UpdateRenderTargets( abstractLayer, regs ) )
		return ReportFailedDraw( "UpdateRenderTargets failed" );
	if ( !UpdateViewportState( abstractLayer, regs ) )
		return ReportFailedDraw( "UpdateViewport failed" );
	if ( !UpdateDepthState( abstractLayer, regs ) )
		return ReportFailedDraw( "UpdateDepthState failed" );
	if ( !UpdateBlendState( abstractLayer, regs ) )
		return ReportFailedDraw( "UpdateBlendState failed" );
	if ( !UpdateRasterState( abstractLayer, regs ) )
		return ReportFailedDraw( "UpdateRasterState failed" );
	if ( !UpdateShaderConstants( abstractLayer, regs, dirtyRegs ) )
		return ReportFailedDraw( "UpdateShaderConstants failed" );
	if ( !UpdateTexturesAndSamplers( abstractLayer, regs, traceDump ) )
		return ReportFailedDraw( "UpdateTexturesAndSamplers failed" );

	// perform draw
	return abstractLayer->DrawGeometry( regs, traceDump, ds );
}

bool CXenonGPUState::IssueSwap( IXenonGPUAbstractLayer* abstractLayer, IXenonGPUDumpWriter* traceDump, const CXenonGPURegisters& regs, const SwapState& ss )
{
	abstractLayer->BeingFrame();
	abstractLayer->Swap( ss );
	return true;
}

bool CXenonGPUState::IssueCopy( IXenonGPUAbstractLayer* abstractLayer, IXenonGPUDumpWriter* traceDump, const CXenonGPURegisters& regs )
{
	// Resolve + optional (quite often) clear
	// Resolve is done through the abstractInterface (which forwards it to the EDRAM emulator)
	// The clear is done directly

	// master register
	const uint32 copyReg = regs[ XenonGPURegister::REG_RB_COPY_CONTROL ].m_dword;

	// which render targets are affected ? (0-3 = colorRT, 4=depth)
	const uint32 copyRT = (copyReg & 7);

	// should we clear after copy ?
	const bool colorClearEnabled = (copyReg >> 8) & 1;
	const bool depthClearEnabled = (copyReg >> 9) & 1;

	// what to do
	auto copyCommand = (XenonCopyCommand)( (copyReg >> 20) & 3);

	// target memory and format for the copy operation
	const uint32 copyDestInfoReg = regs[ XenonGPURegister::REG_RB_COPY_DEST_INFO ].m_dword;
	const auto copyDestEndian = (XenonGPUEndianFormat128)( copyDestInfoReg & 7 );
	const uint32 copyDestArray = (copyDestInfoReg >> 3) & 1;
	DEBUG_CHECK( copyDestArray == 0 ); // other values not yet supported
	const uint32 copyDestSlice = (copyDestInfoReg >> 4) & 1;
	DEBUG_CHECK( copyDestSlice == 0 ); // other values not yet supported
	const auto copyDestFormat = (XenonColorFormat)( (copyDestInfoReg >> 7) & 0x3F );

	const uint32 copyDestNumber = (copyDestInfoReg >> 13) & 7;
	const uint32 copyDestBias = (copyDestInfoReg >> 16) & 0x3F;
	const uint32 copyDestSwap = (copyDestInfoReg >> 25) & 1;

	const uint32 copyDestBase = regs[ XenonGPURegister::REG_RB_COPY_DEST_BASE ].m_dword;
	const uint32 copyDestPitch = regs[ XenonGPURegister::REG_RB_COPY_DEST_PITCH ].m_dword & 0x3FFF;
	const uint32 copyDestHeight = (regs[ XenonGPURegister::REG_RB_COPY_DEST_PITCH ].m_dword >> 16) & 0x3FFF;

	// additional way to copy
	const uint32 copySurfaceSlice = regs[ XenonGPURegister::REG_RB_COPY_SURFACE_SLICE ].m_dword;
	DEBUG_CHECK( copySurfaceSlice == 0 );
	const uint32 copyFunc = regs[ XenonGPURegister::REG_RB_COPY_FUNC ].m_dword;
	DEBUG_CHECK( copyFunc == 0 );
	const uint32 copyRef = regs[ XenonGPURegister::REG_RB_COPY_REF ].m_dword;
	DEBUG_CHECK( copyRef == 0 );
	const uint32 copyMask = regs[ XenonGPURegister::REG_RB_COPY_MASK ].m_dword;
	DEBUG_CHECK( copyMask == 0 );

	// RB_SURFACE_INFO
	// http://fossies.org/dox/MesaLib-10.3.5/fd2__gmem_8c_source.html
	const uint32 surfaceInfoReg = regs[ XenonGPURegister::REG_RB_SURFACE_INFO ].m_dword;
	const uint32 surfacePitch = surfaceInfoReg & 0x3FFF;
	const auto surfaceMSAA = static_cast< XenonMsaaSamples >((surfaceInfoReg >> 16) & 0x3);

	// do the actual copy
	if ( copyCommand != XenonCopyCommand::Null && copyCommand != XenonCopyCommand::ConstantOne )
	{
		const uint32 destLogicalWidth = copyDestPitch;
		const uint32 destLogicalHeight = copyDestHeight;
		const uint32 destBlockWidth = Helper::RoundUp( destLogicalWidth, 32 );
		const uint32 destBlockHeight = Helper::RoundUp( destLogicalHeight, 32 );

		// Compute destination and source area
		XenonRect2D destRect, srcRect;
		{		
			uint32 windowOffset = regs[ XenonGPURegister::REG_PA_SC_WINDOW_OFFSET ].m_dword;
			int16 windowOffsetX = (int16) windowOffset & 0x7FFF;
			int16 windowOffsetY = (int16)( (windowOffset >> 16) & 0x7FFF );

			if ( windowOffsetX & 0x4000)
				windowOffsetX |= 0x8000;

			if ( windowOffsetY & 0x4000)
				windowOffsetY |= 0x8000;

			// HACK: vertices to use are always in vf0.
			{
				const uint32 copyVertexFetchSlot = 0;
				const uint32 reg = (uint32) XenonGPURegister::REG_SHADER_CONSTANT_FETCH_00_0 + (copyVertexFetchSlot * 2);
				const XenonGPUVertexFetchData* fetch = &regs.GetStructAt<XenonGPUVertexFetchData>( (XenonGPURegister) reg );

				DEBUG_CHECK(fetch->type == 3);
				DEBUG_CHECK(fetch->endian == 2);
				DEBUG_CHECK(fetch->size == 6);

				const float* vertexData = (const float*) GPlatform.GetMemory().TranslatePhysicalAddress( fetch->address << 2 );

				// fetch vertex data
				float vertexX[3]; // x coords
				float vertexY[3]; // y coords
				const auto endianess = (XenonGPUEndianFormat) fetch->endian;
				vertexX[0] = XenonGPUSwapFloat( vertexData[0], endianess );
				vertexY[0] = XenonGPUSwapFloat( vertexData[1], endianess );

				vertexX[1] = XenonGPUSwapFloat( vertexData[2], endianess );
				vertexY[1] = XenonGPUSwapFloat( vertexData[3], endianess );

				vertexX[2] = XenonGPUSwapFloat( vertexData[4], endianess );
				vertexY[2] = XenonGPUSwapFloat( vertexData[5], endianess );

				// get min/max ranges
				const float destMinX = std::min<float>( std::min<float>( vertexX[0], vertexX[1] ), vertexX[2] );
				const float destMinY = std::min<float>( std::min<float>( vertexY[0], vertexY[1] ), vertexY[2] );
				const float destMaxX = std::max<float>( std::max<float>( vertexX[0], vertexX[1] ), vertexX[2] );
				const float destMaxY = std::max<float>( std::max<float>( vertexY[0], vertexY[1] ), vertexY[2] );

				// setup dest area
				destRect.x = (int) destMinX;
				destRect.y = (int) destMinY;
				destRect.w = (int)( destMaxX - destMinX );
				destRect.h = (int)( destMaxY - destMinY );

				// setup source copy area
				srcRect.x = 0;
				srcRect.y = 0;
				srcRect.w = destRect.w;
				srcRect.h = destRect.h;
			}

			// The dest base address passed in has already been offset by the window
			// offset, so to ensure texture lookup works we need to offset it.
			const int32 destOffset = (windowOffsetY * copyDestPitch * 4) + (windowOffsetX * 32 * 4);

			// For some reason does not work as expeted, why ?
			//copyDestBase += destOffset;
		}


		// Resolve the source render target to target texture
		// NOTE: we may be requested to do format conversion
		{
			XenonTextureFormat srcFormat = XenonTextureFormat::Format_Unknown;

			// color RT to copy ?
			if ( copyRT <= 3 ) 
			{
				const XenonGPURegister colorInfoRegs[] = { XenonGPURegister::REG_RB_COLOR_INFO, XenonGPURegister::REG_RB_COLOR1_INFO, XenonGPURegister::REG_RB_COLOR2_INFO, XenonGPURegister::REG_RB_COLOR3_INFO };
				const uint32 colorInfo = regs[ colorInfoRegs[ copyRT ] ].m_dword;

				const uint32 colorBase = colorInfo & 0xFFF; // EDRAM base
				const auto colorFormat = ( XenonColorRenderTargetFormat )( (colorInfo >> 16) & 0xF );

				abstractLayer->ResolveColorRenderTarget( copyRT, colorFormat, colorBase, srcRect, 
					copyDestBase, destLogicalWidth, destLogicalHeight, destBlockWidth, destBlockHeight, 
					Helper::ColorFormatToTextureFormat(copyDestFormat), destRect );
			}
			else
			{
				// Source from depth/stencil.
				const uint32 depthInfo = regs[ XenonGPURegister::REG_RB_DEPTH_INFO ].m_dword;
				const uint32 depthBase = (depthInfo & 0xFFF); // EDRAM base

				auto depthFormat = (XenonDepthRenderTargetFormat)( (depthInfo >> 16) & 0x1 );

				abstractLayer->ResolveDepthRenderTarget(depthFormat, depthBase, srcRect,
					copyDestBase, destLogicalWidth, destLogicalHeight, destBlockWidth, destBlockHeight,
					Helper::DepthFormatToTextureFormat(depthFormat), destRect);
			}
		}
	}

	// Perform any requested clears.
	const uint32 copyDepthClear = regs[ XenonGPURegister::REG_RB_DEPTH_CLEAR ].m_dword;
	const uint32 copyColorClear = regs[ XenonGPURegister::REG_RB_COLOR_CLEAR ].m_dword;
	const uint32 copyColorClearLow = regs[ XenonGPURegister::REG_RB_COLOR_CLEAR_LOW ].m_dword;
	DEBUG_CHECK( copyColorClear == copyColorClearLow );

	// Clear the color buffer
	// NOTE: this happens AFTER resolve
	if ( colorClearEnabled )
	{
		DEBUG_CHECK( copyRT <= 3 );

		// Extract clear color
		float clearColor[4];
		clearColor[0] = ((copyColorClear >> 16) & 0xFF) / 255.0f;
		clearColor[1] = ((copyColorClear >> 8) & 0xFF) / 255.0f;
		clearColor[2] = ((copyColorClear >> 0) & 0xFF) / 255.0f;
		clearColor[3] = ((copyColorClear >> 24) & 0xFF) / 255.0f; // Alpha

		// clear the RT using the abstract interface
		// NOTE: this will clear the EDRAM surface + the actual RT that is bound there
		const bool flushToEDRAM = true;
		abstractLayer->ClearColorRenderTarget( copyRT, clearColor, flushToEDRAM );
	}

	// Clear the depth buffer
	// NOTE: this happens AFTER resolve
	if ( depthClearEnabled )
	{
		const float clearDepthValue = (copyDepthClear & 0xFFFFFF00) / (float)0xFFFFFF00; // maps well to 0-1 range for now
		const uint32 clearStencilValue = copyDepthClear & 0xFF;

		// clear the DS
		// NOTE: this will clear the EDRAM surface + the actual RT that is bound there
		const bool flushToEDRAM = true;
		abstractLayer->ClearDepthStencilRenderTarget( clearDepthValue, clearStencilValue, flushToEDRAM );
	}

	// done
	return true;
}

bool CXenonGPUState::ReportFailedDraw( const char* reason )
{
	GLog.Err( "GPU: Failed draw, reason: %hs", reason );
	return false;
}

bool CXenonGPUState::UpdateRenderTargets( IXenonGPUAbstractLayer* abstractLayer, const CXenonGPURegisters& regs )
{
	/// update state and check if it's different
	bool stateChanged = false;
	stateChanged |= Helper::UpdateRegister( regs, XenonGPURegister::REG_RB_MODECONTROL, m_rtState.regModeControl );
	stateChanged |= Helper::UpdateRegister( regs, XenonGPURegister::REG_RB_SURFACE_INFO, m_rtState.regSurfaceInfo );
	stateChanged |= Helper::UpdateRegister( regs, XenonGPURegister::REG_RB_COLOR_INFO, m_rtState.regColorInfo[0] );
	stateChanged |= Helper::UpdateRegister( regs, XenonGPURegister::REG_RB_COLOR1_INFO, m_rtState.regColorInfo[1] );
	stateChanged |= Helper::UpdateRegister( regs, XenonGPURegister::REG_RB_COLOR2_INFO, m_rtState.regColorInfo[2] );
	stateChanged |= Helper::UpdateRegister( regs, XenonGPURegister::REG_RB_COLOR3_INFO, m_rtState.regColorInfo[3] );
	stateChanged |= Helper::UpdateRegister( regs, XenonGPURegister::REG_RB_COLOR_MASK, m_rtState.regColorMask );
	stateChanged |= Helper::UpdateRegister( regs, XenonGPURegister::REG_RB_DEPTHCONTROL, m_rtState.regDepthControl );
	stateChanged |= Helper::UpdateRegister( regs, XenonGPURegister::REG_RB_STENCILREFMASK, m_rtState.regStencilRefMask );
	stateChanged |= Helper::UpdateRegister( regs, XenonGPURegister::REG_RB_DEPTH_INFO, m_rtState.regDepthInfo );

	// check if state is up to date
	if ( !stateChanged )
		return true;

	// apply the state
	return ApplyRenderTargets( abstractLayer, m_rtState, m_physicalRenderWidth, m_physicalRenderHeight );
}

bool CXenonGPUState::UpdateViewportState( IXenonGPUAbstractLayer* abstractLayer, const CXenonGPURegisters& regs )
{
	/// update state and check if it's different
	bool stateChanged = false;
	stateChanged |= Helper::UpdateRegister( regs, XenonGPURegister::REG_RB_SURFACE_INFO, m_viewState.regSurfaceInfo );
	stateChanged |= Helper::UpdateRegister( regs, XenonGPURegister::REG_PA_CL_VTE_CNTL, m_viewState.regPaClVteCntl );
	stateChanged |= Helper::UpdateRegister( regs, XenonGPURegister::REG_PA_SU_SC_MODE_CNTL, m_viewState.regPaSuScModeCntl );
	stateChanged |= Helper::UpdateRegister( regs, XenonGPURegister::REG_PA_SC_WINDOW_OFFSET, m_viewState.regPaScWindowOffset );
	stateChanged |= Helper::UpdateRegister( regs, XenonGPURegister::REG_PA_SC_WINDOW_SCISSOR_TL, m_viewState.regPaScWindowScissorTL );
	stateChanged |= Helper::UpdateRegister( regs, XenonGPURegister::REG_PA_SC_WINDOW_SCISSOR_BR, m_viewState.regPaScWindowScissorBR );
	stateChanged |= Helper::UpdateRegister( regs, XenonGPURegister::REG_PA_CL_VPORT_XOFFSET, m_viewState.regPaClVportXoffset );
	stateChanged |= Helper::UpdateRegister( regs, XenonGPURegister::REG_PA_CL_VPORT_YOFFSET, m_viewState.regPaClVportYoffset );
	stateChanged |= Helper::UpdateRegister( regs, XenonGPURegister::REG_PA_CL_VPORT_ZOFFSET, m_viewState.regPaClVportZoffset );
	stateChanged |= Helper::UpdateRegister( regs, XenonGPURegister::REG_PA_CL_VPORT_XSCALE, m_viewState.regPaClVportXscale );
	stateChanged |= Helper::UpdateRegister( regs, XenonGPURegister::REG_PA_CL_VPORT_YSCALE, m_viewState.regPaClVportYscale );
	stateChanged |= Helper::UpdateRegister( regs, XenonGPURegister::REG_PA_CL_VPORT_ZSCALE, m_viewState.regPaClVportZscale );

	// check if state is up to date
	if ( !stateChanged )
		return true;

	// apply the state
	return ApplyViewportState( abstractLayer, m_viewState, m_physicalRenderWidth, m_physicalRenderHeight );
}

bool CXenonGPUState::UpdateRasterState( IXenonGPUAbstractLayer* abstractLayer, const CXenonGPURegisters& regs )
{
	/// update state and check if it's different
	bool stateChanged = false;
	stateChanged |= Helper::UpdateRegister( regs, XenonGPURegister::REG_PA_SU_SC_MODE_CNTL, m_rasterState.regPaSuScModeCntl );
	stateChanged |= Helper::UpdateRegister( regs, XenonGPURegister::REG_PA_SC_SCREEN_SCISSOR_TL, m_rasterState.regPaScScreenScissorTL );
	stateChanged |= Helper::UpdateRegister( regs, XenonGPURegister::REG_PA_SC_SCREEN_SCISSOR_BR, m_rasterState.regPaScScreenScissorBR );
	stateChanged |= Helper::UpdateRegister( regs, XenonGPURegister::REG_VGT_MULTI_PRIM_IB_RESET_INDX, m_rasterState.regMultiPrimIbResetIndex );

	// check if state is up to date
	if ( !stateChanged )
		return true;

	// apply the state
	return ApplyRasterState( abstractLayer, m_rasterState );
}

bool CXenonGPUState::UpdateBlendState( IXenonGPUAbstractLayer* abstractLayer, const CXenonGPURegisters& regs )
{
	/// update state and check if it's different
	bool stateChanged = false;
	stateChanged |= Helper::UpdateRegister( regs, XenonGPURegister::REG_RB_BLENDCONTROL_0, m_blendState.regRbBlendControl[0] );
	stateChanged |= Helper::UpdateRegister( regs, XenonGPURegister::REG_RB_BLENDCONTROL_1, m_blendState.regRbBlendControl[1] );
	stateChanged |= Helper::UpdateRegister( regs, XenonGPURegister::REG_RB_BLENDCONTROL_2, m_blendState.regRbBlendControl[2] );
	stateChanged |= Helper::UpdateRegister( regs, XenonGPURegister::REG_RB_BLENDCONTROL_3, m_blendState.regRbBlendControl[3] );
	stateChanged |= Helper::UpdateRegister( regs, XenonGPURegister::REG_RB_BLEND_RED, m_blendState.regRbBlendRGBA[0] );
	stateChanged |= Helper::UpdateRegister( regs, XenonGPURegister::REG_RB_BLEND_GREEN, m_blendState.regRbBlendRGBA[1] );
	stateChanged |= Helper::UpdateRegister( regs, XenonGPURegister::REG_RB_BLEND_BLUE, m_blendState.regRbBlendRGBA[2] );
	stateChanged |= Helper::UpdateRegister( regs, XenonGPURegister::REG_RB_BLEND_ALPHA, m_blendState.regRbBlendRGBA[3] );

	// check if state is up to date
	/*if ( !stateChanged )
		return true;*/

	// apply the state
	return ApplyBlendState( abstractLayer, m_blendState );
}

bool CXenonGPUState::UpdateDepthState( IXenonGPUAbstractLayer* abstractLayer, const CXenonGPURegisters& regs )
{
	/// update state and check if it's different
	bool stateChanged = false;
	stateChanged |= Helper::UpdateRegister( regs, XenonGPURegister::REG_RB_DEPTHCONTROL, m_depthState.regRbDepthControl );
	stateChanged |= Helper::UpdateRegister( regs, XenonGPURegister::REG_RB_STENCILREFMASK, m_depthState.regRbStencilRefMask );

	// check if state is up to date
	if ( !stateChanged )
		return true;

	// apply the state
	return ApplyDepthState( abstractLayer, m_depthState );
}

bool CXenonGPUState::ApplyRenderTargets( IXenonGPUAbstractLayer* abstractLayer, const XenonStateRenderTargetsRegisters& rtState, uint32& outPhysicalWidth, uint32& outPhysicalHeight )
{
	// RB_SURFACE_INFO
	// http://fossies.org/dox/MesaLib-10.3.5/fd2__gmem_8c_source.html

	// extract specific settings
	const XenonModeControl enableMode = (XenonModeControl)( rtState.regModeControl & 0x7 );
	const XenonMsaaSamples surfaceMSAA = XenonMsaaSamples::MSSA1X;// (XenonMsaaSamples)((rtState.regSurfaceInfo >> 16) & 3);
	const uint32 surfacePitch = rtState.regSurfaceInfo & 0x3FFF;

	// Get/create all color render targets, if we are using them.
	// In depth-only mode we don't need them.
	// Note that write mask may be more permissive than we want, so we mix that
	// with the actual targets the pixel shader writes to.
	if ( enableMode == XenonModeControl::ColorDepth )
	{
		for ( uint32 rtIndex=0; rtIndex<4; ++rtIndex )
		{
			const uint32 rtInfo = rtState.regColorInfo[ rtIndex ];

			// get color mask for this specific render target
			const uint32 writeMask = (rtState.regColorMask >> (rtIndex * 4)) & 0xF;
			if ( !writeMask )
			{
				// this RT is not used
				abstractLayer->UnbindColorRenderTarget( rtIndex );
				continue;
			}

			// get base EDRAM tile index
			const uint32 memoryBase = rtInfo & 0xFFF;

			// get format of the RT
			const XenonColorRenderTargetFormat rtFormat = (XenonColorRenderTargetFormat) ( (rtInfo >> 16) & 0xF );

			// bind RT
			abstractLayer->BindColorRenderTarget( rtIndex, rtFormat, surfaceMSAA, memoryBase, surfacePitch );
			abstractLayer->SetColorRenderTargetWriteMask( rtIndex, 0 != (writeMask&1), 0 != (writeMask&2), 0 != (writeMask&4), 0 != (writeMask&8) );
		}
	}
	else
	{
		// no color render targets
		abstractLayer->UnbindColorRenderTarget( 0 );
		abstractLayer->UnbindColorRenderTarget( 1 );
		abstractLayer->UnbindColorRenderTarget( 2 );
		abstractLayer->UnbindColorRenderTarget( 3 );
	}

	// Get/create depth buffer, but only if we are going to use it.
	const bool usesDepth = (rtState.regDepthControl & 0x00000002) || (rtState.regDepthControl & 0x00000004);
	const uint32 stencilWriteMask = (rtState.regStencilRefMask & 0x00FF0000) >> 16;
	const bool usesStencil = (rtState.regDepthControl & 0x00000001) || (stencilWriteMask != 0);
	if ( usesDepth || usesStencil )
	{
		// get base EDRAM tile index
		const uint32 memoryBase = rtState.regDepthInfo & 0xFFF;

		// get format of the DS surface
		const XenonDepthRenderTargetFormat dsFormat = (XenonDepthRenderTargetFormat)( (rtState.regDepthInfo >> 16) & 1 );
		abstractLayer->BindDepthStencil( dsFormat, surfaceMSAA, memoryBase, surfacePitch );
	}
	else
	{
		abstractLayer->UnbindDepthStencil();
	}

	// realize the frame setup
	return abstractLayer->RealizeSurfaceSetup( outPhysicalWidth, outPhysicalHeight );
}

bool CXenonGPUState::ApplyViewportState( IXenonGPUAbstractLayer* abstractLayer, const XenonStateViewportRegisters& viewState, const uint32 physicalWidth, const uint32 physicalHeight )
{
	// source:
	// http://fossies.org/dox/MesaLib-10.3.5/fd2__gmem_8c_source.html
	// http://www.x.org/docs/AMD/old/evergreen_3D_registers_v2.pdf
	// https://github.com/freedreno/mesa/blob/master/src/mesa/drivers/dri/r200/r200_state.c

	// VTX_XY_FMT = true: the incoming X, Y have already been multiplied by 1/W0
	// VTX_Z_FMT = true: the incoming Z has already been multiplied by 1/W0
	// VTX_W0_FMT = true: the incoming W0 is not 1/W0. Perform the reciprocal to get 1/W0.
	{
		const bool xyDivided = 0 != ( (viewState.regPaClVteCntl >> 8) & 1 );
		const bool zDivided = 0 != ( (viewState.regPaClVteCntl >> 9) & 1 );
		const bool wNotInverted = 0 != ( (viewState.regPaClVteCntl >> 10) & 1 );
		abstractLayer->SetViewportVertexFormat( xyDivided, zDivided, wNotInverted );
	}

	// Normalized/Unnormalized pixel coordinates 
	{
		const bool bNormalizedCoordinates = (viewState.regPaClVteCntl & 1);
		abstractLayer->SetViewportWindowScale( bNormalizedCoordinates );
		if ( !bNormalizedCoordinates )
			GLog.Log( "Not normalized!");
	}

	// Clipping.
	// https://github.com/freedreno/amd-gpu/blob/master/include/reg/yamato/14/yamato_genenum.h#L1587
	// bool clip_enabled = ((regs.pa_cl_clip_cntl >> 17) & 0x1) == 0;
	// bool dx_clip = ((regs.pa_cl_clip_cntl >> 19) & 0x1) == 0x1;
	// if (dx_clip) {
	//  glClipControl(GL_UPPER_LEFT, GL_ZERO_TO_ONE);
	//} else {
	//  glClipControl(GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE);
	//}

	// Window parameters.
	// http://ftp.tku.edu.tw/NetBSD/NetBSD-current/xsrc/external/mit/xf86-video-ati/dist/src/r600_reg_auto_r6xx.h
	// See r200UpdateWindow:
	// https://github.com/freedreno/mesa/blob/master/src/mesa/drivers/dri/r200/r200_state.c
	int16 windowOffsetX = 0, windowOffsetY = 0;
	if ( (viewState.regPaSuScModeCntl >> 16) & 1)
	{
		// extract 15-bit signes values
		windowOffsetX = viewState.regPaScWindowOffset & 0x7FFF;
		windowOffsetY = (viewState.regPaScWindowOffset >> 16) & 0x7FFF;

		// restore signs
		if ( windowOffsetX & 0x4000 )
			windowOffsetX |= 0x8000;
		if ( windowOffsetY & 0x4000 )
			windowOffsetY |= 0x8000;
	}

	// Setup scissor
	{
		const uint32 scissorX = (viewState.regPaScWindowScissorTL & 0x7FFF);
		const uint32 scissorY = ((viewState.regPaScWindowScissorTL >> 16) & 0x7FFF);
		const uint32 scissorW = (viewState.regPaScWindowScissorBR & 0x7FFF) - scissorX;
		const uint32 scissorH = ((viewState.regPaScWindowScissorBR >> 16) & 0x7FFF) - scissorY;
		abstractLayer->EnableScisor( scissorX + windowOffsetX, scissorY + windowOffsetY, scissorW, scissorH );
	}

	// Setup viewport
	{
		// Whether each of the viewport settings are enabled.
		// http://www.x.org/docs/AMD/old/evergreen_3D_registers_v2.pdf
		const bool xScaleEnabled = (viewState.regPaClVteCntl & (1 << 0)) != 0;
		const bool xOffsetEnabled = (viewState.regPaClVteCntl & (1 << 1)) != 0;
		const bool yScaleEnabled = (viewState.regPaClVteCntl & (1 << 2)) != 0;
		const bool yOffsetEnabled = (viewState.regPaClVteCntl & (1 << 3)) != 0;
		const bool zScaleEnabled = (viewState.regPaClVteCntl & (1 << 4)) != 0;
		const bool zOffsetEnabled = (viewState.regPaClVteCntl & (1 << 5)) != 0;

		// They all should be enabled together
		DEBUG_CHECK( xScaleEnabled == yScaleEnabled == zScaleEnabled == xOffsetEnabled == yOffsetEnabled == zOffsetEnabled );

		// get viewport params
		const float vox = xOffsetEnabled ? viewState.regPaClVportXoffset : 0;
		const float voy = yOffsetEnabled ? viewState.regPaClVportYoffset : 0;
		const float voz = zOffsetEnabled ? viewState.regPaClVportZoffset : 0;
		const float vsx = xScaleEnabled ? viewState.regPaClVportXscale : 0;
		const float vsy = yScaleEnabled ? viewState.regPaClVportYscale : 0;
		const float vsz = zScaleEnabled ? viewState.regPaClVportZscale : 0;

		// pixel->texel offset
		const float xTexelOffset = 0.0f;
		const float yTexelOffset = 0.0f;

		// Setup actual viewport
		if ( xScaleEnabled )
		{
			const float vpw = 2 * vsx;
			const float vph = -2 * vsy;
			const float vpx = vox - vpw / 2 + windowOffsetX;
			const float vpy = voy - vph / 2 + windowOffsetY;
			abstractLayer->SetViewportRange( vpx + xTexelOffset, vpy + yTexelOffset, vpw, vph );
		}
		else
		{
			// We have no viewport information, use default

			// Determine window scale
			// HACK: no clue where to get these values.
			// RB_SURFACE_INFO
			uint32 windowScaleX = 1;
			uint32 windowScaleY = 1;
			const XenonMsaaSamples surfaceMsaa = (XenonMsaaSamples)( (viewState.regSurfaceInfo >> 16) & 3 );
			if ( surfaceMsaa == XenonMsaaSamples::MSSA2X )
			{
				windowScaleX = 2;
			}
			else if ( surfaceMsaa == XenonMsaaSamples::MSSA4X )
			{
				windowScaleX = 2;
				windowScaleY = 2;
			}

			// set the natural (physical) viewport size
			const float vpw = 2.0f * (float) physicalWidth;
			const float vph = 2.0f * (float) physicalHeight;
			const float vpx = -(float) physicalWidth / 1.0f;
			const float vpy = -(float) physicalHeight / 1.0f;

			abstractLayer->SetViewportRange( vpx + xTexelOffset, vpy + yTexelOffset, vpw, vph);
		}

		// Setup depth range
		if ( zScaleEnabled && zOffsetEnabled )
			abstractLayer->SetDepthRange( voz, voz + vsz );
	}

	// valid so far, see what's the abstraction layer has to say
	return abstractLayer->RealizeViewportSetup();
}

bool CXenonGPUState::ApplyRasterState( IXenonGPUAbstractLayer* abstractLayer, const XenonStateRasterizerRegisters& rasterState )
{
	// set cull mode
	const auto cullMode = (XenonCullMode)( (rasterState.regPaSuScModeCntl & 0x3) );
	abstractLayer->SetCullMode( cullMode );

	// set face winding
	const auto frontFaceCW = (rasterState.regPaSuScModeCntl & 0x4) != 0;
	abstractLayer->SetFaceMode( frontFaceCW ? XenonFrontFace::CW :XenonFrontFace::CCW );

	// set polygon mode
	const bool bPolygonMode = ((rasterState.regPaSuScModeCntl >> 3) & 0x3) != 0;
	if ( bPolygonMode )
	{
		const auto frontMode = (XenonFillMode)( (rasterState.regPaSuScModeCntl >> 5) & 0x7 );
		const auto backMode = (XenonFillMode)( (rasterState.regPaSuScModeCntl >> 8) & 0x7 );
		DEBUG_CHECK( frontMode == backMode );

		abstractLayer->SetFillMode( frontMode );
	}
	else
	{
		abstractLayer->SetFillMode( XenonFillMode::Solid );
	}
	
	// primitive restart index - not supported yet
	const bool bPrimitiveRestart = (rasterState.regPaSuScModeCntl & (1 << 21)) != 0;
	abstractLayer->SetPrimitiveRestart( bPrimitiveRestart );
	abstractLayer->SetPrimitiveRestartIndex( rasterState.regMultiPrimIbResetIndex );

	// realize state
	return abstractLayer->RealizeRasterState();
}

bool CXenonGPUState::ApplyBlendState( IXenonGPUAbstractLayer* abstractLayer, const XenonStateBlendRegisters& blendState )
{
	// blend color
	abstractLayer->SetBlendColor( blendState.regRbBlendRGBA[0], blendState.regRbBlendRGBA[1], blendState.regRbBlendRGBA[2], blendState.regRbBlendRGBA[3] );

	// render targets
	for ( uint32 i=0; i<4; ++i )
	{		
		const uint32 blendControl = blendState.regRbBlendControl[i];

		// A2XX_RB_BLEND_CONTROL_COLOR_SRCBLEND
		const auto colorSrc = (XenonBlendArg)( (blendControl & 0x0000001F) >> 0 );

		// A2XX_RB_BLEND_CONTROL_COLOR_DESTBLEND
		const auto colorDest = (XenonBlendArg)( (blendControl & 0x00001F00) >> 8 );

		// A2XX_RB_BLEND_CONTROL_COLOR_COMB_FCN
		const auto colorOp = (XenonBlendOp)( (blendControl & 0x000000E0) >> 5 );

		// A2XX_RB_BLEND_CONTROL_ALPHA_SRCBLEND
		const auto alphaSrc = (XenonBlendArg)( (blendControl & 0x001F0000) >> 16 );

		// A2XX_RB_BLEND_CONTROL_ALPHA_DESTBLEND
		const auto alphaDest = (XenonBlendArg)( (blendControl & 0x1F000000) >> 24 );

		// A2XX_RB_BLEND_CONTROL_ALPHA_COMB_FCN
		const auto alphaOp = (XenonBlendOp)( (blendControl & 0x00E00000) >> 21 );

		// is blending enabled ?
		const bool isColorSolid = ( (colorSrc == XenonBlendArg::One) && (colorDest == XenonBlendArg::Zero) && (colorOp == XenonBlendOp::Add) );
		const bool isAlphaSolid = ( (alphaSrc == XenonBlendArg::One) && (alphaDest == XenonBlendArg::Zero) && (alphaOp == XenonBlendOp::Add) );
		if ( isColorSolid && isAlphaSolid )
		{
			abstractLayer->SetBlend( i, false );
		}
		else
		{
			abstractLayer->SetBlend( i, true );
			abstractLayer->SetBlendOp( i, colorOp, alphaOp );
			abstractLayer->SetBlendArg( i, colorSrc, colorDest, alphaSrc, alphaDest );
		}
	}

	// realize state via the D3D11 interface
	return abstractLayer->RealizeBlendState();
}

bool CXenonGPUState::ApplyDepthState( IXenonGPUAbstractLayer* abstractLayer, const XenonStateDepthStencilRegisters& depthState )
{
	// A2XX_RB_DEPTHCONTROL_Z_ENABLE
	const bool bDepthTest = (0 != (depthState.regRbDepthControl & 0x00000002));
	abstractLayer->SetDepthTest( bDepthTest );

	// A2XX_RB_DEPTHCONTROL_Z_WRITE_ENABLE
	const bool bDepthWrite = (0 != (depthState.regRbDepthControl & 0x00000004));
	abstractLayer->SetDepthWrite( bDepthWrite );

	// A2XX_RB_DEPTHCONTROL_EARLY_Z_ENABLE
	// ?
	// A2XX_RB_DEPTHCONTROL_ZFUNC
	const auto depthFunc = (XenonCmpFunc)( (depthState.regRbDepthControl & 0x00000070) >> 4);
	abstractLayer->SetDepthFunc( depthFunc );

	// A2XX_RB_DEPTHCONTROL_STENCIL_ENABLE
	const bool bStencilEnabled = (0 != (depthState.regRbDepthControl & 0x00000001));
	abstractLayer->SetStencilTest( bStencilEnabled );

	// RB_STENCILREFMASK_STENCILREF
	const uint32 stencilRef = (depthState.regRbStencilRefMask & 0x000000FF);
	abstractLayer->SetStencilRef( (const uint8) stencilRef );

	// RB_STENCILREFMASK_STENCILMASK
	const uint32 stencilReadMask = (depthState.regRbStencilRefMask & 0x0000FF00) >> 8;
	abstractLayer->SetStencilReadMask( (const uint8) stencilReadMask );

	// RB_STENCILREFMASK_STENCILWRITEMASK
	const uint32 stencilWriteMask = (depthState.regRbStencilRefMask & 0x00FF0000) >> 16;
	abstractLayer->SetStencilWriteMask( (const uint8) stencilWriteMask );

	// A2XX_RB_DEPTHCONTROL_BACKFACE_ENABLE
	const bool bBackfaceEnabled = (depthState.regRbDepthControl & 0x00000080) != 0;
	if ( bBackfaceEnabled )
	{
		// A2XX_RB_DEPTHCONTROL_STENCILFUNC
		const auto frontFunc = (XenonCmpFunc)( (depthState.regRbDepthControl & 0x00000700) >> 8 );
		abstractLayer->SetStencilFunc( true, frontFunc );

		// A2XX_RB_DEPTHCONTROL_STENCILFAIL
		// A2XX_RB_DEPTHCONTROL_STENCILZFAIL
		// A2XX_RB_DEPTHCONTROL_STENCILZPASS
		const auto frontSfail = (XenonStencilOp)( (depthState.regRbDepthControl & 0x00003800) >> 11 );
		const auto frontDfail = (XenonStencilOp)( (depthState.regRbDepthControl & 0x000E0000) >> 17 );
		const auto frontDpass = (XenonStencilOp)( (depthState.regRbDepthControl & 0x0001C000) >> 14 );
		abstractLayer->SetStencilOps( true, frontSfail, frontDfail, frontDpass );

		// A2XX_RB_DEPTHCONTROL_STENCILFUNC_BF
		const auto backFunc = (XenonCmpFunc)( (depthState.regRbDepthControl & 0x00700000) >> 20 );
		abstractLayer->SetStencilFunc( false, backFunc );

		// A2XX_RB_DEPTHCONTROL_STENCILFAIL_BF
		// A2XX_RB_DEPTHCONTROL_STENCILZFAIL_BF
		// A2XX_RB_DEPTHCONTROL_STENCILZPASS_BF
		const auto backSfail = (XenonStencilOp)( (depthState.regRbDepthControl & 0x03800000) >> 23 );
		const auto backDfail = (XenonStencilOp)( (depthState.regRbDepthControl & 0xE0000000) >> 29 );
		const auto backDpass = (XenonStencilOp)( (depthState.regRbDepthControl & 0x1C000000) >> 26 );
		abstractLayer->SetStencilOps( false, backSfail, backDfail, backDpass );
	}
	else
	{
		// A2XX_RB_DEPTHCONTROL_STENCILFUNC
		const auto frontFunc = (XenonCmpFunc)( (depthState.regRbDepthControl & 0x00000700) >> 8 );
		abstractLayer->SetStencilFunc( true, frontFunc );
		abstractLayer->SetStencilFunc( false, frontFunc );

		// A2XX_RB_DEPTHCONTROL_STENCILFAIL
		// A2XX_RB_DEPTHCONTROL_STENCILZFAIL
		// A2XX_RB_DEPTHCONTROL_STENCILZPASS
		const auto frontSfail = (XenonStencilOp)( (depthState.regRbDepthControl & 0x00003800) >> 11 );
		const auto frontDfail = (XenonStencilOp)( (depthState.regRbDepthControl & 0x000E0000) >> 17 );
		const auto frontDpass = (XenonStencilOp)( (depthState.regRbDepthControl & 0x0001C000) >> 14 );
		abstractLayer->SetStencilOps( true, frontSfail, frontDfail, frontDpass );
		abstractLayer->SetStencilOps( false, frontSfail, frontDfail, frontDpass );
	}

	// realize state
	return abstractLayer->RealizeDepthStencilState();
}

bool CXenonGPUState::UpdateShaderConstants( IXenonGPUAbstractLayer* abstractLayer, const CXenonGPURegisters& regs, const CXenonGPUDirtyRegisterTracker& dirtyRegs )
{
	// pixel shader constants
	{
		const uint32 firstReg = (uint32) XenonGPURegister::REG_SHADER_CONSTANT_256_X;
		const uint32 lastReg  = (uint32) XenonGPURegister::REG_SHADER_CONSTANT_511_W;

		uint32 regIndex = firstReg;
		while ( regIndex < lastReg )
		{
			const uint32 baseShaderVector = (regIndex - firstReg) / 4;

			const uint64 dirtyMask = dirtyRegs.GetBlock( regIndex ); // 64 registers
			if ( dirtyMask )
			{
				const float* values = &regs[ regIndex ].m_float;
				abstractLayer->SetPixelShaderConsts( baseShaderVector, 16, values );
			}

			regIndex += 64;
		}
	}

	// vertex shader constants
	{
		const uint32 firstReg = (uint32) XenonGPURegister::REG_SHADER_CONSTANT_000_X;
		const uint32 lastReg  = (uint32) XenonGPURegister::REG_SHADER_CONSTANT_255_W;

		uint32 regIndex = firstReg;
		while ( regIndex < lastReg )
		{
			const uint32 baseShaderVector = (regIndex - firstReg) / 4;

			const uint64 dirtyMask = dirtyRegs.GetBlock( regIndex ); // 64 registers
			if ( dirtyMask )
			{
				const float* values = &regs[ regIndex ].m_float;
				abstractLayer->SetVertexShaderConsts( baseShaderVector, 16, values );
			}

			regIndex += 64;
		}
	}

	// boolean constants
	{
		const uint32 firstReg = (uint32) XenonGPURegister::REG_SHADER_CONSTANT_BOOL_000_031;
		const uint64 dirtyMask = dirtyRegs.GetBlock( firstReg );

		if ( dirtyMask & 0xFF ) // 8 regs total
		{
			const uint32* values = &regs[ firstReg ].m_dword;
			abstractLayer->SetBooleanConstants( values );
		}
	}

	// realize state
	return abstractLayer->RealizeShaderConstants();
}

bool CXenonGPUState::UpdateTexturesAndSamplers( IXenonGPUAbstractLayer* abstractLayer, const CXenonGPURegisters& regs, class IXenonGPUDumpWriter* traceDump )
{
	// get the "hot" fetch slots from the bounded shaders
	// we technically could flush ALL of the textures & samples but that's just a waste
	// this is one of the places when leaking some abstraction actually helps...
	const uint32 activeFetchSlotsMask = abstractLayer->GetActiveTextureFetchSlotMask();
	for ( uint32 fetchSlot=0; fetchSlot<32; ++fetchSlot )
	{
		// skip if slot is not active for current shaders
		if ( !(activeFetchSlotsMask & (1 << fetchSlot) ) )
			continue;

		// get texture information from the registers
		{
			const auto fetchReg = (XenonGPURegister)( (uint32)XenonGPURegister::REG_SHADER_CONSTANT_FETCH_00_0 + ((fetchSlot & 15) *6) );
			const auto& fetchInfo = regs.GetStructAt< XenonGPUTextureFetch >( fetchReg );
			
			// parse out texture format info from the GPU structure
			XenonTextureInfo textureInfo;
			if ( !XenonTextureInfo::Parse( fetchInfo, textureInfo ) )
			{
				GLog.Err( "GPU: Failed to parse texture info for fetch slot %d", fetchSlot );

				// bind NULL texture (default)
				abstractLayer->SetTexture( fetchSlot, nullptr );
				continue;
			}

			// set sampler
			auto samplerInfo = XenonSamplerInfo::Parse(fetchInfo);
			abstractLayer->SetSampler(fetchSlot, &samplerInfo);

			// dump texture
			if (traceDump != nullptr)
			{
				const uint32 realAddress = GPlatform.GetMemory().TranslatePhysicalAddress( textureInfo.m_address );
				const uint32 realMemortySize = textureInfo.CalculateMemoryRegionSize();
				traceDump->MemoryAccessRead( realAddress, realMemortySize, "Texture" );
			}

			// set actual texture to the pipeline at given fetch slot
			abstractLayer->SetTexture( fetchSlot, &textureInfo );
		}
	}

	// valid
	return true;
}

