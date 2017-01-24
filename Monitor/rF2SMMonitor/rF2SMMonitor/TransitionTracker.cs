using rF2SMMonitor.rFactor2Data;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using static rF2SMMonitor.rFactor2Constants;

namespace rF2SMMonitor
{
  internal class TransitionTracker
  {
    private string GetEnumString<T>(sbyte value)
    {
      var enumType = typeof(T);

      var enumValue = (T)Enum.ToObject(enumType, value);
      return Enum.IsDefined(enumType, enumValue) ? enumValue.ToString() : string.Format("Unknown({0})", value);
    }

    private string GetEnumString<T>(byte value)
    {
      var enumType = typeof(T);

      var enumValue = (T)Enum.ToObject(enumType, value);
      return Enum.IsDefined(enumType, enumValue) ? enumValue.ToString() : string.Format("Unknown({0})", value);
    }

    internal class PhaseAndState
    {
      internal rF2GamePhase gamePhase = (rF2GamePhase)Enum.ToObject(typeof(rF2GamePhase), -255);
      internal int session = -255;
      internal rF2YellowFlagState yellowFlagState = (rF2YellowFlagState)Enum.ToObject(typeof(rF2YellowFlagState), -255);
      internal int playerSector = -255;
      internal int currentSectorExtended = -255;
      internal byte inRealTime = 255;
      internal rF2YellowFlagState sectorFlag1 = (rF2YellowFlagState)Enum.ToObject(typeof(rF2YellowFlagState), -255);
      internal rF2YellowFlagState sectorFlag2 = (rF2YellowFlagState)Enum.ToObject(typeof(rF2YellowFlagState), -255);
      internal rF2YellowFlagState sectorFlag3 = (rF2YellowFlagState)Enum.ToObject(typeof(rF2YellowFlagState), -255);
      internal rF2Control control;
      internal byte inPits = 255;
      internal byte isPlayer = 255;
      internal int place = -255;      
      internal rF2PitState pitState = (rF2PitState)Enum.ToObject(typeof(rF2PitState), -255);
      internal rF2GamePhase individualPhase = (rF2GamePhase)Enum.ToObject(typeof(rF2GamePhase), -255);
      internal rF2PrimaryFlag flag = (rF2PrimaryFlag)Enum.ToObject(typeof(rF2PrimaryFlag), -255);
      internal byte underYellow = 255;
      internal rF2CountLapFlag countLapFlag = (rF2CountLapFlag)Enum.ToObject(typeof(rF2CountLapFlag), -255);
      internal byte inGarageStall = 255;
    }

    internal PhaseAndState prevPhaseAndSate = new PhaseAndState();
    internal StringBuilder sbChanged = new StringBuilder();
    internal StringBuilder sbLabel = new StringBuilder();
    internal StringBuilder sbValues = new StringBuilder();

    internal void TrackPhase(ref rF2State state, Graphics g)
    {
      try
      {
        if (state.mNumVehicles == 0)
          return;

        var playerVeh = state.mVehicles[0];
        Debug.Assert(state.mID == playerVeh.mID);

        var ps = new PhaseAndState();

        ps.gamePhase = (rF2GamePhase)state.mGamePhase;
        ps.session = state.mSession;
        ps.yellowFlagState = (rF2YellowFlagState)state.mYellowFlagState;
        ps.playerSector = state.mVehicles[0].mSector == 0 ? 3 : state.mVehicles[0].mSector;
        ps.currentSectorExtended = state.mCurrentSector;
        ps.inRealTime = state.mInRealtime;
        ps.sectorFlag1 = (rF2YellowFlagState)state.mSectorFlag[0];
        ps.sectorFlag2 = (rF2YellowFlagState)state.mSectorFlag[1];
        ps.sectorFlag3 = (rF2YellowFlagState)state.mSectorFlag[2];
        ps.control = (rF2Control)playerVeh.mControl;
        ps.inPits = playerVeh.mInPits;
        ps.isPlayer = playerVeh.mIsPlayer;
        ps.place = playerVeh.mPlace;
        ps.pitState = (rF2PitState)playerVeh.mPitState;
        ps.individualPhase = (rF2GamePhase)playerVeh.mIndividualPhase;
        ps.flag = (rF2PrimaryFlag)playerVeh.mFlag;
        ps.underYellow =  playerVeh.mUnderYellow;
        ps.countLapFlag = (rF2CountLapFlag)playerVeh.mCountLapFlag;
        ps.inGarageStall = playerVeh.mInGarageStall;

        // Only refresh UI if there's change.
        if (this.prevPhaseAndSate.gamePhase != ps.gamePhase
          || this.prevPhaseAndSate.session != ps.session
          || this.prevPhaseAndSate.yellowFlagState != ps.yellowFlagState
          || this.prevPhaseAndSate.playerSector != ps.playerSector
          || this.prevPhaseAndSate.currentSectorExtended != ps.currentSectorExtended
          || this.prevPhaseAndSate.inRealTime != ps.inRealTime
          || this.prevPhaseAndSate.sectorFlag1 != ps.sectorFlag1
          || this.prevPhaseAndSate.sectorFlag2 != ps.sectorFlag2
          || this.prevPhaseAndSate.sectorFlag3 != ps.sectorFlag3
          || this.prevPhaseAndSate.control != ps.control
          || this.prevPhaseAndSate.inPits != ps.inPits
          || this.prevPhaseAndSate.isPlayer != ps.isPlayer
          || this.prevPhaseAndSate.place != ps.place
          || this.prevPhaseAndSate.pitState != ps.pitState
          || this.prevPhaseAndSate.individualPhase != ps.individualPhase
          || this.prevPhaseAndSate.flag != ps.flag
          || this.prevPhaseAndSate.underYellow != ps.underYellow
          || this.prevPhaseAndSate.countLapFlag != ps.countLapFlag
          || this.prevPhaseAndSate.inGarageStall != ps.inGarageStall)
        {
          this.sbChanged = new StringBuilder();
          sbChanged.Append((this.prevPhaseAndSate.gamePhase != ps.gamePhase ? "***\n" : "\n")
            + (this.prevPhaseAndSate.session != ps.session ? "***\n" : "\n")
            + (this.prevPhaseAndSate.yellowFlagState != ps.yellowFlagState ? "***\n" : "\n")
            + (this.prevPhaseAndSate.playerSector != ps.playerSector ? "***\n" : "\n")
            + (this.prevPhaseAndSate.currentSectorExtended != ps.currentSectorExtended ? "***\n" : "\n")
            + (this.prevPhaseAndSate.inRealTime != ps.inRealTime ? "***\n" : "\n")
            + (this.prevPhaseAndSate.sectorFlag1 != ps.sectorFlag1 ? "***\n" : "\n")
            + (this.prevPhaseAndSate.sectorFlag2 != ps.sectorFlag2 ? "***\n" : "\n")
            + (this.prevPhaseAndSate.sectorFlag3 != ps.sectorFlag3 ? "***\n" : "\n")
            + (this.prevPhaseAndSate.control != ps.control ? "***\n" : "\n")
            + (this.prevPhaseAndSate.inPits != ps.inPits ? "***\n" : "\n")
            + (this.prevPhaseAndSate.isPlayer != ps.isPlayer ? "***\n" : "\n")
            + (this.prevPhaseAndSate.place != ps.place ? "***\n" : "\n")
            + (this.prevPhaseAndSate.pitState != ps.pitState ? "***\n" : "\n")
            + (this.prevPhaseAndSate.individualPhase != ps.individualPhase ? "***\n" : "\n")
            + (this.prevPhaseAndSate.flag != ps.flag ? "***\n" : "\n")
            + (this.prevPhaseAndSate.underYellow != ps.underYellow ? "***\n" : "\n")
            + (this.prevPhaseAndSate.countLapFlag != ps.countLapFlag ? "***\n" : "\n")
            + (this.prevPhaseAndSate.inGarageStall != ps.inGarageStall ? "***\n" : "\n"));

          // Save current phase and state.
          this.prevPhaseAndSate = ps;

          this.sbLabel = new StringBuilder();
          sbLabel.Append("mGamePhase:\n"
            + "mSession:\n"
            + "mYellowFlagState:\n"
            + "mSector:\n"
            + "mCurrentSector:\n"
            + "mInRealtime:\n"
            + "mSectorFlag[0]:\n"
            + "mSectorFlag[1]:\n"
            + "mSectorFlag[2]:\n"
            + "mControl:\n"
            + "mInPits:\n"
            + "mIsPlayer:\n"
            + "mPlace:\n"
            + "mPitState:\n"
            + "mIndividualPhase:\n"
            + "mFlag:\n"
            + "mUnderYellow:\n"
            + "mCountLapFlag:\n"
            + "mInGarageStall:\n");

          this.sbValues = new StringBuilder();
          sbValues.Append(
            string.Format("{0}\n{1}\n{2}\n{3}\n0x{4:X8}\n{5}\n{6}\n{7}\n{8}\n{9}\n{10}\n{11}\n{12}\n{13}\n{14}\n{15}\n{16}\n{17}\n{18}\n",
            GetEnumString<rF2GamePhase>(state.mGamePhase),
            ps.session,
            GetEnumString<rF2YellowFlagState>(state.mYellowFlagState), 
            ps.playerSector,
            ps.currentSectorExtended, // {4:X} hexadecimal to see values
            ps.inRealTime,// == 0 ? "false" : "true",
            GetEnumString<rF2YellowFlagState>(state.mSectorFlag[0]),
            GetEnumString<rF2YellowFlagState>(state.mSectorFlag[1]),
            GetEnumString<rF2YellowFlagState>(state.mSectorFlag[2]),
            GetEnumString<rF2Control>(playerVeh.mControl),
            ps.inPits,// == 0 ? "false" : "true",
            ps.isPlayer,// == 0 ? "false" : "true",
            ps.place,
            GetEnumString<rF2PitState>(playerVeh.mPitState),
            GetEnumString<rF2GamePhase>(playerVeh.mIndividualPhase),
            GetEnumString<rF2PrimaryFlag>(playerVeh.mFlag),
            ps.underYellow,
            GetEnumString<rF2CountLapFlag>(playerVeh.mCountLapFlag), 
            ps.inGarageStall));
        }

        g.DrawString(this.sbChanged.ToString(), SystemFonts.DefaultFont, Brushes.Orange, 3.0f, 53.0f);
        g.DrawString(this.sbLabel.ToString(), SystemFonts.DefaultFont, Brushes.Green, 30.0f, 50.0f);
        g.DrawString(this.sbValues.ToString(), SystemFonts.DefaultFont, Brushes.Green, 150.0f, 50.0f);
      }
      catch (InvalidCastException)
      {

      }
      
    }
  }
}
