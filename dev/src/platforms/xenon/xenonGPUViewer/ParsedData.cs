using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace xenonGPUViewer
{

    public class ParsedData
    {
        static public ParsedData Parse(RawDumpData raw)
        {
            ParsedData ret = new ParsedData();

            var packetsMap = new Dictionary<RawPacket, ParsedPacket>();

            // create drawcall list
            ret._ShaderCache = new GPUShaderCache();
            ret._DrawCalls = new List<ParsedDrawCall>();
            ret._DrawGroups = new List<ParsedDrawGroup>();

            // prepare GPU state for the duration of parsing
            GPUState parseState = new GPUState(ret._ShaderCache);
            GPUExecutor parseExec = new GPUExecutor(parseState);

            // parse all packets
            int packedIndex = 1;
            int drawCallIndex = 1;
            int drawGroupIndex = 1;
            ParsedDrawCall drawCall = null;
            ParsedDrawGroup drawGroup = null;
            foreach (var rawPacket in raw.Packets)
            {
                // start new drawcall
                if (drawCall == null)
                {
                    drawCall = new ParsedDrawCall( drawCallIndex );
                    ret._DrawCalls.Add(drawCall);
                    drawCallIndex += 1;
                }

                // execute packet
                GPUCommandOutput executeOutput = new GPUCommandOutput();
                var executionResult = parseExec.ExecutePacket(rawPacket, executeOutput);
                if (executionResult != GPUExecutorResult.Invalid)
                {
                    // add data
                    packetsMap[rawPacket] = ParsedPacket.Parse(rawPacket, packedIndex, drawCall, executeOutput);
                    ++packedIndex;
                }

                // restart after each drawcall
                if (packetsMap[rawPacket].Output.IsDraw())
                {
                    // capture final state at each drawcall
                    if (drawCall != null)
                    {
                        drawCall.CapturedState = new GPUStateCapture(parseState);

                        // extract the viewport/render target settings for the draw call - required to match it to the draw group
                        var stateRenderTargets = new GPUStateRenderTargets(drawCall.CapturedState);
                        var stateViewport = new GPUStateCaptureViewport(drawCall.CapturedState);

                        // determine if we should add this draw call to current draw group
                        if ((drawGroup != null) && drawGroup.Compatible(stateViewport, stateRenderTargets))
                        {
                            drawGroup.AddDrawCall(drawCall);
                        }
                        else
                        {
                            drawGroup = new ParsedDrawGroup(drawGroupIndex++, stateViewport, stateRenderTargets);
                            drawGroup.AddDrawCall(drawCall);
                            ret._DrawGroups.Add(drawGroup);
                        }
                    }

                    // reset
                    drawCall = null;
                }
            }

            return ret;
        }

        public List<ParsedDrawGroup> DrawGroups { get { return _DrawGroups; } }
        public List<ParsedDrawCall> DrawCalls { get { return _DrawCalls; } }

        private List<ParsedDrawGroup> _DrawGroups;
        private List<ParsedDrawCall> _DrawCalls;
        private GPUShaderCache _ShaderCache;
    }

    public class ParsedDrawGroup
    {
        private int _Index;    
        private GPUStateCaptureViewport _Viewport;
        private GPUStateRenderTargets _RenderTargets;
        private List<ParsedDrawCall> _DrawCalls;

        private int Index { get { return _Index; } }
        public List<ParsedDrawCall> DrawCalls { get { return _DrawCalls; } }
        private GPUStateCaptureViewport Viewport { get { return _Viewport; } }
        private GPUStateRenderTargets RenderTargets { get { return _RenderTargets; } }

        public ParsedDrawGroup(int index, GPUStateCaptureViewport viewport, GPUStateRenderTargets renderTargets)
        {
            _Index = index;
            _Viewport = viewport;
            _RenderTargets = renderTargets;
            _DrawCalls = new List<ParsedDrawCall>();
        }

        public override string ToString()
        {
            string txt = "";

            int width = _Viewport.ScissorX2 - _Viewport.ScissorX1;
            int height = _Viewport.ScissorY2 - _Viewport.ScissorY1;
            txt += String.Format("[{0}x{1}] ", width, height);

            if (_RenderTargets.Color[0].Enabled)
                txt += "COLOR0 ";

            if (_RenderTargets.Color[1].Enabled)
                txt += "COLOR1 ";

            if (_RenderTargets.Color[2].Enabled)
                txt += "COLOR2 ";

            if (_RenderTargets.Color[3].Enabled)
                txt += "COLOR3 ";

            if (_RenderTargets.Depth.Enabled)
                txt += "DEPTH ";

            txt += "[";
            txt += _DrawCalls.Count.ToString();
            txt += "]";

            return txt;
        }

        public void AddDrawCall(ParsedDrawCall dc)
        {
            _DrawCalls.Add(dc);
        }

        public bool Compatible(GPUStateCaptureViewport viewport, GPUStateRenderTargets renderTargets)
        {
            return _Viewport.Compatible(viewport) && _RenderTargets.Compatible(renderTargets);
        }
    }

    public class ParsedDrawCall
    {
        private int _Index;
        private List<ParsedPacket> _Packets;

        public List<ParsedPacket> Packets { get { return _Packets; } }
        public GPUStateCapture CapturedState;

        public ParsedDrawCall(int index)
        {
            _Index = index;
            _Packets = new List<ParsedPacket>();
        }

        public override String ToString()
        {
            string ret = "";

            ret += String.Format("[{0}] ({1}-{2}): ",
                _Index, _Packets.First().Index, _Packets.Last().Index);

            var last = _Packets.Last();
            ret += last.Output._Desc;

            return ret;
        }

        public void Linkup(ParsedPacket packet)
        {
            _Packets.Add(packet);
        }
    }

    public class ParsedPacket
    {
        public RawPacket Raw { get { return _Raw; } }
        public ParsedDrawCall Drawcall { get { return _DrawCall; } }
        public GPUCommandOutput Output { get { return _CmdOutput; } }
        public int Index { get { return _Index; } }

        private GPUCommandOutput _CmdOutput;
        private RawPacket _Raw;
        private ParsedDrawCall _DrawCall;
        private int _Index;

        public static ParsedPacket Parse( RawPacket raw, int packetIndex, ParsedDrawCall drawcall, GPUCommandOutput commandOutput )
        {
            ParsedPacket ret = new ParsedPacket();
            ret._Raw = raw;
            ret._Index = packetIndex;
            ret._DrawCall = drawcall;
            ret._CmdOutput = commandOutput;

            if (drawcall != null )
                drawcall.Linkup(ret);
  

            return ret;
        }

        public override String ToString()
        {
            string ret = "";

            if (_CmdOutput != null)
            {
                ret = _CmdOutput._Desc;
            }

            return ret;
        }
    }

}
