/*
Definition of SharedMemoryMap class and related types.

Author: The Iron Wolf (vleonavicius@hotmail.com)
Website: thecrewchief.org
*/
#pragma once

#include <time.h>
#include <assert.h>
#include <stdio.h> // for sample output
#include <share.h> // _fsopen share flags

#pragma warning(push)
#pragma warning(disable : 4263) // UpdateGraphics virtual incorrect signature
#pragma warning(disable : 4264) // UpdateGraphics virtual incorrect signature
#pragma warning(disable : 4121) // Alignment sensitivity (ISI sets 4 byte pack)
#pragma warning(disable : 4100) // Unreferenced params
#include "InternalsPlugin.hpp"
#include "RFPluginObjects.hpp"
#pragma warning(pop)

#ifdef _AMD64_
#define PLUGIN_64BIT true
#else
#define PLUGIN_64BIT false
#endif

// Each component can be in [0:99] range.
// Note: each time major version changes, that means layout has changed, and clients might need an update.
#define PLUGIN_VERSION_MAJOR "1.0"
#define PLUGIN_VERSION_MINOR "0.0"

#ifdef VERSION_AVX2
#ifdef VERSION_MT
#define PLUGIN_NAME_AND_VERSION "GTR2 Crew Chief Shared Memory Map Plugin - v" PLUGIN_VERSION_MAJOR " AVX2+PGO+MT"
#else
#define PLUGIN_NAME_AND_VERSION "GTR2 Crew Chief Shared Memory Map Plugin - v" PLUGIN_VERSION_MAJOR " AVX2+PGO"
#endif
#else
#define PLUGIN_NAME_AND_VERSION "GTR2 Crew Chief Shared Memory Map Plugin - v" PLUGIN_VERSION_MAJOR
#endif

#define SHARED_MEMORY_VERSION PLUGIN_VERSION_MAJOR "." PLUGIN_VERSION_MINOR

#define DEBUG_MSG(lvl, src, msg, ...) CrewChiefPlugin::WriteDebugMsg(lvl, src, __FUNCTION__, __LINE__, msg, __VA_ARGS__)
#define RETURN_IF_FALSE(expression)                                                                                    \
  if (!expression) {                                                                                                   \
    DEBUG_MSG(DebugLevel::Errors, DebugSource::General, "Operation failed");                                           \
    return;                                                                                                            \
  }

#include "GTR2State.h"
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
  General = 1, // CriticalInfo, Error and Warning level messages go there as well as some core messages.
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

float
TicksNow();

static float const MILLISECONDS_IN_SECOND = 1000.0f;
static float const MICROSECONDS_IN_MILLISECOND = 1000.0f;
static float const MICROSECONDS_IN_SECOND = MILLISECONDS_IN_SECOND * MICROSECONDS_IN_MILLISECOND;

// This is used for app to find out information about the plugin
class CrewChiefPluginInfo : public PluginObjectInfo
{
public:
  // Constructor/destructor
  CrewChiefPluginInfo() {}
  ~CrewChiefPluginInfo() {}

  // Derived from base class PluginObjectInfo
  char const* GetName() const override;
  char const* GetFullName() const override;
  char const* GetDesc() const override;
  unsigned const GetType() const override;
  char const* GetSubType() const override;
  unsigned const GetVersion() const override;
  void* Create() const override;
};

// This is used for the app to use the plugin for its intended purpose
class CrewChiefPlugin
  : public InternalsPluginV3 // REMINDER: exported function GetPluginVersion() should return 1 if you are deriving from
                             // this InternalsPluginV01, 2 for InternalsPluginV02, etc.
{
public:
  static char constexpr* const MM_TELEMETRY_FILE_NAME = "$GTR2CrewChief_Telemetry$";
  static char constexpr* const MM_SCORING_FILE_NAME = "$GTR2CrewChief_Scoring$";
  static char constexpr* const MM_RULES_FILE_NAME = "$GTR2CrewChief_Rules$";
  static char constexpr* const MM_MULTI_RULES_FILE_NAME = "$GTR2CrewChief_MultiRules$";
  static char constexpr* const MM_FORCE_FEEDBACK_FILE_NAME = "$GTR2CrewChief_ForceFeedback$";
  static char constexpr* const MM_GRAPHICS_FILE_NAME = "$GTR2CrewChief_Graphics$";
  static char constexpr* const MM_EXTENDED_FILE_NAME = "$GTR2CrewChief_Extended$";
  static char constexpr* const MM_PIT_INFO_FILE_NAME = "$GTR2CrewChief_PitInfo$";
  static char constexpr* const MM_WEATHER_FILE_NAME = "$GTR2CrewChief_Weather$";

  static char constexpr* const MM_HWCONTROL_FILE_NAME = "$GTR2CrewChief_HWControl$";
  static char constexpr* const MM_WEATHER_CONTROL_FILE_NAME = "$GTR2CrewChief_WeatherControl$";
  static char constexpr* const MM_RULES_CONTROL_FILE_NAME = "$GTR2CrewChief_RulesControl$";
  static char constexpr* const MM_PLUGIN_CONTROL_FILE_NAME = "$GTR2CrewChief_PluginControl$";

  static char constexpr* const DEBUG_OUTPUT_FILENAME = R"(UserData\Log\CC_DebugOutput.txt)";

  static int const BUFFER_IO_BYTES = 2048;
  static int const DEBUG_IO_FLUSH_PERIOD_SECS = 10;

  static long msDebugOutputLevel;
  static long msDebugOutputSource;
  static long msChangeProcessAffinity;
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
  static void WriteDebugMsg(DebugLevel lvl,
                            long src,
                            char const* const functionName,
                            int line,
                            char const* const msg,
                            ...);

  static void TraceLastWin32Error();

private:
  static char constexpr* const CONFIG_FILE_REL_PATH = R"(\Plugins\CrewChief.ini)";

  class ExtendedStateTracker
  {
  public:
    ExtendedStateTracker()
    {
      // There's a bug somewhere (in my head?), initializing mExtended = {} does not make it all 0.
      // Maybe there's a race between simulation and multimedia threads, but I can't debug due to game crashing on
      // attach. Traces suggest no race however.
      memset(&mExtended, 0, sizeof(GTR2Extended));

      static_assert(sizeof(mExtended.mVersion) >= sizeof(SHARED_MEMORY_VERSION),
                    "Invalid plugin version string (too long).");

      strcpy_s(mExtended.mVersion, SHARED_MEMORY_VERSION);
      mExtended.is64bit = PLUGIN_64BIT;
      mExtended.mSCRPluginDoubleFileType = -1L;

      assert(!mExtended.mMultimediaThreadStarted);
      assert(!mExtended.mSimulationThreadStarted);
    }

    void ProcessTelemetryUpdate(TelemInfoV2 const& info)
    {
      /*auto const id = max(info.mID, 0L) % GTR2Extended::MAX_MAPPED_IDS;

      auto& dti = mDamageTrackingInfos[id];
      if (info.mLastImpactET > dti.mLastPitStopET  // Is this new impact since last pit stop?
        && info.mLastImpactET > dti.mLastImpactProcessedET) { // Is this new impact?
        // Ok, this is either new impact, or first impact since pit stop.
        // Update max and accumulated impact magnitudes.
        auto& td = mExtended.mTrackedDamages[id];
        td.mMaxImpactMagnitude = max(td.mMaxImpactMagnitude, info.mLastImpactMagnitude);
        td.mAccumulatedImpactMagnitude += info.mLastImpactMagnitude;

        dti.mLastImpactProcessedET = info.mLastImpactET;
      }*/
    }

    void ProcessScoringUpdate(ScoringInfoV2 const& info)
    { /*
       for (int i = 0; i < info.mNumVehicles; ++i) {
         if (info.mVehicle[i].mPitState == static_cast<unsigned char>(rF2PitState::Stopped)) {
           // If this car is pitting, clear out any damage tracked.
           auto const id = max(info.mVehicle[i].mID, 0L) % GTR2Extended::MAX_MAPPED_IDS;

           memset(&(mExtended.mTrackedDamages[id]), 0, sizeof(rF2TrackedDamage));

           mDamageTrackingInfos[id].mLastImpactProcessedET = 0.0;
           mDamageTrackingInfos[id].mLastPitStopET = info.mCurrentET;
         }
       }*/
    }

    void CaptureSessionTransition(GTR2Scoring const& scoring)
    {
      // Capture the interesting session end state.
      mExtended.mSessionTransitionCapture.mGamePhase = scoring.mScoringInfo.mGamePhase;
      mExtended.mSessionTransitionCapture.mSession = scoring.mScoringInfo.mSession;

      auto const numScoringVehicles
        = min(scoring.mScoringInfo.mNumVehicles, GTR2MappedBufferHeader::MAX_MAPPED_VEHICLES);
      mExtended.mSessionTransitionCapture.mNumScoringVehicles = numScoringVehicles;

      for (int i = 0; i < numScoringVehicles; ++i) {
        auto& sessEndVeh = mExtended.mSessionTransitionCapture.mScoringVehicles[i];
        auto const& sv = scoring.mVehicles[i];

        // sessEndVeh.mID = sv.mID;
        sessEndVeh.mFinishStatus = sv.mFinishStatus;
        sessEndVeh.mIsPlayer = sv.mIsPlayer;
        sessEndVeh.mPlace = sv.mPlace;
      }
    }

    void ClearState() { ResetDamageState(); }

    void ResetDamageState()
    {
      memset(&(mExtended.mTrackedDamages), 0, sizeof(mExtended.mTrackedDamages));
      memset(&mDamageTrackingInfos, 0, sizeof(mDamageTrackingInfos));
    }

  public:
    GTR2Extended mExtended = {};

  private:
    struct DamageTracking
    {
      double mLastImpactProcessedET = 0.0;
      double mLastPitStopET = 0.0;
    };

    DamageTracking mDamageTrackingInfos[GTR2Extended::MAX_MAPPED_IDS];
  };

public:
  CrewChiefPlugin();
  ~CrewChiefPlugin() override {}

  void Destroy() override { Shutdown(); }
  PluginObjectInfo* GetInfo();
  unsigned GetPropertyCount() const { return 0; }
  PluginObjectProperty* GetProperty(const char*) { return 0; }
  PluginObjectProperty* GetProperty(const unsigned) { return 0; }

  ////////////////////////////////////////////////////
  // InternalsPluginV01 (InternalsPlugin)
  ///////////////////////////////////////////////////
  void Startup() override;  // game startup
  void Shutdown() override; // game shutdown

  void EnterRealtime() override; // entering realtime (where the vehicle can be driven)
  void ExitRealtime() override;  // exiting realtime

  void StartSession() override; // session has started
  void EndSession() override;   // session has ended

  // GAME OUTPUT
  bool WantsTelemetryUpdates() override { return true; }
  void UpdateTelemetry(TelemInfoV2 const& info) override;

  // SCORING OUTPUT
  bool WantsScoringUpdates() override
  {
    return Utils::IsFlagOff(CrewChiefPlugin::msUnsubscribedBuffersMask, SubscribedBuffer::Scoring);
  }
  void UpdateScoring(
    ScoringInfoV2 const& info) override; // update plugin with scoring info (approximately five times per second)

  bool ForceFeedback(
    float& forceValue) override; // alternate force feedback computation - return true if editing the value

  // GRAPHICS
  bool WantsGraphicsUpdates() override
  {
    return Utils::IsFlagOff(CrewChiefPlugin::msUnsubscribedBuffersMask, SubscribedBuffer::Graphics);
  }                                                         // whether we want graphics updates
  void UpdateGraphics(GraphicsInfoV2 const& info) override; // update plugin with graphics info

  // HW Control- action a control within the game
  bool HasHardwareInputs() override
  {
    return CrewChiefPlugin::msHWControlInputRequested && mExtStateTracker.mExtended.mHWControlInputEnabled
           && CrewChiefPlugin::mHWControlInputRequestReceived;
  }

  bool CheckHWControl(char const* const controlName, float& fRetVal) override;

private:
  CrewChiefPlugin(CrewChiefPlugin const& rhs) = delete;
  CrewChiefPlugin& operator=(CrewChiefPlugin const& rhs) = delete;

  void LoadConfig();

  template<typename BuffT>
  bool InitMappedBuffer(BuffT& buffer, char const* const buffLogicalName, SubscribedBuffer sb);

  template<typename BuffT>
  bool InitMappedInputBuffer(BuffT& buffer, char const* const buffLogicalName);

  void UpdateInRealtimeFC(bool inRealTime);
  void UpdateThreadState(long type, bool starting);
  void ClearState();
  void ClearTimingsAndCounters();

  void ScoringTraceBeginUpdate();
  void ReadDMROnScoringUpdate(ScoringInfoV2 const& info);
  void ReadHWControl();
  void DynamicallySubscribeToBuffer(SubscribedBuffer sb, long requestedBuffMask, const char* const buffLogicalName);
  void DynamicallyEnableInputBuffer(bool dependencyMissing,
                                    bool& controlInputRequested,
                                    bool& controlIputEnabled,
                                    char const* const buffLogicalName);
  void ReadPluginControl();
  bool IsHWControlInputDependencyMissing();
  bool IsWeatherControlInputDependencyMissing();
  bool IsRulesControlInputDependencyMissing();

  void TelemetryTraceBeginUpdate(float deltaET);

  template<typename BuffT>
  void TraceBeginUpdate(BuffT const& buffer, float& lastUpdateMillis, char const msgPrefix[]) const;

private:
  // Only used for debugging in Timing level
  float mLastTelemetryUpdateMillis = 0.0f;
  float mLastScoringUpdateMillis = 0.0f;
  float mLastRulesUpdateMillis = 0.0f;
  float mLastMultiRulesUpdateMillis = 0.0f;

  ExtendedStateTracker mExtStateTracker;

  // For telemetry, this is min mElapsedTime in the telemetry update frame.
  double mLastScoringUpdateET = -1.0;

  // Input buffer logic members:
  // Read attempt counter, used to skip reads.
  int mHWControlRequestReadCounter = 0;
  // Boost counter, boost read rate after buffer update.
  int mHWControlRequestBoostCounter = 0;

  bool mHWControlInputRequestReceived = false;
  bool mWeatherControlInputRequestReceived = false;
  bool mRulesControlInputRequestReceived = false;

  MappedBuffer<GTR2Telemetry> mTelemetry;
  MappedBuffer<GTR2Scoring> mScoring;
  MappedBuffer<GTR2ForceFeedback> mForceFeedback;
  MappedBuffer<GTR2Graphics> mGraphics;
  MappedBuffer<GTR2Extended> mExtended;

  // Input buffers:
  MappedBuffer<GTR2HWControl> mHWControl;
  MappedBuffer<GTR2PluginControl> mPluginControl;

  // All buffers mapped successfully or not.
  bool mIsMapped = false;

  //////////////////////////////////////////
  // Direct Memory Access hackery
  //////////////////////////////////////////
  DirectMemoryReader mDMR;
  bool mLastUpdateLSIWasVisible = false;
};
