using System;
using System.Collections.Generic;
using System.Threading;
using System.Windows.Forms;
using PitMenuAPI;
using rF2SharedMemory;

namespace PitMenuSampleApp
{
  using Pmal = PitMenuAbstractionLayer;
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
    Dictionary<string, string> ttDict;
    List<string> tyreCategories;

    public MainForm()
    {
      InitializeComponent();
      trackBarInitialDelay.Value = 230;
      trackBarDelay.Value = 30;
      object sender = null; EventArgs e = null;
      trackBarInitialDelay_ValueChanged(sender, e);
      trackBarDelay_ValueChanged(sender, e);

      this.Connected = this.SendControl.Connect();
      this.checkBox1.Checked = this.Connected;
      if (this.Connected)
      {
        Pmal.Connect();
        List<string> _tyreTypes = Pmal.GetTyreTypeNames();
#if false
        ttDict = Pmal.TranslateTyreTypes(
          PitMenuAbstractionLayer.SampleTyreDict,
          _tyreTypes);
#endif
        tyreCategories = Pmal.GetAllTyreCategories();
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
        var catName = Pmal.Pmc.GetCategory();
        this.cbCategory.SelectedItem = catName;
        var choiceStr = Pmal.Pmc.GetChoice();
        this.textBox1.Text = catName + " " + choiceStr;
        int fuel = Pmal.Pmc.GetFuelLevel();
        if (fuel >= 0)
          this.tbCurrentFuelLevel.Text = fuel.ToString();
      }
      this.timer1.Stop();
    }

    private void cbCategory_ChangeCommitted(object sender, EventArgs e)
    {
      Pmal.Pmc.startUsingPitMenu();
      Pmal.Pmc.SetCategory(this.cbCategory.SelectedItem.ToString());
      this.timer1.Start();
    }

    private void cbChoices_SelectionChangeCommitted(object sender, EventArgs e)
    {
      Pmal.Pmc.startUsingPitMenu();
      Pmal.Pmc.SetChoice(this.cbChoices.SelectedItem.ToString());
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
          Pmal.Pmc.startUsingPitMenu();
          Pmal.Pmc.SetFuelLevel(level);
          this.timer1.Start();
        }
      }
    }

    private void cbTyreChoice_SelectionChangeCommitted(object sender, EventArgs e)
    {
      Pmal.Pmc.startUsingPitMenu();
      Pmal.Pmc.SetTyreType(ttDict[this.cbTyreChoice.SelectedItem.ToString()]);
      this.timer1.Start();
    }

    private void cbAllControls_SelectionChangeCommitted(object sender, EventArgs e)
    {
      this.SendControl.SendHWControl(this.cbAllControls.SelectedItem.ToString(), true);
      this.timer1.Start();
    }

    private void trackBarDelay_ValueChanged(object sender, EventArgs e)
    {
      Pmal.Pmc.setDelay(this.trackBarDelay.Value, this.trackBarInitialDelay.Value);
      this.labelDelay.Text = this.trackBarDelay.Value.ToString() + " mS";
    }
    private void trackBarInitialDelay_ValueChanged(object sender, EventArgs e)
    {
      Pmal.Pmc.setDelay(this.trackBarDelay.Value, this.trackBarInitialDelay.Value);
      this.labelInitialDelay.Text = this.trackBarInitialDelay.Value.ToString() + " mS";
    }

    private void comboBoxAllTyres_SelectionChangeCommitted(object sender, EventArgs e)
    {
      Pmal.Pmc.startUsingPitMenu();
      this.lblSettingTyreType.Text = "Setting to " +
        ttDict[this.comboBoxAllTyres.SelectedItem.ToString()];
      Pmal.SetAllTyreTypes(ttDict[this.comboBoxAllTyres.SelectedItem.ToString()]);
      this.timer1.Start();
    }

    private void buttonToggleMenu_Click(object sender, EventArgs e)
    {
      this.SendControl.SendHWControl("ToggleMFDB", true);
      this.LastControl = "ToggleMFDB";
      this.timer1.Start();
    }

    private void btnStartUsingPitMenu_Click(object sender, EventArgs e)
    {
      Pmal.Pmc.startUsingPitMenu();
    }

    private void cbStressTest_CheckedChanged(object sender, EventArgs e)
    {
      if (this.cbStressTest.Checked)
      {
        this.cbTestStartup.Checked = false;
        numericUpDownTests.Value = 0;
        numericUpDownErrors.Value = 0;
        while (true)
        {
          Pmal.Pmc.startUsingPitMenu();
          foreach (string tyreType in Pmal.GetTyreTypeNames())
          {
            if (tyreType == "No Change")
            {
              continue;
            }
            foreach (string tyre in tyreCategories)
            {
              Pmal.setCategoryAndChoice(tyre, tyreType);
              Application.DoEvents();
              var catName = Pmal.Pmc.GetCategory();
              if (catName != tyre)
              {
                numericUpDownErrors.Value += 1;
                System.Threading.Thread.Sleep(1000);
                Pmal.Pmc.startUsingPitMenu();
              }
              var choiceStr = Pmal.Pmc.GetChoice();
              if (!choiceStr.Contains(tyreType))
              {
                numericUpDownErrors.Value += 1;
                System.Threading.Thread.Sleep(1000);
                Pmal.Pmc.startUsingPitMenu();
              }

              numericUpDownTests.Value += 1;

              Application.DoEvents();
              if (!this.cbStressTest.Checked)
                return;
            }
          }

          Pmal.Pmc.SetFuelLevel(25);
          if (!Pmal.Pmc.GetChoice().Contains("25"))
          {
            numericUpDownErrors.Value += 1;
            System.Threading.Thread.Sleep(1000);
            Pmal.Pmc.startUsingPitMenu();
          }
          if (!this.cbStressTest.Checked)
            return;
          foreach (string tyre in Pmal.GetFrontTyreCategories())
          {
            Pmal.setCategoryAndChoice(tyre, "No Change");
          }
          Application.DoEvents();
          Pmal.Pmc.startUsingPitMenu();
          Pmal.Pmc.SetFuelLevel(15);
          if (!Pmal.Pmc.GetChoice().Contains("15"))
          {
            numericUpDownErrors.Value += 1;
            System.Threading.Thread.Sleep(1000);
            Pmal.Pmc.startUsingPitMenu();
          }
          if (!this.cbStressTest.Checked)
            return;
        }
      }
    }

    private void cbTestStartup_CheckedChanged(object sender, EventArgs e)
    {
      if (this.cbTestStartup.Checked)
      {
        this.cbStressTest.Checked = false;
        numericUpDownTests.Value = 0;
        numericUpDownErrors.Value = 0;
        while (this.cbTestStartup.Checked)
        {
          Pmal.Pmc.sendHWControl.SendHWControl("ToggleMFDA", true);
          System.Threading.Thread.Sleep(trackBarInitialDelay.Value);
          Pmal.Pmc.sendHWControl.SendHWControl("ToggleMFDA", false);
          System.Threading.Thread.Sleep(trackBarDelay.Value);
          Pmal.Pmc.sendHWControl.SendHWControl("ToggleMFDB", true); // Select rFactor Pit Menu
          System.Threading.Thread.Sleep(trackBarDelay.Value);
          Pmal.Pmc.sendHWControl.SendHWControl("ToggleMFDB", false); // Select rFactor Pit Menu
          System.Threading.Thread.Sleep(trackBarDelay.Value);
          if (Pmal.Pmc.SoftMatchCategory("TIRE") || Pmal.Pmc.SoftMatchCategory("FUEL"))
          {
            numericUpDownTests.Value += 1;
            Pmal.Pmc.CategoryDown();
          }
          else
          {
            numericUpDownErrors.Value += 1;
            System.Threading.Thread.Sleep(1000);
          }
          Application.DoEvents();
          System.Threading.Thread.Sleep(100);
        }
      }
    }
  }
}
