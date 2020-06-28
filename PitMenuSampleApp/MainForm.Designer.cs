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
      this.groupBox3 = new System.Windows.Forms.GroupBox();
      this.label4 = new System.Windows.Forms.Label();
      this.cbAllControls = new System.Windows.Forms.ComboBox();
      this.groupBox2 = new System.Windows.Forms.GroupBox();
      this.cbChoices = new System.Windows.Forms.ComboBox();
      this.Box_PitMenu = new System.Windows.Forms.GroupBox();
      this.groupBox4 = new System.Windows.Forms.GroupBox();
      this.tbSetFuel = new System.Windows.Forms.TextBox();
      this.tbCurrentFuelLevel = new System.Windows.Forms.TextBox();
      this.label1 = new System.Windows.Forms.Label();
      this.textBox1 = new System.Windows.Forms.TextBox();
      this.groupBox1 = new System.Windows.Forms.GroupBox();
      this.cbCategory = new System.Windows.Forms.ComboBox();
      this.checkBox1 = new System.Windows.Forms.CheckBox();
      this.timer1 = new System.Windows.Forms.Timer(this.components);
      this.groupBox5 = new System.Windows.Forms.GroupBox();
      this.cbTyreChoice = new System.Windows.Forms.ComboBox();
      this.label5 = new System.Windows.Forms.Label();
      this.flowLayoutPanel1.SuspendLayout();
      this.tableLayoutPanel1.SuspendLayout();
      this.groupBox3.SuspendLayout();
      this.groupBox2.SuspendLayout();
      this.Box_PitMenu.SuspendLayout();
      this.groupBox4.SuspendLayout();
      this.groupBox1.SuspendLayout();
      this.groupBox5.SuspendLayout();
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
      // groupBox3
      // 
      this.groupBox3.Controls.Add(this.label4);
      this.groupBox3.Controls.Add(this.cbAllControls);
      this.groupBox3.Location = new System.Drawing.Point(529, 283);
      this.groupBox3.Name = "groupBox3";
      this.groupBox3.Size = new System.Drawing.Size(467, 134);
      this.groupBox3.TabIndex = 7;
      this.groupBox3.TabStop = false;
      this.groupBox3.Text = "All the control strings";
      // 
      // label4
      // 
      this.label4.AutoSize = true;
      this.label4.Location = new System.Drawing.Point(9, 40);
      this.label4.Name = "label4";
      this.label4.Size = new System.Drawing.Size(385, 25);
      this.label4.TabIndex = 4;
      this.label4.Text = "A lot don\'t do anything - USE WITH CARE!";
      // 
      // cbAllControls
      // 
      this.cbAllControls.FormattingEnabled = true;
      this.cbAllControls.Items.AddRange(new object[] {
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
      this.cbAllControls.Location = new System.Drawing.Point(6, 68);
      this.cbAllControls.Name = "cbAllControls";
      this.cbAllControls.Size = new System.Drawing.Size(388, 32);
      this.cbAllControls.TabIndex = 2;
      this.cbAllControls.SelectionChangeCommitted += new System.EventHandler(this.cbAllControls_SelectionChangeCommitted);
      // 
      // groupBox2
      // 
      this.groupBox2.Controls.Add(this.cbChoices);
      this.groupBox2.Location = new System.Drawing.Point(529, 145);
      this.groupBox2.Name = "groupBox2";
      this.groupBox2.Size = new System.Drawing.Size(467, 120);
      this.groupBox2.TabIndex = 6;
      this.groupBox2.TabStop = false;
      this.groupBox2.Text = "Pit Menu choices";
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
            "3",
            "4",
            "5",
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
            "No Change",
            "P2M - Wet",
            "S7M - Soft",
            "S8M - Medium",
            "S9M - Hard",
            "P6",
            "P7",
            "P8"});
      this.cbChoices.Location = new System.Drawing.Point(6, 68);
      this.cbChoices.Name = "cbChoices";
      this.cbChoices.Size = new System.Drawing.Size(388, 32);
      this.cbChoices.TabIndex = 2;
      this.cbChoices.SelectionChangeCommitted += new System.EventHandler(this.cbChoices_SelectionChangeCommitted);
      // 
      // Box_PitMenu
      // 
      this.Box_PitMenu.Controls.Add(this.groupBox5);
      this.Box_PitMenu.Controls.Add(this.groupBox4);
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
      // groupBox4
      // 
      this.groupBox4.Controls.Add(this.label5);
      this.groupBox4.Controls.Add(this.tbSetFuel);
      this.groupBox4.Controls.Add(this.tbCurrentFuelLevel);
      this.groupBox4.Location = new System.Drawing.Point(7, 118);
      this.groupBox4.Name = "groupBox4";
      this.groupBox4.Size = new System.Drawing.Size(440, 94);
      this.groupBox4.TabIndex = 1;
      this.groupBox4.TabStop = false;
      this.groupBox4.Text = "Fuel level";
      // 
      // tbSetFuel
      // 
      this.tbSetFuel.Font = new System.Drawing.Font("LCDMono", 8.142858F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.tbSetFuel.Location = new System.Drawing.Point(306, 45);
      this.tbSetFuel.Name = "tbSetFuel";
      this.tbSetFuel.Size = new System.Drawing.Size(100, 27);
      this.tbSetFuel.TabIndex = 1;
      this.tbSetFuel.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.tbSetFuel_TextChanged);
      // 
      // tbCurrentFuelLevel
      // 
      this.tbCurrentFuelLevel.Font = new System.Drawing.Font("LCDMono", 8.142858F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.tbCurrentFuelLevel.Location = new System.Drawing.Point(22, 45);
      this.tbCurrentFuelLevel.Name = "tbCurrentFuelLevel";
      this.tbCurrentFuelLevel.ReadOnly = true;
      this.tbCurrentFuelLevel.Size = new System.Drawing.Size(100, 27);
      this.tbCurrentFuelLevel.TabIndex = 0;
      this.tbCurrentFuelLevel.Text = "---";
      // 
      // label1
      // 
      this.label1.AutoSize = true;
      this.label1.Location = new System.Drawing.Point(105, 363);
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
      this.textBox1.Size = new System.Drawing.Size(440, 69);
      this.textBox1.TabIndex = 0;
      this.textBox1.Text = "rFactor not connected";
      this.textBox1.KeyDown += new System.Windows.Forms.KeyEventHandler(this.MainForm_KeyDown);
      // 
      // groupBox1
      // 
      this.groupBox1.Controls.Add(this.cbCategory);
      this.groupBox1.Location = new System.Drawing.Point(529, 3);
      this.groupBox1.Name = "groupBox1";
      this.groupBox1.Size = new System.Drawing.Size(467, 124);
      this.groupBox1.TabIndex = 5;
      this.groupBox1.TabStop = false;
      this.groupBox1.Text = "Pit Menu categories";
      // 
      // cbCategory
      // 
      this.cbCategory.FormattingEnabled = true;
      this.cbCategory.Items.AddRange(new object[] {
            "FL PRESS:",
            "FR PRESS:",
            "RL PRESS:",
            "RR PRESS:",
            "FL TIRE:",
            "FR TIRE:",
            "RL TIRE:",
            "RR TIRE:",
            "FUEL:",
            "GRILLE:",
            "R WING:"});
      this.cbCategory.Location = new System.Drawing.Point(6, 68);
      this.cbCategory.Name = "cbCategory";
      this.cbCategory.Size = new System.Drawing.Size(388, 32);
      this.cbCategory.TabIndex = 2;
      this.cbCategory.SelectionChangeCommitted += new System.EventHandler(this.cbCategory_ChangeCommitted);
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
      // timer1
      // 
      this.timer1.Tick += new System.EventHandler(this.timer1_Tick);
      // 
      // groupBox5
      // 
      this.groupBox5.Controls.Add(this.cbTyreChoice);
      this.groupBox5.Location = new System.Drawing.Point(5, 228);
      this.groupBox5.Name = "groupBox5";
      this.groupBox5.Size = new System.Drawing.Size(447, 81);
      this.groupBox5.TabIndex = 2;
      this.groupBox5.TabStop = false;
      this.groupBox5.Text = "Tyre choice";
      // 
      // cbTyreChoice
      // 
      this.cbTyreChoice.FormattingEnabled = true;
      this.cbTyreChoice.Items.AddRange(new object[] {
            "Soft",
            "Medium",
            "Hard",
            "Wet",
            "No Change"});
      this.cbTyreChoice.Location = new System.Drawing.Point(46, 29);
      this.cbTyreChoice.Name = "cbTyreChoice";
      this.cbTyreChoice.Size = new System.Drawing.Size(165, 32);
      this.cbTyreChoice.TabIndex = 0;
      this.cbTyreChoice.SelectionChangeCommitted += new System.EventHandler(this.cbTyreChoice_SelectionChangeCommitted);
      // 
      // label5
      // 
      this.label5.AutoSize = true;
      this.label5.Location = new System.Drawing.Point(201, 45);
      this.label5.Name = "label5";
      this.label5.Size = new System.Drawing.Size(87, 25);
      this.label5.TabIndex = 2;
      this.label5.Text = "Set level";
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
      this.groupBox3.ResumeLayout(false);
      this.groupBox3.PerformLayout();
      this.groupBox2.ResumeLayout(false);
      this.Box_PitMenu.ResumeLayout(false);
      this.Box_PitMenu.PerformLayout();
      this.groupBox4.ResumeLayout(false);
      this.groupBox4.PerformLayout();
      this.groupBox1.ResumeLayout(false);
      this.groupBox5.ResumeLayout(false);
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
    private System.Windows.Forms.TableLayoutPanel tableLayoutPanel1;
    private System.Windows.Forms.GroupBox groupBox3;
    private System.Windows.Forms.Label label4;
    private System.Windows.Forms.ComboBox cbAllControls;
    private System.Windows.Forms.GroupBox groupBox2;
    private System.Windows.Forms.ComboBox cbChoices;
    private System.Windows.Forms.GroupBox groupBox4;
    private System.Windows.Forms.TextBox tbSetFuel;
    private System.Windows.Forms.TextBox tbCurrentFuelLevel;
    private System.Windows.Forms.GroupBox groupBox5;
    private System.Windows.Forms.ComboBox cbTyreChoice;
    private System.Windows.Forms.Label label5;
  }
}

