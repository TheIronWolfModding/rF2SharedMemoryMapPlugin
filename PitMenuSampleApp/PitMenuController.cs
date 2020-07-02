using System;
using System.Collections.Generic;
using System.Linq;
using System.Linq.Expressions;
using System.Security.Cryptography;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using rF2SharedMemory;
using rF2SharedMemory.rFactor2Data;

namespace rF2SharedMemoryAPI
{
  class PitMenuController
  {
    private
    SendrF2HWControl SendControl = new SendrF2HWControl();

    MappedBuffer<rF2PitInfo> pitInfoBuffer = new MappedBuffer<rF2PitInfo>(rFactor2Constants.MM_PITINFO_FILE_NAME, false /*partial*/, true /*skipUnchanged*/);
    rF2PitInfo pitInfo;
    string LastControl;
    bool Connected = false;

    // Delay in mS after sending a HW control to rFactor before sending another, set by experiment
    // 20 works for category selection and tyres but fuel needs it slower
    int delay = 200;

    /// <summary>
    /// Connect to the Shared Memory running in rFactor
    /// </summary>
    /// <returns>
    /// true of connected
    /// </returns>
    public bool Connect()
    {
      this.Connected = this.SendControl.Connect();
      if (this.Connected)
      {
        this.pitInfoBuffer.Connect();
        this.SendControl.SendHWControl("ToggleMFDB", true); // Select rFactor Pit Menu
        System.Threading.Thread.Sleep(delay);
        this.SendControl.SendHWControl("ToggleMFDB", false); // Select rFactor Pit Menu
      }
      return this.Connected;
    }

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
      //var choiceStr = GetStringFromBytes(this.pitInfo.mPitMneu.mChoiceString);
      return catName;
    }

    /// <summary>
    /// Move up or down to the next category
    /// </summary>
    /// <param name="up">
    /// true: up
    /// false: down
    /// </param>
    public void UpDownOne(bool up)
    {
      string cmd;
      if (up)
        cmd = "PitMenuUp";
      else
        cmd = "PitMenuDown";
      this.SendControl.SendHWControl(cmd, true);
      System.Threading.Thread.Sleep(delay);
      this.SendControl.SendHWControl(cmd, false);
      System.Threading.Thread.Sleep(delay);
    }

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
      while (GetCategory() != category)
      {
        UpDownOne(true);
        if (GetCategory() == InitialCategory)
        {  // Wrapped around, category not found
          return false;
        }
      }
      return true;
    }

    //////////////////////////////////////////////////////////////////////////
    // Menu Choices
    /// <summary>
    /// Increment or decrement the current choice
    /// </summary>
    /// <param name="inc">
    /// true: inc
    /// false: dec
    /// </param>
    public void IncDecOne(bool inc)
    {
      string cmd;
      if (inc)
        cmd = "PitMenuIncrementValue";
      else
        cmd = "PitMenuDecrementValue";
      this.SendControl.SendHWControl(cmd, true);
      System.Threading.Thread.Sleep(delay);
      this.SendControl.SendHWControl(cmd, false);
      System.Threading.Thread.Sleep(delay);
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
        IncDecOne(inc);
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

    //////////////////////////////////////////////////////////////////////////
    // Fuel

    /// <summary>
    /// Read the fuel level in the Pit Menu display
    /// Player.JSON needs to be set "Relative Fuel Strategy":FALSE,
    /// "+ 1.6/2"	Gallons to ADD/laps "Relative Fuel Strategy":TRUE,
    /// "65/25"		Litres TOTAL/laps   "Relative Fuel Strategy":FALSE,
    /// </summary>
    /// <returns>
    /// Fuel level in current units (litres or (US?) gallons)
    /// -1 if parsing the number failed
    /// -2 if Relative Fuel Strategy true
    /// </returns>
    public int GetFuelLevel()
    {
      Int16 current = -1;
      Match match; // = Regex.Match(input, pattern);
      Regex reggie = new Regex(@"(.*)/(.*)");
      // if (this.GetCategory() == "FUEL:")
      match = reggie.Match(GetChoice());
      if (match.Groups.Count == 3)
      {
        bool parsed = Int16.TryParse(match.Groups[1].Value, out current);
        if (parsed)
        {
          if (match.Groups[1].Value.StartsWith("+"))
          {
            current = -2;
          }
        }
      }
      return current;
    }

    /// <summary>
    /// Set the fuel level in the Pit Menu display
    /// Player.JSON needs to be set "Relative Fuel Strategy":FALSE,
    /// </summary>
    /// <param name="requiredFuel"> in current units (litres or (US?) gallons)</param>
    /// <returns>
    /// true if level set (or it reached max/min possible
    /// false if the level can't be read
    /// </returns>
    public bool SetFuelLevel(int requiredFuel)
    {
      this.SetCategory("FUEL:");
      int current = GetFuelLevel();
      if (current < 0)
      {
        return false; // Can't read value
      }

      // Adjust down if necessary
      while (current > requiredFuel)
      {
        IncDecOne(false);
        int newLevel = GetFuelLevel();
        if (newLevel == current)
        { // Can't adjust further
          break;
        }
        else
        {
          current = newLevel;
        }
      }
      // Adjust up to >= required level
      while (current < requiredFuel)
      {
        IncDecOne(true);
        int newLevel = GetFuelLevel();
        if (newLevel == current)
        { // Can't adjust further
          break;
        }
        else
        {
          current = newLevel;
        }
      }
      return true;
    }

    //////////////////////////////////////////////////////////////////////////
    // Tyres

    /// <summary>
    /// Get the type of tyre selected
    /// </summary>
    /// <returns>
    /// Soft
    /// Medium
    /// Hard
    /// Wet
    /// No Change
    /// </returns>
    public string GetTyreType()
    {
      //if (this.GetCategory().Contains("TIRE"))
      string current = GetChoice();
      string result = "NO_TYRE";
      if (current.Contains("Soft"))
        result = "Soft";
      else if (current.Contains("Medium"))
        result = "Medium";
      else if (current.Contains("Hard"))
        result = "Hard";
      else if (current.Contains("Wet"))
        result = "Wet";
      else if (current.Contains("No Change"))
        result = "No Change";
      return result;
    }


    /// <summary>
    /// Set the type of tyre selected
    /// Soft
    /// Medium
    /// Hard
    /// Wet
    /// No Change
    /// </summary>
    /// <returns>
    /// true if successful
    /// </returns>
    public bool SetTyreType(string requiredType)
    {
      if (this.GetCategory().Contains("TIRE"))
      {
        string current = GetTyreType();

        // Adjust  if necessary
        while (GetTyreType() != requiredType)
        {
          IncDecOne(true);
          string newType = GetTyreType();
          if (newType == current)
          { // Didn't find it
            return false;
          }
        }
        return true;
      }
      return false;
    }


    //////////////////////////////////////////////////////////////////////////
    // Testing
    /// <summary>
    /// Set the delay between sending each control
    /// </summary>
    /// <param name="mS"></param>
    public void setDelay(int mS)
    {
      this.delay = mS;
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
