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
    public partial class ViewShader : UserControl, DataViewUserControl 
    {
        private RawMemoryBlock _MemoryBlock;
        private GPUShader _Shader;

        public ViewShader()
        {
            InitializeComponent();
        }

        RawMemoryBlock DataViewUserControl.GetMemoryBlock()
        {
            return _MemoryBlock;
        }

        string DataViewUserControl.GetTitle()
        {
            if (_Shader.PixelShader)
                return String.Format("PixelShader 0x{0:X6}", _MemoryBlock.Adress);
            else
                return String.Format("VertexShader 0x{0:X6}", _MemoryBlock.Adress);
        }

        public bool Setup(RawMemoryBlock _block, ResourceViewParamReader paramReader)
        {
            // load source data
            var words = _block.LoadAllDataAs32BE();
            if (words == null)
                return false;

            // source memory block
            _MemoryBlock = _block;

            // shader type
            var isPixelShader = (paramReader.Param(0) == "pixelshader");

            // decompile shader
            _Shader = GPUShader.Decompile(isPixelShader, words);
            if (_Shader == null)
                return false;

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
                foreach (var lineTxt in _Shader.Decompiled)
                {
                    txt += lineTxt;
                    txt += "<br>";
                }
                txt += "</font>";
            }

            txt += "</body></html>";
            shaderCode.DocumentText = txt;
        }

    }
}
