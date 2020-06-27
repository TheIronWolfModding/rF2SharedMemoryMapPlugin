using System;
using System.Collections.Generic;
using System.Linq;
using System.Security.Cryptography;
using System.Text;
using System.Threading.Tasks;
using rF2SharedMemory;
using rF2SharedMemory.rFactor2Data;

namespace PitMenuSampleApp
{
  class PitMenuController
  {
    SendrF2HWControl SendControl = new SendrF2HWControl();

    MappedBuffer<rF2PitInfo> pitInfoBuffer = new MappedBuffer<rF2PitInfo>(rFactor2Constants.MM_PITINFO_FILE_NAME, false /*partial*/, true /*skipUnchanged*/);
    rF2PitInfo pitInfo;
    string LastControl;
    bool Connected = false;

    private int delay = 100;

    public bool Connect()
    {
      this.Connected = this.SendControl.Connect();
      if (this.Connected)
      {
        this.pitInfoBuffer.Connect();
      }
      return this.Connected;
    }

    public string ReadPitMenuCategory()
    {
      pitInfoBuffer.GetMappedData(ref pitInfo);
      var catName = GetStringFromBytes(this.pitInfo.mPitMneu.mCategoryName);
      //var choiceStr = GetStringFromBytes(this.pitInfo.mPitMneu.mChoiceString);
      return catName;
    }

    public bool SelectCategory(string category)
    {
      string InitialCategory = ReadPitMenuCategory();
      while (ReadPitMenuCategory() != category)
      {
        this.SendControl.SendHWControl("PitMenuUp", true);
        System.Threading.Thread.Sleep(delay);
        this.SendControl.SendHWControl("PitMenuUp", false);
        System.Threading.Thread.Sleep(delay);
        if (ReadPitMenuCategory() == InitialCategory)
        {
          return false; // Category not found
        }
      }
      return true;
    }

    public string ReadPitMenuChoice()
    {
      pitInfoBuffer.GetMappedData(ref pitInfo);
      var choiceStr = GetStringFromBytes(this.pitInfo.mPitMneu.mChoiceString);
      return choiceStr;
    }

    public bool SelectChoice(string choice)
    {
      string LastChoice = ReadPitMenuChoice();
      string cmd = "PitMenuIncrementValue";
      while (!ReadPitMenuChoice().StartsWith(choice))
      {
        this.SendControl.SendHWControl(cmd, true);
        System.Threading.Thread.Sleep(delay);
        this.SendControl.SendHWControl(cmd, false);
        System.Threading.Thread.Sleep(delay);
        if (ReadPitMenuChoice() == LastChoice)
        {
          if (cmd == "PitMenuIncrementValue")
          { // Go the other way
            cmd = "PitMenuDecrementValue";
          }
          else
          {
            return false; // Choice not found
          }
        }
        LastChoice = ReadPitMenuChoice();
      }
      return true;
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
  }
}
