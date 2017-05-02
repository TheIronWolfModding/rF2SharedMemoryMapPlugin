/*
Definition of SharedMemoryMap class and related types.

Author: The Iron Wolf (vleonavicius@hotmail.com)
*/

#pragma once

#include <time.h>
#include <assert.h>
#include <stdio.h>                              // for sample output
#include <share.h>                              // _fsopen share flags

#pragma warning(push)
#pragma warning(disable : 4263)   // UpdateGraphics virtual incorrect signature
#pragma warning(disable : 4264)   // UpdateGraphics virtual incorrect signature
#pragma warning(disable : 4121)   // Alignment sensitivity (ISI sets 4 byte pack)
#pragma warning(disable : 4100)   // Unreferenced params
#include "InternalsPlugin.hpp"
#pragma warning(pop)

// Each component can be in [0:99] range.
#define PLUGIN_VERSION_MAJOR "2.0"
#define PLUGIN_VERSION_MINOR "0.0"
#define PLUGIN_NAME_AND_VERSION "rFactor 2 Shared Memory Map Plugin - v" PLUGIN_VERSION_MAJOR
#define SHARED_MEMORY_VERSION PLUGIN_VERSION_MAJOR "." PLUGIN_VERSION_MINOR

// NOTE: too much logging will kill performance.  Can be improved with buffering.
// This is hell on earth, but I do not want to add additional dependencies needed for STL right now.
// Be super careful with those, there's no type safety or checks of any kind.
#define DEBUG_MSG(lvl, msg) SharedMemoryPlugin::WriteDebugMsg(lvl, "%s : %s\n", __FUNCTION__, msg)
#define DEBUG_MSG2(lvl, msg, msg2) SharedMemoryPlugin::WriteDebugMsg(lvl, "%s : %s %s\n", __FUNCTION__, msg, msg2)
#define DEBUG_INT2(lvl, msg, intValue) SharedMemoryPlugin::WriteDebugMsg(lvl, "%s : %s %d\n", __FUNCTION__, msg, intValue)
#define DEBUG_FLOAT2(lvl, msg, floatValue) SharedMemoryPlugin::WriteDebugMsg(lvl, "%s : %s %f\n", __FUNCTION__, msg, floatValue)
#define DEBUG_MSG3(lvl, msg, msg2, msg3) SharedMemoryPlugin::WriteDebugMsg(lvl, "%s : %s %s %s\n", __FUNCTION__, msg, msg2, msg3)

#include "rF2State.h"
#include "MappedDoubleBuffer.h"

enum DebugLevel
{
  Off = 0,
  Errors = 1,
  Warnings = 2,          // Errors + Warnings
  Synchronization = 3,   // Errors + Warnings + Sync messages
  Perf = 4,              // Errors + Warnings + Sync messages + Perf
  Timing = 5,            // Errors + Warnings + Sync messages + Perf + Timing deltas
  Verbose = 6            // All
};

// This is used for the app to use the plugin for its intended purpose
class SharedMemoryPlugin : public InternalsPluginV07  // REMINDER: exported function GetPluginVersion() should return 1 if you are deriving from this InternalsPluginV01, 2 for InternalsPluginV02, etc.
{
public:
  static char const* const MM_FILE_NAME1;
  static char const* const MM_FILE_NAME2;
  static char const* const MM_FILE_ACCESS_MUTEX;
  static char const* const MM_TELEMETRY_FILE_NAME1;
  static char const* const MM_TELEMETRY_FILE_NAME2;
  static char const* const MM_TELEMETRY_FILE_ACCESS_MUTEX;
  static char const* const CONFIG_FILE_REL_PATH;
  static char const* const INTERNALS_TELEMETRY_FILENAME;
  static char const* const INTERNALS_SCORING_FILENAME;
  static char const* const DEBUG_OUTPUT_FILENAME;
  
  static int const MAX_ASYNC_RETRIES = 3;
  static int const MAX_PARTICIPANT_SLOTS = 256;
  static int const BUFFER_IO_BYTES = 2048;
  static int const DEBUG_IO_FLUSH_PERIOD_SECS = 10;

  static DebugLevel msDebugOutputLevel;
  static bool msDebugISIInternals;
  static int msMillisRefresh;
  static DWORD msMillisMutexWait;

  // Ouptut files:
  static FILE* msDebugFile;
  static FILE* msIsiTelemetryFile;
  static FILE* msIsiScoringFile;


  static void LoadConfig();

  // Debug output helpers
  static void WriteDebugMsg(DebugLevel lvl, char const* const format, ...);
  static void WriteToAllExampleOutputFiles(char const* const openStr, char const* const msg);
  static void WriteTelemetryInternals(TelemInfoV01 const& info);
  static void WriteScoringInternals(ScoringInfoV01 const& info);

private:
  // internal state tracking
  struct InternalVehScoringInfo
  {
    double mLapDist;
    rF2Vec3 mPos;
    rF2Vec3 mLocalVel;
    rF2Vec3 mLocalAccel;
    rF2Quat mOriQuatBegin;  // Orientation quat at scoring update
    rF2Quat mOriQuatEnd;    // Estimated orientation quat one second later
    rF2Vec3 mLocalVelEnd;   // Estimated local speed one second later
  };

  struct InternalScoringInfo
  {
    int mNumVehicles;
    char mPlrFileName[64];
    InternalVehScoringInfo mVehicles[rF2State::MAX_VSI_SIZE];
  };

  struct ExtendedStateTracker
  {
    unsigned char mInvulnerable = 0;
    double mMaxImpactMagnitude = 0.0;
    double mAccumulatedImpactMagnitude = 0.0;
    
    double mLastImpactProcessedET = 0.0;
    double mLastPitStopET = 0.0;

    void ProcessTelemetryUpdate(TelemInfoV01 const& info)
    {
      if (info.mLastImpactET > mLastPitStopET  // Is this new impact since last pit stop?
        && info.mLastImpactET > mLastImpactProcessedET) { // Is this new impact?
        // Ok, this is either new impact, or first impact since pit stop.
        // Update max and accumulated impact magnitudes.
        mMaxImpactMagnitude = max(mMaxImpactMagnitude, info.mLastImpactMagnitude);
        mAccumulatedImpactMagnitude += info.mLastImpactMagnitude;
        mLastImpactProcessedET = info.mLastImpactET;
      }
    }

    void ProcessScoringUpdate(ScoringInfoV01 const& info)
    {
      // This code bravely assumes that car at 0 is player.
      // Not sure how correct this is.
      if (info.mNumVehicles > 0 
        && info.mVehicle[0].mPitState == static_cast<unsigned char>(rF2PitState::Stopped)) {
        ResetDamageState();
        mLastPitStopET = info.mCurrentET;
      }
    }

    void ProcessPhysicsOptions(PhysicsOptionsV01& options)
    {
      mInvulnerable = options.mInvulnerable;
    }

    void ResetDamageState()
    {
      mMaxImpactMagnitude = 0.0;
      mAccumulatedImpactMagnitude = 0.0;
      mLastImpactProcessedET = 0.0;
      mLastPitStopET = 0.0;
    }

    void FlushToBuffer(rF2State* pBuf) const
    {
      assert(pBuf != nullptr);
      assert(pBuf->mCurrentRead != true);

      pBuf->mMaxImpactMagnitude = mMaxImpactMagnitude;
      pBuf->mAccumulatedImpactMagnitude = mAccumulatedImpactMagnitude;
      pBuf->mInvulnerable = mInvulnerable;
    }
  };

public:
  SharedMemoryPlugin();
  ~SharedMemoryPlugin() override {}

  ////////////////////////////////////////////////////
  // InternalsPluginV01 (InternalsPlugin)
  ///////////////////////////////////////////////////
  void Startup(long version) override;    // game startup
  void Shutdown() override;               // game shutdown

  void EnterRealtime() override;          // entering realtime (where the vehicle can be driven)
  void ExitRealtime() override;           // exiting realtime

  void StartSession() override;           // session has started
  void EndSession() override;             // session has ended

  // GAME OUTPUT
  long WantsTelemetryUpdates() override { return 2L; } // whether we want telemetry updates (0=no 1=player-only 2=all vehicles)
  void UpdateTelemetry(TelemInfoV01 const& info) override;

  // SCORING OUTPUT
  bool WantsScoringUpdates() override { return true; }
  void UpdateScoring(ScoringInfoV01 const& info) override; // update plugin with scoring info (approximately five times per second)

  // EXP: MESSAGE BOX INPUT
  bool WantsToDisplayMessage(MessageInfoV01& msgInfo) override; // set message and return true

  // EXP: ADDITIONAL GAMEFLOW NOTIFICATIONS
  void ThreadStarted(long type) override; // called just after a primary thread is started (type is 0=multimedia or 1=simulation)
  void ThreadStopping(long type) override;  // called just before a primary thread is stopped (type is 0=multimedia or 1=simulation)

  // EXP: 
  bool WantsTrackRulesAccess() override { return(true); } // change to true in order to read or write track order (during formation or caution laps)
  bool AccessTrackRules(TrackRulesV01& info) override; // current track order passed in; return true if you want to change it (note: this will be called immediately after UpdateScoring() when appropriate)

  // EXP: 
  // PIT MENU INFO (currently, the only way to edit the pit menu is to use this in conjunction with CheckHWControl())
  bool WantsPitMenuAccess() { return(true); } // change to true in order to view pit menu info
  bool AccessPitMenu(PitMenuV01& info) override; // currently, the return code should always be false (because we may allow more direct editing in the future)

  void SetPhysicsOptions(PhysicsOptionsV01& options) override 
  {
    mExtStateTracker.ProcessPhysicsOptions(options);
  }

  // FUTURE/V2: SCORING CONTROL (only available in single-player or on multiplayer server)
  // virtual bool WantsMultiSessionRulesAccess() { return(false); } // change to true in order to read or write multi-session rules
  // virtual bool AccessMultiSessionRules(MultiSessionRulesV01 &info) { return(false); } // current internal rules passed in; return true if you want to change them

private:
  void UpdateInRealtimeFC(bool inRealTime);
  void UpdateTelemetryHelper(double const ticksNow, TelemInfoV01 const& info);
  void UpdateScoringHelper(double const ticksNow, ScoringInfoV01 const& info);
  void SyncBuffers(bool telemetryOnly);

  void ClearState();
  void ClearTimingsAndCounters();
  void TraceSkipTelemetryUpdate(TelemInfoV01 const& info) const;
  void TraceBeginTelemetryUpdate();
  void TraceEndTelemetryUpdate(int numVehiclesInChain) const;

private:

  // Only used for debugging in Timing level
  double mLastTelUpdate = 0.0;
  double mLastScoringUpdate = 0.0;

  // Frame delta is in seconds.
  // TODO: Not sure we need this.
  double mDelta = 0.0;

  // Extended state tracker
  ExtendedStateTracker mExtStateTracker;

  double mLastTelemetryUpdateET = 0.0;
  double mLastScoringUpdateET = 0.0;

  bool mTelemetryUpdateInProgress = false;
  int mCurTelemetryVehicleIndex = 0;
  int mScoringNumVehicles = 0;

  bool mIsMapped = false;
  bool mInRealTimeLastFunctionCall = false;
  InternalScoringInfo mScoringInfo = {};

  bool mRetryFlip = false;
  int mRetriesLeft = 0;

  bool mParticipantTelemetryUpdated[MAX_PARTICIPANT_SLOTS];

  MappedDoubleBuffer<rF2Telemetry> mTelemetry;
};


