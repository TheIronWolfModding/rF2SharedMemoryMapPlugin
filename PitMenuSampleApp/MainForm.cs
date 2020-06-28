using System;
using System.Collections.Generic;
using System.Windows.Forms;
using rF2SharedMemory;
using rF2SharedMemoryAPI;

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

    string LastControl;
    bool Connected = false;
    PitMenuController Pmc = new PitMenuController();
    public MainForm()
    {
      InitializeComponent();
      this.Connected = this.SendControl.Connect();
      this.checkBox1.Checked = this.Connected;
      if (this.Connected)
      {
        this.Pmc.Connect();
      }
      this.timer1.Start();
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
    private void timer1_Tick(object sender, EventArgs e)
    {
      if (this.Connected)
      {
        //this.SendControl.SendHWControl(this.LastControl, false);

        var catName = this.Pmc.GetCategory();
        this.cbCategory.SelectedItem = catName;
        var choiceStr = this.Pmc.GetChoice();
        this.textBox1.Text = catName + " " + choiceStr;
        int fuel = this.Pmc.GetFuelLevel();
        if (fuel >= 0)
          this.tbCurrentFuelLevel.Text = fuel.ToString();
      }
      this.timer1.Stop();
    }

    private void cbCategory_ChangeCommitted(object sender, EventArgs e)
    {
      this.Pmc.SetCategory(this.cbCategory.SelectedItem.ToString());
      this.timer1.Start();
    }

    private void cbChoices_SelectionChangeCommitted(object sender, EventArgs e)
    {
      this.Pmc.SetChoice(this.cbChoices.SelectedItem.ToString());
      this.timer1.Start();
    }

    private void tbSetFuel_TextChanged(object sender, KeyPressEventArgs e)
    {
      if (e.KeyChar == '\r')
      {
        Int16 level;
        bool parsed = Int16.TryParse(tbSetFuel.Text, out level);
        if (parsed && level >= 0)
        {
          this.Pmc.SetFuelLevel(level);
          this.timer1.Start();
        }
      }
    }

    private void cbTyreChoice_SelectionChangeCommitted(object sender, EventArgs e)
    {
      this.Pmc.SetTyreType(this.cbTyreChoice.SelectedItem.ToString());
      this.timer1.Start();
    }

    private void cbAllControls_SelectionChangeCommitted(object sender, EventArgs e)
    {
      this.SendControl.SendHWControl(this.cbAllControls.SelectedItem.ToString(), true);
      this.timer1.Start();
    }
  }
}
