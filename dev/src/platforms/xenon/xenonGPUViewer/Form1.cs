using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using xenonGPUViewer.ResourceView;

namespace xenonGPUViewer
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();

            CreateRegisters();
        }

        private void toolLoad_Click(object sender, EventArgs e)
        {
            // ask for file
            var res = openDumpDialog.ShowDialog();
            if (res != System.Windows.Forms.DialogResult.OK)
                return;

            // try to load it
            var path = openDumpDialog.FileName;
            var newData = RawDumpData.Load( path );
            if (newData == null)
            {
                MessageBox.Show(this, "Unable to load GPU trace from selected file");
            }

            // close current file
            if (!CloseCurrent())
                return;

            // set new raw view
            _RawData = newData;

            // create parsed view
            _Parsed = ParsedData.Parse(_RawData);

            // create new data viewer
            _DataViewer = new ResourceViewCollection(tabDataViews, _RawData);

            // refresh UI
            RebuildDrawCallTree();
            RefreshPacketDump();
            RefreshDrawCallDump();
            RefreshRegisterDump();
            RefreshStateDump();
        }

        public bool CloseCurrent()
        {
            // it's fine to load new shit if we don't have anything
            if (_RawData == null)
                return true;

            // ask user
            if (MessageBox.Show(this, "Close current file?", "Question", MessageBoxButtons.YesNo) == System.Windows.Forms.DialogResult.No)
                return false;

            // close current raw data view
            _RawData.Close();
            _RawData = null;

            // destroy parsed view
            _Parsed = null;

            // close all tabs
            tabData.TabPages.Clear();
            _DataViewer = null;

            // refresh ui
            RebuildDrawCallTree();
            RefreshPacketDump();
            RefreshDrawCallDump();
            RefreshRegisterDump();
            RefreshStateDump();
            return true;
        }

        public abstract class TreeNodeWithPackets : TreeNode
        {
            public abstract ParsedDrawCall GetDrawCall();
        }

        private class TreeNode_DrawCall : TreeNodeWithPackets
        {
            public TreeNode_DrawCall(ParsedDrawCall drawCall)
            {
                Text = drawCall.ToString();
                _DrawCall = drawCall;
            }

            public override ParsedDrawCall GetDrawCall()
            {
                return _DrawCall;
            }

            private ParsedDrawCall _DrawCall;
        }

        private class TreeNode_DrawGroup : TreeNodeWithPackets
        {
            public TreeNode_DrawGroup(ParsedDrawGroup drawGroup)
            {
                Text = drawGroup.ToString();
                _DrawGroup = drawGroup;

                foreach ( var drawCall in drawGroup.DrawCalls)
                    this.Nodes.Add( new TreeNode_DrawCall(drawCall) );
            }

            public override ParsedDrawCall GetDrawCall()
            {
                return null;
            }

            private ParsedDrawGroup _DrawGroup;
        }

        private void RebuildDrawCallTree()
        {
            // delete current content
            treeDrawCalls.Nodes.Clear();

            // add elements
            if (_Parsed != null)
            {
                foreach (var drawGroup in _Parsed.DrawGroups)
                    treeDrawCalls.Nodes.Add(new TreeNode_DrawGroup(drawGroup) );
            }

            // redraw
            treeDrawCalls.Refresh();
        }

        private ParsedDrawCall GetSelectedDrawCall()
        {
            if (tabTree.SelectedIndex == 0)
            {
                var selectedNode = (TreeNodeWithPackets)treeDrawCalls.SelectedNode;
                if ( selectedNode != null )
                    return selectedNode.GetDrawCall();
            }

            return null;
        }

        private GPURegisterInfos _RegInfos;
        private UInt32[] _RegCurrentValues;

        private void CreateRegisters()
        {
            _RegInfos = new GPURegisterInfos();
            _RegCurrentValues = new UInt32[ _RegInfos._Info.Count ];

            listAllRegs.BeginUpdate();
            listAllRegs.Items.Clear();

            foreach ( var item in _RegInfos._Info )
            {
                ListViewItem listItem = new ListViewItem();
                listItem.Text = ((UInt32) item.Index).ToString();

                listItem.SubItems.Add(item.Name);
                listItem.SubItems.Add( "-" );
                listItem.SubItems.Add( "-" );
                listItem.SubItems.Add( "-" );

                listAllRegs.Items.Add(listItem);
            }

            listAllRegs.EndUpdate();
            listAllRegs.Refresh();
        }

        private List<ListViewItem> _ColoredRegItems;

        private void RefreshRegisterDump()
        {
            var drawCall = GetSelectedDrawCall();

            if (_ColoredRegItems == null)
                _ColoredRegItems = new List<ListViewItem>();

            foreach (var listItem in _ColoredRegItems)
            {
                listItem.ForeColor = Color.Black;
            }

            _ColoredRegItems.Clear();

            if (drawCall != null)
            {
                listAllRegs.BeginUpdate();

                for ( UInt32 i=0; i<_RegInfos._Info.Count; ++i )
                {
                    var item = _RegInfos._Info[(int)i];
                    var incomingValue = drawCall.CapturedState.Reg(item.Index);
                    if ( incomingValue != _RegCurrentValues[i] )
                    {
                        _RegCurrentValues[i] = incomingValue;

                        var listItem = listAllRegs.Items[(int)i];
                        if (item.Type == GPURegisterValueType.DataDWord)
                        {
                            listItem.SubItems[2].Text = incomingValue.ToString();
                            listItem.SubItems[3].Text = String.Format("0x{0:X8}", incomingValue);
                        }
                        else
                        {
                            listItem.SubItems[3].Text = String.Format("0x{0:X8}", incomingValue);
                            listItem.SubItems[4].Text = GPUExecutor.ToFloat( incomingValue ).ToString();
                        }

                        listItem.ForeColor = Color.Red;
                        _ColoredRegItems.Add(listItem);
                    }
                }

                listAllRegs.EndUpdate();
            }
        }

        private void RefreshStateDump()
        {
            string txt = "";
            txt += "<html><body>";

            var drawCall = GetSelectedDrawCall();
            if (drawCall == null)
            {
                txt += "No draw call";
            }
            else
            {
                {
                    var viewportState = new GPUStateCaptureViewport( drawCall.CapturedState );
                    viewportState.OutputInfo( ref txt );
                }

                {
                    var localState = new GPUStateRenderTargets(drawCall.CapturedState);
                    localState.OutputInfo(ref txt);
                }
            }

            // flush
            txt += "</body></html>";
            dumpAllStates.DocumentText = txt;
        }

        private void RefreshDrawCallDump()
        {
            string txt = "";
            txt += "<html><body>";

            var drawCall = GetSelectedDrawCall();
            if (drawCall == null)
            {
                txt += "No draw call";
            }
            else
            {
                // packet
                foreach (var drawPacket in drawCall.Packets)
                {
                    if (drawPacket.Output._DumpInDrawCall)
                    {
                        txt += "<p>";

                        txt += "<h2>";
                        txt += drawPacket.Output._Desc;
                        txt += "</h2><br>";

                        foreach (var str in drawPacket.Output._Log)
                        {
                            txt += str;
                            txt += "<br>";
                        }

                        txt += "</p>";
                    }
                }
            }

            // flush
            txt += "</body></html>";
            drawCallDump.DocumentText = txt;
        }

        private void RefreshUsedConsts(GPUShader shader, GPUStateCapture state, ListView view)
        {
            view.BeginUpdate();
            view.Items.Clear();

            foreach ( UInt32 baseRegIndex in shader.UsedRegisters )
            {
                var item = new ListViewItem();
                item.Text = baseRegIndex.ToString();

                var stateRegOffset = baseRegIndex + (shader.PixelShader ? 256U : 0);
                var stateRegIndex = (UInt32)GPURegisterName.SHADER_CONSTANT_000_X + (4*stateRegOffset);

                var varR = GPUExecutor.ToFloat(state.Reg(stateRegIndex + 0));
                var varG = GPUExecutor.ToFloat(state.Reg(stateRegIndex + 1));
                var varB = GPUExecutor.ToFloat(state.Reg(stateRegIndex + 2));
                var varA = GPUExecutor.ToFloat(state.Reg(stateRegIndex + 3));

                item.SubItems.Add(varR.ToString());
                item.SubItems.Add(varG.ToString());
                item.SubItems.Add(varB.ToString());
                item.SubItems.Add(varA.ToString());

                if (((varR >= 0.0f) && (varR <= 1.0f)) && ((varG >= 0.0f) && (varG <= 1.0f)) && ((varB >= 0.0f) && (varB <= 1.0f)) && ((varA >= 0.0f) && (varA <= 1.0f)))
                {
                    string txt = String.Format("0x{0:X2}{1:X2}{2:X2}{3:X2}", (int)(varR * 255.0f), (int)(varG * 255.0f), (int)(varB * 255.0f), (int)(varA * 255.0f));
                    item.SubItems.Add(txt);

                    var colorItem = item.SubItems.Add(" ");
                    colorItem.BackColor = Color.FromArgb((int)(varR * 255.0f), (int)(varG * 255.0f), (int)(varB * 255.0f));
                    item.UseItemStyleForSubItems = false;
                }
                else
                {
                    item.SubItems.Add("-");
                    item.SubItems.Add("-");
                }

                view.Items.Add(item);
            }

            view.EndUpdate();
            view.Refresh();
        }

        private void RefreshUsedConsts()
        {
            var drawCall = GetSelectedDrawCall();
            if (drawCall == null)
            {
                listPixelConsts.Items.Clear();
                listVertexConsts.Items.Clear();
            }
            else
            {
                // refresh pixel constants
                if (drawCall.CapturedState.PixelShader != null)
                {
                    RefreshUsedConsts(drawCall.CapturedState.PixelShader, drawCall.CapturedState, listPixelConsts);
                }
                else
                {
                    listPixelConsts.Items.Clear();
                }

                // refresh vertex constants
                if (drawCall.CapturedState.VertexShader != null)
                {
                    RefreshUsedConsts(drawCall.CapturedState.VertexShader, drawCall.CapturedState, listVertexConsts);
                }
                else
                {
                    listVertexConsts.Items.Clear();
                }
            }

        }

        private void RefreshPacketDump()
        {
            string txt = "";

            var drawCall = GetSelectedDrawCall();

            txt += "<html><body>";

            if (drawCall == null)
            {
                txt += "No draw call";
            }
            else if (drawCall.Packets.Count == 0)
            {
                txt += "No packets in draw call";
            }
            else
            {
                foreach (var packet in drawCall.Packets)
                {
                    txt += "<p>";
                    
                    txt += "<h2>";
                    txt += packet.Output._Desc;
                    txt += "</h2><br>";

                    foreach ( var str in packet.Output._Log )
                    {
                        txt += str;
                        txt += "<br>";
                    }

                    txt += "</p>";
                }
            }

            txt += "</body></html>";

            packetBreakdown.DocumentText = txt;
        }

        private RawDumpData _RawData;
        private ParsedData _Parsed;
        private ResourceViewCollection _DataViewer;
        
        private void treeDrawCalls_AfterSelect(object sender, TreeViewEventArgs e)
        {
            RefreshRegisterDump();
            RefreshStateDump();
            RefreshPacketDump();
            RefreshDrawCallDump();
            RefreshUsedConsts();
        }

        public void OpenDataView(string command)
        {
            if (_DataViewer != null)
                _DataViewer.Open(command);
        }

        private void packetBreakdown_Navigating(object sender, WebBrowserNavigatingEventArgs e)
        {
            if (e.Url.AbsolutePath != "blank")
            {
                OpenDataView(e.Url.AbsolutePath);
                e.Cancel = true;
            }
        }
    }
}
