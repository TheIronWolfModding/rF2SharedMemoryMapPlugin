/*
rF2SMMonitor is visual debugger for rF2 Shared Memory Plugin.

MainForm implementation, contains main loop and render calls.

Author: The Iron Wolf (vleonavicius@hotmail.com)
Website: thecrewchief.org
*/
using rF2SMMonitor.rFactor2Data;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.IO.MemoryMappedFiles;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Windows.Forms;
using static rF2SMMonitor.rFactor2Constants;

namespace rF2SMMonitor
{
  public partial class MainForm : Form
  {
    // Connection fields
    private const int CONNECTION_RETRY_INTERVAL_MS = 1000;
    private const int DISCONNECTED_CHECK_INTERVAL_MS = 15000;
    private const float DEGREES_IN_RADIAN = 57.2957795f;
    private const int LIGHT_MODE_REFRESH_MS = 500;
    System.Windows.Forms.Timer connectTimer = new System.Windows.Forms.Timer();
    System.Windows.Forms.Timer disconnectTimer = new System.Windows.Forms.Timer();
    bool connected = false;

    private class MappedBuffer<MappedBufferT>
    {
      const int NUM_MAX_RETRIEES = 10;
      readonly int RF2_BUFFER_VERSION_BLOCK_SIZE_BYTES = Marshal.SizeOf(typeof(rF2MappedBufferVersionBlock));
      readonly int RF2_BUFFER_VERSION_BLOCK_WITH_SIZE_SIZE_BYTES = Marshal.SizeOf(typeof(rF2MappedBufferVersionBlockWithSize));

      readonly int BUFFER_SIZE_BYTES;
      readonly string BUFFER_NAME;

      // Holds the entire byte array that can be marshalled to a MappedBufferT.  Partial updates
      // only read changed part of buffer, ignoring trailing uninteresting bytes.  However,
      // to marshal we still need to supply entire structure size.  So, on update new bytes are copied
      // (outside of the mutex).
      byte[] fullSizeBuffer = null;

      MemoryMappedFile memoryMappedFile = null;

      public MappedBuffer(string buffName)
      {
        this.BUFFER_SIZE_BYTES = Marshal.SizeOf(typeof(MappedBufferT));
        this.BUFFER_NAME = buffName;
      }

      public void Connect()
      {
        this.memoryMappedFile = MemoryMappedFile.OpenExisting(this.BUFFER_NAME);

        // NOTE: Make sure that BUFFER_SIZE matches the structure size in the plugin (debug mode prints that).
        this.fullSizeBuffer = new byte[this.BUFFER_SIZE_BYTES];
      }

      public void Disconnect()
      {
        if (this.memoryMappedFile != null)
          this.memoryMappedFile.Dispose();

        this.memoryMappedFile = null;
        this.fullSizeBuffer = null;
      }

      // Read success statistics.
      int numReadRetries = 0;
      int numReadRetriesOnCheck = 0;
      int numReadFailures = 0;
      int numStuckFrames = 0;
      uint stuckVersionBegin = 0;
      uint stuckVersionEnd = 0;

      public void GetMappedData(ref MappedBufferT mappedData)
      {
        using (var sharedMemoryStreamView = this.memoryMappedFile.CreateViewStream())
        {
          this.GetMappedDataHelper(sharedMemoryStreamView, this.BUFFER_SIZE_BYTES, ref mappedData);
        }
      }
          //var sharedMemoryStream = new BinaryReader(sharedMemoryStreamView);
          /*for (var retry = 0; retry < NUM_MAX_RETRIEES; ++retry)  // TODO: max
          {
            sharedMemoryStream.BaseStream.Position = 0;
            sharedMemoryReadBuffer = sharedMemoryStream.ReadBytes(this.BUFFER_SIZE_BYTES);

            // Marshal rF2 State buffer
            var handle = GCHandle.Alloc(sharedMemoryReadBuffer, GCHandleType.Pinned);
            mappedData = (MappedBufferT)Marshal.PtrToStructure(handle.AddrOfPinnedObject(), typeof(MappedBufferT));
            var versionHeader = (rF2MappedBufferVersionBlock)Marshal.PtrToStructure(handle.AddrOfPinnedObject(), typeof(rF2MappedBufferVersionBlock));
            handle.Free();

            // Verify if Begin/End versions match:
            if (versionHeader.mVersionUpdateBegin != versionHeader.mVersionUpdateEnd)
            {
              failedVersionBegin = versionHeader.mVersionUpdateBegin;
              failedVersionEnd = versionHeader.mVersionUpdateEnd;

              if (failedVersionBegin == this.stuckVersionBegin
                && failedVersionEnd == this.stuckVersionEnd)
              {
                // If this is stale "out of sync" situation, that is, we're stuck in, no point in retrying here.
                // Could be bug in a game, plugin or a game crash.
                ++this.numStuckFrames;
                return;
              }

              ++numReadRetries;
              continue;
            }

            // Read the version header one more time.  This is for the case, that might not be even possible in reality,
            // but it is possible in my head.  Since it is cheap, no harm reading again really, aside from retry that
            // sometimes will be required if buffer is updated between checks.
            //
            // Anyway, the case is
            // * Reader thread reads updateBegin version and continues to read buffer. 
            // * Simultaneously, Writer thread begins overwriting the buffer.
            // * If Reader thread reads updateEnd before Writer thread finishes, it will look 
            //   like updateBegin == updateEnd.But we actually just read a partially overwritten buffer.
            //
            // Hence, this second check is needed here.Even if writer thread still hasn't finished writing,
            // we still will be able to detect this case because now updateBegin version changed, so we
            // know Writer is updating the buffer.

            sharedMemoryStream.BaseStream.Position = 0;
            sharedMemoryReadBuffer = sharedMemoryStream.ReadBytes(this.RF2_BUFFER_VERSION_BLOCK_SIZE_BYTES);

            handle = GCHandle.Alloc(sharedMemoryReadBuffer, GCHandleType.Pinned);
            var versionHeaderCheck2 = (rF2MappedBufferVersionBlock)Marshal.PtrToStructure(handle.AddrOfPinnedObject(), typeof(rF2MappedBufferVersionBlock));
            handle.Free();

            if (versionHeader.mVersionUpdateBegin != versionHeaderCheck2.mVersionUpdateBegin
              || versionHeader.mVersionUpdateEnd != versionHeaderCheck2.mVersionUpdateEnd)
            {
              ++this.numReadRetriesOnCheck;
              continue;
            }

            // Success.
            this.stuckVersionBegin = this.stuckVersionEnd = 0;
            return;*/
          //}

          // Failure.
         // this.stuckVersionBegin = failedVersionBegin;
          //this.stuckVersionEnd = failedVersionEnd;

//          ++numReadFailures;
        //}

        /*//
        // IMPORTANT:  Clients that do not need consistency accross the whole buffer, like dashboards that visualize data, _do not_ need to use mutexes.
        //

        // Note: if it is critical for client minimize wait time, same strategy as plugin uses can be employed.
        // Pass 0 timeout and skip update if someone holds the lock.
        if (this.mutex.WaitOne(5000))
        {
          byte[] sharedMemoryReadBuffer = null;
          try
          {
            bool buf1Current = false;
            // Try buffer 1:
            using (var sharedMemoryStreamView = this.memoryMappedFile.CreateViewStream())
            {
              var sharedMemoryStream = new BinaryReader(sharedMemoryStreamView);
              sharedMemoryReadBuffer = sharedMemoryStream.ReadBytes(this.RF2_BUFFER_HEADER_SIZE_BYTES);

              // Marhsal header.
              var headerHandle = GCHandle.Alloc(sharedMemoryReadBuffer, GCHandleType.Pinned);
              var header = (rF2MappedBufferHeader)Marshal.PtrToStructure(headerHandle.AddrOfPinnedObject(), typeof(rF2MappedBufferHeader));
              headerHandle.Free();

              if (header.mCurrentRead == 1)
              {
                sharedMemoryStream.BaseStream.Position = 0;
                sharedMemoryReadBuffer = sharedMemoryStream.ReadBytes(this.BUFFER_SIZE_BYTES);
                buf1Current = true;
              }
            }

            // Read buffer 2
            if (!buf1Current)
            {
              using (var sharedMemoryStreamView = this.memoryMappedFile2.CreateViewStream())
              {
                var sharedMemoryStream = new BinaryReader(sharedMemoryStreamView);
                sharedMemoryReadBuffer = sharedMemoryStream.ReadBytes(this.BUFFER_SIZE_BYTES);
              }
            }
          }
          finally
          {
            this.mutex.ReleaseMutex();
          }

          // Marshal rF2 State buffer
          var handle = GCHandle.Alloc(sharedMemoryReadBuffer, GCHandleType.Pinned);
          
          mappedData = (MappedBufferT)Marshal.PtrToStructure(handle.AddrOfPinnedObject(), typeof(MappedBufferT));

          handle.Free();
        }*/
      //}

      private void GetMappedDataHelper(MemoryMappedViewStream sharedMemoryStreamView, int bufferSizeBytes, ref MappedBufferT mappedData)
      {
        var sharedMemoryStream = new BinaryReader(sharedMemoryStreamView);
        uint failedVersionBegin = 0;
        uint failedVersionEnd = 0;
        for (var retry = 0; retry < MappedBuffer<MappedBufferT>.NUM_MAX_RETRIEES; ++retry)
        {
          sharedMemoryStream.BaseStream.Position = 0;
          var sharedMemoryReadBuffer = sharedMemoryStream.ReadBytes(bufferSizeBytes);

          // Marshal rF2 State buffer
          var handle = GCHandle.Alloc(sharedMemoryReadBuffer, GCHandleType.Pinned);
          // TODO: this could possibly be avoded till we succeded.
          mappedData = (MappedBufferT)Marshal.PtrToStructure(handle.AddrOfPinnedObject(), typeof(MappedBufferT));
          var versionHeader = (rF2MappedBufferVersionBlock)Marshal.PtrToStructure(handle.AddrOfPinnedObject(), typeof(rF2MappedBufferVersionBlock));
          handle.Free();

          // Verify if Begin/End versions match:
          if (versionHeader.mVersionUpdateBegin != versionHeader.mVersionUpdateEnd)
          {
            failedVersionBegin = versionHeader.mVersionUpdateBegin;
            failedVersionEnd = versionHeader.mVersionUpdateEnd;

            if (failedVersionBegin == this.stuckVersionBegin
              && failedVersionEnd == this.stuckVersionEnd)
            {
              // If this is stale "out of sync" situation, that is, we're stuck in, no point in retrying here.
              // Could be bug in a game, plugin or a game crash.
              ++this.numStuckFrames;
              return;
            }

            ++numReadRetries;
            continue;
          }

          // Read the version header one more time.  This is for the case, that might not be even possible in reality,
          // but it is possible in my head.  Since it is cheap, no harm reading again really, aside from retry that
          // sometimes will be required if buffer is updated between checks.
          //
          // Anyway, the case is
          // * Reader thread reads updateBegin version and continues to read buffer. 
          // * Simultaneously, Writer thread begins overwriting the buffer.
          // * If Reader thread reads updateEnd before Writer thread finishes, it will look 
          //   like updateBegin == updateEnd.But we actually just read a partially overwritten buffer.
          //
          // Hence, this second check is needed here.Even if writer thread still hasn't finished writing,
          // we still will be able to detect this case because now updateBegin version changed, so we
          // know Writer is updating the buffer.

          sharedMemoryStream.BaseStream.Position = 0;
          sharedMemoryReadBuffer = sharedMemoryStream.ReadBytes(this.RF2_BUFFER_VERSION_BLOCK_SIZE_BYTES);

          handle = GCHandle.Alloc(sharedMemoryReadBuffer, GCHandleType.Pinned);
          var versionHeaderCheck2 = (rF2MappedBufferVersionBlock)Marshal.PtrToStructure(handle.AddrOfPinnedObject(), typeof(rF2MappedBufferVersionBlock));
          handle.Free();

          if (versionHeader.mVersionUpdateBegin != versionHeaderCheck2.mVersionUpdateBegin
            || versionHeader.mVersionUpdateEnd != versionHeaderCheck2.mVersionUpdateEnd)
          {
            ++this.numReadRetriesOnCheck;
            continue;
          }

          // Success.
          this.stuckVersionBegin = this.stuckVersionEnd = 0;
          return;
        }

        // Failure.
        this.stuckVersionBegin = failedVersionBegin;
        this.stuckVersionEnd = failedVersionEnd;

        ++this.numReadFailures;
      }

      public string GetStats()
      {
        return string.Format("R: {0}    R2: {1}    F: {2}    ST: {3}", this.numReadRetries, this.numReadRetriesOnCheck, this.numReadFailures, this.numStuckFrames);
      }

      public void GetMappedDataPartial(ref MappedBufferT mappedData)
      {
        using (var sharedMemoryStreamView = this.memoryMappedFile.CreateViewStream())
        {
          // Get the size of 
          var sharedMemoryStream = new BinaryReader(sharedMemoryStreamView);
          uint failedVersionBegin = 0;
          uint failedVersionEnd = 0;
          for (var retry = 0; retry < MappedBuffer<MappedBufferT>.NUM_MAX_RETRIEES; ++retry)
          {
            sharedMemoryStream.BaseStream.Position = 0;
            var sharedMemoryReadBufferHeader = sharedMemoryStream.ReadBytes(this.RF2_BUFFER_VERSION_BLOCK_WITH_SIZE_SIZE_BYTES);

            var handle = GCHandle.Alloc(sharedMemoryReadBufferHeader, GCHandleType.Pinned);
            var versionHeaderWithSize = (rF2MappedBufferVersionBlockWithSize)Marshal.PtrToStructure(handle.AddrOfPinnedObject(), typeof(rF2MappedBufferVersionBlockWithSize)); ;
            handle.Free();

            sharedMemoryStream.BaseStream.Position = 0;
            var sharedMemoryReadBuffer = sharedMemoryStream.ReadBytes(versionHeaderWithSize.mBytesUpdatedHint != 0 ? versionHeaderWithSize.mBytesUpdatedHint : this.BUFFER_SIZE_BYTES);

            // Marshal rF2 State buffer
            handle = GCHandle.Alloc(sharedMemoryReadBuffer, GCHandleType.Pinned);
            var versionHeader = (rF2MappedBufferVersionBlock)Marshal.PtrToStructure(handle.AddrOfPinnedObject(), typeof(rF2MappedBufferVersionBlock));
            handle.Free();

            // Verify if Begin/End versions match:
            if (versionHeader.mVersionUpdateBegin != versionHeader.mVersionUpdateEnd)
            {
              failedVersionBegin = versionHeader.mVersionUpdateBegin;
              failedVersionEnd = versionHeader.mVersionUpdateEnd;

              if (failedVersionBegin == this.stuckVersionBegin
                && failedVersionEnd == this.stuckVersionEnd)
              {
                // If this is stale "out of sync" situation, that is, we're stuck in, no point in retrying here.
                // Could be bug in a game, plugin or a game crash.
                ++this.numStuckFrames;
                return;
              }

              Thread.Sleep(1);
              ++numReadRetries;
              continue;
            }

            // Read the version header one more time.  This is for the case, that might not be even possible in reality,
            // but it is possible in my head.  Since it is cheap, no harm reading again really, aside from retry that
            // sometimes will be required if buffer is updated between checks.
            //
            // Anyway, the case is
            // * Reader thread reads updateBegin version and continues to read buffer. 
            // * Simultaneously, Writer thread begins overwriting the buffer.
            // * If Reader thread reads updateEnd before Writer thread finishes, it will look 
            //   like updateBegin == updateEnd.But we actually just read a partially overwritten buffer.
            //
            // Hence, this second check is needed here.Even if writer thread still hasn't finished writing,
            // we still will be able to detect this case because now updateBegin version changed, so we
            // know Writer is updating the buffer.

            sharedMemoryStream.BaseStream.Position = 0;
            var sharedMemoryReadBufferLastCheck = sharedMemoryStream.ReadBytes(this.RF2_BUFFER_VERSION_BLOCK_SIZE_BYTES);

            handle = GCHandle.Alloc(sharedMemoryReadBufferLastCheck, GCHandleType.Pinned);
            var versionHeaderCheck2 = (rF2MappedBufferVersionBlock)Marshal.PtrToStructure(handle.AddrOfPinnedObject(), typeof(rF2MappedBufferVersionBlock));
            handle.Free();

            if (versionHeader.mVersionUpdateBegin != versionHeaderCheck2.mVersionUpdateBegin
              || versionHeader.mVersionUpdateEnd != versionHeaderCheck2.mVersionUpdateEnd)
            {
              ++this.numReadRetriesOnCheck;
              continue;
            }

            // Marshal rF2 State buffer
            Array.Copy(sharedMemoryReadBuffer, this.fullSizeBuffer, sharedMemoryReadBuffer.Length);
            handle = GCHandle.Alloc(this.fullSizeBuffer, GCHandleType.Pinned);
            mappedData = (MappedBufferT)Marshal.PtrToStructure(handle.AddrOfPinnedObject(), typeof(MappedBufferT));
            handle.Free();

            // Success.
            this.stuckVersionBegin = this.stuckVersionEnd = 0;
            return;
          }

          // Failure.
          this.stuckVersionBegin = failedVersionBegin;
          this.stuckVersionEnd = failedVersionEnd;

          ++this.numReadFailures;
        }

        /*
        //
        // IMPORTANT:  Clients that do not need consistency accross the whole buffer, like dashboards that visualize data, _do not_ need to use mutexes.
        //

        // Note: if it is critical for client minimize wait time, same strategy as plugin uses can be employed.
        // Pass 0 timeout and skip update if someone holds the lock.

        // Using partial buffer copying reduces time under lock.  Scoring by 30%, telemetry by 70%.
        if (this.mutex.WaitOne(5000))
        {
          byte[] sharedMemoryReadBuffer = null;
          try
          {
            bool buf1Current = false;
            // Try buffer 1:
            using (var sharedMemoryStreamView = this.memoryMappedFile.CreateViewStream())
            {
              var sharedMemoryStream = new BinaryReader(sharedMemoryStreamView);
              sharedMemoryReadBuffer = sharedMemoryStream.ReadBytes(this.RF2_BUFFER_HEADER_WITH_SIZE_SIZE_BYTES);

              // Marhsal header.
              var headerHandle = GCHandle.Alloc(sharedMemoryReadBuffer, GCHandleType.Pinned);
              var header = (rF2MappedBufferHeaderWithSize)Marshal.PtrToStructure(headerHandle.AddrOfPinnedObject(), typeof(rF2MappedBufferHeaderWithSize));
              headerHandle.Free();

              if (header.mCurrentRead == 1)
              {
                sharedMemoryStream.BaseStream.Position = 0;
                sharedMemoryReadBuffer = sharedMemoryStream.ReadBytes(header.mBytesUpdatedHint != 0 ? header.mBytesUpdatedHint : this.BUFFER_SIZE_BYTES);
                buf1Current = true;
              }
            }

            // Read buffer 2
            if (!buf1Current)
            {
              using (var sharedMemoryStreamView = this.memoryMappedFile2.CreateViewStream())
              {
                var sharedMemoryStream = new BinaryReader(sharedMemoryStreamView);
                sharedMemoryReadBuffer = sharedMemoryStream.ReadBytes(this.RF2_BUFFER_HEADER_WITH_SIZE_SIZE_BYTES);

                // Marhsal header.
                var headerHandle = GCHandle.Alloc(sharedMemoryReadBuffer, GCHandleType.Pinned);
                var header = (rF2MappedBufferHeaderWithSize)Marshal.PtrToStructure(headerHandle.AddrOfPinnedObject(), typeof(rF2MappedBufferHeaderWithSize));
                headerHandle.Free();

                sharedMemoryStream.BaseStream.Position = 0;
                sharedMemoryReadBuffer = sharedMemoryStream.ReadBytes(header.mBytesUpdatedHint != 0 ? header.mBytesUpdatedHint : this.BUFFER_SIZE_BYTES);
              }
            }
          }
          finally
          {
            this.mutex.ReleaseMutex();
          }

          Array.Copy(sharedMemoryReadBuffer, this.fullSizeBuffer, sharedMemoryReadBuffer.Length);

          // Marshal rF2 State buffer
          var handle = GCHandle.Alloc(this.fullSizeBuffer, GCHandleType.Pinned);

          mappedData = (MappedBufferT)Marshal.PtrToStructure(handle.AddrOfPinnedObject(), typeof(MappedBufferT));

          handle.Free();
        }*/
      }
    }

    MappedBuffer<rF2Telemetry> telemetryBuffer = new MappedBuffer<rF2Telemetry>(rFactor2Constants.MM_TELEMETRY_FILE_NAME);
    MappedBuffer<rF2Scoring> scoringBuffer = new MappedBuffer<rF2Scoring>(rFactor2Constants.MM_SCORING_FILE_NAME);
    MappedBuffer<rF2Rules> rulesBuffer = new MappedBuffer<rF2Rules>(rFactor2Constants.MM_RULES_FILE_NAME);
    MappedBuffer<rF2Extended> extendedBuffer = new MappedBuffer<rF2Extended>(rFactor2Constants.MM_EXTENDED_FILE_NAME);

    // Marshalled views:
    rF2Telemetry telemetry;
    rF2Scoring scoring;
    rF2Rules rules;
    rF2Extended extended;

    // Track rF2 transitions.
    TransitionTracker tracker = new TransitionTracker();

    // Config
    IniFile config = new IniFile();
    float scale = 2.0f;
    float xOffset = 0.0f;
    float yOffset = 0.0f;
    int focusVehicle = 0;
    bool centerOnVehicle = true;
    bool rotateAroundVehicle = true;
    bool logPhaseAndState = true;
    bool logDamage = true;
    bool logTiming = true;
    bool logRules = true;
    bool logLightMode = false;
    public static bool useStockCarRulesPlugin = false;

    [StructLayout(LayoutKind.Sequential)]
    public struct NativeMessage
    {
      public IntPtr Handle;
      public uint Message;
      public IntPtr WParameter;
      public IntPtr LParameter;
      public uint Time;
      public Point Location;
    }

    [DllImport("user32.dll")]
    public static extern int PeekMessage(out NativeMessage message, IntPtr window, uint filterMin, uint filterMax, uint remove);

    public MainForm()
    {
      this.InitializeComponent();

      this.DoubleBuffered = true;
      this.StartPosition = FormStartPosition.Manual;
      this.Location = new Point(0, 0);

      this.EnableControls(false);
      this.scaleTextBox.KeyDown += TextBox_KeyDown;
      this.scaleTextBox.LostFocus += ScaleTextBox_LostFocus;
      this.xOffsetTextBox.KeyDown += TextBox_KeyDown;
      this.xOffsetTextBox.LostFocus += XOffsetTextBox_LostFocus;
      this.yOffsetTextBox.KeyDown += TextBox_KeyDown;
      this.yOffsetTextBox.LostFocus += YOffsetTextBox_LostFocus;
      this.focusVehTextBox.KeyDown += TextBox_KeyDown;
      this.focusVehTextBox.LostFocus += FocusVehTextBox_LostFocus;
      this.setAsOriginCheckBox.CheckedChanged += SetAsOriginCheckBox_CheckedChanged;
      this.rotateAroundCheckBox.CheckedChanged += RotateAroundCheckBox_CheckedChanged;
      this.checkBoxLogPhaseAndState.CheckedChanged += CheckBoxLogPhaseAndState_CheckedChanged;
      this.checkBoxLogDamage.CheckedChanged += CheckBoxLogDamage_CheckedChanged;
      this.checkBoxLogTiming.CheckedChanged += CheckBoxLogTiming_CheckedChanged;
      this.checkBoxLogRules.CheckedChanged += CheckBoxLogRules_CheckedChanged;
      this.checkBoxLightMode.CheckedChanged += CheckBoxLightMode_CheckedChanged;
      this.checkBoxStockCarRules.CheckedChanged += CheckBoxStockCarRules_CheckedChanged;
      this.MouseWheel += MainForm_MouseWheel;

      this.LoadConfig();
      this.connectTimer.Interval = CONNECTION_RETRY_INTERVAL_MS;
      this.connectTimer.Tick += ConnectTimer_Tick;
      this.disconnectTimer.Interval = DISCONNECTED_CHECK_INTERVAL_MS;
      this.disconnectTimer.Tick += DisconnectTimer_Tick;
      this.connectTimer.Start();
      this.disconnectTimer.Start();

      this.view.BorderStyle = BorderStyle.Fixed3D;
      this.view.Paint += View_Paint;
      this.MouseClick += MainForm_MouseClick;
      this.view.MouseClick += MainForm_MouseClick;

      Application.Idle += HandleApplicationIdle;
    }

    private void CheckBoxLogRules_CheckedChanged(object sender, EventArgs e)
    {
      this.logRules = this.checkBoxLogRules.Checked;
      this.config.Write("logRules", this.logRules ? "1" : "0");
    }

    private void CheckBoxLightMode_CheckedChanged(object sender, EventArgs e)
    {
      this.logLightMode = this.checkBoxLightMode.Checked;
      
      // Disable/enable rendering options
      this.globalGroupBox.Enabled = !this.logLightMode;
      this.groupBoxFocus.Enabled = !this.logLightMode;

      this.config.Write("logLightMode", this.logLightMode ? "1" : "0");
    }

    private void CheckBoxStockCarRules_CheckedChanged(object sender, EventArgs e)
    {
      MainForm.useStockCarRulesPlugin = this.checkBoxStockCarRules.Checked;

      this.config.Write("useStockCarRules", MainForm.useStockCarRulesPlugin ? "1" : "0");
    }

    private void CheckBoxLogDamage_CheckedChanged(object sender, EventArgs e)
    {
      this.logDamage = this.checkBoxLogDamage.Checked;
      this.config.Write("logDamage", this.logDamage ? "1" : "0");
    }

    private void CheckBoxLogTiming_CheckedChanged(object sender, EventArgs e)
    {
      this.logTiming = this.checkBoxLogTiming.Checked;
      this.config.Write("logTiming", this.logTiming ? "1" : "0");
    }

    private void CheckBoxLogPhaseAndState_CheckedChanged(object sender, EventArgs e)
    {
      this.logPhaseAndState = this.checkBoxLogPhaseAndState.Checked;
      this.config.Write("logPhaseAndState", this.logPhaseAndState ? "1" : "0");
    }

    private void MainForm_MouseClick(object sender, MouseEventArgs e)
    {
      //if (e.Button == MouseButtons.Right)
       // this.interpolationStats.Reset();
    }

    private void YOffsetTextBox_LostFocus(object sender, EventArgs e)
    {
      float result = 0.0f;
      if (float.TryParse(this.yOffsetTextBox.Text, out result))
      {
        this.yOffset = result;
        this.config.Write("yOffset", this.yOffset.ToString());
      }
      else
        this.yOffsetTextBox.Text = this.yOffset.ToString();

    }

    private void XOffsetTextBox_LostFocus(object sender, EventArgs e)
    {
      float result = 0.0f;
      if (float.TryParse(this.xOffsetTextBox.Text, out result))
      {
        this.xOffset = result;
        this.config.Write("xOffset", this.xOffset.ToString());
      }
      else
        this.xOffsetTextBox.Text = this.xOffset.ToString();

    }

    private void MainForm_MouseWheel(object sender, MouseEventArgs e)
    {
      float step = 0.5f;
      if (this.scale < 5.0f)
        step = 0.25f;
      else if (this.scale < 2.0f)
        step = 0.1f;
      else if (this.scale < 1.0f)
        step = 0.05f;

      if (e.Delta > 0)
        this.scale += step;
      else if (e.Delta < 0)
        this.scale -= step;

      if (this.scale <= 0.0f)
        this.scale = 0.05f;

      this.config.Write("scale", this.scale.ToString());
      this.scaleTextBox.Text = this.scale.ToString();
    }

    private void RotateAroundCheckBox_CheckedChanged(object sender, EventArgs e)
    {
      this.rotateAroundVehicle = this.rotateAroundCheckBox.Checked;
      this.config.Write("rotateAroundVehicle", this.rotateAroundVehicle ? "1" : "0");
    }

    private void SetAsOriginCheckBox_CheckedChanged(object sender, EventArgs e)
    {
      this.centerOnVehicle = this.setAsOriginCheckBox.Checked;
      this.rotateAroundCheckBox.Enabled = this.setAsOriginCheckBox.Checked;
      this.config.Write("centerOnVehicle", this.centerOnVehicle ? "1" : "0");
    }

    private void FocusVehTextBox_LostFocus(object sender, EventArgs e)
    {
      int result = 0;
      if (int.TryParse(this.focusVehTextBox.Text, out result) && result >= 0)
      {
        this.focusVehicle = result;
        this.config.Write("focusVehicle", this.focusVehicle.ToString());
      }
      else
        this.focusVehTextBox.Text = this.focusVehTextBox.ToString();
    }

    private void ScaleTextBox_LostFocus(object sender, EventArgs e)
    {
      float result = 0.0f;
      if (float.TryParse(this.scaleTextBox.Text, out result))
      {
        this.scale = result;
        this.config.Write("scale", this.scale.ToString());
      }
      else
        this.scaleTextBox.Text = this.scale.ToString();
    }

    private void TextBox_KeyDown(object sender, KeyEventArgs e)
    {
      if (e.KeyCode == Keys.Enter)
        this.view.Focus();
    }

    protected override void Dispose(bool disposing)
    {
      if (disposing && (components != null))
        components.Dispose();
      
      if (disposing)
        Disconnect();

      base.Dispose(disposing);
    }

    // Amazing loop implementation by Josh Petrie from:
    // http://gamedev.stackexchange.com/questions/67651/what-is-the-standard-c-windows-forms-game-loop
    bool IsApplicationIdle()
    {
      NativeMessage result;
      return PeekMessage(out result, IntPtr.Zero, (uint)0, (uint)0, (uint)0) == 0;
    }

    void HandleApplicationIdle(object sender, EventArgs e)
    {
      while (this.IsApplicationIdle())
      {
        try
        {
          this.MainUpdate();

          if (base.WindowState == FormWindowState.Minimized)
          {
            // being lazy lazy lazy.
            this.tracker.TrackPhase(ref this.scoring, ref this.telemetry, ref this.extended, null, this.logPhaseAndState);
            this.tracker.TrackDamage(ref this.scoring, ref this.telemetry, ref this.extended, null, this.logDamage);
            this.tracker.TrackTimings(ref this.scoring, ref this.telemetry, ref this.rules, ref this.extended, null, this.logTiming);
            this.tracker.TrackRules(ref this.scoring, ref this.telemetry, ref this.rules, ref this.extended, null, this.logRules);
          }
          else
          {
            this.MainRender();
          }

          if (this.logLightMode)
            Thread.Sleep(LIGHT_MODE_REFRESH_MS);
        }
        catch (Exception)
        {
          this.Disconnect();
        }
      }
    }

    void MainUpdate()
    {
      if (!this.connected)
        return;

      try
      {
        extendedBuffer.GetMappedData(ref extended);
        scoringBuffer.GetMappedDataPartial(ref scoring);
        //scoringBuffer.GetMappedData(ref scoring);
        //telemetryBuffer.GetMappedDataPartial(ref telemetry);
        telemetryBuffer.GetMappedData(ref telemetry);
        rulesBuffer.GetMappedDataPartial(ref rules);
        //rulesBuffer.GetMappedData(ref rules);
      }
      catch (Exception)
      {
        this.Disconnect();
      }
    }

    void MainRender()
    {
      this.view.Refresh();
    }

    int framesAvg = 20;
    int frame = 0;
    int fps = 0;
    Stopwatch fpsStopWatch = new Stopwatch();
    private void UpdateFPS()
    {
      if (this.frame > this.framesAvg)
      {
        this.fpsStopWatch.Stop();
        var tsSinceLastRender = this.fpsStopWatch.Elapsed;
        this.fps = tsSinceLastRender.Milliseconds > 0 ? (1000 * this.framesAvg) / tsSinceLastRender.Milliseconds : 0;
        this.fpsStopWatch.Restart();
        this.frame = 0;
      }
      else
        ++this.frame;
    }

    private static string GetStringFromBytes(byte[] bytes)
    {
      if (bytes == null)
        return "";

      var nullIdx = Array.IndexOf(bytes, (byte)0);

      return nullIdx >= 0
        ? Encoding.Default.GetString(bytes, 0, nullIdx) 
        : Encoding.Default.GetString(bytes);
    }

    // Corrdinate conversion:
    // rF2 +x = screen +x
    // rF2 +z = screen -z
    // rF2 +yaw = screen -yaw
    // If I don't flip z, the projection will look from below.
    void View_Paint(object sender, PaintEventArgs e)
    {
      var g = e.Graphics;

      this.tracker.TrackPhase(ref this.scoring, ref this.telemetry, ref this.extended, g, this.logPhaseAndState);
      this.tracker.TrackDamage(ref this.scoring, ref this.telemetry, ref this.extended, g, this.logDamage);
      this.tracker.TrackTimings(ref this.scoring, ref this.telemetry, ref this.rules, ref this.extended, g, this.logTiming);
      this.tracker.TrackRules(ref this.scoring, ref this.telemetry, ref this.rules, ref this.extended, g, this.logRules);

      this.UpdateFPS();

      if (!this.connected)
      {
        var brush = new SolidBrush(System.Drawing.Color.Black);
        g.DrawString("Not connected", SystemFonts.DefaultFont, brush, 3.0f, 3.0f);

        if (this.logLightMode)
          return;
      }
      else
      {
        var brush = new SolidBrush(System.Drawing.Color.Green);

        var currX = 3.0f;
        var currY = 3.0f;
        float yStep = SystemFonts.DefaultFont.Height;
        var gameStateText = new StringBuilder();
        gameStateText.Append(
          $"Plugin Version:    Expected: 2.4.0.0 64bit   Actual: {MainForm.GetStringFromBytes(this.extended.mVersion)} {(this.extended.is64bit == 1 ? "64bit" : "32bit")}    {(this.extended.mHostedPluginVars.StockCarRules_IsHosted == 1 ? "SCR Plugin Hosted" : "")}    FPS: {this.fps}");

        // Draw header
        g.DrawString(gameStateText.ToString(), SystemFonts.DefaultFont, brush, currX, currY);

        gameStateText.Clear();

        // Build map of mID -> telemetry.mVehicles[i]. 
        // They are typically matching values, however, we need to handle online cases and dropped vehicles (mID can be reused).
        var idsToTelIndices = new Dictionary<long, int>();
        for (int i = 0; i < this.telemetry.mNumVehicles; ++i)
        {
          if (!idsToTelIndices.ContainsKey(this.telemetry.mVehicles[i].mID))
            idsToTelIndices.Add(this.telemetry.mVehicles[i].mID, i);
        }

        var playerVehScoring = GetPlayerScoring(ref this.scoring);

        var scoringPlrId = playerVehScoring.mID;
        var playerVeh = new rF2VehicleTelemetry();
        int resolvedPlayerIdx = -1;  // We're fine here with unitialized vehicle telemetry..
        if (idsToTelIndices.ContainsKey(scoringPlrId))
        {
          resolvedPlayerIdx = idsToTelIndices[scoringPlrId];
          playerVeh = this.telemetry.mVehicles[resolvedPlayerIdx];
        }

        // Figure out prev session end player mID
        var playerSessionEndInfo = new rF2VehScoringCapture();
        for (int i = 0; i < this.extended.mSessionTransitionCapture.mNumScoringVehicles; ++i)
        {
          var veh = this.extended.mSessionTransitionCapture.mScoringVehicles[i];
          if (veh.mIsPlayer == 1)
            playerSessionEndInfo = veh;
        }

        gameStateText.Append(
          "mElapsedTime:\n"
          + "mCurrentET:\n"
          + "mElapsedTime-mCurrentET:\n"
          + "mDetlaTime:\n"
          + "mInvulnerable:\n"
          + "mVehicleName:\n"
          + "mTrackName:\n"
          + "mLapStartET:\n"
          + "mLapDist:\n"
          + "mEndET:\n"
          + "mPlayerName:\n"
          + "mPlrFileName:\n\n"
          + "Session Started:\n"
          + "Sess. End Session:\n"
          + "Sess. End Phase:\n"
          + "Sess. End Place:\n"
          + "Sess. End Finish:\n"
          + "Display msg capture:\n"
          );

        // Col 1 labels
        g.DrawString(gameStateText.ToString(), SystemFonts.DefaultFont, brush, currX, currY += yStep);

        gameStateText.Clear();

        gameStateText.Append(
                $"{playerVeh.mElapsedTime:N3}\n"
                + $"{this.scoring.mScoringInfo.mCurrentET:N3}\n"
                + $"{(playerVeh.mElapsedTime - this.scoring.mScoringInfo.mCurrentET):N3}\n"
                + $"{playerVeh.mDeltaTime:N3}\n"
                + (this.extended.mPhysics.mInvulnerable == 0 ? "off" : "on") + "\n"
                + $"{MainForm.GetStringFromBytes(playerVeh.mVehicleName)}\n"
                + $"{MainForm.GetStringFromBytes(playerVeh.mTrackName)}\n"
                + $"{playerVeh.mLapStartET:N3}\n"
                + $"{this.scoring.mScoringInfo.mLapDist:N3}\n"
                + (this.scoring.mScoringInfo.mEndET < 0.0 ? "Unknown" : this.scoring.mScoringInfo.mEndET.ToString("N3")) + "\n"
                + $"{MainForm.GetStringFromBytes(this.scoring.mScoringInfo.mPlayerName)}\n"
                + $"{MainForm.GetStringFromBytes(this.scoring.mScoringInfo.mPlrFileName)}\n\n"
                + $"{this.extended.mSessionStarted != 0}\n"
                + $"{TransitionTracker.GetSessionString(this.extended.mSessionTransitionCapture.mSession)}\n"
                + $"{(rFactor2Constants.rF2GamePhase)this.extended.mSessionTransitionCapture.mGamePhase}\n"
                + $"{playerSessionEndInfo.mPlace}\n"
                + $"{(rFactor2Constants.rF2FinishStatus)playerSessionEndInfo.mFinishStatus}\n"
                + $"{MainForm.GetStringFromBytes(this.extended.mDisplayedMessageUpdateCapture)}\n"
                );

        // Col1 values
        g.DrawString(gameStateText.ToString(), SystemFonts.DefaultFont, Brushes.Purple, currX + 145, currY);

        // Print buffer stats.
        gameStateText.Clear();
        gameStateText.Append(
          "Telemetry:\n"
          + "Scoring:\n"
          + "Rules:\n"
          + "Extended:");

        g.DrawString(gameStateText.ToString(), SystemFonts.DefaultFont, Brushes.Black, 1600, 570);

        gameStateText.Clear();
        gameStateText.Append(
          this.telemetryBuffer.GetStats() + '\n'
          + this.scoringBuffer.GetStats() + '\n'
          + this.rulesBuffer.GetStats() + '\n'
          + this.extendedBuffer.GetStats());

        g.DrawString(gameStateText.ToString(), SystemFonts.DefaultFont, Brushes.Black, 1700, 570);
        
        if (this.scoring.mScoringInfo.mNumVehicles == 0
          || resolvedPlayerIdx == -1)  // We need telemetry for stats below.
          return;

        gameStateText.Clear();

        gameStateText.Append(
          "mTimeIntoLap:\n"
          + "mEstimatedLapTime:\n"
          + "mTimeBehindNext:\n"
          + "mTimeBehindLeader:\n"
          + "mPitGroup:\n"
          + "mLapDist(Plr):\n"
          + "mLapDist(Est):\n"
          + "yaw:\n"
          + "pitch:\n"
          + "roll:\n"
          + "speed:\n");

        // Col 2 labels
        g.DrawString(gameStateText.ToString(), SystemFonts.DefaultFont, brush, currX += 275, currY);
        gameStateText.Clear();


        // Calculate derivatives:
        var yaw = Math.Atan2(playerVeh.mOri[RowZ].x, playerVeh.mOri[RowZ].z);

        var pitch = Math.Atan2(-playerVeh.mOri[RowY].z,
          Math.Sqrt(playerVeh.mOri[RowX].z * playerVeh.mOri[RowX].z + playerVeh.mOri[RowZ].z * playerVeh.mOri[RowZ].z));

        var roll = Math.Atan2(playerVeh.mOri[RowY].x,
          Math.Sqrt(playerVeh.mOri[RowX].x * playerVeh.mOri[RowX].x + playerVeh.mOri[RowZ].x * playerVeh.mOri[RowZ].x));

        var speed = Math.Sqrt((playerVeh.mLocalVel.x * playerVeh.mLocalVel.x)
          + (playerVeh.mLocalVel.y * playerVeh.mLocalVel.y)
          + (playerVeh.mLocalVel.z * playerVeh.mLocalVel.z));

        // Estimate lapdist
        // See how much ahead telemetry is ahead of scoring update
        var delta = playerVeh.mElapsedTime - scoring.mScoringInfo.mCurrentET;
        var lapDistEstimated = playerVehScoring.mLapDist;
        if (delta > 0.0)
        {
          var localZAccelEstimated = playerVehScoring.mLocalAccel.z * delta;
          var localZVelEstimated = playerVehScoring.mLocalVel.z + localZAccelEstimated;

          lapDistEstimated = playerVehScoring.mLapDist - localZVelEstimated * delta;
        }

        gameStateText.Append(
          $"{playerVehScoring.mTimeIntoLap:N3}\n"
          + $"{playerVehScoring.mEstimatedLapTime:N3}\n"
          + $"{playerVehScoring.mTimeBehindNext:N3}\n"
          + $"{playerVehScoring.mTimeBehindLeader:N3}\n"
          + $"{MainForm.GetStringFromBytes(playerVehScoring.mPitGroup)}\n"
          + $"{playerVehScoring.mLapDist:N3}\n"
          + $"{lapDistEstimated:N3}\n"
          + $"{yaw:N3}\n"
          + $"{pitch:N3}\n"
          + $"{roll:N3}\n"
          + string.Format("{0:n3} m/s {1:n4} km/h\n", speed, speed * 3.6));

        // Col2 values
        g.DrawString(gameStateText.ToString(), SystemFonts.DefaultFont, Brushes.Purple, currX + 120, currY);

        if (this.logLightMode)
            return;

        // Branch of UI choice: origin center or car# center
        // Fix rotation on car of choice or no.
        // Draw axes
        // Scale will be parameter, scale applied last on render to zoom.
        float scale = this.scale;

        var xVeh = (float)playerVeh.mPos.x;
        var zVeh = (float)playerVeh.mPos.z;
        var yawVeh = yaw;

        // View center
        var xScrOrigin = this.view.Width / 2.0f;
        var yScrOrigin = this.view.Height / 2.0f;
        if (!this.centerOnVehicle)
        {
          // Set world origin.
          g.TranslateTransform(xScrOrigin, yScrOrigin);
          this.RenderOrientationAxis(g);
          g.ScaleTransform(scale, scale);

          RenderCar(g, xVeh, -zVeh, -(float)yawVeh, Brushes.Green);

          for (int i = 0; i < this.telemetry.mNumVehicles; ++i)
          {
            if (i == resolvedPlayerIdx)
              continue;

            var veh = this.telemetry.mVehicles[i];
            var thisYaw = Math.Atan2(veh.mOri[2].x, veh.mOri[2].z);
            this.RenderCar(g,
              (float)veh.mPos.x,
              -(float)veh.mPos.z,
              -(float)thisYaw, Brushes.Red);
          }
        }
        else
        {
          g.TranslateTransform(xScrOrigin, yScrOrigin);

          if (this.rotateAroundVehicle)
            g.RotateTransform(180.0f + (float)yawVeh * DEGREES_IN_RADIAN);

          this.RenderOrientationAxis(g);
          g.ScaleTransform(scale, scale);
          g.TranslateTransform(-xVeh, zVeh);

          RenderCar(g, xVeh, -zVeh, -(float)yawVeh, Brushes.Green);

          for (int i = 0; i < this.telemetry.mNumVehicles; ++i)
          {
            if (i == resolvedPlayerIdx)
              continue;

            var veh = this.telemetry.mVehicles[i];
            var thisYaw = Math.Atan2(veh.mOri[2].x, veh.mOri[2].z);
            this.RenderCar(g,
              (float)veh.mPos.x,
              -(float)veh.mPos.z,
              -(float)thisYaw, Brushes.Red);
          }
        }
      }
    }

    public static rF2VehicleScoring GetPlayerScoring(ref rF2Scoring scoring)
    {
      var playerVehScoring = new rF2VehicleScoring();
      for (int i = 0; i < scoring.mScoringInfo.mNumVehicles; ++i)
      {
        var vehicle = scoring.mVehicles[i];
        switch ((rFactor2Constants.rF2Control)vehicle.mControl)
        {
          case rFactor2Constants.rF2Control.AI:
          case rFactor2Constants.rF2Control.Player:
          case rFactor2Constants.rF2Control.Remote:
            if (vehicle.mIsPlayer == 1)
              playerVehScoring = vehicle;

            break;

          default:
            continue;
        }

        if (playerVehScoring.mIsPlayer == 1)
          break;
      }

      return playerVehScoring;
    }


    // Length
    // 174.6in (4,435mm)
    // 175.6in (4,460mm) (Z06, ZR1)
    // Width
    // 72.6in (1,844mm)
    // 75.9in (1,928mm) (Z06, ZR1)
    /*PointF[] carPoly =
    {
        new PointF(0.922f, 2.217f),
        new PointF(0.922f, -1.4f),
        new PointF(1.3f, -1.4f),
        new PointF(0.0f, -2.217f),
        new PointF(-1.3f, -1.4f),
        new PointF(-0.922f, -1.4f),
        new PointF(-0.922f, 2.217f),
      };*/

    PointF[] carPoly =
    {
      new PointF(-0.922f, -2.217f),
      new PointF(-0.922f, 1.4f),
      new PointF(-1.3f, 1.4f),
      new PointF(0.0f, 2.217f),
      new PointF(1.3f, 1.4f),
      new PointF(0.922f, 1.4f),
      new PointF(0.922f, -2.217f),
    };

    private void RenderCar(Graphics g, float x, float y, float yaw, Brush brush)
    {
      var state = g.Save();

      g.TranslateTransform(x, y);

      g.RotateTransform(yaw * DEGREES_IN_RADIAN);

      g.FillPolygon(brush, this.carPoly);

      g.Restore(state);
    }

    static float arrowSide = 10.0f;
    PointF[] arrowHead =
    {
      new PointF(-arrowSide / 2.0f, -arrowSide / 2.0f),
      new PointF(0.0f, arrowSide / 2.0f),
      new PointF(arrowSide / 2.0f, -arrowSide / 2.0f)
    };

    private void RenderOrientationAxis(Graphics g)
    {

      float length = 1000.0f;
      float arrowDistX = this.view.Width / 2.0f - 10.0f;
      float arrowDistY = this.view.Height / 2.0f - 10.0f;

      // X (x screen) axis
      g.DrawLine(Pens.Red, -length, 0.0f, length, 0.0f);
      var state = g.Save();
      g.TranslateTransform(this.rotateAroundVehicle ? arrowDistY : arrowDistX, 0.0f);
      g.RotateTransform(-90.0f);
      g.FillPolygon(Brushes.Red, this.arrowHead);
      g.RotateTransform(90.0f);
      g.DrawString("x+", SystemFonts.DefaultFont, Brushes.Red, -10.0f, 10.0f);
      g.Restore(state);

      state = g.Save();
      // Z (y screen) axis
      g.DrawLine(Pens.Blue, 0.0f, -length, 0.0f, length);
      g.TranslateTransform(0.0f, -arrowDistY);
      g.RotateTransform(180.0f);
      g.FillPolygon(Brushes.Blue, this.arrowHead);
      g.DrawString("z+", SystemFonts.DefaultFont, Brushes.Blue, 10.0f, -10.0f);

      g.Restore(state);
    }

    private void ConnectTimer_Tick(object sender, EventArgs e)
    {
      if (!this.connected)
      {
        try
        {
          this.telemetryBuffer.Connect();
          this.scoringBuffer.Connect();
          this.rulesBuffer.Connect();
          this.extendedBuffer.Connect();

          this.connected = true;

          this.EnableControls(true);
        }
        catch (Exception)
        {
          Disconnect();
        }
      }
    }

    private void DisconnectTimer_Tick(object sender, EventArgs e)
    {
      if (!this.connected)
        return;

      try
      {
        // Alternatively, I could release resources and try re-acquiring them immidiately.
        var processes = Process.GetProcessesByName(rF2SMMonitor.rFactor2Constants.RFACTOR2_PROCESS_NAME);
        if (processes.Length == 0)
          Disconnect();
      }
      catch (Exception)
      {
        Disconnect();
      }
    }

    private void Disconnect()
    {
      this.extendedBuffer.Disconnect();
      this.scoringBuffer.Disconnect();
      this.rulesBuffer.Disconnect();
      this.telemetryBuffer.Disconnect();

      this.connected = false;

      this.EnableControls(false);
    }

    void EnableControls(bool enable)
    {
      this.globalGroupBox.Enabled = enable;
      this.groupBoxFocus.Enabled = enable;
      this.groupBoxLogging.Enabled = enable;
      this.groupBoxMisc.Enabled = enable;

      this.focusVehLabel.Enabled = false;
      this.focusVehTextBox.Enabled = false;
      this.xOffsetLabel.Enabled = false;
      this.xOffsetTextBox.Enabled = false;
      this.yOffsetLabel.Enabled = false;
      this.yOffsetTextBox.Enabled = false;

      if (enable)
      {
        this.rotateAroundCheckBox.Enabled = this.setAsOriginCheckBox.Checked;
        this.globalGroupBox.Enabled = !this.logLightMode;
        this.groupBoxFocus.Enabled = !this.logLightMode;
      }
    }

    void LoadConfig()
    {
      float result = 0.0f;
      this.scale = 2.0f;
      if (float.TryParse(this.config.Read("scale"), out result))
        this.scale = result;

      if (this.scale <= 0.0f)
        this.scale = 0.1f;

      this.scaleTextBox.Text = this.scale.ToString();

      result = 0.0f;
      this.xOffset = 0.0f;
      if (float.TryParse(this.config.Read("xOffset"), out result))
        this.xOffset = result;

      this.xOffsetTextBox.Text = this.xOffset.ToString();

      result = 0.0f;
      this.yOffset = 0.0f;
      if (float.TryParse(this.config.Read("yOffset"), out result))
        this.yOffset = result;

      this.yOffsetTextBox.Text = this.yOffset.ToString();

      int intResult = 0;
      this.focusVehicle = 0;
      if (int.TryParse(this.config.Read("focusVehicle"), out intResult) && intResult >= 0)
        this.focusVehicle = intResult;

      this.focusVehTextBox.Text = this.focusVehicle.ToString();

      intResult = 0;
      this.centerOnVehicle = true;
      if (int.TryParse(this.config.Read("centerOnVehicle"), out intResult) && intResult == 0)
        this.centerOnVehicle = false;

      this.setAsOriginCheckBox.Checked = this.centerOnVehicle;

      intResult = 0;
      this.rotateAroundVehicle = true;
      if (int.TryParse(this.config.Read("rotateAroundVehicle"), out intResult) && intResult == 0)
        this.rotateAroundVehicle = false;

      this.rotateAroundCheckBox.Checked = this.rotateAroundVehicle;

      intResult = 0;
      this.logLightMode = false;
      if (int.TryParse(this.config.Read("logLightMode"), out intResult) && intResult == 1)
        this.logLightMode = true;

      this.checkBoxLightMode.Checked = this.logLightMode;

      intResult = 0;
      this.logPhaseAndState = true;
      if (int.TryParse(this.config.Read("logPhaseAndState"), out intResult) && intResult == 0)
        this.logPhaseAndState = false;

      this.checkBoxLogPhaseAndState.Checked = this.logPhaseAndState;

      intResult = 0;
      this.logDamage = true;
      if (int.TryParse(this.config.Read("logDamage"), out intResult) && intResult == 0)
        this.logDamage = false;

      this.checkBoxLogDamage.Checked = this.logDamage;

      intResult = 0;
      this.logTiming = true;
      if (int.TryParse(this.config.Read("logTiming"), out intResult) && intResult == 0)
        this.logTiming = false;
      
      this.checkBoxLogTiming.Checked = this.logTiming;

      intResult = 0;
      this.logRules = true;
      if (int.TryParse(this.config.Read("logRules"), out intResult) && intResult == 0)
        this.logRules = false;

      this.checkBoxLogRules.Checked = this.logRules;

      intResult = 0;
      MainForm.useStockCarRulesPlugin = false;
      this.checkBoxStockCarRules.Enabled = false;
      // Disable this option for now, it might come in handy down the line.
      //if (int.TryParse(this.config.Read("useStockCarRules"), out intResult) && intResult == 1)
      // MainForm.useStockCarRulesPlugin = true;

      //this.checkBoxStockCarRules.Checked = MainForm.useStockCarRulesPlugin;
    }
  }
}