/*
Definition of SharedMemoryMap class and related types.

Author: The Iron Wolf (vleonavicius@hotmail.com)
Website: thecrewchief.org
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

#ifdef _AMD64_
#define PLUGIN_64BIT true
#else
#define PLUGIN_64BIT false
#endif

// Each component can be in [0:99] range.
// Note: each time major version changes, that means layout has changed, and clients might need an update.
#define PLUGIN_VERSION_MAJOR "3.7"
#define PLUGIN_VERSION_MINOR "15.1"

#ifdef VERSION_AVX2
#ifdef VERSION_MT
#define PLUGIN_NAME_AND_VERSION "rFactor 2 Shared Memory Map Plugin - v" PLUGIN_VERSION_MAJOR " AVX2+PGO+MT"
#else
#define PLUGIN_NAME_AND_VERSION "rFactor 2 Shared Memory Map Plugin - v" PLUGIN_VERSION_MAJOR " AVX2+PGO"
#endif
#else
#define PLUGIN_NAME_AND_VERSION "rFactor 2 Shared Memory Map Plugin - v" PLUGIN_VERSION_MAJOR
#endif

#define SHARED_MEMORY_VERSION PLUGIN_VERSION_MAJOR "." PLUGIN_VERSION_MINOR

#define DEBUG_MSG(lvl, src, msg, ...) SharedMemoryPlugin::WriteDebugMsg(lvl, src, __FUNCTION__, __LINE__, msg, __VA_ARGS__)
#define RETURN_IF_FALSE(expression) if (!expression) { DEBUG_MSG(DebugLevel::Errors, DebugSource::General, "Operation failed"); return; }

#include "rF2State.h"
#include "MappedBuffer.h"
#include "DirectMemoryReader.h"

enum class DebugLevel : long
{
  Off = 0,
  Errors = 1,
  CriticalInfo = 2,
  DevInfo = 4,
  Warnings = 8,
  Synchronization = 16,
  Perf = 32,
  Timing = 64,
  Verbose = 128,
  All = 255,
};

enum DebugSource : long
{
  General = 1,  // CriticalInfo, Error and Warning level messages go there as well as some core messages.
  DMR = 2,
  MappedBufferSource = 4,
  Telemetry = 4,
  Scoring = 8,
  Rules = 16,
  MultiRules = 32,
  ForceFeedback = 64,
  Graphics = 128,
  Weather = 256,
  Extended = 512,
  HWControlInput = 1024,
  WeatherControlInput = 2048,
  RulesControlInput = 4096,
  PluginControlInput = 8192,
  PitInfo = 16384,
  All = 32767,
};

enum class SubscribedBuffer : long
{
  Telemetry = 1,
  Scoring = 2,
  Rules = 4,
  MultiRules = 8,
  ForceFeedback = 16,
  Graphics = 32,
  PitInfo = 64,
  Weather = 128,
  All = 255
};

double TicksNow();

static double const MILLISECONDS_IN_SECOND = 1000.0;
static double const MICROSECONDS_IN_MILLISECOND = 1000.0;
static double const MICROSECONDS_IN_SECOND = MILLISECONDS_IN_SECOND * MICROSECONDS_IN_MILLISECOND;

// This is used for the app to use the plugin for its intended purpose
class SharedMemoryPlugin : public InternalsPluginV07  // REMINDER: exported function GetPluginVersion() should return 1 if you are deriving from this InternalsPluginV01, 2 for InternalsPluginV02, etc.
{
public:
  static char const* const MM_TELEMETRY_FILE_NAME;
  static char const* const MM_SCORING_FILE_NAME;
  static char const* const MM_RULES_FILE_NAME;
  static char const* const MM_MULTI_RULES_FILE_NAME;
  static char const* const MM_FORCE_FEEDBACK_FILE_NAME;
  static char const* const MM_GRAPHICS_FILE_NAME;
  static char const* const MM_EXTENDED_FILE_NAME;
  static char const* const MM_PIT_INFO_FILE_NAME;
  static char const* const MM_WEATHER_FILE_NAME;

  // Input buffers:
  static char const* const MM_HWCONTROL_FILE_NAME;
  static char const* const MM_WEATHER_CONTROL_FILE_NAME;
  static char const* const MM_RULES_CONTROL_FILE_NAME;
  static char const* const MM_PLUGIN_CONTROL_FILE_NAME;

  static char const* const INTERNALS_TELEMETRY_FILENAME;
  static char const* const INTERNALS_SCORING_FILENAME;
  static char const* const DEBUG_OUTPUT_FILENAME;

  static int const BUFFER_IO_BYTES = 2048;
  static int const DEBUG_IO_FLUSH_PERIOD_SECS = 10;

  static long msDebugOutputLevel;
  static long msDebugOutputSource;
  static bool msDebugISIInternals;
  static bool msDedicatedServerMapGlobally;
  static bool msDirectMemoryAccessRequested;
  static long msUnsubscribedBuffersMask;
  static bool msHWControlInputRequested;
  static bool msWeatherControlInputRequested;
  static bool msRulesControlInputRequested;

  // Ouptut files:
  static FILE* msDebugFile;
  static FILE* msIsiTelemetryFile;
  static FILE* msIsiScoringFile;

  // Debug output helpers
  static void WriteDebugMsg(
    DebugLevel lvl,
    long src,
    char const* const functionName,
    int line,
    char const* const msg,
    ...);

  static void WriteToAllExampleOutputFiles(char const* const openStr, char const* const msg);
  static void WriteTelemetryInternals(TelemInfoV01 const& info);
  static void WriteScoringInternals(ScoringInfoV01 const& info);
  static void TraceLastWin32Error();

private:
  class ExtendedStateTracker
  {
  public:
    ExtendedStateTracker()
    {
      // There's a bug somewhere (in my head?), initializing mExtended = {} does not make it all 0.
      // Maybe there's a race between simulation and multimedia threads, but I can't debug due to game crashing on attach.
      // Traces suggest no race however.
      memset(&mExtended, 0, sizeof(rF2Extended));

      static_assert(sizeof(mExtended.mVersion) >= sizeof(SHARED_MEMORY_VERSION), "Invalid plugin version string (too long).");

      strcpy_s(mExtended.mVersion, SHARED_MEMORY_VERSION);
      mExtended.is64bit = PLUGIN_64BIT;
      mExtended.mSCRPluginDoubleFileType = -1L;

      assert(!mExtended.mMultimediaThreadStarted);
      assert(!mExtended.mSimulationThreadStarted);
    }

    void ProcessTelemetryUpdate(TelemInfoV01 const& info)
    {
      auto const id = max(info.mID, 0L) % rF2Extended::MAX_MAPPED_IDS;

      auto& dti = mDamageTrackingInfos[id];
      if (info.mLastImpactET > dti.mLastPitStopET  // Is this new impact since last pit stop?
        && info.mLastImpactET > dti.mLastImpactProcessedET) { // Is this new impact?
        // Ok, this is either new impact, or first impact since pit stop.
        // Update max and accumulated impact magnitudes.
        auto& td = mExtended.mTrackedDamages[id];
        td.mMaxImpactMagnitude = max(td.mMaxImpactMagnitude, info.mLastImpactMagnitude);
        td.mAccumulatedImpactMagnitude += info.mLastImpactMagnitude;

        dti.mLastImpactProcessedET = info.mLastImpactET;
      }
    }

    void ProcessScoringUpdate(ScoringInfoV01 const& info)
    {
      for (int i = 0; i < info.mNumVehicles; ++i) {
        if (info.mVehicle[i].mPitState == static_cast<unsigned char>(rF2PitState::Stopped)) {
          // If this car is pitting, clear out any damage tracked.
          auto const id = max(info.mVehicle[i].mID, 0L) % rF2Extended::MAX_MAPPED_IDS;

          memset(&(mExtended.mTrackedDamages[id]), 0, sizeof(rF2TrackedDamage));

          mDamageTrackingInfos[id].mLastImpactProcessedET = 0.0;
          mDamageTrackingInfos[id].mLastPitStopET = info.mCurrentET;
        }
      }
    }

    void CaptureSessionTransition(rF2Scoring const& scoring)
    {
      // Capture the interesting session end state.
      mExtended.mSessionTransitionCapture.mGamePhase = scoring.mScoringInfo.mGamePhase;
      mExtended.mSessionTransitionCapture.mSession = scoring.mScoringInfo.mSession;

      auto const numScoringVehicles = min(scoring.mScoringInfo.mNumVehicles, rF2MappedBufferHeader::MAX_MAPPED_VEHICLES);
      mExtended.mSessionTransitionCapture.mNumScoringVehicles = numScoringVehicles;

      for (int i = 0; i < numScoringVehicles; ++i) {
        auto& sessEndVeh = mExtended.mSessionTransitionCapture.mScoringVehicles[i];
        auto const& sv = scoring.mVehicles[i];

        sessEndVeh.mID = sv.mID;
        sessEndVeh.mFinishStatus = sv.mFinishStatus;
        sessEndVeh.mIsPlayer = sv.mIsPlayer;
        sessEndVeh.mPlace = sv.mPlace;
      }
    }

    void ClearState()
    {
      ResetDamageState();
    }

    void ResetDamageState()
    {
      memset(&(mExtended.mTrackedDamages), 0, sizeof(mExtended.mTrackedDamages));
      memset(&mDamageTrackingInfos, 0, sizeof(mDamageTrackingInfos));
    }

  public:
    rF2Extended mExtended = {};

  private:
    struct DamageTracking
    {
      double mLastImpactProcessedET = 0.0;
      double mLastPitStopET = 0.0;
    };

    DamageTracking mDamageTrackingInfos[rF2Extended::MAX_MAPPED_IDS];
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
  long WantsTelemetryUpdates() override { return Utils::IsFlagOff(SharedMemoryPlugin::msUnsubscribedBuffersMask, SubscribedBuffer::Telemetry) ? 2L : 0L; } // whether we want telemetry updates (0=no 1=player-only 2=all vehicles)
  void UpdateTelemetry(TelemInfoV01 const& info) override;

  // SCORING OUTPUT
  bool WantsScoringUpdates() override { return Utils::IsFlagOff(SharedMemoryPlugin::msUnsubscribedBuffersMask, SubscribedBuffer::Scoring); }
  void UpdateScoring(ScoringInfoV01 const& info) override; // update plugin with scoring info (approximately five times per second)

  bool ForceFeedback(double& forceValue) override; // alternate force feedback computation - return true if editing the value

  // ADDITIONAL GAMEFLOW NOTIFICATIONS
  void ThreadStarted(long type) override; // called just after a primary thread is started (type is 0=multimedia or 1=simulation)
  void ThreadStopping(long type) override;  // called just before a primary thread is stopped (type is 0=multimedia or 1=simulation)

  bool WantsTrackRulesAccess() override { return Utils::IsFlagOff(SharedMemoryPlugin::msUnsubscribedBuffersMask, SubscribedBuffer::Rules); } // change to true in order to read or write track order (during formation or caution laps)
  bool AccessTrackRules(TrackRulesV01& info) override; // current track order passed in; return true if you want to change it (note: this will be called immediately after UpdateScoring() when appropriate)

  void SetPhysicsOptions(PhysicsOptionsV01& options) override;

  // SCORING CONTROL (only available in single-player or on multiplayer server)
  bool WantsMultiSessionRulesAccess() override { return Utils::IsFlagOff(SharedMemoryPlugin::msUnsubscribedBuffersMask, SubscribedBuffer::MultiRules); } // change to true in order to read or write multi-session rules
  bool AccessMultiSessionRules(MultiSessionRulesV01& info); // current internal rules passed in; return true if you want to change them

  // CUSTOM PLUGIN VARIABLES
  // This relatively simple feature allows plugins to store settings in a shared location without doing their own
  // file I/O. Direct UI support may also be added in the future so that end users can control plugin settings within
  // rFactor. But for now, users can access the data in UserData\Player\CustomPluginOptions.JSON.
  // Plugins should only access these variables through this interface, though:
  bool GetCustomVariable(long i, CustomVariableV01& var) override; // At startup, this will be called with increasing index (starting at zero) until false is returned. Feel free to add/remove/rearrange the variables when updating your plugin; the index does not have to be consistent from run to run.
  void AccessCustomVariable(CustomVariableV01& var) override;      // This will be called at startup, shutdown, and any time that the variable is changed (within the UI).
  void GetCustomVariableSetting(CustomVariableV01& var, long i, CustomSettingV01& setting) override; // This gets the name of each possible setting for a given variable.

  // GRAPHICS
  bool WantsGraphicsUpdates() override { return Utils::IsFlagOff(SharedMemoryPlugin::msUnsubscribedBuffersMask, SubscribedBuffer::Graphics); }       // whether we want graphics updates
  void UpdateGraphics(GraphicsInfoV02 const& info) override;  // update plugin with graphics info

  // Supress C4266.
  void UpdateGraphics(GraphicsInfoV01 const&) override {}     // update plugin with graphics info

  // PIT MENU INFO (currently, the only way to edit the pit menu is to use this in conjunction with CheckHWControl())
  bool WantsPitMenuAccess()  override { return Utils::IsFlagOff(SharedMemoryPlugin::msUnsubscribedBuffersMask, SubscribedBuffer::PitInfo); } // change to true in order to view pit menu info
  bool AccessPitMenu(PitMenuV01& info) override; // currently, the return code should always be false (because we may allow more direct editing in the future)

  // HW Control- action a control within the game
  bool HasHardwareInputs() override
  {
    return SharedMemoryPlugin::msHWControlInputRequested
      && mExtStateTracker.mExtended.mHWControlInputEnabled
      && SharedMemoryPlugin::mHWControlInputRequestReceived;
  }

  bool CheckHWControl(char const* const controlName, double& fRetVal) override;

  // CONDITIONS CONTROL
  bool WantsWeatherAccess() override { return Utils::IsFlagOff(SharedMemoryPlugin::msUnsubscribedBuffersMask, SubscribedBuffer::Weather); } // change to true in order to read or write weather with AccessWeather() call:
  bool AccessWeather(double trackNodeSize, WeatherControlInfoV01& info) override;

private:
  SharedMemoryPlugin(SharedMemoryPlugin const& rhs) = delete;
  SharedMemoryPlugin& operator =(SharedMemoryPlugin const& rhs) = delete;

  template <typename BuffT>
  bool InitMappedBuffer(BuffT& buffer, char const* const buffLogicalName, SubscribedBuffer sb);

  template <typename BuffT>
  bool InitMappedInputBuffer(BuffT& buffer, char const* const buffLogicalName);

  void UpdateInRealtimeFC(bool inRealTime);
  void UpdateThreadState(long type, bool starting);
  void ClearState();
  void ClearTimingsAndCounters();

  void TelemetryTraceSkipUpdate(TelemInfoV01 const& info, double deltaET);
  void TelemetryTraceBeginUpdate(double telUpdateET, double deltaET);
  void TelemetryTraceVehicleAdded(TelemInfoV01 const& infos);
  void TelemetryTraceEndUpdate(int numVehiclesInChain);
  void TelemetryBeginNewFrame(TelemInfoV01 const& info, double deltaET);
  void TelemetryCompleteFrame();

  void ScoringTraceBeginUpdate();
  void ReadDMROnScoringUpdate(ScoringInfoV01 const& info);
  void ReadHWControl();
  void ReadWeatherControl();
  void ReadRulesControl();
  void DynamicallySubscribeToBuffer(SubscribedBuffer sb, long requestedBuffMask, const char* const buffLogicalName);
  void DynamicallyEnableInputBuffer(bool dependencyMissing, bool& controlInputRequested, bool& controlIputEnabled, char const* const buffLogicalName);
  void ReadPluginControl();
  bool IsHWControlInputDependencyMissing();
  bool IsWeatherControlInputDependencyMissing();
  bool IsRulesControlInputDependencyMissing();

  template <typename BuffT>
  void TraceBeginUpdate(BuffT const& buffer, double& lastUpdateMillis, char const msgPrefix[]) const;

private:
  // Only used for debugging in Timing level
  double mLastTelemetryUpdateMillis = 0.0;
  double mLastTelemetryVehicleAddedMillis = 0.0;
  double mLastScoringUpdateMillis = 0.0;
  double mLastRulesUpdateMillis = 0.0;
  double mLastMultiRulesUpdateMillis = 0.0;

  ExtendedStateTracker mExtStateTracker;

  // Elapsed times reported by the game.
  double mLastTelemetryUpdateET = -1.0;

  // For telemetry, this is min mElapsedTime in the telemetry update frame.
  double mLastScoringUpdateET = -1.0;
  // Telemetry update tracking variables:
  bool mTelemetryFrameCompleted = true;
  bool mTelemetrySkipFrameReported = false;
  int mCurrTelemetryVehicleIndex = 0;
  // Array used to track if mID telemetry is captured for this update.
  // Indexed by mID % rF2MappedBufferHeader::MAX_MAPPED_IDS, so will break down if max(mID) - min(mID) in a frame >= rF2MappedBufferHeader::MAX_MAPPED_IDS
  // If this becomes a problem (people complain), different mechanism will be necessary.
  // One way to handle this is to take first mID in a telemetry frame, and use it as a starting offset.
  // This might be fine, because game appears to be sending mIDs in an ascending order.
  bool mParticipantTelemetryUpdated[rF2Extended::MAX_MAPPED_IDS];

  // Pit menu update trackers.
  long mPitMenuLastCategoryIndex = -1L;
  long mPitMenuLastChoiceIndex = -1L;
  long mPitMenuLastNumChoices = -1L;

  // Input buffer logic members:
  // Read attempt counter, used to skip reads.
  int mHWControlRequestReadCounter = 0;
  // Boost counter, boost read rate after buffer update.
  int mHWControlRequestBoostCounter = 0;

  bool mHWControlInputRequestReceived = false;
  bool mWeatherControlInputRequestReceived = false;
  bool mRulesControlInputRequestReceived = false;

  MappedBuffer<rF2Telemetry> mTelemetry;
  MappedBuffer<rF2Scoring> mScoring;
  MappedBuffer<rF2Rules> mRules;
  MappedBuffer<rF2MultiRules> mMultiRules;
  MappedBuffer<rF2ForceFeedback> mForceFeedback;
  MappedBuffer<rF2Graphics> mGraphics;
  MappedBuffer<rF2Extended> mExtended;
  MappedBuffer<rF2PitInfo> mPitInfo;
  MappedBuffer<rF2Weather> mWeather;

  // Input buffers:
  MappedBuffer<rF2HWControl> mHWControl;
  MappedBuffer<rF2WeatherControl> mWeatherControl;
  MappedBuffer<rF2RulesControl> mRulesControl;
  MappedBuffer<rF2PluginControl> mPluginControl;

  // All buffers mapped successfully or not.
  bool mIsMapped = false;

  //////////////////////////////////////////
  // Direct Memory Access hackery
  //////////////////////////////////////////
  DirectMemoryReader mDMR;
  bool mLastUpdateLSIWasVisible = false;
};
