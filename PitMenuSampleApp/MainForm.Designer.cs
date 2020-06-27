namespace PitMenuSampleApp
{
    partial class MainForm
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
            this.components = new System.ComponentModel.Container();
            this.flowLayoutPanel1 = new System.Windows.Forms.FlowLayoutPanel();
            this.tableLayoutPanel1 = new System.Windows.Forms.TableLayoutPanel();
            this.Box_PitMenu = new System.Windows.Forms.GroupBox();
            this.label1 = new System.Windows.Forms.Label();
            this.textBox1 = new System.Windows.Forms.TextBox();
            this.checkBox1 = new System.Windows.Forms.CheckBox();
            this.groupBox3 = new System.Windows.Forms.GroupBox();
            this.label4 = new System.Windows.Forms.Label();
            this.comboBox3 = new System.Windows.Forms.ComboBox();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.label3 = new System.Windows.Forms.Label();
            this.cbChoices = new System.Windows.Forms.ComboBox();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.label2 = new System.Windows.Forms.Label();
            this.cbCategory = new System.Windows.Forms.ComboBox();
            this.timer1 = new System.Windows.Forms.Timer(this.components);
            this.flowLayoutPanel1.SuspendLayout();
            this.tableLayoutPanel1.SuspendLayout();
            this.Box_PitMenu.SuspendLayout();
            this.groupBox3.SuspendLayout();
            this.groupBox2.SuspendLayout();
            this.groupBox1.SuspendLayout();
            this.SuspendLayout();
            // 
            // flowLayoutPanel1
            // 
            this.flowLayoutPanel1.Controls.Add(this.tableLayoutPanel1);
            this.flowLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.flowLayoutPanel1.Location = new System.Drawing.Point(0, 0);
            this.flowLayoutPanel1.Margin = new System.Windows.Forms.Padding(4);
            this.flowLayoutPanel1.Name = "flowLayoutPanel1";
            this.flowLayoutPanel1.Size = new System.Drawing.Size(1051, 493);
            this.flowLayoutPanel1.TabIndex = 0;
            // 
            // tableLayoutPanel1
            // 
            this.tableLayoutPanel1.ColumnCount = 2;
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 51.20428F));
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 48.79572F));
            this.tableLayoutPanel1.Controls.Add(this.groupBox3, 1, 2);
            this.tableLayoutPanel1.Controls.Add(this.groupBox2, 1, 1);
            this.tableLayoutPanel1.Controls.Add(this.Box_PitMenu, 0, 0);
            this.tableLayoutPanel1.Controls.Add(this.groupBox1, 1, 0);
            this.tableLayoutPanel1.Controls.Add(this.checkBox1, 1, 3);
            this.tableLayoutPanel1.Location = new System.Drawing.Point(3, 3);
            this.tableLayoutPanel1.Name = "tableLayoutPanel1";
            this.tableLayoutPanel1.RowCount = 4;
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 50.72464F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 49.27536F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 140F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 42F));
            this.tableLayoutPanel1.Size = new System.Drawing.Size(1028, 463);
            this.tableLayoutPanel1.TabIndex = 6;
            // 
            // Box_PitMenu
            // 
            this.Box_PitMenu.Controls.Add(this.label1);
            this.Box_PitMenu.Controls.Add(this.textBox1);
            this.Box_PitMenu.Location = new System.Drawing.Point(4, 4);
            this.Box_PitMenu.Margin = new System.Windows.Forms.Padding(4);
            this.Box_PitMenu.Name = "Box_PitMenu";
            this.Box_PitMenu.Padding = new System.Windows.Forms.Padding(4);
            this.tableLayoutPanel1.SetRowSpan(this.Box_PitMenu, 4);
            this.Box_PitMenu.Size = new System.Drawing.Size(511, 445);
            this.Box_PitMenu.TabIndex = 0;
            this.Box_PitMenu.TabStop = false;
            this.Box_PitMenu.Text = "Pit Menu";
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(102, 325);
            this.label1.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(245, 50);
            this.label1.TabIndex = 0;
            this.label1.Text = "Use ASWD as cursor keys\r\nto control the Pit Menu\r\n";
            // 
            // textBox1
            // 
            this.textBox1.Font = new System.Drawing.Font("LCDMono", 14F, System.Drawing.FontStyle.Bold);
            this.textBox1.Location = new System.Drawing.Point(7, 30);
            this.textBox1.Margin = new System.Windows.Forms.Padding(4);
            this.textBox1.Multiline = true;
            this.textBox1.Name = "textBox1";
            this.textBox1.ReadOnly = true;
            this.textBox1.Size = new System.Drawing.Size(473, 244);
            this.textBox1.TabIndex = 0;
            this.textBox1.Text = "rFactor not connected";
            this.textBox1.KeyDown += new System.Windows.Forms.KeyEventHandler(this.MainForm_KeyDown);
            // 
            // checkBox1
            // 
            this.checkBox1.AutoSize = true;
            this.checkBox1.Location = new System.Drawing.Point(529, 423);
            this.checkBox1.Name = "checkBox1";
            this.checkBox1.Size = new System.Drawing.Size(195, 29);
            this.checkBox1.TabIndex = 3;
            this.checkBox1.Text = "rFactor connected";
            this.checkBox1.UseVisualStyleBackColor = true;
            // 
            // groupBox3
            // 
            this.groupBox3.Controls.Add(this.label4);
            this.groupBox3.Controls.Add(this.comboBox3);
            this.groupBox3.Location = new System.Drawing.Point(529, 283);
            this.groupBox3.Name = "groupBox3";
            this.groupBox3.Size = new System.Drawing.Size(467, 134);
            this.groupBox3.TabIndex = 7;
            this.groupBox3.TabStop = false;
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(6, 15);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(393, 50);
            this.label4.TabIndex = 4;
            this.label4.Text = "All the control strings. A lot don\'t do anything\r\n              USE WITH CARE!";
            // 
            // comboBox3
            // 
            this.comboBox3.FormattingEnabled = true;
            this.comboBox3.Items.AddRange(new object[] {
            "AdjustSeatAft",
            "AdjustSeatDown",
            "AdjustSeatFore",
            "AdjustSeatUp",
            "AltBrake",
            "AlternateEsc",
            "AltShiftDown",
            "AltShiftUp",
            "AltSteerLeft",
            "AltSteerRight",
            "AltThrottle",
            "AntiLockBrakes",
            "AntilockBrakeSystemDown",
            "AntilockBrakeSystemUp",
            "AutoClutch",
            "AutoPit",
            "AutoShifting",
            "BiasForward",
            "BiasRearward",
            "Brake",
            "BrakingHelp",
            "Camera_CamEd_Slow",
            "Camera_MoveBackward",
            "Camera_MoveDown",
            "Camera_MoveForward",
            "Camera_MoveLeft",
            "Camera_MoveRight",
            "Camera_MoveUp",
            "Camera_PitchDown",
            "Camera_PitchUp",
            "Camera_RollLeft",
            "Camera_RollRight",
            "Camera_YawLeft",
            "Camera_YawRight",
            "Camera_ZoomIn",
            "Camera_ZoomOut",
            "CameraChange",
            "ClutchIn",
            "CPUTime",
            "Custom Plugin #10",
            "Custom Plugin #11",
            "Custom Plugin #12",
            "Custom Plugin #3",
            "Custom Plugin #4",
            "Custom Plugin #5",
            "Custom Plugin #6",
            "Custom Plugin #7",
            "Custom Plugin #8",
            "Custom Plugin #9",
            "CustomPlugin01",
            "CustomPlugin02",
            "CustomPlugin03",
            "CustomPlugin04",
            "CustomPlugin05",
            "CustomPlugin06",
            "CustomPlugin07",
            "CustomPlugin08",
            "CustomPlugin09",
            "CustomPlugin10",
            "CustomPlugin11",
            "CustomPlugin12",
            "CycleDriving",
            "CycleOnboard",
            "CycleSpectator",
            "CycleSwingman",
            "CycleTracking",
            "DecreaseVerticalFOV",
            "DecrementBoost",
            "DecrementFrontAntiSway",
            "DecrementMixture",
            "DecrementRearAntiSway",
            "DisplayMode",
            "DisplayVehicleLabels",
            "DriverHotSwap",
            "EngineBrakingMapDown",
            "EngineBrakingMapUp",
            "Framerate",
            "FrontFlap",
            "Gear_1",
            "Gear_2",
            "Gear_3",
            "Gear_4",
            "Gear_5",
            "Gear_6",
            "Gear_7",
            "Gear_8",
            "Gear_9",
            "Gear_R",
            "Handbrake",
            "Handfrontbrake",
            "Headlights",
            "Horn",
            "Ignition",
            "IncreaseVerticalFOV",
            "IncrementBoost",
            "IncrementFrontAntiSway",
            "IncrementMixture",
            "IncrementRearAntiSway",
            "InstantReplay",
            "Invulnerability",
            "LaunchControl",
            "LoadVehicles",
            "LookDown",
            "LookLeft",
            "LookRight",
            "LookRollLeft",
            "LookRollRight",
            "LookUp",
            "LowerTrackBar",
            "MessageHistory",
            "MusicPause",
            "MusicPlay",
            "MusicPlayNext",
            "MusicPlayPrevious",
            "MusicVolumeDown",
            "MusicVolumeUp",
            "Neutral",
            "OppositeLock",
            "PassengerSelect",
            "Pause",
            "PitMenuDecrementValue",
            "PitMenuDown",
            "PitMenuIncrementValue",
            "PitMenuUp",
            "PitRequest",
            "PowerDemand",
            "QuickChat01",
            "QuickChat02",
            "QuickChat03",
            "QuickChat04",
            "QuickChat05",
            "QuickChat06",
            "QuickChat07",
            "QuickChat08",
            "QuickChat09",
            "QuickChat10",
            "QuickChat11",
            "QuickChat12",
            "RaiseTrackBar",
            "RealtimeChat",
            "RearFlap",
            "RearLook",
            "ResetFFB",
            "RestartRace",
            "Screenshot",
            "SharedMemCancel",
            "SharedMemDown",
            "SharedMemLeft",
            "SharedMemRight",
            "SharedMemSelect",
            "SharedMemUp",
            "ShiftDown",
            "ShiftUp",
            "SkipFormation",
            "SpeedLimiter",
            "SpinRecovery",
            "StabilityControl",
            "Starter",
            "SteeringHelp",
            "SteerLeft",
            "SteerRight",
            "SwingmanDecRadius",
            "SwingmanIncRadius",
            "SwingmanPitchDown",
            "SwingmanPitchUp",
            "SwingmanReset",
            "SwingmanYawLeft",
            "SwingmanYawRight",
            "TCOverride",
            "TemporaryBoost",
            "Throttle",
            "TimeAcceleration",
            "Toggle_HWPlugin",
            "ToggleAIControl",
            "ToggleFreeLook",
            "ToggleHUDMFD",
            "ToggleHUDStats",
            "ToggleHUDTach",
            "ToggleMFDA",
            "ToggleMFDB",
            "ToggleMFDC",
            "ToggleMFDD",
            "ToggleMFDE",
            "ToggleMFDF",
            "ToggleMFDG",
            "ToggleMFDH",
            "ToggleMirror",
            "ToggleOverlays",
            "TractionControl",
            "TractionControlDown",
            "TractionControlUp",
            "TriplesAndTires",
            "UIMouseClick",
            "UIMouseDown",
            "UIMouseLeft",
            "UIMouseRight",
            "UIMouseUp",
            "ViewMyVehicle",
            "ViewNextVehicle",
            "ViewPrevVehicle",
            "VoiceChat_PTT",
            "VRCenter",
            "VRIPDScaleDecrease",
            "VRIPDScaleIncrease",
            "VRIPDScaleReset",
            "Wipers",
            "ZeroFreeLook",
            "_DAMP Logging",
            "_Telemetry Marker"});
            this.comboBox3.Location = new System.Drawing.Point(6, 68);
            this.comboBox3.Name = "comboBox3";
            this.comboBox3.Size = new System.Drawing.Size(388, 32);
            this.comboBox3.TabIndex = 2;
            // 
            // groupBox2
            // 
            this.groupBox2.Controls.Add(this.label3);
            this.groupBox2.Controls.Add(this.cbChoices);
            this.groupBox2.Location = new System.Drawing.Point(529, 145);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(467, 120);
            this.groupBox2.TabIndex = 6;
            this.groupBox2.TabStop = false;
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(62, 25);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(161, 25);
            this.label3.TabIndex = 4;
            this.label3.Text = "Pit Menu choices";
            // 
            // cbChoices
            // 
            this.cbChoices.FormattingEnabled = true;
            this.cbChoices.Items.AddRange(new object[] {
            "+ 0.0",
            "+ 0.3",
            "+ 0.5",
            "+ 0.8",
            "+ 1.1",
            "+ 1.3",
            "+ 1.6",
            "1",
            "2",
            "20.3",
            "20.5",
            "20.6",
            "20.7",
            "20.9",
            "21.0",
            "21.2",
            "21.3",
            "21.5",
            "21.6",
            "21.8",
            "21.9",
            "22.0",
            "22.2",
            "22.3",
            "22.5",
            "22.6",
            "22.8",
            "3",
            "4",
            "5",
            "No Change",
            "P2M - Wet",
            "P6",
            "P7",
            "P8",
            "S7M - Soft",
            "S8M - Medium",
            "S9M - Hard"});
            this.cbChoices.Location = new System.Drawing.Point(6, 68);
            this.cbChoices.Name = "cbChoices";
            this.cbChoices.Size = new System.Drawing.Size(388, 32);
            this.cbChoices.TabIndex = 2;
            this.cbChoices.SelectionChangeCommitted += new System.EventHandler(this.cbChoices_SelectionChangeCommitted);
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.label2);
            this.groupBox1.Controls.Add(this.cbCategory);
            this.groupBox1.Location = new System.Drawing.Point(529, 3);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(467, 124);
            this.groupBox1.TabIndex = 5;
            this.groupBox1.TabStop = false;
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(62, 25);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(184, 25);
            this.label2.TabIndex = 4;
            this.label2.Text = "Pit Menu categories";
            // 
            // cbCategory
            // 
            this.cbCategory.FormattingEnabled = true;
            this.cbCategory.Items.AddRange(new object[] {
            "FL PRESS:",
            "FL TIRE:",
            "FR PRESS:",
            "FR TIRE:",
            "FUEL:",
            "GRILLE:",
            "R WING:",
            "RL PRESS:",
            "RL TIRE:",
            "RR PRESS:",
            "RR TIRE:"});
            this.cbCategory.Location = new System.Drawing.Point(6, 68);
            this.cbCategory.Name = "cbCategory";
            this.cbCategory.Size = new System.Drawing.Size(388, 32);
            this.cbCategory.TabIndex = 2;
            this.cbCategory.SelectionChangeCommitted += new System.EventHandler(this.cbCategory_ChangeCommitted);
            // 
            // timer1
            // 
            this.timer1.Tick += new System.EventHandler(this.timer1_Tick);
            // 
            // MainForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(11F, 24F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(1051, 493);
            this.Controls.Add(this.flowLayoutPanel1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.Margin = new System.Windows.Forms.Padding(4);
            this.MaximizeBox = false;
            this.Name = "MainForm";
            this.Text = "Pit Menu Control Demo";
            this.KeyDown += new System.Windows.Forms.KeyEventHandler(this.MainForm_KeyDown);
            this.KeyUp += new System.Windows.Forms.KeyEventHandler(this.MainForm_KeyUp);
            this.flowLayoutPanel1.ResumeLayout(false);
            this.tableLayoutPanel1.ResumeLayout(false);
            this.tableLayoutPanel1.PerformLayout();
            this.Box_PitMenu.ResumeLayout(false);
            this.Box_PitMenu.PerformLayout();
            this.groupBox3.ResumeLayout(false);
            this.groupBox3.PerformLayout();
            this.groupBox2.ResumeLayout(false);
            this.groupBox2.PerformLayout();
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.ResumeLayout(false);

        }

    #endregion

    private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel1;
    private System.Windows.Forms.GroupBox Box_PitMenu;
    private System.Windows.Forms.TextBox textBox1;
    private System.Windows.Forms.Label label1;
    private System.Windows.Forms.Timer timer1;
    private System.Windows.Forms.ComboBox cbCategory;
    private System.Windows.Forms.CheckBox checkBox1;
    private System.Windows.Forms.GroupBox groupBox1;
    private System.Windows.Forms.Label label2;
    private System.Windows.Forms.TableLayoutPanel tableLayoutPanel1;
    private System.Windows.Forms.GroupBox groupBox3;
    private System.Windows.Forms.Label label4;
    private System.Windows.Forms.ComboBox comboBox3;
    private System.Windows.Forms.GroupBox groupBox2;
    private System.Windows.Forms.Label label3;
    private System.Windows.Forms.ComboBox cbChoices;
  }
}

