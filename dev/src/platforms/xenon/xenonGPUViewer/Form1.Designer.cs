namespace xenonGPUViewer
{
    partial class Form1
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(Form1));
            this.statusStrip1 = new System.Windows.Forms.StatusStrip();
            this.toolStrip1 = new System.Windows.Forms.ToolStrip();
            this.toolLoad = new System.Windows.Forms.ToolStripButton();
            this.splitContainer1 = new System.Windows.Forms.SplitContainer();
            this.tabTree = new System.Windows.Forms.TabControl();
            this.tabPage2 = new System.Windows.Forms.TabPage();
            this.treeDrawCalls = new System.Windows.Forms.TreeView();
            this.splitContainer2 = new System.Windows.Forms.SplitContainer();
            this.tabData = new System.Windows.Forms.TabControl();
            this.tabPage4 = new System.Windows.Forms.TabPage();
            this.packetBreakdown = new System.Windows.Forms.WebBrowser();
            this.tabPage3 = new System.Windows.Forms.TabPage();
            this.listAllRegs = new System.Windows.Forms.ListView();
            this.columnHeader1 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.columnHeader2 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.columnHeader3 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.columnHeader4 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.columnHeader5 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.tabDataViews = new System.Windows.Forms.TabControl();
            this.openDumpDialog = new System.Windows.Forms.OpenFileDialog();
            this.tabPage1 = new System.Windows.Forms.TabPage();
            this.drawCallDump = new System.Windows.Forms.WebBrowser();
            this.tabPage5 = new System.Windows.Forms.TabPage();
            this.dumpAllStates = new System.Windows.Forms.WebBrowser();
            this.tabPage6 = new System.Windows.Forms.TabPage();
            this.tabControl1 = new System.Windows.Forms.TabControl();
            this.tabPage7 = new System.Windows.Forms.TabPage();
            this.tabPage8 = new System.Windows.Forms.TabPage();
            this.listVertexConsts = new System.Windows.Forms.ListView();
            this.columnHeader6 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.columnHeader10 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.columnHeader11 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.columnHeader12 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.columnHeader13 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.columnHeader7 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.columnHeader8 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.listPixelConsts = new System.Windows.Forms.ListView();
            this.columnHeader9 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.columnHeader14 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.columnHeader15 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.columnHeader16 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.columnHeader17 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.columnHeader18 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.columnHeader19 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.toolStrip1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.splitContainer1)).BeginInit();
            this.splitContainer1.Panel1.SuspendLayout();
            this.splitContainer1.Panel2.SuspendLayout();
            this.splitContainer1.SuspendLayout();
            this.tabTree.SuspendLayout();
            this.tabPage2.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.splitContainer2)).BeginInit();
            this.splitContainer2.Panel1.SuspendLayout();
            this.splitContainer2.Panel2.SuspendLayout();
            this.splitContainer2.SuspendLayout();
            this.tabData.SuspendLayout();
            this.tabPage4.SuspendLayout();
            this.tabPage3.SuspendLayout();
            this.tabPage1.SuspendLayout();
            this.tabPage5.SuspendLayout();
            this.tabPage6.SuspendLayout();
            this.tabControl1.SuspendLayout();
            this.tabPage7.SuspendLayout();
            this.tabPage8.SuspendLayout();
            this.SuspendLayout();
            // 
            // statusStrip1
            // 
            this.statusStrip1.Location = new System.Drawing.Point(0, 663);
            this.statusStrip1.Name = "statusStrip1";
            this.statusStrip1.Size = new System.Drawing.Size(1287, 22);
            this.statusStrip1.TabIndex = 0;
            this.statusStrip1.Text = "statusStrip1";
            // 
            // toolStrip1
            // 
            this.toolStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolLoad});
            this.toolStrip1.Location = new System.Drawing.Point(0, 0);
            this.toolStrip1.Name = "toolStrip1";
            this.toolStrip1.Size = new System.Drawing.Size(1287, 25);
            this.toolStrip1.TabIndex = 1;
            this.toolStrip1.Text = "toolStrip1";
            // 
            // toolLoad
            // 
            this.toolLoad.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.toolLoad.Image = ((System.Drawing.Image)(resources.GetObject("toolLoad.Image")));
            this.toolLoad.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.toolLoad.Name = "toolLoad";
            this.toolLoad.Size = new System.Drawing.Size(23, 22);
            this.toolLoad.Text = "Load...";
            this.toolLoad.Click += new System.EventHandler(this.toolLoad_Click);
            // 
            // splitContainer1
            // 
            this.splitContainer1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.splitContainer1.Location = new System.Drawing.Point(0, 25);
            this.splitContainer1.Name = "splitContainer1";
            // 
            // splitContainer1.Panel1
            // 
            this.splitContainer1.Panel1.Controls.Add(this.tabTree);
            // 
            // splitContainer1.Panel2
            // 
            this.splitContainer1.Panel2.Controls.Add(this.splitContainer2);
            this.splitContainer1.Size = new System.Drawing.Size(1287, 638);
            this.splitContainer1.SplitterDistance = 429;
            this.splitContainer1.TabIndex = 2;
            // 
            // tabTree
            // 
            this.tabTree.Controls.Add(this.tabPage2);
            this.tabTree.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tabTree.Location = new System.Drawing.Point(0, 0);
            this.tabTree.Name = "tabTree";
            this.tabTree.SelectedIndex = 0;
            this.tabTree.Size = new System.Drawing.Size(429, 638);
            this.tabTree.TabIndex = 0;
            // 
            // tabPage2
            // 
            this.tabPage2.Controls.Add(this.treeDrawCalls);
            this.tabPage2.Location = new System.Drawing.Point(4, 22);
            this.tabPage2.Name = "tabPage2";
            this.tabPage2.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage2.Size = new System.Drawing.Size(421, 612);
            this.tabPage2.TabIndex = 1;
            this.tabPage2.Text = "Draw calls";
            this.tabPage2.UseVisualStyleBackColor = true;
            // 
            // treeDrawCalls
            // 
            this.treeDrawCalls.Dock = System.Windows.Forms.DockStyle.Fill;
            this.treeDrawCalls.Location = new System.Drawing.Point(3, 3);
            this.treeDrawCalls.Name = "treeDrawCalls";
            this.treeDrawCalls.Size = new System.Drawing.Size(415, 606);
            this.treeDrawCalls.TabIndex = 0;
            this.treeDrawCalls.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.treeDrawCalls_AfterSelect);
            // 
            // splitContainer2
            // 
            this.splitContainer2.Dock = System.Windows.Forms.DockStyle.Fill;
            this.splitContainer2.Location = new System.Drawing.Point(0, 0);
            this.splitContainer2.Name = "splitContainer2";
            this.splitContainer2.Orientation = System.Windows.Forms.Orientation.Horizontal;
            // 
            // splitContainer2.Panel1
            // 
            this.splitContainer2.Panel1.Controls.Add(this.tabData);
            // 
            // splitContainer2.Panel2
            // 
            this.splitContainer2.Panel2.Controls.Add(this.tabDataViews);
            this.splitContainer2.Size = new System.Drawing.Size(854, 638);
            this.splitContainer2.SplitterDistance = 284;
            this.splitContainer2.TabIndex = 0;
            // 
            // tabData
            // 
            this.tabData.Controls.Add(this.tabPage1);
            this.tabData.Controls.Add(this.tabPage4);
            this.tabData.Controls.Add(this.tabPage3);
            this.tabData.Controls.Add(this.tabPage5);
            this.tabData.Controls.Add(this.tabPage6);
            this.tabData.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tabData.Location = new System.Drawing.Point(0, 0);
            this.tabData.Name = "tabData";
            this.tabData.SelectedIndex = 0;
            this.tabData.Size = new System.Drawing.Size(854, 284);
            this.tabData.TabIndex = 1;
            // 
            // tabPage4
            // 
            this.tabPage4.Controls.Add(this.packetBreakdown);
            this.tabPage4.Location = new System.Drawing.Point(4, 22);
            this.tabPage4.Name = "tabPage4";
            this.tabPage4.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage4.Size = new System.Drawing.Size(846, 258);
            this.tabPage4.TabIndex = 1;
            this.tabPage4.Text = "Packets";
            this.tabPage4.UseVisualStyleBackColor = true;
            // 
            // packetBreakdown
            // 
            this.packetBreakdown.AllowWebBrowserDrop = false;
            this.packetBreakdown.Dock = System.Windows.Forms.DockStyle.Fill;
            this.packetBreakdown.IsWebBrowserContextMenuEnabled = false;
            this.packetBreakdown.Location = new System.Drawing.Point(3, 3);
            this.packetBreakdown.MinimumSize = new System.Drawing.Size(20, 20);
            this.packetBreakdown.Name = "packetBreakdown";
            this.packetBreakdown.Size = new System.Drawing.Size(840, 252);
            this.packetBreakdown.TabIndex = 0;
            this.packetBreakdown.Navigating += new System.Windows.Forms.WebBrowserNavigatingEventHandler(this.packetBreakdown_Navigating);
            // 
            // tabPage3
            // 
            this.tabPage3.Controls.Add(this.listAllRegs);
            this.tabPage3.Location = new System.Drawing.Point(4, 22);
            this.tabPage3.Name = "tabPage3";
            this.tabPage3.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage3.Size = new System.Drawing.Size(846, 258);
            this.tabPage3.TabIndex = 0;
            this.tabPage3.Text = "All registers";
            this.tabPage3.UseVisualStyleBackColor = true;
            // 
            // listAllRegs
            // 
            this.listAllRegs.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.columnHeader1,
            this.columnHeader2,
            this.columnHeader3,
            this.columnHeader4,
            this.columnHeader5});
            this.listAllRegs.Dock = System.Windows.Forms.DockStyle.Fill;
            this.listAllRegs.Location = new System.Drawing.Point(3, 3);
            this.listAllRegs.Name = "listAllRegs";
            this.listAllRegs.Size = new System.Drawing.Size(840, 252);
            this.listAllRegs.TabIndex = 0;
            this.listAllRegs.UseCompatibleStateImageBehavior = false;
            this.listAllRegs.View = System.Windows.Forms.View.Details;
            // 
            // columnHeader1
            // 
            this.columnHeader1.Text = "Index";
            // 
            // columnHeader2
            // 
            this.columnHeader2.Text = "Name";
            this.columnHeader2.Width = 400;
            // 
            // columnHeader3
            // 
            this.columnHeader3.Text = "Value (Int)";
            this.columnHeader3.Width = 80;
            // 
            // columnHeader4
            // 
            this.columnHeader4.Text = "Value (Hex)";
            this.columnHeader4.Width = 80;
            // 
            // columnHeader5
            // 
            this.columnHeader5.Text = "Value (Float)";
            this.columnHeader5.Width = 80;
            // 
            // tabDataViews
            // 
            this.tabDataViews.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tabDataViews.Location = new System.Drawing.Point(0, 0);
            this.tabDataViews.Name = "tabDataViews";
            this.tabDataViews.SelectedIndex = 0;
            this.tabDataViews.Size = new System.Drawing.Size(854, 350);
            this.tabDataViews.TabIndex = 0;
            // 
            // openDumpDialog
            // 
            this.openDumpDialog.DefaultExt = "gpu";
            this.openDumpDialog.Filter = "Xenon GPU Dumps [*.gpu]|*.gpu";
            // 
            // tabPage1
            // 
            this.tabPage1.Controls.Add(this.drawCallDump);
            this.tabPage1.Location = new System.Drawing.Point(4, 22);
            this.tabPage1.Name = "tabPage1";
            this.tabPage1.Size = new System.Drawing.Size(846, 258);
            this.tabPage1.TabIndex = 2;
            this.tabPage1.Text = "Draw call";
            this.tabPage1.UseVisualStyleBackColor = true;
            // 
            // drawCallDump
            // 
            this.drawCallDump.Dock = System.Windows.Forms.DockStyle.Fill;
            this.drawCallDump.Location = new System.Drawing.Point(0, 0);
            this.drawCallDump.MinimumSize = new System.Drawing.Size(20, 20);
            this.drawCallDump.Name = "drawCallDump";
            this.drawCallDump.Size = new System.Drawing.Size(846, 258);
            this.drawCallDump.TabIndex = 0;
            this.drawCallDump.Navigating += new System.Windows.Forms.WebBrowserNavigatingEventHandler(this.packetBreakdown_Navigating);
            // 
            // tabPage5
            // 
            this.tabPage5.Controls.Add(this.dumpAllStates);
            this.tabPage5.Location = new System.Drawing.Point(4, 22);
            this.tabPage5.Name = "tabPage5";
            this.tabPage5.Size = new System.Drawing.Size(846, 258);
            this.tabPage5.TabIndex = 3;
            this.tabPage5.Text = "All states";
            this.tabPage5.UseVisualStyleBackColor = true;
            // 
            // dumpAllStates
            // 
            this.dumpAllStates.Dock = System.Windows.Forms.DockStyle.Fill;
            this.dumpAllStates.Location = new System.Drawing.Point(0, 0);
            this.dumpAllStates.MinimumSize = new System.Drawing.Size(20, 20);
            this.dumpAllStates.Name = "dumpAllStates";
            this.dumpAllStates.Size = new System.Drawing.Size(846, 258);
            this.dumpAllStates.TabIndex = 0;
            this.dumpAllStates.Navigating += new System.Windows.Forms.WebBrowserNavigatingEventHandler(this.packetBreakdown_Navigating);
            // 
            // tabPage6
            // 
            this.tabPage6.Controls.Add(this.tabControl1);
            this.tabPage6.Location = new System.Drawing.Point(4, 22);
            this.tabPage6.Name = "tabPage6";
            this.tabPage6.Size = new System.Drawing.Size(846, 258);
            this.tabPage6.TabIndex = 4;
            this.tabPage6.Text = "Used constants";
            this.tabPage6.UseVisualStyleBackColor = true;
            // 
            // tabControl1
            // 
            this.tabControl1.Controls.Add(this.tabPage7);
            this.tabControl1.Controls.Add(this.tabPage8);
            this.tabControl1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tabControl1.Location = new System.Drawing.Point(0, 0);
            this.tabControl1.Name = "tabControl1";
            this.tabControl1.SelectedIndex = 0;
            this.tabControl1.Size = new System.Drawing.Size(846, 258);
            this.tabControl1.TabIndex = 0;
            // 
            // tabPage7
            // 
            this.tabPage7.Controls.Add(this.listVertexConsts);
            this.tabPage7.Location = new System.Drawing.Point(4, 22);
            this.tabPage7.Name = "tabPage7";
            this.tabPage7.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage7.Size = new System.Drawing.Size(838, 232);
            this.tabPage7.TabIndex = 0;
            this.tabPage7.Text = "Vertex constants";
            this.tabPage7.UseVisualStyleBackColor = true;
            // 
            // tabPage8
            // 
            this.tabPage8.Controls.Add(this.listPixelConsts);
            this.tabPage8.Location = new System.Drawing.Point(4, 22);
            this.tabPage8.Name = "tabPage8";
            this.tabPage8.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage8.Size = new System.Drawing.Size(838, 232);
            this.tabPage8.TabIndex = 1;
            this.tabPage8.Text = "Pixel constants";
            this.tabPage8.UseVisualStyleBackColor = true;
            // 
            // listVertexConsts
            // 
            this.listVertexConsts.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.columnHeader6,
            this.columnHeader10,
            this.columnHeader11,
            this.columnHeader12,
            this.columnHeader13,
            this.columnHeader7,
            this.columnHeader8});
            this.listVertexConsts.Dock = System.Windows.Forms.DockStyle.Fill;
            this.listVertexConsts.Location = new System.Drawing.Point(3, 3);
            this.listVertexConsts.Name = "listVertexConsts";
            this.listVertexConsts.Size = new System.Drawing.Size(832, 226);
            this.listVertexConsts.TabIndex = 1;
            this.listVertexConsts.UseCompatibleStateImageBehavior = false;
            this.listVertexConsts.View = System.Windows.Forms.View.Details;
            // 
            // columnHeader6
            // 
            this.columnHeader6.Text = "Index";
            // 
            // columnHeader10
            // 
            this.columnHeader10.Text = "X";
            this.columnHeader10.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            this.columnHeader10.Width = 80;
            // 
            // columnHeader11
            // 
            this.columnHeader11.Text = "Y";
            this.columnHeader11.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            this.columnHeader11.Width = 80;
            // 
            // columnHeader12
            // 
            this.columnHeader12.Text = "Z";
            this.columnHeader12.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            this.columnHeader12.Width = 80;
            // 
            // columnHeader13
            // 
            this.columnHeader13.Text = "W";
            this.columnHeader13.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            this.columnHeader13.Width = 80;
            // 
            // columnHeader7
            // 
            this.columnHeader7.Text = "RGBA";
            this.columnHeader7.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            this.columnHeader7.Width = 90;
            // 
            // columnHeader8
            // 
            this.columnHeader8.Text = "Color";
            this.columnHeader8.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            this.columnHeader8.Width = 90;
            // 
            // listPixelConsts
            // 
            this.listPixelConsts.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.columnHeader9,
            this.columnHeader14,
            this.columnHeader15,
            this.columnHeader16,
            this.columnHeader17,
            this.columnHeader18,
            this.columnHeader19});
            this.listPixelConsts.Dock = System.Windows.Forms.DockStyle.Fill;
            this.listPixelConsts.Location = new System.Drawing.Point(3, 3);
            this.listPixelConsts.Name = "listPixelConsts";
            this.listPixelConsts.Size = new System.Drawing.Size(832, 226);
            this.listPixelConsts.TabIndex = 2;
            this.listPixelConsts.UseCompatibleStateImageBehavior = false;
            this.listPixelConsts.View = System.Windows.Forms.View.Details;
            // 
            // columnHeader9
            // 
            this.columnHeader9.Text = "Index";
            // 
            // columnHeader14
            // 
            this.columnHeader14.Text = "X";
            this.columnHeader14.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            this.columnHeader14.Width = 80;
            // 
            // columnHeader15
            // 
            this.columnHeader15.Text = "Y";
            this.columnHeader15.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            this.columnHeader15.Width = 80;
            // 
            // columnHeader16
            // 
            this.columnHeader16.Text = "Z";
            this.columnHeader16.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            this.columnHeader16.Width = 80;
            // 
            // columnHeader17
            // 
            this.columnHeader17.Text = "W";
            this.columnHeader17.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            this.columnHeader17.Width = 80;
            // 
            // columnHeader18
            // 
            this.columnHeader18.Text = "RGBA";
            this.columnHeader18.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            this.columnHeader18.Width = 90;
            // 
            // columnHeader19
            // 
            this.columnHeader19.Text = "Color";
            this.columnHeader19.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            this.columnHeader19.Width = 90;
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(1287, 685);
            this.Controls.Add(this.splitContainer1);
            this.Controls.Add(this.toolStrip1);
            this.Controls.Add(this.statusStrip1);
            this.Name = "Form1";
            this.Text = "Xbox 360 Emulator by RexDex - GPU Trace Dump Viewer";
            this.toolStrip1.ResumeLayout(false);
            this.toolStrip1.PerformLayout();
            this.splitContainer1.Panel1.ResumeLayout(false);
            this.splitContainer1.Panel2.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.splitContainer1)).EndInit();
            this.splitContainer1.ResumeLayout(false);
            this.tabTree.ResumeLayout(false);
            this.tabPage2.ResumeLayout(false);
            this.splitContainer2.Panel1.ResumeLayout(false);
            this.splitContainer2.Panel2.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.splitContainer2)).EndInit();
            this.splitContainer2.ResumeLayout(false);
            this.tabData.ResumeLayout(false);
            this.tabPage4.ResumeLayout(false);
            this.tabPage3.ResumeLayout(false);
            this.tabPage1.ResumeLayout(false);
            this.tabPage5.ResumeLayout(false);
            this.tabPage6.ResumeLayout(false);
            this.tabControl1.ResumeLayout(false);
            this.tabPage7.ResumeLayout(false);
            this.tabPage8.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.StatusStrip statusStrip1;
        private System.Windows.Forms.ToolStrip toolStrip1;
        private System.Windows.Forms.ToolStripButton toolLoad;
        private System.Windows.Forms.SplitContainer splitContainer1;
        private System.Windows.Forms.TabControl tabTree;
        private System.Windows.Forms.TabPage tabPage2;
        private System.Windows.Forms.OpenFileDialog openDumpDialog;
        private System.Windows.Forms.TreeView treeDrawCalls;
        private System.Windows.Forms.SplitContainer splitContainer2;
        private System.Windows.Forms.TabControl tabData;
        private System.Windows.Forms.TabPage tabPage4;
        private System.Windows.Forms.WebBrowser packetBreakdown;
        private System.Windows.Forms.TabPage tabPage3;
        private System.Windows.Forms.ListView listAllRegs;
        private System.Windows.Forms.ColumnHeader columnHeader1;
        private System.Windows.Forms.ColumnHeader columnHeader2;
        private System.Windows.Forms.ColumnHeader columnHeader3;
        private System.Windows.Forms.ColumnHeader columnHeader4;
        private System.Windows.Forms.ColumnHeader columnHeader5;
        private System.Windows.Forms.TabControl tabDataViews;
        private System.Windows.Forms.TabPage tabPage1;
        private System.Windows.Forms.WebBrowser drawCallDump;
        private System.Windows.Forms.TabPage tabPage5;
        private System.Windows.Forms.WebBrowser dumpAllStates;
        private System.Windows.Forms.TabPage tabPage6;
        private System.Windows.Forms.TabControl tabControl1;
        private System.Windows.Forms.TabPage tabPage7;
        private System.Windows.Forms.ListView listVertexConsts;
        private System.Windows.Forms.ColumnHeader columnHeader6;
        private System.Windows.Forms.ColumnHeader columnHeader10;
        private System.Windows.Forms.ColumnHeader columnHeader11;
        private System.Windows.Forms.ColumnHeader columnHeader12;
        private System.Windows.Forms.ColumnHeader columnHeader13;
        private System.Windows.Forms.ColumnHeader columnHeader7;
        private System.Windows.Forms.ColumnHeader columnHeader8;
        private System.Windows.Forms.TabPage tabPage8;
        private System.Windows.Forms.ListView listPixelConsts;
        private System.Windows.Forms.ColumnHeader columnHeader9;
        private System.Windows.Forms.ColumnHeader columnHeader14;
        private System.Windows.Forms.ColumnHeader columnHeader15;
        private System.Windows.Forms.ColumnHeader columnHeader16;
        private System.Windows.Forms.ColumnHeader columnHeader17;
        private System.Windows.Forms.ColumnHeader columnHeader18;
        private System.Windows.Forms.ColumnHeader columnHeader19;
    }
}

