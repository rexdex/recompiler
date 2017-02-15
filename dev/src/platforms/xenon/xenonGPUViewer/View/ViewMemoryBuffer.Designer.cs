namespace xenonGPUViewer.View
{
    partial class ViewMemoryBuffer
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

        #region Component Designer generated code

        /// <summary> 
        /// Required method for Designer support - do not modify 
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            System.Windows.Forms.ListViewItem listViewItem1 = new System.Windows.Forms.ListViewItem(new string[] {
            "0x500000",
            "0x0000",
            "00 00 00 00 FF FF FF FF 00 00 00 00 FF FF FF FF"}, -1);
            this.toolStrip1 = new System.Windows.Forms.ToolStrip();
            this.toolButtonSave = new System.Windows.Forms.ToolStripButton();
            this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
            this.tabsViews = new System.Windows.Forms.TabControl();
            this.tabFormatedView = new System.Windows.Forms.TabPage();
            this.listViewBuffer = new System.Windows.Forms.ListView();
            this.tabRawView = new System.Windows.Forms.TabPage();
            this.listViewRaw = new System.Windows.Forms.ListView();
            this.columnHeader1 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.columnHeader2 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.tabSetup = new System.Windows.Forms.TabPage();
            this.propertyGrid1 = new System.Windows.Forms.PropertyGrid();
            this.columnHeader3 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.toolStrip1.SuspendLayout();
            this.tabsViews.SuspendLayout();
            this.tabFormatedView.SuspendLayout();
            this.tabRawView.SuspendLayout();
            this.tabSetup.SuspendLayout();
            this.SuspendLayout();
            // 
            // toolStrip1
            // 
            this.toolStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolButtonSave,
            this.toolStripSeparator1});
            this.toolStrip1.Location = new System.Drawing.Point(0, 0);
            this.toolStrip1.Name = "toolStrip1";
            this.toolStrip1.Size = new System.Drawing.Size(648, 25);
            this.toolStrip1.TabIndex = 0;
            this.toolStrip1.Text = "toolStrip1";
            // 
            // toolButtonSave
            // 
            this.toolButtonSave.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.toolButtonSave.Image = global::xenonGPUViewer.Properties.Resources.save;
            this.toolButtonSave.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.toolButtonSave.Name = "toolButtonSave";
            this.toolButtonSave.Size = new System.Drawing.Size(23, 22);
            this.toolButtonSave.Text = "Export...";
            // 
            // toolStripSeparator1
            // 
            this.toolStripSeparator1.Name = "toolStripSeparator1";
            this.toolStripSeparator1.Size = new System.Drawing.Size(6, 25);
            // 
            // tabsViews
            // 
            this.tabsViews.Controls.Add(this.tabFormatedView);
            this.tabsViews.Controls.Add(this.tabRawView);
            this.tabsViews.Controls.Add(this.tabSetup);
            this.tabsViews.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tabsViews.Location = new System.Drawing.Point(0, 25);
            this.tabsViews.Name = "tabsViews";
            this.tabsViews.SelectedIndex = 0;
            this.tabsViews.Size = new System.Drawing.Size(648, 474);
            this.tabsViews.TabIndex = 1;
            // 
            // tabFormatedView
            // 
            this.tabFormatedView.Controls.Add(this.listViewBuffer);
            this.tabFormatedView.Location = new System.Drawing.Point(4, 22);
            this.tabFormatedView.Name = "tabFormatedView";
            this.tabFormatedView.Padding = new System.Windows.Forms.Padding(3);
            this.tabFormatedView.Size = new System.Drawing.Size(640, 448);
            this.tabFormatedView.TabIndex = 0;
            this.tabFormatedView.Text = "Buffer view";
            this.tabFormatedView.UseVisualStyleBackColor = true;
            // 
            // listViewBuffer
            // 
            this.listViewBuffer.Dock = System.Windows.Forms.DockStyle.Fill;
            this.listViewBuffer.Font = new System.Drawing.Font("Courier New", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(238)));
            this.listViewBuffer.Location = new System.Drawing.Point(3, 3);
            this.listViewBuffer.Name = "listViewBuffer";
            this.listViewBuffer.Size = new System.Drawing.Size(634, 442);
            this.listViewBuffer.TabIndex = 0;
            this.listViewBuffer.UseCompatibleStateImageBehavior = false;
            this.listViewBuffer.View = System.Windows.Forms.View.Details;
            // 
            // tabRawView
            // 
            this.tabRawView.Controls.Add(this.listViewRaw);
            this.tabRawView.Location = new System.Drawing.Point(4, 22);
            this.tabRawView.Name = "tabRawView";
            this.tabRawView.Padding = new System.Windows.Forms.Padding(3);
            this.tabRawView.Size = new System.Drawing.Size(640, 448);
            this.tabRawView.TabIndex = 1;
            this.tabRawView.Text = "Raw view";
            this.tabRawView.UseVisualStyleBackColor = true;
            // 
            // listViewRaw
            // 
            this.listViewRaw.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.columnHeader3,
            this.columnHeader1,
            this.columnHeader2});
            this.listViewRaw.Dock = System.Windows.Forms.DockStyle.Fill;
            this.listViewRaw.Font = new System.Drawing.Font("Courier New", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(238)));
            this.listViewRaw.Items.AddRange(new System.Windows.Forms.ListViewItem[] {
            listViewItem1});
            this.listViewRaw.Location = new System.Drawing.Point(3, 3);
            this.listViewRaw.Name = "listViewRaw";
            this.listViewRaw.Size = new System.Drawing.Size(634, 442);
            this.listViewRaw.TabIndex = 0;
            this.listViewRaw.UseCompatibleStateImageBehavior = false;
            this.listViewRaw.View = System.Windows.Forms.View.Details;
            // 
            // columnHeader1
            // 
            this.columnHeader1.Text = "Offset";
            this.columnHeader1.Width = 80;
            // 
            // columnHeader2
            // 
            this.columnHeader2.Text = "Data";
            this.columnHeader2.Width = 700;
            // 
            // tabSetup
            // 
            this.tabSetup.Controls.Add(this.propertyGrid1);
            this.tabSetup.Location = new System.Drawing.Point(4, 22);
            this.tabSetup.Name = "tabSetup";
            this.tabSetup.Size = new System.Drawing.Size(640, 448);
            this.tabSetup.TabIndex = 2;
            this.tabSetup.Text = "Format";
            this.tabSetup.UseVisualStyleBackColor = true;
            // 
            // propertyGrid1
            // 
            this.propertyGrid1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.propertyGrid1.Location = new System.Drawing.Point(0, 0);
            this.propertyGrid1.Name = "propertyGrid1";
            this.propertyGrid1.Size = new System.Drawing.Size(640, 448);
            this.propertyGrid1.TabIndex = 0;
            // 
            // columnHeader3
            // 
            this.columnHeader3.Text = "Address";
            this.columnHeader3.Width = 80;
            // 
            // ViewMemoryBuffer
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.tabsViews);
            this.Controls.Add(this.toolStrip1);
            this.Name = "ViewMemoryBuffer";
            this.Size = new System.Drawing.Size(648, 499);
            this.toolStrip1.ResumeLayout(false);
            this.toolStrip1.PerformLayout();
            this.tabsViews.ResumeLayout(false);
            this.tabFormatedView.ResumeLayout(false);
            this.tabRawView.ResumeLayout(false);
            this.tabSetup.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.ToolStrip toolStrip1;
        private System.Windows.Forms.ToolStripButton toolButtonSave;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
        private System.Windows.Forms.TabControl tabsViews;
        private System.Windows.Forms.TabPage tabFormatedView;
        private System.Windows.Forms.ListView listViewBuffer;
        private System.Windows.Forms.TabPage tabRawView;
        private System.Windows.Forms.ListView listViewRaw;
        private System.Windows.Forms.ColumnHeader columnHeader1;
        private System.Windows.Forms.ColumnHeader columnHeader2;
        private System.Windows.Forms.TabPage tabSetup;
        private System.Windows.Forms.PropertyGrid propertyGrid1;
        private System.Windows.Forms.ColumnHeader columnHeader3;
    }
}
