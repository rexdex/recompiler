using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace xenonGPUViewer
{
    public class GPUStateCaptureViewport
    {
        [StructLayout(LayoutKind.Explicit)]
        private struct IntFloat
        {
            [FieldOffset(0)]
            public int IntValue;
            [FieldOffset(0)]
            public UInt32 UintValue;
            [FieldOffset(0)]
            public float FloatValue;
        }

        private Single ToFloat( UInt32 x )
        {
            var intFloat = new IntFloat { UintValue = x };
            return intFloat.FloatValue;
        }

        public GPUStateCaptureViewport(GPUStateCapture captureFrom)
        {
            var regSurfaceInfo = captureFrom.Reg(GPURegisterName.RB_SURFACE_INFO);
            var regPaClVteCntl = captureFrom.Reg(GPURegisterName.PA_CL_VTE_CNTL);
            var regPaSuScModeCntl = captureFrom.Reg(GPURegisterName.PA_SU_SC_MODE_CNTL);
            var regPaScWindowOffset = captureFrom.Reg(GPURegisterName.PA_SC_WINDOW_OFFSET);
            var regPaScWindowScissorTL = captureFrom.Reg(GPURegisterName.PA_SC_WINDOW_SCISSOR_TL);
            var regPaScWindowScissorBR = captureFrom.Reg(GPURegisterName.PA_SC_WINDOW_SCISSOR_BR);
            var regPaClVportXoffset = captureFrom.Reg(GPURegisterName.PA_CL_VPORT_XOFFSET);
            var regPaClVportYoffset = captureFrom.Reg(GPURegisterName.PA_CL_VPORT_YOFFSET);
            var regPaClVportZoffset = captureFrom.Reg(GPURegisterName.PA_CL_VPORT_ZOFFSET);
            var regPaClVportXscale = captureFrom.Reg(GPURegisterName.PA_CL_VPORT_XSCALE);
            var regPaClVportYscale = captureFrom.Reg(GPURegisterName.PA_CL_VPORT_YSCALE);
            var regPaClVportZscale = captureFrom.Reg(GPURegisterName.PA_CL_VPORT_ZSCALE);

            {
                ViewportXYDivided = (0 != ((regPaClVteCntl >> 8) & 1));
                ViewportZDivided = 0 != ((regPaClVteCntl >> 9) & 1);
                ViewportWNotDivided = 0 != ((regPaClVteCntl >> 10) & 1);

                NormalizedCoordinates = (0 != (regPaClVteCntl & 1));
            }

            {
                WindowOffsetX = 0;
                WindowOffsetY = 0;
                if (0 != ((regPaSuScModeCntl >> 16) & 1))
                {
                    // extract 15-bit signes values
                    UInt16 ofsX = (UInt16)(regPaScWindowOffset & 0x7FFF);
                    UInt16 ofsY = (UInt16)((regPaScWindowOffset >> 16) & 0x7FFF);

                    if (0 != (ofsX & 0x4000))
                        ofsX |= 0x8000;
                    if (0 != (ofsY & 0x4000))
                        ofsY |= 0x8000;

                    WindowOffsetX = (Int16)ofsX;
                    WindowOffsetY = (Int16)ofsY;
                }
            }

            {
                ScissorX1 = (int)(regPaScWindowScissorTL & 0x7FFF);
                ScissorY1 = (int)((regPaScWindowScissorTL >> 16) & 0x7FFF);
                ScissorX2 = (int)(regPaScWindowScissorBR & 0x7FFF);
                ScissorY2 = (int)((regPaScWindowScissorBR >> 16) & 0x7FFF);
            }

            {
                XScaleEnabled = (regPaClVteCntl & (1 << 0)) != 0;
                XOffsetEnabled = (regPaClVteCntl & (1 << 1)) != 0;
                YScaleEnabled = (regPaClVteCntl & (1 << 2)) != 0;
                YOffsetEnabled = (regPaClVteCntl & (1 << 3)) != 0;
                ZScaleEnabled = (regPaClVteCntl & (1 << 4)) != 0;
                ZOffsetEnabled = (regPaClVteCntl & (1 << 5)) != 0;

                XScale = XScaleEnabled ? ToFloat(regPaClVportXscale) : 0.0f;
                YScale = YScaleEnabled ? ToFloat(regPaClVportYscale) : 0.0f;
                ZScale = ZScaleEnabled ? ToFloat(regPaClVportZscale) : 0.0f;
                XOffset = XOffsetEnabled ? ToFloat(regPaClVportXoffset) : 0.0f;
                YOffset = YOffsetEnabled ? ToFloat(regPaClVportYoffset) : 0.0f;
                ZOffset = ZOffsetEnabled ? ToFloat(regPaClVportZoffset) : 0.0f;
            }
        }

        public Boolean ViewportXYDivided;
        public Boolean ViewportZDivided;
        public Boolean ViewportWNotDivided;
        public Boolean NormalizedCoordinates;

        public Int32 WindowOffsetX;
        public Int32 WindowOffsetY;

        public Int32 ScissorX1;
        public Int32 ScissorY1;
        public Int32 ScissorX2;
        public Int32 ScissorY2;

        public Boolean XScaleEnabled;
        public Boolean YScaleEnabled;
        public Boolean ZScaleEnabled;
        public Boolean XOffsetEnabled;
        public Boolean YOffsetEnabled;
        public Boolean ZOffsetEnabled;

        public Single XOffset;
        public Single YOffset;
        public Single ZOffset;
        public Single XScale;
        public Single YScale;
        public Single ZScale;

        public void OutputInfo(ref string txt)
        {
            txt += "<h2>Viewport state</h2><br>";
            txt += "<p>";
            txt += "ViewportXYDivided: " + ViewportXYDivided.ToString() + "<br>";
            txt += "ViewportZDivided: " + ViewportZDivided.ToString() + "<br>";
            txt += "NormalizedCoordinates: " + NormalizedCoordinates.ToString() + "<br>";
            txt += "<br>";

            txt += "WindowsOffset: [" + WindowOffsetX.ToString() + "," + WindowOffsetY.ToString() + "]<br>";
            txt += "<br>";

            txt += "Scissor: [" + ScissorX1.ToString() + "," + ScissorY1.ToString() + "] to [" + ScissorX2.ToString() + "," + ScissorY2.ToString() + "]<br>";
            txt += "<br>";

            if ( XScaleEnabled || YScaleEnabled || ZScaleEnabled || XOffsetEnabled || YOffsetEnabled || ZOffsetEnabled )
            {
                if ( XScaleEnabled ) txt += "XScale: " + XScale.ToString() + "<br>";
                if ( YScaleEnabled ) txt += "YScale: " + YScale.ToString() + "<br>";
                if ( YScaleEnabled ) txt += "ZScale: " + ZScale.ToString() + "<br>";
                if ( XOffsetEnabled ) txt += "XOffset: " + XOffset.ToString() + "<br>";
                if ( YOffsetEnabled ) txt += "YOffset: " + YOffset.ToString() + "<br>";
                if ( ZOffsetEnabled) txt += "ZOffset: " + ZOffset.ToString() + "<br>";
            }
            txt += "</p>";
        }

        public bool Compatible(GPUStateCaptureViewport other)
        {
            if (ScissorX1 != other.ScissorX1) return false;
            if (ScissorY1 != other.ScissorY1) return false;
            if (ScissorX2 != other.ScissorX2) return false;
            if (ScissorY2 != other.ScissorY2) return false;

            if (WindowOffsetX != other.WindowOffsetX) return false;
            if (WindowOffsetY != other.WindowOffsetY) return false;

            if (ViewportXYDivided != other.ViewportXYDivided) return false;
            if (ViewportZDivided != other.ViewportZDivided) return false;
            if (ViewportWNotDivided != other.ViewportWNotDivided) return false;
            if (NormalizedCoordinates != other.NormalizedCoordinates) return false;

            if (XOffset != other.XOffset) return false;
            if (YOffset != other.YOffset) return false;
            if (ZOffset != other.ZOffset) return false;
            if (XScale != other.XScale) return false;
            if (YScale != other.YScale) return false;
            if (ZScale != other.ZScale) return false;

            return true;
        }
    }

    public class GPUStateRenderTargets
    {
        public struct ColorState
        {
            public Boolean Enabled;
            public UInt32 EdramBase;
            public Boolean WriteRed;
            public Boolean WriteGreen;
            public Boolean WriteBlue;
            public Boolean WriteAlpha;
            public GPUColorRenderTargetFormat Format;

                 public bool Compatible(ColorState other)
            {
                if (Enabled != other.Enabled) return false;
                if (EdramBase != other.EdramBase) return false;
                if (Format != other.Format) return false;

                return true;
            }
        }

        public struct DepthState
        {
            public Boolean Enabled;
            public UInt32 EdramBase;
            public GPUDepthRenderTargetFormat Format;

            public bool Compatible(DepthState other)
            {
                if (Enabled != other.Enabled) return false;
                if (EdramBase != other.EdramBase) return false;
                if (Format != other.Format) return false;

                return true;
            }
        }

        public UInt32           SurfacePitch;
        public GPUMsaaSamples   MSAA;
        public ColorState[]     Color;
        public DepthState       Depth;

        public GPUStateRenderTargets( GPUStateCapture state )
        {
            var regModeControl = state.Reg( GPURegisterName.RB_MODECONTROL );
            var regSurfaceInfo = state.Reg( GPURegisterName.RB_SURFACE_INFO );
            var regColor0Info = state.Reg( GPURegisterName.RB_COLOR_INFO );
            var regColor1Info = state.Reg( GPURegisterName.RB_COLOR1_INFO );
            var regColor2Info = state.Reg( GPURegisterName.RB_COLOR2_INFO );
            var regColor3Info = state.Reg( GPURegisterName.RB_COLOR3_INFO );
            var regColorMask = state.Reg( GPURegisterName.RB_COLOR_MASK );
            var regDepthInfo = state.Reg( GPURegisterName.RB_DEPTH_INFO );
            var regDepthControl = state.Reg( GPURegisterName.RB_DEPTHCONTROL );
            var regStencilRefMask = state.Reg( GPURegisterName.RB_STENCILREFMASK );

            // read standard stuff
	        MSAA = (GPUMsaaSamples)( (regSurfaceInfo >> 16) & 3 );
	        SurfacePitch = regSurfaceInfo & 0x3FFF;

            // reset color/depth state
            Color = new ColorState[4];
            Depth = new DepthState();

            // read surface settings
            var enableMode = (GPUModeControl)( regModeControl & 0x7 );
            if ( enableMode == GPUModeControl.ColorDepth )
	        {
                UInt32[] regColor = { regColor0Info, regColor1Info, regColor2Info, regColor3Info };

		        for ( int rtIndex=0; rtIndex<4; ++rtIndex )
		        {
			        var rtInfo = regColor[ rtIndex ];

			        // get color mask for this specific render target
			        var writeMask = (regColorMask >> (rtIndex * 4)) & 0xF;
			        if ( writeMask != 0 )
			        {
                        ColorState colorInfo = new ColorState();
                        colorInfo.WriteRed = (writeMask & 1) != 0;
                        colorInfo.WriteGreen = (writeMask & 2) != 0;
                        colorInfo.WriteBlue = (writeMask & 4) != 0;
                        colorInfo.WriteAlpha = (writeMask & 8) != 0;
                        colorInfo.Enabled = true;
                        colorInfo.EdramBase = (rtInfo & 0xFFF); // tile address
                        colorInfo.Format = (GPUColorRenderTargetFormat)( (rtInfo >> 16) & 0xF );
                        Color[rtIndex] = colorInfo;
                    }
                }
            }

            // depth
            {
                var usesDepth = (0 != (regDepthControl & 0x00000002)) || (0 != (regDepthControl & 0x00000004));
	            var stencilWriteMask = (regStencilRefMask & 0x00FF0000) >> 16;
	            var usesStencil = (0 != (regDepthControl & 0x00000001)) || (stencilWriteMask != 0);

	            if ( usesDepth || usesStencil )
	            {
                    DepthState depthState = new DepthState();
                    depthState.Enabled = true;
                    depthState.EdramBase = (regDepthInfo & 0xFFF);
                    depthState.Format = (GPUDepthRenderTargetFormat)( (regDepthInfo >> 16) & 1 );
                    Depth = depthState;
                }
            }
        }

        public void OutputInfo(ref string txt)
        {
            txt += "<h2>Render target state</h2><br>";
            txt += "<p>";
            txt += "SurfacePitch: " + SurfacePitch.ToString() + "<br>";
            txt += "MSAA: " + Enum.GetName( typeof(GPUMsaaSamples), MSAA ) + "<br>";
            txt += "<br>";

            for (int i = 0; i < 4; ++i)
            {
                var info = Color[i];
                txt += "Color" + i.ToString() + ": " + (info.Enabled ? "Enabled" : "Disabled") + "<br>";
                if (info.Enabled)
                {
                    txt += String.Format( "EDRAM: 0x{0:X4}<br>", info.EdramBase );
                    txt += "Format: " + Enum.GetName(typeof(GPUColorRenderTargetFormat), info.Format) + "<br>";
                    txt += "WriteRed: " + info.WriteRed.ToString() + "<br>";
                    txt += "WriteGreen: " + info.WriteGreen.ToString() + "<br>";
                    txt += "WRiteBlue: " + info.WriteBlue.ToString() + "<br>";
                    txt += "WriteAlpha: " + info.WriteAlpha.ToString() + "<br>";
                }

                txt += "<br>";
            }

            {
                var info = Depth;
                txt += "Depth: " + (info.Enabled ? "Enabled" : "Disabled") + "<br>";
                if (info.Enabled)
                {
                    txt += String.Format("EDRAM: 0x{0:X4}<br>", info.EdramBase);
                    txt += "Format: " + Enum.GetName(typeof(GPUDepthRenderTargetFormat), info.Format) + "<br>";
                }

                txt += "<br>";
            }

            txt += "</p>";
        }

        public bool Compatible(GPUStateRenderTargets other)
        {
            if (MSAA != other.MSAA) return false;
            if (SurfacePitch != other.SurfacePitch) return false;

            if (!Color[0].Compatible( other.Color[0] ) ) return false;
            if (!Color[1].Compatible( other.Color[1] ) ) return false;
            if (!Color[2].Compatible( other.Color[2] ) ) return false;
            if (!Color[3].Compatible( other.Color[3] ) ) return false;
            if (!Depth.Compatible( Depth ) ) return false;

            return true;
        }
    }

    public class GPUStateCapture
    {
        private UInt32[] _RegValues;              // Current values for the registers
        private GPUShader _PixelShader;         // Last loaded Pixel Shader
        private GPUShader _VertexShader;        // Last loaded Vertex Shader

        public GPUShader PixelShader { get { return _PixelShader; } }
        public GPUShader VertexShader { get { return _VertexShader; } }
        public UInt32 Reg(UInt32 index) { return _RegValues[index];  }
        public UInt32 Reg(GPURegisterName index) { return _RegValues[(UInt32)index]; }

        public GPUStateCapture(GPUState captureFrom)
        {
            _RegValues = new UInt32[captureFrom.RegValues.Length];
            for (UInt32 i = 0; i < _RegValues.Length; ++i)
                _RegValues[i] = captureFrom.RegValues[i];

            _PixelShader = captureFrom.PixelShader;
            _VertexShader = captureFrom.VertexShader;
        }

    }
}
