using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace xenonGPUViewer.ResourceView
{
    public class ResourceViewParamReader
    {
        public ResourceViewParamReader(string paramString)
        {
            _Current = 0;
            Char[] splitList = { ',' };
            _Params = paramString.Split(splitList);
        }

        public string ReadString()
        {
            if ( _Current >= _Params.Length)
                return "";

            return _Params[_Current++];
        }

        public Int32 ReadInt32()
        {
            if (_Current >= _Params.Length)
                return 0;

            Int32 ret = 0;
            Int32.TryParse(_Params[_Current++], out ret);
            return ret;
        }

        public UInt32 ReadUInt32()
        {
            if (_Current >= _Params.Length)
                return 0;

            UInt32 ret = 0;
            UInt32.TryParse(_Params[_Current++], out ret);
            return ret;
        }

        public GPUEndianFormat ReadEndianFormat()
        {
            if (_Current >= _Params.Length)
                return 0;

            GPUEndianFormat ret = 0;
            var str = ReadString();
            System.Enum.TryParse<GPUEndianFormat>(str, out ret);
            return ret;
        }

        public GPUFetchFormat ReadFetchFormat()
        {
            if (_Current >= _Params.Length)
                return 0;

            GPUFetchFormat ret = 0;
            var str = ReadString();
            System.Enum.TryParse< GPUFetchFormat>(str, out ret);
            return ret;
        }

        public GPUFetchDataType ReadFetchDataType()
        {
            if (_Current >= _Params.Length)
                return 0;

            GPUFetchDataType ret = 0;
            var str = ReadString();
            System.Enum.TryParse<GPUFetchDataType>(str, out ret);
            return ret;
        }

        public string Param(UInt32 index)
        {
            if (index < _Params.Length)
                return _Params[index];

            return "";
        }

        private string[] _Params;
        private int _Current;
    }

    public class ResourceViewCollection
    {
        private TabControl _Tabs;
        private RawDumpData _RawData;

        public ResourceViewCollection(TabControl tabs, RawDumpData rawData)
        {
            _Tabs = tabs;
            _RawData = rawData;
        }

        
        public bool Open(string command)
        {
            var paramReader = new ResourceViewParamReader( command );

            // parse the type
            var type = paramReader.ReadString().ToLower();

            // parse the ref ID
            var id = paramReader.ReadUInt32();
            if (id >= _RawData.MemoryBlocks.Length)
                return false;

            // get the memory block
            var block = _RawData.MemoryBlocks[id];
            if (null == block)
                return false;

            // find in existing tabs, if found, switch to it
            foreach (TabPage tab in _Tabs.TabPages)
            {
                try
                {
                    var resourceView = (View.DataViewUserControl)tab.Controls[0];
                    if (resourceView != null)
                    {
                        if (resourceView.GetMemoryBlock() == block)
                        {
                            _Tabs.SelectedTab = tab;
                            return true;
                        }
                    }
                }
                catch (InvalidCastException)
                {
                }
            }

            string error = "Unknown data format";
            UserControl ctrl = null;
            try
            {
                // create the data view
                if (type == "buffer")
                {
                    var view = new View.ViewMemoryBuffer();
                    if (view.Setup(block, paramReader))
                    {
                        ctrl = view;
                    }
                }
                
                // create the shader view
                else if (type == "pixelshader" || type == "vertexshader")
                {
                    var view = new View.ViewShader();
                    if (view.Setup(block, paramReader))
                    {
                        ctrl = view;
                    }
                }

                 // create the HLSL view
                else if (type == "hlsl" )
                {
                    var view = new View.ViewShaderGeneratedCode();
                    if (view.Setup(block, paramReader))
                    {
                        ctrl = view;
                    }
                }
            }
            catch (Exception e)
            {
                error = e.ToString();
            }

            // no viewer
            if ( ctrl == null )
            {
                MessageBox.Show( _Tabs, "Unable to create data viewer: " + error );
                return false;
            }

            // add to tabs
            var genericInterface = (View.DataViewUserControl)ctrl;
            TabPage tabPage = new TabPage( genericInterface.GetTitle() );
            ctrl.Dock = DockStyle.Fill;
            tabPage.Controls.Add(ctrl);

            _Tabs.TabPages.Add(tabPage);
            _Tabs.SelectedTab = tabPage;
            return true;
        }

    }

}
