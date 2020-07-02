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
      this.groupBox5 = new System.Windows.Forms.GroupBox();
      this.cbTyreChoice = new System.Windows.Forms.ComboBox();
      this.groupBox4 = new System.Windows.Forms.GroupBox();
      this.label5 = new System.Windows.Forms.Label();
      this.tbSetFuel = new System.Windows.Forms.TextBox();
      this.tbCurrentFuelLevel = new System.Windows.Forms.TextBox();
      this.label1 = new System.Windows.Forms.Label();
      this.textBox1 = new System.Windows.Forms.TextBox();
      this.groupBox1 = new System.Windows.Forms.GroupBox();
      this.cbCategory = new System.Windows.Forms.ComboBox();
      this.checkBox1 = new System.Windows.Forms.CheckBox();
      this.timer1 = new System.Windows.Forms.Timer(this.components);
      this.trackBarDelay = new System.Windows.Forms.TrackBar();
      this.comboBoxAllTyres = new System.Windows.Forms.ComboBox();
      this.groupBox6 = new System.Windows.Forms.GroupBox();
      this.labelDelay = new System.Windows.Forms.Label();
      this.label2 = new System.Windows.Forms.Label();
      this.label3 = new System.Windows.Forms.Label();
      this.buttonToggleMenu = new System.Windows.Forms.Button();
      this.toolTip1 = new System.Windows.Forms.ToolTip(this.components);
      this.checkBox2 = new System.Windows.Forms.CheckBox();
      this.numericUpDownErrors = new System.Windows.Forms.NumericUpDown();
      this.groupBox7 = new System.Windows.Forms.GroupBox();
      this.label6 = new System.Windows.Forms.Label();
      this.label7 = new System.Windows.Forms.Label();
      this.numericUpDownTests = new System.Windows.Forms.NumericUpDown();
      this.flowLayoutPanel1.SuspendLayout();
      this.tableLayoutPanel1.SuspendLayout();
      this.groupBox3.SuspendLayout();
      this.groupBox2.SuspendLayout();
      this.Box_PitMenu.SuspendLayout();
      this.groupBox5.SuspendLayout();
      this.groupBox4.SuspendLayout();
      this.groupBox1.SuspendLayout();
      ((System.ComponentModel.ISupportInitialize)(this.trackBarDelay)).BeginInit();
      this.groupBox6.SuspendLayout();
      ((System.ComponentModel.ISupportInitialize)(this.numericUpDownErrors)).BeginInit();
      this.groupBox7.SuspendLayout();
      ((System.ComponentModel.ISupportInitialize)(this.numericUpDownTests)).BeginInit();
      this.SuspendLayout();
      // 
      // flowLayoutPanel1
      // 
      this.flowLayoutPanel1.Controls.Add(this.tableLayoutPanel1);
      this.flowLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
      this.flowLayoutPanel1.Location = new System.Drawing.Point(0, 0);
      this.flowLayoutPanel1.Name = "flowLayoutPanel1";
      this.flowLayoutPanel1.Size = new System.Drawing.Size(951, 520);
      this.flowLayoutPanel1.TabIndex = 0;
      // 
      // tableLayoutPanel1
      // 
      this.tableLayoutPanel1.ColumnCount = 2;
      this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 57.35608F));
      this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 42.64392F));
      this.tableLayoutPanel1.Controls.Add(this.groupBox3, 1, 2);
      this.tableLayoutPanel1.Controls.Add(this.groupBox2, 1, 1);
      this.tableLayoutPanel1.Controls.Add(this.Box_PitMenu, 0, 0);
      this.tableLayoutPanel1.Controls.Add(this.groupBox1, 1, 0);
      this.tableLayoutPanel1.Controls.Add(this.checkBox1, 1, 3);
      this.tableLayoutPanel1.Controls.Add(this.groupBox6, 0, 2);
      this.tableLayoutPanel1.Controls.Add(this.groupBox7, 0, 3);
      this.tableLayoutPanel1.Location = new System.Drawing.Point(2, 2);
      this.tableLayoutPanel1.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
      this.tableLayoutPanel1.Name = "tableLayoutPanel1";
      this.tableLayoutPanel1.RowCount = 4;
      this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 50.72464F));
      this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 49.27536F));
      this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 119F));
      this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 57F));
      this.tableLayoutPanel1.Size = new System.Drawing.Size(938, 507);
      this.tableLayoutPanel1.TabIndex = 6;
      // 
      // groupBox3
      // 
      this.groupBox3.Controls.Add(this.label4);
      this.groupBox3.Controls.Add(this.cbAllControls);
      this.groupBox3.Location = new System.Drawing.Point(539, 332);
      this.groupBox3.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
      this.groupBox3.Name = "groupBox3";
      this.groupBox3.Padding = new System.Windows.Forms.Padding(2, 2, 2, 2);
      this.groupBox3.Size = new System.Drawing.Size(382, 112);
      this.groupBox3.TabIndex = 7;
      this.groupBox3.TabStop = false;
      this.groupBox3.Text = "All the control strings";
      // 
      // label4
      // 
      this.label4.AutoSize = true;
      this.label4.Location = new System.Drawing.Point(7, 33);
      this.label4.Margin = new System.Windows.Forms.Padding(2, 0, 2, 0);
      this.label4.Name = "label4";
      this.label4.Size = new System.Drawing.Size(328, 20);
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
      this.cbAllControls.Location = new System.Drawing.Point(5, 57);
      this.cbAllControls.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
      this.cbAllControls.Name = "cbAllControls";
      this.cbAllControls.Size = new System.Drawing.Size(318, 28);
      this.cbAllControls.TabIndex = 2;
      this.cbAllControls.SelectionChangeCommitted += new System.EventHandler(this.cbAllControls_SelectionChangeCommitted);
      // 
      // groupBox2
      // 
      this.groupBox2.Controls.Add(this.cbChoices);
      this.groupBox2.Location = new System.Drawing.Point(539, 169);
      this.groupBox2.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
      this.groupBox2.Name = "groupBox2";
      this.groupBox2.Padding = new System.Windows.Forms.Padding(2, 2, 2, 2);
      this.groupBox2.Size = new System.Drawing.Size(382, 100);
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
      this.cbChoices.Location = new System.Drawing.Point(5, 57);
      this.cbChoices.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
      this.cbChoices.Name = "cbChoices";
      this.cbChoices.Size = new System.Drawing.Size(318, 28);
      this.cbChoices.TabIndex = 2;
      this.cbChoices.SelectionChangeCommitted += new System.EventHandler(this.cbChoices_SelectionChangeCommitted);
      // 
      // Box_PitMenu
      // 
      this.Box_PitMenu.Controls.Add(this.buttonToggleMenu);
      this.Box_PitMenu.Controls.Add(this.groupBox5);
      this.Box_PitMenu.Controls.Add(this.groupBox4);
      this.Box_PitMenu.Controls.Add(this.textBox1);
      this.Box_PitMenu.Controls.Add(this.label1);
      this.Box_PitMenu.Location = new System.Drawing.Point(3, 3);
      this.Box_PitMenu.Name = "Box_PitMenu";
      this.tableLayoutPanel1.SetRowSpan(this.Box_PitMenu, 2);
      this.Box_PitMenu.Size = new System.Drawing.Size(524, 324);
      this.Box_PitMenu.TabIndex = 0;
      this.Box_PitMenu.TabStop = false;
      this.Box_PitMenu.Text = "Pit Menu";
      // 
      // groupBox5
      // 
      this.groupBox5.Controls.Add(this.label3);
      this.groupBox5.Controls.Add(this.label2);
      this.groupBox5.Controls.Add(this.comboBoxAllTyres);
      this.groupBox5.Controls.Add(this.cbTyreChoice);
      this.groupBox5.Location = new System.Drawing.Point(4, 190);
      this.groupBox5.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
      this.groupBox5.Name = "groupBox5";
      this.groupBox5.Padding = new System.Windows.Forms.Padding(2, 2, 2, 2);
      this.groupBox5.Size = new System.Drawing.Size(366, 119);
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
      this.cbTyreChoice.Location = new System.Drawing.Point(38, 24);
      this.cbTyreChoice.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
      this.cbTyreChoice.Name = "cbTyreChoice";
      this.cbTyreChoice.Size = new System.Drawing.Size(136, 28);
      this.cbTyreChoice.TabIndex = 0;
      this.cbTyreChoice.SelectionChangeCommitted += new System.EventHandler(this.cbTyreChoice_SelectionChangeCommitted);
      // 
      // groupBox4
      // 
      this.groupBox4.Controls.Add(this.label5);
      this.groupBox4.Controls.Add(this.tbSetFuel);
      this.groupBox4.Controls.Add(this.tbCurrentFuelLevel);
      this.groupBox4.Location = new System.Drawing.Point(6, 98);
      this.groupBox4.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
      this.groupBox4.Name = "groupBox4";
      this.groupBox4.Padding = new System.Windows.Forms.Padding(2, 2, 2, 2);
      this.groupBox4.Size = new System.Drawing.Size(360, 78);
      this.groupBox4.TabIndex = 1;
      this.groupBox4.TabStop = false;
      this.groupBox4.Text = "Fuel level";
      // 
      // label5
      // 
      this.label5.AutoSize = true;
      this.label5.Location = new System.Drawing.Point(164, 37);
      this.label5.Margin = new System.Windows.Forms.Padding(2, 0, 2, 0);
      this.label5.Name = "label5";
      this.label5.Size = new System.Drawing.Size(73, 20);
      this.label5.TabIndex = 2;
      this.label5.Text = "Set level";
      // 
      // tbSetFuel
      // 
      this.tbSetFuel.Font = new System.Drawing.Font("LCDMono", 8.142858F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.tbSetFuel.Location = new System.Drawing.Point(250, 37);
      this.tbSetFuel.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
      this.tbSetFuel.Name = "tbSetFuel";
      this.tbSetFuel.Size = new System.Drawing.Size(83, 27);
      this.tbSetFuel.TabIndex = 1;
      this.tbSetFuel.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.tbSetFuel_TextChanged);
      // 
      // tbCurrentFuelLevel
      // 
      this.tbCurrentFuelLevel.Font = new System.Drawing.Font("LCDMono", 8.142858F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.tbCurrentFuelLevel.Location = new System.Drawing.Point(18, 37);
      this.tbCurrentFuelLevel.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
      this.tbCurrentFuelLevel.Name = "tbCurrentFuelLevel";
      this.tbCurrentFuelLevel.ReadOnly = true;
      this.tbCurrentFuelLevel.Size = new System.Drawing.Size(83, 27);
      this.tbCurrentFuelLevel.TabIndex = 0;
      this.tbCurrentFuelLevel.Text = "---";
      // 
      // label1
      // 
      this.label1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
      this.label1.AutoSize = true;
      this.label1.Location = new System.Drawing.Point(381, 23);
      this.label1.Name = "label1";
      this.label1.Size = new System.Drawing.Size(120, 140);
      this.label1.TabIndex = 0;
      this.label1.Text = "<- Click the\r\ndisplay then\r\nuse ASWD as \r\ncursor keys to \r\ncontrol the \r\nPit Menu" +
    "\r\ndirectly\r\n";
      this.label1.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
      // 
      // textBox1
      // 
      this.textBox1.Font = new System.Drawing.Font("LCDMono", 14F, System.Drawing.FontStyle.Bold);
      this.textBox1.Location = new System.Drawing.Point(6, 25);
      this.textBox1.Multiline = true;
      this.textBox1.Name = "textBox1";
      this.textBox1.ReadOnly = true;
      this.textBox1.Size = new System.Drawing.Size(361, 58);
      this.textBox1.TabIndex = 0;
      this.textBox1.Text = "rFactor not connected";
      this.textBox1.KeyDown += new System.Windows.Forms.KeyEventHandler(this.MainForm_KeyDown);
      // 
      // groupBox1
      // 
      this.groupBox1.Controls.Add(this.cbCategory);
      this.groupBox1.Location = new System.Drawing.Point(539, 2);
      this.groupBox1.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
      this.groupBox1.Name = "groupBox1";
      this.groupBox1.Padding = new System.Windows.Forms.Padding(2, 2, 2, 2);
      this.groupBox1.Size = new System.Drawing.Size(382, 103);
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
      this.cbCategory.Location = new System.Drawing.Point(5, 57);
      this.cbCategory.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
      this.cbCategory.Name = "cbCategory";
      this.cbCategory.Size = new System.Drawing.Size(318, 28);
      this.cbCategory.TabIndex = 2;
      this.cbCategory.SelectionChangeCommitted += new System.EventHandler(this.cbCategory_ChangeCommitted);
      // 
      // checkBox1
      // 
      this.checkBox1.AutoSize = true;
      this.checkBox1.Location = new System.Drawing.Point(539, 451);
      this.checkBox1.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
      this.checkBox1.Name = "checkBox1";
      this.checkBox1.Size = new System.Drawing.Size(171, 24);
      this.checkBox1.TabIndex = 3;
      this.checkBox1.Text = "rFactor connected";
      this.checkBox1.UseVisualStyleBackColor = true;
      // 
      // timer1
      // 
      this.timer1.Tick += new System.EventHandler(this.timer1_Tick);
      // 
      // trackBarDelay
      // 
      this.trackBarDelay.LargeChange = 50;
      this.trackBarDelay.Location = new System.Drawing.Point(7, 31);
      this.trackBarDelay.Maximum = 500;
      this.trackBarDelay.Minimum = 10;
      this.trackBarDelay.Name = "trackBarDelay";
      this.trackBarDelay.Size = new System.Drawing.Size(251, 80);
      this.trackBarDelay.SmallChange = 10;
      this.trackBarDelay.TabIndex = 3;
      this.trackBarDelay.TickFrequency = 50;
      this.toolTip1.SetToolTip(this.trackBarDelay, "200 mS seems to be \r\nthe minimum reliable\r\nvalue");
      this.trackBarDelay.Value = 40;
      this.trackBarDelay.ValueChanged += new System.EventHandler(this.trackBarDelay_ValueChanged);
      // 
      // comboBoxAllTyres
      // 
      this.comboBoxAllTyres.FormattingEnabled = true;
      this.comboBoxAllTyres.Items.AddRange(new object[] {
            "Soft",
            "Medium",
            "Hard",
            "Wet",
            "No Change"});
      this.comboBoxAllTyres.Location = new System.Drawing.Point(216, 24);
      this.comboBoxAllTyres.Margin = new System.Windows.Forms.Padding(2);
      this.comboBoxAllTyres.Name = "comboBoxAllTyres";
      this.comboBoxAllTyres.Size = new System.Drawing.Size(136, 28);
      this.comboBoxAllTyres.TabIndex = 1;
      this.comboBoxAllTyres.SelectionChangeCommitted += new System.EventHandler(this.comboBoxAllTyres_SelectionChangeCommitted);
      // 
      // groupBox6
      // 
      this.groupBox6.Controls.Add(this.labelDelay);
      this.groupBox6.Controls.Add(this.trackBarDelay);
      this.groupBox6.Location = new System.Drawing.Point(3, 333);
      this.groupBox6.Name = "groupBox6";
      this.groupBox6.Size = new System.Drawing.Size(276, 100);
      this.groupBox6.TabIndex = 8;
      this.groupBox6.TabStop = false;
      this.groupBox6.Text = "Delay between sending controls";
      this.toolTip1.SetToolTip(this.groupBox6, "200 mS seems to be \r\nthe minimum reliable\r\nvalue\r\n");
      // 
      // labelDelay
      // 
      this.labelDelay.AutoSize = true;
      this.labelDelay.Location = new System.Drawing.Point(76, 75);
      this.labelDelay.Name = "labelDelay";
      this.labelDelay.Size = new System.Drawing.Size(52, 20);
      this.labelDelay.TabIndex = 4;
      this.labelDelay.Text = "40mS";
      // 
      // label2
      // 
      this.label2.AutoSize = true;
      this.label2.Location = new System.Drawing.Point(55, 69);
      this.label2.Name = "label2";
      this.label2.Size = new System.Drawing.Size(98, 20);
      this.label2.TabIndex = 2;
      this.label2.Text = "Current tyre";
      // 
      // label3
      // 
      this.label3.AutoSize = true;
      this.label3.Location = new System.Drawing.Point(248, 69);
      this.label3.Name = "label3";
      this.label3.Size = new System.Drawing.Size(70, 20);
      this.label3.TabIndex = 3;
      this.label3.Text = "All tyres";
      // 
      // buttonToggleMenu
      // 
      this.buttonToggleMenu.Font = new System.Drawing.Font("Lucida Console", 8F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.buttonToggleMenu.Location = new System.Drawing.Point(385, 188);
      this.buttonToggleMenu.Name = "buttonToggleMenu";
      this.buttonToggleMenu.Size = new System.Drawing.Size(113, 121);
      this.buttonToggleMenu.TabIndex = 3;
      this.buttonToggleMenu.Text = "Toggle Menu Display";
      this.buttonToggleMenu.UseVisualStyleBackColor = true;
      this.buttonToggleMenu.Click += new System.EventHandler(this.button1_Click);
      // 
      // checkBox2
      // 
      this.checkBox2.AutoSize = true;
      this.checkBox2.Location = new System.Drawing.Point(7, 25);
      this.checkBox2.Name = "checkBox2";
      this.checkBox2.Size = new System.Drawing.Size(22, 21);
      this.checkBox2.TabIndex = 3;
      this.checkBox2.UseVisualStyleBackColor = true;
      this.checkBox2.CheckedChanged += new System.EventHandler(this.checkBox2_CheckedChanged);
      // 
      // numericUpDownErrors
      // 
      this.numericUpDownErrors.Location = new System.Drawing.Point(408, 18);
      this.numericUpDownErrors.Maximum = new decimal(new int[] {
            10000000,
            0,
            0,
            0});
      this.numericUpDownErrors.Name = "numericUpDownErrors";
      this.numericUpDownErrors.ReadOnly = true;
      this.numericUpDownErrors.Size = new System.Drawing.Size(116, 26);
      this.numericUpDownErrors.TabIndex = 3;
      // 
      // groupBox7
      // 
      this.groupBox7.Controls.Add(this.label7);
      this.groupBox7.Controls.Add(this.numericUpDownTests);
      this.groupBox7.Controls.Add(this.label6);
      this.groupBox7.Controls.Add(this.numericUpDownErrors);
      this.groupBox7.Controls.Add(this.checkBox2);
      this.groupBox7.Location = new System.Drawing.Point(3, 452);
      this.groupBox7.Name = "groupBox7";
      this.groupBox7.Size = new System.Drawing.Size(531, 52);
      this.groupBox7.TabIndex = 9;
      this.groupBox7.TabStop = false;
      this.groupBox7.Text = "Stress test!";
      // 
      // label6
      // 
      this.label6.AutoSize = true;
      this.label6.Location = new System.Drawing.Point(346, 20);
      this.label6.Name = "label6";
      this.label6.Size = new System.Drawing.Size(56, 20);
      this.label6.TabIndex = 4;
      this.label6.Text = "Errors";
      // 
      // label7
      // 
      this.label7.AutoSize = true;
      this.label7.Location = new System.Drawing.Point(144, 20);
      this.label7.Name = "label7";
      this.label7.Size = new System.Drawing.Size(51, 20);
      this.label7.TabIndex = 6;
      this.label7.Text = "Tests";
      // 
      // numericUpDownTests
      // 
      this.numericUpDownTests.Location = new System.Drawing.Point(206, 18);
      this.numericUpDownTests.Maximum = new decimal(new int[] {
            10000000,
            0,
            0,
            0});
      this.numericUpDownTests.Name = "numericUpDownTests";
      this.numericUpDownTests.ReadOnly = true;
      this.numericUpDownTests.Size = new System.Drawing.Size(116, 26);
      this.numericUpDownTests.TabIndex = 5;
      // 
      // MainForm
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(9F, 20F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.ClientSize = new System.Drawing.Size(951, 520);
      this.Controls.Add(this.flowLayoutPanel1);
      this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
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
      this.groupBox5.ResumeLayout(false);
      this.groupBox5.PerformLayout();
      this.groupBox4.ResumeLayout(false);
      this.groupBox4.PerformLayout();
      this.groupBox1.ResumeLayout(false);
      ((System.ComponentModel.ISupportInitialize)(this.trackBarDelay)).EndInit();
      this.groupBox6.ResumeLayout(false);
      this.groupBox6.PerformLayout();
      ((System.ComponentModel.ISupportInitialize)(this.numericUpDownErrors)).EndInit();
      this.groupBox7.ResumeLayout(false);
      this.groupBox7.PerformLayout();
      ((System.ComponentModel.ISupportInitialize)(this.numericUpDownTests)).EndInit();
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
    private System.Windows.Forms.TrackBar trackBarDelay;
    private System.Windows.Forms.ComboBox comboBoxAllTyres;
    private System.Windows.Forms.GroupBox groupBox6;
    private System.Windows.Forms.Label labelDelay;
    private System.Windows.Forms.Button buttonToggleMenu;
    private System.Windows.Forms.Label label3;
    private System.Windows.Forms.Label label2;
    private System.Windows.Forms.ToolTip toolTip1;
    private System.Windows.Forms.CheckBox checkBox2;
    private System.Windows.Forms.GroupBox groupBox7;
    private System.Windows.Forms.Label label6;
    private System.Windows.Forms.NumericUpDown numericUpDownErrors;
    private System.Windows.Forms.Label label7;
    private System.Windows.Forms.NumericUpDown numericUpDownTests;
  }
}

