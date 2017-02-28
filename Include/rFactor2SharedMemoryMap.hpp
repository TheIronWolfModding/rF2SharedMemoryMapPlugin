/*
Definition of SharedMemoryMap class and related types.

Author: The Iron Wolf (vleonavicius@hotmail.com)
*/

#pragma once

#pragma warning(push)
#pragma warning(disable : 4263)   // UpdateGraphics virtual incorrect signature
#pragma warning(disable : 4264)   // UpdateGraphics virtual incorrect signature
#pragma warning(disable : 4121)   // Alignment sensitivity (ISI sets 4 byte pack)
#pragma warning(disable : 4100)   // Unreferenced params
#include "InternalsPlugin.hpp"
#pragma warning(pop)

#include "rF2State.h"
#include <time.h>
#include <assert.h>

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
  static char const* const CONFIG_FILE_REL_PATH;
  static char const* const INTERNALS_TELEMETRY_FILENAME;
  static char const* const INTERNALS_SCORING_FILENAME;
  static char const* const DEBUG_OUTPUT_FILENAME;
  static int const MAX_ASYNC_RETRIES;

  static DebugLevel msDebugOutputLevel;
  static bool msDebugISIInternals;
  static int msMillisRefresh;
  static DWORD msMillisMutexWait;

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

    void ProcessPhysicsOptions(PhysicsOptionsV01 &options)
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
  SharedMemoryPlugin() {}
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
  long WantsTelemetryUpdates() override { return 1L; }
  // whether we want telemetry updates (0=no 1=player-only 2=all vehicles)
  void UpdateTelemetry(TelemInfoV01 const& info) override;

  // SCORING OUTPUT
  bool WantsScoringUpdates() override { return true; }
  void UpdateScoring(ScoringInfoV01 const& info) override; // update plugin with scoring info (approximately five times per second)


  void SetPhysicsOptions(PhysicsOptionsV01 &options) override 
  {
    mExtStateTracker.ProcessPhysicsOptions(options);
  }


private:
  void UpdateInRealtimeFC(bool inRealTime);
  void UpdateTelemetryHelper(double const ticksNow, TelemInfoV01 const& info);
  void UpdateScoringHelper(double const ticksNow, ScoringInfoV01 const& info);
  void SyncBuffers(bool telemetryOnly);
  void FlipBuffersHelper();
  void FlipBuffers();
  void TryFlipBuffers();
  
  HANDLE MapMemoryFile(char const * const fileName, rF2State*& pBuf) const;
  void ClearState();
  void ClearTimings();

  // Timings are in microseconds.
  double mLastTelUpdate = 0.0;
  double mLastScoringUpdate = 0.0;

  // Frame delta is in seconds.
  double mDelta = 0.0;

  // Extended state tracker
  ExtendedStateTracker mExtStateTracker;

  HANDLE mhMutex = nullptr;
  HANDLE mhMap1 = nullptr;
  HANDLE mhMap2 = nullptr;

  // Current write buffer.
  rF2State* mpBufCurWrite = nullptr;

  // Flip between 2 buffers.  Clients should read the one with mCurrentRead == true.
  rF2State* mpBuf1 = nullptr;
  rF2State* mpBuf2 = nullptr;

  bool mIsMapped = false;
  InternalScoringInfo mScoringInfo = {};

  bool mRetryFlip = false;
  int mRetriesLeft = 0;
};
