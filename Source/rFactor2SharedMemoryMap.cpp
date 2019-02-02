/*
Implementation of rFactor 2 internal state mapping into shared memory buffers.

Author: The Iron Wolf (vleonavicius@hotmail.com)
Website: thecrewchief.org


Acknowledgements:
  This work is based on:
    - rF2 Internals Plugin sample #7 by ISI/S397 found at: https://www.studio-397.com/modding-resources/
  Was inspired by:
    - rF1 Shared Memory Map Plugin by Dan Allongo found at: https://github.com/dallongo/rFactorSharedMemoryMap


Shared resources:

  Shared resources use the following naming convention:
    - $rFactor2SMMP_<BUFFER_TYPE>$
      or
    - $rFactor2SMMP_<BUFFER_TYPE>$PID where PID is dedicated server process PID.
      or
    - Global\\$rFactor2SMMP_<BUFFER_TYPE>$PID if running in dedicated server and DedicatedServerMapGlobally option is set to 1.

  where <BUFFER_TYPE> is one of the following:
    * Telemetry - mapped view of rF2Telemetry structure
    * Scoring - mapped view of rF2Scoring structure
    * Rules - mapped view of rF2Rules structure
    * MultiRules - mapped view of rF2MultiRules structure
    * ForceFeedback - mapped view of rF2ForceFeedback structure
    * Extended - mapped view of rF2Extended structure

  Aside from Extended (see below), those types are (with few exceptions) exact mirror of ISI structures, plugin constantly memcpy'es them from game to memory mapped files.

  Plugin offers optional weak synchronization by using version variables on each of the buffers.

Refresh rates:
  Telemetry - updated every 10ms, but in practice only every other update contains updated data, so real update rate is around 50FPS (20ms).
  Scoring - every 200ms (5FPS).
  Rules - every 300ms (3FPS).
  MultiRules - updated only on session change.
  ForceFeedback - approximately every 2.5ms (400FPS).
  Extended - every 200ms or on tracked function call.

  Plugin does not add artificial delays, except:
    - game calls UpdateTelemetry in bursts every 10ms.  However, as of 02/18 data changes only every 20ms, so one of those bursts is dropped.
    - telemetry updates with same game time are skipped

Telemetry state:
  rF2 calls UpdateTelemetry for each vehicle.  Plugin tries to guess when all vehicles received an update, and only after that buffer write is marked as complete.


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

  Also, Extended state exposes values obtaned via Direct Memory access.  This functionality is enabled via "EnableDirectMemoryAccess" plugin variable.  See DirectMemoryReader class for more details.


Synchronization:
  Plugin does not offer hard guarantees for mapped buffer synchronization, because using synchronization primitives opens door for misuse and
  eventually, way of harming game FPS as number of clients grows.

  However, each of shared memory buffers begins with rF2MappedBufferVersionBlock structure.  If you would like to make sure you're not 
  reading a torn (partially overwritten) frame, you can check rF2MappedBufferVersionBlock::mVersionUpdateBegin and rF2MappedBufferVersionBlock::mVersionUpdateEnd values.
  If they are equal, buffer is either not torn, or, in an extreme case, currently being written into.
  Note: $rFactor2SMMP_ForceFeedback$ buffer consists of a single double variable.  Since write into double is atomic, version block is not used (I assume compiler aligned
  double member correctly for x64, and I am too lazy atm to check).

  Most clients (HUDs, Dashes, visualizers) won't need synchronization.  There are many ways on detecting torn frames, Monitor app contains sample approach
  used in the Crew Chief app.
  * For basic reading from C#, see: rF2SMMonitor.MainForm.MappedBuffer<>.GetMappedDataUnsynchronized.
  * To see one of the ways to avoid torn frames, see: rF2SMMonitor.MainForm.MappedBuffer<>.GetMappedData. 


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

DebugLevel SharedMemoryPlugin::msDebugOutputLevel = DebugLevel::Off;
bool SharedMemoryPlugin::msDebugISIInternals = false;
bool SharedMemoryPlugin::msDedicatedServerMapGlobally = false;
bool SharedMemoryPlugin::msDirectMemoryAccessRequested = false;

FILE* SharedMemoryPlugin::msDebugFile;
FILE* SharedMemoryPlugin::msIsiTelemetryFile;
FILE* SharedMemoryPlugin::msIsiScoringFile;

// _Weather ?
char const* const SharedMemoryPlugin::MM_TELEMETRY_FILE_NAME = "$rFactor2SMMP_Telemetry$";
char const* const SharedMemoryPlugin::MM_SCORING_FILE_NAME = "$rFactor2SMMP_Scoring$";
char const* const SharedMemoryPlugin::MM_RULES_FILE_NAME = "$rFactor2SMMP_Rules$";
char const* const SharedMemoryPlugin::MM_MULTI_RULES_FILE_NAME = "$rFactor2SMMP_MultiRules$";
char const* const SharedMemoryPlugin::MM_FORCE_FEEDBACK_FILE_NAME = "$rFactor2SMMP_ForceFeedback$";
char const* const SharedMemoryPlugin::MM_EXTENDED_FILE_NAME = "$rFactor2SMMP_Extended$";

char const* const SharedMemoryPlugin::INTERNALS_TELEMETRY_FILENAME = R"(UserData\Log\RF2SMMP_InternalsTelemetryOutput.txt)";
char const* const SharedMemoryPlugin::INTERNALS_SCORING_FILENAME = R"(UserData\Log\RF2SMMP_InternalsScoringOutput.txt)";
char const* const SharedMemoryPlugin::DEBUG_OUTPUT_FILENAME = R"(UserData\Log\RF2SMMP_DebugOutput.txt)";

// plugin information
extern "C" __declspec(dllexport)
const char * __cdecl GetPluginName() { return PLUGIN_NAME_AND_VERSION; }

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
    , mExtended(SharedMemoryPlugin::MM_EXTENDED_FILE_NAME)
{
  memset(mParticipantTelemetryUpdated, 0, sizeof(mParticipantTelemetryUpdated));
}


void SharedMemoryPlugin::Startup(long version)
{
  // Print out configuration.
#ifdef VERSION_AVX2
#ifdef VERSION_MT
  DEBUG_MSG2(DebugLevel::CriticalInfo, "Starting rFactor 2 Shared Memory Map Plugin 64bit Version:", SHARED_MEMORY_VERSION " AVX2+PGO+MT");
#else
  DEBUG_MSG2(DebugLevel::CriticalInfo, "Starting rFactor 2 Shared Memory Map Plugin 64bit Version:", SHARED_MEMORY_VERSION " AVX2+PGO");
#endif
#else
  DEBUG_MSG2(DebugLevel::CriticalInfo, "Starting rFactor 2 Shared Memory Map Plugin 64bit Version:", SHARED_MEMORY_VERSION);
#endif
  DEBUG_MSG(DebugLevel::CriticalInfo, "Configuration:");
  DEBUG_INT2(DebugLevel::CriticalInfo, "DebugOutputLevel:", SharedMemoryPlugin::msDebugOutputLevel);
  DEBUG_INT2(DebugLevel::CriticalInfo, "DebugISIInternals:", SharedMemoryPlugin::msDebugISIInternals);
  DEBUG_INT2(DebugLevel::CriticalInfo, "DedicatedServerMapGlobally:", SharedMemoryPlugin::msDedicatedServerMapGlobally);
  DEBUG_INT2(DebugLevel::CriticalInfo, "EnableDirectMemoryAccess:", SharedMemoryPlugin::msDirectMemoryAccessRequested);

  char temp[80] = {};
  sprintf(temp, "-STARTUP- (version %.3f)", (float)version / 1000.0f);
  WriteToAllExampleOutputFiles("w", temp);

  if (!mTelemetry.Initialize(SharedMemoryPlugin::msDedicatedServerMapGlobally)) {
    DEBUG_MSG(DebugLevel::Errors, "Failed to initialize telemetry mapping");
    return;
  }

  if (!mScoring.Initialize(SharedMemoryPlugin::msDedicatedServerMapGlobally)) {
    DEBUG_MSG(DebugLevel::Errors, "Failed to initialize scoring mapping");
    return;
  }

  if (!mRules.Initialize(SharedMemoryPlugin::msDedicatedServerMapGlobally)) {
    DEBUG_MSG(DebugLevel::Errors, "Failed to initialize rules mapping");
    return;
  }

  if (!mMultiRules.Initialize(SharedMemoryPlugin::msDedicatedServerMapGlobally)) {
    DEBUG_MSG(DebugLevel::Errors, "Failed to initialize multi rules mapping");
    return;
  }

  if (!mForceFeedback.Initialize(SharedMemoryPlugin::msDedicatedServerMapGlobally)) {
    DEBUG_MSG(DebugLevel::Errors, "Failed to initialize force feedback mapping");
    return;
  }

  if (!mExtended.Initialize(SharedMemoryPlugin::msDedicatedServerMapGlobally)) {
    DEBUG_MSG(DebugLevel::Errors, "Failed to initialize extended mapping");
    return;
  }

  mIsMapped = true;

  ClearState();

  // Keep multi rules as a special case for now, zero initialize here.
  mMultiRules.ClearState(nullptr /*pInitialContents*/);

  DEBUG_MSG(DebugLevel::CriticalInfo, "Files mapped successfully");
  if (SharedMemoryPlugin::msDebugOutputLevel != DebugLevel::Off) {
    char sizeSz[20] = {};
    auto size = static_cast<int>(sizeof(rF2Telemetry) + sizeof(rF2MappedBufferVersionBlock));
    _itoa_s(size, sizeSz, 10);
    DEBUG_MSG3(DebugLevel::CriticalInfo, "Size of telemetry buffer:", sizeSz, "bytes.");
    assert(sizeof(rF2Telemetry) == offsetof(rF2Telemetry, mVehicles[rF2MappedBufferHeader::MAX_MAPPED_VEHICLES]));

    sizeSz[0] = '\0';
    size = static_cast<int>(sizeof(rF2Scoring) + sizeof(rF2MappedBufferVersionBlock));
    _itoa_s(size, sizeSz, 10);
    DEBUG_MSG3(DebugLevel::CriticalInfo, "Size of scoring buffer:", sizeSz, "bytes.");
    assert(sizeof(rF2Scoring) == offsetof(rF2Scoring, mVehicles[rF2MappedBufferHeader::MAX_MAPPED_VEHICLES]));

    sizeSz[0] = '\0';
    size = static_cast<int>(sizeof(rF2Rules) + sizeof(rF2MappedBufferVersionBlock));
    _itoa_s(size, sizeSz, 10);
    DEBUG_MSG3(DebugLevel::CriticalInfo, "Size of rules buffer:", sizeSz, "bytes.");
    assert(sizeof(rF2Rules) == offsetof(rF2Rules, mParticipants[rF2MappedBufferHeader::MAX_MAPPED_VEHICLES]));

    sizeSz[0] = '\0';
    size = static_cast<int>(sizeof(rF2MultiRules) + sizeof(rF2MappedBufferVersionBlock));
    _itoa_s(size, sizeSz, 10);
    DEBUG_MSG3(DebugLevel::CriticalInfo, "Size of multi rules buffer:", sizeSz, "bytes.");
    assert(sizeof(rF2MultiRules) == offsetof(rF2MultiRules, mParticipants[rF2MappedBufferHeader::MAX_MAPPED_VEHICLES]));

    sizeSz[0] = '\0';
    size = static_cast<int>(sizeof(rF2ForceFeedback) + sizeof(rF2MappedBufferVersionBlock));
    _itoa_s(size, sizeSz, 10);
    DEBUG_MSG3(DebugLevel::CriticalInfo, "Size of force feedback buffer:", sizeSz, "bytes.");

    sizeSz[0] = '\0';
    size = static_cast<int>(sizeof(rF2Extended) + sizeof(rF2MappedBufferVersionBlock));
    _itoa_s(size, sizeSz, 10);
    DEBUG_MSG3(DebugLevel::CriticalInfo, "Size of extended buffer:", sizeSz, "bytes.");
  }

  if (SharedMemoryPlugin::msDirectMemoryAccessRequested) {
    if (!mDMR.Initialize()) {
      DEBUG_MSG(DebugLevel::Errors, "ERROR: Failed to initialize DMA, disabling DMA.");

      // Disable DMA on failure.
      SharedMemoryPlugin::msDirectMemoryAccessRequested = false;
      mExtStateTracker.mExtended.mDirectMemoryAccessEnabled = false;  // No flip necessary as this defaults to false anyway.
    }
    else {
      mExtStateTracker.mExtended.mDirectMemoryAccessEnabled = true;
      mExtStateTracker.mExtended.mSCRPluginEnabled = mDMR.IsSCRPluginEnabled();
      mExtStateTracker.mExtended.mSCRPluginDoubleFileType = mDMR.GetSCRPluginDoubleFileType();

      mExtended.BeginUpdate();
      memcpy(mExtended.mpBuff, &(mExtStateTracker.mExtended), sizeof(rF2Extended));
      mExtended.EndUpdate();
    }
  }
}

void SharedMemoryPlugin::Shutdown()
{
  WriteToAllExampleOutputFiles("a", "-SHUTDOWN-");

  if (mIsMapped)
    TelemetryCompleteFrame();

  DEBUG_MSG(DebugLevel::CriticalInfo, "Shutting down");

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

  mExtended.ClearState(nullptr /*pInitialContents*/);
  mExtended.ReleaseResources();
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
}


void SharedMemoryPlugin::ClearState()
{
  if (!mIsMapped)
    return;

  mTelemetry.ClearState(nullptr /*pInitialContents*/);
  mScoring.ClearState(nullptr /*pInitialContents*/);
  mRules.ClearState(nullptr /*pInitialContents*/);
  mForceFeedback.ClearState(nullptr /*pInitialContents*/);
  // Do not clear mMultiRules as they're updated in between sessions.

  // Certain members of the extended state persist between restarts/sessions.
  // So, clear the state but pass persisting state as initial state.
  mExtStateTracker.ClearState();
  mExtended.ClearState(&(mExtStateTracker.mExtended));

  ClearTimingsAndCounters();
}

void SharedMemoryPlugin::StartSession()
{
  WriteToAllExampleOutputFiles("a", "--STARTSESSION--");

  if (!mIsMapped)
    return;

  DEBUG_MSG(DebugLevel::Timing, "SESSION - Started.");

  mExtStateTracker.mExtended.mSessionStarted = true;
  mExtStateTracker.mExtended.mTicksSessionStarted = ::GetTickCount64();
  
  // Sometimes, game sends updates, including final qualification positions,
  // between Session Start/End.  We need to capture some of that info, because
  // it might be overwritten by the next session.
  // Current read buffer for Scoring info contains last Scoring Update.
  mExtStateTracker.CaptureSessionTransition(*mScoring.mpBuff);

  if (SharedMemoryPlugin::msDirectMemoryAccessRequested) {
    if (!mDMR.ReadOnNewSession(mExtStateTracker.mExtended)) {
      DEBUG_MSG(DebugLevel::Errors, "ERROR: DMA read failed, disabling.");

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

  DEBUG_MSG(DebugLevel::Timing, "SESSION - Ended.");

  mExtStateTracker.mExtended.mSessionStarted = false;
  mExtStateTracker.mExtended.mTicksSessionEnded = ::GetTickCount64();

  // Capture Session End state.
  mExtStateTracker.CaptureSessionTransition(*mScoring.mpBuff);

  mExtended.BeginUpdate();
  memcpy(mExtended.mpBuff, &(mExtStateTracker.mExtended), sizeof(rF2Extended));
  mExtended.EndUpdate();
}


void SharedMemoryPlugin::UpdateInRealtimeFC(bool inRealTime)
{
  if (!mIsMapped)
    return;

  DEBUG_MSG(DebugLevel::Synchronization, inRealTime ? "Entering Realtime" : "Exiting Realtime");

  mExtStateTracker.mExtended.mInRealtimeFC = inRealTime;

  mExtended.BeginUpdate();
  memcpy(mExtended.mpBuff, &(mExtStateTracker.mExtended), sizeof(rF2Extended));
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
  if (SharedMemoryPlugin::msDebugOutputLevel >= DebugLevel::Timing
    && !mTelemetrySkipFrameReported) {
    mTelemetrySkipFrameReported = true;
    char msg[512] = {};
    sprintf(msg, "TELEMETRY - Skipping update due to no changes in the input data.  Delta ET: %f  New ET: %f  Prev ET:%f  mID(new):%d", deltaET, info.mElapsedTime, mLastTelemetryUpdateET, info.mID);
    DEBUG_MSG(DebugLevel::Timing, msg);

    // We complete frame every 20ms, so on skip we need to compare to the current write buffer.
    // Below a  ssumes that we begin skip on the first vehicle, which is not guaranteed.  However, that's ok
    // since this code is diagnostic.
    // The goal here is to detect situation where rF2 changes and begins to send telemetry more frequently.
    auto const prevBuff = mTelemetry.mpBuff;
    if (info.mPos.x != prevBuff->mVehicles->mPos.x
      || info.mPos.y != prevBuff->mVehicles->mPos.y
      || info.mPos.z != prevBuff->mVehicles->mPos.z)
    {
      char msg[512] = {};
      sprintf(msg, "WARNING - Pos Mismatch on skip update!!!  New ET: %f  Prev ET:%f  mID(old):%d  Prev Pos: %f %f %f  New Pos %f %f %f", info.mElapsedTime, mLastTelemetryUpdateET, prevBuff->mVehicles->mID,
        info.mPos.x, info.mPos.y, info.mPos.z,
        prevBuff->mVehicles->mPos.x,
        prevBuff->mVehicles->mPos.y,
        prevBuff->mVehicles->mPos.z);
      DEBUG_MSG(DebugLevel::Timing, msg);
    }
  }
}


void SharedMemoryPlugin::TelemetryTraceBeginUpdate(double telUpdateET, double deltaET)
{
  auto ticksNow = 0.0;
  if (SharedMemoryPlugin::msDebugOutputLevel >= DebugLevel::Timing) {
    ticksNow = TicksNow();
    auto const delta = ticksNow - mLastTelemetryUpdateMillis;

    char msg[512] = {};
    sprintf(msg, "TELEMETRY - Begin Update:  ET:%f  ET delta:%f  Time delta since last update:%f  Version Begin:%d  End:%d",
      telUpdateET, deltaET, delta / MICROSECONDS_IN_SECOND, mTelemetry.mpBuffVersionBlock->mVersionUpdateBegin, mTelemetry.mpBuffVersionBlock->mVersionUpdateEnd);

    DEBUG_MSG(DebugLevel::Timing, msg);
  }

  mLastTelemetryUpdateMillis = ticksNow;
}


void SharedMemoryPlugin::TelemetryTraceVehicleAdded(TelemInfoV01 const& info) 
{
  if (SharedMemoryPlugin::msDebugOutputLevel == DebugLevel::Verbose) {
    auto const prevBuff = mTelemetry.mpBuff;
    bool const samePos = info.mPos.x == prevBuff->mVehicles[mCurrTelemetryVehicleIndex].mPos.x
      && info.mPos.y == prevBuff->mVehicles[mCurrTelemetryVehicleIndex].mPos.y
      && info.mPos.z == prevBuff->mVehicles[mCurrTelemetryVehicleIndex].mPos.z;

    char msg[512] = {};
    sprintf(msg, "Telemetry added - mID:%d  ET:%f  Pos Changed:%s", info.mID, info.mElapsedTime, samePos ? "Same" : "Changed");
    DEBUG_MSG(DebugLevel::Verbose, msg);
  }

  if (SharedMemoryPlugin::msDebugOutputLevel >= DebugLevel::Timing)
    mLastTelemetryVehicleAddedMillis = TicksNow();
}


void SharedMemoryPlugin::TelemetryTraceEndUpdate(int numVehiclesInChain) const
{
  if (SharedMemoryPlugin::msDebugOutputLevel >= DebugLevel::Timing) {
    auto const deltaSysTimeMicroseconds = mLastTelemetryVehicleAddedMillis - mLastTelemetryUpdateMillis;

    char msg[512] = {};
    sprintf(msg, "TELEMETRY - End Update.  Telemetry chain update took %f:  Vehicles in chain: %d  Version Begin:%d  End:%d",
      deltaSysTimeMicroseconds / MICROSECONDS_IN_SECOND, numVehiclesInChain, mTelemetry.mpBuffVersionBlock->mVersionUpdateBegin, mTelemetry.mpBuffVersionBlock->mVersionUpdateEnd);

    DEBUG_MSG(DebugLevel::Timing, msg);
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

  mTelemetry.mpBuff->mNumVehicles = mCurrTelemetryVehicleIndex;
  mTelemetry.mpBuff->mBytesUpdatedHint = static_cast<int>(offsetof(rF2Telemetry, mVehicles[mTelemetry.mpBuff->mNumVehicles]));

  mTelemetry.EndUpdate();

  TelemetryTraceEndUpdate(mTelemetry.mpBuff->mNumVehicles);

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
  auto const participantIndex = max(info.mID, 0L) % rF2MappedBufferHeader::MAX_MAPPED_IDS;
  auto const alreadyUpdated = mParticipantTelemetryUpdated[participantIndex];

  if (!alreadyUpdated) {
    if (mCurrTelemetryVehicleIndex >= rF2MappedBufferHeader::MAX_MAPPED_VEHICLES) {
      DEBUG_MSG(DebugLevel::Errors, "TELEMETRY - Exceeded maximum of allowed mapped vehicles.");
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
    memcpy(&(mTelemetry.mpBuff->mVehicles[mCurrTelemetryVehicleIndex]), &info, sizeof(rF2VehicleTelemetry));
    ++mCurrTelemetryVehicleIndex;

    // Do not hold this frame open for longer than necessary, as it increases collision window.
    // This also reduces latency to the minimum.
    if (mScoring.mpBuff->mScoringInfo.mNumVehicles == mCurrTelemetryVehicleIndex)
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
  if (SharedMemoryPlugin::msDebugOutputLevel >= DebugLevel::Timing) {
    TraceBeginUpdate(mScoring, mLastScoringUpdateMillis, "SCORING");

    char msg[512] = {};
    sprintf(msg, "SCORING - Scoring ET:%f  Telemetry ET:%f", mLastScoringUpdateET, mLastTelemetryUpdateET);

    DEBUG_MSG(DebugLevel::Timing, msg);
  }
}


template <typename BuffT>
void SharedMemoryPlugin::TraceBeginUpdate(BuffT const& buffer, double& lastUpdateMillis, char const msgPrefix[]) const
{
  auto ticksNow = 0.0;
  if (SharedMemoryPlugin::msDebugOutputLevel >= DebugLevel::Timing) {
    ticksNow = TicksNow();
    auto const delta = ticksNow - lastUpdateMillis;

    char msg[512] = {};
    sprintf(msg, "%s - Begin Update:  Delta since last update:%f  Version Begin:%d  End:%d", msgPrefix, delta / MICROSECONDS_IN_SECOND,
      buffer.mpBuffVersionBlock->mVersionUpdateBegin, buffer.mpBuffVersionBlock->mVersionUpdateEnd);

    DEBUG_MSG(DebugLevel::Timing, msg);

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
    DEBUG_MSG(DebugLevel::Warnings, "WARNING: Scoring update is ahead of telemetry.");

  mScoring.BeginUpdate();

  memcpy(&(mScoring.mpBuff->mScoringInfo), &info, sizeof(rF2ScoringInfo));

  if (info.mNumVehicles >= rF2MappedBufferHeader::MAX_MAPPED_VEHICLES)
    DEBUG_MSG(DebugLevel::Errors, "ERROR: Scoring exceeded maximum of allowed mapped vehicles.");

  auto const numScoringVehicles = min(info.mNumVehicles, rF2MappedBufferHeader::MAX_MAPPED_VEHICLES);
  for (int i = 0; i < numScoringVehicles; ++i)
    memcpy(&(mScoring.mpBuff->mVehicles[i]), &(info.mVehicle[i]), sizeof(rF2VehicleScoring));

  mScoring.mpBuff->mBytesUpdatedHint = static_cast<int>(offsetof(rF2Scoring, mVehicles[numScoringVehicles]));

  mScoring.EndUpdate();

  if (SharedMemoryPlugin::msDirectMemoryAccessRequested) {
    if (!mDMR.Read(mExtStateTracker.mExtended)
      || (info.mYellowFlagState != 0 && !mDMR.ReadOnFCY(mExtStateTracker.mExtended))) { 
      DEBUG_MSG(DebugLevel::Errors, "ERROR: DMA read failed, disabling.");

      // Disable DMA on failure.
      SharedMemoryPlugin::msDirectMemoryAccessRequested = false;
      mExtStateTracker.mExtended.mDirectMemoryAccessEnabled = false;
    }
  }

  // Update extended state.
  mExtStateTracker.ProcessScoringUpdate(info);

  // FUTURE: Minor optimization would be track if mExtended has been updated in Telemetry/Scoring and flip only if that is the case.
  mExtended.BeginUpdate();
  memcpy(mExtended.mpBuff, &(mExtStateTracker.mExtended), sizeof(rF2Extended));
  mExtended.EndUpdate();
}

// Invoked at ~400FPS.
bool SharedMemoryPlugin::ForceFeedback(double& forceValue)
{
  if (!mIsMapped)
    return false;

  DEBUG_MSG(DebugLevel::Timing, "ForceFeedback - Invoked.");

  // If I understand correctly, this is atomic operation.  Since this is a single value buffer, no need to do anything else.
  mForceFeedback.mpBuff->mForceValue = forceValue;

  return false;
}


void SharedMemoryPlugin::UpdateThreadState(long type, bool starting)
{
  (type == 0 ? mExtStateTracker.mExtended.mMultimediaThreadStarted : mExtStateTracker.mExtended.mSimulationThreadStarted)
    = starting;

  mExtended.BeginUpdate();
  memcpy(mExtended.mpBuff, &(mExtStateTracker.mExtended), sizeof(rF2Extended));
  mExtended.EndUpdate();
}


void SharedMemoryPlugin::ThreadStarted(long type)
{
  if (!mIsMapped)
    return;

  DEBUG_MSG(DebugLevel::Synchronization, type == 0 ? "Multimedia thread started" : "Simulation thread started");
  UpdateThreadState(type, true /*starting*/);
}

void SharedMemoryPlugin::ThreadStopping(long type)
{
  if (!mIsMapped)
    return;

  DEBUG_MSG(DebugLevel::Synchronization, type == 0 ? "Multimedia thread stopped" : "Simulation thread stopped");
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
  memcpy(&(mRules.mpBuff->mTrackRules), &info, sizeof(rF2TrackRules));

  // Copy actions.
  if (info.mNumActions >= rF2MappedBufferHeader::MAX_MAPPED_VEHICLES)
    DEBUG_MSG(DebugLevel::Errors, "ERROR: Rules exceeded maximum of allowed actions.");

  auto const numActions = min(info.mNumActions, rF2MappedBufferHeader::MAX_MAPPED_VEHICLES);
  for (int i = 0; i < numActions; ++i)
    memcpy(&(mRules.mpBuff->mActions[i]), &(info.mAction[i]), sizeof(rF2TrackRulesAction));

  // Copy participants.
  if (info.mNumParticipants >= rF2MappedBufferHeader::MAX_MAPPED_VEHICLES)
    DEBUG_MSG(DebugLevel::Errors, "ERROR: Rules exceeded maximum of allowed mapped vehicles.");

  auto const numRulesVehicles = min(info.mNumParticipants, rF2MappedBufferHeader::MAX_MAPPED_VEHICLES);
  for (int i = 0; i < numRulesVehicles; ++i) {
    memcpy(&(mRules.mpBuff->mParticipants[i]), &(info.mParticipant[i]), sizeof(rF2TrackRulesParticipant));
  }

  mRules.mpBuff->mBytesUpdatedHint = static_cast<int>(offsetof(rF2Rules, mParticipants[numRulesVehicles]));

  mRules.EndUpdate();

  return false;
}


void SharedMemoryPlugin::SetPhysicsOptions(PhysicsOptionsV01& options)
{
  if (!mIsMapped)
    return;

  DEBUG_MSG(DebugLevel::Timing, "PHYSICS - Updated.");

  memcpy(&(mExtStateTracker.mExtended.mPhysics), &options, sizeof(rF2PhysicsOptions));

  mExtended.BeginUpdate();
  memcpy(mExtended.mpBuff, &(mExtStateTracker.mExtended), sizeof(rF2Extended));
  mExtended.EndUpdate();
}


bool SharedMemoryPlugin::AccessMultiSessionRules(MultiSessionRulesV01& info)
{
  if (!mIsMapped)
    return false;

  TraceBeginUpdate(mMultiRules, mLastMultiRulesUpdateMillis, "MULTI RULES");
 
  mMultiRules.BeginUpdate();
  // Copy main struct.
  memcpy(&(mMultiRules.mpBuff->mMultiSessionRules), &info, sizeof(rF2MultiSessionRules));

  // Copy participants.
  if (info.mNumParticipants >= rF2MappedBufferHeader::MAX_MAPPED_VEHICLES)
    DEBUG_MSG(DebugLevel::Errors, "ERROR: Multi rules exceeded maximum of allowed mapped vehicles.");

  auto const numMultiRulesVehicles = min(info.mNumParticipants, rF2MappedBufferHeader::MAX_MAPPED_VEHICLES);
  for (int i = 0; i < numMultiRulesVehicles; ++i)
    memcpy(&(mMultiRules.mpBuff->mParticipants[i]), &(info.mParticipant[i]), sizeof(rF2MultiSessionParticipant));

  mMultiRules.mpBuff->mBytesUpdatedHint = static_cast<int>(offsetof(rF2MultiRules, mParticipants[numMultiRulesVehicles]));

  mMultiRules.EndUpdate();

  return false;
}


bool SharedMemoryPlugin::GetCustomVariable(long i, CustomVariableV01& var)
{
  DEBUG_MSG2(DebugLevel::Timing, "GetCustomVariable - Invoked:  mCaption - ", var.mCaption);

  if (i == 0) {
    // rF2 will automatically create this variable and default it to 1 (true) unless we create it first, in which case we can choose the default.
    strcpy_s(var.mCaption, " Enabled");
    var.mNumSettings = 2;
    var.mCurrentSetting = 0;
    return true;
  }
  else if (i == 1) {
    strcpy_s(var.mCaption, "DebugOutputLevel");
    var.mNumSettings = 8;
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

  return false;
}


void SharedMemoryPlugin::AccessCustomVariable(CustomVariableV01& var)
{
  DEBUG_MSG(DebugLevel::Timing, "AccessCustomVariable - Invoked.");

  if (_stricmp(var.mCaption, " Enabled") == 0)
    ; // Do nothing; this variable is just for rF2 to know whether to keep the plugin loaded.
  else if (_stricmp(var.mCaption, "DebugOutputLevel") == 0) {
    auto sanitized = min(max(var.mCurrentSetting, 0L), DebugLevel::Verbose);
    SharedMemoryPlugin::msDebugOutputLevel = static_cast<DebugLevel>(sanitized);

    // Remove previous debug output.
    if (SharedMemoryPlugin::msDebugOutputLevel != DebugLevel::Off)
      remove(SharedMemoryPlugin::DEBUG_OUTPUT_FILENAME);
  }
  else if (_stricmp(var.mCaption, "DebugISIInternals") == 0)
    SharedMemoryPlugin::msDebugISIInternals = var.mCurrentSetting != 0;
  else if (_stricmp(var.mCaption, "DedicatedServerMapGlobally") == 0)
    SharedMemoryPlugin::msDedicatedServerMapGlobally = var.mCurrentSetting != 0;
  else if (_stricmp(var.mCaption, "EnableDirectMemoryAccess") == 0)
    SharedMemoryPlugin::msDirectMemoryAccessRequested = var.mCurrentSetting != 0;
}


void SharedMemoryPlugin::GetCustomVariableSetting(CustomVariableV01& var, long i, CustomSettingV01& setting)
{
  DEBUG_MSG(DebugLevel::Timing, "GetCustomVariableSetting - Invoked.");

  if (_stricmp(var.mCaption, " Enabled") == 0) {
    if (i == 0)
      strcpy_s(setting.mName, "False");
    else
      strcpy_s(setting.mName, "True");
  }
  else if (_stricmp(var.mCaption, "DebugOutputLevel") == 0)
    sprintf_s(setting.mName, "%d%%", i);
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
}

////////////////////////////////////////////
// Debug output helpers.
////////////////////////////////////////////

void SharedMemoryPlugin::WriteToAllExampleOutputFiles(const char * const openStr, const char * const msg)
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


void SharedMemoryPlugin::WriteDebugMsg(DebugLevel lvl, const char* const format, ...)
{
  if (lvl > SharedMemoryPlugin::msDebugOutputLevel)
    return;

  va_list argList;
  if (SharedMemoryPlugin::msDebugFile == nullptr) {
    SharedMemoryPlugin::msDebugFile = _fsopen(SharedMemoryPlugin::DEBUG_OUTPUT_FILENAME, "a", _SH_DENYNO);
    setvbuf(SharedMemoryPlugin::msDebugFile, nullptr, _IOFBF, SharedMemoryPlugin::BUFFER_IO_BYTES);
  }

  SYSTEMTIME st = {};
  ::GetLocalTime(&st);

  fprintf(SharedMemoryPlugin::msDebugFile, "%.2d:%.2d:%.2d.%.3d TID:0x%04x  ", st.wHour, st.wMinute, st.wSecond , st.wMilliseconds, ::GetCurrentThreadId());
  if (SharedMemoryPlugin::msDebugFile != nullptr) {
    va_start(argList, format);
    vfprintf(SharedMemoryPlugin::msDebugFile, format, argList);
    va_end(argList);
  }

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
  if (SharedMemoryPlugin::msDebugOutputLevel < DebugLevel::Errors)
    return;

  auto const lastError = ::GetLastError();
  if (lastError == 0)
    return;

  LPSTR messageBuffer = nullptr;
  auto const retChars = ::FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
    nullptr  /*lpSource*/, lastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPSTR>(&messageBuffer), 
    0  /*nSize*/, nullptr  /*argunments*/);

  DEBUG_INT2(DebugLevel::Errors, "Win32 error code:", lastError);
  
  if (retChars > 0 && messageBuffer != nullptr)
    DEBUG_MSG2(DebugLevel::Errors, "Win32 error description:", messageBuffer);

  ::LocalFree(messageBuffer);
}
