namespace xenonGPUViewer.View
{
    partial class ViewShaderGeneratedCode
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
            this.toolStrip1 = new System.Windows.Forms.ToolStrip();
            this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
            this.toolButtonSave = new System.Windows.Forms.ToolStripButton();
            this.tabBase = new System.Windows.Forms.TabControl();
            this.tabPage1 = new System.Windows.Forms.TabPage();
            this.shaderCode = new System.Windows.Forms.WebBrowser();
            this.toolStrip1.SuspendLayout();
            this.tabBase.SuspendLayout();
            this.tabPage1.SuspendLayout();
            this.SuspendLayout();
            // 
            // toolStrip1
            // 
            this.toolStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolButtonSave,
            this.toolStripSeparator1});
            this.toolStrip1.Location = new System.Drawing.Point(0, 0);
            this.toolStrip1.Name = "toolStrip1";
            this.toolStrip1.Size = new System.Drawing.Size(150, 25);
            this.toolStrip1.TabIndex = 2;
            this.toolStrip1.Text = "toolStrip1";
            // 
            // toolStripSeparator1
            // 
            this.toolStripSeparator1.Name = "toolStripSeparator1";
            this.toolStripSeparator1.Size = new System.Drawing.Size(6, 25);
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
            // tabBase
            // 
            this.tabBase.Controls.Add(this.tabPage1);
            this.tabBase.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tabBase.Location = new System.Drawing.Point(0, 25);
            this.tabBase.Name = "tabBase";
            this.tabBase.SelectedIndex = 0;
            this.tabBase.Size = new System.Drawing.Size(150, 125);
            this.tabBase.TabIndex = 3;
            // 
            // tabPage1
            // 
            this.tabPage1.Controls.Add(this.shaderCode);
            this.tabPage1.Location = new System.Drawing.Point(4, 22);
            this.tabPage1.Name = "tabPage1";
            this.tabPage1.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage1.Size = new System.Drawing.Size(142, 99);
            this.tabPage1.TabIndex = 0;
            this.tabPage1.Text = "Generated HLSL";
            this.tabPage1.UseVisualStyleBackColor = true;
            // 
            // shaderCode
            // 
            this.shaderCode.AllowNavigation = false;
            this.shaderCode.AllowWebBrowserDrop = false;
            this.shaderCode.Dock = System.Windows.Forms.DockStyle.Fill;
            this.shaderCode.Location = new System.Drawing.Point(3, 3);
            this.shaderCode.MinimumSize = new System.Drawing.Size(20, 20);
            this.shaderCode.Name = "shaderCode";
            this.shaderCode.Size = new System.Drawing.Size(136, 93);
            this.shaderCode.TabIndex = 0;
            // 
            // ViewShaderGeneratedCode
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.tabBase);
            this.Controls.Add(this.toolStrip1);
            this.Name = "ViewShaderGeneratedCode";
            this.toolStrip1.ResumeLayout(false);
            this.toolStrip1.PerformLayout();
            this.tabBase.ResumeLayout(false);
            this.tabPage1.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.ToolStrip toolStrip1;
        private System.Windows.Forms.ToolStripButton toolButtonSave;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
        private System.Windows.Forms.TabControl tabBase;
        private System.Windows.Forms.TabPage tabPage1;
        private System.Windows.Forms.WebBrowser shaderCode;
    }
}
