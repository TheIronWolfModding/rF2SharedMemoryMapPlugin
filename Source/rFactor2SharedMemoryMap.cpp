/*
Implementation of rFactor 2 internal state mapping into shared memory buffers.

Author: The Iron Wolf (vleonavicius@hotmail.com)
Website: thecrewchief.org


Acknowledgements:
  This work is based on:
    - rF2 Internals Plugin sample #7 by ISI/S397 found at: https://www.studio-397.com/modding-resources/
  Was inspired by:
    - rF1 Shared Memory Map Plugin by Dan Allongo found at: https://github.com/dallongo/rFactorSharedMemoryMap
  With contributions by: Tony Whitley


Shared resources:

  Shared resources use the following naming convention:
    - $rFactor2SMMP_<BUFFER_TYPE>$
      or
    - $rFactor2SMMP_<BUFFER_TYPE>$PID where PID is dedicated server process PID.
      or
    - Global\\$rFactor2SMMP_<BUFFER_TYPE>$PID if running in dedicated server and DedicatedServerMapGlobally option is set to 1.

  where <BUFFER_TYPE> is one of the following:
  Output buffers:
    * Telemetry - mapped view of rF2Telemetry structure
    * Scoring - mapped view of rF2Scoring structure
    * Rules - mapped view of rF2Rules structure
    * MultiRules - mapped view of rF2MultiRules structure
    * ForceFeedback - mapped view of rF2ForceFeedback structure
    * PitInfo - mapped view of rF2PitInfo structure
    * Weather - mapped view of rF2Weather structure
    * Extended - mapped view of rF2Extended structure

  Input buffers:
    * HWControl - mapped view of rF2HWControl structure
    * WeatherControl - mapped view of rF2WeatherControl structure
    * RulesControl - mapped view of rF2RulesControl structure
    * PluginControl - mapped view of rF2PluginControl structure

  Aside from Extended (see below), output buffers are (with few exceptions) exact mirror of ISI structures, plugin constantly memcpy'es them
  from game to memory mapped files.

  Plugin offers optional weak synchronization by using version variables on each of the output buffers.

  Input buffers are meant to be filled out by the clients.  To avoid complex locking input buffers use version variables as well, and were
  designed with a single client in mind.  For more high level overview of the input buffers see "Input Buffers" section in the README.md.
 
Output buffer refresh rates:
  Telemetry - updated every 10ms, but in practice only every other update contains updated data, so real update rate is around 50FPS (20ms).
  Scoring - every 200ms (5FPS).
  Rules - every 300ms (3FPS).
  MultiRules - updated only on session change.
  ForceFeedback - approximately every 2.5ms (400FPS).
  Graphics - approximately 400FPS.
  PitInfo - 100FPS.
  Weather - 1FPS.
  Extended - every 200ms (5FPS) or on tracked function call.

  Plugin does not add artificial delays, except:
    - game calls UpdateTelemetry in bursts every 10ms.  However, as of 02/18 data changes only every 20ms, so one of those bursts is dropped.
    - telemetry updates with same game time are skipped

  Plugin supports unsubscribing from buffer updates via UnsubscribedBuffersMask CustomPluginVariables.json flag.  Clients can also subscribe
  to the currently unsubscribed buffers via rF2PluginControl input buffer.

Input buffer refresh rates:

  HWControl - Read at 5FPS with 100ms boost to 50FPS once update is received.  Applied at 100FPS.
  WeatherControl - Read at 5FPS.  Applied at 1FPS.
  RulesControl - Read at 5FPS.  Applied at 3FPS.
  PluginControl - Read at 5FPS.  Applied on read.


Telemetry state:
  rF2 calls UpdateTelemetry for each vehicle.  Plugin tries to guess when all vehicles received an update, and only after
  that buffer write is marked as complete.


Extended state:
  Exposed extended state consists of:

  * Non periodically updated game state:
      Physics settings updates and various callback based properties are tracked.

  * Heuristic data exposed as an attempt to compensate for values not currently available from the game:
      Damage state is tracked, since game provides no accumulated damage data.  Tracking happens on _every_ telemetry/scoring
      update for full precision.

  * Captures parts of rF2Scoring contents when SessionEnd/SessionStart is invoked.  This helps callers to last update information
    from the previous session.  Note: In future, might get replaced with the full capture of rF2Scoring.

  See SharedMemoryPlugin::ExtendedStateTracker struct for details.

  Extended state exposes values obtaned via Direct Memory access.  This functionality is enabled via "EnableDirectMemoryAccess"
  plugin variable.  See DirectMemoryReader class for more details.

  Lastly, active plugin configuration is exposed with the intent of clients to be able to detect missing features dynamically.


Output buffer synchronization:
  Plugin does not offer hard guarantees for mapped buffer synchronization, because using synchronization primitives opens door for misuse and
  eventually, way of harming game FPS as number of clients grows.

  However, each of shared memory buffers begins with rF2MappedBufferVersionBlock structure.  If you would like to make sure you're not
  reading a torn (partially overwritten) frame, you can check rF2MappedBufferVersionBlock::mVersionUpdateBegin and
  rF2MappedBufferVersionBlock::mVersionUpdateEnd values. If they are equal, buffer is either not torn, or, in an extreme case,
  currently being written into.
  
  Note: $rFactor2SMMP_ForceFeedback$ buffer consists of a single double variable.  Since write into double is atomic, version block
  is not used (I assume compiler aligned double member correctly for x64, and I am too lazy atm to check).

  Most clients (HUDs, Dashes, visualizers) won't need synchronization.  There are many ways on detecting torn frames,
  Monitor app contains sample approach used in the Crew Chief app.
  * For basic reading from C#, see: rF2SMMonitor.MappedBuffer<>.GetMappedDataUnsynchronized.
  * To see one of the ways to avoid torn frames, see: rF2SMMonitor.MappedBuffer<>.GetMappedData.


Input buffer synchronization:
  Input buffers are designed with a single client in mind.  Plugin detects that input buffer has changed if last saved value of
  rF2MappedBufferVersionBlock::mVersionUpdateBegin has changed.  Plugin will attempt to detect "torn frame" situations, but won't attempt
  any recovery.

  See rF2SMMonitor.MappedBuffer<>.PutMappedData for sample client code.


Dedicated server use:
  If ran in dedicated server process, each shared memory buffer name has server PID appended.  If DedicatedServerMapGlobally
  preference is set to 1, plugin will attempt creating shared memory buffers in the Global section.  Note that "Create Global Objects"
  permission is needed on user account running dedicated server.


Configuration:
  Standard rF2 plugin configuration is used.  See: SharedMemoryPlugin::AccessCustomVariable.


Limitations/Assumptions:
  - Negative mID is not supported.
  - Distance between max(mID) and min(mID) in a session cannot exceed rF2MappedBufferHeader::MAX_MAPPED_IDS.
  - Max mapped vehicles: rF2MappedBufferHeader::MAX_MAPPED_VEHICLES.
  - Plugin assumes that delta Elapsed Time in a telemetry update frame cannot exceed 20ms (which effectively limits telemetry refresh rate to 50FPS).


Sample consumption:
  For sample C# client, see Monitor\rF2SMMonitor\rF2SMMonitor\MainForm.cs
*/
#include "rFactor2SharedMemoryMap.hpp"          // corresponding header file
#include <stdlib.h>
#include <cstddef>                              // offsetof

long SharedMemoryPlugin::msDebugOutputLevel = static_cast<long>(DebugLevel::Off);
static_assert(sizeof(long) <= sizeof(DebugLevel), "sizeof(long) <= sizeof(DebugLevel)");

long SharedMemoryPlugin::msDebugOutputSource = static_cast<long>(DebugSource::General);
static_assert(sizeof(long) <= sizeof(DebugSource), "sizeof(long) <= sizeof(DebugSource)");

bool SharedMemoryPlugin::msDebugISIInternals = false;
bool SharedMemoryPlugin::msDedicatedServerMapGlobally = false;
bool SharedMemoryPlugin::msDirectMemoryAccessRequested = false;

long SharedMemoryPlugin::msUnsubscribedBuffersMask = 0L;
static_assert(sizeof(long) <= sizeof(SubscribedBuffer), "sizeof(long) <= sizeof(SubscribedBuffer)");

bool SharedMemoryPlugin::msHWControlInputRequested = false;
bool SharedMemoryPlugin::msWeatherControlInputRequested = false;
bool SharedMemoryPlugin::msRulesControlInputRequested = false;

FILE* SharedMemoryPlugin::msDebugFile;
FILE* SharedMemoryPlugin::msIsiTelemetryFile;
FILE* SharedMemoryPlugin::msIsiScoringFile;

char const* const SharedMemoryPlugin::MM_TELEMETRY_FILE_NAME = "$rFactor2SMMP_Telemetry$";
char const* const SharedMemoryPlugin::MM_SCORING_FILE_NAME = "$rFactor2SMMP_Scoring$";
char const* const SharedMemoryPlugin::MM_RULES_FILE_NAME = "$rFactor2SMMP_Rules$";
char const* const SharedMemoryPlugin::MM_MULTI_RULES_FILE_NAME = "$rFactor2SMMP_MultiRules$";
char const* const SharedMemoryPlugin::MM_FORCE_FEEDBACK_FILE_NAME = "$rFactor2SMMP_ForceFeedback$";
char const* const SharedMemoryPlugin::MM_GRAPHICS_FILE_NAME = "$rFactor2SMMP_Graphics$";
char const* const SharedMemoryPlugin::MM_EXTENDED_FILE_NAME = "$rFactor2SMMP_Extended$";
char const* const SharedMemoryPlugin::MM_PIT_INFO_FILE_NAME = "$rFactor2SMMP_PitInfo$";
char const* const SharedMemoryPlugin::MM_WEATHER_FILE_NAME = "$rFactor2SMMP_Weather$";

char const* const SharedMemoryPlugin::MM_HWCONTROL_FILE_NAME = "$rFactor2SMMP_HWControl$";
char const* const SharedMemoryPlugin::MM_WEATHER_CONTROL_FILE_NAME = "$rFactor2SMMP_WeatherControl$";
char const* const SharedMemoryPlugin::MM_RULES_CONTROL_FILE_NAME = "$rFactor2SMMP_RulesControl$";
char const* const SharedMemoryPlugin::MM_PLUGIN_CONTROL_FILE_NAME = "$rFactor2SMMP_PluginControl$";

char const* const SharedMemoryPlugin::INTERNALS_TELEMETRY_FILENAME = R"(UserData\Log\RF2SMMP_InternalsTelemetryOutput.txt)";
char const* const SharedMemoryPlugin::INTERNALS_SCORING_FILENAME = R"(UserData\Log\RF2SMMP_InternalsScoringOutput.txt)";
char const* const SharedMemoryPlugin::DEBUG_OUTPUT_FILENAME = R"(UserData\Log\RF2SMMP_DebugOutput.txt)";

// plugin information
extern "C" __declspec(dllexport)
char const* __cdecl GetPluginName() { return PLUGIN_NAME_AND_VERSION; }

extern "C" __declspec(dllexport)
PluginObjectType __cdecl GetPluginType() { return(PO_INTERNALS); }

extern "C" __declspec(dllexport)
int __cdecl GetPluginVersion() { return(7); } // InternalsPluginV07 functionality (if you change this return value, you must derive from the appropriate class!)

extern "C" __declspec(dllexport)
PluginObject* __cdecl CreatePluginObject() { return((PluginObject*) new SharedMemoryPlugin); }

extern "C" __declspec(dllexport)
void __cdecl DestroyPluginObject(PluginObject* obj) { delete((SharedMemoryPlugin*) obj); }


//////////////////////////////////////
// SharedMemoryPlugin class
//////////////////////////////////////

SharedMemoryPlugin::SharedMemoryPlugin()
  : mTelemetry(SharedMemoryPlugin::MM_TELEMETRY_FILE_NAME)
    , mScoring(SharedMemoryPlugin::MM_SCORING_FILE_NAME)
    , mRules(SharedMemoryPlugin::MM_RULES_FILE_NAME)
    , mMultiRules(SharedMemoryPlugin::MM_MULTI_RULES_FILE_NAME)
    , mForceFeedback(SharedMemoryPlugin::MM_FORCE_FEEDBACK_FILE_NAME)
    , mGraphics(SharedMemoryPlugin::MM_GRAPHICS_FILE_NAME)
    , mExtended(SharedMemoryPlugin::MM_EXTENDED_FILE_NAME)
    , mPitInfo(SharedMemoryPlugin::MM_PIT_INFO_FILE_NAME)
    , mWeather(SharedMemoryPlugin::MM_WEATHER_FILE_NAME)
    , mHWControl(SharedMemoryPlugin::MM_HWCONTROL_FILE_NAME, rF2HWControl::SUPPORTED_LAYOUT_VERSION)
    , mWeatherControl(SharedMemoryPlugin::MM_WEATHER_CONTROL_FILE_NAME, rF2WeatherControl::SUPPORTED_LAYOUT_VERSION)
    , mRulesControl(SharedMemoryPlugin::MM_RULES_CONTROL_FILE_NAME, rF2HWControl::SUPPORTED_LAYOUT_VERSION)
    , mPluginControl(SharedMemoryPlugin::MM_PLUGIN_CONTROL_FILE_NAME, rF2PluginControl::SUPPORTED_LAYOUT_VERSION)
{
  memset(mParticipantTelemetryUpdated, 0, sizeof(mParticipantTelemetryUpdated));
}


bool SharedMemoryPlugin::IsHWControlInputDependencyMissing()
{
  if (Utils::IsFlagOn(SharedMemoryPlugin::msUnsubscribedBuffersMask, SubscribedBuffer::Telemetry)) {
    DEBUG_MSG(DebugLevel::Errors, DebugSource::General, "HWControl input cannot be enabled because Telemetry updates are turned off.");

    return true;
  }

  return false;
}


bool SharedMemoryPlugin::IsWeatherControlInputDependencyMissing()
{
  if (Utils::IsFlagOn(SharedMemoryPlugin::msUnsubscribedBuffersMask, SubscribedBuffer::Scoring)) {
    DEBUG_MSG(DebugLevel::Errors, DebugSource::General, "Weather control input cannot be enabled because Scoring updates are turned off.");

    return true;
  }
  else if (Utils::IsFlagOn(SharedMemoryPlugin::msUnsubscribedBuffersMask, SubscribedBuffer::Weather)) {
    DEBUG_MSG(DebugLevel::Errors, DebugSource::General, "Weather control input cannot be enabled because Weather updates are turned off.");

    return true;
  }

  return false;
}


bool SharedMemoryPlugin::IsRulesControlInputDependencyMissing()
{
  if (Utils::IsFlagOn(SharedMemoryPlugin::msUnsubscribedBuffersMask, SubscribedBuffer::Scoring)) {
    DEBUG_MSG(DebugLevel::Errors, DebugSource::General, "Rules control input cannot be enabled because Scoring updates are turned off.");

    return true;
  }
  else if (Utils::IsFlagOn(SharedMemoryPlugin::msUnsubscribedBuffersMask, SubscribedBuffer::Rules)) {
    DEBUG_MSG(DebugLevel::Errors, DebugSource::General, "Rules control input cannot be enabled because Weather updates are turned off.");

    return true;
  }

  return false;
}


void SharedMemoryPlugin::Startup(long version)
{
  // Print out configuration.
#ifdef VERSION_AVX2
#ifdef VERSION_MT
  DEBUG_MSG(DebugLevel::CriticalInfo, DebugSource::General, "Starting rFactor 2 Shared Memory Map Plugin 64bit Version: %s", SHARED_MEMORY_VERSION " AVX2+PGO+MT");
#else
  DEBUG_MSG(DebugLevel::CriticalInfo, DebugSource::General, "Starting rFactor 2 Shared Memory Map Plugin 64bit Version: %s", SHARED_MEMORY_VERSION " AVX2+PGO");
#endif
#else
  DEBUG_MSG(DebugLevel::CriticalInfo, DebugSource::General, "Starting rFactor 2 Shared Memory Map Plugin 64bit Version: %s", SHARED_MEMORY_VERSION);
#endif
  DEBUG_MSG(DebugLevel::CriticalInfo, DebugSource::General, "Configuration:");
  DEBUG_MSG(DebugLevel::CriticalInfo, DebugSource::General, "DebugOutputLevel: %ld", SharedMemoryPlugin::msDebugOutputLevel);
  DEBUG_MSG(DebugLevel::CriticalInfo, DebugSource::General, "DebugOutputSource: %ld", SharedMemoryPlugin::msDebugOutputSource);
  DEBUG_MSG(DebugLevel::CriticalInfo, DebugSource::General, "DebugISIInternals: %d", SharedMemoryPlugin::msDebugISIInternals);
  DEBUG_MSG(DebugLevel::CriticalInfo, DebugSource::General, "DedicatedServerMapGlobally: %d", SharedMemoryPlugin::msDedicatedServerMapGlobally);
  DEBUG_MSG(DebugLevel::CriticalInfo, DebugSource::General, "EnableDirectMemoryAccess: %d", SharedMemoryPlugin::msDirectMemoryAccessRequested);
  DEBUG_MSG(DebugLevel::CriticalInfo, DebugSource::General, "UnsubscribedBuffersMask: %ld", SharedMemoryPlugin::msUnsubscribedBuffersMask);
  DEBUG_MSG(DebugLevel::CriticalInfo, DebugSource::General, "EnableHWControlInput: %d", SharedMemoryPlugin::msHWControlInputRequested);
  DEBUG_MSG(DebugLevel::CriticalInfo, DebugSource::General, "EnableWeatherControlInput: %d", SharedMemoryPlugin::msWeatherControlInputRequested);
  DEBUG_MSG(DebugLevel::CriticalInfo, DebugSource::General, "EnableRulesControlInput: %d", SharedMemoryPlugin::msRulesControlInputRequested);

  char charBuff[80] = {};
  sprintf(charBuff, "-STARTUP- (version %.3f)", (float)version / 1000.0f);
  WriteToAllExampleOutputFiles("w", charBuff);

  // Even if some buffers are unsubscribed from, create them all.  Unsubscribe simply
  // means buffer won't get updated.  This simplifies client code a bit.
  RETURN_IF_FALSE(InitMappedBuffer(mTelemetry, "Telemetry",  SubscribedBuffer::Telemetry));
  RETURN_IF_FALSE(InitMappedBuffer(mScoring, "Scoring", SubscribedBuffer::Scoring));
  RETURN_IF_FALSE(InitMappedBuffer(mRules, "Rules", SubscribedBuffer::Rules));
  RETURN_IF_FALSE(InitMappedBuffer(mMultiRules, "Multi Rules", SubscribedBuffer::MultiRules));
  RETURN_IF_FALSE(InitMappedBuffer(mForceFeedback, "Force Feedback", SubscribedBuffer::ForceFeedback));
  RETURN_IF_FALSE(InitMappedBuffer(mGraphics, "Graphics", SubscribedBuffer::Graphics));
  RETURN_IF_FALSE(InitMappedBuffer(mPitInfo, "Pit Info", SubscribedBuffer::PitInfo));
  RETURN_IF_FALSE(InitMappedBuffer(mWeather, "Weather", SubscribedBuffer::Weather));
  RETURN_IF_FALSE(InitMappedInputBuffer(mHWControl, "HWControl"));
  RETURN_IF_FALSE(InitMappedInputBuffer(mWeatherControl, "Weather control"));
  RETURN_IF_FALSE(InitMappedInputBuffer(mRulesControl, "Rules control"));
  RETURN_IF_FALSE(InitMappedInputBuffer(mPluginControl, "Plugin control"));
  
  // Extended buffer is initialized last and is an indicator of initialization completed.
  RETURN_IF_FALSE(InitMappedBuffer(mExtended, "Extended", SubscribedBuffer::All));
  
  // Runtime asserts to ensure the correct layout of partially updated buffers.
  assert(sizeof(rF2Telemetry) == offsetof(rF2Telemetry, mVehicles[rF2MappedBufferHeader::MAX_MAPPED_VEHICLES]));
  assert(sizeof(rF2Scoring) == offsetof(rF2Scoring, mVehicles[rF2MappedBufferHeader::MAX_MAPPED_VEHICLES]));
  assert(sizeof(rF2Rules) == offsetof(rF2Rules, mParticipants[rF2MappedBufferHeader::MAX_MAPPED_VEHICLES]));
  assert(sizeof(rF2MultiRules) == offsetof(rF2MultiRules, mParticipants[rF2MappedBufferHeader::MAX_MAPPED_VEHICLES]));

  // Figure out input buffer dependencies.
  auto hwCtrlDependencyMissing = false;
  if (SharedMemoryPlugin::msHWControlInputRequested)
    hwCtrlDependencyMissing = IsHWControlInputDependencyMissing();

  auto weatherCtrlDependencyMissing = false;
  if (SharedMemoryPlugin::msWeatherControlInputRequested)
    weatherCtrlDependencyMissing = IsWeatherControlInputDependencyMissing();

  auto rulesCtrlDependencyMissing = false;
  if (SharedMemoryPlugin::msRulesControlInputRequested)
    rulesCtrlDependencyMissing = IsRulesControlInputDependencyMissing();

  mExtStateTracker.mExtended.mUnsubscribedBuffersMask = SharedMemoryPlugin::msUnsubscribedBuffersMask;
  if (SharedMemoryPlugin::msDirectMemoryAccessRequested) {
    if (!mDMR.Initialize()) {
      DEBUG_MSG(DebugLevel::Errors, DebugSource::General, "Failed to initialize DMA, disabling DMA.");

      // Disable DMA on failure.
      SharedMemoryPlugin::msDirectMemoryAccessRequested = false;
      mExtStateTracker.mExtended.mDirectMemoryAccessEnabled = false;
    }
    else {
      mExtStateTracker.mExtended.mDirectMemoryAccessEnabled = true;
      mExtStateTracker.mExtended.mSCRPluginEnabled = mDMR.IsSCRPluginEnabled();
      mExtStateTracker.mExtended.mSCRPluginDoubleFileType = mDMR.GetSCRPluginDoubleFileType();
    }
  }

  mExtStateTracker.mExtended.mHWControlInputEnabled = SharedMemoryPlugin::msHWControlInputRequested && !hwCtrlDependencyMissing;
  mExtStateTracker.mExtended.mWeatherControlInputEnabled = SharedMemoryPlugin::msWeatherControlInputRequested && !weatherCtrlDependencyMissing;
  mExtStateTracker.mExtended.mRulesControlInputEnabled = SharedMemoryPlugin::msRulesControlInputRequested && !rulesCtrlDependencyMissing;
  mExtStateTracker.mExtended.mPluginControlInputEnabled = Utils::IsFlagOff(SharedMemoryPlugin::msUnsubscribedBuffersMask, SubscribedBuffer::Scoring);
  
  if (!mExtStateTracker.mExtended.mPluginControlInputEnabled)
    DEBUG_MSG(DebugLevel::Warnings, DebugSource::General, "Plugin control is disabled due to Scoring updates being disabled.");

  mIsMapped = true;
  DEBUG_MSG(DebugLevel::CriticalInfo, DebugSource::General, "Files mapped successfully.");

  ClearState();

  // Keep multi rules as a special case for now, zero initialize here.
  mMultiRules.ClearState(nullptr /*pInitialContents*/);

  mExtended.BeginUpdate();
  memcpy(mExtended.mpWriteBuff, &(mExtStateTracker.mExtended), sizeof(rF2Extended));
  mExtended.EndUpdate();
}


void SharedMemoryPlugin::Shutdown()
{
  WriteToAllExampleOutputFiles("a", "-SHUTDOWN-");

  if (mIsMapped)
    TelemetryCompleteFrame();

  DEBUG_MSG(DebugLevel::CriticalInfo, DebugSource::General, "Shutting down");

  if (msDebugFile != nullptr) {
    fclose(msDebugFile);
    msDebugFile = nullptr;
  }

  if (msIsiTelemetryFile != nullptr) {
    fclose(msIsiTelemetryFile);
    msIsiTelemetryFile = nullptr;
  }

  if (msIsiScoringFile != nullptr) {
    fclose(msIsiScoringFile);
    msIsiScoringFile = nullptr;
  }

  mIsMapped = false;

  mTelemetry.ClearState(nullptr /*pInitialContents*/);
  mTelemetry.ReleaseResources();

  mScoring.ClearState(nullptr /*pInitialContents*/);
  mScoring.ReleaseResources();

  mRules.ClearState(nullptr /*pInitialContents*/);
  mRules.ReleaseResources();

  mMultiRules.ClearState(nullptr /*pInitialContents*/);
  mMultiRules.ReleaseResources();

  mForceFeedback.ClearState(nullptr /*pInitialContents*/);
  mForceFeedback.ReleaseResources();

  mGraphics.ClearState(nullptr /*pInitialContents*/);
  mGraphics.ReleaseResources();

  mExtended.ClearState(nullptr /*pInitialContents*/);
  mExtended.ReleaseResources();

  mPitInfo.ClearState(nullptr /*pInitialContents*/);
  mPitInfo.ReleaseResources();

  mWeather.ClearState(nullptr /*pInitialContents*/);
  mWeather.ReleaseResources();

  mHWControl.ReleaseResources();
  mWeatherControl.ReleaseResources();
  mRulesControl.ReleaseResources();
  mPluginControl.ReleaseResources();
}

void SharedMemoryPlugin::ClearTimingsAndCounters()
{
  TelemetryCompleteFrame();

  mLastTelemetryUpdateMillis = 0.0;
  mLastTelemetryVehicleAddedMillis = 0.0;
  mLastScoringUpdateMillis = 0.0;
  mLastRulesUpdateMillis = 0.0;
  mLastMultiRulesUpdateMillis = 0.0;

  mLastTelemetryUpdateET = -1.0;
  mLastScoringUpdateET = -1.0;

  mCurrTelemetryVehicleIndex = 0;

  memset(mParticipantTelemetryUpdated, 0, sizeof(mParticipantTelemetryUpdated));

  mLastUpdateLSIWasVisible = false;

  mPitMenuLastCategoryIndex = -1L;
  mPitMenuLastChoiceIndex = -1L;
  mPitMenuLastNumChoices = -1L;

  mHWControlRequestReadCounter = 0;
  mHWControlRequestBoostCounter = 0;
  
  mHWControlInputRequestReceived = false;
  mWeatherControlInputRequestReceived = false;
  mRulesControlInputRequestReceived = false;
}


void SharedMemoryPlugin::ClearState()
{
  if (!mIsMapped)
    return;

  mTelemetry.ClearState(nullptr /*pInitialContents*/);
  mScoring.ClearState(nullptr /*pInitialContents*/);
  mRules.ClearState(nullptr /*pInitialContents*/);
  mForceFeedback.ClearState(nullptr /*pInitialContents*/);
  mGraphics.ClearState(nullptr /*pInitialContents*/);
  // Do not clear mMultiRules as they're updated in between sessions.

  // Certain members of the extended state persist between restarts/sessions.
  // So, clear the state but pass persisting state as initial state.
  mExtStateTracker.ClearState();
  mExtended.ClearState(&(mExtStateTracker.mExtended));

  mPitInfo.ClearState(nullptr /*pInitialContents*/);
  mWeather.ClearState(nullptr /*pInitialContents*/);

  ClearTimingsAndCounters();
}


void SharedMemoryPlugin::StartSession()
{
  WriteToAllExampleOutputFiles("a", "--STARTSESSION--");

  if (!mIsMapped)
    return;

  DEBUG_MSG(DebugLevel::Timing, DebugSource::General, "SESSION - Started.");

  mExtStateTracker.mExtended.mSessionStarted = true;
  mExtStateTracker.mExtended.mTicksSessionStarted = ::GetTickCount64();

  // Sometimes, game sends updates, including final qualification positions,
  // between Session Start/End.  We need to capture some of that info, because
  // it might be overwritten by the next session.
  // Current read buffer for Scoring info contains last Scoring Update.
  mExtStateTracker.CaptureSessionTransition(*mScoring.mpWriteBuff);

  if (SharedMemoryPlugin::msDirectMemoryAccessRequested) {
    if (!mDMR.ReadOnNewSession(mExtStateTracker.mExtended)) {
      DEBUG_MSG(DebugLevel::Errors, DebugSource::General, "DMA read failed, disabling.");

      // Disable DMA on failure.
      SharedMemoryPlugin::msDirectMemoryAccessRequested = false;
      mExtStateTracker.mExtended.mDirectMemoryAccessEnabled = false;
    }
  }

  // Clear state will do the flip for extended state.
  ClearState();
}


void SharedMemoryPlugin::EndSession()
{
  WriteToAllExampleOutputFiles("a", "--ENDSESSION--");

  if (!mIsMapped)
    return;

  TelemetryCompleteFrame();

  DEBUG_MSG(DebugLevel::Timing, DebugSource::General, "SESSION - Ended.");

  mExtStateTracker.mExtended.mSessionStarted = false;
  mExtStateTracker.mExtended.mTicksSessionEnded = ::GetTickCount64();

  // Capture Session End state.
  mExtStateTracker.CaptureSessionTransition(*mScoring.mpWriteBuff);

  mExtended.BeginUpdate();
  memcpy(mExtended.mpWriteBuff, &(mExtStateTracker.mExtended), sizeof(rF2Extended));
  mExtended.EndUpdate();
}


void SharedMemoryPlugin::UpdateInRealtimeFC(bool inRealTime)
{
  if (!mIsMapped)
    return;

  DEBUG_MSG(DebugLevel::Synchronization, DebugSource::General, inRealTime ? "Entering Realtime" : "Exiting Realtime");

  mExtStateTracker.mExtended.mInRealtimeFC = inRealTime;
  if (!inRealTime)
    mExtStateTracker.ResetDamageState();

  mExtended.BeginUpdate();
  memcpy(mExtended.mpWriteBuff, &(mExtStateTracker.mExtended), sizeof(rF2Extended));
  mExtended.EndUpdate();
}


void SharedMemoryPlugin::EnterRealtime()
{
  if (!mIsMapped)
    return;

  // start up timer every time we enter realtime
  WriteToAllExampleOutputFiles("a", "---ENTERREALTIME---");

  UpdateInRealtimeFC(true /*inRealtime*/);
}


void SharedMemoryPlugin::ExitRealtime()
{
  if (!mIsMapped)
    return;

  TelemetryCompleteFrame();

  WriteToAllExampleOutputFiles("a", "---EXITREALTIME---");

  UpdateInRealtimeFC(false /*inRealtime*/);
}


// Using GTC64 produces 7x larger average interpolation delta (roughly from 5cm to 35cm).
// The max offset stays close, so it might not matter much.
// So, let's keep QPC and see if it causes problems (FPS cost)?
#define USE_QPC
double TicksNow() {
#ifdef USE_QPC
  static double frequencyMicrosecond = 0.0;
  static bool once = false;
  if (!once) {
    LARGE_INTEGER qpcFrequency = {};
    QueryPerformanceFrequency(&qpcFrequency);
    frequencyMicrosecond = static_cast<double>(qpcFrequency.QuadPart) / MICROSECONDS_IN_SECOND;
    once = true;
  }

  LARGE_INTEGER now = {};
  QueryPerformanceCounter(&now);
  return static_cast<double>(now.QuadPart) / frequencyMicrosecond;
#else
  return GetTickCount64() * MICROSECONDS_IN_MILLISECOND;
#endif
}


void SharedMemoryPlugin::TelemetryTraceSkipUpdate(TelemInfoV01 const& info, double deltaET)
{
  if (Utils::IsFlagOn(SharedMemoryPlugin::msDebugOutputLevel, DebugLevel::Timing)
    && !mTelemetrySkipFrameReported) {
    mTelemetrySkipFrameReported = true;
    DEBUG_MSG(DebugLevel::Timing, DebugSource::Telemetry, "TELEMETRY - Skipping update due to no changes in the input data.  Delta ET: %f  New ET: %f  Prev ET:%f  mID(new):%ld", deltaET, info.mElapsedTime, mLastTelemetryUpdateET, info.mID);

    // We complete frame every 20ms, so on skip we need to compare to the current write buffer.
    // Below a  ssumes that we begin skip on the first vehicle, which is not guaranteed.  However, that's ok
    // since this code is diagnostic.
    // The goal here is to detect situation where rF2 changes and begins to send telemetry more frequently.
    auto const prevBuff = mTelemetry.mpWriteBuff;
    if (info.mPos.x != prevBuff->mVehicles->mPos.x
      || info.mPos.y != prevBuff->mVehicles->mPos.y
      || info.mPos.z != prevBuff->mVehicles->mPos.z)
    {
      DEBUG_MSG(
        DebugLevel::Timing, 
        DebugSource::Telemetry,
        "WARNING - Pos Mismatch on skip update!!!  New ET: %f  Prev ET:%f  mID(old):%ld  Prev Pos: %f %f %f  New Pos %f %f %f",
        info.mElapsedTime,
        mLastTelemetryUpdateET,
        prevBuff->mVehicles->mID,
        info.mPos.x,
        info.mPos.y,
        info.mPos.z,
        prevBuff->mVehicles->mPos.x,
        prevBuff->mVehicles->mPos.y,
        prevBuff->mVehicles->mPos.z);
    }
  }
}


void SharedMemoryPlugin::TelemetryTraceBeginUpdate(double telUpdateET, double deltaET)
{
  auto ticksNow = 0.0;
  if (Utils::IsFlagOn(SharedMemoryPlugin::msDebugOutputLevel, DebugLevel::Timing)) {
    ticksNow = TicksNow();
    auto const delta = ticksNow - mLastTelemetryUpdateMillis;

    char msg[512] = {};
    sprintf(msg, "TELEMETRY - Begin Update:  ET:%f  ET delta:%f  Time delta since last update:%f  Version Begin:%ld  End:%ld",
      telUpdateET, deltaET, delta / MICROSECONDS_IN_SECOND, mTelemetry.mpWriteBuffVersionBlock->mVersionUpdateBegin, mTelemetry.mpWriteBuffVersionBlock->mVersionUpdateEnd);

    DEBUG_MSG(DebugLevel::Timing, DebugSource::Telemetry, msg);
  }

  mLastTelemetryUpdateMillis = ticksNow;
}


void SharedMemoryPlugin::TelemetryTraceVehicleAdded(TelemInfoV01 const& info)
{
  if (Utils::IsFlagOn(SharedMemoryPlugin::msDebugOutputLevel, DebugLevel::Verbose)) {
    auto const prevBuff = mTelemetry.mpWriteBuff;
    bool const samePos = info.mPos.x == prevBuff->mVehicles[mCurrTelemetryVehicleIndex].mPos.x
      && info.mPos.y == prevBuff->mVehicles[mCurrTelemetryVehicleIndex].mPos.y
      && info.mPos.z == prevBuff->mVehicles[mCurrTelemetryVehicleIndex].mPos.z;
   DEBUG_MSG(DebugLevel::Verbose, DebugSource::Telemetry, "Telemetry added - mID:%ld  ET:%f  Pos Changed:%s", info.mID, info.mElapsedTime, samePos ? "Same" : "Changed");
  }

  if (Utils::IsFlagOn(SharedMemoryPlugin::msDebugOutputLevel, DebugLevel::Timing))
    mLastTelemetryVehicleAddedMillis = TicksNow();
}


void SharedMemoryPlugin::TelemetryTraceEndUpdate(int numVehiclesInChain)
{
  ReadHWControl();

  if (Utils::IsFlagOn(SharedMemoryPlugin::msDebugOutputLevel, DebugLevel::Timing)) {
    auto const deltaSysTimeMicroseconds = mLastTelemetryVehicleAddedMillis - mLastTelemetryUpdateMillis;

    DEBUG_MSG(DebugLevel::Timing, DebugSource::Telemetry, "TELEMETRY - End Update.  Telemetry chain update took %f:  Vehicles in chain: %d  Version Begin:%ld  End:%ld",
      deltaSysTimeMicroseconds / MICROSECONDS_IN_SECOND, numVehiclesInChain, mTelemetry.mpWriteBuffVersionBlock->mVersionUpdateBegin, mTelemetry.mpWriteBuffVersionBlock->mVersionUpdateEnd);
  }
}


void SharedMemoryPlugin::TelemetryBeginNewFrame(TelemInfoV01 const& info, double deltaET)
{
  TelemetryTraceBeginUpdate(info.mElapsedTime, deltaET);

  memset(mParticipantTelemetryUpdated, 0, sizeof(mParticipantTelemetryUpdated));

  mTelemetryFrameCompleted = false;
  mCurrTelemetryVehicleIndex = 0;
  mTelemetrySkipFrameReported = false;

  // Update telemetry frame Elapsed Time.
  mLastTelemetryUpdateET = info.mElapsedTime;

  mTelemetry.BeginUpdate();
}

void SharedMemoryPlugin::TelemetryCompleteFrame()
{
  if (mTelemetryFrameCompleted)
    return;

  mTelemetry.mpWriteBuff->mNumVehicles = mCurrTelemetryVehicleIndex;
  mTelemetry.mpWriteBuff->mBytesUpdatedHint = static_cast<int>(offsetof(rF2Telemetry, mVehicles[mTelemetry.mpWriteBuff->mNumVehicles]));

  mTelemetry.EndUpdate();

  TelemetryTraceEndUpdate(mTelemetry.mpWriteBuff->mNumVehicles);

  mTelemetryFrameCompleted = true;
}


/*
rF2 sends telemetry updates for each vehicle.  The problem is that we do not know when all vehicles received an update.
Below I am trying to complete buffer update per-frame, where "frame" means all vehicles received telemetry update.

I am detecting new frame by checking time distance between mElapsedTime.  It appears that rF2 sends vehicle telemetry every 20ms
(every 10ms really, but most of the time contents are duplicated).  As a consquence, we do flip every 20ms (50FPS).

Frame end is detected by either:
- checking if number of vehicles in telemetry frame matching number of vehicles reported via scoring
- we detect cycle (same mID updated twice)

Note that sometimes mElapsedTime for player vehicle is slightly ahead of the rest of vehicles (but never more than 20ms, most often being 2.5ms off).
This mostly happens during first few seconds of going green.

There's an alternative approach that can be taken: it appears that game sends vehicle telemetry ordered by mID (ascending order).
So we could detect new frame by checking mIDs and cut when mIDPrev >= mIDCurrent.
*/
void SharedMemoryPlugin::UpdateTelemetry(TelemInfoV01 const& info)
{
  WriteTelemetryInternals(info);

  if (!mIsMapped)
    return;

  bool isNewFrame = false;
  auto const deltaET = info.mElapsedTime - mLastTelemetryUpdateET;
  if (abs(deltaET) >= 0.0199)  // Apparently, rF2 telemetry update step is 20ms.
    isNewFrame = true;
  else {
    // Sometimes, player vehicle telemetry is updated more frequently than other vehicles.  What that means is that ET of player is
    // ahead of other vehicles.  This creates torn frames, and is a problem especially in online due to player
    // vehicle not having predefined position in a chain.
    // Current solution is to detect when 20ms step happens, which means that we effectively limit refresh
    // to 50FPS (seems to be what game's doing anyway).  Alternatively, we could test position info changes.

    // We need to pick min ET for the frame because one of the vehicles in a frame might be slightly ahead of the rest.
    mLastTelemetryUpdateET = min(mLastTelemetryUpdateET, info.mElapsedTime);
  }

  if (isNewFrame
    || mCurrTelemetryVehicleIndex >= rF2MappedBufferHeader::MAX_MAPPED_VEHICLES) {
    // This is the new frame.  End the previous frame if it is still open:
    TelemetryCompleteFrame();

    // Begin the new frame.  Reset tracking variables.
    TelemetryBeginNewFrame(info, deltaET);
  }

  if (mTelemetryFrameCompleted) {
    TelemetryTraceSkipUpdate(info, deltaET);
    return;  // Nothing to do.
  }

  // See if we are in a cycle.
  auto const participantIndex = max(info.mID, 0L) % rF2Extended::MAX_MAPPED_IDS;
  auto const alreadyUpdated = mParticipantTelemetryUpdated[participantIndex];

  if (!alreadyUpdated) {
    if (mCurrTelemetryVehicleIndex >= rF2MappedBufferHeader::MAX_MAPPED_VEHICLES) {
      DEBUG_MSG(DebugLevel::Errors, DebugSource::General, "TELEMETRY - Exceeded maximum of allowed mapped vehicles.");
      return;
    }

    // Update extended state for this vehicle.
    // Since I do not want to miss impact data, and it is not accumulated in any way
    // I am aware of in rF2 internals, process on every telemetry update.  Actual buffer update will happen on Scoring update.
    mExtStateTracker.ProcessTelemetryUpdate(info);

    // Mark participant as updated
    assert(mParticipantTelemetryUpdated[participantIndex] == false);
    mParticipantTelemetryUpdated[participantIndex] = true;

    TelemetryTraceVehicleAdded(info);

    // Write vehicle telemetry.
    memcpy(&(mTelemetry.mpWriteBuff->mVehicles[mCurrTelemetryVehicleIndex]), &info, sizeof(rF2VehicleTelemetry));
    ++mCurrTelemetryVehicleIndex;

    // Do not hold this frame open for longer than necessary, as it increases collision window.
    // This also reduces latency to the minimum.
    if (mScoring.mpWriteBuff->mScoringInfo.mNumVehicles == mCurrTelemetryVehicleIndex)
      TelemetryCompleteFrame();
  }
  else {
    // The chain is complete, most likely due to vehicle updated again.
    if (!mTelemetryFrameCompleted) {
      // Trace the message marking beginning of ignore updates chain.
      TelemetryTraceSkipUpdate(info, deltaET);

      // And close the current frame.
      TelemetryCompleteFrame();
    }
  }
}


void SharedMemoryPlugin::ScoringTraceBeginUpdate()
{
  if (Utils::IsFlagOn(SharedMemoryPlugin::msDebugOutputLevel, DebugLevel::Timing)) {
    TraceBeginUpdate(mScoring, mLastScoringUpdateMillis, "SCORING");
    DEBUG_MSG(DebugLevel::Timing, DebugSource::Scoring, "SCORING - Scoring ET:%f  Telemetry ET:%f", mLastScoringUpdateET, mLastTelemetryUpdateET);
  }
}


template<typename BuffT>
bool SharedMemoryPlugin::InitMappedBuffer(BuffT& buffer, char const* const buffLogicalName, SubscribedBuffer sb)
{
  if (sb != SubscribedBuffer::All  // All indicates that buffer cannot be unsubscribed from.
    && Utils::IsFlagOn(SharedMemoryPlugin::msUnsubscribedBuffersMask, sb))
    DEBUG_MSG(DebugLevel::CriticalInfo, DebugSource::General, "Unsubscribed from the %s updates", buffLogicalName);

  if (!buffer.Initialize(SharedMemoryPlugin::msDedicatedServerMapGlobally)) {
    DEBUG_MSG(DebugLevel::Errors, DebugSource::General, "Failed to initialize %s mapping", buffLogicalName);
    return false;
  }

  auto size = static_cast<int>(sizeof(BuffT::BufferType) + sizeof(rF2MappedBufferVersionBlock));
  DEBUG_MSG(DebugLevel::CriticalInfo, DebugSource::General, "Size of the %s buffer: %d bytes.", buffLogicalName, size);

  return true;
}


template<typename BuffT>
bool SharedMemoryPlugin::InitMappedInputBuffer(BuffT& buffer, char const* const buffLogicalName)
{
  if (!buffer.Initialize(SharedMemoryPlugin::msDedicatedServerMapGlobally)) {
    DEBUG_MSG(DebugLevel::Errors, DebugSource::General, "Failed to initialize %s input mapping", buffLogicalName);
    return false;
  }

  auto size = static_cast<int>(sizeof(BuffT::BufferType) + sizeof(rF2MappedBufferVersionBlock));
  DEBUG_MSG(DebugLevel::CriticalInfo, DebugSource::General, "Size of the %s buffer: %d bytes.  %s input buffer supported layout version: '%ld'", buffLogicalName, size, buffLogicalName, BuffT::BufferType::SUPPORTED_LAYOUT_VERSION);

  return true;
}


template <typename BuffT>
void SharedMemoryPlugin::TraceBeginUpdate(BuffT const& buffer, double& lastUpdateMillis, char const msgPrefix[]) const
{
  auto ticksNow = 0.0;
  if (Utils::IsFlagOn(SharedMemoryPlugin::msDebugOutputLevel, DebugLevel::Timing)) {
    ticksNow = TicksNow();
    auto const delta = ticksNow - lastUpdateMillis;

    DEBUG_MSG(DebugLevel::Timing, DebugSource::Telemetry | DebugSource::Scoring, "%s - Begin Update:  Delta since last update:%f  Version Begin:%ld  End:%ld", msgPrefix, delta / MICROSECONDS_IN_SECOND,
      buffer.mpWriteBuffVersionBlock->mVersionUpdateBegin, buffer.mpWriteBuffVersionBlock->mVersionUpdateEnd);

    lastUpdateMillis = ticksNow;
  }
}


void SharedMemoryPlugin::UpdateScoring(ScoringInfoV01 const& info)
{
  WriteScoringInternals(info);

  if (!mIsMapped)
    return;

  mLastScoringUpdateET = info.mCurrentET;

  ScoringTraceBeginUpdate();

  // Below apparently never happens, but let's keep it in case there's a regression in the game.
  // So far, this appears to only happen on session end, when telemetry is already zeroed out.
  if (mLastScoringUpdateET > mLastTelemetryUpdateET)
    DEBUG_MSG(DebugLevel::Warnings, DebugSource::General, "Scoring update is ahead of telemetry.");

  mScoring.BeginUpdate();

  memcpy(&(mScoring.mpWriteBuff->mScoringInfo), &info, sizeof(rF2ScoringInfo));

  if (info.mNumVehicles >= rF2MappedBufferHeader::MAX_MAPPED_VEHICLES)
    DEBUG_MSG(DebugLevel::Errors, DebugSource::General, "Scoring exceeded maximum of allowed mapped vehicles.");

  auto const numScoringVehicles = min(info.mNumVehicles, rF2MappedBufferHeader::MAX_MAPPED_VEHICLES);
  for (int i = 0; i < numScoringVehicles; ++i)
    memcpy(&(mScoring.mpWriteBuff->mVehicles[i]), &(info.mVehicle[i]), sizeof(rF2VehicleScoring));

  mScoring.mpWriteBuff->mBytesUpdatedHint = static_cast<int>(offsetof(rF2Scoring, mVehicles[numScoringVehicles]));

  mScoring.EndUpdate();

  //
  // Piggyback on the ::UpdateScoring callback to perform operations that depend on scoring updates
  // or do not have appropriate callbacks, and 5FPS is fine.
  //

  ReadDMROnScoringUpdate(info);

  ReadWeatherControl();
  ReadRulesControl();
  ReadPluginControl();

  // Update extended state.
  mExtStateTracker.ProcessScoringUpdate(info);

  mExtended.BeginUpdate();
  memcpy(mExtended.mpWriteBuff, &(mExtStateTracker.mExtended), sizeof(rF2Extended));
  mExtended.EndUpdate();
}


void SharedMemoryPlugin::ReadDMROnScoringUpdate(ScoringInfoV01 const& info)
{
  if (SharedMemoryPlugin::msDirectMemoryAccessRequested) {
    auto const LSIVisible = info.mYellowFlagState != 0 || info.mGamePhase == static_cast<unsigned char>(rF2GamePhase::Formation);
    if (!mDMR.Read(mExtStateTracker.mExtended)
      || (LSIVisible && !mDMR.ReadOnLSIVisible(mExtStateTracker.mExtended))) {  // Read on FCY or Formation lap.
      DEBUG_MSG(DebugLevel::Errors, DebugSource::General, "DMA read failed, disabling.");

      // Disable DMA on failure.
      SharedMemoryPlugin::msDirectMemoryAccessRequested = false;
      mExtStateTracker.mExtended.mDirectMemoryAccessEnabled = false;
      // Extended flip will happen in ScoringUpdate.
    }
    else {  // Read succeeded.
      if (mLastUpdateLSIWasVisible && !LSIVisible)
        mDMR.ClearLSIValues(mExtStateTracker.mExtended);  // Clear LSI Values on LSI going away.

      mLastUpdateLSIWasVisible = LSIVisible;
    }
  }
}


void SharedMemoryPlugin::ReadHWControl()
{
  if (!mIsMapped
    || !mExtStateTracker.mExtended.mHWControlInputEnabled)
    return;

  // Control the rate of reads.
  auto const needsBoost = ++mHWControlRequestBoostCounter < 5;  // 100ms boost.
  ++mHWControlRequestReadCounter;
  if (!needsBoost
    && (mHWControlRequestReadCounter % 10) != 0) // Normal 200ms poll (this function is called at 20ms update rate))
    return;  // Skip read attempt.

  // Read input buffers.
  if (mHWControl.ReadUpdate()) {
    if (mHWControl.mReadBuff.mLayoutVersion != rF2HWControl::SUPPORTED_LAYOUT_VERSION) {
      DEBUG_MSG(DebugLevel::Errors, DebugSource::General, "HWControl: unsupported input buffer layout version: %ld.  Disabling.", mHWControl.mReadBuff.mLayoutVersion);

      mExtStateTracker.mExtended.mHWControlInputEnabled = false;

      mExtended.BeginUpdate();
      memcpy(mExtended.mpWriteBuff, &(mExtStateTracker.mExtended), sizeof(rF2Extended));
      mExtended.EndUpdate();

      return;
    }

    mHWControlInputRequestReceived = true;
    mHWControlRequestBoostCounter = 0;  // Boost refresh for the next 100ms.

    if (Utils::IsFlagOn(SharedMemoryPlugin::msDebugOutputLevel, DebugLevel::DevInfo)) {
      DEBUG_MSG(DebugLevel::DevInfo, DebugSource::HWControlInput, "HWControl: received:  '%s'  %1.1f   boosted: '%s'", 
        mHWControl.mReadBuff.mControlName, mHWControl.mReadBuff.mfRetVal, 
        needsBoost ? "True" : "False");
    }
  }

  // Guard against bad inputs, even though it is not plugin's job to do that really.
  if (mHWControlRequestBoostCounter >= 5
    && mHWControlInputRequestReceived) {
    mHWControlInputRequestReceived = false;
    DEBUG_MSG(DebugLevel::Errors, DebugSource::General, "Resetting mHWControlInputRequestReceived for input value: '%s'.  Bad input value?", mHWControl.mReadBuff.mControlName);
  }
}


void SharedMemoryPlugin::ReadWeatherControl()
{
  if (!mIsMapped
    || !mExtStateTracker.mExtended.mWeatherControlInputEnabled)
    return;

  // Read input buffers.
  if (mWeatherControl.ReadUpdate()) {
    if (mWeatherControl.mReadBuff.mLayoutVersion != rF2WeatherControl::SUPPORTED_LAYOUT_VERSION) {
      DEBUG_MSG(DebugLevel::Errors, DebugSource::General, "Weather control: unsupported input buffer layout version: %ld.  Disabling.", mWeatherControl.mReadBuff.mLayoutVersion);

      mExtStateTracker.mExtended.mWeatherControlInputEnabled = false;
      // Extended flip will happen in ScoringUpdate.
      return;
    }

    DEBUG_MSG(DebugLevel::DevInfo, DebugSource::WeatherControlInput, "Weather control input received.");
    mWeatherControlInputRequestReceived = true;
  }
}


void SharedMemoryPlugin::ReadRulesControl()
{
  if (!mIsMapped
    || !mExtStateTracker.mExtended.mRulesControlInputEnabled)
    return;

  // Read input buffers.
  if (mRulesControl.ReadUpdate()) {
    if (mRulesControl.mReadBuff.mLayoutVersion != rF2RulesControl::SUPPORTED_LAYOUT_VERSION) {
      DEBUG_MSG(DebugLevel::Errors, DebugSource::General, "Rules control: unsupported input buffer layout version: %ld.  Disabling.", mRulesControl.mReadBuff.mLayoutVersion);

      mExtStateTracker.mExtended.mRulesControlInputEnabled = false;
      // Extended flip will happen in ScoringUpdate.
      return;
    }

    DEBUG_MSG(DebugLevel::DevInfo, DebugSource::RulesControlInput, "Rules control input received.");
    mRulesControlInputRequestReceived = true;
  }
}


// TODO: if STL is added eventually, use map SubscribedBuffer -> buffLogicalName
void SharedMemoryPlugin::DynamicallySubscribeToBuffer(SubscribedBuffer sb, long requestedBuffMask, char const* const buffLogicalName)
{
  if (Utils::IsFlagOn(SharedMemoryPlugin::msUnsubscribedBuffersMask, sb)
    && Utils::IsFlagOn(requestedBuffMask, sb)) {
    DEBUG_MSG(DebugLevel::CriticalInfo, DebugSource::General, "Subscribing to the %s updates based on the dynamic request.", buffLogicalName);

    SharedMemoryPlugin::msUnsubscribedBuffersMask ^= static_cast<long>(sb);
  }
}


void SharedMemoryPlugin::DynamicallyEnableInputBuffer(bool dependencyMissing, bool& controlInputRequested, bool& controlIputEnabled, char const* const buffLogicalName)
{
  if (!dependencyMissing) {
    DEBUG_MSG(DebugLevel::CriticalInfo, DebugSource::General, "Enabling %s input updates based on the dynamic request.", buffLogicalName);

    // Dynamic enable is allowed only once.
    controlInputRequested = controlIputEnabled = true;
  }
}


void SharedMemoryPlugin::ReadPluginControl()
{
  if (!mIsMapped
    || !mExtStateTracker.mExtended.mPluginControlInputEnabled)
    return;

  // Read the input buffer.
  if (mPluginControl.ReadUpdate()) {
    if (mPluginControl.mReadBuff.mLayoutVersion != rF2PluginControl::SUPPORTED_LAYOUT_VERSION) {
      DEBUG_MSG(DebugLevel::Errors, DebugSource::General, "Plugin control: unsupported input buffer layout version: %ld.  Disabling.", mPluginControl.mReadBuff.mLayoutVersion);

      // Re-enable not supported.
      mExtStateTracker.mExtended.mPluginControlInputEnabled = false;
      // Extended flip will happen in ScoringUpdate.
      return;
    }

    auto const prevUBM =  SharedMemoryPlugin::msUnsubscribedBuffersMask;
    auto const rebm = mPluginControl.mReadBuff.mRequestEnableBuffersMask;
    DEBUG_MSG(DebugLevel::CriticalInfo, DebugSource::General, "Plugin control input received.");

    DynamicallySubscribeToBuffer(SubscribedBuffer::Telemetry, rebm, "Telemetry");
    // Scoring cannot be re-enabled.  If it is disabled, plugin is pretty much off anyway.
    DynamicallySubscribeToBuffer(SubscribedBuffer::Rules, rebm, "Rules");
    DynamicallySubscribeToBuffer(SubscribedBuffer::MultiRules, rebm, "Multi Rules");
    DynamicallySubscribeToBuffer(SubscribedBuffer::ForceFeedback, rebm, "Force Feedback");
    DynamicallySubscribeToBuffer(SubscribedBuffer::Graphics, rebm, "Graphics");
    DynamicallySubscribeToBuffer(SubscribedBuffer::PitInfo, rebm, "PitInfo");
    DynamicallySubscribeToBuffer(SubscribedBuffer::Weather, rebm, "Weather");

    mExtStateTracker.mExtended.mUnsubscribedBuffersMask = SharedMemoryPlugin::msUnsubscribedBuffersMask;

    if (prevUBM != SharedMemoryPlugin::msUnsubscribedBuffersMask)
      DEBUG_MSG(DebugLevel::CriticalInfo, DebugSource::General, "Updated UnsubscribedBuffersMask: %ld", SharedMemoryPlugin::msUnsubscribedBuffersMask);

    if (!SharedMemoryPlugin::msHWControlInputRequested
      && mPluginControl.mReadBuff.mRequestHWControlInput)
      DynamicallyEnableInputBuffer(IsHWControlInputDependencyMissing(), SharedMemoryPlugin::msHWControlInputRequested, mExtStateTracker.mExtended.mHWControlInputEnabled, "HWControl");

    if (!SharedMemoryPlugin::msWeatherControlInputRequested
      && mPluginControl.mReadBuff.mRequestWeatherControlInput) 
      DynamicallyEnableInputBuffer(IsWeatherControlInputDependencyMissing(), SharedMemoryPlugin::msWeatherControlInputRequested, mExtStateTracker.mExtended.mWeatherControlInputEnabled, "Weather control");

    if (!SharedMemoryPlugin::msRulesControlInputRequested
      && mPluginControl.mReadBuff.mRequestRulesControlInput)
      DynamicallyEnableInputBuffer(IsRulesControlInputDependencyMissing(), SharedMemoryPlugin::msRulesControlInputRequested, mExtStateTracker.mExtended.mRulesControlInputEnabled, "Rules control");
  
    // Extended flip will happen in ScoringUpdate.
  }
}


// Invoked at ~400FPS.
bool SharedMemoryPlugin::ForceFeedback(double& forceValue)
{
  if (Utils::IsFlagOn(SharedMemoryPlugin::msUnsubscribedBuffersMask, SubscribedBuffer::ForceFeedback) || !mIsMapped)
    return false;

  DEBUG_MSG(DebugLevel::Timing, DebugSource::ForceFeedback, "FORCE FEEDBACK - Updated");

  // If I understand correctly, this is atomic operation.  Since this is a single value buffer, no need to do anything else.
  mForceFeedback.mpWriteBuff->mForceValue = forceValue;

  return false;
}


void SharedMemoryPlugin::UpdateThreadState(long type, bool starting)
{
  (type == 0 ? mExtStateTracker.mExtended.mMultimediaThreadStarted : mExtStateTracker.mExtended.mSimulationThreadStarted)
    = starting;

  mExtended.BeginUpdate();
  memcpy(mExtended.mpWriteBuff, &(mExtStateTracker.mExtended), sizeof(rF2Extended));
  mExtended.EndUpdate();
}


void SharedMemoryPlugin::ThreadStarted(long type)
{
  if (!mIsMapped)
    return;

  DEBUG_MSG(DebugLevel::Synchronization, DebugSource::General, type == 0 ? "Multimedia thread started" : "Simulation thread started");
  UpdateThreadState(type, true /*starting*/);
}


void SharedMemoryPlugin::ThreadStopping(long type)
{
  if (!mIsMapped)
    return;

  DEBUG_MSG(DebugLevel::Synchronization, DebugSource::General, type == 0 ? "Multimedia thread stopped" : "Simulation thread stopped");
  UpdateThreadState(type, false /*starting*/);
}


// Called roughly every 300ms.
bool SharedMemoryPlugin::AccessTrackRules(TrackRulesV01& info)
{
  if (!mIsMapped)
    return false;

  TraceBeginUpdate(mRules, mLastRulesUpdateMillis, "RULES");

  mRules.BeginUpdate();

  // Copy main struct.
  memcpy(&(mRules.mpWriteBuff->mTrackRules), &info, sizeof(rF2TrackRules));

  // Copy actions.
  if (info.mNumActions >= rF2MappedBufferHeader::MAX_MAPPED_VEHICLES)
    DEBUG_MSG(DebugLevel::Errors, DebugSource::General, "Rules exceeded maximum of allowed actions.");

  auto const numActions = min(info.mNumActions, rF2MappedBufferHeader::MAX_MAPPED_VEHICLES);
  for (int i = 0; i < numActions; ++i)
    memcpy(&(mRules.mpWriteBuff->mActions[i]), &(info.mAction[i]), sizeof(rF2TrackRulesAction));

  // Copy participants.
  if (info.mNumParticipants >= rF2MappedBufferHeader::MAX_MAPPED_VEHICLES)
    DEBUG_MSG(DebugLevel::Errors, DebugSource::General, "Rules exceeded maximum of allowed mapped vehicles.");

  auto const numRulesVehicles = min(info.mNumParticipants, rF2MappedBufferHeader::MAX_MAPPED_VEHICLES);
  for (int i = 0; i < numRulesVehicles; ++i)
    memcpy(&(mRules.mpWriteBuff->mParticipants[i]), &(info.mParticipant[i]), sizeof(rF2TrackRulesParticipant));

  mRules.mpWriteBuff->mBytesUpdatedHint = static_cast<int>(offsetof(rF2Rules, mParticipants[numRulesVehicles]));

  mRules.EndUpdate();

  if (mRulesControlInputRequestReceived) {
    // Note: all experimental/WIP.
    // Try to keep updated input safe and try to keep it close to the current state/frame.
    auto const currentET = info.mCurrentET;
    auto const numActions = info.mNumActions;
    auto const pAction = info.mAction;
    auto const numParticipants = info.mNumParticipants;
    auto const safetyCarExists = info.mSafetyCarExists;
    auto const safetyCarThreshold = info.mSafetyCarThreshold;
    auto const safetyCarLapDist = info.mSafetyCarLapDist;
    auto const safetyCarLapDistAtStart = info.mSafetyCarLapDistAtStart;
    auto const pitLaneStartDist = info.mPitLaneStartDist;
    auto const pParticipant = info.mParticipant;

    memcpy(&info, &(mRulesControl.mReadBuff.mTrackRules), sizeof(TrackRulesV01));

    if (info.mNumActions != numActions)
        DEBUG_MSG(DebugLevel::Warnings, DebugSource::General, "Rules control input: mNumActions mismatch.");

    if (info.mNumParticipants != numParticipants)
        DEBUG_MSG(DebugLevel::Warnings, DebugSource::General, "Rules control input: mNumParticipants mismatch.");

    info.mCurrentET = currentET;
    info.mNumActions = numActions;
    info.mAction =  pAction;
    info.mNumParticipants = numParticipants;
    info.mSafetyCarExists = safetyCarExists;
    info.mSafetyCarThreshold = safetyCarThreshold;
    info.mSafetyCarLapDist = safetyCarLapDist;
    info.mSafetyCarLapDistAtStart = safetyCarLapDistAtStart;
    info.mPitLaneStartDist = pitLaneStartDist;
    info.mParticipant = pParticipant;

    // Safely update arrays:
    auto const numActionsToUpdate = min(
      min(info.mNumActions, rF2MappedBufferHeader::MAX_MAPPED_VEHICLES), 
      mRulesControl.mReadBuff.mTrackRules.mNumActions);

    for (int i = 0; i < numActionsToUpdate; ++i)
      memcpy(&(info.mAction[i]), &(mRulesControl.mReadBuff.mActions[i]), sizeof(TrackRulesActionV01));  // Note: this overwrites mET, not sure if that's right.

    auto const numParticipantsToUpdate = min(
      min(info.mNumParticipants, rF2MappedBufferHeader::MAX_MAPPED_VEHICLES),
      mRulesControl.mReadBuff.mTrackRules.mNumParticipants);

    for (int i = 0; i < numParticipantsToUpdate; ++i) {
      // Save input only vars (per ISI comment).
      auto const ID = info.mParticipant[i].mID;
      auto const frozenOrder = info.mParticipant[i].mFrozenOrder;
      auto const place = info.mParticipant[i].mPlace;
      auto const yellowSeverity = info.mParticipant[i].mYellowSeverity;
      auto const currentRelativeDistance = info.mParticipant[i].mCurrentRelativeDistance;

      memcpy(&(info.mParticipant[i]), &(mRulesControl.mReadBuff.mParticipants[i]), sizeof(TrackRulesParticipantV01));

      if (info.mParticipant[i].mID != ID)
        DEBUG_MSG(DebugLevel::Warnings, DebugSource::General, "Rules control input: participant mID mismatch at index: %d", i);

      // Restore input only vars.
      info.mParticipant[i].mID = ID;
      info.mParticipant[i].mFrozenOrder = frozenOrder;
      info.mParticipant[i].mPlace = place;
      info.mParticipant[i].mYellowSeverity = yellowSeverity;
      info.mParticipant[i].mCurrentRelativeDistance = currentRelativeDistance;
    }

    mRulesControlInputRequestReceived = false;

    DEBUG_MSG(DebugLevel::DevInfo, DebugSource::Rules | DebugSource::RulesControlInput, "Rules control input: applied.  Update version: %ld", mRulesControl.mReadLastVersionUpdateBegin);
    return true;
  }

  return false;
}


void SharedMemoryPlugin::SetPhysicsOptions(PhysicsOptionsV01& options)
{
  if (!mIsMapped)
    return;

  DEBUG_MSG(DebugLevel::Timing, DebugSource::Extended, "PHYSICS - Updated.");

  memcpy(&(mExtStateTracker.mExtended.mPhysics), &options, sizeof(rF2PhysicsOptions));

  mExtended.BeginUpdate();
  memcpy(mExtended.mpWriteBuff, &(mExtStateTracker.mExtended), sizeof(rF2Extended));
  mExtended.EndUpdate();
}


bool SharedMemoryPlugin::AccessMultiSessionRules(MultiSessionRulesV01& info)
{
  if (!mIsMapped)
    return false;

  TraceBeginUpdate(mMultiRules, mLastMultiRulesUpdateMillis, "MULTI RULES");

  mMultiRules.BeginUpdate();
  // Copy main struct.
  memcpy(&(mMultiRules.mpWriteBuff->mMultiSessionRules), &info, sizeof(rF2MultiSessionRules));

  // Copy participants.
  if (info.mNumParticipants >= rF2MappedBufferHeader::MAX_MAPPED_VEHICLES)
    DEBUG_MSG(DebugLevel::Errors, DebugSource::General, "Multi rules exceeded maximum of allowed mapped vehicles.");

  auto const numMultiRulesVehicles = min(info.mNumParticipants, rF2MappedBufferHeader::MAX_MAPPED_VEHICLES);
  for (int i = 0; i < numMultiRulesVehicles; ++i)
    memcpy(&(mMultiRules.mpWriteBuff->mParticipants[i]), &(info.mParticipant[i]), sizeof(rF2MultiSessionParticipant));

  mMultiRules.mpWriteBuff->mBytesUpdatedHint = static_cast<int>(offsetof(rF2MultiRules, mParticipants[numMultiRulesVehicles]));

  mMultiRules.EndUpdate();

  return false;
}



void SharedMemoryPlugin::UpdateGraphics(GraphicsInfoV02 const& info)
{
  if (!mIsMapped)
    return;

  DEBUG_MSG(DebugLevel::Timing, DebugSource::Graphics, "GRAPHICS - updated.");

  // Do not version Graphics buffer, as it is asynchronous by the nature anyway.
  memcpy(&(mGraphics.mpWriteBuff->mGraphicsInfo), &info, sizeof(rF2GraphicsInfo));
}


// Invoked at 100FPS.
bool SharedMemoryPlugin::AccessPitMenu(PitMenuV01& info)
{
  if (!mIsMapped)
    return false;

  if (mPitMenuLastCategoryIndex == info.mCategoryIndex
    && mPitMenuLastChoiceIndex == info.mChoiceIndex
    && mPitMenuLastNumChoices == info.mNumChoices) {
    DEBUG_MSG(DebugLevel::Timing, DebugSource::PitInfo, "PIT MENU - no changes.");
    return false;
  }

  if (Utils::IsFlagOn(SharedMemoryPlugin::msDebugOutputLevel, DebugLevel::DevInfo))
    DEBUG_MSG(DebugLevel::DevInfo, DebugSource::PitInfo, "PIT MENU - Updated.  Category: '%s'  Value: '%s'", info.mCategoryName, info.mChoiceString);

  mPitInfo.BeginUpdate();

  memcpy(&(mPitInfo.mpWriteBuff->mPitMenu), &info, sizeof(rF2PitMenu));

  mPitInfo.EndUpdate();

  // Avoid unnecessary interlocked operations unless pit menu state actually changed.
  mPitMenuLastCategoryIndex = info.mCategoryIndex;
  mPitMenuLastChoiceIndex = info.mChoiceIndex;
  mPitMenuLastNumChoices = info.mNumChoices;

  return false;
}


// Invoked at 100FPS twice for each control (836 times per frame in my test)
bool SharedMemoryPlugin::CheckHWControl(char const* const controlName, double& fRetVal)
{
  if (!mIsMapped
    || !mExtStateTracker.mExtended.mHWControlInputEnabled)
    return false;
 
  DEBUG_MSG(DebugLevel::Timing, DebugSource::HWControlInput, "CheckHWControl - invoked for: '%s'", controlName);

  if (mHWControlInputRequestReceived
    && _stricmp(controlName, mHWControl.mReadBuff.mControlName) == 0) {
    if (Utils::IsFlagOn(SharedMemoryPlugin::msDebugOutputLevel, DebugLevel::DevInfo)) {
      DEBUG_MSG(DebugLevel::DevInfo, DebugSource::HWControlInput, "CheckHWControl input applied:  '%s'  %1.1f .  Update version: %ld",
        mHWControl.mReadBuff.mControlName, mHWControl.mReadBuff.mfRetVal, mHWControl.mReadLastVersionUpdateBegin);
    }

    fRetVal = mHWControl.mReadBuff.mfRetVal;

    mHWControlInputRequestReceived = false;

    return true;
  }

  return false;
}


// Invoked at 1FPS.
bool SharedMemoryPlugin::AccessWeather(double trackNodeSize, WeatherControlInfoV01& info)
{
  if (!mIsMapped)
    return false;
 
  DEBUG_MSG(DebugLevel::Timing, DebugSource::Weather, "WEATHER - invoked.");

  mWeather.BeginUpdate();

  mWeather.mpWriteBuff->mTrackNodeSize = trackNodeSize;
  memcpy(&(mWeather.mpWriteBuff->mWeatherInfo), &info, sizeof(rF2WeatherControlInfo));

  mWeather.EndUpdate();

  if (mWeatherControlInputRequestReceived) {
    memcpy(&info, &(mWeatherControl.mReadBuff.mWeatherInfo), sizeof(WeatherControlInfoV01));
    mWeatherControlInputRequestReceived = false;

    DEBUG_MSG(DebugLevel::DevInfo, DebugSource::Weather | DebugSource::WeatherControlInput, "Weather control input applied.  Update version: %ld", mWeatherControl.mReadLastVersionUpdateBegin);
    return true;
  }

  return false;
}


bool SharedMemoryPlugin::GetCustomVariable(long i, CustomVariableV01& var)
{
  DEBUG_MSG(DebugLevel::Timing, DebugSource::General, "GetCustomVariable - Invoked:  mCaption - '%s'", var.mCaption);

  if (i == 0) {
    // rF2 will automatically create this variable and default it to 1 (true) unless we create it first, in which case we can choose the default.
    strcpy_s(var.mCaption, " Enabled");
    var.mNumSettings = 2;
    var.mCurrentSetting = 0;
    return true;
  }
  else if (i == 1) {
    strcpy_s(var.mCaption, "DebugOutputLevel");
    var.mNumSettings = 1;
    var.mCurrentSetting = 0;
    return true;
  }
  else if (i == 2) {
    strcpy_s(var.mCaption, "DebugISIInternals");
    var.mNumSettings = 2;
    var.mCurrentSetting = 0;
    return true;
  }
  else if (i == 3) {
    strcpy_s(var.mCaption, "DedicatedServerMapGlobally");
    var.mNumSettings = 2;
    var.mCurrentSetting = 0;
    return true;
  }
  else if (i == 4) {
    strcpy_s(var.mCaption, "EnableDirectMemoryAccess");
    var.mNumSettings = 2;
    var.mCurrentSetting = 0;
    return true;
  }
  else if (i == 5) {
    strcpy_s(var.mCaption, "UnsubscribedBuffersMask");
    var.mNumSettings = 1;

    // By default, unsubscribe from the Graphics and Weather buffer updates.
    // CC does not need some other buffers either, however it is going to be a headache
    // to explain SH users who rely on them how to configure plugin, so let it be.
    var.mCurrentSetting = 160;
    return true;
  }
  else if (i == 6) {
    strcpy_s(var.mCaption, "EnableHWControlInput");
    var.mNumSettings = 2;
    var.mCurrentSetting = 1;
    return true;
  }
  else if (i == 7) {
    strcpy_s(var.mCaption, "EnableWeatherControlInput");
    var.mNumSettings = 2;
    var.mCurrentSetting = 0;
    return true;
  }
  else if (i == 8) {
    strcpy_s(var.mCaption, "EnableRulesControlInput");
    var.mNumSettings = 2;
    var.mCurrentSetting = 0;
    return true;
  }
  else if (i == 9) {
    strcpy_s(var.mCaption, "DebugOutputSource");
    var.mNumSettings = 1;
    var.mCurrentSetting = 1;
    return true;
  }

  return false;
}


void SharedMemoryPlugin::AccessCustomVariable(CustomVariableV01& var)
{
  DEBUG_MSG(DebugLevel::Timing, DebugSource::General, "AccessCustomVariable - Invoked.");

  if (_stricmp(var.mCaption, " Enabled") == 0)
    ; // Do nothing; this variable is just for rF2 to know whether to keep the plugin loaded.
  else if (_stricmp(var.mCaption, "DebugOutputLevel") == 0) {
    auto sanitized = min(max(var.mCurrentSetting, 0L), static_cast<long>(DebugLevel::All));
    SharedMemoryPlugin::msDebugOutputLevel = sanitized;

    // Remove previous debug output.
    if (SharedMemoryPlugin::msDebugOutputLevel != static_cast<long>(DebugLevel::Off))
      remove(SharedMemoryPlugin::DEBUG_OUTPUT_FILENAME);
  }
  else if (_stricmp(var.mCaption, "DebugISIInternals") == 0)
    SharedMemoryPlugin::msDebugISIInternals = var.mCurrentSetting != 0;
  else if (_stricmp(var.mCaption, "DedicatedServerMapGlobally") == 0)
    SharedMemoryPlugin::msDedicatedServerMapGlobally = var.mCurrentSetting != 0;
  else if (_stricmp(var.mCaption, "EnableDirectMemoryAccess") == 0)
    SharedMemoryPlugin::msDirectMemoryAccessRequested = var.mCurrentSetting != 0;
  else if (_stricmp(var.mCaption, "UnsubscribedBuffersMask") == 0) {
    auto sanitized = min(max(var.mCurrentSetting, 0L), static_cast<long>(SubscribedBuffer::All));
    SharedMemoryPlugin::msUnsubscribedBuffersMask = sanitized;
  }
  else if (_stricmp(var.mCaption, "EnableHWControlInput") == 0)
    SharedMemoryPlugin::msHWControlInputRequested = var.mCurrentSetting != 0;
  else if (_stricmp(var.mCaption, "EnableWeatherControlInput") == 0)
    SharedMemoryPlugin::msWeatherControlInputRequested = var.mCurrentSetting != 0;
  else if (_stricmp(var.mCaption, "EnableRulesControlInput") == 0)
    SharedMemoryPlugin::msRulesControlInputRequested = var.mCurrentSetting != 0;
  else if (_stricmp(var.mCaption, "DebugOutputSource") == 0) {
    auto sanitized = min(max(var.mCurrentSetting, 1L), static_cast<long>(DebugSource::All));
    SharedMemoryPlugin::msDebugOutputSource = sanitized;
  }
}


void SharedMemoryPlugin::GetCustomVariableSetting(CustomVariableV01& var, long i, CustomSettingV01& setting)
{
  DEBUG_MSG(DebugLevel::Timing, DebugSource::General, "GetCustomVariableSetting - Invoked.");

  if (_stricmp(var.mCaption, " Enabled") == 0) {
    if (i == 0)
      strcpy_s(setting.mName, "False");
    else
      strcpy_s(setting.mName, "True");
  }
  else if (_stricmp(var.mCaption, "DebugISIInternals") == 0) {
    if (i == 0)
      strcpy_s(setting.mName, "False");
    else
      strcpy_s(setting.mName, "True");
  }
  else if (_stricmp(var.mCaption, "DedicatedServerMapGlobally") == 0) {
    if (i == 0)
      strcpy_s(setting.mName, "False");
    else
      strcpy_s(setting.mName, "True");
  }
  else if (_stricmp(var.mCaption, "EnableDirectMemoryAccess") == 0) {
    if (i == 0)
      strcpy_s(setting.mName, "False");
    else
      strcpy_s(setting.mName, "True");
  }
  else if (_stricmp(var.mCaption, "EnableHWControlInput") == 0) {
    if (i == 0)
      strcpy_s(setting.mName, "False");
    else
      strcpy_s(setting.mName, "True");
  }
  else if (_stricmp(var.mCaption, "EnableWeatherControlInput") == 0) {
    if (i == 0)
      strcpy_s(setting.mName, "False");
    else
      strcpy_s(setting.mName, "True");
  }
  else if (_stricmp(var.mCaption, "EnableRulesControlInput") == 0) {
    if (i == 0)
      strcpy_s(setting.mName, "False");
    else
      strcpy_s(setting.mName, "True");
  }
}


////////////////////////////////////////////
// Debug output helpers.
////////////////////////////////////////////

void SharedMemoryPlugin::WriteToAllExampleOutputFiles(char const* const openStr, char const* const msg)
{
  if (!SharedMemoryPlugin::msDebugISIInternals)
    return;

  auto fo = fopen(SharedMemoryPlugin::INTERNALS_TELEMETRY_FILENAME, openStr);
  if (fo != nullptr) {
    fprintf(fo, "%s\n", msg);
    fclose(fo);
  }

  fo = fopen(SharedMemoryPlugin::INTERNALS_SCORING_FILENAME, openStr);
  if (fo != nullptr) {
    fprintf(fo, "%s\n", msg);
    fclose(fo);
  }
}


void SharedMemoryPlugin::WriteDebugMsg(
  DebugLevel lvl,
  long src,
  char const* const functionName,
  int line,
  char const* const msg,
  ...)
{
  if (Utils::IsFlagOff(SharedMemoryPlugin::msDebugOutputLevel, lvl)
    || Utils::IsFlagOff(SharedMemoryPlugin::msDebugOutputSource, src))
    return;

  va_list argList;
  if (SharedMemoryPlugin::msDebugFile == nullptr) {
    SharedMemoryPlugin::msDebugFile = _fsopen(SharedMemoryPlugin::DEBUG_OUTPUT_FILENAME, "a", _SH_DENYNO);
    setvbuf(SharedMemoryPlugin::msDebugFile, nullptr, _IOFBF, SharedMemoryPlugin::BUFFER_IO_BYTES);
  }

  SYSTEMTIME st = {};
  ::GetLocalTime(&st);

  fprintf_s(SharedMemoryPlugin::msDebugFile, "%.2d:%.2d:%.2d.%.3d TID:0x%04lx  ", st.wHour, st.wMinute, st.wSecond , st.wMilliseconds, ::GetCurrentThreadId());
  fprintf_s(SharedMemoryPlugin::msDebugFile, "%s(%d) : ", functionName, line);

  if (lvl == DebugLevel::Errors)
    fprintf_s(SharedMemoryPlugin::msDebugFile, "ERROR: ");
  else if (lvl == DebugLevel::Warnings)
    fprintf_s(SharedMemoryPlugin::msDebugFile, "WARNING: ");

  if (SharedMemoryPlugin::msDebugFile != nullptr) {
    va_start(argList, msg);
    vfprintf_s(SharedMemoryPlugin::msDebugFile, msg, argList);
    va_end(argList);
  }

  fprintf_s(SharedMemoryPlugin::msDebugFile, "\n");

  // Flush periodically for low volume messages.
  static ULONGLONG lastFlushTicks = 0uLL;
  auto const ticksNow = GetTickCount64();
  if ((ticksNow - lastFlushTicks) / MILLISECONDS_IN_SECOND > DEBUG_IO_FLUSH_PERIOD_SECS) {
    fflush(SharedMemoryPlugin::msDebugFile);
    lastFlushTicks = ticksNow;
  }
}


void SharedMemoryPlugin::TraceLastWin32Error()
{
  if (Utils::IsFlagOn(SharedMemoryPlugin::msDebugOutputLevel, DebugLevel::Errors))
    return;

  auto const lastError = ::GetLastError();
  if (lastError == 0)
    return;

  LPSTR messageBuffer = nullptr;
  auto const retChars = ::FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
    nullptr  /*lpSource*/, lastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPSTR>(&messageBuffer),
    0  /*nSize*/, nullptr  /*argunments*/);

  DEBUG_MSG(DebugLevel::Errors, DebugSource::General, "Win32 error code: %d", lastError);

  if (retChars > 0 && messageBuffer != nullptr)
    DEBUG_MSG(DebugLevel::Errors, DebugSource::General, "Win32 error description: %s", messageBuffer);

  ::LocalFree(messageBuffer);
}
