using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;

namespace xenonGPUViewer
{
    public enum GPUExecutorResult
    {
        Executed,
        Skipped,
        Invalid,
    }

    public class GPUCommandOutput
    {
        public List<String> _Log;
        public String _Desc;
        public GPUPacketType _PacketType;
        public bool _DumpInDrawCall;
        public List<GPUTextureFetchData> _Textures; // to expensive to re-get

        public bool IsDraw()
        {
            return (_PacketType == GPUPacketType.PM4_DRAW_INDX ||
                _PacketType == GPUPacketType.PM4_DRAW_INDX_2 ||
                _PacketType == GPUPacketType.PM4_DRAW_INDX_2_BIN ||
                _PacketType == GPUPacketType.PM4_DRAW_INDX_BIN ||
                _PacketType == GPUPacketType.PM4_HACK_SWAP);
        }

        public GPUCommandOutput()
        {
            _Log = new List<String>();
            _PacketType = GPUPacketType.INVALID;
            _Textures = new List<GPUTextureFetchData>();
            _DumpInDrawCall = false;
            _Desc = "Packet";
        }

        public void Log(String s)
        {
            _Log.Add(s);
        }
    }

    public class GPUExecutor
    {
        public GPUExecutor(GPUState state)
        {
            _State = state;

            // tiled rendering handling :)
            _TileMask = 0xFFFFFFFF;
            _TileSelector = 0xFFFFFFFF;
        }

        // advance the GPU state by executing the packet - NOTE, some packets don't change the regs
        public GPUExecutorResult ExecutePacket(RawPacket _packet, GPUCommandOutput _out)
        {
            var packetType = _packet.Data >> 30;

            switch (packetType)
            {
                case 0x00: return ExecutePacketType0(_packet, _out);
                case 0x01: return ExecutePacketType1(_packet, _out);
                case 0x02: return ExecutePacketType2(_packet, _out);
                case 0x03: return ExecutePacketType3(_packet, _out);
            }

            return GPUExecutorResult.Invalid;
        }        	

        [StructLayout(LayoutKind.Explicit)]
        struct FloatUint32
        {
            [FieldOffset(0)]
            public UInt32 DWord;
            [FieldOffset(0)]
            public float Float;
        }

        public static float ToFloat( UInt32 var )
        {
            FloatUint32 str;
            str.Float = 0;
            str.DWord = var;
            return str.Float;
        }

        /*
         * 
            if (last.Output._PacketType.Type == GPUPacketType.PM4_DRAW_INDX_2)
            {
                var dword0 = last.Raw.Words[0];
                var indexCount = dword0 >> 16;
                var primitiveType = (GPUPrimitiveType)(dword0 & 0x3F);

                ret += "Draw2 " + Enum.GetName(typeof(GPUPrimitiveType), primitiveType) + " (Count: " + indexCount.ToString() + ")";
            }
            else if (last._PacketType.Type == GPUPacketType.PM4_DRAW_INDX)
            {
                var dword0 = last.Raw.Words[0];
                var dword1 = last.Raw.Words[1];

                var indexCount = dword1 >> 16;
                var primitiveType = (GPUPrimitiveType)(dword1 & 0x3F);
                var source = (dword1 >> 6) & 0x3;

                if (source == 0x0)
                {
                    bool is32Bit = 0 != ((dword1 >> 11) & 0x1);
                    ret += (is32Bit ? "DrawIndexed32 " : "DrawIndexed16 ") + Enum.GetName(typeof(GPUPrimitiveType), primitiveType) + " (Count: " + indexCount.ToString() + ")";
                }
                else if (source == 0x2)
                {
                    ret += "Draw " + Enum.GetName(typeof(GPUPrimitiveType), primitiveType) + " (Count: " + indexCount.ToString() + ")";
                }
            }
            else if (last.Type == GPUPacketType.PM4_HACK_SWAP)
            {
                return "SWAP";
            }
        */

        private GPUExecutorResult ExecutePacketType0(RawPacket _packet, GPUCommandOutput _out)
        {
            var packetData = _packet.Data;
            var regCount = ((packetData >> 16) & 0x3FFF) + 1;
	        var baseIndex = (packetData & 0x7FFF);
            var writeOneReg = 0 != ((packetData >> 15) & 0x1);

        	for ( UInt32 i = 0; i<regCount; i++ )
	        {
		        UInt32 regData = _packet.Words[i];
		        UInt32 regIndex = writeOneReg ? baseIndex : baseIndex + i;
		        WriteRegister( regIndex, regData, _out );
	        }

            if (_out != null)
            {
                if (writeOneReg)
                    _out._Desc = String.Format("SetRegs [Start: {0}, Count: {1}]", baseIndex, regCount);
                else
                    _out._Desc = String.Format("SetRegs [At: {0}, Times: {1}]", baseIndex, regCount);

                _out._PacketType = GPUPacketType.PM1_REGS;
            }

            return GPUExecutorResult.Executed;
        }

        private GPUExecutorResult ExecutePacketType1(RawPacket _packet, GPUCommandOutput _out)
        {
            var packetData = _packet.Data;
        	var regIndexA = packetData & 0x7FF;
	        var regIndexB = (packetData >> 11) & 0x7FF;

	        var regDataA = _packet.Words[0];
            var regDataB = _packet.Words[1];

        	WriteRegister( regIndexA, regDataA, _out );
	        WriteRegister( regIndexB, regDataB, _out );

            if (_out != null)
            {
                _out._Desc = String.Format("SetReg2 [RegA: {0}, RegB: {1}]", regIndexA, regIndexB);
                _out._PacketType = GPUPacketType.PM2_REGS;
            }

	        return GPUExecutorResult.Executed;
        }

        private GPUExecutorResult ExecutePacketType2(RawPacket _packet, GPUCommandOutput _out)
        {
            if (_out != null)
            {
                _out._PacketType = GPUPacketType.PM3_NOP;
            }

            return GPUExecutorResult.Skipped;
        }

        private GPUExecutorResult ExecutePacketType3(RawPacket _packet, GPUCommandOutput _out)
        {
            var packetData = _packet.Data;
        	var opcode = (packetData >> 8) & 0x7F;
	        var count = ((packetData >> 16) & 0x3FFF) + 1;

            if (_out != null)
            {
                _out._PacketType = (GPUPacketType)opcode;
                _out._Desc = Enum.GetName(typeof(GPUPacketType), opcode);
            }

    	    // in predicated mode we may need to skip this command if the tile mask does not match tile selector
	        if ( (packetData & 1) != 0 )
	        {
		        // skip if we are NOT the target
		        bool shouldSkip = (_TileMask & _TileSelector) == 0;
		        if ( shouldSkip ) 
                    return GPUExecutorResult.Skipped;
            }

        	// process opcode
	        switch ( (GPUPacketType)opcode )
	        {
		        case GPUPacketType.PM4_ME_INIT:
                    return ExecutePacketType3_ME_INIT(_packet, packetData, count, _out);

		        case GPUPacketType.PM4_NOP:
                    return ExecutePacketType3_NOP(_packet, packetData, count, _out);

		        case GPUPacketType.PM4_INTERRUPT:
                    return ExecutePacketType3_INTERRUPT(_packet, packetData, count, _out);

		        case GPUPacketType.PM4_HACK_SWAP:
                    return ExecutePacketType3_HACK_SWAP(_packet, packetData, count, _out);

		        case GPUPacketType.PM4_WAIT_REG_MEM:
                    return ExecutePacketType3_WAIT_REG_MEM(_packet, packetData, count, _out);

		        case GPUPacketType.PM4_REG_RMW:
                    return ExecutePacketType3_REG_RMW(_packet, packetData, count, _out);

		        case GPUPacketType.PM4_COND_WRITE:
                    return ExecutePacketType3_COND_WRITE(_packet, packetData, count, _out);

		        case GPUPacketType.PM4_EVENT_WRITE:
                    return ExecutePacketType3_EVENT_WRITE(_packet, packetData, count, _out);

		        case GPUPacketType.PM4_EVENT_WRITE_SHD:
                    return ExecutePacketType3_EVENT_WRITE_SHD(_packet, packetData, count, _out);

		        case GPUPacketType.PM4_EVENT_WRITE_EXT:
                    return ExecutePacketType3_EVENT_WRITE_EXT(_packet, packetData, count, _out);

		        case GPUPacketType.PM4_DRAW_INDX:
                    return ExecutePacketType3_DRAW_INDX(_packet, packetData, count, _out);

		        case GPUPacketType.PM4_DRAW_INDX_2:
                    return ExecutePacketType3_DRAW_INDX_2(_packet, packetData, count, _out);

		        case GPUPacketType.PM4_SET_CONSTANT:
                    return ExecutePacketType3_SET_CONSTANT(_packet, packetData, count, _out);

		        case GPUPacketType.PM4_SET_CONSTANT2:
                    return ExecutePacketType3_SET_CONSTANT2(_packet, packetData, count, _out);

		        case GPUPacketType.PM4_LOAD_ALU_CONSTANT:
                    return ExecutePacketType3_LOAD_ALU_CONSTANT(_packet, packetData, count, _out);

		        case GPUPacketType.PM4_SET_SHADER_CONSTANTS:
                    return ExecutePacketType3_SET_SHADER_CONSTANTS(_packet, packetData, count, _out);

		        case GPUPacketType.PM4_IM_LOAD:
                    return ExecutePacketType3_IM_LOAD(_packet, packetData, count, _out);

		        case GPUPacketType.PM4_IM_LOAD_IMMEDIATE:
                    return ExecutePacketType3_IM_LOAD_IMMEDIATE(_packet, packetData, count, _out);

		        case GPUPacketType.PM4_INVALIDATE_STATE:
                    return ExecutePacketType3_INVALIDATE_STATE(_packet, packetData, count, _out);

		        // tiled rendering bit mask
		        case GPUPacketType.PM4_SET_BIN_MASK_LO:
		        {
			        var loMask = _packet.Words[0];
			        _TileMask = (_TileMask & 0xFFFFFFFF00000000UL) | ((UInt64)loMask);
                    _out.Log(String.Format("LoMask = {0:X}", loMask));
                    return GPUExecutorResult.Executed;
		        }
		
		        case GPUPacketType.PM4_SET_BIN_MASK_HI:
		        {
			        var hiMask = _packet.Words[0];
			        _TileMask = (_TileMask & 0xFFFFFFFFUL) | (((UInt64)hiMask) << 32);
                    _out.Log(String.Format("HiMask = {0:X}", hiMask));
                    return GPUExecutorResult.Executed;
		        }

		        case GPUPacketType.PM4_SET_BIN_SELECT_LO:
		        {
	                var loSelect = _packet.Words[0];
			        _TileSelector = (_TileSelector & 0xFFFFFFFF00000000UL) | ((UInt64)loSelect);
                    _out.Log(String.Format("LoSelector = {0:X}", loSelect));
                    return GPUExecutorResult.Executed;
		        }

		        case GPUPacketType.PM4_SET_BIN_SELECT_HI:
		        {
			        var hiSelect = _packet.Words[0];
			        _TileSelector = (_TileSelector & 0xFFFFFFFFUL) | (((UInt64)hiSelect) << 32);
                    _out.Log(String.Format("HiSelector = {0:X}", hiSelect));
                    return GPUExecutorResult.Executed;
		        }

		        // Ignored packets - useful if breaking on the default handler below.
		        case (GPUPacketType)0x50:  // 0xC0015000 usually 2 words, 0xFFFFFFFF / 0x00000000
		        case (GPUPacketType)0x51:  // 0xC0015100 usually 2 words, 0xFFFFFFFF / 0xFFFFFFFF
		        {
                    return GPUExecutorResult.Skipped;
		        }
    	    }

               // invalid packet encountered
            return GPUExecutorResult.Invalid;
        }

        private GPUExecutorResult ExecutePacketType3_ME_INIT(RawPacket _packet, UInt32 packetData, UInt32 count, GPUCommandOutput _out)
        {
            // nothing happens in the replay
            return GPUExecutorResult.Executed;
        }

        private GPUExecutorResult ExecutePacketType3_NOP(RawPacket _packet, UInt32 packetData, UInt32 count, GPUCommandOutput _out)
        {
            // nothing happens in the replay
            return GPUExecutorResult.Executed;
        }

        private GPUExecutorResult ExecutePacketType3_INTERRUPT(RawPacket _packet, UInt32 packetData, UInt32 count, GPUCommandOutput _out)
        {
            // nothing happens in the replay
            return GPUExecutorResult.Executed;
        }

        private GPUExecutorResult ExecutePacketType3_HACK_SWAP(RawPacket _packet, UInt32 packetData, UInt32 count, GPUCommandOutput _out)
        {
            // nothing happens in the replay
            return GPUExecutorResult.Executed;
        }

		private GPUExecutorResult ExecutePacketType3_WAIT_REG_MEM(RawPacket _packet, UInt32 packetData, UInt32 count, GPUCommandOutput _out)
        {
            if (_out == null)
                return GPUExecutorResult.Executed;

            var waitInfo = _packet.Words[0];
            var pollRegAddr = _packet.Words[1];
            var refValue = _packet.Words[2];
            var mask = _packet.Words[3];
            var wait = _packet.Words[4];

            UInt32 incomingValue = 0;

            if ( 0 != (waitInfo & 0x10) )
		    {
                var memoryAddress = pollRegAddr & ~0x3;
                var memoryMode = pollRegAddr & 3;

                _out.Log( String.Format( "Memory address: 0x{0:X}", memoryAddress ) );
                _out.Log( String.Format( "Memory mode: {0}", memoryMode ) );

                var memBlock = _packet.FindMemoryReadBlock((UInt32)memoryAddress);
                if ( memBlock != null )
                {
                    incomingValue = memBlock.LoadUint32AtBE(0);
                }
			}
            else
            {
                _out.Log( String.Format( "Read register: {0}", Enum.GetName(typeof(GPURegisterName), pollRegAddr)));
                incomingValue = _State.RegValues[ pollRegAddr ];
            }

            _out.Log( String.Format( "Incoming value: 0x{0:X}", incomingValue) );
            _out.Log(String.Format("Incoming mask: 0x{0:X}", mask));
            _out.Log( String.Format( "Masked value: 0x{0:X}", incomingValue & mask ) );

            switch ( waitInfo & 0x7 )
		    {
			    case 0x0: _out.Log( "Condition: Never"); break;
                case 0x1: _out.Log(String.Format("Condition: 0x{0:X} < 0x{1:X}", incomingValue & mask, refValue)); break;
                case 0x2: _out.Log(String.Format("Condition: 0x{0:X} <= 0x{1:X}", incomingValue & mask, refValue)); break;
                case 0x3: _out.Log(String.Format("Condition: 0x{0:X} == 0x{1:X}", incomingValue & mask, refValue)); break;
                case 0x4: _out.Log(String.Format("Condition: 0x{0:X} != 0x{1:X}", incomingValue & mask, refValue)); break;
                case 0x5: _out.Log(String.Format("Condition: 0x{0:X} >= 0x{1:X}", incomingValue & mask, refValue)); break;
                case 0x6: _out.Log(String.Format("Condition: 0x{0:X} > 0x{1:X}", incomingValue & mask, refValue)); break;
                case 0x7: _out.Log("Condition: Always"); break;
            }

            return GPUExecutorResult.Executed;
        }

        private GPUExecutorResult ExecutePacketType3_REG_RMW(RawPacket _packet, UInt32 packetData, UInt32 count, GPUCommandOutput _out)
        {
            var rmwSetup = _packet.Words[0];
            var andMask = _packet.Words[0];
            var orMask = _packet.Words[0];

            // read value
            var regIndex = (rmwSetup & 0x1FFF);
            if (_out != null) _out.Log(String.Format("Register: {0} ({1})", Enum.GetName(typeof(GPURegisterName), regIndex), regIndex));
            var value = _State.RegValues[regIndex];
            if (_out != null) _out.Log(String.Format("Incoming value: 0x{0:X}", value));
            var oldValue = value;

            // OR value (with reg or immediate value)
            if (0 != ((rmwSetup >> 30) & 0x1))
            {
                // | reg
                if (_out != null) _out.Log(String.Format("Or Mask Register: {0} ({1})", Enum.GetName(typeof(GPURegisterName), orMask & 0x1FFF), orMask & 0x1FFF));
                var orValue = _State.RegValues[orMask & 0x1FFF];
                if (_out != null) _out.Log(String.Format("Or Mask Value: 0x{0:X}", orValue));
                value |= orValue;
            }
            else
            {
                // | imm
                if (_out != null) _out.Log(String.Format("Or Mask Value: 0x{0:X}", orMask));
                value |= orMask;
            }

            // AND value (with reg or immediate value)
            if (0 != ((rmwSetup >> 31) & 0x1))
            {
                // & reg
                if (_out != null) _out.Log(String.Format("And Mask Register: {0} ({1})", Enum.GetName(typeof(GPURegisterName), andMask & 0x1FFF), andMask & 0x1FFF));
                var andValue = _State.RegValues[andMask & 0x1FFF];
                if (_out != null) _out.Log(String.Format("And Mask Value: 0x{0:X}", andValue));
                value &= andValue;
            }
            else
            {
                // & imm
                value &= andMask;
                if (_out != null) _out.Log(String.Format("And Mask Value: 0x{0:X}", andMask));
            }

            // write new value
            if (_out != null) _out.Log(String.Format("Final Value: 0x{0:X}", value));
            WriteRegister(regIndex, value, _out);

            // executed
            return GPUExecutorResult.Executed;
        }

        public static UInt32 SwapBytes(UInt32  x)
        {
            return ((x & 0x000000ff) << 24) +
                   ((x & 0x0000ff00) << 8) +
                   ((x & 0x00ff0000) >> 8) +
                   ((x & 0xff000000) >> 24);
        }

        public static UInt32 GPUSwap32( UInt32 value, UInt32 format )
        {
            if ( format == 0 )
                return value;

            else if ( format == 1 )
    			return ((value << 8) & 0xFF00FF00) | ((value >> 8) & 0x00FF00FF);

            else if ( format == 2 )
                return SwapBytes( value );

            else if ( format == 3 )
    			return ((value >> 16) & 0xFFFF) | (value << 16);

            return value;
	    }

        private GPUExecutorResult ExecutePacketType3_COND_WRITE(RawPacket _packet, UInt32 packetData, UInt32 count, GPUCommandOutput _out)
        {
            // conditional write to memory or register
	        var waitInfo = _packet.Words[0];
	        var pollRegAddr = _packet.Words[1];
	        var refValue = _packet.Words[2];
	        var mask = _packet.Words[3];
	        var writeRegAddr = _packet.Words[4];
	        var writeData = _packet.Words[5];

	        // read input value
	        UInt32 value = 0;
	        if ( 0 != (waitInfo & 0x10) )
	        {
		        // get swap format (swap)
		        var readFormat = (pollRegAddr & 3);
                var readAddress = (pollRegAddr & ~3);
                if (_out != null) _out.Log(String.Format("Read Address: 0x{0:X}", readAddress));
                if ( _out != null ) _out.Log( String.Format("Read Mode: {0}", readFormat) );
                
                // load the value
                var memBlock = _packet.FindMemoryReadBlock((UInt32)readAddress);
                if ( memBlock != null )
                {
                    value = GPUSwap32( (UInt32)memBlock.LoadUint32At(0), readFormat );
                }
	        }
	        else
	        {
                if ( _out != null ) _out.Log( String.Format("Read Register: {0} ({1})",  Enum.GetName(typeof(GPURegisterName), pollRegAddr), pollRegAddr ) );
		        value = _State.RegValues[ pollRegAddr ];
	        }

            if ( _out != null )
            {
                _out.Log( String.Format("Incoming Value: 0x{0:X}", value) );
                _out.Log( String.Format("Incoming Mask: 0x{0:X}", mask) );
                _out.Log( String.Format("Masked Value: 0x{0:X}", mask & value) );

                if ( (waitInfo&7) > 0 && ((waitInfo&7) < 7) )
                    _out.Log( String.Format("Reference Value: 0x{0:X}", refValue));
            }

	        // compare
	        bool matched = false;
	        switch ( waitInfo & 0x7 )
	        {
		        case 0x0: 
                    matched = false; 
                    if (_out != null) _out.Log( "Condition: Never");
			        break;

		        case 0x1:  // Less than reference.
			        matched = (value & mask) < refValue;
                    if (_out != null) _out.Log(String.Format("Condition: 0x{0:X} < 0x{1:X}", (value & mask), refValue));
			        break;

		        case 0x2:  // Less than or equal to reference.
			        matched = (value & mask) <= refValue;
                    if (_out != null) _out.Log(String.Format("Condition: 0x{0:X} <= 0x{1:X}", (value & mask), refValue));
			        break;

		        case 0x3:  // Equal to reference.
			        matched = (value & mask) == refValue;
                    if (_out != null) _out.Log(String.Format("Condition: 0x{0:X} == 0x{1:X}", (value & mask), refValue));
			        break;

		        case 0x4:  // Not equal to reference.
			        matched = (value & mask) != refValue;
                    if (_out != null) _out.Log(String.Format("Condition: 0x{0:X} != 0x{1:X}", (value & mask), refValue));
			        break;

		        case 0x5:  // Greater than or equal to reference.
			        matched = (value & mask) >= refValue;
                    if (_out != null) _out.Log(String.Format("Condition: 0x{0:X} >= 0x{1:X}", (value & mask), refValue));
			        break;

		        case 0x6:  // Greater than reference.
                    matched = (value & mask) > refValue;
                    if (_out != null) _out.Log(String.Format("Condition: 0x{0:X} > 0x{1:X}", (value & mask), refValue));
			        break;

		        case 0x7:  // Always
			        matched = true;
                    if (_out != null) _out.Log("Condition: Always");
			        break;
	        }

	        // write result
            _out.Log( matched ? "Matched: YES" : "Matched: NO" );
	        if ( matched )
	        {
                 if (_out != null) _out.Log(String.Format("Write Data: 0x{0:X}", writeData));

		        // Write.
		        if ( 0 != (waitInfo & 0x100) )
		        {
                    var writeFormat = (writeRegAddr & 3);
                    var writeAddress = (writeRegAddr & ~3);
                    if (_out != null) _out.Log(String.Format("Write Address: 0x{0:X}", writeAddress));
                    if (_out != null) _out.Log(String.Format("Write Mode: {0}", writeFormat));

		        }
		        else
		        {
			        WriteRegister( writeRegAddr, writeData, _out );
		        }
	        }

            return GPUExecutorResult.Executed;
        }

        private GPUExecutorResult ExecutePacketType3_EVENT_WRITE(RawPacket _packet, UInt32 packetData, UInt32 count, GPUCommandOutput _out)
        {
            var initiator = _packet.Words[0];
        	WriteRegister( GPURegisterName.VGT_EVENT_INITIATOR, initiator & 0x3F, _out );
            return GPUExecutorResult.Executed;
        }

        private GPUExecutorResult ExecutePacketType3_EVENT_WRITE_SHD(RawPacket _packet, UInt32 packetData, UInt32 count, GPUCommandOutput _out)
        {
            return GPUExecutorResult.Executed;
        }

        private GPUExecutorResult ExecutePacketType3_EVENT_WRITE_EXT(RawPacket _packet, UInt32 packetData, UInt32 count, GPUCommandOutput _out)
        {
            return GPUExecutorResult.Executed;
        }

        struct DrawInfo
        {
            public UInt32 IndexData;
            public GPUIndexFormat IndexFormat;
            public GPUEndianFormat IndexEndianess;
            public UInt32 IndexCount;
            public UInt32 BaseVertexIndex;
            public GPUPrimitiveType PrimitiveType;
        }

        private static String MakeMemoryAddr(RawPacket packet, UInt32 address, UInt32 stride, GPUEndianFormat endianess, GPUIndexFormat format)
        {
            var block = packet.FindMemoryReadBlock(address);
            if (block != null)
            {
                var endianStr = Enum.GetName(typeof(GPUEndianFormat), endianess);
                var formatStr = (format == GPUIndexFormat.Index16) ? "FMT_16,UINT" : "FMT_32,UINT";
                return String.Format("<a href=\"buffer,{0},{1},{3},1,0,{4}\">0x{2:X}</a>", block.LocalIndex, stride, block.Adress, endianStr, formatStr);
            }
            else
            {
                return String.Format("0x{0:X6}", address);
            }
        }

        private static String MakeMemoryAddrALUConst(RawPacket packet, UInt32 address)
        {
            var block = packet.FindMemoryReadBlock(address);
            if (block != null)
            {
                var endianStr = Enum.GetName(typeof(GPUEndianFormat), GPUEndianFormat.Format8in32);
                var formatStr = "FMT_32_32_32_32,FLOAT";
                return String.Format("<a href=\"buffer,{0},{1},{3},1,0,{4}\">0x{2:X}</a>", block.LocalIndex, 16, block.Adress, endianStr, formatStr);
            }
            else
            {
                return String.Format("0x{0:X,6}", address);
            }
        }

        private class VFetchMerger
        {
            public struct Elem
            {
                public UInt32 Offset;
                public GPUFetchFormat Format;
                public GPUFetchDataType DataType;

                public string MakeLink()
                {
                    string txt = "";

                    txt += Offset;

                    txt += ",";
                    txt += Enum.GetName(typeof(GPUFetchFormat), Format);

                    txt += ",";
                    txt += Enum.GetName(typeof(GPUFetchDataType), DataType);

                    return txt;
                }

            }

            public class Stream
            {
                public RawMemoryBlock Block;
                public UInt32 Stride;
                public GPUEndianFormat Endianess;
                public List<Elem> Elements;

                public Stream()
                {
                    Elements = new List<Elem>();
                }

                public string MakeLink()
                {
                    string txt = "buffer";

                    txt += ",";
                    txt += Block.LocalIndex;

                    txt += ",";
                    txt += Stride;

                    txt += ",";
                    txt += Enum.GetName( typeof(GPUEndianFormat), Endianess);

                    txt += ",";
                    txt += Elements.Count;

                    foreach (var elem in Elements)
                    {
                        txt += ",";
                        txt += elem.MakeLink();
                    }

                    return txt;
                }
            }

            public List<Stream> Streams;

            public VFetchMerger()
            {
                Streams = new List<Stream>();
            }

            public void AddVFetch(RawMemoryBlock block, GPUVertexFetch vfetch)
            {
                // find matching stream
                Stream stream = null;
                foreach (var it in Streams)
                {
                    if (it.Block == block && it.Stride == (vfetch.stride * 4))
                    {
                        stream = it;
                        break;
                    }
                }

                // create new stream
                if (stream == null)
                {
                    stream = new Stream();
                    stream.Stride = vfetch.stride * 4; // vfetch stride is in DWORDS, we need stride in BYTES
                    stream.Block = block;
                    stream.Endianess = GPUEndianFormat.Format8in32;
                    Streams.Add(stream);
                }

                // add fetch entry to the stream itself
                Elem elem = new Elem();
                elem.DataType = vfetch.dataType;
                elem.Format = vfetch.format;
                elem.Offset = vfetch.offset * 4; // vfetch offset is in DWORDS, we need offset in BYTES
                stream.Elements.Add(elem);
            }
        }

        private void IssueDraw( RawPacket _packet, DrawInfo ds, GPUCommandOutput _out )
        {
            if (_out == null)
                return;

            if (ds.IndexData != 0)
            {
                var indexSize = ((ds.IndexFormat == GPUIndexFormat.Index16) ? 2U : 4U);
                _out.Log(String.Format("Index Data Endianess: {0}", Enum.GetName( typeof(GPUEndianFormat), ds.IndexEndianess)) );
                _out.Log(String.Format("Index Data Address: {0}", MakeMemoryAddr(_packet, ds.IndexData, indexSize, ds.IndexEndianess, ds.IndexFormat)));
            }

            _out.Log(String.Format("Index Count: {0}", ds.IndexCount ));
            _out.Log(String.Format("Base Vertex: {0}", ds.BaseVertexIndex));
            _out.Log(String.Format("Primitive Type: {0}", Enum.GetName( typeof(GPUPrimitiveType), ds.PrimitiveType)) );

            // raw shaders
            foreach (var memoryRef in _packet.Memory)
            {
                if (memoryRef.Tag == "PSSourceCode")
                {
                    _out.Log(String.Format("Pixel shader HLSL: <a href=\"hlsl,{0}\">0x{1:X6}</a>", memoryRef.Block.LocalIndex, memoryRef.Block.Adress));
                }
                else if (memoryRef.Tag == "VSSourceCode")
                {
                    _out.Log(String.Format("Vertex shader HLSL: <a href=\"hlsl,{0}\">0x{1:X6}</a>", memoryRef.Block.LocalIndex, memoryRef.Block.Adress));
                }
            }

            if (_State.VertexShader != null)
            {
                VFetchMerger vfetchMerger = new VFetchMerger();

                // output fetch parameters, merge streams
                var fetches = _State.VertexShader.VertexFetches;
                foreach (var vfetch in fetches)
                {
                    UInt32 regIndex = ((UInt32)GPURegisterName.SHADER_CONSTANT_FETCH_00_0) + (vfetch.slot * 2);

                    GPUVertexFetchData fetchReg = new GPUVertexFetchData( _State.RegValues[regIndex+0], _State.RegValues[regIndex+1] );
                    var fetchAddress = (fetchReg.address << 2);

                    _out.Log(String.Format("VFetch: Address=0x{0:X6} Slot={1} Offset={2} Stride={3} Format={4} DataType={5}",
                        fetchAddress, vfetch.slot, vfetch.offset, vfetch.stride,
                        Enum.GetName(typeof(GPUFetchFormat), vfetch.format),
                        Enum.GetName(typeof(GPUFetchDataType), vfetch.dataType)) );

                    var block = _packet.FindMemoryReadBlock( fetchAddress );
                    if ( block != null )
                    {
                        vfetchMerger.AddVFetch(block, vfetch);
                    }
                }

                // create vertex data references
                foreach (var stream in vfetchMerger.Streams)
                {
                    _out.Log(String.Format("Stream: <a href=\"{0}\">0x{1:X6}, {2} element(s)</a>", stream.MakeLink(), stream.Block.Adress, stream.Elements.Count));
                }
            }

            if (_State.PixelShader != null)
            {
                var tfetches = _State.PixelShader.TextureFetches;
                foreach (var tfetch in tfetches)
                {
                    _out.Log(String.Format("<h2>Pixel Shader Texture {0}</h2>", tfetch.slot));

                    {
                        string tfetchText = "";
                        tfetch.Descibe(ref tfetchText);
                        _out.Log("<h4>Shader texture:</h4>" + tfetchText);
                    }

                    {
                        var fetchReg = (UInt32)GPURegisterName.SHADER_CONSTANT_FETCH_00_0 + (tfetch.slot * 6);
                        var data0 = _State.RegValues[fetchReg + 0];
                        var data1 = _State.RegValues[fetchReg + 1];
                        var data2 = _State.RegValues[fetchReg + 2];
                        var data3 = _State.RegValues[fetchReg + 3];
                        var data4 = _State.RegValues[fetchReg + 4];
                        var data5 = _State.RegValues[fetchReg + 5];

                        var boundedTex = new GPUTextureFetchData(data0, data1, data2, data3, data4, data5);
                        _out._Textures.Add(boundedTex);

                        string boundexText = "";
                        boundedTex.Describe(ref boundexText);
                        _out.Log("<h4>Bounded texture:</h4>" + boundexText);
                    }

                    _out.Log("<hr/>");
                }
            } 
        }

        private GPUExecutorResult ExecutePacketType3_DRAW_INDX(RawPacket _packet, UInt32 packetData, UInt32 count, GPUCommandOutput _out)
        {
        	var dword0 = _packet.Words[0];
	        var dword1 = _packet.Words[1];

	        var indexCount = dword1 >> 16;
	        var primitiveType = (GPUPrimitiveType)( dword1 & 0x3F );
	        var source = (dword1 >> 6) & 0x3;

	        if ( source == 0x0 )
	        {
                if (_out != null)
                {
                    _out._Desc = String.Format("Draw: Indexed {0} {1}", Enum.GetName(typeof(GPUPrimitiveType),primitiveType), indexCount);
                    _out._DumpInDrawCall = true;
                    _out.Log("Draw: Indexed");
                }

		        // extract information
		        bool is32Bit = 0 != ((dword1 >> 11) & 0x1);
		        var indexAddr = _packet.Words[2];
		        var indexSize = _packet.Words[3];

		        // setup index state
		        var indexFormat =  is32Bit ? GPUIndexFormat.Index32 : GPUIndexFormat.Index16;
		        var indexEndianess = (GPUEndianFormat)( indexSize >> 30 );

		        // setup draw state
                DrawInfo ds = new DrawInfo();
		        ds.IndexData = indexAddr;
		        ds.IndexFormat = indexFormat;
		        ds.IndexEndianess = indexEndianess;
		        ds.IndexCount = indexCount;
		        ds.BaseVertexIndex = _State.RegValues[ (int)GPURegisterName.VGT_INDX_OFFSET ];
		        ds.PrimitiveType = primitiveType;
		
                IssueDraw( _packet, ds, _out );
            }		
	        else if ( source == 0x2 )
	        {
                if (_out != null)
                {
                    _out._Desc = String.Format("Draw: {0} {1}", Enum.GetName(typeof(GPUPrimitiveType), primitiveType), indexCount);
                    _out._DumpInDrawCall = true;
                    _out.Log("Draw: Not Indexed");
                }

                DrawInfo ds = new DrawInfo();
		        ds.IndexData = 0;
		        ds.IndexFormat = GPUIndexFormat.Index16;
		        ds.IndexEndianess = GPUEndianFormat.FormatUnspecified;
		        ds.IndexCount = indexCount;
		        ds.BaseVertexIndex = 0;//_State.RegValues[ GPURegisterName.VGT_INDX_OFFSET ];
		        ds.PrimitiveType = primitiveType;
		
                IssueDraw( _packet, ds, _out );
            }
		
            return GPUExecutorResult.Executed;
        }

        private GPUExecutorResult ExecutePacketType3_DRAW_INDX_2(RawPacket _packet, UInt32 packetData, UInt32 count, GPUCommandOutput _out)
        {
            // draw using supplied indices in packet
	        var dword0 = _packet.Words[0];
	        var indexCount = dword0 >> 16;

	        var primitiveType = (GPUPrimitiveType)( dword0 & 0x3F );

            if (_out != null)
            {
                _out._Desc = String.Format("Draw: Indexed2 {0} {1}", Enum.GetName(typeof(GPUPrimitiveType), primitiveType), indexCount);
                _out._DumpInDrawCall = true;
                _out.Log("Draw: Indexed2");
            }

	        var source = (dword0 >> 6) & 0x3;
            if ( source == 2 )
            {
	            DrawInfo ds = new DrawInfo();
	            ds.IndexCount = indexCount;
	            ds.PrimitiveType = primitiveType;
	            ds.IndexData = 0;
	            ds.IndexFormat = GPUIndexFormat.Index16;
	            ds.IndexEndianess = GPUEndianFormat.FormatUnspecified;
	            ds.BaseVertexIndex = 0;

                IssueDraw( _packet, ds, _out );
            }
		
            return GPUExecutorResult.Executed;
        }

        private GPUExecutorResult ExecutePacketType3_SET_CONSTANT(RawPacket _packet, UInt32 packetData, UInt32 count, GPUCommandOutput _out)
        {
            var offsetType = _packet.Words[0];
            var index = offsetType & 0x7FF;
            var type = (offsetType >> 16) & 0xFF;

            var baseIndex = index;
            switch (type)
            {
                case 0:  // ALU
                    if (_out != null) _out.Log("Block: ALU");
                    baseIndex += 0x4000;
                    break;

                case 1:  // FETCH
                    if (_out != null) _out.Log("Block: FETCH");
                    baseIndex += 0x4800;
                    break;

                case 2:  // BOOL
                    if (_out != null) _out.Log("Block: BOOL");
                    baseIndex += 0x4900;
                    break;

                case 3:  // LOOP
                    if (_out != null) _out.Log("Block: LOOP");
                    baseIndex += 0x4908;
                    break;

                case 4:  // REGISTERS
                    if (_out != null) _out.Log("Block: REGISTERS");
                    baseIndex += 0x2000;
                    break;

                default:
                    return GPUExecutorResult.Invalid;
            }

            // set data
            for ( UInt32 n = 0; n < (count - 1); n++)
            {
                UInt32 data = _packet.Words[n+1];
                WriteRegister(index + n, data, _out);
            }

            return GPUExecutorResult.Executed;
        }

        private GPUExecutorResult ExecutePacketType3_SET_CONSTANT2(RawPacket _packet, UInt32 packetData, UInt32 count, GPUCommandOutput _out)
        {
            var offsetType = _packet.Words[0];
            var index = offsetType & 0xFFFF;

            for ( UInt32 n = 0; n < (count - 1); n++)
            {
                var data = _packet.Words[1 + n];
                WriteRegister(index + n, data, _out);
            }

            return GPUExecutorResult.Executed;
        }

        private GPUExecutorResult ExecutePacketType3_LOAD_ALU_CONSTANT(RawPacket _packet, UInt32 packetData, UInt32 count, GPUCommandOutput _out)
        {
            var address = _packet.Words[0] & 0x3FFFFFFF;
	        var offset_type = _packet.Words[1];
		    var index = offset_type & 0x7FF;
            var baseIndex = index;
	        var size_dwords = _packet.Words[2] & 0xFFF;
	        var type = (offset_type >> 16) & 0xFF;

	        switch (type)
	        {
		        case 0:  // ALU
			        index += 0x4000;
			        break;
		        case 1:  // FETCH
			        index += 0x4800;
			        break;
		        case 2:  // BOOL
			        index += 0x4900;
			        break;
		        case 3:  // LOOP
			        index += 0x4908;
			        break;
		        case 4:  // REGISTERS
			        index += 0x2000;
			        break;
	        }

            if ( _out != null)
            {
                _out.Log(String.Format("ALU First Register: {0}", baseIndex));
                _out.Log(String.Format("ALU Actual Register: {0}", Enum.GetName( typeof(GPURegisterName), index )));
                _out.Log(String.Format("ALU Num Registers: {0}", size_dwords/4 ));
                _out.Log(String.Format("ALU Data Address: {0}", MakeMemoryAddrALUConst(_packet, address) ));
            }

            var block = _packet.FindMemoryReadBlock(address);
            if ( block != null )
            {
                var data = block.LoadAllDataAs32BE();
                for ( int i=0; i<(int)size_dwords; ++i )
                {
                    WriteRegister((UInt32)(index + i), data[i], _out);
                }
            }

            return GPUExecutorResult.Executed;
        }

        private GPUExecutorResult ExecutePacketType3_SET_SHADER_CONSTANTS(RawPacket _packet, UInt32 packetData, UInt32 count, GPUCommandOutput _out)
        {
            var offsetType = _packet.Words[0];
            var index = offsetType & 0xFFFF;

            for ( UInt32 n=0; n < count - 1; n++)
            {
                var data = _packet.Words[1 + n];
                WriteRegister(index + n, data, _out);
            }

            return GPUExecutorResult.Executed;
        }

        private static String MakeShaderAddr(RawMemoryBlock block, bool pixel)
        {
            if ( block != null )
            {
                return String.Format("<a href=\"{0}shader,{1}\">0x{2:X6}</a>",
                    pixel ? "pixel" : "vertex",
                    block.LocalIndex,
                    block.Adress);
            }
            else
            {
                return "NONE";
            }
        }

        private GPUExecutorResult ExecutePacketType3_IM_LOAD(RawPacket _packet, UInt32 packetData, UInt32 count, GPUCommandOutput _out)
        {
            // load sequencer instruction memory (pointer-based)
	        var addrType = _packet.Words[0];
	        var shaderType = (GPUShaderType)( addrType & 0x3 );
	        var addr = addrType & ~0x3;
	        var startSize = _packet.Words[1];
	        var start = startSize >> 16;
	        var sizeDwords = startSize & 0xFFFF;  // dwords

            var block = _packet.FindMemoryReadBlock( (UInt32)addr );
            var pixel = (shaderType == GPUShaderType.ShaderPixel);

            if (_out != null)
            {
                _out.Log("Shader type: " + ((shaderType == GPUShaderType.ShaderPixel) ? "Pixel" : "Vertex"));
                _out.Log("Shader words: " + sizeDwords.ToString());
                _out.Log(String.Format("Shader data: {0}", MakeShaderAddr( block, pixel )) );
                _out._DumpInDrawCall = true;
            }

            if ( block != null )
            {
                if ( pixel )
	            {
                    _out._Desc = "LoadPixelShader";
                    _State.SetPixelShader( block );
	            }
	            else
	            {
                    _out._Desc = "LoadVertexShader";
                    _State.SetVertexShader( block );
	            }
            }

            return GPUExecutorResult.Executed;
        }

        private GPUExecutorResult ExecutePacketType3_IM_LOAD_IMMEDIATE(RawPacket _packet, UInt32 packetData, UInt32 count, GPUCommandOutput _out)
        {
            // load sequencer instruction memory (code embedded in packet)
            var dword0 = _packet.Words[0];
            var dword1 = _packet.Words[1];

            var shaderType = (GPUShaderType)(dword0 & 0x3);

            var startSize = dword1;
            var start = startSize >> 16;
            var sizeDwords = startSize & 0xFFFF;  // dwords

            if (_packet.Memory.Length == 1)
            {
                var block = _packet.Memory[0].Block;
                var pixel = (shaderType == GPUShaderType.ShaderPixel);

                if (_out != null)
                {
                    _out.Log("Shader type: " + ((shaderType == GPUShaderType.ShaderPixel) ? "Pixel" : "Vertex"));
                    _out.Log("Shader words: " + sizeDwords.ToString());
                    _out.Log(String.Format("Shader data: {0}", MakeShaderAddr(block, pixel)));
                    _out._DumpInDrawCall = true;
                }

                if (block != null)
                {
                    if (pixel)
                    {
                        _out._Desc = "LoadPixelShaderIM";
                        _State.SetPixelShader(block);
                    }
                    else
                    {
                        _out._Desc = "LoadVertexShaderIM";
                        _State.SetVertexShader(block);
                    }
                }
            }

            return GPUExecutorResult.Executed;
        }

        private GPUExecutorResult ExecutePacketType3_INVALIDATE_STATE(RawPacket _packet, UInt32 packetData, UInt32 count, GPUCommandOutput _out)
        {
            return GPUExecutorResult.Executed;
        }

        private void WriteRegister(GPURegisterName _reg, UInt32 _value, GPUCommandOutput _out)
        {
            _State.RegValues[(UInt32)_reg] = _value;

            if ( _out != null )
            {
                _out.Log( String.Format( "Register {0} ({1}) written with {2} ({3})", 
                    Enum.GetName( typeof(GPURegisterName), _reg ), (UInt32)_reg, 
                    _value, ToFloat(_value) ) );
            }
        }

        private void WriteRegister(UInt32 _regIndex, UInt32 _value, GPUCommandOutput _out)
        {
            _State.RegValues[_regIndex] = _value;

             if ( _out != null )
            {
                _out.Log( String.Format( "Register {0} ({1}) written with {2} ({3})", 
                    Enum.GetName( typeof(GPURegisterName), _regIndex ), _regIndex, 
                    _value, ToFloat(_value) ) );
            }
        }

        private GPUState _State;
        private UInt64 _TileMask;
        private UInt64 _TileSelector;
    }
}
