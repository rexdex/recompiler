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

namespace xenonGPUViewer.View
{
    public partial class ViewShaderGeneratedCode : UserControl, DataViewUserControl 
    {
        private RawMemoryBlock _MemoryBlock;

        public ViewShaderGeneratedCode()
        {
            InitializeComponent();
        }

        RawMemoryBlock DataViewUserControl.GetMemoryBlock()
        {
            return _MemoryBlock;
        }

        string DataViewUserControl.GetTitle()
        {
            return String.Format("HLSL 0x{0:X6}", _MemoryBlock.Adress);
        }

        public bool Setup(RawMemoryBlock _block, ResourceViewParamReader paramReader)
        {
            // source memory block
            _MemoryBlock = _block;

            // valid shader
            ShowCode();
            return true;
        }

        private void ShowCode()
        {
            string txt = "<html><body>";

            // code
            {
                txt += "<font face=\"courier new, arial\" size=\"3\">";

                txt += "<pre>";
                byte[] data = _MemoryBlock.LoadAllData();
                string rawCode = System.Text.Encoding.ASCII.GetString(data);
                txt += rawCode;
                txt += "</pre>";

                txt += "</font>";
            }

            txt += "</body></html>";
            shaderCode.DocumentText = txt;
        }
    }
}
