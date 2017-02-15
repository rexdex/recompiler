using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace xenonGPUViewer
{
    public class RawDumpData
    {
        public static RawDumpData Load(String path)
        {
            try
            {
                FileStream fs = File.OpenRead(path);
                if (fs != null)
                {
                    return LoadFromFile(fs);
                }
            }
            catch (Exception)
            {
            }

            return null;
        }

        public void Close()
        {
            if (FileHandle != null)
            {
                FileHandle.Close();
                FileHandle = null;
            }
        }

        public static RawDumpData LoadFromFile(FileStream fs)
        {
            BinaryReader reader = new BinaryReader(fs);

            // load and verify header
            try
            {
                UInt32 magic = reader.ReadUInt32();
                UInt32 version = reader.ReadUInt32();

                if (magic != 0x47505544 || version != 1)
                    return null;
            }
            catch (Exception)
            {
                return null;
            }

            // load header data
            var numBlocks = reader.ReadUInt32();
            var blocksOffset = reader.ReadUInt64();
            var numPackets = reader.ReadUInt32();
            var packetsOffset = reader.ReadUInt64();
            var numMemoryRefs = reader.ReadUInt32();
            var memoryRefsOffset = reader.ReadUInt64();
            var numMemoryBlocks = reader.ReadUInt32();
            var memoryBlocksOffset = reader.ReadUInt64();
            var numDataRegs = reader.ReadUInt32();
            var dataRegsOffset = reader.ReadUInt64();

            // offset to actual memory dump data
            var memoryDumpOffset = reader.ReadUInt64();

            // create arrays
            RawDumpData ret = new RawDumpData();

            // load memory blocks - tempshit
            ret.AllMemoryBlocks = new RawMemoryBlock[numMemoryBlocks];
            for ( UInt32 i=0; i<numMemoryBlocks; ++i )
            {
                RawMemoryBlock elem = new RawMemoryBlock();
                ret.AllMemoryBlocks[i] = elem;

                elem.LocalIndex = i;
                elem.File = fs;
                elem.CRC = reader.ReadUInt64();
                elem.FileOffset = memoryDumpOffset + reader.ReadUInt64();
                elem.Adress = reader.ReadUInt32();
                elem.Size = reader.ReadUInt32();
            }

            // load memory references - tempshit
            RawMemoryRef[] memoryRefs = new RawMemoryRef[ numMemoryRefs ];
            for ( UInt32 i=0; i<numMemoryRefs; ++i )
            {
                RawMemoryRef elem = new RawMemoryRef();
                memoryRefs[i] = elem;

                elem.Block = ret.AllMemoryBlocks[reader.ReadUInt32()];

                var mode = reader.ReadUInt32();
                elem.Mode = (mode == 1) ? RawMemoryRefMode.Write : RawMemoryRefMode.Read;

                Byte[] tag = reader.ReadBytes(16);
                elem.Tag = System.Text.Encoding.ASCII.GetString(tag).Replace("\0", string.Empty);
            }

            // load reg data
            UInt32[] regData = new UInt32[ numDataRegs ];
            for ( UInt32 i=0; i<numDataRegs; ++i )
            {
                regData[i] = reader.ReadUInt32();
            }

            // load packets
            ret.AllPackets = new RawPacket[ numPackets ];
            for (UInt32 i = 0; i < numPackets; ++i)
            {
                RawPacket packet = new RawPacket();
                ret.AllPackets[i] = packet;

                // packet data word
                packet.Data = reader.ReadUInt32();

                // load additional data
                {
                    var firstWord = reader.ReadUInt32();
                    var numWords = reader.ReadUInt32();

                    packet.Words = new UInt32[numWords];
                    if ( numWords > 0 )
                    {
                        for ( UInt32 j=0; j<numWords; ++j )
                            packet.Words[j] = regData[ firstWord + j ];
                    }
                }

                // load memory references
                {
                    var firstMemoryRef = reader.ReadUInt32();
                    var numPacketMemoryRefs = reader.ReadUInt32();

                    packet.Memory = new RawMemoryRef[numPacketMemoryRefs];
                    if ( numPacketMemoryRefs > 0 )
                    {
                        for ( UInt32 j=0; j<numPacketMemoryRefs; ++j )
                            packet.Memory[j] = memoryRefs[ firstMemoryRef + j ];
                    }
                }
            }

            // create all blocks beforehand
            ret.AllBlocks = new RawBlock[numBlocks];
            for (UInt32 i = 0; i < numBlocks; ++i)
            {
                RawBlock block = new RawBlock();
                ret.AllBlocks[i] = block;
            }

            // load blocks
            for (UInt32 i = 0; i < numBlocks; ++i)
            {
                RawBlock block = ret.AllBlocks[i];

                Byte[] tag = reader.ReadBytes(16);
                block.Tag = System.Text.Encoding.ASCII.GetString(tag);

                // load sub blocks refs
                {
                    var firstSubBlock = reader.ReadUInt32();
                    var numSubBlocks = reader.ReadUInt32();

                    block.SubBlocks = new RawBlock[numSubBlocks];
                    for (UInt32 j = 0; j < numSubBlocks; ++j)
                        block.SubBlocks[j] = ret.AllBlocks[firstSubBlock + j];
                }

                // load packets refs
                {
                    var firstSubPacket = reader.ReadUInt32();
                    var numSubPackets = reader.ReadUInt32();

                    block.Packets = new RawPacket[numSubPackets];
                    for (UInt32 j = 0; j < numSubPackets; ++j)
                        block.Packets[j] = ret.AllPackets[firstSubPacket + j];
                }
            }

            // keep the file handle opened
            ret.FileHandle = fs;

            // raw data loaded
            return ret;
        }

        public RawBlock RootBlock
        {
            get
            {
                return AllBlocks[0];
            }
        }

        public RawBlock[] Blocks
        {
            get
            {
                return AllBlocks;
            }
        }

        public RawPacket[] Packets
        {
            get
            {
                return AllPackets;
            }
        }

        public RawMemoryBlock[] MemoryBlocks
        {
            get
            {
                return AllMemoryBlocks;
            }
        }

        private RawBlock[] AllBlocks;
        private RawPacket[] AllPackets;
        private RawMemoryBlock[] AllMemoryBlocks;

        private FileStream FileHandle; // for EDRAM and memory blocks viewer
    }

    public class RawBlock
    {
        public String Tag;
        public RawBlock[] SubBlocks;
        public RawPacket[] Packets;
    }

    public class RawPacket
    {
        public UInt32 Data; // packet data itself
        public UInt32[] Words; // additional words
        public RawMemoryRef[] Memory; // references to memory

        public RawMemoryBlock FindMemoryReadBlock(UInt32 addr)
        {
            foreach (var mem in Memory)
            {
                if (mem.Block.Adress == addr && mem.Mode == RawMemoryRefMode.Read)
                    return mem.Block;
            }

            foreach (var mem in Memory)
            {
                if (mem.Block.Adress == (0xC0000000U | addr) && mem.Mode == RawMemoryRefMode.Read)
                    return mem.Block;
            }           

            return null;
        }
    }

    public enum RawMemoryRefMode
    {
        Read=0,
        Write=1
    }

    public class RawMemoryRef
    {
        public RawMemoryBlock Block;
        public RawMemoryRefMode Mode;
        public String Tag;
    }

    public class RawMemoryBlock
    {
        public UInt32 LocalIndex;
        public UInt64 FileOffset;
        public UInt32 Size;
        public UInt32 Adress;
        public UInt64 CRC;
        public FileStream File;

        public byte[] LoadAllData()
        {
            byte[] ret = new byte[Size];

            File.Seek((long)FileOffset, SeekOrigin.Begin);
            File.Read(ret, 0, (int)Size);

            return ret;
        }

        private static UInt32 ReverseBytes(UInt32 value)
        {
            return (value & 0x000000FFU) << 24 | (value & 0x0000FF00U) << 8 |
                   (value & 0x00FF0000U) >> 8 | (value & 0xFF000000U) >> 24;
        }

        public UInt32[] LoadAllDataAs32BE()
        {
            UInt32[] ret = new UInt32[Size/4];

            byte[] temp = new byte[Size];
            File.Seek((long)FileOffset, SeekOrigin.Begin);
            File.Read(temp, 0, (int)Size);

            for (int i = 0; i < Size; i += 4)
            {
                ret[i/4] = ReverseBytes( BitConverter.ToUInt32(temp, i) );
            }

            return ret;
        }

        public byte[] LoadDataAt( int LocalOffset, int LocalSize)
        {
            byte[] ret = new byte[LocalSize];

            File.Seek((long)FileOffset + LocalOffset, SeekOrigin.Begin);
            File.Read(ret, 0, LocalSize);

            return ret;
        }

        public UInt32 LoadUint32At(int LocalOffset)
        {
            byte[] ret = new byte[4];
            File.Seek((long)FileOffset + LocalOffset, SeekOrigin.Begin);
            File.Read(ret, 0, 4);
            return BitConverter.ToUInt32(ret, 0);
        }

        public UInt32 LoadUint32AtBE(int LocalOffset)
        {
            byte[] ret = new byte[4];
            File.Seek((long)FileOffset + LocalOffset, SeekOrigin.Begin);
            File.Read(ret, 0, 4);
            ret.Reverse();
            return BitConverter.ToUInt32(ret, 0);
        }

    }
}
