using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using xenonGPUViewer.ResourceView;
using System.Globalization;

namespace xenonGPUViewer.View
{
    public partial class ViewMemoryBuffer : UserControl, DataViewUserControl 
    {
        private UInt32 _MemoryStride;
        public GPUEndianFormat _MemoryEndianess;
        private ViewMemoryBufferElement[] _MemoryElements;
        private RawMemoryBlock _MemoryBlock;
        private Byte[] _Data;
        private Byte[] _DataEndian;

        [TypeConverter(typeof(ExpandableObjectConverter))]
        [Serializable()]
        public class ViewMemoryBufferPreviewFormat
        {
            public UInt32 _Address;
            public UInt32 _Size;
            public UInt32 _Stride;
            public String _Tag;

            [TypeConverter(typeof(IntToHexTypeConverter))]
            public UInt32 Address { get { return _Address; } }

            [TypeConverter(typeof(IntToHexTypeConverter))]
            public UInt32 Size { get { return _Size; } }

            public UInt32 Stride { get { return _Stride; } }
            public UInt32 Elements { get { return _Size / _Stride; } }
            public String Tag { get { return _Tag; } }

            public ViewMemoryBufferElement[] Layout { get { return _Layout; } set { _Layout = value; } }

            public ViewMemoryBufferElement[] _Layout;
        }

        RawMemoryBlock DataViewUserControl.GetMemoryBlock()
        {
            return _MemoryBlock;
        }

        string DataViewUserControl.GetTitle()
        {
            return String.Format("Buffer 0x{0:X6}", _MemoryBlock.Adress);
        }

        public UInt32 MemoryAddress
        {
            get
            {
                return _MemoryBlock.Adress;
            }
        }

        public UInt32 MemorySize
        {
            get
            {
                return _MemoryBlock.Size;
            }
        }

        public UInt32 MemoryStride
        {
            get
            {
                return _MemoryStride;
            }
            set
            {
                _MemoryStride = value;
                RebuildLayout();
            }
        }

        public ViewMemoryBufferElement[] MemoryElements
        {
            get
            {
                return _MemoryElements;
            }
        }

        public ViewMemoryBuffer()
        {
            InitializeComponent();
        }

        public bool Setup(RawMemoryBlock _block, ResourceViewParamReader paramReader)
        {
            // load source data
            _Data = _block.LoadAllData();
            if (_Data == null)
                return false;

            // source memory block
            _MemoryBlock = _block;

            // stride of data
            _MemoryStride = paramReader.ReadUInt32();

            // endianess
            _MemoryEndianess = paramReader.ReadEndianFormat();

            // convert endianess
            if (_MemoryEndianess == GPUEndianFormat.FormatUnspecified)
            {
                _DataEndian = _Data;
            }
            else if (_MemoryEndianess == GPUEndianFormat.Format8in32)
            {
                _DataEndian = new Byte[_Data.Length];
                for (UInt32 i = 0; i < _Data.Length; i += 4)
                {
                    _DataEndian[i + 0] = _Data[i + 3];
                    _DataEndian[i + 1] = _Data[i + 2];
                    _DataEndian[i + 2] = _Data[i + 1];
                    _DataEndian[i + 3] = _Data[i + 0];
                }
            }
            else if (_MemoryEndianess == GPUEndianFormat.Format8in16)
            {
                _DataEndian = new Byte[_Data.Length];
                for (UInt32 i = 0; i < _Data.Length; i += 2)
                {
                    _DataEndian[i + 0] = _Data[i + 1];
                    _DataEndian[i + 1] = _Data[i + 0];
                }
            }
            else if (_MemoryEndianess == GPUEndianFormat.Format16in32)
            {
                _DataEndian = new Byte[_Data.Length];
                for (UInt32 i = 0; i < _Data.Length; i += 4)
                {
                    _DataEndian[i + 0] = _Data[i + 1];
                    _DataEndian[i + 1] = _Data[i + 0];
                    _DataEndian[i + 2] = _Data[i + 3];
                    _DataEndian[i + 3] = _Data[i + 2];
                }
            }

            // number of format elements
            UInt32 numElems = paramReader.ReadUInt32();
            _MemoryElements = new ViewMemoryBufferElement[numElems];

            // load elements
            for (UInt32 i = 0; i < numElems; ++i)
            {
                ViewMemoryBufferElement elem = new ViewMemoryBufferElement();
                elem._Offset = paramReader.ReadUInt32();
                elem._Format = paramReader.ReadFetchFormat();
                elem._DataType = paramReader.ReadFetchDataType();
                _MemoryElements[i] = elem;
            }

            // rebuild view
            RebuildLayout();
            RefreshRawView();
            RefreshProperties();
            return true;
        }

        private static string Convert(Byte[] data, int offset, UInt32 numBytes, GPUFetchDataType dataType)
        {
            switch (dataType)
            {
                case GPUFetchDataType.UINT:
                {
                    if ( numBytes == 1 )
                        return data[offset].ToString();
                    else if ( numBytes == 2 )
                        return BitConverter.ToUInt16(data, offset).ToString();
                    else if ( numBytes == 4 )
                        return BitConverter.ToUInt32(data, offset).ToString();
                    break;
                }

                case GPUFetchDataType.UNORM:
                {
                    if ( numBytes == 1 )
                    {
                        var val = data[offset];
                        return ((float)val / (float)byte.MaxValue).ToString();
                    }
                    else if ( numBytes == 2 )
                    {
                        var val = BitConverter.ToUInt16(data, offset);
                        return ((float)val / (float)UInt16.MaxValue).ToString();
                    }
                    else if ( numBytes == 4 )
                    {
                        var val = BitConverter.ToUInt32(data, offset);
                        return ((double)val / (double)UInt32.MaxValue).ToString();
                    }
                    break;
                }

                case GPUFetchDataType.SINT:
                {
                    if ( numBytes == 1 )
                        return ((Char)data[offset]).ToString();
                    else if ( numBytes == 2 )
                        return BitConverter.ToInt16(data, offset).ToString();
                    else if ( numBytes == 4 )
                        return BitConverter.ToInt32(data, offset).ToString();
                    break;
                }

                case GPUFetchDataType.SNORM:
                {
                    if ( numBytes == 1 )
                    {
                        var val = (char)data[offset];
                        return ((float)val / (float)char.MaxValue).ToString();
                    }
                    else if ( numBytes == 2 )
                    {
                        var val = BitConverter.ToInt16(data, offset);
                        return ((float)val / (float)Int16.MaxValue).ToString();
                    }
                    else if ( numBytes == 4 )
                    {
                        var val = BitConverter.ToInt32(data, offset);
                        return ((double)val / (double)Int32.MaxValue).ToString();
                    }
                    break;
                }

                case GPUFetchDataType.FLOAT:
                {
                    if ( numBytes == 4 )
                    {
                        var val = BitConverter.ToSingle(data, offset);
                        return val.ToString();
                    }
                    break;

                }
            }

            return "UNKNOWN";
        }

        private static bool Convert(Byte[] data, int offset, GPUFetchFormat format, GPUFetchDataType dataType, string[] values, ref int numStr)
        {
            switch (format)
            {
                case GPUFetchFormat.FMT_8:
                {
                    values[numStr++] = Convert(data, offset+0, 1, dataType);
                    return false;
                }

                case GPUFetchFormat.FMT_8_8_8_8:
                {
                    values[numStr++] = Convert(data, offset+0, 1, dataType);
                    values[numStr++] = Convert(data, offset+1, 1, dataType);
                    values[numStr++] = Convert(data, offset+2, 1, dataType);
                    values[numStr++] = Convert(data, offset+3, 1, dataType);
                    return true;
                }

                case GPUFetchFormat.FMT_2_10_10_10:
                {
                    values[numStr++] = "FMT_2_10_10_10.X";
                    values[numStr++] = "FMT_2_10_10_10.Y";
                    values[numStr++] = "FMT_2_10_10_10.Z";
                    values[numStr++] = "FMT_2_10_10_10.W";
                    return true;
                }

		        case GPUFetchFormat.FMT_8_8:
                {
                    values[numStr++] = Convert(data, offset + 0, 1, dataType);
                    values[numStr++] = Convert(data, offset + 1, 1, dataType);
                    return true;
                }

		        case GPUFetchFormat.FMT_16:
                {
                    values[numStr++] = Convert(data, offset + 0, 2, dataType);
                    return true;
                }

		        case GPUFetchFormat.FMT_16_16:
                {
                    values[numStr++] = Convert(data, offset + 0, 2, dataType);
                    values[numStr++] = Convert(data, offset + 2, 2, dataType);
                    return true;
                }

		        case GPUFetchFormat.FMT_16_16_16_16:
                {
                    values[numStr++] = Convert(data, offset + 0, 2, dataType);
                    values[numStr++] = Convert(data, offset + 2, 2, dataType);
                    values[numStr++] = Convert(data, offset + 4, 2, dataType);
                    values[numStr++] = Convert(data, offset + 6, 2, dataType);
                    return true;
                }

		        case GPUFetchFormat.FMT_32:
                {
                    values[numStr++] = Convert(data, offset + 0, 4, dataType);
                    return true;
                }

		        case GPUFetchFormat.FMT_32_32:
                {
                    values[numStr++] = Convert(data, offset + 0, 4, dataType);
                    values[numStr++] = Convert(data, offset + 4, 4, dataType);
                    return true;
                }

		        case GPUFetchFormat.FMT_32_32_32_32:
                {
                    values[numStr++] = Convert(data, offset + 0, 4, dataType);
                    values[numStr++] = Convert(data, offset + 4, 4, dataType);
                    values[numStr++] = Convert(data, offset + 8, 4, dataType);
                    values[numStr++] = Convert(data, offset + 12, 4, dataType);
                    return true;
                }

		        case GPUFetchFormat.FMT_32_FLOAT:
                {
                    values[numStr++] = Convert(data, offset + 0, 4, GPUFetchDataType.FLOAT);
                    return true;
                }

		        case GPUFetchFormat.FMT_32_32_FLOAT:
                {
                    values[numStr++] = Convert(data, offset + 0, 4, GPUFetchDataType.FLOAT);
                    values[numStr++] = Convert(data, offset + 4, 4, GPUFetchDataType.FLOAT);
                    return true;
                }

		        case GPUFetchFormat.FMT_32_32_32_32_FLOAT:
                {
                    values[numStr++] = Convert(data, offset + 0, 4, GPUFetchDataType.FLOAT);
                    values[numStr++] = Convert(data, offset + 4, 4, GPUFetchDataType.FLOAT);
                    values[numStr++] = Convert(data, offset + 8, 4, GPUFetchDataType.FLOAT);
                    values[numStr++] = Convert(data, offset + 12, 4, GPUFetchDataType.FLOAT);
                    return true;
                }

		        case GPUFetchFormat.FMT_32_32_32_FLOAT:
                {
                    values[numStr++] = Convert(data, offset + 0, 4, GPUFetchDataType.FLOAT);
                    values[numStr++] = Convert(data, offset + 4, 4, GPUFetchDataType.FLOAT);
                    values[numStr++] = Convert(data, offset + 8, 4, GPUFetchDataType.FLOAT);
                    return true;
                }
            }

            return false;
        }

        private void RebuildLayout()
        {
            // create generic columns
            listViewBuffer.BeginUpdate();
            listViewBuffer.Columns.Clear();
            listViewBuffer.Columns.Add("Offset", 90);
            listViewBuffer.Columns.Add("Index", 60);

            // create columns related to format
            foreach (var elem in _MemoryElements)
            {
                switch (elem._Format)
                {
                    case GPUFetchFormat.FMT_8:
                    case GPUFetchFormat.FMT_16:
                    case GPUFetchFormat.FMT_32:
		            case GPUFetchFormat.FMT_32_FLOAT:
                    {
                        var name = String.Format("Elem{0}", elem.Offset);
                        listViewBuffer.Columns.Add(name, 80);
                        break;
                    }

                    case GPUFetchFormat.FMT_16_16:
                    case GPUFetchFormat.FMT_32_32:
                    case GPUFetchFormat.FMT_32_32_FLOAT:
                    case GPUFetchFormat.FMT_8_8:
                    {
                        var name = String.Format("Elem{0}", elem.Offset);
                        listViewBuffer.Columns.Add(name + ".X", 80);
                        listViewBuffer.Columns.Add(name + ".Y", 80);
                        break;
                    }

                    case GPUFetchFormat.FMT_32_32_32_FLOAT:
                    {
                        var name = String.Format("Elem{0}", elem.Offset);
                        listViewBuffer.Columns.Add(name + ".X", 80);
                        listViewBuffer.Columns.Add(name + ".Y", 80);
                        listViewBuffer.Columns.Add(name + ".Z", 80);
                        break;
                    }

                    case GPUFetchFormat.FMT_8_8_8_8:
		            case GPUFetchFormat.FMT_2_10_10_10:
		            case GPUFetchFormat.FMT_16_16_16_16:
                    case GPUFetchFormat.FMT_32_32_32_32:
                    case GPUFetchFormat.FMT_32_32_32_32_FLOAT:
                    {
                        var name = String.Format("Elem{0}", elem.Offset);
                        listViewBuffer.Columns.Add(name + ".X", 80);
                        listViewBuffer.Columns.Add(name + ".Y", 80);
                        listViewBuffer.Columns.Add(name + ".Z", 80);
                        listViewBuffer.Columns.Add(name + ".W", 80);
                        break;
                    }
                }
            }

            // create data
            string[] strings = new string[64];
            for (UInt32 offset = 0; offset < _Data.Length; offset += _MemoryStride)
            {
                int curString = 0;
                foreach (var elem in _MemoryElements)
                {
                    Convert(_DataEndian, (int)(offset + elem.Offset), elem.Format, elem.DataType, strings, ref curString);
                }

                ListViewItem item = new ListViewItem();
                item.Text = String.Format("+0x{0:X5}", offset);
                item.SubItems.Add((offset / _MemoryStride).ToString());

                for (int i = 0; i < curString; ++i)
                    item.SubItems.Add(strings[i]);

                listViewBuffer.Items.Add(item);
            }

            // end update
            listViewBuffer.EndUpdate();
            listViewBuffer.Refresh();
        }

        private void RefreshRawView()
        {
            // begin update
            listViewRaw.BeginUpdate();
            listViewRaw.Items.Clear();

            // add rows
            UInt32 numBytesPerRow = 16;
            for (UInt32 pos = 0; pos < _Data.Length; )
            {
                ListViewItem item = new ListViewItem();
                item.Text = String.Format("0x{0:X6}", _MemoryBlock.Adress + pos); // address

                item.SubItems.Add(String.Format("+0x{0:X4}", pos));

                string text = "";
                for (UInt32 j = 0; (j < numBytesPerRow) && (pos < _Data.Length); ++pos, ++j)
                {
                    if (j > 0)
                        text += " ";

                    text += String.Format("{0:X2}", _Data[pos]);
                }

                item.SubItems.Add(text);

                listViewRaw.Items.Add(item);
            }

            // finish update
            listViewRaw.EndUpdate();
            listViewRaw.Refresh();            
        }

        private void RefreshProperties()
        {
            ViewMemoryBufferPreviewFormat temp = new ViewMemoryBufferPreviewFormat();
            temp._Address = _MemoryBlock.Adress;
            temp._Size = _MemoryBlock.Size;
            temp._Stride = _MemoryStride;
            temp._Tag = "Buffer";
            temp._Layout = _MemoryElements;

            propertyGrid1.SelectedObject = temp;
        }

    }
}


