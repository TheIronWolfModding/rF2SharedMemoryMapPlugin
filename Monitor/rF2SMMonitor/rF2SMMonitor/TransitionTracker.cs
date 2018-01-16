/*
TransitionTracker class various state transitions in rF2 state and optionally logs transitions to files.

Author: The Iron Wolf (vleonavicius@hotmail.com)
Website: thecrewchief.org
*/
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
    private static readonly string rulesTrackingFilePath = $"{basePath}\\{fileTimesTampString}___RulesTracking.log";
    private static readonly string phaseAndStateDeltaTrackingFilePath = $"{basePath}\\{fileTimesTampString}___PhaseAndStateTrackingDelta.log";
    private static readonly string damageTrackingDeltaFilePath = $"{basePath}\\{fileTimesTampString}___DamageTrackingDelta.log";
    private static readonly string rulesTrackingDeltaFilePath = $"{basePath}\\{fileTimesTampString}___RulesTrackingDelta.log";
    private static readonly string timingTrackingFilePath = $"{basePath}\\{fileTimesTampString}___TimingTracking.log";

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

    public static string GetSessionString(int session)
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


    // TODO: Telemetry section
    // Telemetry values (separate section)

    internal class PhaseAndState
    {
      internal rF2GamePhase mGamePhase = (rF2GamePhase)Enum.ToObject(typeof(rF2GamePhase), -255);
      internal int mSession = -255;
      internal rF2YellowFlagState mYellowFlagState = (rF2YellowFlagState)Enum.ToObject(typeof(rF2YellowFlagState), -255);
      internal int mSector = -255;
      internal int mCurrentSector = -255;
      internal byte mInRealTimeFC = 255;
      internal byte mInRealTime = 255;
      internal rF2YellowFlagState mSector1Flag = (rF2YellowFlagState)Enum.ToObject(typeof(rF2YellowFlagState), -255);
      internal rF2YellowFlagState mSector2Flag = (rF2YellowFlagState)Enum.ToObject(typeof(rF2YellowFlagState), -255);
      internal rF2YellowFlagState mSector3Flag = (rF2YellowFlagState)Enum.ToObject(typeof(rF2YellowFlagState), -255);
      internal rF2Control mControl;
      internal byte mInPits = 255;
      internal byte mIsPlayer = 255;
      internal int mPlace = -255;
      internal rF2PitState mPitState = (rF2PitState)Enum.ToObject(typeof(rF2PitState), -255);
      internal rF2GamePhase mIndividualPhase = (rF2GamePhase)Enum.ToObject(typeof(rF2GamePhase), -255);
      internal rF2PrimaryFlag mFlag = (rF2PrimaryFlag)Enum.ToObject(typeof(rF2PrimaryFlag), -255);
      internal byte mUnderYellow = 255;
      internal rF2CountLapFlag mCountLapFlag = (rF2CountLapFlag)Enum.ToObject(typeof(rF2CountLapFlag), -255);
      internal byte mInGarageStall = 255;
      internal rF2FinishStatus mFinishStatus = (rF2FinishStatus)Enum.ToObject(typeof(rF2FinishStatus), -255);
      internal int mLapNumber = -255;
      internal short mTotalLaps = -255;
      internal int mMaxLaps = -1;
      internal int mNumVehicles = -1;
      internal byte mScheduledStops = 255;
      internal byte mHeadlights = 255;
      internal byte mSpeedLimiter = 255;
      internal byte mFrontTireCompoundIndex = 255;
      internal byte mRearTireCompoundIndex = 255;
      internal string mFrontTireCompoundName = "Unknown";
      internal string mRearTireCompoundName = "Unknown";
      internal byte mFrontFlapActivated = 255;
      internal byte mRearFlapActivated = 255;
      internal rF2RearFlapLegalStatus mRearFlapLegalStatus = (rF2RearFlapLegalStatus)Enum.ToObject(typeof(rF2RearFlapLegalStatus), -255);
      internal rF2IgnitionStarterStatus mIgnitionStarter = (rF2IgnitionStarterStatus)Enum.ToObject(typeof(rF2IgnitionStarterStatus), -255);
      internal byte mSpeedLimiterAvailable = 255;
      internal byte mAntiStallActivated = 255;
      internal byte mStartLight = 255;
      internal byte mNumRedLights = 255;
      internal short mNumPitstops = -255;
      internal short mNumPenalties = -255;
      internal int mLapsBehindNext = -1;
      internal int mLapsBehindLeader = -1;
      internal byte mPlayerHeadlights = 255;
      internal byte mServerScored = 255;
      internal int mQualification = -1;
    }

    internal PhaseAndState prevPhaseAndSate = new PhaseAndState();
    internal StringBuilder sbPhaseChanged = new StringBuilder();
    internal StringBuilder sbPhaseLabel = new StringBuilder();
    internal StringBuilder sbPhaseValues = new StringBuilder();
    internal StringBuilder sbPhaseChangedCol2 = new StringBuilder();
    internal StringBuilder sbPhaseLabelCol2 = new StringBuilder();
    internal StringBuilder sbPhaseValuesCol2 = new StringBuilder();

    rF2GamePhase lastDamageTrackingGamePhase = (rF2GamePhase)Enum.ToObject(typeof(rF2GamePhase), -255);
    rF2GamePhase lastPhaseTrackingGamePhase = (rF2GamePhase)Enum.ToObject(typeof(rF2GamePhase), -255);
    rF2GamePhase lastTimingTrackingGamePhase = (rF2GamePhase)Enum.ToObject(typeof(rF2GamePhase), -255);
    rF2GamePhase lastRulesTrackingGamePhase = (rF2GamePhase)Enum.ToObject(typeof(rF2GamePhase), -255);

    private float screenYStart = 253.0f;

    internal void TrackPhase(ref rF2Scoring scoring, ref rF2Telemetry telemetry, ref rF2Extended extended, Graphics g, bool logToFile)
    {
      if (logToFile)
      {
        if ((this.lastPhaseTrackingGamePhase == rF2GamePhase.Garage
              || this.lastPhaseTrackingGamePhase == rF2GamePhase.SessionOver
              || this.lastPhaseTrackingGamePhase == rF2GamePhase.SessionStopped
              || (int)this.lastPhaseTrackingGamePhase == 9)  // What is 9? 
            && ((rF2GamePhase)scoring.mScoringInfo.mGamePhase == rF2GamePhase.Countdown
              || (rF2GamePhase)scoring.mScoringInfo.mGamePhase == rF2GamePhase.Formation
              || (rF2GamePhase)scoring.mScoringInfo.mGamePhase == rF2GamePhase.GridWalk
              || (rF2GamePhase)scoring.mScoringInfo.mGamePhase == rF2GamePhase.GreenFlag))
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

      this.lastPhaseTrackingGamePhase = (rF2GamePhase)scoring.mScoringInfo.mGamePhase;

      if (scoring.mScoringInfo.mNumVehicles == 0)
        return;

      // Build map of mID -> telemetry.mVehicles[i]. 
      // They are typically matching values, however, we need to handle online cases and dropped vehicles (mID can be reused).
      var idsToTelIndices = new Dictionary<long, int>();
      for (int i = 0; i < telemetry.mNumVehicles; ++i)
      {
        if (!idsToTelIndices.ContainsKey(telemetry.mVehicles[i].mID))
          idsToTelIndices.Add(telemetry.mVehicles[i].mID, i);
      }

      var playerVeh = MainForm.GetPlayerScoring(ref scoring);

      if (playerVeh.mIsPlayer != 1)
        return;

      var scoringPlrId = playerVeh.mID;
      if (!idsToTelIndices.ContainsKey(scoringPlrId))
        return;

      var resolvedIdx = idsToTelIndices[scoringPlrId];
      var playerVehTelemetry = telemetry.mVehicles[resolvedIdx];

      var ps = new PhaseAndState();

      ps.mGamePhase = (rF2GamePhase)scoring.mScoringInfo.mGamePhase;
      ps.mSession = scoring.mScoringInfo.mSession;
      ps.mYellowFlagState = (rF2YellowFlagState)scoring.mScoringInfo.mYellowFlagState;
      ps.mSector = playerVeh.mSector == 0 ? 3 : playerVeh.mSector;
      ps.mCurrentSector = playerVehTelemetry.mCurrentSector;
      ps.mInRealTime = scoring.mScoringInfo.mInRealtime;
      ps.mInRealTimeFC = extended.mInRealtimeFC;
      ps.mSector1Flag = (rF2YellowFlagState)scoring.mScoringInfo.mSectorFlag[0];
      ps.mSector2Flag = (rF2YellowFlagState)scoring.mScoringInfo.mSectorFlag[1];
      ps.mSector3Flag = (rF2YellowFlagState)scoring.mScoringInfo.mSectorFlag[2];
      ps.mControl = (rF2Control)playerVeh.mControl;
      ps.mInPits = playerVeh.mInPits;
      ps.mIsPlayer = playerVeh.mIsPlayer;
      ps.mPlace = playerVeh.mPlace;
      ps.mPitState = (rF2PitState)playerVeh.mPitState;
      ps.mIndividualPhase = (rF2GamePhase)playerVeh.mIndividualPhase;
      ps.mFlag = (rF2PrimaryFlag)playerVeh.mFlag;
      ps.mUnderYellow = playerVeh.mUnderYellow;
      ps.mCountLapFlag = (rF2CountLapFlag)playerVeh.mCountLapFlag;
      ps.mInGarageStall = playerVeh.mInGarageStall;
      ps.mFinishStatus = (rF2FinishStatus)playerVeh.mFinishStatus;
      ps.mLapNumber = playerVehTelemetry.mLapNumber;
      ps.mTotalLaps = playerVeh.mTotalLaps;
      ps.mMaxLaps = scoring.mScoringInfo.mMaxLaps;
      ps.mNumVehicles = scoring.mScoringInfo.mNumVehicles;
      ps.mScheduledStops = playerVehTelemetry.mScheduledStops;
      ps.mHeadlights = playerVeh.mHeadlights;
      ps.mSpeedLimiter = playerVehTelemetry.mSpeedLimiter;
      ps.mFrontTireCompoundIndex = playerVehTelemetry.mFrontTireCompoundIndex;
      ps.mRearTireCompoundIndex = playerVehTelemetry.mRearTireCompoundIndex;
      ps.mFrontTireCompoundName = TransitionTracker.GetStringFromBytes(playerVehTelemetry.mFrontTireCompoundName);
      ps.mRearTireCompoundName = TransitionTracker.GetStringFromBytes(playerVehTelemetry.mRearTireCompoundName);
      ps.mFrontFlapActivated = playerVehTelemetry.mFrontFlapActivated;
      ps.mRearFlapActivated = playerVehTelemetry.mRearFlapActivated;
      ps.mRearFlapLegalStatus = (rF2RearFlapLegalStatus)playerVehTelemetry.mRearFlapLegalStatus;
      ps.mIgnitionStarter = (rF2IgnitionStarterStatus)playerVehTelemetry.mIgnitionStarter;
      ps.mSpeedLimiterAvailable = playerVehTelemetry.mSpeedLimiterAvailable;
      ps.mAntiStallActivated = playerVehTelemetry.mAntiStallActivated;
      ps.mStartLight = scoring.mScoringInfo.mStartLight;
      ps.mNumRedLights = scoring.mScoringInfo.mNumRedLights;
      ps.mNumPitstops = playerVeh.mNumPitstops;
      ps.mNumPenalties = playerVeh.mNumPenalties;
      ps.mLapsBehindNext = playerVeh.mLapsBehindNext;
      ps.mLapsBehindLeader = playerVeh.mLapsBehindLeader;
      ps.mPlayerHeadlights = playerVeh.mHeadlights;
      ps.mServerScored = playerVeh.mServerScored;
      ps.mQualification = playerVeh.mQualification;

      // Only refresh UI if there's change.
      if (this.prevPhaseAndSate.mGamePhase != ps.mGamePhase
        || this.prevPhaseAndSate.mSession != ps.mSession
        || this.prevPhaseAndSate.mYellowFlagState != ps.mYellowFlagState
        || this.prevPhaseAndSate.mSector != ps.mSector
        || this.prevPhaseAndSate.mCurrentSector != ps.mCurrentSector
        || this.prevPhaseAndSate.mInRealTimeFC != ps.mInRealTimeFC
        || this.prevPhaseAndSate.mInRealTime != ps.mInRealTime
        || this.prevPhaseAndSate.mSector1Flag != ps.mSector1Flag
        || this.prevPhaseAndSate.mSector2Flag != ps.mSector2Flag
        || this.prevPhaseAndSate.mSector3Flag != ps.mSector3Flag
        || this.prevPhaseAndSate.mControl != ps.mControl
        || this.prevPhaseAndSate.mInPits != ps.mInPits
        || this.prevPhaseAndSate.mIsPlayer != ps.mIsPlayer
        || this.prevPhaseAndSate.mPlace != ps.mPlace
        || this.prevPhaseAndSate.mPitState != ps.mPitState
        || this.prevPhaseAndSate.mIndividualPhase != ps.mIndividualPhase
        || this.prevPhaseAndSate.mFlag != ps.mFlag
        || this.prevPhaseAndSate.mUnderYellow != ps.mUnderYellow
        || this.prevPhaseAndSate.mCountLapFlag != ps.mCountLapFlag
        || this.prevPhaseAndSate.mInGarageStall != ps.mInGarageStall
        || this.prevPhaseAndSate.mFinishStatus != ps.mFinishStatus
        || this.prevPhaseAndSate.mLapNumber != ps.mLapNumber
        || this.prevPhaseAndSate.mTotalLaps != playerVeh.mTotalLaps
        || this.prevPhaseAndSate.mMaxLaps != ps.mMaxLaps
        || this.prevPhaseAndSate.mNumVehicles != ps.mNumVehicles
        || this.prevPhaseAndSate.mScheduledStops != ps.mScheduledStops
        || this.prevPhaseAndSate.mHeadlights != ps.mHeadlights
        || this.prevPhaseAndSate.mSpeedLimiter != ps.mSpeedLimiter
        || this.prevPhaseAndSate.mFrontTireCompoundIndex != ps.mFrontTireCompoundIndex
        || this.prevPhaseAndSate.mRearTireCompoundIndex != ps.mRearTireCompoundIndex
        || this.prevPhaseAndSate.mFrontTireCompoundName != ps.mFrontTireCompoundName
        || this.prevPhaseAndSate.mRearTireCompoundName != ps.mRearTireCompoundName
        || this.prevPhaseAndSate.mFrontFlapActivated != ps.mFrontFlapActivated
        || this.prevPhaseAndSate.mRearFlapActivated != ps.mRearFlapActivated
        || this.prevPhaseAndSate.mRearFlapLegalStatus != ps.mRearFlapLegalStatus
        || this.prevPhaseAndSate.mIgnitionStarter != ps.mIgnitionStarter
        || this.prevPhaseAndSate.mSpeedLimiterAvailable != ps.mSpeedLimiterAvailable
        || this.prevPhaseAndSate.mAntiStallActivated != ps.mAntiStallActivated
        || this.prevPhaseAndSate.mStartLight != ps.mStartLight
        || this.prevPhaseAndSate.mNumRedLights != ps.mNumRedLights
        || this.prevPhaseAndSate.mNumPitstops != ps.mNumPitstops
        || this.prevPhaseAndSate.mNumPenalties != ps.mNumPenalties
        || this.prevPhaseAndSate.mLapsBehindNext != ps.mLapsBehindNext
        || this.prevPhaseAndSate.mLapsBehindLeader != ps.mLapsBehindLeader
        || this.prevPhaseAndSate.mPlayerHeadlights != ps.mHeadlights
        || this.prevPhaseAndSate.mServerScored != ps.mServerScored
        || this.prevPhaseAndSate.mQualification != ps.mQualification)
      {
        this.sbPhaseChanged = new StringBuilder();
        sbPhaseChanged.Append((this.prevPhaseAndSate.mGamePhase != ps.mGamePhase ? "***\n" : "\n")
          + (this.prevPhaseAndSate.mSession != ps.mSession ? "***\n" : "\n")
          + (this.prevPhaseAndSate.mYellowFlagState != ps.mYellowFlagState ? "***\n" : "\n")
          + (this.prevPhaseAndSate.mSector != ps.mSector ? "***\n" : "\n")
          + (this.prevPhaseAndSate.mCurrentSector != ps.mCurrentSector ? "***\n" : "\n")
          + (this.prevPhaseAndSate.mInRealTimeFC != ps.mInRealTimeFC ? "***\n" : "\n")
          + (this.prevPhaseAndSate.mInRealTime != ps.mInRealTime ? "***\n" : "\n")
          + (this.prevPhaseAndSate.mSector1Flag != ps.mSector1Flag ? "***\n" : "\n")
          + (this.prevPhaseAndSate.mSector2Flag != ps.mSector2Flag ? "***\n" : "\n")
          + (this.prevPhaseAndSate.mSector3Flag != ps.mSector3Flag ? "***\n" : "\n")
          + (this.prevPhaseAndSate.mControl != ps.mControl ? "***\n" : "\n")
          + (this.prevPhaseAndSate.mInPits != ps.mInPits ? "***\n" : "\n")
          + (this.prevPhaseAndSate.mIsPlayer != ps.mIsPlayer ? "***\n" : "\n")
          + (this.prevPhaseAndSate.mPlace != ps.mPlace ? "***\n" : "\n")
          + (this.prevPhaseAndSate.mPitState != ps.mPitState ? "***\n" : "\n")
          + (this.prevPhaseAndSate.mIndividualPhase != ps.mIndividualPhase ? "***\n" : "\n")
          + (this.prevPhaseAndSate.mFlag != ps.mFlag ? "***\n" : "\n")
          + (this.prevPhaseAndSate.mUnderYellow != ps.mUnderYellow ? "***\n" : "\n")
          + (this.prevPhaseAndSate.mCountLapFlag != ps.mCountLapFlag ? "***\n" : "\n")
          + (this.prevPhaseAndSate.mInGarageStall != ps.mInGarageStall ? "***\n" : "\n")
          + (this.prevPhaseAndSate.mFinishStatus != ps.mFinishStatus ? "***\n" : "\n")
          + (this.prevPhaseAndSate.mLapNumber != ps.mLapNumber ? "***\n" : "\n")
          + (this.prevPhaseAndSate.mTotalLaps != ps.mTotalLaps ? "***\n" : "\n")
          + (this.prevPhaseAndSate.mMaxLaps != ps.mMaxLaps ? "***\n" : "\n"));

        this.sbPhaseChangedCol2 = new StringBuilder();
        sbPhaseChangedCol2.Append((this.prevPhaseAndSate.mNumVehicles != ps.mNumVehicles ? "***\n" : "\n")
          + (this.prevPhaseAndSate.mScheduledStops != ps.mScheduledStops ? "***\n" : "\n")
          + (this.prevPhaseAndSate.mHeadlights != ps.mHeadlights ? "***\n" : "\n")
          + (this.prevPhaseAndSate.mSpeedLimiter != ps.mSpeedLimiter ? "***\n" : "\n")
          + (this.prevPhaseAndSate.mFrontTireCompoundIndex != ps.mFrontTireCompoundIndex ? "***\n" : "\n")
          + (this.prevPhaseAndSate.mRearTireCompoundIndex != ps.mRearTireCompoundIndex ? "***\n" : "\n")
          + (this.prevPhaseAndSate.mFrontTireCompoundName != ps.mFrontTireCompoundName ? "***\n" : "\n")
          + (this.prevPhaseAndSate.mRearTireCompoundName != ps.mRearTireCompoundName ? "***\n" : "\n")
          + (this.prevPhaseAndSate.mFrontFlapActivated != ps.mFrontFlapActivated ? "***\n" : "\n")
          + (this.prevPhaseAndSate.mRearFlapActivated != ps.mRearFlapActivated ? "***\n" : "\n")
          + (this.prevPhaseAndSate.mRearFlapLegalStatus != ps.mRearFlapLegalStatus ? "***\n" : "\n")
          + (this.prevPhaseAndSate.mIgnitionStarter != ps.mIgnitionStarter ? "***\n" : "\n")
          + (this.prevPhaseAndSate.mSpeedLimiterAvailable != ps.mSpeedLimiterAvailable ? "***\n" : "\n")
          + (this.prevPhaseAndSate.mAntiStallActivated != ps.mAntiStallActivated ? "***\n" : "\n")
          + (this.prevPhaseAndSate.mStartLight != ps.mStartLight ? "***\n" : "\n")
          + (this.prevPhaseAndSate.mNumRedLights != ps.mNumRedLights ? "***\n" : "\n")
          + (this.prevPhaseAndSate.mNumPitstops != ps.mNumPitstops ? "***\n" : "\n")
          + (this.prevPhaseAndSate.mNumPenalties != ps.mNumPenalties ? "***\n" : "\n")
          + (this.prevPhaseAndSate.mLapsBehindNext != ps.mLapsBehindNext ? "***\n" : "\n")
          + (this.prevPhaseAndSate.mLapsBehindLeader != ps.mLapsBehindLeader ? "***\n" : "\n")
          + (this.prevPhaseAndSate.mPlayerHeadlights != ps.mPlayerHeadlights ? "***\n" : "\n")
          + (this.prevPhaseAndSate.mServerScored != ps.mServerScored ? "***\n" : "\n")
          + (this.prevPhaseAndSate.mQualification != ps.mQualification ? "***\n" : "\n"));

        // Save current phase and state.
        this.prevPhaseAndSate = ps;

        this.sbPhaseLabel = new StringBuilder();
        sbPhaseLabel.Append("mGamePhase:\n"
          + "mSession:\n"
          + "mYellowFlagState:\n"
          + "mSector:\n"
          + "mCurrentSector:\n"
          + "mInRealtimeFC:\n"
          + "mInRealtimeSU:\n"
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
          + "mInGarageStall:\n"
          + "mFinishStatus:\n"
          + "mLapNumber:\n"
          + "mTotalLaps:\n"
          + "mMaxLaps:\n");

        this.sbPhaseLabelCol2 = new StringBuilder();
        sbPhaseLabelCol2.Append("mNumVehicles:\n"
          + "mScheduledStops:\n"
          + "mHeadlights:\n"
          + "mSpeedLimiter:\n"
          + "mFrontTireCompoundIndex:\n"
          + "mRearTireCompoundIndex:\n"
          + "mFrontTireCompoundName:\n"
          + "mRearTireCompoundName:\n"
          + "mFrontFlapActivated:\n"
          + "mRearFlapActivated:\n"
          + "mRearFlapLegalStatus:\n"
          + "mIgnitionStarter:\n"
          + "mSpeedLimiterAvailable:\n"
          + "mAntiStallActivated:\n"
          + "mStartLight:\n"
          + "mNumRedLights:\n"
          + "mNumPitstops:\n"
          + "mNumPenalties:\n"
          + "mLapsBehindNext:\n"
          + "mLapsBehindLeader:\n"
          + "mPlayerHeadlights:\n"
          + "mServerScored:\n"
          + "mQualification:\n"
        );

        this.sbPhaseValues = new StringBuilder();
        sbPhaseValues.Append(
          $"{GetEnumString<rF2GamePhase>(scoring.mScoringInfo.mGamePhase)}\n"
          + $"{GetSessionString(scoring.mScoringInfo.mSession)}\n"
          + $"{GetEnumString<rF2YellowFlagState>(scoring.mScoringInfo.mYellowFlagState)}\n"
          + $"{ps.mSector}\n"
          + $"0x{ps.mCurrentSector,4:X8}\n" // {4:X} hexadecimal to see values
          + (ps.mInRealTimeFC == 0 ? $"false({ps.mInRealTimeFC})" : $"true({ps.mInRealTimeFC})") + "\n"
          + (ps.mInRealTime == 0 ? $"false({ps.mInRealTime})" : $"true({ps.mInRealTime})") + "\n"
          + $"{GetEnumString<rF2YellowFlagState>(scoring.mScoringInfo.mSectorFlag[0])}\n"
          + $"{GetEnumString<rF2YellowFlagState>(scoring.mScoringInfo.mSectorFlag[1])}\n"
          + $"{GetEnumString<rF2YellowFlagState>(scoring.mScoringInfo.mSectorFlag[2])}\n"
          + $"{GetEnumString<rF2Control>(playerVeh.mControl)}\n"
          + (ps.mInPits == 0 ? $"false({ps.mInPits})" : $"true({ps.mInPits})") + "\n"
          + (ps.mIsPlayer == 0 ? $"false({ps.mIsPlayer})" : $"true({ps.mIsPlayer})") + "\n"
          + $"{ps.mPlace}\n"
          + $"{GetEnumString<rF2PitState>(playerVeh.mPitState)}\n"
          + $"{GetEnumString<rF2GamePhase>(playerVeh.mIndividualPhase)}\n"
          + $"{GetEnumString<rF2PrimaryFlag>(playerVeh.mFlag)}\n"
          + $"{ps.mUnderYellow}\n"
          + $"{GetEnumString<rF2CountLapFlag>(playerVeh.mCountLapFlag)}\n"
          + (ps.mInGarageStall == 0 ? $"false({ps.mInGarageStall})" : $"true({ps.mInGarageStall})") + "\n"
          + $"{GetEnumString<rF2FinishStatus>(playerVeh.mFinishStatus)}\n"
          + $"{ps.mLapNumber}\n"
          + $"{ps.mTotalLaps}\n"
          + $"{ps.mMaxLaps}\n");

        this.sbPhaseValuesCol2 = new StringBuilder();
        sbPhaseValuesCol2.Append($"{ps.mNumVehicles}\n"
          + (ps.mScheduledStops == 0 ? $"false({ps.mScheduledStops})" : $"true({ps.mScheduledStops})") + "\n"
          + (ps.mHeadlights == 0 ? $"false({ps.mHeadlights})" : $"true({ps.mHeadlights})") + "\n"
          + (ps.mSpeedLimiter == 0 ? $"false({ps.mSpeedLimiter})" : $"true({ps.mSpeedLimiter})") + "\n"
          + (ps.mFrontTireCompoundIndex == 0 ? $"false({ps.mFrontTireCompoundIndex})" : $"true({ps.mFrontTireCompoundIndex})") + "\n"
          + (ps.mRearTireCompoundIndex == 0 ? $"false({ps.mRearTireCompoundIndex})" : $"true({ps.mRearTireCompoundIndex})") + "\n"
          + $"{ps.mFrontTireCompoundName}\n"
          + $"{ps.mRearTireCompoundName}\n"
          + (ps.mFrontFlapActivated == 0 ? $"false({ps.mFrontFlapActivated})" : $"true({ps.mFrontFlapActivated})") + "\n"
          + (ps.mRearFlapActivated == 0 ? $"false({ps.mRearFlapActivated})" : $"true({ps.mRearFlapActivated})") + "\n"
          + $"{GetEnumString<rF2RearFlapLegalStatus>(playerVehTelemetry.mRearFlapLegalStatus)}\n"
          + $"{GetEnumString<rF2IgnitionStarterStatus>(playerVehTelemetry.mIgnitionStarter)}\n"
          + (ps.mSpeedLimiterAvailable == 0 ? $"false({ps.mSpeedLimiterAvailable})" : $"true({ps.mSpeedLimiterAvailable})") + "\n"
          + (ps.mAntiStallActivated == 0 ? $"false({ps.mAntiStallActivated})" : $"true({ps.mAntiStallActivated})") + "\n"
          + $"{ps.mStartLight}\n"
          + $"{ps.mNumRedLights}\n"
          + $"{ps.mNumPitstops}\n"
          + $"{ps.mNumPenalties}\n"
          + $"{ps.mLapsBehindNext}\n"
          + $"{ps.mLapsBehindLeader}\n"
          + (ps.mPlayerHeadlights == 0 ? $"false({ps.mPlayerHeadlights})" : $"true({ps.mPlayerHeadlights})") + "\n"
          + (ps.mServerScored == 0 ? $"false({ps.mServerScored})" : $"true({ps.mServerScored})") + "\n"
          + $"{ps.mQualification}\n");

        if (logToFile)
        {
          var changed = this.sbPhaseChanged.ToString().Split('\n');
          var labels = this.sbPhaseLabel.ToString().Split('\n');
          var values = this.sbPhaseValues.ToString().Split('\n');

          var changedCol2 = this.sbPhaseChangedCol2.ToString().Split('\n');
          var labelsCol2 = this.sbPhaseLabelCol2.ToString().Split('\n');
          var valuesCol2 = this.sbPhaseValuesCol2.ToString().Split('\n');

          var list = new List<string>(changed);
          list.AddRange(changedCol2);
          changed = list.ToArray();

          list = new List<string>(labels);
          list.AddRange(labelsCol2);
          labels = list.ToArray();

          list = new List<string>(values);
          list.AddRange(valuesCol2);
          values = list.ToArray();

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
        g.DrawString(this.sbPhaseChanged.ToString(), SystemFonts.DefaultFont, Brushes.Orange, 3.0f, this.screenYStart + 3.0f);
        g.DrawString(this.sbPhaseLabel.ToString(), SystemFonts.DefaultFont, Brushes.Green, 30.0f, this.screenYStart);
        g.DrawString(this.sbPhaseValues.ToString(), SystemFonts.DefaultFont, Brushes.Purple, 130.0f, this.screenYStart);

        g.DrawString(this.sbPhaseChangedCol2.ToString(), SystemFonts.DefaultFont, Brushes.Orange, 253.0f, this.screenYStart + 3.0f);
        g.DrawString(this.sbPhaseLabelCol2.ToString(), SystemFonts.DefaultFont, Brushes.Green, 280.0f, this.screenYStart);
        g.DrawString(this.sbPhaseValuesCol2.ToString(), SystemFonts.DefaultFont, Brushes.Purple, 430.0f, this.screenYStart);
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

    internal void TrackDamage(ref rF2Scoring scoring, ref rF2Telemetry telemetry, ref rF2Extended extended, Graphics g, bool logToFile)
    {
      if (logToFile)
      {
        if ((this.lastDamageTrackingGamePhase == rF2GamePhase.Garage
              || this.lastDamageTrackingGamePhase == rF2GamePhase.SessionOver
              || this.lastDamageTrackingGamePhase == rF2GamePhase.SessionStopped
              || (int)this.lastDamageTrackingGamePhase == 9)  // What is 9? 
            && ((rF2GamePhase)scoring.mScoringInfo.mGamePhase == rF2GamePhase.Countdown
              || (rF2GamePhase)scoring.mScoringInfo.mGamePhase == rF2GamePhase.Formation
              || (rF2GamePhase)scoring.mScoringInfo.mGamePhase == rF2GamePhase.GridWalk
              || (rF2GamePhase)scoring.mScoringInfo.mGamePhase == rF2GamePhase.GreenFlag))
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

      this.lastDamageTrackingGamePhase = (rF2GamePhase)scoring.mScoringInfo.mGamePhase;

      if (scoring.mScoringInfo.mNumVehicles == 0)
        return;

      // Build map of mID -> telemetry.mVehicles[i]. 
      // They are typically matching values, however, we need to handle online cases and dropped vehicles (mID can be reused).
      var idsToTelIndices = new Dictionary<long, int>();
      for (int i = 0; i < telemetry.mNumVehicles; ++i)
      {
        if (!idsToTelIndices.ContainsKey(telemetry.mVehicles[i].mID))
          idsToTelIndices.Add(telemetry.mVehicles[i].mID, i);
      }

      var playerVeh = MainForm.GetPlayerScoring(ref scoring);

      if (playerVeh.mIsPlayer != 1)
        return;

      var scoringPlrId = playerVeh.mID;
      if (!idsToTelIndices.ContainsKey(scoringPlrId))
        return;

      var resolvedIdx = idsToTelIndices[scoringPlrId];
      var playerVehTelemetry = telemetry.mVehicles[resolvedIdx];

      var di = new DamageInfo();
      di.mDentSeverity = playerVehTelemetry.mDentSeverity;
      di.mLastImpactMagnitude = playerVehTelemetry.mLastImpactMagnitude;
      di.mAccumulatedImpactMagnitude = extended.mTrackedDamages[playerVehTelemetry.mID % rFactor2Constants.MAX_MAPPED_IDS].mAccumulatedImpactMagnitude;
      di.mMaxImpactMagnitude = extended.mTrackedDamages[playerVehTelemetry.mID % rFactor2Constants.MAX_MAPPED_IDS].mMaxImpactMagnitude;
      di.mLastImpactPos = playerVehTelemetry.mLastImpactPos;
      di.mLastImpactET = playerVehTelemetry.mLastImpactET;
      di.mOverheating = playerVehTelemetry.mOverheating;
      di.mDetached = playerVehTelemetry.mDetached;
      di.mFrontLeftFlat = playerVehTelemetry.mWheels[(int)rF2WheelIndex.FrontLeft].mFlat;
      di.mFrontLeftDetached = playerVehTelemetry.mWheels[(int)rF2WheelIndex.FrontLeft].mDetached;
      di.mFrontRightFlat = playerVehTelemetry.mWheels[(int)rF2WheelIndex.FrontRight].mFlat;
      di.mFrontRightDetached = playerVehTelemetry.mWheels[(int)rF2WheelIndex.FrontRight].mDetached;
      di.mRearLeftFlat = playerVehTelemetry.mWheels[(int)rF2WheelIndex.RearLeft].mFlat;
      di.mRearLeftDetached = playerVehTelemetry.mWheels[(int)rF2WheelIndex.RearLeft].mDetached;
      di.mRearRightFlat = playerVehTelemetry.mWheels[(int)rF2WheelIndex.RearRight].mFlat;
      di.mRearRightDetached = playerVehTelemetry.mWheels[(int)rF2WheelIndex.RearRight].mDetached;

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
        var dmgYStart = this.screenYStart + 310.0f;
        g.DrawString(this.sbDamageChanged.ToString(), SystemFonts.DefaultFont, Brushes.Orange, 3.0f, dmgYStart + 3.0f);
        g.DrawString(this.sbDamageLabel.ToString(), SystemFonts.DefaultFont, Brushes.Green, 30.0f, dmgYStart);
        g.DrawString(this.sbDamageValues.ToString(), SystemFonts.DefaultFont, Brushes.Purple, 200.0f, dmgYStart);
      }
    }

    internal class PlayerTimingInfo
    {
      internal string name = null;
      internal double lastS1Time = -1.0;
      internal double lastS2Time = -1.0;
      internal double lastS3Time = -1.0;

      internal double currS1Time = -1.0;
      internal double currS2Time = -1.0;
      internal double currS3Time = -1.0;

      internal double bestS1Time = -1.0;
      internal double bestS2Time = -1.0;
      internal double bestS3Time = -1.0;

      internal double currLapET = -1.0;
      internal double lastLapTime = -1.0;
      internal double currLapTime = -1.0;
      internal double bestLapTime = -1.0;

      internal int currLap = -1;
    }

    internal class OpponentTimingInfo
    {
      internal string name = null;
      internal int position = -1;
      internal double lastS1Time = -1.0;
      internal double lastS2Time = -1.0;
      internal double lastS3Time = -1.0;

      internal double currS1Time = -1.0;
      internal double currS2Time = -1.0;
      internal double currS3Time = -1.0;

      internal double bestS1Time = -1.0;
      internal double bestS2Time = -1.0;
      internal double bestS3Time = -1.0;

      internal double currLapET = -1.0;
      internal double lastLapTime = -1.0;
      internal double currLapTime = -1.0;
      internal double bestLapTime = -1.0;

      internal int currLap = -1;

      internal string vehicleName = null;
      internal string vehicleClass = null;

      internal long mID = -1;
    }

    // string -> lap data

    internal class LapData
    {
      internal class LapStats
      {
        internal int lapNumber = -1;
        internal double lapTime = -1.0;
        internal double S1Time = -1.0;
        internal double S2Time = -1.0;
        internal double S3Time = -1.0;
      }

      internal int lastLapCompleted = -1;
      internal List<LapStats> lapStats = new List<LapStats>();
    }

    internal Dictionary<string, LapData> lapDataMap = null;

    int lastTimingSector = -1;
    string bestSplitString = "";

    private int GetSector(int rf2Sector) { return rf2Sector == 0 ? 3 : rf2Sector; }
    private string LapTimeStr(double time)
    {
      return time > 0.0 ? TimeSpan.FromSeconds(time).ToString(@"mm\:ss\:fff") : time.ToString();
    }

    internal void TrackTimings(ref rF2Scoring scoring, ref rF2Telemetry telemetry, ref rF2Rules rules, ref rF2Extended extended, Graphics g, bool logToFile)
    {
      if ((this.lastTimingTrackingGamePhase == rF2GamePhase.Garage
            || this.lastTimingTrackingGamePhase == rF2GamePhase.SessionOver
            || this.lastTimingTrackingGamePhase == rF2GamePhase.SessionStopped
            || (int)this.lastTimingTrackingGamePhase == 9)  // What is 9? 
          && ((rF2GamePhase)scoring.mScoringInfo.mGamePhase == rF2GamePhase.Countdown
            || (rF2GamePhase)scoring.mScoringInfo.mGamePhase == rF2GamePhase.Formation
            || (rF2GamePhase)scoring.mScoringInfo.mGamePhase == rF2GamePhase.GridWalk
            || (rF2GamePhase)scoring.mScoringInfo.mGamePhase == rF2GamePhase.GreenFlag))
      {
        this.lapDataMap = null;
        this.bestSplitString = "";
        if (logToFile)
        {
          var lines = new List<string>();
          lines.Add("\n");
          lines.Add("************************************************************************************");
          lines.Add("* NEW SESSION **********************************************************************");
          lines.Add("************************************************************************************");
          File.AppendAllLines(timingTrackingFilePath, lines);
        }
      }

      this.lastTimingTrackingGamePhase = (rF2GamePhase)scoring.mScoringInfo.mGamePhase;

      if (scoring.mScoringInfo.mNumVehicles == 0 
        || extended.mSessionStarted == 0)
      {
        this.lastTimingSector = -1;
        this.lapDataMap = null;
        this.bestSplitString = "";

        return;
      }

      if (this.lapDataMap == null)
        this.lapDataMap = new Dictionary<string, LapData>();

      // Build map of mID -> telemetry.mVehicles[i]. 
      // They are typically matching values, however, we need to handle online cases and dropped vehicles (mID can be reused).
      var idsToTelIndices = new Dictionary<long, int>();
      for (int i = 0; i < telemetry.mNumVehicles; ++i)
      {
        if (!idsToTelIndices.ContainsKey(telemetry.mVehicles[i].mID))
          idsToTelIndices.Add(telemetry.mVehicles[i].mID, i);
      }

      var playerVeh = MainForm.GetPlayerScoring(ref scoring);

      if (playerVeh.mIsPlayer != 1)
        return;

      var scoringPlrId = playerVeh.mID;
      if (!idsToTelIndices.ContainsKey(scoringPlrId))
        return;

      var resolvedIdx = idsToTelIndices[scoringPlrId];
      var playerVehTelemetry = telemetry.mVehicles[resolvedIdx];

      bool sectorChanged = this.lastTimingSector != this.GetSector(playerVeh.mSector);
      bool newLap = this.lastTimingSector == 3 && this.GetSector(playerVeh.mSector) == 1;

      this.lastTimingSector = this.GetSector(playerVeh.mSector);

      StringBuilder sbPlayer = null;
      PlayerTimingInfo ptiPlayer = null;
      var bls = this.getBestLapStats(TransitionTracker.GetStringFromBytes(playerVeh.mDriverName), newLap /*skipLastLap*/);
      this.getDetailedVehTiming("Player:", ref playerVeh, bls, ref scoring, out sbPlayer, out ptiPlayer);

      var opponentInfos = new List<OpponentTimingInfo>();
      for (int i = 0; i < scoring.mScoringInfo.mNumVehicles; ++i)
      {
        var veh = scoring.mVehicles[i];
        var o = new OpponentTimingInfo();
        o.mID = veh.mID;
        o.name = TransitionTracker.GetStringFromBytes(veh.mDriverName);
        o.position = veh.mPlace;

        o.lastS1Time = veh.mLastSector1 > 0.0 ? veh.mLastSector1 : -1.0;
        o.lastS2Time = veh.mLastSector1 > 0.0 && veh.mLastSector2 > 0.0
          ? veh.mLastSector2 - veh.mLastSector1 : -1.0;
        o.lastS3Time = veh.mLastSector2 > 0.0 && veh.mLastLapTime > 0.0
          ? veh.mLastLapTime - veh.mLastSector2 : -1.0;

        o.currS1Time = o.lastS1Time;
        o.currS2Time = o.lastS2Time;
        o.currS3Time = o.lastS3Time;

        // Check if we have more current values for S1 and S2.
        // S3 always equals to lastS3Time.
        if (veh.mCurSector1 > 0.0)
          o.currS1Time = veh.mCurSector1;

        if (veh.mCurSector1 > 0.0 && veh.mCurSector2 > 0.0)
          o.currS2Time = veh.mCurSector2 - veh.mCurSector1;

        o.bestS1Time = veh.mBestSector1 > 0.0 ? veh.mBestSector1 : -1.0;
        o.bestS2Time = veh.mBestSector1 > 0.0 && veh.mBestSector2 > 0.0 ? veh.mBestSector2 - veh.mBestSector1 : -1.0;

        // Wrong:
        o.bestS3Time = veh.mBestSector2 > 0.0 && veh.mBestLapTime > 0.0 ? veh.mBestLapTime - veh.mBestSector2 : -1.0;

        o.currLapET = veh.mLapStartET;
        o.lastLapTime = veh.mLastLapTime;
        o.currLapTime = scoring.mScoringInfo.mCurrentET - veh.mLapStartET;
        o.bestLapTime = veh.mBestLapTime;
        o.currLap = veh.mTotalLaps;
        o.vehicleName = TransitionTracker.GetStringFromBytes(veh.mVehicleName);
        o.vehicleClass = TransitionTracker.GetStringFromBytes(veh.mVehicleClass);

        opponentInfos.Add(o);
      }

      // Order by pos, ascending.
      opponentInfos.Sort((o1, o2) => o1.position.CompareTo(o2.position));
      var sbOpponentNames = new StringBuilder();
      sbOpponentNames.Append("Name | Class | Vehicle:\n");
      foreach (var o in opponentInfos)
        sbOpponentNames.Append($"{o.name} | {o.vehicleClass} | {o.vehicleName}\n");

      // Save lap times history.
      for (int i = 0; i < scoring.mScoringInfo.mNumVehicles; ++i)
      {
        var veh = scoring.mVehicles[i];
        var driverName = TransitionTracker.GetStringFromBytes(veh.mDriverName);

        // If we don't have this vehicle in a map, add it. (And initialize laps completed).
        if (!this.lapDataMap.ContainsKey(driverName))
        {
          var ldNew = new LapData();
          ldNew.lastLapCompleted = veh.mTotalLaps;
          this.lapDataMap.Add(driverName, ldNew);
        }

        // If this is the new lap for this vehicle, update the lastLapNumber, and save last lap stats.
        var ld = this.lapDataMap[driverName];
        if (ld.lastLapCompleted != veh.mTotalLaps)
        {
          ld.lastLapCompleted = veh.mTotalLaps;

          // Only record valid laps.
          if (veh.mLastLapTime > 0.0)
          {
            var lsNew = new LapData.LapStats
            {
              lapNumber = veh.mTotalLaps,
              lapTime = veh.mLastLapTime,
              S1Time = veh.mLastSector1,
              S2Time = veh.mLastSector2 - veh.mLastSector1,
              S3Time = veh.mLastLapTime - veh.mLastSector2
            };

            ld.lapStats.Add(lsNew);
          }
        }
      }

      var idsToParticipantIndices = new Dictionary<long, int>();
      for (int i = 0; i < rules.mTrackRules.mNumParticipants; ++i)
      {
        if (!idsToParticipantIndices.ContainsKey(rules.mParticipants[i].mID))
          idsToParticipantIndices.Add(rules.mParticipants[i].mID, i);
      }

      var sbOpponentStats = new StringBuilder();
      sbOpponentStats.Append("Pos:  Lap:      Best Tracked:      Best S1:      Best S2:      Best S3:     Col. Assigned:    Pos. Assigned:\n");
      foreach (var o in opponentInfos)
      {
        var skipLastLap = o.name == TransitionTracker.GetStringFromBytes(playerVeh.mDriverName) && newLap;
        var bestLapStats = this.getBestLapStats(o.name, skipLastLap);

        var bestLapS1 = bestLapStats.S1Time;
        var bestLapS2 = bestLapStats.S2Time;
        var bestLapS3 = bestLapStats.S3Time;
        var bestLapTimeTracked = bestLapStats.lapTime;

        var opponetRules = new rF2TrackRulesParticipant();
        int participantIdx = -1;
        if (idsToParticipantIndices.TryGetValue(o.mID, out participantIdx))
          opponetRules = rules.mParticipants[participantIdx];

        sbOpponentStats.Append($"{o.position,5}{o.currLap,8}{this.LapTimeStr(bestLapTimeTracked),22:N3}{this.LapTimeStr(bestLapS1),13:N3}{this.LapTimeStr(bestLapS2),13:N3}{this.LapTimeStr(bestLapS3),13:N3}{opponetRules.mColumnAssignment,17}{opponetRules.mPositionAssignment,13}\n");
      }

      // Find fastest vehicle.
      var blsFastest = new LapData.LapStats();
      var fastestName = "";
      foreach (var lapData in this.lapDataMap)
      {
        // If this is the new lap, ignore just completed lap for the player vehicle, and use time of one lap before.
        bool skipLastLap = newLap && lapData.Key == TransitionTracker.GetStringFromBytes(playerVeh.mDriverName);

        var blsCandidate = this.getBestLapStats(lapData.Key, skipLastLap);
        if (blsCandidate.lapTime < 0.0)
          continue;

        if (blsFastest.lapTime < 0.0 
          || blsCandidate.lapTime < blsFastest.lapTime)
        {
          fastestName = lapData.Key;
          blsFastest = blsCandidate;
        }
      }

      int fastestIndex = -1;
      for (int i = 0; i < scoring.mScoringInfo.mNumVehicles; ++i)
      {
        if (fastestName == TransitionTracker.GetStringFromBytes(scoring.mVehicles[i].mDriverName))
        {
          fastestIndex = i;
          break;
        }
      }

      PlayerTimingInfo ptiFastest = null;
      var sbFastest = new StringBuilder("");
      if (fastestIndex != -1)
      {
        var fastestVeh = scoring.mVehicles[fastestIndex];
        if (blsFastest.lapTime > 0.0)
        {
        //'  var blsFastest = this.getBestLapStats(this.getStringFromBytes(fastestVeh.mDriverName));
          this.getDetailedVehTiming("Fastest:", ref fastestVeh, blsFastest, ref scoring, out sbFastest, out ptiFastest);
        }
      }

      var sbPlayerDeltas = new StringBuilder("");
      if (ptiFastest != null)
      {
        var deltaLapTime = ptiPlayer.bestLapTime - ptiFastest.bestLapTime;
        var deltaS1Time = ptiPlayer.bestS1Time - ptiFastest.bestS1Time;
        var deltaS2Time = ptiPlayer.bestS2Time - ptiFastest.bestS2Time;
        var deltaS3Time = ptiPlayer.bestS3Time - ptiFastest.bestS3Time;

        var deltaSelfLapTime = ptiPlayer.lastLapTime - ptiPlayer.bestLapTime;
        var deltaSelfS1Time = ptiPlayer.currS1Time - ptiPlayer.bestS1Time;
        var deltaSelfS2Time = ptiPlayer.currS2Time - ptiPlayer.bestS2Time;
        var deltaSelfS3Time = ptiPlayer.currS3Time - ptiPlayer.bestS3Time;

        var deltaCurrSelfLapTime = ptiPlayer.lastLapTime - ptiFastest.bestLapTime;
        var deltaCurrSelfS1Time = ptiPlayer.currS1Time - ptiFastest.bestS1Time;
        var deltaCurrSelfS2Time = ptiPlayer.currS2Time - ptiFastest.bestS2Time;
        var deltaCurrSelfS3Time = ptiPlayer.currS3Time - ptiFastest.bestS3Time;

        var deltaCurrSelfLapStr = deltaCurrSelfLapTime > 0.0 ? "+" : "";
        deltaCurrSelfLapStr = deltaCurrSelfLapStr + $"{deltaCurrSelfLapTime:N3}";

        var deltaCurrSelfS1Str = deltaCurrSelfS1Time > 0.0 ? "+" : "";
        deltaCurrSelfS1Str = deltaCurrSelfS1Str + $"{deltaCurrSelfS1Time:N3}";

        var deltaCurrSelfS2Str = deltaCurrSelfS2Time > 0.0 ? "+" : "";
        deltaCurrSelfS2Str = deltaCurrSelfS2Str + $"{deltaCurrSelfS2Time:N3}";

        var deltaCurrSelfS3Str = deltaCurrSelfS3Time > 0.0 ? "+" : "";
        deltaCurrSelfS3Str = deltaCurrSelfS3Str + $"{deltaCurrSelfS3Time:N3}";

        sbPlayerDeltas.Append($"Player delta current vs session best:    deltaCurrSelfBestLapTime: {deltaCurrSelfLapStr}\ndeltaCurrSelfBestS1: {deltaCurrSelfS1Str}    deltaCurrSelfBestS2: {deltaCurrSelfS2Str}    deltaCurrSelfBestS3: {deltaCurrSelfS3Str}\n\n");

        // Once per sector change.
        if (sectorChanged)
        {
          // Calculate "Best Split" to match rFactor 2 HUDs
          var currSector = this.GetSector(playerVeh.mSector);
          double bestSplit = 0.0;
          if (currSector == 1)
            bestSplit = ptiPlayer.lastLapTime - ptiFastest.bestLapTime;
          else if (currSector == 2)
            bestSplit = ptiPlayer.currS1Time - ptiFastest.bestS1Time;
          else
            bestSplit = (ptiPlayer.currS1Time + ptiPlayer.currS2Time) - (ptiFastest.bestS1Time + ptiFastest.bestS2Time);

          var bestSplitStr = bestSplit > 0.0 ? "+" : "";
          bestSplitStr += $"{bestSplit:N3}";

          this.bestSplitString = $"Best Split: {bestSplitStr}\n\n";
        }

        sbPlayerDeltas.Append(this.bestSplitString);

        var deltaSelfLapStr = deltaSelfLapTime > 0.0 ? "+" : "";
        deltaSelfLapStr = deltaSelfLapStr + $"{deltaSelfLapTime:N3}";

        var deltaSelfS1Str = deltaSelfS1Time > 0.0 ? "+" : "";
        deltaSelfS1Str = deltaSelfS1Str + $"{deltaSelfS1Time:N3}";

        var deltaSelfS2Str = deltaSelfS2Time > 0.0 ? "+" : "";
        deltaSelfS2Str = deltaSelfS2Str + $"{deltaSelfS2Time:N3}";

        var deltaSelfS3Str = deltaSelfS3Time > 0.0 ? "+" : "";
        deltaSelfS3Str = deltaSelfS3Str + $"{deltaSelfS3Time:N3}";

        sbPlayerDeltas.Append($"Player delta current vs self best:    deltaSelfBestLapTime: {deltaSelfLapStr}\ndeltaSelfBestS1: {deltaSelfS1Str}    deltaSelfBestS2: {deltaSelfS2Str}    deltaBestS3: {deltaSelfS3Str}\n\n");

        var deltaLapStr = deltaLapTime > 0.0 ? "+" : "";
        deltaLapStr = deltaLapStr + $"{deltaLapTime:N3}";

        var deltaS1Str = deltaS1Time > 0.0 ? "+" : "";
        deltaS1Str = deltaS1Str + $"{deltaS1Time:N3}";

        var deltaS2Str = deltaS2Time > 0.0 ? "+" : "";
        deltaS2Str = deltaS2Str + $"{deltaS2Time:N3}";

        var deltaS3Str = deltaS3Time > 0.0 ? "+" : "";
        deltaS3Str = deltaS3Str + $"{deltaS3Time:N3}";

        sbPlayerDeltas.Append($"Player delta best vs session best:    deltaBestLapTime: {deltaLapStr}\ndeltaBestS1: {deltaS1Str}    deltaBestS2: {deltaS2Str}    deltaBestS3: {deltaS3Str}\n\n");
      }

      if (logToFile && sectorChanged)
      {
        var updateTime = DateTime.Now.ToString();
        File.AppendAllText(timingTrackingFilePath, $"\n\n{updateTime}    Sector: {this.lastTimingSector}  ***************************************************** \n\n");

        File.AppendAllText(timingTrackingFilePath, sbPlayer.ToString() + "\n");
        File.AppendAllText(timingTrackingFilePath, sbPlayerDeltas.ToString() + "\n");
        File.AppendAllText(timingTrackingFilePath, sbFastest.ToString() + "\n");

        var names = sbOpponentNames.ToString().Split('\n');
        var stats = sbOpponentStats.ToString().Split('\n');
        var lines = new List<string>();
        Debug.Assert(names.Count() == stats.Count());

        for (int i = 0; i < names.Count(); ++i)
          lines.Add($"{stats[i]}    {names[i]}");

        File.AppendAllLines(timingTrackingFilePath, lines);
      }

      if (g != null)
      {
        var timingsYStart = this.screenYStart + 435.0f;
        g.DrawString(sbPlayer.ToString(), SystemFonts.DefaultFont, Brushes.Magenta, 3.0f, timingsYStart);
        g.DrawString(sbPlayerDeltas.ToString(), SystemFonts.DefaultFont, Brushes.Black, 3.0f, timingsYStart + 70.0f);
        g.DrawString(sbFastest.ToString(), SystemFonts.DefaultFont, Brushes.OrangeRed, 3.0f, timingsYStart + 200.0f);
        g.DrawString(sbOpponentNames.ToString(), SystemFonts.DefaultFont, Brushes.Green, 530.0f, 3.0f);
        g.DrawString(sbOpponentStats.ToString(), SystemFonts.DefaultFont, Brushes.Purple, 860.0f, 3.0f);
      }
    }

    private LapData.LapStats getBestLapStats(string opponentName, bool skipLastLap)
    {
      LapData.LapStats bestLapStats = new LapData.LapStats();
      if (this.lapDataMap.ContainsKey(opponentName))
      {
        var opLd = this.lapDataMap[opponentName];

        double bestLapTimeTracked = -1.0;
        var lapsToCheck = opLd.lapStats.Count;
        if (skipLastLap)
          --lapsToCheck;

        for (int i = 0; i < lapsToCheck; ++i)
        {
          var ls = opLd.lapStats[i];
          if (bestLapStats.lapTime < 0.0
            || ls.lapTime < bestLapTimeTracked)
          {
            bestLapTimeTracked = ls.lapTime;
            bestLapStats = ls;
          }
        }
      }

      return bestLapStats;
    }

    private void getDetailedVehTiming(string name, ref rF2VehicleScoring vehicle, LapData.LapStats bestLapStats, ref rF2Scoring scoring, out StringBuilder sbDetails, out PlayerTimingInfo pti)
    {
      pti = new PlayerTimingInfo();
      pti.name = TransitionTracker.GetStringFromBytes(vehicle.mDriverName);
      pti.lastS1Time = vehicle.mLastSector1 > 0.0 ? vehicle.mLastSector1 : -1.0;
      pti.lastS2Time = vehicle.mLastSector1 > 0.0 && vehicle.mLastSector2 > 0.0
        ? vehicle.mLastSector2 - vehicle.mLastSector1 : -1.0;
      pti.lastS3Time = vehicle.mLastSector2 > 0.0 && vehicle.mLastLapTime > 0.0
        ? vehicle.mLastLapTime - vehicle.mLastSector2 : -1.0;

      pti.currS1Time = pti.lastS1Time;
      pti.currS2Time = pti.lastS2Time;
      pti.currS3Time = pti.lastS3Time;

      // Check if we have more current values for S1 and S2.
      // S3 always equals to lastS3Time.
      if (vehicle.mCurSector1 > 0.0)
        pti.currS1Time = vehicle.mCurSector1;

      if (vehicle.mCurSector1 > 0.0 && vehicle.mCurSector2 > 0.0)
        pti.currS2Time = vehicle.mCurSector2 - vehicle.mCurSector1;

      /*pti.bestS1Time = vehicle.mBestSector1 > 0.0 ? vehicle.mBestSector1 : -1.0;
      pti.bestS2Time = vehicle.mBestSector1 > 0.0 && vehicle.mBestSector2 > 0.0 ? vehicle.mBestSector2 - vehicle.mBestSector1 : -1.0;

      // This is not correct.  mBestLapTime does not neccessarily includes all three best sectors together.  The only way to calculate this is by continuous tracking.
      // However, currently there's no need for this value at all, so I don't care.
      pti.bestS3Time = vehicle.mBestSector2 > 0.0 && vehicle.mBestLapTime > 0.0 ? vehicle.mBestLapTime - vehicle.mBestSector2 : -1.0;*/

      // We need to skip previous player lap stats during comparison on new lap, hence we don't use vehicle values for those.
      pti.bestS1Time = bestLapStats.S1Time;
      pti.bestS2Time = bestLapStats.S2Time;
      pti.bestS3Time = bestLapStats.S3Time;
      pti.bestLapTime = bestLapStats.lapTime;

      pti.currLapET = vehicle.mLapStartET;
      pti.lastLapTime = vehicle.mLastLapTime;
      pti.currLapTime = scoring.mScoringInfo.mCurrentET - vehicle.mLapStartET;
      
      pti.currLap = vehicle.mTotalLaps;

      sbDetails = new StringBuilder();
      sbDetails.Append($"{name} {pti.name}\ncurrLapET: {this.LapTimeStr(pti.currLapET)}    lastLapTime: {this.LapTimeStr(pti.lastLapTime)}    currLapTime: {this.LapTimeStr(pti.currLapTime)}    bestLapTime: {this.LapTimeStr(pti.bestLapTime)}\n");
      sbDetails.Append($"lastS1: {this.LapTimeStr(pti.lastS1Time)}    lastS2: {this.LapTimeStr(pti.lastS2Time)}    lastS3: {this.LapTimeStr(pti.lastS3Time)}\n");
      sbDetails.Append($"currS1: {this.LapTimeStr(pti.currS1Time)}    currS2: {this.LapTimeStr(pti.currS2Time)}    currS3: {this.LapTimeStr(pti.currS3Time)}\n");
      sbDetails.Append($"bestS1: {this.LapTimeStr(pti.bestS1Time)}    bestS2: {this.LapTimeStr(pti.bestS2Time)}    bestS3: {this.LapTimeStr(pti.bestS3Time)}    bestTotal: {this.LapTimeStr(pti.bestS1Time + pti.bestS2Time + pti.bestS3Time)}\n");
    }

    internal class Rules
    {
      public rF2TrackRulesStage mStage = (rF2TrackRulesStage)Enum.ToObject(typeof(rF2TrackRulesStage), -255);
      public rF2TrackRulesColumn mPoleColumn = (rF2TrackRulesColumn)Enum.ToObject(typeof(rF2TrackRulesColumn), -255);      // column assignment where pole position seems to be located
      public int mNumActions = -1;                     // number of recent actions
      public int mNumParticipants = -1;                // number of participants (vehicles)

      public byte mYellowFlagDetected = 255;             // whether yellow flag was requested or sum of participant mYellowSeverity's exceeds mSafetyCarThreshold
      public byte mYellowFlagLapsWasOverridden = 255;    // whether mYellowFlagLaps (below) is an admin request

      public byte mSafetyCarExists = 255;                // whether safety car even exists
      public byte mSafetyCarActive = 255;                // whether safety car is active
      public int mSafetyCarLaps = 255;                  // number of laps
      public float mSafetyCarThreshold = -1.0f;            // the threshold at which a safety car is called out (compared to the sum of TrackRulesParticipantV01::mYellowSeverity for each vehicle)
      public double mSafetyCarLapDist;             // safety car lap distance
      public float mSafetyCarLapDistAtStart;       // where the safety car starts from

      public float mPitLaneStartDist = -1.0f;              // where the waypoint branch to the pits breaks off (this may not be perfectly accurate)
      public float mTeleportLapDist = -1.0f;               // the front of the teleport locations (a useful first guess as to where to throw the green flag)

      // input/output
      public sbyte mYellowFlagState = 127;         // see ScoringInfoV01 for values
      public short mYellowFlagLaps = 127;                // suggested number of laps to run under yellow (may be passed in with admin command)

      public rF2SafetyCarInstruction mSafetyCarInstruction = (rF2SafetyCarInstruction)Enum.ToObject(typeof(rF2SafetyCarInstruction), -255);
      public float mSafetyCarSpeed = -1.0f;                // maximum speed at which to drive
      public float mSafetyCarMinimumSpacing = -2.0f;       // minimum spacing behind safety car (-1 to indicate no limit)
      public float mSafetyCarMaximumSpacing = -2.0f;       // maximum spacing behind safety car (-1 to indicate no limit)

      public float mMinimumColumnSpacing = -2.0f;          // minimum desired spacing between vehicles in a column (-1 to indicate indeterminate/unenforced)
      public float mMaximumColumnSpacing = -2.0f;          // maximum desired spacing between vehicles in a column (-1 to indicate indeterminate/unenforced)

      public float mMinimumSpeed = -2.0f;                  // minimum speed that anybody should be driving (-1 to indicate no limit)
      public float mMaximumSpeed = -2.0f;                  // maximum speed that anybody should be driving (-1 to indicate no limit)

      public string mMessage = "unknown";                  // a message for everybody to explain what is going on (which will get run through translator on client machines)

      public short mFrozenOrder = 127;                           // 0-based place when caution came out (not valid for formation laps)
      public short mPlace = 127;                                 // 1-based place (typically used for the initialization of the formation lap track order)
      public float mYellowSeverity = -1.0f;                        // a rating of how much this vehicle is contributing to a yellow flag (the sum of all vehicles is compared to TrackRulesV01::mSafetyCarThreshold)
      public double mCurrentRelativeDistance = -1.0;              // equal to ( ( ScoringInfoV01::mLapDist * this->mRelativeLaps ) + VehicleScoringInfoV01::mLapDist )

      // input/output
      public int mRelativeLaps = -1;                            // current formation/caution laps relative to safety car (should generally be zero except when safety car crosses s/f line); this can be decremented to implement 'wave around' or 'beneficiary rule' (a.k.a. 'lucky dog' or 'free pass')
      public rF2TrackRulesColumn mColumnAssignment = (rF2TrackRulesColumn)Enum.ToObject(typeof(rF2TrackRulesColumn), -255);        // which column (line/lane) that participant is supposed to be in
      public int mPositionAssignment = -1;                      // 0-based position within column (line/lane) that participant is supposed to be located at (-1 is invalid)
      public byte mAllowedToPit = 255;                           // whether the rules allow this particular vehicle to enter pits right now

      public double mGoalRelativeDistance = -1.0;                 // calculated based on where the leader is, and adjusted by the desired column spacing and the column/position assignments

      public string mMessage_Participant = "unknown";                  // a message for this participant to explain what is going on (untranslated; it will get run through translator on client machines)
    }

    internal Rules prevRules = new Rules();
    internal StringBuilder sbRulesChanged = new StringBuilder();
    internal StringBuilder sbRulesLabel = new StringBuilder();
    internal StringBuilder sbRulesValues = new StringBuilder();
    internal StringBuilder sbFrozenOrderInfo = new StringBuilder();
    private FrozenOrderData prevFrozenOrderData;

    public enum FrozenOrderPhase
    {
      None,
      FullCourseYellow,
      FormationStanding,
      Rolling,
      FastRolling
    }

    public enum FrozenOrderColumn
    {
      None,
      Left,
      Right
    }

    public enum FrozenOrderAction
    {
      None,
      Follow,
      CatchUp,
      AllowToPass
    }

    public class FrozenOrderData
    {
      public FrozenOrderPhase Phase = FrozenOrderPhase.None;
      public FrozenOrderAction Action = FrozenOrderAction.None;

      // If column is assigned, p1 and p2 follows SC.  Otherwise,
      // only p1 follows SC.
      public int AssignedPosition = -1;

      public FrozenOrderColumn AssignedColumn = FrozenOrderColumn.None;
      // Only matters if AssignedColumn != None
      public int AssignedGridPosition = -1;

      public string DriverToFollow = "";

      // Meters/s.  If -1, SC either left or not present.
      public float SafetyCarSpeed = -1.0f;
    }

    internal void TrackRules(ref rF2Scoring scoring, ref rF2Telemetry telemetry, ref rF2Rules rules, ref rF2Extended extended, Graphics g, bool logToFile)
    {
      if (logToFile)
      {
        if ((this.lastRulesTrackingGamePhase == rF2GamePhase.Garage
              || this.lastRulesTrackingGamePhase == rF2GamePhase.SessionOver
              || this.lastRulesTrackingGamePhase == rF2GamePhase.SessionStopped
              || (int)this.lastRulesTrackingGamePhase == 9)  // What is 9? 
            && ((rF2GamePhase)scoring.mScoringInfo.mGamePhase == rF2GamePhase.Countdown
              || (rF2GamePhase)scoring.mScoringInfo.mGamePhase == rF2GamePhase.Formation
              || (rF2GamePhase)scoring.mScoringInfo.mGamePhase == rF2GamePhase.GridWalk
              || (rF2GamePhase)scoring.mScoringInfo.mGamePhase == rF2GamePhase.GreenFlag))
        {
          var lines = new List<string>();
          lines.Add("\n");
          lines.Add("************************************************************************************");
          lines.Add("* NEW SESSION **********************************************************************");
          lines.Add("************************************************************************************");
          File.AppendAllLines(rulesTrackingFilePath, lines);
          File.AppendAllLines(rulesTrackingDeltaFilePath, lines);
        }
      }

      this.lastRulesTrackingGamePhase = (rF2GamePhase)scoring.mScoringInfo.mGamePhase;

      if (scoring.mScoringInfo.mNumVehicles == 0)
        return;

      // Build map of mID -> telemetry.mVehicles[i]. 
      // They are typically matching values, however, we need to handle online cases and dropped vehicles (mID can be reused).
      var idsToTelIndices = new Dictionary<long, int>();
      for (int i = 0; i < telemetry.mNumVehicles; ++i)
      {
        if (!idsToTelIndices.ContainsKey(telemetry.mVehicles[i].mID))
          idsToTelIndices.Add(telemetry.mVehicles[i].mID, i);
      }

      var playerVeh = MainForm.GetPlayerScoring(ref scoring);

      if (playerVeh.mIsPlayer != 1)
        return;

      var scoringPlrId = playerVeh.mID;
      if (!idsToTelIndices.ContainsKey(scoringPlrId))
        return;

      var resolvedIdx = idsToTelIndices[scoringPlrId];
      var playerVehTelemetry = telemetry.mVehicles[resolvedIdx];

      var playerRules = new rF2TrackRulesParticipant();
      for (int i = 0; i < rules.mTrackRules.mNumParticipants; ++i)
      {
        if (rules.mParticipants[i].mID == scoringPlrId)
        {
          playerRules = rules.mParticipants[i];
          break;
        }
      }

      var rs = new Rules();

      rs.mStage = rules.mTrackRules.mStage;
      rs.mPoleColumn = rules.mTrackRules.mPoleColumn;
      rs.mNumActions = rules.mTrackRules.mNumActions;
      rs.mNumParticipants = rules.mTrackRules.mNumParticipants;
      rs.mYellowFlagDetected = rules.mTrackRules.mYellowFlagDetected;
      rs.mYellowFlagLapsWasOverridden = rules.mTrackRules.mYellowFlagLapsWasOverridden;
      rs.mSafetyCarExists = rules.mTrackRules.mSafetyCarExists;
      rs.mSafetyCarActive = rules.mTrackRules.mSafetyCarActive;
      rs.mSafetyCarLaps = rules.mTrackRules.mSafetyCarLaps;
      rs.mSafetyCarThreshold = rules.mTrackRules.mSafetyCarThreshold;
      rs.mSafetyCarLapDist = rules.mTrackRules.mSafetyCarLapDist;
      rs.mSafetyCarLapDistAtStart = rules.mTrackRules.mSafetyCarLapDistAtStart;
      rs.mPitLaneStartDist = rules.mTrackRules.mPitLaneStartDist;
      rs.mTeleportLapDist = rules.mTrackRules.mTeleportLapDist;
      rs.mYellowFlagState = rules.mTrackRules.mYellowFlagState;
      rs.mYellowFlagLaps = rules.mTrackRules.mYellowFlagLaps;
      rs.mSafetyCarInstruction = (rF2SafetyCarInstruction)rules.mTrackRules.mSafetyCarInstruction;
      rs.mSafetyCarSpeed = rules.mTrackRules.mSafetyCarSpeed;
      rs.mSafetyCarMinimumSpacing = rules.mTrackRules.mSafetyCarMinimumSpacing;
      rs.mSafetyCarMaximumSpacing = rules.mTrackRules.mSafetyCarMaximumSpacing;
      rs.mMinimumColumnSpacing = rules.mTrackRules.mMinimumColumnSpacing;
      rs.mMaximumColumnSpacing = rules.mTrackRules.mMaximumColumnSpacing;
      rs.mMinimumSpeed = rules.mTrackRules.mMinimumSpeed;
      rs.mMaximumSpeed = rules.mTrackRules.mMaximumSpeed;
      rs.mMessage = TransitionTracker.GetStringFromBytes(rules.mTrackRules.mMessage);

      // Player specific:
      rs.mFrozenOrder = playerRules.mFrozenOrder;
      rs.mPlace = playerRules.mPlace;
      rs.mYellowSeverity = playerRules.mYellowSeverity;
      rs.mCurrentRelativeDistance = playerRules.mCurrentRelativeDistance;
      rs.mRelativeLaps = playerRules.mRelativeLaps;
      rs.mColumnAssignment = playerRules.mColumnAssignment;
      rs.mPositionAssignment = playerRules.mPositionAssignment;
      rs.mAllowedToPit = playerRules.mAllowedToPit;
      rs.mGoalRelativeDistance = playerRules.mGoalRelativeDistance;
      rs.mMessage_Participant = TransitionTracker.GetStringFromBytes(playerRules.mMessage);

      // Only refresh UI if there's change.
      // some fields are commented out because they change pretty much every frame.
      if (rs.mStage != this.prevRules.mStage
        || rs.mPoleColumn != this.prevRules.mPoleColumn
        || rs.mNumActions != this.prevRules.mNumActions
        || rs.mNumParticipants != this.prevRules.mNumParticipants
        || rs.mYellowFlagDetected != this.prevRules.mYellowFlagDetected
        || rs.mYellowFlagLapsWasOverridden != this.prevRules.mYellowFlagLapsWasOverridden
        || rs.mSafetyCarExists != this.prevRules.mSafetyCarExists
        || rs.mSafetyCarActive != this.prevRules.mSafetyCarActive
        || rs.mSafetyCarLaps != this.prevRules.mSafetyCarLaps
        || rs.mSafetyCarThreshold != this.prevRules.mSafetyCarThreshold
        //public double mSafetyCarLapDist             // safety car lap distance
        //public float mSafetyCarLapDistAtStart       // where the safety car starts from
        || rs.mPitLaneStartDist != this.prevRules.mPitLaneStartDist
        || rs.mTeleportLapDist != this.prevRules.mTeleportLapDist
        || rs.mYellowFlagState != this.prevRules.mYellowFlagState
        || rs.mYellowFlagLaps != this.prevRules.mYellowFlagLaps
        || rs.mSafetyCarInstruction != this.prevRules.mSafetyCarInstruction
        || rs.mSafetyCarSpeed != this.prevRules.mSafetyCarSpeed
        || rs.mSafetyCarMinimumSpacing != this.prevRules.mSafetyCarMinimumSpacing
        || rs.mSafetyCarMaximumSpacing != this.prevRules.mSafetyCarMaximumSpacing
        || rs.mMinimumColumnSpacing != this.prevRules.mMinimumColumnSpacing
        || rs.mMaximumColumnSpacing != this.prevRules.mMaximumColumnSpacing
        || rs.mMinimumSpeed != this.prevRules.mMinimumSpeed
        || rs.mMaximumSpeed != this.prevRules.mMaximumSpeed
        || rs.mMessage != this.prevRules.mMessage
        || rs.mFrozenOrder != this.prevRules.mFrozenOrder
        || rs.mPlace != this.prevRules.mPlace
        //|| rs.mYellowSeverity != this.prevRules.mYellowSeverity
        //|| rs.mCurrentRelativeDistance != this.prevRules.mCurrentRelativeDistance
        || rs.mRelativeLaps != this.prevRules.mRelativeLaps
        || rs.mColumnAssignment != this.prevRules.mColumnAssignment
        || rs.mPositionAssignment != this.prevRules.mPositionAssignment
        || rs.mAllowedToPit != this.prevRules.mAllowedToPit
        || rs.mGoalRelativeDistance != this.prevRules.mGoalRelativeDistance
        || rs.mMessage_Participant != this.prevRules.mMessage_Participant)
      {
        this.sbRulesChanged = new StringBuilder();
        sbRulesChanged.Append((rs.mStage != this.prevRules.mStage ? "***\n" : "\n")
          + (rs.mPoleColumn != this.prevRules.mPoleColumn ? "***\n" : "\n")
          + (rs.mNumActions != this.prevRules.mNumActions ? "***\n" : "\n")
          + (rs.mNumParticipants != this.prevRules.mNumParticipants ? "***\n" : "\n")
          + (rs.mYellowFlagDetected != this.prevRules.mYellowFlagDetected ? "***\n" : "\n")
          + (rs.mYellowFlagLapsWasOverridden != this.prevRules.mYellowFlagLapsWasOverridden ? "***\n" : "\n")
          + (rs.mSafetyCarExists != this.prevRules.mSafetyCarExists ? "***\n" : "\n")
          + (rs.mSafetyCarActive != this.prevRules.mSafetyCarActive ? "***\n" : "\n")
          + (rs.mSafetyCarLaps != this.prevRules.mSafetyCarLaps ? "***\n" : "\n")
          + (rs.mSafetyCarThreshold != this.prevRules.mSafetyCarThreshold ? "***\n" : "\n")
          + (rs.mSafetyCarLapDist != this.prevRules.mSafetyCarLapDist ? "***\n" : "\n")
          + (rs.mSafetyCarLapDistAtStart != this.prevRules.mSafetyCarLapDistAtStart ? "***\n" : "\n")
          + (rs.mPitLaneStartDist != this.prevRules.mPitLaneStartDist ? "***\n" : "\n")
          + (rs.mTeleportLapDist != this.prevRules.mTeleportLapDist ? "***\n" : "\n")
          + (rs.mYellowFlagState != this.prevRules.mYellowFlagState ? "***\n" : "\n")
          + (rs.mYellowFlagLaps != this.prevRules.mYellowFlagLaps ? "***\n" : "\n")
          + (rs.mSafetyCarInstruction != this.prevRules.mSafetyCarInstruction ? "***\n" : "\n")
          + (rs.mSafetyCarSpeed != this.prevRules.mSafetyCarSpeed ? "***\n" : "\n")
          + (rs.mSafetyCarMinimumSpacing != this.prevRules.mSafetyCarMinimumSpacing ? "***\n" : "\n")
          + (rs.mSafetyCarMaximumSpacing != this.prevRules.mSafetyCarMaximumSpacing ? "***\n" : "\n")
          + (rs.mMinimumColumnSpacing != this.prevRules.mMinimumColumnSpacing ? "***\n" : "\n")
          + (rs.mMaximumColumnSpacing != this.prevRules.mMaximumColumnSpacing ? "***\n" : "\n")
          + (rs.mMinimumSpeed != this.prevRules.mMinimumSpeed ? "***\n" : "\n")
          + (rs.mMaximumSpeed != this.prevRules.mMaximumSpeed ? "***\n" : "\n")
          + (rs.mMessage != this.prevRules.mMessage ? "***\n" : "\n")
          + (rs.mFrozenOrder != this.prevRules.mFrozenOrder ? "***\n" : "\n")
          + (rs.mPlace != this.prevRules.mPlace ? "***\n" : "\n")
          + (rs.mYellowSeverity != this.prevRules.mYellowSeverity ? "***\n" : "\n")
          + (rs.mCurrentRelativeDistance != this.prevRules.mCurrentRelativeDistance ? "***\n" : "\n")
          + (rs.mRelativeLaps != this.prevRules.mRelativeLaps ? "***\n" : "\n")
          + (rs.mColumnAssignment != this.prevRules.mColumnAssignment ? "***\n" : "\n")
          + (rs.mPositionAssignment != this.prevRules.mPositionAssignment ? "***\n" : "\n")
          + (rs.mAllowedToPit != this.prevRules.mAllowedToPit ? "***\n" : "\n")
          + (rs.mGoalRelativeDistance != this.prevRules.mGoalRelativeDistance ? "***\n" : "\n")
          + (rs.mMessage_Participant != this.prevRules.mMessage_Participant ? "***\n" : "\n"));

        // Save current Rules and state.
        this.prevRules = rs;

        this.sbRulesLabel = new StringBuilder();
        sbRulesLabel.Append("mStage:\n"
          + "mPoleColumn:\n"
          + "mNumActions:\n"
          + "mNumParticipants:\n"
          + "mYellowFlagDetected:\n"
          + "mYellowFlagLapsWasOverridden:\n"
          + "mSafetyCarExists:\n"
          + "mSafetyCarActive:\n"
          + "mSafetyCarLaps:\n"
          + "mSafetyCarThreshold:\n"
          + "mSafetyCarLapDist:\n"
          + "mSafetyCarLapDistAtStart:\n"
          + "mPitLaneStartDist:\n"
          + "mTeleportLapDist:\n"
          + "mYellowFlagState:\n"
          + "mYellowFlagLaps:\n"
          + "mSafetyCarInstruction:\n"
          + "mSafetyCarSpeed:\n"
          + "mSafetyCarMinimumSpacing:\n"
          + "mSafetyCarMaximumSpacing:\n"
          + "mMinimumColumnSpacing:\n"
          + "mMaximumColumnSpacing:\n"
          + "mMinimumSpeed:\n"
          + "mMaximumSpeed:\n"
          + "mMessage:\n"
          + "mFrozenOrder:\n"
          + "mPlace:\n"
          + "mYellowSeverity:\n"
          + "mCurrentRelativeDistance:\n"
          + "mRelativeLaps:\n"
          + "mColumnAssignment:\n"
          + "mPositionAssignment:\n"
          + "mAllowedToPit:\n"
          + "mGoalRelativeDistance:\n"
          + "mMessage_Participant:\n");

        this.sbRulesValues = new StringBuilder();
        sbRulesValues.Append($"{rs.mStage}\n"
          + $"{rs.mPoleColumn}\n"
          + $"{rs.mNumActions}\n"
          + $"{rs.mNumParticipants}\n"
          + $"{rs.mYellowFlagDetected}\n"
          + (rs.mYellowFlagLapsWasOverridden == 0 ? $"false({rs.mYellowFlagLapsWasOverridden})" : $"true({rs.mYellowFlagLapsWasOverridden})") + "\n"
          + (rs.mSafetyCarExists == 0 ? $"false({rs.mSafetyCarExists})" : $"true({rs.mSafetyCarExists})") + "\n"
          + (rs.mSafetyCarActive == 0 ? $"false({rs.mSafetyCarActive})" : $"true({rs.mSafetyCarActive})") + "\n"
          + $"{rs.mSafetyCarLaps}\n"
          + $"{rs.mSafetyCarThreshold:N3}\n"
          + $"{rs.mSafetyCarLapDist:N3}\n"
          + $"{rs.mSafetyCarLapDistAtStart:N3}\n"
          + $"{rs.mPitLaneStartDist:N3}\n"
          + $"{rs.mTeleportLapDist:N3}\n"
          + $"{GetEnumString<rF2YellowFlagState>(rs.mYellowFlagState)}\n"
          + $"{rs.mYellowFlagLaps}\n"
          + $"{rs.mSafetyCarInstruction}\n"
          + $"{rs.mSafetyCarSpeed:N3}\n"
          + $"{rs.mSafetyCarMinimumSpacing:N3}\n"
          + $"{rs.mSafetyCarMaximumSpacing:N3}\n"
          + $"{rs.mMinimumColumnSpacing:N3}\n"
          + $"{rs.mMaximumColumnSpacing}\n"
          + $"{rs.mMinimumSpeed:N3}\n"
          + $"{rs.mMaximumSpeed:N3}\n"
          + $"{rs.mMessage}\n"
          + $"{rs.mFrozenOrder}\n"
          + $"{rs.mPlace}\n"
          + $"{rs.mYellowSeverity:N3}\n"
          + $"{rs.mCurrentRelativeDistance:N3}\n"
          + $"{rs.mRelativeLaps}\n"
          + $"{rs.mColumnAssignment}\n"
          + $"{rs.mPositionAssignment}\n"
          + $"{rs.mAllowedToPit}\n"
          + (rs.mGoalRelativeDistance > 20000 ? $"too large" : $"{rs.mGoalRelativeDistance:N3})") + "\n"
          + $"{rs.mMessage_Participant}\n");

        if (logToFile)
        {
          var changed = this.sbRulesChanged.ToString().Split('\n');
          var labels = this.sbRulesLabel.ToString().Split('\n');
          var values = this.sbRulesValues.ToString().Split('\n');

          var list = new List<string>(changed);
          changed = list.ToArray();

          list = new List<string>(labels);
          labels = list.ToArray();

          list = new List<string>(values);
          values = list.ToArray();

          Debug.Assert(changed.Length == labels.Length && values.Length == labels.Length);

          var lines = new List<string>();
          var updateTime = DateTime.Now.ToString();

          lines.Add($"\n{updateTime}");
          for (int i = 0; i < changed.Length; ++i)
            lines.Add($"{changed[i]}{labels[i]}{values[i]}");

          File.AppendAllLines(rulesTrackingFilePath, lines);

          lines.Clear();

          lines.Add($"\n{updateTime}");
          for (int i = 0; i < changed.Length; ++i)
          {
            if (changed[i].StartsWith("***"))
              lines.Add($"{changed[i]}{labels[i]}{values[i]}");
          }

          File.AppendAllLines(rulesTrackingDeltaFilePath, lines);
        }
      }

      var distToSC = -1.0;
      var fod = this.GetFrozenOrderData(prevFrozenOrderData, ref playerVeh, ref scoring, ref playerRules, ref rules, ref extended, ref distToSC);
      prevFrozenOrderData = fod;

      var driverToFollow = fod.DriverToFollow;
      if (fod.AssignedPosition == 1 && fod.SafetyCarSpeed > 0.0)
        driverToFollow = "Safety Car";

      this.sbFrozenOrderInfo = new StringBuilder();
      this.sbFrozenOrderInfo.Append(
        $"Frozen Order Phase: {fod.Phase}\n"
        + $"Frozen Order Action: {fod.Action}\n"
        + $"Assigned Position: {fod.AssignedPosition}\n"
        + $"Assigned Column: {fod.AssignedColumn}\n"
        + $"Assigned Grid Position: {fod.AssignedGridPosition}\n"
        + $"Driver To Follow: {driverToFollow}\n"
        + $"Safety Car Speed: {(fod.SafetyCarSpeed == -1.0f ? "Not Present" : string.Format("{0:N3}km/h", fod.SafetyCarSpeed * 3.6f))}\n"
        + $"SCR DoubleFileType: {extended.mHostedPluginVars.StockCarRules_DoubleFileType}\n"
        + $"Distance To Safety Car: {distToSC:N3}\n"
        );

      if (g != null)
      {
        float rulesY = 3.0f;
        float rulesX = 1600.0f;
        g.DrawString(this.sbRulesChanged.ToString(), SystemFonts.DefaultFont, Brushes.Orange, rulesX, rulesY);
        g.DrawString(this.sbRulesLabel.ToString(), SystemFonts.DefaultFont, Brushes.Green, rulesX + 30.0f, rulesY);
        g.DrawString(this.sbRulesValues.ToString(), SystemFonts.DefaultFont, Brushes.Purple, rulesX + 200.0f, rulesY);
        g.DrawString(this.sbFrozenOrderInfo.ToString(), SystemFonts.DefaultFont, Brushes.DarkCyan, rulesX, rulesY + 450);
      }
    }

    private FrozenOrderData GetFrozenOrderData(FrozenOrderData prevFrozenOrderData, ref rF2VehicleScoring vehicle,
      ref rF2Scoring scoring, ref rF2TrackRulesParticipant vehicleRules, ref rF2Rules rules, ref rF2Extended extended, ref double distToSC)
    {
      var fod = new FrozenOrderData();

      // Only applies to formation laps and FCY.
      if (scoring.mScoringInfo.mGamePhase != (int)rF2GamePhase.Formation
        && scoring.mScoringInfo.mGamePhase != (int)rF2GamePhase.FullCourseYellow)
        return fod;

      var foStage = rules.mTrackRules.mStage;
      if (foStage == rF2TrackRulesStage.Normal)
        return fod; // Note, there's slight race between scoring and rules here, FO messages should have validation on them.

      // rF2 currently does not expose what kind of race start is chosen.  For tracks with SC, I use presence of SC to distinguish between
      // Formation/Standing and Rolling starts.  However, if SC does not exist (Kart tracks), I used the fact that in Rolling start leader is
      // typically standing past S/F line (mLapDist is positive).  Obviously, there will be perverted tracks where that won't be true, but this
      // all I could come up with, and real problem is in game being shit in this area.
      var leaderLapDistAtFOPhaseStart = 0.0;
      var leaderSectorAtFOPhaseStart = -1;
      if (foStage != rF2TrackRulesStage.CautionInit && foStage != rF2TrackRulesStage.CautionUpdate  // If this is not FCY.
        && (prevFrozenOrderData == null || prevFrozenOrderData.Phase == FrozenOrderPhase.None))  // And, this is first FO calculation.
      {
        // Find where leader is relatively to F/S line.
        for (int i = 0; i < scoring.mScoringInfo.mNumVehicles; ++i)
        {
          var veh = scoring.mVehicles[i];
          if (veh.mPlace == 1)
          {
            leaderLapDistAtFOPhaseStart = veh.mLapDist;
            leaderSectorAtFOPhaseStart = this.GetSector(veh.mSector);
            break;
          }
        }
      }

      // Figure out the phase:
      if (foStage == rF2TrackRulesStage.CautionInit || foStage == rF2TrackRulesStage.CautionUpdate)
        fod.Phase = FrozenOrderPhase.FullCourseYellow;
      else if (foStage == rF2TrackRulesStage.FormationInit || foStage == rF2TrackRulesStage.FormationUpdate)
      {
        // Check for signs of a rolling start.
        if ((prevFrozenOrderData != null && prevFrozenOrderData.Phase == FrozenOrderPhase.Rolling)  // If FO started as Rolling, keep it as Rolling even after SC leaves the track
          || (rules.mTrackRules.mSafetyCarExists == 1 && rules.mTrackRules.mSafetyCarActive == 1)  // Of, if SC exists and is active
          || (rules.mTrackRules.mSafetyCarExists == 0 && leaderLapDistAtFOPhaseStart > 0.0 && leaderSectorAtFOPhaseStart == 1)) // Or, if SC is not present on a track, and leader started ahead of S/F line and is insector 1.  This will be problem on some tracks.
          fod.Phase = FrozenOrderPhase.Rolling;
        else
        {
          // Formation / Standing and Fast Rolling have no Safety Car.
          fod.Phase = rules.mTrackRules.mStage == rF2TrackRulesStage.FormationInit && this.GetSector(vehicle.mSector) == 3
            ? FrozenOrderPhase.FastRolling  // Fast rolling never goes into FormationUpdate and usually starts in S3.
            : FrozenOrderPhase.FormationStanding;
        }
      }

      Debug.Assert(fod.Phase != FrozenOrderPhase.None);

      if (vehicleRules.mPositionAssignment != -1)
      {
        var gridOrder = false;

        var scrLastLapDoubleFile = fod.Phase == FrozenOrderPhase.FullCourseYellow
          && extended.mHostedPluginVars.StockCarRules_IsHosted == 1
          && (extended.mHostedPluginVars.StockCarRules_DoubleFileType == 1 || extended.mHostedPluginVars.StockCarRules_DoubleFileType == 2)
          && scoring.mScoringInfo.mYellowFlagState == (sbyte)rF2YellowFlagState.LastLap;

        if (fod.Phase == FrozenOrderPhase.FullCourseYellow  // Core FCY does not use grid order. 
          && !scrLastLapDoubleFile)  // With SCR rules, however, last lap might be double file depending on DoubleFileType configuration var value.
        {
          gridOrder = false;
          fod.AssignedPosition = vehicleRules.mPositionAssignment + 1;  // + 1, because it is zero based with 0 meaning follow SC.
        }
        else  // This is not FCY, or last lap of Double File FCY with SCR plugin enabled.  The order reported is grid order, with columns specified.
        {
          gridOrder = true;
          fod.AssignedGridPosition = vehicleRules.mPositionAssignment + 1;
          fod.AssignedColumn = vehicleRules.mColumnAssignment == rF2TrackRulesColumn.LeftLane ? FrozenOrderColumn.Left : FrozenOrderColumn.Right;

          if (rules.mTrackRules.mPoleColumn == rF2TrackRulesColumn.LeftLane)
          {
            fod.AssignedPosition = (vehicleRules.mColumnAssignment == rF2TrackRulesColumn.LeftLane
              ? vehicleRules.mPositionAssignment * 2
              : vehicleRules.mPositionAssignment * 2 + 1) + 1;
          }
          else if (rules.mTrackRules.mPoleColumn == rF2TrackRulesColumn.RightLane)
          {
            fod.AssignedPosition = (vehicleRules.mColumnAssignment == rF2TrackRulesColumn.RightLane
              ? vehicleRules.mPositionAssignment * 2
              : vehicleRules.mPositionAssignment * 2 + 1) + 1;
          }

        }

        // Figure out Driver Name to follow.
        // NOTE: In Formation/Standing, game does not report those in UI, but we could.
        var vehToFollowId = -1;
        var followSC = true;
        if ((gridOrder && fod.AssignedPosition > 2)  // In grid order, first 2 vehicles are following SC.
          || (!gridOrder && fod.AssignedPosition > 1))  // In non-grid order, 1st car is following SC.
        {
          followSC = false;
          // Find the mID of a vehicle in front of us by frozen order.
          for (int i = 0; i < rules.mTrackRules.mNumParticipants; ++i)
          {
            var p = rules.mParticipants[i];
            if ((!gridOrder  // Don't care about column in non-grid order case.
                || (gridOrder && p.mColumnAssignment == vehicleRules.mColumnAssignment))  // Should be vehicle in the same column.
              && p.mPositionAssignment == (vehicleRules.mPositionAssignment - 1))
            {
              vehToFollowId = p.mID;
              break;
            }
          }
        }

        var playerDist = this.GetDistanceCompleteded(ref scoring, ref vehicle);
        var toFollowDist = -1.0;

        if (!followSC)
        {
          // Now find the vehicle to follow from the scoring info.
          for (int i = 0; i < scoring.mScoringInfo.mNumVehicles; ++i)
          {
            var v = scoring.mVehicles[i];
            if (v.mID == vehToFollowId)
            {
              fod.DriverToFollow = TransitionTracker.GetStringFromBytes(v.mDriverName);

              toFollowDist = this.GetDistanceCompleteded(ref scoring, ref v);
              break;
            }
          }
        }
        else
          toFollowDist = ((vehicle.mTotalLaps - vehicleRules.mRelativeLaps) * scoring.mScoringInfo.mLapDist) + rules.mTrackRules.mSafetyCarLapDist;

        distToSC = rules.mTrackRules.mSafetyCarActive == 1
          ? (((vehicle.mTotalLaps - vehicleRules.mRelativeLaps) * scoring.mScoringInfo.mLapDist) + rules.mTrackRules.mSafetyCarLapDist) - playerDist
          : -1.0;

        Debug.Assert(toFollowDist != -1.0);

        fod.Action = FrozenOrderAction.Follow;

        var distDelta = toFollowDist - playerDist;
        if (distDelta < 0.0)
          fod.Action = FrozenOrderAction.AllowToPass;
        else if (distDelta > 70.0)
          fod.Action = FrozenOrderAction.CatchUp;
      }

      if (rules.mTrackRules.mSafetyCarActive == 1)
        fod.SafetyCarSpeed = rules.mTrackRules.mSafetyCarSpeed;

      return fod;
    }

    private double GetDistanceCompleteded(ref rF2Scoring scoring, ref rF2VehicleScoring vehicle)
    {
      // Note: Can be interpolated a bit.
      return vehicle.mTotalLaps * scoring.mScoringInfo.mLapDist + vehicle.mLapDist;
    }
  }
}
