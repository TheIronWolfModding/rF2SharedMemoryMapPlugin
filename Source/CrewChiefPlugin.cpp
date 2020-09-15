/*

Author: The Iron Wolf (vleonavicius@hotmail.com)
Website: thecrewchief.org
*/
#include "CrewChiefPlugin.h" // corresponding header file
#include <stdlib.h>
#include <cstddef> // offsetof
#include <cmath>

// long CrewChiefPlugin::msDebugOutputLevel = static_cast<long>(DebugLevel::Off);
long CrewChiefPlugin::msDebugOutputLevel = static_cast<long>(DebugLevel::All);
static_assert(sizeof(long) <= sizeof(DebugLevel), "sizeof(long) <= sizeof(DebugLevel)");

// long CrewChiefPlugin::msDebugOutputSource = static_cast<long>(DebugSource::General);
long CrewChiefPlugin::msDebugOutputSource = static_cast<long>(DebugSource::All);
static_assert(sizeof(long) <= sizeof(DebugSource), "sizeof(long) <= sizeof(DebugSource)");

bool CrewChiefPlugin::msDedicatedServerMapGlobally = false;
bool CrewChiefPlugin::msDirectMemoryAccessRequested = false;
long CrewChiefPlugin::msChangeProcessAffinity = 0L;

long CrewChiefPlugin::msUnsubscribedBuffersMask = 0L;
static_assert(sizeof(long) <= sizeof(SubscribedBuffer), "sizeof(long) <= sizeof(SubscribedBuffer)");

bool CrewChiefPlugin::msHWControlInputRequested = false;
bool CrewChiefPlugin::msWeatherControlInputRequested = false;
bool CrewChiefPlugin::msRulesControlInputRequested = false;

FILE* CrewChiefPlugin::msDebugFile = nullptr;

CrewChiefPluginInfo gPluginInfo;
// get the plugin-info object used to create the plugin.
extern "C" __declspec(dllexport) PluginObjectInfo* __cdecl GetPluginObjectInfo(unsigned const uIndex)
{
  if (uIndex == 0)
    return &gPluginInfo;

  return nullptr;
}

char const*
CrewChiefPluginInfo::GetName() const
{
  return PLUGIN_NAME_AND_VERSION;
}

char const*
CrewChiefPluginInfo::GetFullName() const
{
  return PLUGIN_NAME_AND_VERSION;
}

char const*
CrewChiefPluginInfo::GetDesc() const
{
  return PLUGIN_NAME_AND_VERSION;
}

unsigned const
CrewChiefPluginInfo::GetType() const
{
  return PO_INTERNALS;
}

char const*
CrewChiefPluginInfo::GetSubType() const
{
  return "Internals";
}

unsigned const
CrewChiefPluginInfo::GetVersion() const
{
  return 3;
}

void*
CrewChiefPluginInfo::Create() const
{
  return new CrewChiefPlugin();
}

PluginObjectInfo*
CrewChiefPlugin::GetInfo()
{
  return &gPluginInfo;
}

// plugin information
extern "C" __declspec(dllexport) char const* __cdecl GetPluginName()
{
  return PLUGIN_NAME_AND_VERSION;
}

extern "C" __declspec(dllexport) PluginObjectType __cdecl GetPluginType()
{
  return (PO_INTERNALS);
}

extern "C" __declspec(dllexport) int __cdecl GetPluginVersion()
{
  return (3);
} // InternalsPluginV03 functionality (if you change this return value, you must derive from the appropriate class!)

extern "C" __declspec(dllexport) PluginObject* __cdecl CreatePluginObject()
{
  return ((PluginObject*)new CrewChiefPlugin);
}

extern "C" __declspec(dllexport) void __cdecl DestroyPluginObject(PluginObject* obj)
{
  delete ((CrewChiefPlugin*)obj);
}

//////////////////////////////////////
// SharedMemoryPlugin class
//////////////////////////////////////

CrewChiefPlugin::CrewChiefPlugin()
  : mTelemetry(CrewChiefPlugin::MM_TELEMETRY_FILE_NAME)
  , mScoring(CrewChiefPlugin::MM_SCORING_FILE_NAME)
  , mForceFeedback(CrewChiefPlugin::MM_FORCE_FEEDBACK_FILE_NAME)
  , mGraphics(CrewChiefPlugin::MM_GRAPHICS_FILE_NAME)
  , mExtended(CrewChiefPlugin::MM_EXTENDED_FILE_NAME)
  , mHWControl(CrewChiefPlugin::MM_HWCONTROL_FILE_NAME, GTR2HWControl::SUPPORTED_LAYOUT_VERSION)
  , mPluginControl(CrewChiefPlugin::MM_PLUGIN_CONTROL_FILE_NAME, GTR2PluginControl::SUPPORTED_LAYOUT_VERSION)
{
  LoadConfig();

  if (CrewChiefPlugin::msChangeProcessAffinity != 0L) {
    DEBUG_MSG(DebugLevel::CriticalInfo,
              DebugSource::General,
              "Changing process affinity to: 0x%08lX",
              CrewChiefPlugin::msChangeProcessAffinity);

    auto const proc = ::GetCurrentProcess();
    DWORD_PTR aff = CrewChiefPlugin::msChangeProcessAffinity;
    if (::SetProcessAffinityMask(proc, aff) == FALSE) {
      CrewChiefPlugin::TraceLastWin32Error();
      DEBUG_MSG(DebugLevel::Errors, DebugSource::General, "Failed to change process affinity.");
    }

    ::CloseHandle(proc);
  }
}

bool
CrewChiefPlugin::IsHWControlInputDependencyMissing()
{
  if (Utils::IsFlagOn(CrewChiefPlugin::msUnsubscribedBuffersMask, SubscribedBuffer::Telemetry)) {
    DEBUG_MSG(DebugLevel::Errors,
              DebugSource::General,
              "HWControl input cannot be enabled because Telemetry updates are turned off.");

    return true;
  }

  return false;
}

bool
CrewChiefPlugin::IsWeatherControlInputDependencyMissing()
{
  if (Utils::IsFlagOn(CrewChiefPlugin::msUnsubscribedBuffersMask, SubscribedBuffer::Scoring)) {
    DEBUG_MSG(DebugLevel::Errors,
              DebugSource::General,
              "Weather control input cannot be enabled because Scoring updates are turned off.");

    return true;
  } else if (Utils::IsFlagOn(CrewChiefPlugin::msUnsubscribedBuffersMask, SubscribedBuffer::Weather)) {
    DEBUG_MSG(DebugLevel::Errors,
              DebugSource::General,
              "Weather control input cannot be enabled because Weather updates are turned off.");

    return true;
  }

  return false;
}

bool
CrewChiefPlugin::IsRulesControlInputDependencyMissing()
{
  if (Utils::IsFlagOn(CrewChiefPlugin::msUnsubscribedBuffersMask, SubscribedBuffer::Scoring)) {
    DEBUG_MSG(DebugLevel::Errors,
              DebugSource::General,
              "Rules control input cannot be enabled because Scoring updates are turned off.");

    return true;
  } else if (Utils::IsFlagOn(CrewChiefPlugin::msUnsubscribedBuffersMask, SubscribedBuffer::Rules)) {
    DEBUG_MSG(DebugLevel::Errors,
              DebugSource::General,
              "Rules control input cannot be enabled because Weather updates are turned off.");

    return true;
  }

  return false;
}

void
CrewChiefPlugin::Startup()
{
  // Print out configuration.
#ifdef VERSION_AVX2
#ifdef VERSION_MT
  DEBUG_MSG(DebugLevel::CriticalInfo,
            DebugSource::General,
            "Starting GTR2 Crew Chief Plugin Version: %s",
            SHARED_MEMORY_VERSION " AVX2+PGO+MT");
#else
  DEBUG_MSG(DebugLevel::CriticalInfo,
            DebugSource::General,
            "Starting GTR2 Crew Chief Plugin Version: %s",
            SHARED_MEMORY_VERSION " AVX2+PGO");
#endif
#else
  DEBUG_MSG(DebugLevel::CriticalInfo,
            DebugSource::General,
            "Starting GTR2 Crew Chief Plugin Version: %s",
            SHARED_MEMORY_VERSION);
#endif
  DEBUG_MSG(DebugLevel::CriticalInfo, DebugSource::General, "Configuration:");
  DEBUG_MSG(
    DebugLevel::CriticalInfo, DebugSource::General, "DebugOutputLevel: %ld", CrewChiefPlugin::msDebugOutputLevel);
  DEBUG_MSG(
    DebugLevel::CriticalInfo, DebugSource::General, "DebugOutputSource: %ld", CrewChiefPlugin::msDebugOutputSource);
  DEBUG_MSG(DebugLevel::CriticalInfo,
            DebugSource::General,
            "DedicatedServerMapGlobally: %d",
            CrewChiefPlugin::msDedicatedServerMapGlobally);
  DEBUG_MSG(DebugLevel::CriticalInfo,
            DebugSource::General,
            "EnableDirectMemoryAccess: %d",
            CrewChiefPlugin::msDirectMemoryAccessRequested);
  DEBUG_MSG(DebugLevel::CriticalInfo,
            DebugSource::General,
            "UnsubscribedBuffersMask: %ld",
            CrewChiefPlugin::msUnsubscribedBuffersMask);
  DEBUG_MSG(DebugLevel::CriticalInfo,
            DebugSource::General,
            "EnableHWControlInput: %d",
            CrewChiefPlugin::msHWControlInputRequested);
  DEBUG_MSG(DebugLevel::CriticalInfo,
            DebugSource::General,
            "EnableWeatherControlInput: %d",
            CrewChiefPlugin::msWeatherControlInputRequested);
  DEBUG_MSG(DebugLevel::CriticalInfo,
            DebugSource::General,
            "EnableRulesControlInput: %d",
            CrewChiefPlugin::msRulesControlInputRequested);

  // Even if some buffers are unsubscribed from, create them all.  Unsubscribe simply
  // means buffer won't get updated.  This simplifies client code a bit.
  RETURN_IF_FALSE(InitMappedBuffer(mTelemetry, "Telemetry", SubscribedBuffer::Telemetry));
  RETURN_IF_FALSE(InitMappedBuffer(mScoring, "Scoring", SubscribedBuffer::Scoring));
  RETURN_IF_FALSE(InitMappedBuffer(mForceFeedback, "Force Feedback", SubscribedBuffer::ForceFeedback));
  RETURN_IF_FALSE(InitMappedBuffer(mGraphics, "Graphics", SubscribedBuffer::Graphics));
  RETURN_IF_FALSE(InitMappedInputBuffer(mHWControl, "HWControl"));
  RETURN_IF_FALSE(InitMappedInputBuffer(mPluginControl, "Plugin control"));

  // Extended buffer is initialized last and is an indicator of initialization completed.
  RETURN_IF_FALSE(InitMappedBuffer(mExtended, "Extended", SubscribedBuffer::All));

  // Runtime asserts to ensure the correct layout of partially updated buffers.
  assert(sizeof(GTR2Scoring) == offsetof(GTR2Scoring, mVehicles[GTR2MappedBufferHeader::MAX_MAPPED_VEHICLES]));

  // Figure out the input buffer dependency state.
  auto hwCtrlDependencyMissing = false;
  if (CrewChiefPlugin::msHWControlInputRequested)
    hwCtrlDependencyMissing = IsHWControlInputDependencyMissing();

  auto weatherCtrlDependencyMissing = false;
  if (CrewChiefPlugin::msWeatherControlInputRequested)
    weatherCtrlDependencyMissing = IsWeatherControlInputDependencyMissing();

  auto rulesCtrlDependencyMissing = false;
  if (CrewChiefPlugin::msRulesControlInputRequested)
    rulesCtrlDependencyMissing = IsRulesControlInputDependencyMissing();

  mExtStateTracker.mExtended.mUnsubscribedBuffersMask = CrewChiefPlugin::msUnsubscribedBuffersMask;
  if (CrewChiefPlugin::msDirectMemoryAccessRequested) {
    if (!mDMR.Initialize()) {
      DEBUG_MSG(DebugLevel::Errors, DebugSource::General, "Failed to initialize DMA, disabling DMA.");

      // Disable DMA on failure.
      CrewChiefPlugin::msDirectMemoryAccessRequested = false;
      mExtStateTracker.mExtended.mDirectMemoryAccessEnabled = false;
    } else {
      mExtStateTracker.mExtended.mDirectMemoryAccessEnabled = true;
      mExtStateTracker.mExtended.mSCRPluginEnabled = mDMR.IsSCRPluginEnabled();
      mExtStateTracker.mExtended.mSCRPluginDoubleFileType = mDMR.GetSCRPluginDoubleFileType();
    }
  }

  mExtStateTracker.mExtended.mHWControlInputEnabled
    = CrewChiefPlugin::msHWControlInputRequested && !hwCtrlDependencyMissing;
  mExtStateTracker.mExtended.mWeatherControlInputEnabled
    = CrewChiefPlugin::msWeatherControlInputRequested && !weatherCtrlDependencyMissing;
  mExtStateTracker.mExtended.mRulesControlInputEnabled
    = CrewChiefPlugin::msRulesControlInputRequested && !rulesCtrlDependencyMissing;
  mExtStateTracker.mExtended.mPluginControlInputEnabled
    = Utils::IsFlagOff(CrewChiefPlugin::msUnsubscribedBuffersMask, SubscribedBuffer::Scoring);

  if (!mExtStateTracker.mExtended.mPluginControlInputEnabled)
    DEBUG_MSG(
      DebugLevel::Warnings, DebugSource::General, "Plugin control is disabled due to Scoring updates being disabled.");

  mIsMapped = true;
  DEBUG_MSG(DebugLevel::CriticalInfo, DebugSource::General, "Files mapped successfully.");

  ClearState();

  mExtended.BeginUpdate();
  memcpy(mExtended.mpWriteBuff, &(mExtStateTracker.mExtended), sizeof(GTR2Extended));
  mExtended.EndUpdate();
}

void
CrewChiefPlugin::Shutdown()
{
  DEBUG_MSG(DebugLevel::CriticalInfo, DebugSource::General, "Shutting down");

  if (msDebugFile != nullptr) {
    fclose(msDebugFile);
    msDebugFile = nullptr;
  }

  mIsMapped = false;

  mExtended.ClearState(nullptr /*pInitialContents*/);
  mExtended.ReleaseResources();

  mTelemetry.ClearState(nullptr /*pInitialContents*/);
  mTelemetry.ReleaseResources();

  mScoring.ClearState(nullptr /*pInitialContents*/);
  mScoring.ReleaseResources();

  mForceFeedback.ClearState(nullptr /*pInitialContents*/);
  mForceFeedback.ReleaseResources();

  mGraphics.ClearState(nullptr /*pInitialContents*/);
  mGraphics.ReleaseResources();

  mHWControl.ReleaseResources();
  mPluginControl.ReleaseResources();
}

void
CrewChiefPlugin::ClearTimingsAndCounters()
{

  mLastTelemetryUpdateMillis = 0.0f;
  mLastScoringUpdateMillis = 0.0f;
  mLastRulesUpdateMillis = 0.0f;
  mLastMultiRulesUpdateMillis = 0.0f;

  mLastScoringUpdateET = -1.0f;

  mLastUpdateLSIWasVisible = false;

  mHWControlRequestReadCounter = 0;
  mHWControlRequestBoostCounter = 0;

  mHWControlInputRequestReceived = false;
  mWeatherControlInputRequestReceived = false;
  mRulesControlInputRequestReceived = false;
}

void
CrewChiefPlugin::ClearState()
{
  if (!mIsMapped)
    return;

  mTelemetry.ClearState(nullptr /*pInitialContents*/);
  mScoring.ClearState(nullptr /*pInitialContents*/);
  mForceFeedback.ClearState(nullptr /*pInitialContents*/);
  mGraphics.ClearState(nullptr /*pInitialContents*/);

  // Certain members of the extended state persist between restarts/sessions.
  // So, clear the state but pass persisting state as initial state.
  mExtStateTracker.ClearState();
  mExtended.ClearState(&(mExtStateTracker.mExtended));

  ClearTimingsAndCounters();
}

void
CrewChiefPlugin::StartSession()
{
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

  if (CrewChiefPlugin::msDirectMemoryAccessRequested) {
    if (!mDMR.ReadOnNewSession(mExtStateTracker.mExtended)) {
      DEBUG_MSG(DebugLevel::Errors, DebugSource::General, "DMA read failed, disabling.");

      // Disable DMA on failure.
      CrewChiefPlugin::msDirectMemoryAccessRequested = false;
      mExtStateTracker.mExtended.mDirectMemoryAccessEnabled = false;
    }
  }

  // Clear state will do the flip for extended state.
  ClearState();
}

void
CrewChiefPlugin::EndSession()
{
  if (!mIsMapped)
    return;

  DEBUG_MSG(DebugLevel::Timing, DebugSource::General, "SESSION - Ended.");

  mExtStateTracker.mExtended.mSessionStarted = false;
  mExtStateTracker.mExtended.mTicksSessionEnded = ::GetTickCount64();

  // Capture Session End state.
  mExtStateTracker.CaptureSessionTransition(*mScoring.mpWriteBuff);

  mExtended.BeginUpdate();
  memcpy(mExtended.mpWriteBuff, &(mExtStateTracker.mExtended), sizeof(GTR2Extended));
  mExtended.EndUpdate();
}

void
CrewChiefPlugin::UpdateInRealtimeFC(bool inRealTime)
{
  if (!mIsMapped)
    return;

  DEBUG_MSG(DebugLevel::Synchronization, DebugSource::General, inRealTime ? "Entering Realtime" : "Exiting Realtime");

  mExtStateTracker.mExtended.mInRealtimeFC = inRealTime;
  if (!inRealTime)
    mExtStateTracker.ResetDamageState();

  mExtended.BeginUpdate();
  memcpy(mExtended.mpWriteBuff, &(mExtStateTracker.mExtended), sizeof(GTR2Extended));
  mExtended.EndUpdate();
}

void
CrewChiefPlugin::EnterRealtime()
{
  if (!mIsMapped)
    return;

  // start up timer every time we enter realtime
  UpdateInRealtimeFC(true /*inRealtime*/);
}

void
CrewChiefPlugin::ExitRealtime()
{
  if (!mIsMapped)
    return;

  UpdateInRealtimeFC(false /*inRealtime*/);
}

// Using GTC64 produces 7x larger average interpolation delta (roughly from 5cm to 35cm).
// The max offset stays close, so it might not matter much.
// So, let's keep QPC and see if it causes problems (FPS cost)?
#define USE_QPC
float
TicksNow()
{
#ifdef USE_QPC
  static float frequencyMicrosecond = 0.0f;
  static bool once = false;
  if (!once) {
    LARGE_INTEGER qpcFrequency = {};
    QueryPerformanceFrequency(&qpcFrequency);
    frequencyMicrosecond = static_cast<float>(qpcFrequency.QuadPart) / MICROSECONDS_IN_SECOND;
    once = true;
  }

  LARGE_INTEGER now = {};
  QueryPerformanceCounter(&now);
  return static_cast<float>(now.QuadPart) / frequencyMicrosecond;
#else
  return GetTickCount64() * MICROSECONDS_IN_MILLISECOND;
#endif
}

void
CrewChiefPlugin::TelemetryTraceBeginUpdate(float deltaET)
{
  auto ticksNow = 0.0f;
  if (Utils::IsFlagOn(CrewChiefPlugin::msDebugOutputLevel, DebugLevel::Timing)) {
    ticksNow = TicksNow();
    auto const delta = ticksNow - mLastTelemetryUpdateMillis;

    char msg[512] = {};
    sprintf(msg,
            "TELEMETRY - Begin Update:  ET delta:%f  Time delta since last update:%f  Version Begin:%ld  End:%ld",
            deltaET,
            delta / MICROSECONDS_IN_SECOND,
            mTelemetry.mpWriteBuffVersionBlock->mVersionUpdateBegin,
            mTelemetry.mpWriteBuffVersionBlock->mVersionUpdateEnd);

    DEBUG_MSG(DebugLevel::Timing, DebugSource::Telemetry, msg);
  }

  mLastTelemetryUpdateMillis = ticksNow;
}

void
CrewChiefPlugin::UpdateTelemetry(TelemInfoV2 const& info)
{
  if (!mIsMapped)
    return;

  TelemetryTraceBeginUpdate(info.mDeltaTime);
  /*
  if (!mIsMapped)
    return;

  bool isNewFrame = false;
  auto const deltaET = info.mElapsedTime - mLastTelemetryUpdateET;
  if (abs(deltaET) >= 0.0199)  // Apparently, rF2 telemetry update step is 20ms.
    isNewFrame = true;
  else {
    // Sometimes, player vehicle telemetry is updated more frequently than other vehicles.  What that means is that ET
  of player is
    // ahead of other vehicles.  This creates torn frames, and is a problem especially in online due to player
    // vehicle not having predefined position in a chain.
    // Current solution is to detect when 20ms step happens, which means that we effectively limit refresh
    // to 50FPS (seems to be what game's doing anyway).  Alternatively, we could test position info changes.

    // We need to pick min ET for the frame because one of the vehicles in a frame might be slightly ahead of the rest.
    mLastTelemetryUpdateET = min(mLastTelemetryUpdateET, info.mElapsedTime);
  }

  if (isNewFrame
    || mCurrTelemetryVehicleIndex >= GTR2MappedBufferHeader::MAX_MAPPED_VEHICLES) {
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
  auto const participantIndex = max(info.mID, 0L) % GTR2Extended::MAX_MAPPED_IDS;
  auto const alreadyUpdated = mParticipantTelemetryUpdated[participantIndex];

  if (!alreadyUpdated) {
    if (mCurrTelemetryVehicleIndex >= GTR2MappedBufferHeader::MAX_MAPPED_VEHICLES) {
      DEBUG_MSG(DebugLevel::Errors, DebugSource::General, "TELEMETRY - Exceeded maximum of allowed mapped vehicles.");
      return;
    }

    // Update extended state for this vehicle.
    // Since I do not want to miss impact data, and it is not accumulated in any way
    // I am aware of in rF2 internals, process on every telemetry update.  Actual buffer update will happen on Scoring
  update. mExtStateTracker.ProcessTelemetryUpdate(info);

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
  }*/
}

void
CrewChiefPlugin::ScoringTraceBeginUpdate()
{
  if (Utils::IsFlagOn(CrewChiefPlugin::msDebugOutputLevel, DebugLevel::Timing)) {
    TraceBeginUpdate(mScoring, mLastScoringUpdateMillis, "SCORING");
    DEBUG_MSG(DebugLevel::Timing, DebugSource::Scoring, "SCORING - Scoring ET:%f", mLastScoringUpdateET);
  }
}

template<typename BuffT>
bool
CrewChiefPlugin::InitMappedBuffer(BuffT& buffer, char const* const buffLogicalName, SubscribedBuffer sb)
{
  if (sb != SubscribedBuffer::All // All indicates that buffer cannot be unsubscribed from.
      && Utils::IsFlagOn(CrewChiefPlugin::msUnsubscribedBuffersMask, sb))
    DEBUG_MSG(DebugLevel::CriticalInfo, DebugSource::General, "Unsubscribed from the %s updates", buffLogicalName);

  if (!buffer.Initialize(CrewChiefPlugin::msDedicatedServerMapGlobally)) {
    DEBUG_MSG(DebugLevel::Errors, DebugSource::General, "Failed to initialize %s mapping", buffLogicalName);
    return false;
  }

  auto const size = static_cast<int>(sizeof(BuffT::BufferType) + sizeof(GTR2MappedBufferVersionBlock));
  DEBUG_MSG(DebugLevel::CriticalInfo, DebugSource::General, "Size of the %s buffer: %d bytes.", buffLogicalName, size);

  return true;
}

template<typename BuffT>
bool
CrewChiefPlugin::InitMappedInputBuffer(BuffT& buffer, char const* const buffLogicalName)
{
  if (!buffer.Initialize(CrewChiefPlugin::msDedicatedServerMapGlobally)) {
    DEBUG_MSG(DebugLevel::Errors, DebugSource::General, "Failed to initialize %s input mapping", buffLogicalName);
    return false;
  }

  auto const size = static_cast<int>(sizeof(BuffT::BufferType) + sizeof(GTR2MappedBufferVersionBlock));
  DEBUG_MSG(DebugLevel::CriticalInfo,
            DebugSource::General,
            "Size of the %s buffer: %d bytes.  %s input buffer supported layout version: '%ld'",
            buffLogicalName,
            size,
            buffLogicalName,
            BuffT::BufferType::SUPPORTED_LAYOUT_VERSION);

  return true;
}

template<typename BuffT>
void
CrewChiefPlugin::TraceBeginUpdate(BuffT const& buffer, float& lastUpdateMillis, char const msgPrefix[]) const
{
  auto ticksNow = 0.0f;
  if (Utils::IsFlagOn(CrewChiefPlugin::msDebugOutputLevel, DebugLevel::Timing)) {
    ticksNow = TicksNow();
    auto const delta = ticksNow - lastUpdateMillis;

    DEBUG_MSG(DebugLevel::Timing,
              DebugSource::Telemetry | DebugSource::Scoring,
              "%s - Begin Update:  Delta since last update:%f  Version Begin:%ld  End:%ld",
              msgPrefix,
              delta / MICROSECONDS_IN_SECOND,
              buffer.mpWriteBuffVersionBlock->mVersionUpdateBegin,
              buffer.mpWriteBuffVersionBlock->mVersionUpdateEnd);

    lastUpdateMillis = ticksNow;
  }
}

void
CrewChiefPlugin::UpdateScoring(ScoringInfoV2 const& info)
{
  if (!mIsMapped)
    return;

  mLastScoringUpdateET = info.mCurrentET;

  ScoringTraceBeginUpdate();

  mScoring.BeginUpdate();

  memcpy(&(mScoring.mpWriteBuff->mScoringInfo), &info, sizeof(GTR2ScoringInfo));

  if (info.mNumVehicles >= GTR2MappedBufferHeader::MAX_MAPPED_VEHICLES)
    DEBUG_MSG(DebugLevel::Errors, DebugSource::General, "Scoring exceeded maximum of allowed mapped vehicles.");

  auto const numScoringVehicles = min(info.mNumVehicles, GTR2MappedBufferHeader::MAX_MAPPED_VEHICLES);
  for (int i = 0; i < numScoringVehicles; ++i)
    memcpy(&(mScoring.mpWriteBuff->mVehicles[i]), &(info.mVehicle[i]), sizeof(GTR2VehicleScoring));

  mScoring.mpWriteBuff->mBytesUpdatedHint = static_cast<int>(offsetof(GTR2Scoring, mVehicles[numScoringVehicles]));

  mScoring.EndUpdate();

  //
  // Piggyback on the ::UpdateScoring callback to perform operations that depend on scoring updates
  // or do not have appropriate callbacks, and 5FPS is fine.
  //

  ReadDMROnScoringUpdate(info);

  ReadPluginControl();

  // Update extended state.
  mExtStateTracker.ProcessScoringUpdate(info);

  mExtended.BeginUpdate();
  memcpy(mExtended.mpWriteBuff, &(mExtStateTracker.mExtended), sizeof(GTR2Extended));
  mExtended.EndUpdate();
}

void
CrewChiefPlugin::ReadDMROnScoringUpdate(ScoringInfoV2 const& info)
{
  if (CrewChiefPlugin::msDirectMemoryAccessRequested) {
    auto const LSIVisible
      = info.mYellowFlagState != 0 || info.mGamePhase == static_cast<unsigned char>(GTR2GamePhase::Formation);
    if (!mDMR.Read(mExtStateTracker.mExtended)
        || (LSIVisible && !mDMR.ReadOnLSIVisible(mExtStateTracker.mExtended))) { // Read on FCY or Formation lap.
      DEBUG_MSG(DebugLevel::Errors, DebugSource::General, "DMA read failed, disabling.");

      // Disable DMA on failure.
      CrewChiefPlugin::msDirectMemoryAccessRequested = false;
      mExtStateTracker.mExtended.mDirectMemoryAccessEnabled = false;
      // Extended flip will happen in ScoringUpdate.
    } else { // Read succeeded.
      if (mLastUpdateLSIWasVisible && !LSIVisible)
        mDMR.ClearLSIValues(mExtStateTracker.mExtended); // Clear LSI Values on LSI going away.

      mLastUpdateLSIWasVisible = LSIVisible;
    }
  }
}

void
CrewChiefPlugin::ReadHWControl()
{
  if (!mIsMapped || !mExtStateTracker.mExtended.mHWControlInputEnabled)
    return;

  static auto const BOOST_COUNTER_THRESHOULD_END = 500 / 20; // 500ms boost.

  // Control the rate of reads.
  auto const needsBoost = ++mHWControlRequestBoostCounter < BOOST_COUNTER_THRESHOULD_END;
  ++mHWControlRequestReadCounter;
  if (!needsBoost
      && (mHWControlRequestReadCounter % 10) != 0) // Normal 200ms poll (this function is called at 20ms update rate))
    return;                                        // Skip read attempt.

  // Read input buffers.
  if (mHWControl.ReadUpdate()) {
    if (mHWControl.mReadBuff.mLayoutVersion != GTR2HWControl::SUPPORTED_LAYOUT_VERSION) {
      DEBUG_MSG(DebugLevel::Errors,
                DebugSource::General,
                "HWControl: unsupported input buffer layout version: %ld.  Disabling.",
                mHWControl.mReadBuff.mLayoutVersion);

      mExtStateTracker.mExtended.mHWControlInputEnabled = false;

      mExtended.BeginUpdate();
      memcpy(mExtended.mpWriteBuff, &(mExtStateTracker.mExtended), sizeof(GTR2Extended));
      mExtended.EndUpdate();

      return;
    }

    mHWControlInputRequestReceived = true;
    mHWControlRequestBoostCounter = 0; // Boost refresh for the next 500ms.

    if (Utils::IsFlagOn(CrewChiefPlugin::msDebugOutputLevel, DebugLevel::DevInfo)) {
      DEBUG_MSG(DebugLevel::DevInfo,
                DebugSource::HWControlInput,
                "HWControl: received:  '%s'  %1.1f   boosted: '%s'",
                mHWControl.mReadBuff.mControlName,
                mHWControl.mReadBuff.mfRetVal,
                needsBoost ? "True" : "False");
    }
  }

  // Guard against bad inputs, even though it is not plugin's job to do that really.
  if (mHWControlRequestBoostCounter >= BOOST_COUNTER_THRESHOULD_END && mHWControlInputRequestReceived) {
    mHWControlInputRequestReceived = false;
    DEBUG_MSG(DebugLevel::Errors,
              DebugSource::General,
              "Resetting mHWControlInputRequestReceived for input value: '%s'.  Bad input value?",
              mHWControl.mReadBuff.mControlName);
  }
}

// TODO: if STL is added eventually, use map SubscribedBuffer -> buffLogicalName
void
CrewChiefPlugin::DynamicallySubscribeToBuffer(SubscribedBuffer sb,
                                              long requestedBuffMask,
                                              char const* const buffLogicalName)
{
  if (Utils::IsFlagOn(CrewChiefPlugin::msUnsubscribedBuffersMask, sb) && Utils::IsFlagOn(requestedBuffMask, sb)) {
    DEBUG_MSG(DebugLevel::CriticalInfo,
              DebugSource::General,
              "Subscribing to the %s updates based on the dynamic request.",
              buffLogicalName);

    CrewChiefPlugin::msUnsubscribedBuffersMask ^= static_cast<long>(sb);
  }
}

void
CrewChiefPlugin::DynamicallyEnableInputBuffer(bool dependencyMissing,
                                              bool& controlInputRequested,
                                              bool& controlIputEnabled,
                                              char const* const buffLogicalName)
{
  if (!dependencyMissing) {
    DEBUG_MSG(DebugLevel::CriticalInfo,
              DebugSource::General,
              "Enabling %s input updates based on the dynamic request.",
              buffLogicalName);

    // Dynamic enable is allowed only once.
    controlInputRequested = controlIputEnabled = true;
  }
}

void
CrewChiefPlugin::ReadPluginControl()
{
  if (!mIsMapped || !mExtStateTracker.mExtended.mPluginControlInputEnabled)
    return;

  // Read the input buffer.
  if (mPluginControl.ReadUpdate()) {
    if (mPluginControl.mReadBuff.mLayoutVersion != GTR2PluginControl::SUPPORTED_LAYOUT_VERSION) {
      DEBUG_MSG(DebugLevel::Errors,
                DebugSource::General,
                "Plugin control: unsupported input buffer layout version: %ld.  Disabling.",
                mPluginControl.mReadBuff.mLayoutVersion);

      // Re-enable not supported.
      mExtStateTracker.mExtended.mPluginControlInputEnabled = false;
      // Extended flip will happen in ScoringUpdate.
      return;
    }

    auto const prevUBM = CrewChiefPlugin::msUnsubscribedBuffersMask;
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

    mExtStateTracker.mExtended.mUnsubscribedBuffersMask = CrewChiefPlugin::msUnsubscribedBuffersMask;

    if (prevUBM != CrewChiefPlugin::msUnsubscribedBuffersMask)
      DEBUG_MSG(DebugLevel::CriticalInfo,
                DebugSource::General,
                "Updated UnsubscribedBuffersMask: %ld",
                CrewChiefPlugin::msUnsubscribedBuffersMask);

    if (!CrewChiefPlugin::msHWControlInputRequested && mPluginControl.mReadBuff.mRequestHWControlInput)
      DynamicallyEnableInputBuffer(IsHWControlInputDependencyMissing(),
                                   CrewChiefPlugin::msHWControlInputRequested,
                                   mExtStateTracker.mExtended.mHWControlInputEnabled,
                                   "HWControl");

    // Extended flip will happen in ScoringUpdate.
  }
}

// Invoked at ~400FPS.
bool
CrewChiefPlugin::ForceFeedback(float& forceValue)
{
  if (Utils::IsFlagOn(CrewChiefPlugin::msUnsubscribedBuffersMask, SubscribedBuffer::ForceFeedback) || !mIsMapped)
    return false;

  DEBUG_MSG(DebugLevel::Timing, DebugSource::ForceFeedback, "FORCE FEEDBACK - Updated");

  // If I understand correctly, this is atomic operation.  Since this is a single value buffer, no need to do anything
  // else.
  mForceFeedback.mpWriteBuff->mForceValue = forceValue;

  return false;
}

void
CrewChiefPlugin::UpdateThreadState(long type, bool starting)
{
  (type == 0 ? mExtStateTracker.mExtended.mMultimediaThreadStarted
             : mExtStateTracker.mExtended.mSimulationThreadStarted)
    = starting;

  mExtended.BeginUpdate();
  memcpy(mExtended.mpWriteBuff, &(mExtStateTracker.mExtended), sizeof(GTR2Extended));
  mExtended.EndUpdate();
}

// Invoked at 100FPS twice for each control (836 times per frame in my test).
bool
CrewChiefPlugin::CheckHWControl(char const* const controlName, float& fRetVal)
{
  if (!mIsMapped || !mExtStateTracker.mExtended.mHWControlInputEnabled)
    return false;

  DEBUG_MSG(DebugLevel::Timing, DebugSource::HWControlInput, "CheckHWControl - invoked for: '%s'", controlName);

  // Note that we disable this callback if there's no pending HWControl update.
  // However, game checks if we have input to processs once per frame, so mHWControlInputRequestReceived can still be
  // false after we handled it (until the next HasHardwareInputs() test).
  if (!mHWControlInputRequestReceived) {
    DEBUG_MSG(
      DebugLevel::Timing, DebugSource::HWControlInput, "CheckHWControl - skipping processing, no pending input.");
    return false;
  }

  if (_stricmp(controlName, mHWControl.mReadBuff.mControlName) == 0) {
    if (Utils::IsFlagOn(CrewChiefPlugin::msDebugOutputLevel, DebugLevel::DevInfo)) {
      DEBUG_MSG(DebugLevel::DevInfo,
                DebugSource::HWControlInput,
                "CheckHWControl input applied:  '%s'  %1.1f .  Update version: %ld",
                mHWControl.mReadBuff.mControlName,
                mHWControl.mReadBuff.mfRetVal,
                mHWControl.mReadLastVersionUpdateBegin);
    }

    fRetVal = mHWControl.mReadBuff.mfRetVal;

    mHWControlInputRequestReceived = false;

    return true;
  }

  return false;
}

void
CrewChiefPlugin::UpdateGraphics(GraphicsInfoV2 const& info)
{
  if (!mIsMapped)
    return;

  DEBUG_MSG(DebugLevel::Timing, DebugSource::Graphics, "GRAPHICS - updated.");

  // Do not version Graphics buffer, as it is asynchronous by the nature anyway.
  memcpy(&(mGraphics.mpWriteBuff->mGraphicsInfo), &info, sizeof(GTR2GraphicsInfo));
}

////////////////////////////////////////////
// Debug output helpers.
////////////////////////////////////////////
void
CrewChiefPlugin::WriteDebugMsg(DebugLevel lvl,
                               long src,
                               char const* const functionName,
                               int line,
                               char const* const msg,
                               ...)
{
  if (Utils::IsFlagOff(CrewChiefPlugin::msDebugOutputLevel, lvl)
      || Utils::IsFlagOff(CrewChiefPlugin::msDebugOutputSource, src))
    return;

  va_list argList;
  if (CrewChiefPlugin::msDebugFile == nullptr) {
    CrewChiefPlugin::msDebugFile = _fsopen(CrewChiefPlugin::DEBUG_OUTPUT_FILENAME, "a", _SH_DENYNO);
    setvbuf(CrewChiefPlugin::msDebugFile, nullptr, _IOFBF, CrewChiefPlugin::BUFFER_IO_BYTES);
  }

  SYSTEMTIME st = {};
  ::GetLocalTime(&st);

  fprintf_s(CrewChiefPlugin::msDebugFile,
            "%.2d:%.2d:%.2d.%.3d TID:0x%04lx  ",
            st.wHour,
            st.wMinute,
            st.wSecond,
            st.wMilliseconds,
            ::GetCurrentThreadId());
  fprintf_s(CrewChiefPlugin::msDebugFile, "%s(%d) : ", functionName, line);

  if (lvl == DebugLevel::Errors)
    fprintf_s(CrewChiefPlugin::msDebugFile, "ERROR: ");
  else if (lvl == DebugLevel::Warnings)
    fprintf_s(CrewChiefPlugin::msDebugFile, "WARNING: ");

  if (CrewChiefPlugin::msDebugFile != nullptr) {
    va_start(argList, msg);
    vfprintf_s(CrewChiefPlugin::msDebugFile, msg, argList);
    va_end(argList);
  }

  fprintf_s(CrewChiefPlugin::msDebugFile, "\n");

  // Flush periodically for low volume messages.
  static ULONGLONG lastFlushTicks = 0uLL;
  auto const ticksNow = GetTickCount64();
  if ((ticksNow - lastFlushTicks) / MILLISECONDS_IN_SECOND > DEBUG_IO_FLUSH_PERIOD_SECS) {
    fflush(CrewChiefPlugin::msDebugFile);
    lastFlushTicks = ticksNow;
  }
}

void
CrewChiefPlugin::TraceLastWin32Error()
{
  if (Utils::IsFlagOn(CrewChiefPlugin::msDebugOutputLevel, DebugLevel::Errors))
    return;

  auto const lastError = ::GetLastError();
  if (lastError == 0)
    return;

  LPSTR messageBuffer = nullptr;
  auto const retChars
    = ::FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                       nullptr /*lpSource*/,
                       lastError,
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       reinterpret_cast<LPSTR>(&messageBuffer),
                       0 /*nSize*/,
                       nullptr /*argunments*/);

  DEBUG_MSG(DebugLevel::Errors, DebugSource::General, "Win32 error code: %d", lastError);

  if (retChars > 0 && messageBuffer != nullptr)
    DEBUG_MSG(DebugLevel::Errors, DebugSource::General, "Win32 error description: %s", messageBuffer);

  ::LocalFree(messageBuffer);
}

void
CrewChiefPlugin::LoadConfig()
{
  char wd[MAX_PATH] = {};
  ::GetCurrentDirectory(MAX_PATH, wd);

  auto iniPath = ::lstrcatA(wd, CrewChiefPlugin::CONFIG_FILE_REL_PATH);
  auto const attr = ::GetFileAttributes(iniPath);

  if (attr == INVALID_FILE_ATTRIBUTES || attr & FILE_ATTRIBUTE_DIRECTORY) {
    if (CrewChiefPlugin::msDebugOutputLevel != static_cast<long>(DebugLevel::Off))
      remove(CrewChiefPlugin::DEBUG_OUTPUT_FILENAME);
    DEBUG_MSG(
      DebugLevel::CriticalInfo, DebugSource::General, "Configuration file: %s not found, defaults loaded", iniPath);
  } else {
    auto outputLvl = ::GetPrivateProfileInt("config", "debugOutputLevel", 0, iniPath);
    auto sanitized
      = min(max(outputLvl, static_cast<long>(DebugLevel::CriticalInfo)), static_cast<long>(DebugLevel::All));
    CrewChiefPlugin::msDebugOutputLevel = sanitized;

    if (CrewChiefPlugin::msDebugOutputLevel != static_cast<long>(DebugLevel::Off))
      remove(CrewChiefPlugin::DEBUG_OUTPUT_FILENAME);

    auto outputSrc = ::GetPrivateProfileInt("config", "debugOutputSource", 0, iniPath);
    sanitized = min(max(outputSrc, static_cast<long>(DebugSource::General)), static_cast<long>(DebugSource::All));
    CrewChiefPlugin::msDebugOutputSource = sanitized;

    CrewChiefPlugin::msChangeProcessAffinity = ::GetPrivateProfileInt("config", "changeProcessAffinity", 0, iniPath);

    DEBUG_MSG(DebugLevel::CriticalInfo, DebugSource::General, "Loaded config from: %s", iniPath);
  }
}
