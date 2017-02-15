using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace xenonGPUViewer.View
{
    [TypeConverter(typeof(ExpandableObjectConverter))]
    [Serializable()]
    public struct ViewMemoryBufferElement
    {
        public GPUFetchFormat _Format;
        public GPUFetchDataType _DataType;
        public UInt32 _Offset;

        public GPUFetchFormat Format { get { return _Format; } }
        public GPUFetchDataType DataType { get { return _DataType; } }
        public UInt32 Offset { get { return _Offset; }  }

    }

    public interface DataViewUserControl
    {
        RawMemoryBlock GetMemoryBlock();
        string GetTitle();
    }

    public class IntToHexTypeConverter : TypeConverter
    {
        public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType)
        {
            if (sourceType == typeof(string))
            {
                return true;
            }
            else
            {
                return base.CanConvertFrom(context, sourceType);
            }
        }

        public override bool CanConvertTo(ITypeDescriptorContext context, Type destinationType)
        {
            if (destinationType == typeof(string))
            {
                return true;
            }
            else
            {
                return base.CanConvertTo(context, destinationType);
            }
        }

        public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType)
        {
            if (destinationType == typeof(string) && value.GetType() == typeof(int))
            {
                return string.Format("0x{0:X8}", value);
            }
            else if (destinationType == typeof(string) && value.GetType() == typeof(UInt32))
            {
                return string.Format("0x{0:X8}", value);
            }
            else
            {
                return base.ConvertTo(context, culture, value, destinationType);
            }
        }

        public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value)
        {
            if (value.GetType() == typeof(string))
            {
                string input = (string)value;

                if (input.StartsWith("0x", StringComparison.OrdinalIgnoreCase))
                {
                    input = input.Substring(2);
                }

                return int.Parse(input, NumberStyles.HexNumber, culture);
            }
            else
            {
                return base.ConvertFrom(context, culture, value);
            }
        }
    }

}
