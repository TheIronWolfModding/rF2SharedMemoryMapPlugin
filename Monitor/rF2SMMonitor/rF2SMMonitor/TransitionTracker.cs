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
      internal rF2GamePhase gamePhase;
      internal rF2YellowFlagState yellowFlagState;
      internal int playerSector;
      internal int currentSectorExtended;
      internal byte inRealTime;
      internal rF2YellowFlagState sectorFlag1;
      internal rF2YellowFlagState sectorFlag2;
      internal rF2YellowFlagState sectorFlag3;
      internal rF2Control control;
      internal byte inPits;
      internal byte isPlayer;
      internal int place;
      // TODO: add enum for pit state
      internal byte pitState;
      internal rF2GamePhase individualPhase;
      internal byte flag;
      internal byte underYellow;
      internal byte countLapFlag;
      internal byte inGarageStall;
    }

    internal void TrackPhase(ref rF2State state, Graphics g)
    {
      try
      {
        if (state.mNumVehicles == 0)
          return;

        var playerVeh = state.mVehicles[0];
        Debug.Assert(state.mID == playerVeh.mID);

        var ps = new PhaseAndState();

        // When tracking phases, mark changed items with ***  ***
        // Log multiline output
        ps.gamePhase = (rF2GamePhase)state.mGamePhase;
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
        ps.pitState = playerVeh.mPitState;
        ps.individualPhase = (rF2GamePhase)playerVeh.mIndividualPhase;
        ps.flag = playerVeh.mFlag;
        ps.underYellow =  playerVeh.mUnderYellow;
        ps.countLapFlag = playerVeh.mCountLapFlag;
        ps.inGarageStall = playerVeh.mInGarageStall;

        var sbLabel = new StringBuilder();
        sbLabel.Append("mGamePhase:\n"
          + "mYellowFlagState:\n"
          + "mSector:\n");

        var sbValues = new StringBuilder();
        sbValues.Append(
          string.Format("{0}\n{1}\n{2}\n", GetEnumString<rF2GamePhase>(state.mGamePhase), GetEnumString<rF2GamePhase>(state.mYellowFlagState), ps.playerSector)
          );

        g.DrawString(sbLabel.ToString(), SystemFonts.DefaultFont, Brushes.Green, 3.0f, 50.0f);
        g.DrawString(sbValues.ToString(), SystemFonts.DefaultFont, Brushes.Green, 120.0f, 50.0f);

        //var surface = (rF2GamePhase)state.mSu;

      }
      catch (InvalidCastException)
      {

      }
      
    }
  }
}
