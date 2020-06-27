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
            this.Box_PitMenu = new System.Windows.Forms.GroupBox();
            this.textBox1 = new System.Windows.Forms.TextBox();
            this.Box_Controls = new System.Windows.Forms.GroupBox();
            this.label1 = new System.Windows.Forms.Label();
            this.timer1 = new System.Windows.Forms.Timer(this.components);
            this.comboBox1 = new System.Windows.Forms.ComboBox();
            this.checkBox1 = new System.Windows.Forms.CheckBox();
            this.label2 = new System.Windows.Forms.Label();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.flowLayoutPanel1.SuspendLayout();
            this.Box_PitMenu.SuspendLayout();
            this.Box_Controls.SuspendLayout();
            this.groupBox1.SuspendLayout();
            this.SuspendLayout();
            // 
            // flowLayoutPanel1
            // 
            this.flowLayoutPanel1.Controls.Add(this.Box_PitMenu);
            this.flowLayoutPanel1.Controls.Add(this.Box_Controls);
            this.flowLayoutPanel1.Controls.Add(this.checkBox1);
            this.flowLayoutPanel1.Controls.Add(this.groupBox1);
            this.flowLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.flowLayoutPanel1.Location = new System.Drawing.Point(0, 0);
            this.flowLayoutPanel1.Margin = new System.Windows.Forms.Padding(4);
            this.flowLayoutPanel1.Name = "flowLayoutPanel1";
            this.flowLayoutPanel1.Size = new System.Drawing.Size(978, 540);
            this.flowLayoutPanel1.TabIndex = 0;
            // 
            // Box_PitMenu
            // 
            this.Box_PitMenu.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.Box_PitMenu.Controls.Add(this.textBox1);
            this.Box_PitMenu.Location = new System.Drawing.Point(4, 4);
            this.Box_PitMenu.Margin = new System.Windows.Forms.Padding(4);
            this.Box_PitMenu.Name = "Box_PitMenu";
            this.Box_PitMenu.Padding = new System.Windows.Forms.Padding(4);
            this.Box_PitMenu.Size = new System.Drawing.Size(507, 304);
            this.Box_PitMenu.TabIndex = 0;
            this.Box_PitMenu.TabStop = false;
            this.Box_PitMenu.Text = "Pit Menu";
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
            this.textBox1.Text = "FUEL: +10.0/4\r\nFL PRESS: 20.0\r\nFR PRESS: 20.0\r\nRL PRESS: 18.5\r\nRR PRESS: 18.5\r\n\r\n" +
    "";
            this.textBox1.KeyDown += new System.Windows.Forms.KeyEventHandler(this.MainForm_KeyDown);
            // 
            // Box_Controls
            // 
            this.Box_Controls.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.Box_Controls.Controls.Add(this.label1);
            this.Box_Controls.Location = new System.Drawing.Point(519, 4);
            this.Box_Controls.Margin = new System.Windows.Forms.Padding(4);
            this.Box_Controls.Name = "Box_Controls";
            this.Box_Controls.Padding = new System.Windows.Forms.Padding(4);
            this.Box_Controls.Size = new System.Drawing.Size(325, 170);
            this.Box_Controls.TabIndex = 1;
            this.Box_Controls.TabStop = false;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(17, 42);
            this.label1.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(245, 50);
            this.label1.TabIndex = 0;
            this.label1.Text = "Use ASWD as cursor keys\r\nto control the Pit Menu\r\n";
            // 
            // timer1
            // 
            this.timer1.Tick += new System.EventHandler(this.timer1_Tick);
            // 
            // comboBox1
            // 
            this.comboBox1.FormattingEnabled = true;
            this.comboBox1.Items.AddRange(new object[] {
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
            this.comboBox1.Location = new System.Drawing.Point(6, 68);
            this.comboBox1.Name = "comboBox1";
            this.comboBox1.Size = new System.Drawing.Size(388, 32);
            this.comboBox1.TabIndex = 2;
            this.comboBox1.SelectionChangeCommitted += new System.EventHandler(this.comboBox1_ChangeCommitted);
            // 
            // checkBox1
            // 
            this.checkBox1.AutoSize = true;
            this.checkBox1.Location = new System.Drawing.Point(3, 315);
            this.checkBox1.Name = "checkBox1";
            this.checkBox1.Size = new System.Drawing.Size(195, 29);
            this.checkBox1.TabIndex = 3;
            this.checkBox1.Text = "rFactor connected";
            this.checkBox1.UseVisualStyleBackColor = true;
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(62, 25);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(191, 25);
            this.label2.TabIndex = 4;
            this.label2.Text = "All the control strings";
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.label2);
            this.groupBox1.Controls.Add(this.comboBox1);
            this.groupBox1.Location = new System.Drawing.Point(204, 315);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(467, 151);
            this.groupBox1.TabIndex = 5;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "groupBox1";
            // 
            // MainForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(11F, 24F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(978, 540);
            this.Controls.Add(this.flowLayoutPanel1);
            this.Margin = new System.Windows.Forms.Padding(4);
            this.Name = "MainForm";
            this.Text = "Form1";
            this.KeyDown += new System.Windows.Forms.KeyEventHandler(this.MainForm_KeyDown);
            this.KeyUp += new System.Windows.Forms.KeyEventHandler(this.MainForm_KeyUp);
            this.flowLayoutPanel1.ResumeLayout(false);
            this.flowLayoutPanel1.PerformLayout();
            this.Box_PitMenu.ResumeLayout(false);
            this.Box_PitMenu.PerformLayout();
            this.Box_Controls.ResumeLayout(false);
            this.Box_Controls.PerformLayout();
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.ResumeLayout(false);

        }

    #endregion

    private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel1;
    private System.Windows.Forms.GroupBox Box_PitMenu;
    private System.Windows.Forms.TextBox textBox1;
    private System.Windows.Forms.GroupBox Box_Controls;
    private System.Windows.Forms.Label label1;
    private System.Windows.Forms.Timer timer1;
    private System.Windows.Forms.ComboBox comboBox1;
    private System.Windows.Forms.CheckBox checkBox1;
    private System.Windows.Forms.GroupBox groupBox1;
    private System.Windows.Forms.Label label2;
  }
}

