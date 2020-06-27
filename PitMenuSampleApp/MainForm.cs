using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;
using rF2SharedMemory;
using rF2SharedMemory.rFactor2Data;

namespace PitMenuSampleApp
{
  public partial class MainForm : Form
  {
    SendrF2HWControl SendControl = new SendrF2HWControl();
    Dictionary<Keys, string> KeysToPitControls = new Dictionary<Keys, string>()
    {
      {Keys.A,  "PitMenuDecrementValue"},
      {Keys.D,  "PitMenuIncrementValue"},
      {Keys.W,  "PitMenuUp"},
      {Keys.S,  "PitMenuDown"}
    };

    MappedBuffer<rF2PitInfo> pitInfoBuffer = new MappedBuffer<rF2PitInfo>(rFactor2Constants.MM_PITINFO_FILE_NAME, false /*partial*/, true /*skipUnchanged*/);
    rF2PitInfo pitInfo;
    string LastControl;
    bool Connected = false;

    public MainForm()
    {
      InitializeComponent();
      this.Connected = this.SendControl.Connect();
      this.checkBox1.Checked = this.Connected;
      if (this.Connected)
      {
        this.pitInfoBuffer.Connect();
      }
    }

    private void MainForm_KeyDown(object sender, KeyEventArgs e)
    {
      if (this.KeysToPitControls.ContainsKey(e.KeyCode))
      {
        this.SendControl.SendHWControl(this.KeysToPitControls[e.KeyCode], true);
        this.LastControl = this.KeysToPitControls[e.KeyCode];
        this.timer1.Start();
      }
    }

    private void MainForm_KeyUp(object sender, KeyEventArgs e)
    {
      if (this.KeysToPitControls.ContainsKey(e.KeyCode))
      {
        this.SendControl.SendHWControl(this.KeysToPitControls[e.KeyCode], false);
      }
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

    private void timer1_Tick(object sender, EventArgs e)
    {
      if (this.Connected)
      {
        this.SendControl.SendHWControl(this.LastControl, false);

        pitInfoBuffer.GetMappedData(ref pitInfo);
        var catName = GetStringFromBytes(this.pitInfo.mPitMneu.mCategoryName);
        var choiceStr = GetStringFromBytes(this.pitInfo.mPitMneu.mChoiceString);
        this.textBox1.Text = catName + choiceStr;
      }
      this.timer1.Stop();
    }

    private void comboBox1_ChangeCommitted(object sender, EventArgs e)
    {
      this.SendControl.SendHWControl(this.comboBox1.SelectedItem.ToString(), true);
      this.timer1.Start();
    }
  }
}
