// Pit Menu API to use TheIronWolf's rF2 Shared Memory Map plugin
// https://github.com/TheIronWolfModding/rF2SharedMemoryMapPlugin
//
// It provides functions to directly control the Pit Menu (selecting it,
// moving up and down, changing the menu choice) and "smart" controls that
// move to a specified category or a specified choice.
//

using System;
using System.Text;
using rF2SharedMemory;
using rF2SharedMemory.rFactor2Data;

namespace PitMenuAPI
{
  /// <summary>
  /// PitMenuAPI consists PitMenuAbstractionLayer : PitMenuController : PitMenu
  /// </summary>
  public class PitMenu
  {
    public
    SendrF2HWControl sendHWControl = new SendrF2HWControl();

    private
    MappedBuffer<rF2PitInfo> pitInfoBuffer = new MappedBuffer<rF2PitInfo>(
      rFactor2Constants.MM_PITINFO_FILE_NAME,
      false /*partial*/,
      true /*skipUnchanged*/);
    rF2PitInfo pitInfo;
    string LastControl;
    bool Connected = false;

    // Delay in mS after sending a HW control to rFactor before sending another,
    // set by experiment
    // 20 works for category selection and tyres but fuel needs it slower
    int delay = 100;
    // Shared memory scans slowly until the first control is received. It
    // returns to scanning slowly when it hasn't received a control for a while.
    int initialDelay = 200;

    ///////////////////////////////////////////////////////////////////////////
    /// Setup
    ///
    /// <summary>
    /// Connect to the Shared Memory running in rFactor
    /// </summary>
    /// <returns>
    /// true if connected
    /// </returns>
    public bool Connect()
    {
      if (!this.Connected)
      {
        this.Connected = this.sendHWControl.Connect();
        if (this.Connected)
        {
          this.pitInfoBuffer.Connect();
        }
      }
      return this.Connected;
    }

    /// <summary>
    /// Disconnect from the Shared Memory running in rFactor
    /// </summary>
    public void Disconnect()
    {
      this.pitInfoBuffer.Disconnect();
      this.sendHWControl.Disconnect();
    }


    /// <summary>
    /// Shared memory is normally scanning slowly until a control is received
    /// so send the first control (to select the Pit Menu) with a longer delay
    /// </summary>
    public bool startUsingPitMenu()
    {
      if (!this.Connected)
      {
        Connect();
      }
      if (this.Connected)
      {
        // Need to select the Pit Menu
        // If it is off ToggleMFDA will turn it on then ToggleMFDB will switch
        // to the Pit Menu
        // If it is showing MFDA ToggleMFDA will turn it off then ToggleMFDB
        // will show the Pit Menu
        // If it is showing MFD"x" ToggleMFDA will show MFDA then ToggleMFDB
        // will show the Pit Menu
        do
        {
          this.sendHWControl.SendHWControl("ToggleMFDA", true);
          System.Threading.Thread.Sleep(initialDelay);
          this.sendHWControl.SendHWControl("ToggleMFDA", false);
          System.Threading.Thread.Sleep(delay);
          this.sendHWControl.SendHWControl("ToggleMFDB", true); // Select rFactor Pit Menu
          System.Threading.Thread.Sleep(delay);
          this.sendHWControl.SendHWControl("ToggleMFDB", false); // Select rFactor Pit Menu
          System.Threading.Thread.Sleep(delay);
        }
        while (!(SoftMatchCategory("TIRE") || SoftMatchCategory("FUEL")));
      }
      return this.Connected;
    }

    /// <summary>
    /// Set the delay between sending each control
    /// After sending the first control in sequence the delay should be longer
    /// as the Shared Memory takes up to 200 mS to switch to its higher update
    /// rate.  After 200 mS without receiving any controls it returns to a
    /// 200 mS update.
    /// </summary>
    /// <param name="mS"></param>
    public void setDelay(int mS, int initialDelay)
    {
      this.delay = mS;
      this.initialDelay = initialDelay;
    }

    //////////////////////////////////////////////////////////////////////////
    /// Direct menu control
    //////////////////////////////////////////////////////////////////////////
    // Menu Categories
    /// <summary>
    /// Get the current Pit Menu category
    /// </summary>
    /// <returns>
    /// Name of the category
    /// </returns>
    public string GetCategory()
    {
      pitInfoBuffer.GetMappedData(ref pitInfo);
      var catName = GetStringFromBytes(this.pitInfo.mPitMneu.mCategoryName);
      return catName;
    }

    /// <summary>
    /// Move up to the next category
    /// </summary>
    public void CategoryUp()
    {
      sendControl("PitMenuUp");
    }

    /// <summary>
    /// Move down to the next category
    /// </summary>
    public void CategoryDown()
    {
      sendControl("PitMenuDown");
    }

    //////////////////////////////////////////////////////////////////////////
    // Menu Choices
    /// <summary>
    /// Increment the current choice
    /// </summary>
    public void ChoiceInc()
    {
      sendControl("PitMenuIncrementValue");
    }

    /// <summary>
    /// Decrement the current choice
    /// </summary>
    public void ChoiceDec()
    {
      sendControl("PitMenuDecrementValue");
    }

    /// <summary>
    /// Get the text of the current choice
    /// </summary>
    /// <returns>string</returns>
    public string GetChoice()
    {
      pitInfoBuffer.GetMappedData(ref pitInfo);
      var choiceStr = GetStringFromBytes(this.pitInfo.mPitMneu.mChoiceString);
      return choiceStr;
    }


    //////////////////////////////////////////////////////////////////////////
    /// "Smart" menu control - specify which category or choice and it will be
    /// selected
    //////////////////////////////////////////////////////////////////////////
    // Menu Categories
    /// <summary>
    /// Set the Pit Menu category
    /// </summary>
    /// <param name="category">string</param>
    /// <returns>
    /// false: Category not found
    /// </returns>
    public bool SetCategory(string category)
    {
      string InitialCategory = GetCategory();
      int tryNo = 3;
      while (GetCategory() != category)
      {
        CategoryDown();
        if (GetCategory() == InitialCategory)
        {  // Wrapped around, category not found
          if (tryNo-- > 0)
          {
            return false;
          }
        }
      }
      return true;
    }

    /// <summary>
    /// Select a category that includes "category"
    /// </summary>
    /// <param name="category"></param>
    /// <returns>
    /// True: category found
    /// </returns>
    public bool SoftMatchCategory(string category)
    {
      string InitialCategory = GetCategory();
      int tryNo = 3;
      while (!GetCategory().Contains(category))
      {
        CategoryDown();
        if (GetCategory() == InitialCategory)
        {  // Wrapped around, category not found
          if (tryNo-- > 0)
          {
            return false;
          }
        }
      }
      return true;
    }

    /// <summary>
    /// Set the current choice
    /// </summary>
    /// <param name="choice">string</param>
    /// <returns>
    /// false: Choice not found
    /// </returns>
    public bool SetChoice(string choice)
    {
      string LastChoice = GetChoice();
      bool inc = true;
      while (!GetChoice().StartsWith(choice))
      {
        if (inc)
          ChoiceInc();
        else
          ChoiceDec();
        if (GetChoice() == LastChoice)
        {
          if (inc)
          { // Go the other way
            inc = false;
          }
          else
          {
            return false;
          }
        }
        LastChoice = GetChoice();
      }
      return true;
    }

    private void sendControl(string control)
    {
      this.sendHWControl.SendHWControl(control, true);
      System.Threading.Thread.Sleep(delay);
      // Doesn't seem to be necessary to do "retVal false" too
      this.sendHWControl.SendHWControl(control, false);
      System.Threading.Thread.Sleep(delay);
    }

    //////////////////////////////////////////////////////////////////////////
    // Utils
    private static string GetStringFromBytes(byte[] bytes)
    {
      if (bytes == null)
        return "";

      var nullIdx = Array.IndexOf(bytes, (byte)0);

      return nullIdx >= 0
        ? Encoding.Default.GetString(bytes, 0, nullIdx)
        : Encoding.Default.GetString(bytes);
    }
  }
}
