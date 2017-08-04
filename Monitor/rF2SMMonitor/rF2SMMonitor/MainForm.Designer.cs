/*
MainForm Designer part of file.

Author: The Iron Wolf (vleonavicius@hotmail.com)
Website: thecrewchief.org
*/
namespace rF2SMMonitor
{
    partial class MainForm
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
      this.view = new System.Windows.Forms.PictureBox();
      this.scaleLabel = new System.Windows.Forms.Label();
      this.scaleTextBox = new System.Windows.Forms.TextBox();
      this.focusVehLabel = new System.Windows.Forms.Label();
      this.focusVehTextBox = new System.Windows.Forms.TextBox();
      this.setAsOriginCheckBox = new System.Windows.Forms.CheckBox();
      this.groupBoxFocus = new System.Windows.Forms.GroupBox();
      this.rotateAroundCheckBox = new System.Windows.Forms.CheckBox();
      this.globalGroupBox = new System.Windows.Forms.GroupBox();
      this.yOffsetTextBox = new System.Windows.Forms.TextBox();
      this.yOffsetLabel = new System.Windows.Forms.Label();
      this.xOffsetTextBox = new System.Windows.Forms.TextBox();
      this.xOffsetLabel = new System.Windows.Forms.Label();
      this.groupBoxLogging = new System.Windows.Forms.GroupBox();
      this.checkBoxLogTiming = new System.Windows.Forms.CheckBox();
      this.checkBoxLogDamage = new System.Windows.Forms.CheckBox();
      this.checkBoxLogPhaseAndState = new System.Windows.Forms.CheckBox();
      this.checkBoxLightMode = new System.Windows.Forms.CheckBox();
      this.checkBoxLogRules = new System.Windows.Forms.CheckBox();
      ((System.ComponentModel.ISupportInitialize)(this.view)).BeginInit();
      this.groupBoxFocus.SuspendLayout();
      this.globalGroupBox.SuspendLayout();
      this.groupBoxLogging.SuspendLayout();
      this.SuspendLayout();
      // 
      // view
      // 
      this.view.Location = new System.Drawing.Point(-1, 56);
      this.view.Name = "view";
      this.view.Size = new System.Drawing.Size(1902, 975);
      this.view.TabIndex = 0;
      this.view.TabStop = false;
      // 
      // scaleLabel
      // 
      this.scaleLabel.AutoSize = true;
      this.scaleLabel.Location = new System.Drawing.Point(6, 17);
      this.scaleLabel.Name = "scaleLabel";
      this.scaleLabel.Size = new System.Drawing.Size(37, 13);
      this.scaleLabel.TabIndex = 1;
      this.scaleLabel.Text = "Scale:";
      // 
      // scaleTextBox
      // 
      this.scaleTextBox.AcceptsReturn = true;
      this.scaleTextBox.Location = new System.Drawing.Point(49, 14);
      this.scaleTextBox.Name = "scaleTextBox";
      this.scaleTextBox.Size = new System.Drawing.Size(61, 20);
      this.scaleTextBox.TabIndex = 2;
      // 
      // focusVehLabel
      // 
      this.focusVehLabel.AutoSize = true;
      this.focusVehLabel.Location = new System.Drawing.Point(7, 18);
      this.focusVehLabel.Name = "focusVehLabel";
      this.focusVehLabel.Size = new System.Drawing.Size(55, 13);
      this.focusVehLabel.TabIndex = 3;
      this.focusVehLabel.Text = "Vehicle #:";
      // 
      // focusVehTextBox
      // 
      this.focusVehTextBox.Location = new System.Drawing.Point(67, 16);
      this.focusVehTextBox.Name = "focusVehTextBox";
      this.focusVehTextBox.Size = new System.Drawing.Size(54, 20);
      this.focusVehTextBox.TabIndex = 4;
      // 
      // setAsOriginCheckBox
      // 
      this.setAsOriginCheckBox.AutoSize = true;
      this.setAsOriginCheckBox.Location = new System.Drawing.Point(129, 13);
      this.setAsOriginCheckBox.Name = "setAsOriginCheckBox";
      this.setAsOriginCheckBox.Size = new System.Drawing.Size(86, 17);
      this.setAsOriginCheckBox.TabIndex = 6;
      this.setAsOriginCheckBox.Text = "Set as Origin";
      this.setAsOriginCheckBox.UseVisualStyleBackColor = true;
      // 
      // groupBoxFocus
      // 
      this.groupBoxFocus.Controls.Add(this.rotateAroundCheckBox);
      this.groupBoxFocus.Controls.Add(this.focusVehTextBox);
      this.groupBoxFocus.Controls.Add(this.setAsOriginCheckBox);
      this.groupBoxFocus.Controls.Add(this.focusVehLabel);
      this.groupBoxFocus.Location = new System.Drawing.Point(433, -1);
      this.groupBoxFocus.Name = "groupBoxFocus";
      this.groupBoxFocus.Size = new System.Drawing.Size(290, 54);
      this.groupBoxFocus.TabIndex = 7;
      this.groupBoxFocus.TabStop = false;
      this.groupBoxFocus.Text = "Focus";
      // 
      // rotateAroundCheckBox
      // 
      this.rotateAroundCheckBox.AutoSize = true;
      this.rotateAroundCheckBox.Location = new System.Drawing.Point(129, 32);
      this.rotateAroundCheckBox.Name = "rotateAroundCheckBox";
      this.rotateAroundCheckBox.Size = new System.Drawing.Size(129, 17);
      this.rotateAroundCheckBox.TabIndex = 8;
      this.rotateAroundCheckBox.Text = "Set as Rotation Origin";
      this.rotateAroundCheckBox.UseVisualStyleBackColor = true;
      // 
      // globalGroupBox
      // 
      this.globalGroupBox.Controls.Add(this.yOffsetTextBox);
      this.globalGroupBox.Controls.Add(this.yOffsetLabel);
      this.globalGroupBox.Controls.Add(this.xOffsetTextBox);
      this.globalGroupBox.Controls.Add(this.xOffsetLabel);
      this.globalGroupBox.Controls.Add(this.scaleTextBox);
      this.globalGroupBox.Controls.Add(this.scaleLabel);
      this.globalGroupBox.Location = new System.Drawing.Point(89, -1);
      this.globalGroupBox.Name = "globalGroupBox";
      this.globalGroupBox.Size = new System.Drawing.Size(335, 54);
      this.globalGroupBox.TabIndex = 8;
      this.globalGroupBox.TabStop = false;
      this.globalGroupBox.Text = "Global";
      // 
      // yOffsetTextBox
      // 
      this.yOffsetTextBox.Location = new System.Drawing.Point(272, 14);
      this.yOffsetTextBox.Name = "yOffsetTextBox";
      this.yOffsetTextBox.Size = new System.Drawing.Size(51, 20);
      this.yOffsetTextBox.TabIndex = 6;
      // 
      // yOffsetLabel
      // 
      this.yOffsetLabel.AutoSize = true;
      this.yOffsetLabel.Location = new System.Drawing.Point(226, 18);
      this.yOffsetLabel.Name = "yOffsetLabel";
      this.yOffsetLabel.Size = new System.Drawing.Size(44, 13);
      this.yOffsetLabel.TabIndex = 5;
      this.yOffsetLabel.Text = "y offset:";
      // 
      // xOffsetTextBox
      // 
      this.xOffsetTextBox.Location = new System.Drawing.Point(169, 14);
      this.xOffsetTextBox.Name = "xOffsetTextBox";
      this.xOffsetTextBox.Size = new System.Drawing.Size(51, 20);
      this.xOffsetTextBox.TabIndex = 4;
      // 
      // xOffsetLabel
      // 
      this.xOffsetLabel.AutoSize = true;
      this.xOffsetLabel.Location = new System.Drawing.Point(123, 18);
      this.xOffsetLabel.Name = "xOffsetLabel";
      this.xOffsetLabel.Size = new System.Drawing.Size(44, 13);
      this.xOffsetLabel.TabIndex = 3;
      this.xOffsetLabel.Text = "x offset:";
      // 
      // groupBoxLogging
      // 
      this.groupBoxLogging.Controls.Add(this.checkBoxLogRules);
      this.groupBoxLogging.Controls.Add(this.checkBoxLogTiming);
      this.groupBoxLogging.Controls.Add(this.checkBoxLogDamage);
      this.groupBoxLogging.Controls.Add(this.checkBoxLogPhaseAndState);
      this.groupBoxLogging.Location = new System.Drawing.Point(730, -1);
      this.groupBoxLogging.Name = "groupBoxLogging";
      this.groupBoxLogging.Size = new System.Drawing.Size(256, 54);
      this.groupBoxLogging.TabIndex = 9;
      this.groupBoxLogging.TabStop = false;
      this.groupBoxLogging.Text = "File Logging";
      // 
      // checkBoxLogTiming
      // 
      this.checkBoxLogTiming.AutoSize = true;
      this.checkBoxLogTiming.Location = new System.Drawing.Point(109, 13);
      this.checkBoxLogTiming.Name = "checkBoxLogTiming";
      this.checkBoxLogTiming.Size = new System.Drawing.Size(57, 17);
      this.checkBoxLogTiming.TabIndex = 11;
      this.checkBoxLogTiming.Text = "Timing";
      this.checkBoxLogTiming.UseVisualStyleBackColor = true;
      // 
      // checkBoxLogDamage
      // 
      this.checkBoxLogDamage.AutoSize = true;
      this.checkBoxLogDamage.Location = new System.Drawing.Point(7, 32);
      this.checkBoxLogDamage.Name = "checkBoxLogDamage";
      this.checkBoxLogDamage.Size = new System.Drawing.Size(66, 17);
      this.checkBoxLogDamage.TabIndex = 10;
      this.checkBoxLogDamage.Text = "Damage";
      this.checkBoxLogDamage.UseVisualStyleBackColor = true;
      // 
      // checkBoxLogPhaseAndState
      // 
      this.checkBoxLogPhaseAndState.AutoSize = true;
      this.checkBoxLogPhaseAndState.Location = new System.Drawing.Point(7, 13);
      this.checkBoxLogPhaseAndState.Name = "checkBoxLogPhaseAndState";
      this.checkBoxLogPhaseAndState.Size = new System.Drawing.Size(105, 17);
      this.checkBoxLogPhaseAndState.TabIndex = 9;
      this.checkBoxLogPhaseAndState.Text = "Phase and State";
      this.checkBoxLogPhaseAndState.UseVisualStyleBackColor = true;
      // 
      // checkBoxLightMode
      // 
      this.checkBoxLightMode.AutoSize = true;
      this.checkBoxLightMode.Location = new System.Drawing.Point(5, 6);
      this.checkBoxLightMode.Name = "checkBoxLightMode";
      this.checkBoxLightMode.Size = new System.Drawing.Size(78, 17);
      this.checkBoxLightMode.TabIndex = 11;
      this.checkBoxLightMode.Text = "Light mode";
      this.checkBoxLightMode.UseVisualStyleBackColor = true;
      // 
      // checkBox1
      // 
      this.checkBoxLogRules.AutoSize = true;
      this.checkBoxLogRules.Location = new System.Drawing.Point(109, 32);
      this.checkBoxLogRules.Name = "checkBoxLogRules";
      this.checkBoxLogRules.Size = new System.Drawing.Size(57, 17);
      this.checkBoxLogRules.TabIndex = 12;
      this.checkBoxLogRules.Text = "Rules";
      this.checkBoxLogRules.UseVisualStyleBackColor = true;
      // 
      // MainForm
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.ClientSize = new System.Drawing.Size(1902, 1033);
      this.Controls.Add(this.checkBoxLightMode);
      this.Controls.Add(this.groupBoxLogging);
      this.Controls.Add(this.globalGroupBox);
      this.Controls.Add(this.groupBoxFocus);
      this.Controls.Add(this.view);
      this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.Fixed3D;
      this.MaximizeBox = false;
      this.Name = "MainForm";
      this.Text = "rF2 Shared Memory Monitor";
      ((System.ComponentModel.ISupportInitialize)(this.view)).EndInit();
      this.groupBoxFocus.ResumeLayout(false);
      this.groupBoxFocus.PerformLayout();
      this.globalGroupBox.ResumeLayout(false);
      this.globalGroupBox.PerformLayout();
      this.groupBoxLogging.ResumeLayout(false);
      this.groupBoxLogging.PerformLayout();
      this.ResumeLayout(false);
      this.PerformLayout();

        }

    #endregion

    private System.Windows.Forms.PictureBox view;
    private System.Windows.Forms.Label scaleLabel;
    private System.Windows.Forms.TextBox scaleTextBox;
    private System.Windows.Forms.Label focusVehLabel;
    private System.Windows.Forms.TextBox focusVehTextBox;
    private System.Windows.Forms.CheckBox setAsOriginCheckBox;
    private System.Windows.Forms.GroupBox groupBoxFocus;
    private System.Windows.Forms.CheckBox rotateAroundCheckBox
;
    private System.Windows.Forms.GroupBox globalGroupBox;
    private System.Windows.Forms.TextBox xOffsetTextBox;
    private System.Windows.Forms.Label xOffsetLabel;
    private System.Windows.Forms.TextBox yOffsetTextBox;
    private System.Windows.Forms.Label yOffsetLabel;
    private System.Windows.Forms.GroupBox groupBoxLogging;
    private System.Windows.Forms.CheckBox checkBoxLogPhaseAndState;
    private System.Windows.Forms.CheckBox checkBoxLightMode;
    private System.Windows.Forms.CheckBox checkBoxLogDamage;
    private System.Windows.Forms.CheckBox checkBoxLogTiming;
    private System.Windows.Forms.CheckBox checkBoxLogRules;
  }
}

