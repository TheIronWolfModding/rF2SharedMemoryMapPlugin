using rF2SMMonitor.rFactor2Data;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using static rF2SMMonitor.rFactor2Constants;

namespace rF2SMMonitor
{
  internal class TransitionTracker
  {
    private static readonly string fileTimesTampString = DateTime.Now.ToString("yyyy-MM-dd_HH-mm-ss");
    private static readonly string basePath = Path.GetDirectoryName(System.Reflection.Assembly.GetEntryAssembly().Location) + "\\logs";
    private static readonly string phaseAndStateTrackingFilePath = $"{basePath}\\{fileTimesTampString}___PhaseAndStateTracking.log";
    private static readonly string damageTrackingFilePath = $"{basePath}\\{fileTimesTampString}___DamageTracking.log";
    private static readonly string phaseAndStateDeltaTrackingFilePath = $"{basePath}\\{fileTimesTampString}___PhaseAndStateTrackingDelta.log";
    private static readonly string damageTrackingDeltaFilePath = $"{basePath}\\{fileTimesTampString}___DamageTrackingDelta.log";

    internal TransitionTracker()
    {
      if (!Directory.Exists(basePath))
        Directory.CreateDirectory(basePath);
    }

    private string GetEnumString<T>(sbyte value)
    {
      var enumType = typeof(T);

      var enumValue = (T)Enum.ToObject(enumType, value);
      return Enum.IsDefined(enumType, enumValue) ? $"{enumValue.ToString()}({value})" : string.Format("Unknown({0})", value);
    }

    private string GetEnumString<T>(byte value)
    {
      var enumType = typeof(T);

      var enumValue = (T)Enum.ToObject(enumType, value);
      return Enum.IsDefined(enumType, enumValue) ? $"{enumValue.ToString()}({value})" : string.Format("Unknown({0})", value);
    }

    private string GetSessionString(int session)
    {
      // current session (0=testday 1-4=practice 5-8=qual 9=warmup 10-13=race)
      if (session == 0)
        return $"TestDay({session})";
      else if (session >= 1 && session <= 4)
        return $"Practice({session})";
      else if (session >= 5 && session <= 8)
        return $"Qualification({session})";
      else if (session == 9)
        return $"WarmUp({session})";
      else if (session >= 10 && session <= 13)
        return $"Race({session})";

      return $"Unknown({session})";
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
    internal StringBuilder sbPhaseChanged = new StringBuilder();
    internal StringBuilder sbPhaseLabel = new StringBuilder();
    internal StringBuilder sbPhaseValues = new StringBuilder();

    rF2GamePhase lastDamageTrackingGamePhase = (rF2GamePhase)Enum.ToObject(typeof(rF2GamePhase), -255);
    rF2GamePhase lastPhaseTrackingGamePhase = (rF2GamePhase)Enum.ToObject(typeof(rF2GamePhase), -255);

    internal void TrackPhase(ref rF2State state, Graphics g, bool logToFile)
    {
      if (logToFile)
      {
        if ((this.lastPhaseTrackingGamePhase == rF2GamePhase.Garage
              || this.lastPhaseTrackingGamePhase == rF2GamePhase.SessionOver
              || this.lastPhaseTrackingGamePhase == rF2GamePhase.SessionStopped
              || (int)this.lastPhaseTrackingGamePhase == 9)  // What is 9? 
            && ((rF2GamePhase)state.mGamePhase == rF2GamePhase.Countdown
              || (rF2GamePhase)state.mGamePhase == rF2GamePhase.Formation
              || (rF2GamePhase)state.mGamePhase == rF2GamePhase.GridWalk
              || (rF2GamePhase)state.mGamePhase == rF2GamePhase.GreenFlag))
        {
          var lines = new List<string>();
          lines.Add("\n");
          lines.Add("************************************************************************************");
          lines.Add("* NEW SESSION **********************************************************************");
          lines.Add("************************************************************************************");
          File.AppendAllLines(phaseAndStateTrackingFilePath, lines);
          File.AppendAllLines(phaseAndStateDeltaTrackingFilePath, lines);
        }
      }

      this.lastPhaseTrackingGamePhase = (rF2GamePhase)state.mGamePhase;


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
      ps.underYellow = playerVeh.mUnderYellow;
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
        this.sbPhaseChanged = new StringBuilder();
        sbPhaseChanged.Append((this.prevPhaseAndSate.gamePhase != ps.gamePhase ? "***\n" : "\n")
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

        this.sbPhaseLabel = new StringBuilder();
        sbPhaseLabel.Append("mGamePhase:\n"
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

        this.sbPhaseValues = new StringBuilder();
        sbPhaseValues.Append(
          string.Format("{0}\n{1}\n{2}\n{3}\n0x{4:X8}\n{5}\n{6}\n{7}\n{8}\n{9}\n{10}\n{11}\n{12}\n{13}\n{14}\n{15}\n{16}\n{17}\n{18}\n",
          GetEnumString<rF2GamePhase>(state.mGamePhase),
          GetSessionString(state.mSession),
          GetEnumString<rF2YellowFlagState>(state.mYellowFlagState),
          ps.playerSector,
          ps.currentSectorExtended, // {4:X} hexadecimal to see values
          ps.inRealTime == 0 ? $"false({ps.inRealTime})" : $"true({ps.inRealTime})",
          GetEnumString<rF2YellowFlagState>(state.mSectorFlag[0]),
          GetEnumString<rF2YellowFlagState>(state.mSectorFlag[1]),
          GetEnumString<rF2YellowFlagState>(state.mSectorFlag[2]),
          GetEnumString<rF2Control>(playerVeh.mControl),
          ps.inPits == 0 ? $"false({ps.inPits})" : $"true({ps.inPits})",
          ps.isPlayer == 0 ? $"false({ps.isPlayer})" : $"true({ps.isPlayer})",
          ps.place,
          GetEnumString<rF2PitState>(playerVeh.mPitState),
          GetEnumString<rF2GamePhase>(playerVeh.mIndividualPhase),
          GetEnumString<rF2PrimaryFlag>(playerVeh.mFlag),
          ps.underYellow,
          GetEnumString<rF2CountLapFlag>(playerVeh.mCountLapFlag),
          ps.inGarageStall == 0 ? $"false({ps.inGarageStall})" : $"true({ps.inGarageStall})"));

        if (logToFile)
        {
          var changed = this.sbPhaseChanged.ToString().Split('\n');
          var labels = this.sbPhaseLabel.ToString().Split('\n');
          var values = this.sbPhaseValues.ToString().Split('\n');
          Debug.Assert(changed.Length == labels.Length && values.Length == labels.Length);

          var lines = new List<string>();
          var updateTime = DateTime.Now.ToString();

          lines.Add($"\n{updateTime}");
          for (int i = 0; i < changed.Length; ++i)
            lines.Add($"{changed[i]}{labels[i]}{values[i]}");

          File.AppendAllLines(phaseAndStateTrackingFilePath, lines);

          lines.Clear();

          lines.Add($"\n{updateTime}");
          for (int i = 0; i < changed.Length; ++i)
          {
            if (changed[i].StartsWith("***"))
              lines.Add($"{changed[i]}{labels[i]}{values[i]}");
          }

          File.AppendAllLines(phaseAndStateDeltaTrackingFilePath, lines);
        }
      }

      if (g != null)
      {
        g.DrawString(this.sbPhaseChanged.ToString(), SystemFonts.DefaultFont, Brushes.Orange, 3.0f, 33.0f);
        g.DrawString(this.sbPhaseLabel.ToString(), SystemFonts.DefaultFont, Brushes.Green, 30.0f, 30.0f);
        g.DrawString(this.sbPhaseValues.ToString(), SystemFonts.DefaultFont, Brushes.Purple, 130.0f, 30.0f);
      }
    }


    internal class DamageInfo
    {
      internal byte[] mDentSeverity = new byte[8];         // dent severity at 8 locations around the car (0=none, 1=some, 2=more)
      internal double mLastImpactMagnitude = -1.0;   // magnitude of last impact
      internal double mAccumulatedImpactMagnitude = -1.0;   // magnitude of last impact
      internal double mMaxImpactMagnitude = -1.0;   // magnitude of last impact
      internal rF2Vec3 mLastImpactPos;        // location of last impact
      internal double mLastImpactET = -1.0;          // time of last impact
      internal byte mOverheating = 255;            // whether overheating icon is shown
      internal byte mDetached = 255;               // whether any parts (besides wheels) have been detached
      //internal byte mHeadlights = 255;             // whether headlights are on

      internal byte mFrontLeftFlat = 255;                    // whether tire is flat
      internal byte mFrontLeftDetached = 255;                // whether wheel is detached
      internal byte mFrontRightFlat = 255;                    // whether tire is flat
      internal byte mFrontRightDetached = 255;                // whether wheel is detached

      internal byte mRearLeftFlat = 255;                    // whether tire is flat
      internal byte mRearLeftDetached = 255;                // whether wheel is detached
      internal byte mRearRightFlat = 255;                    // whether tire is flat
      internal byte mRearRightDetached = 255;                // whether wheel is detached
    }

    internal DamageInfo prevDamageInfo = new DamageInfo();
    internal StringBuilder sbDamageChanged = new StringBuilder();
    internal StringBuilder sbDamageLabel = new StringBuilder();
    internal StringBuilder sbDamageValues = new StringBuilder();

    internal void TrackDamage(ref rF2State state, Graphics g, bool logToFile)
    {
      if (logToFile)
      {
        if ((this.lastDamageTrackingGamePhase == rF2GamePhase.Garage
              || this.lastDamageTrackingGamePhase == rF2GamePhase.SessionOver
              || this.lastDamageTrackingGamePhase == rF2GamePhase.SessionStopped
              || (int)this.lastDamageTrackingGamePhase == 9)  // What is 9? 
            && ((rF2GamePhase)state.mGamePhase == rF2GamePhase.Countdown
              || (rF2GamePhase)state.mGamePhase == rF2GamePhase.Formation
              || (rF2GamePhase)state.mGamePhase == rF2GamePhase.GridWalk
              || (rF2GamePhase)state.mGamePhase == rF2GamePhase.GreenFlag))
        {
          var lines = new List<string>();
          lines.Add("\n");
          lines.Add("************************************************************************************");
          lines.Add("* NEW SESSION **********************************************************************");
          lines.Add("************************************************************************************");
          File.AppendAllLines(damageTrackingFilePath, lines);
          File.AppendAllLines(damageTrackingDeltaFilePath, lines);
        }
      }

      this.lastDamageTrackingGamePhase = (rF2GamePhase)state.mGamePhase;

      if (state.mNumVehicles == 0)
        return;

      var playerVeh = state.mVehicles[0];
      Debug.Assert(state.mID == playerVeh.mID);

      var di = new DamageInfo();
      di.mDentSeverity = state.mDentSeverity;
      di.mLastImpactMagnitude = state.mLastImpactMagnitude;
      di.mAccumulatedImpactMagnitude = state.mAccumulatedImpactMagnitude;
      di.mMaxImpactMagnitude = state.mMaxImpactMagnitude;
      di.mLastImpactPos = state.mLastImpactPos;
      di.mLastImpactET = state.mLastImpactET;
      di.mOverheating = state.mOverheating;
      di.mDetached = state.mDetached;
      di.mFrontLeftFlat = state.mWheels[(int)rF2WheelIndex.FrontLeft].mFlat;
      di.mFrontLeftDetached = state.mWheels[(int)rF2WheelIndex.FrontLeft].mDetached;
      di.mFrontRightFlat = state.mWheels[(int)rF2WheelIndex.FrontRight].mFlat;
      di.mFrontRightDetached = state.mWheels[(int)rF2WheelIndex.FrontRight].mDetached;
      di.mRearLeftFlat = state.mWheels[(int)rF2WheelIndex.RearLeft].mFlat;
      di.mRearLeftDetached = state.mWheels[(int)rF2WheelIndex.RearLeft].mDetached;
      di.mRearRightFlat = state.mWheels[(int)rF2WheelIndex.RearRight].mFlat;
      di.mRearRightDetached = state.mWheels[(int)rF2WheelIndex.RearRight].mDetached;

      bool dentSevChanged = false;
      for (int i = 0; i < 8; ++i) {
        if (this.prevDamageInfo.mDentSeverity[i] != di.mDentSeverity[i]) {
          dentSevChanged = true;
          break;
        }
      }

      bool lastImpactPosChanged = di.mLastImpactPos.x != this.prevDamageInfo.mLastImpactPos.x
        || di.mLastImpactPos.y != this.prevDamageInfo.mLastImpactPos.y
        || di.mLastImpactPos.z != this.prevDamageInfo.mLastImpactPos.z;

      // Only refresh UI if there's change.
      if (dentSevChanged
        || di.mLastImpactMagnitude != this.prevDamageInfo.mLastImpactMagnitude
        || di.mAccumulatedImpactMagnitude != this.prevDamageInfo.mAccumulatedImpactMagnitude
        || di.mMaxImpactMagnitude != this.prevDamageInfo.mMaxImpactMagnitude
        || lastImpactPosChanged
        || di.mLastImpactET != this.prevDamageInfo.mLastImpactET
        || di.mOverheating != this.prevDamageInfo.mOverheating
        || di.mDetached != this.prevDamageInfo.mDetached
        || di.mFrontLeftFlat != this.prevDamageInfo.mFrontLeftFlat
        || di.mFrontRightFlat != this.prevDamageInfo.mFrontRightFlat
        || di.mRearLeftFlat != this.prevDamageInfo.mRearLeftFlat
        || di.mRearRightFlat != this.prevDamageInfo.mRearRightFlat
        || di.mFrontLeftDetached != this.prevDamageInfo.mFrontLeftDetached
        || di.mFrontRightDetached != this.prevDamageInfo.mFrontRightDetached
        || di.mRearLeftDetached != this.prevDamageInfo.mRearLeftDetached
        || di.mRearRightDetached != this.prevDamageInfo.mRearRightDetached)
      {
        this.sbDamageChanged = new StringBuilder();
        sbDamageChanged.Append((dentSevChanged ? "***\n" : "\n")
          + (di.mLastImpactMagnitude != this.prevDamageInfo.mLastImpactMagnitude ? "***\n" : "\n")
          + (di.mAccumulatedImpactMagnitude != this.prevDamageInfo.mAccumulatedImpactMagnitude ? "***\n" : "\n")
          + (di.mMaxImpactMagnitude != this.prevDamageInfo.mMaxImpactMagnitude ? "***\n" : "\n")
          + (lastImpactPosChanged ? "***\n" : "\n")
          + (di.mLastImpactET != this.prevDamageInfo.mLastImpactET ? "***\n" : "\n")
          + (di.mOverheating != this.prevDamageInfo.mOverheating ? "***\n" : "\n")
          + (di.mDetached != this.prevDamageInfo.mDetached ? "***\n" : "\n")
          + ((di.mFrontLeftFlat != this.prevDamageInfo.mFrontLeftFlat
              || di.mFrontRightFlat != this.prevDamageInfo.mFrontRightFlat
              || di.mFrontLeftDetached != this.prevDamageInfo.mFrontLeftDetached
              || di.mFrontRightDetached != this.prevDamageInfo.mFrontRightDetached) ? "***\n" : "\n")
          + ((di.mRearLeftFlat != this.prevDamageInfo.mRearLeftFlat
              || di.mRearRightFlat != this.prevDamageInfo.mRearRightFlat
              || di.mRearLeftDetached != this.prevDamageInfo.mRearLeftDetached
              || di.mRearRightDetached != this.prevDamageInfo.mRearRightDetached) ? "***\n" : "\n"));

        // Save current damage info.
        this.prevDamageInfo = di;

        this.sbDamageLabel = new StringBuilder();
        sbDamageLabel.Append(
          "mDentSeverity:\n"
          + "mLastImpactMagnitude:\n"
          + "mAccumulatedImpactMagnitude:\n"
          + "mMaxImpactMagnitude:\n"
          + "mLastImpactPos:\n"
          + "mLastImpactET:\n"
          + "mOverheating:\n"
          + "mDetached:\n"
          + "Front Wheels:\n"
          + "Rear Wheels:\n");

        this.sbDamageValues = new StringBuilder();
        sbDamageValues.Append(
          $"{di.mDentSeverity[0]},{di.mDentSeverity[1]},{di.mDentSeverity[2]},{di.mDentSeverity[3]},{di.mDentSeverity[4]},{di.mDentSeverity[5]},{di.mDentSeverity[6]},{di.mDentSeverity[7]}\n"
          + $"{di.mLastImpactMagnitude:N1}\n"
          + $"{di.mAccumulatedImpactMagnitude:N1}\n"
          + $"{di.mMaxImpactMagnitude:N1}\n"
          + $"x={di.mLastImpactPos.x:N4} y={di.mLastImpactPos.y:N4} z={di.mLastImpactPos.z:N4}\n"
          + $"{di.mLastImpactET}\n"
          + $"{di.mOverheating}\n"
          + $"{di.mDetached}\n"
          + $"Left Flat:{di.mFrontLeftFlat}    Left Detached:{di.mFrontLeftDetached}        Right Flat:{di.mFrontRightFlat}    Right Detached:{di.mFrontRightDetached}\n"
          + $"Left Flat:{di.mRearLeftFlat}    Left Detached:{di.mRearLeftDetached}        Right Flat:{di.mRearRightFlat}    Right Detached:{di.mRearRightDetached}\n"
          );

        if (logToFile)
        {
          var changed = this.sbDamageChanged.ToString().Split('\n');
          var labels = this.sbDamageLabel.ToString().Split('\n');
          var values = this.sbDamageValues.ToString().Split('\n');
          Debug.Assert(changed.Length == labels.Length && values.Length == labels.Length);

          var lines = new List<string>();

          var updateTime = DateTime.Now.ToString();
          lines.Add($"\n{updateTime}");
          for (int i = 0; i < changed.Length; ++i)
            lines.Add($"{changed[i]}{labels[i]}{values[i]}");

          File.AppendAllLines(damageTrackingFilePath, lines);

          lines.Clear();
          lines.Add($"\n{updateTime}");
          for (int i = 0; i < changed.Length; ++i)
          {
            if (changed[i].StartsWith("***"))
              lines.Add($"{changed[i]}{labels[i]}{values[i]}");
          }

          File.AppendAllLines(damageTrackingDeltaFilePath, lines);
        }
      }

      if (g != null)
      {
        g.DrawString(this.sbDamageChanged.ToString(), SystemFonts.DefaultFont, Brushes.Orange, 3.0f, 303.0f);
        g.DrawString(this.sbDamageLabel.ToString(), SystemFonts.DefaultFont, Brushes.Green, 30.0f, 300.0f);
        g.DrawString(this.sbDamageValues.ToString(), SystemFonts.DefaultFont, Brushes.Purple, 200.0f, 300.0f);
      }
    }
  }
}
