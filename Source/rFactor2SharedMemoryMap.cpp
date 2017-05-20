/*
Implementation of rF2 internal state mapping into shared memory buffers.

Author: The Iron Wolf (vleonavicius@hotmail.com)


Acknowledgements:
  This work is based on:
    - rF2 Internals Plugin sample #7 by ISI/S397 found at: https://www.studio-397.com/modding-resources/
    - rF1 Shared Memory Map Plugin by Dan Allongo found at: https://github.com/dallongo/rFactorSharedMemoryMap
    - 3D Math tutorials and samples from all around the internet.  http://www.euclideanspace.com/ and http://www.3dkingdoms.com
      were particularly useful in preserving my sanity.  Same goes to Dan's plugin, that was used as math baseline.


Shared resources:
  This plugin uses double buffering and mutex to allow optional synchronized access.
  Shared resources are:
    - $rFactor2SMMPBuffer1$ - memory mapped file of rF2State structure
    - $rFactor2SMMPBuffer2$ - memory mapped file of rF2State structure
    - Global\$rFactor2SMMPMutex - global mutex


State updates:
  Plugin captures player telemetry and scoring updates in rF2State structure.

  rF2 Internals plugin updates scoring info at ~5FPS via SharedMemoryPlugin::UpdateScoring.  Player vehicle telemetry is
  updated at ~90FPS via SharedMemoryPlugin::UpdateTelemetry.  Current implementation of this plugin artificially
  reduces refresh rate to ~30FPS by default (to reduce CPU load).


Extended state:
  Plugin also tracks and tries to make sense of updates with the goal of exposing additional info not currently available
  via internals model (see SharedMemoryPlugin::ExtendedStateTracker struct).  Tracking happens on _every_ telemetry/scoring
  update for highest precision.

  Currently, damage and invulnerability changes are tracked.


Interpolation:
  Plugin interpolates opponent vehicle positions using quaternion nlerp.  Exact positions are received via
  SharedMemoryPlugin::UpdateScoring and are interpolated during telemetry refreshes.  Future version might instead request
  telemetry for all vehicles on track and use real positions from game.

  Begin/End quaternion setup is done in SharedMemoryPlugin::UpdateScoringHelper, nlerp itself in
  SharedMemoryPlugin::UpdateTelemetryHelper.


Double buffering:
  Plugin maps rF2State structure into two memory mapped files.  Buffers are written to alternatively.
  rF2State::mCurrentRead indicates last updated buffer.

  Buffers are flipped after each telemetry update (if no mutex wait is needed) and scoring update.

  Contents of scoring updates are written to both buffers.  This is done because game state is
  communicated via scoring updates and scoring/telemetry updates are not happening in menu/garage.
  See: SharedMemoryPlugin::SyncBuffers and callers.


Synchronization:
  While mutex is exposed for synchronized access, plugin tries to minimize wait time by retrying
  during telemetry updates (~90FPS) and only waiting for 1ms max during scoring updates
  (and on second telemetry retry), before forcefully flipping buffers.


Configuration file:
  Optional configuration file is supported (primarily for debugging purposes).
  See SharedMemoryPlugin::LoadConfig.


Sample consumption:
  For sample C# client, see Monitor\rF2SMMonitor\rF2SMMonitor\MainForm.cs
*/

#define _USE_MATH_DEFINES                       // for M_PI
#include <math.h>                               // for atan2, sqrt

#include "rFactor2SharedMemoryMap.hpp"          // corresponding header file
#include <stdlib.h>
#include <cstddef>                              // offsetof

double TicksNow();

static double const MILLISECONDS_IN_SECOND = 1000.0;
static double const MICROSECONDS_IN_MILLISECOND = 1000.0;
static double const MICROSECONDS_IN_SECOND = MILLISECONDS_IN_SECOND * MICROSECONDS_IN_MILLISECOND;

DebugLevel SharedMemoryPlugin::msDebugOutputLevel = DebugLevel::Off;
bool SharedMemoryPlugin::msDebugISIInternals = false;
DWORD SharedMemoryPlugin::msMillisMutexWait = 1;

FILE* SharedMemoryPlugin::msDebugFile;
FILE* SharedMemoryPlugin::msIsiTelemetryFile;
FILE* SharedMemoryPlugin::msIsiScoringFile;

// Future/V2:  split into telemetry/scoring/rules etc.
// _Telemetry - possibly no need to interpolate.
// _Scoring
// _Extended
// _Rules
// _Weather
char const* const SharedMemoryPlugin::MM_TELEMETRY_FILE_NAME1 = "$rFactor2SMMP_TelemetryBuffer1$";
char const* const SharedMemoryPlugin::MM_TELEMETRY_FILE_NAME2 = "$rFactor2SMMP_TelemetryBuffer2$";
char const* const SharedMemoryPlugin::MM_TELEMETRY_FILE_ACCESS_MUTEX = R"(Global\$rFactor2SMMP_TelemeteryMutex)";

char const* const SharedMemoryPlugin::MM_SCORING_FILE_NAME1 = "$rFactor2SMMP_ScoringBuffer1$";
char const* const SharedMemoryPlugin::MM_SCORING_FILE_NAME2 = "$rFactor2SMMP_ScoringBuffer2$";
char const* const SharedMemoryPlugin::MM_SCORING_FILE_ACCESS_MUTEX = R"(Global\$rFactor2SMMP_ScoringMutex)";

char const* const SharedMemoryPlugin::MM_EXTENDED_FILE_NAME1 = "$rFactor2SMMP_ExtendedBuffer1$";
char const* const SharedMemoryPlugin::MM_EXTENDED_FILE_NAME2 = "$rFactor2SMMP_ExtendedBuffer2$";
char const* const SharedMemoryPlugin::MM_EXTENDED_FILE_ACCESS_MUTEX = R"(Global\$rFactor2SMMP_ExtendedMutex)";

char const* const SharedMemoryPlugin::CONFIG_FILE_REL_PATH = R"(\UserData\player\rf2smmp.ini)";  // Relative to rF2 root.
char const* const SharedMemoryPlugin::INTERNALS_TELEMETRY_FILENAME = "RF2SMMP_InternalsTelemetryOutput.txt";
char const* const SharedMemoryPlugin::INTERNALS_SCORING_FILENAME = "RF2SMMP_InternalsScoringOutput.txt";
char const* const SharedMemoryPlugin::DEBUG_OUTPUT_FILENAME = "RF2SMMP_DebugOutput.txt";

// plugin information
extern "C" __declspec(dllexport)
const char * __cdecl GetPluginName() { return PLUGIN_NAME_AND_VERSION; }

extern "C" __declspec(dllexport)
PluginObjectType __cdecl GetPluginType() { return(PO_INTERNALS); }

extern "C" __declspec(dllexport)
int __cdecl GetPluginVersion() { return(7); } // InternalsPluginV07 functionality (if you change this return value, you must derive from the appropriate class!)

extern "C" __declspec(dllexport)
PluginObject * __cdecl CreatePluginObject() { return((PluginObject *) new SharedMemoryPlugin); }

extern "C" __declspec(dllexport)
void __cdecl DestroyPluginObject(PluginObject *obj) { delete((SharedMemoryPlugin *)obj); }


//////////////////////////////////////
// SharedMemoryPlugin class
//////////////////////////////////////

SharedMemoryPlugin::SharedMemoryPlugin()
  : mTelemetry(SharedMemoryPlugin::MAX_ASYNC_RETRIES
     , SharedMemoryPlugin::MM_TELEMETRY_FILE_NAME1
     , SharedMemoryPlugin::MM_TELEMETRY_FILE_NAME2
     , SharedMemoryPlugin::MM_TELEMETRY_FILE_ACCESS_MUTEX),
    mScoring(0 /*maxRetries*/
      , SharedMemoryPlugin::MM_SCORING_FILE_NAME1
      , SharedMemoryPlugin::MM_SCORING_FILE_NAME2
      , SharedMemoryPlugin::MM_SCORING_FILE_ACCESS_MUTEX),
    mExtended(0 /*maxRetries*/
      , SharedMemoryPlugin::MM_EXTENDED_FILE_NAME1
      , SharedMemoryPlugin::MM_EXTENDED_FILE_NAME2
      , SharedMemoryPlugin::MM_EXTENDED_FILE_ACCESS_MUTEX)
{
}

void SharedMemoryPlugin::Startup(long version)
{
  // Read configuration .ini if there's one.
  LoadConfig();

  char temp[80] = {};
  sprintf(temp, "-STARTUP- (version %.3f)", (float)version / 1000.0f);
  WriteToAllExampleOutputFiles("w", temp);
#if 0
  mhMap1 = MapMemoryFile(SharedMemoryPlugin::MM_FILE_NAME1, mpBuf1);
  if (mhMap1 == nullptr)
    return;

  mhMap2 = MapMemoryFile(SharedMemoryPlugin::MM_FILE_NAME2, mpBuf2);
  if (mhMap2 == nullptr)
    return;

  mhMutex = CreateMutex(nullptr, FALSE, SharedMemoryPlugin::MM_FILE_ACCESS_MUTEX);
  if (mhMutex == nullptr) {
    DEBUG_MSG(DebugLevel::Errors, "Failed to create mutex");
    return;
  }
#endif
  if (!mTelemetry.Initialize()) {
    DEBUG_MSG(DebugLevel::Errors, "Failed to initialize telemetry mapping");
    return;
  }

  if (!mScoring.Initialize()) {
    DEBUG_MSG(DebugLevel::Errors, "Failed to initialize scoring mapping");
    return;
  }

  if (!mExtended.Initialize()) {
    DEBUG_MSG(DebugLevel::Errors, "Failed to initialize extended mapping");
    return;
  }

  mIsMapped = true;
  if (mIsMapped) {
    ClearState();

    DEBUG_MSG(DebugLevel::Errors, "Files mapped successfully");

    if (SharedMemoryPlugin::msDebugOutputLevel != DebugLevel::Off) {
      char sizeSz[20] = {};
      auto size = static_cast<int>(sizeof(rF2Telemetry));
      _itoa_s(size, sizeSz, 10);
      DEBUG_MSG3(DebugLevel::Errors, "Size of telemetry buffers:", sizeSz, "bytes each.");

      sizeSz[0] = '\0';
      size = static_cast<int>(sizeof(rF2Scoring));
      _itoa_s(size, sizeSz, 10);
      DEBUG_MSG3(DebugLevel::Errors, "Size of scoring buffers:", sizeSz, "bytes each.");
    }
  }

  return;
}

void SharedMemoryPlugin::Shutdown()
{
  WriteToAllExampleOutputFiles("a", "-SHUTDOWN-");

  DEBUG_MSG(DebugLevel::Errors, "Shutting down");

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

#if 0

  // Unmap views and close all handles.
  BOOL ret = FALSE;
  if (mpBuf1 != nullptr) ret = UnmapViewOfFile(mpBuf1);
  if (!ret) DEBUG_MSG(DebugLevel::Errors, "Failed to unmap buffer1");

  if (mpBuf2 != nullptr) ret = UnmapViewOfFile(mpBuf2);
  if (!ret) DEBUG_MSG(DebugLevel::Errors, "Failed to unmap buffer2");

  if (mhMap1 != nullptr) ret = CloseHandle(mhMap1);
  if (!ret) DEBUG_MSG(DebugLevel::Errors, "Failed to close map1 handle");

  if (mhMap2 != nullptr) ret = CloseHandle(mhMap2);
  if (!ret) DEBUG_MSG(DebugLevel::Errors, "Failed to close map2 handle");

  if (mhMutex != nullptr) ret = CloseHandle(mhMutex);
  if (!ret) DEBUG_MSG(DebugLevel::Errors, "Failed to close mutex handle");

  // Telemetry buffers.
  if (mpTelemetryBuf1 != nullptr) ret = UnmapViewOfFile(mpTelemetryBuf1);
  if (!ret) DEBUG_MSG(DebugLevel::Errors, "Failed to unmap telemetry buffer1");

  if (mpTelemetryBuf2 != nullptr) ret = UnmapViewOfFile(mpTelemetryBuf2);
  if (!ret) DEBUG_MSG(DebugLevel::Errors, "Failed to unmap telemetry buffer2");

  if (mhTelemetryMap1 != nullptr) ret = CloseHandle(mhTelemetryMap1);
  if (!ret) DEBUG_MSG(DebugLevel::Errors, "Failed to close telemetry map1 handle");

  if (mhTelemetryMap2 != nullptr) ret = CloseHandle(mhTelemetryMap2);
  if (!ret) DEBUG_MSG(DebugLevel::Errors, "Failed to close telemetry map2 handle");

  if (mhTelemetryMutex != nullptr) ret = CloseHandle(mhTelemetryMutex);
  if (!ret) DEBUG_MSG(DebugLevel::Errors, "Failed to close telemetry mutex handle");

  mpBuf1 = nullptr;
  mpBuf2 = nullptr;
  mhMap1 = nullptr;
  mhMap2 = nullptr;
  mhMutex = nullptr;
  mpBufCurWrite = nullptr;

  mpTelemetryBuf1 = nullptr;
  mpTelemetryBuf2 = nullptr;
  mhTelemetryMap1 = nullptr;
  mhTelemetryMap2 = nullptr;
  mhTelemetryMutex = nullptr;
  mpCurTelemetryBufWrite = nullptr;
#endif

  mTelemetry.ClearState();
  mTelemetry.ReleaseResources();

  mScoring.ClearState();
  mScoring.ReleaseResources();

  mExtended.ClearState();
  mExtended.ReleaseResources();

  mIsMapped = false;
}

void SharedMemoryPlugin::ClearTimingsAndCounters()
{
  mLastTelemetryUpdateTicks = 0.0;
  mLastScoringUpdateTicks = 0.0;
  mDelta = 0.0;

  mLastTelemetryUpdateET = 0.0;
  mLastScoringUpdateET = 0.0;

  mTelemetryUpdateInProgress = false;
  mCurTelemetryVehicleIndex = 0;

  mScoringNumVehicles = 0;

  memset(mParticipantTelemetryUpdated, 0, sizeof(mParticipantTelemetryUpdated));
}


void SharedMemoryPlugin::ClearState()
{
  if (mIsMapped) {
#if 0
    mRetryFlip = false;
    mRetriesLeft = 0;

    auto ret = WaitForSingleObject(mhMutex, SharedMemoryPlugin::msMillisMutexWait);

    memset(mpBuf1, 0, sizeof(rF2State));
    strcpy_s(mpBuf1->mVersion, SHARED_MEMORY_VERSION);
    mpBuf1->mCurrentRead = true;

    memset(mpBuf2, 0, sizeof(rF2State));
    strcpy_s(mpBuf2->mVersion, SHARED_MEMORY_VERSION);
    mpBuf2->mCurrentRead = false;

    mpBufCurWrite = mpBuf2;
    assert(!mpBufCurWrite->mCurrentRead);

    if (ret == WAIT_OBJECT_0)
      ReleaseMutex(mhMutex);
    else if (ret == WAIT_TIMEOUT)
      DEBUG_MSG(DebugLevel::Warnings, "WARNING: - Timed out while waiting on mutex.");
    else
      DEBUG_MSG(DebugLevel::Errors, "ERROR: - wait on mutex failed.");
#endif

    mTelemetry.ClearState();
    mScoring.ClearState();
    mExtended.ClearState();
  }

  ClearTimingsAndCounters();

  mExtStateTracker.ResetDamageState();
}

void SharedMemoryPlugin::StartSession()
{
  WriteToAllExampleOutputFiles("a", "--STARTSESSION--");

  ClearState();
}


void SharedMemoryPlugin::EndSession()
{
  WriteToAllExampleOutputFiles("a", "--ENDSESSION--");

  ClearState();
}

// TODO: move to extended state.
void SharedMemoryPlugin::UpdateInRealtimeFC(bool inRealTime)
{
  if (mIsMapped) {
#if 0
    assert(mpBuf1->mInRealtimeFC == mpBuf2->mInRealtimeFC);

    // Make sure we're updating latest buffer (pickup telemetry update).
    if (!mRetryFlip) // If flip wasn't successful, that means write buffer is already up to date.
      SyncBuffers(true /*telemetryOnly*/);
    else
      DEBUG_MSG(DebugLevel::Synchronization, "Skipped initial write buffer sync due to retry mode.");

    auto pBuf = mpBufCurWrite;
    assert(!pBuf->mCurrentRead);

    // Update current write buffer.
    pBuf->mInRealtimeFC = inRealTime;

    // Make it available to readers.
    FlipBuffers(BufferType::Extended);

    assert(pBuf != mpBufCurWrite);

    pBuf = mpBufCurWrite;
    assert(!pBuf->mCurrentRead);

    // Update current write buffer.
    pBuf->mInRealtimeFC = inRealTime;

    assert(mpBuf1->mInRealtimeFC == mpBuf2->mInRealtimeFC);
#endif
  }
}

// TODO: extended state
void SharedMemoryPlugin::EnterRealtime()
{
  // start up timer every time we enter realtime
  WriteToAllExampleOutputFiles("a", "---ENTERREALTIME---");

  UpdateInRealtimeFC(true /*inRealtime*/);

  mInRealTimeLastFunctionCall = true;
}

// TODO: extended state
void SharedMemoryPlugin::ExitRealtime()
{
  WriteToAllExampleOutputFiles("a", "---EXITREALTIME---");

  UpdateInRealtimeFC(false /*inRealtime*/);

  mInRealTimeLastFunctionCall = false;
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


void SharedMemoryPlugin::TelemetryTraceSkipUpdate(TelemInfoV01 const& info) const
{
  if (SharedMemoryPlugin::msDebugOutputLevel >= DebugLevel::Timing) {
    char msg[512] = {};
    sprintf(msg, "Skipping update due to no changes in the input data.  New ET: %f  Prev ET:%f", info.mElapsedTime, mLastTelemetryUpdateET);
    DEBUG_MSG(DebugLevel::Timing, msg);

    if (info.mPos.x != mTelemetry.mpCurReadBuf->mVehicles->mPos.x
      || info.mPos.y != mTelemetry.mpCurReadBuf->mVehicles->mPos.y
      || info.mPos.z != mTelemetry.mpCurReadBuf->mVehicles->mPos.z)
    {
      sprintf(msg, "Pos Mismatch on skip update!!!  New ET: %f  Prev ET:%f  Prev Pos: %f %f %f  New Pos %f %f %f", info.mElapsedTime, mLastTelemetryUpdateET,
        info.mPos.x, info.mPos.y, info.mPos.z,
        mTelemetry.mpCurReadBuf->mVehicles->mPos.x,
        mTelemetry.mpCurReadBuf->mVehicles->mPos.y,
        mTelemetry.mpCurReadBuf->mVehicles->mPos.z);
      DEBUG_MSG(DebugLevel::Timing, msg);
    }
  }
}


void SharedMemoryPlugin::TelemetryTraceBeginUpdate()
{
  auto ticksNow = 0.0;
  if (SharedMemoryPlugin::msDebugOutputLevel >= DebugLevel::Timing) {
    ticksNow = TicksNow();
    auto const delta = ticksNow - mLastTelemetryUpdateTicks;

    if (mTelemetry.mpCurWriteBuf == mTelemetry.mpBuf1)
      DEBUG_FLOAT2(DebugLevel::Timing, "TELEMETRY - Begin Update: Buffer 1.  Delta since last update:", delta / MICROSECONDS_IN_SECOND);
    else
      DEBUG_FLOAT2(DebugLevel::Timing, "TELEMETRY - Begin Update: Buffer 2.  Delta since last update:", delta / MICROSECONDS_IN_SECOND);
  }

  mLastTelemetryUpdateTicks = ticksNow;
}


void SharedMemoryPlugin::TelemetryTraceEndUpdate(int numVehiclesInChain) const
{
  if (SharedMemoryPlugin::msDebugOutputLevel >= DebugLevel::Timing) {
    auto const ticksNow = TicksNow();
    auto const deltaSysTimeMicroseconds = ticksNow - mLastTelemetryUpdateTicks;

    char msg[512] = {};
    sprintf(msg, "End Update.  Telemetry chain update took %f:  Vehicles in chain: %d", deltaSysTimeMicroseconds / MICROSECONDS_IN_SECOND, numVehiclesInChain);

    DEBUG_MSG(DebugLevel::Timing, msg);
  }
}


/*
rF2 sends telemetry updates for each vehicle.  The problem is that I do not know when all vehicles received an update.
Below I am trying to flip buffers per-frame, where frame means all vehicles received telemetry update.

I am detecting frame end in two ways:
  * Count vehicles from mID == 0 to mScoringNumVehicles.
  * As a backup for case where mID == 0 drops out of the session, I use mParticipantTelemetryUpdated index to detect the loop.

There's one more check that can be done - 10ms since update chain start will also work, but I am trying to avoid call to QPC.

Note that I am seeing different ET for vehicles in frame (typically no more than 2 values), no idea WTF that is.
*/
void SharedMemoryPlugin::UpdateTelemetry(TelemInfoV01 const& info)
{
  WriteTelemetryInternals(info);
  
  if (!mIsMapped)
    return;

  // TODO check if scoring update is ahead of telemetry, if it is we need to force flip if pending.
  auto const partiticpantIndex = min(info.mID, MAX_PARTICIPANT_SLOTS - 1);
  auto const alreadyUpdated = mParticipantTelemetryUpdated[partiticpantIndex];
  if (info.mID == 0 || alreadyUpdated) {
    if (info.mElapsedTime <= mLastTelemetryUpdateET) {
      TelemetryTraceSkipUpdate(info);
      assert(!mTelemetryUpdateInProgress);
      goto skipUpdate;
    }

    TelemetryTraceBeginUpdate();

    // Ok, this is the new sequence of telemetry updates, and it contains updated data.
    if (mCurTelemetryVehicleIndex != 0)
      DEBUG_INT2(DebugLevel::Warnings, "Previous update ended at:", mCurTelemetryVehicleIndex);

    if (alreadyUpdated)
      DEBUG_INT2(DebugLevel::Timing, "Update chain started at:", info.mID);

    // Update extended state.
    // Since I do not want to miss impact data, and it is not accumulated in any way
    // I am aware of in rF2 internals, process on every call (for player vehicle.
    if (info.mID == 0)
      mExtStateTracker.ProcessTelemetryUpdate(info);

    // Start new telemetry update chain.
    mLastTelemetryUpdateET = info.mElapsedTime;
    mTelemetryUpdateInProgress = true;
    mCurTelemetryVehicleIndex = 0;
    memset(mParticipantTelemetryUpdated, 0, sizeof(mParticipantTelemetryUpdated));
    mTelemetry.mpCurWriteBuf->mNumVehicles = mScoringNumVehicles;
  }

  if (mTelemetryUpdateInProgress) {
    auto const partiticpantIndex = min(info.mID, MAX_PARTICIPANT_SLOTS - 1);
    assert(mParticipantTelemetryUpdated[partiticpantIndex] == false);
    mParticipantTelemetryUpdated[partiticpantIndex] = true;

    memcpy(&(mTelemetry.mpCurWriteBuf->mVehicles[mCurTelemetryVehicleIndex]), &info, sizeof(rF2VehicleTelemetry));
    ++mCurTelemetryVehicleIndex;

    // See if this is the last vehicle to update.
    if (mCurTelemetryVehicleIndex >= mTelemetry.mpCurWriteBuf->mNumVehicles
      || mCurTelemetryVehicleIndex >= rF2Telemetry::MAX_MAPPED_VEHICLES) {
      auto const numVehiclesInChain = mCurTelemetryVehicleIndex;
      mTelemetryUpdateInProgress = false;
      mCurTelemetryVehicleIndex = 0;
      memset(mParticipantTelemetryUpdated, 0, sizeof(mParticipantTelemetryUpdated));

      if (mLastTelemetryUpdateET <= mLastScoringUpdateET) {
        // If scoring update is ahead of this telemetry update, force flip.
        DEBUG_MSG(DebugLevel::Timing, "Force flip due to: mLastTelemetryUpdateET <= mLastScoringUpdateET.");
        mTelemetry.FlipBuffers();
      }
      else if (mTelemetry.AsyncRetriesLeft() > 0) {
        // Otherwise, try buffer flip.
        mTelemetry.TryFlipBuffers();

        // Print msg about buffer flip failure or success.
        if (mTelemetry.RetryPending())
          DEBUG_INT2(DebugLevel::Synchronization, "Buffer flip failed, retries remaining:", mTelemetry.AsyncRetriesLeft());
      } 
      else {
        // Force flip if no more retries are left
        assert(mTelemetry.AsyncRetriesLeft() == 0);
        mTelemetry.FlipBuffers();
        DEBUG_MSG(DebugLevel::Synchronization, "Force flip due to retry limit exceeded.");
      }

      TelemetryTraceEndUpdate(numVehiclesInChain);
    }

    return;
  }

skipUpdate:
  // If there's flip pending, retry.
  if (mTelemetry.RetryPending()) {
    assert(!mTelemetryUpdateInProgress);
    // Retry/flip if pending.
    if (mTelemetry.AsyncRetriesLeft() > 0) {
      DEBUG_MSG(DebugLevel::Synchronization, "Retrying incomplete buffer flip.");
      mTelemetry.TryFlipBuffers();
      if (mTelemetry.RetryPending())
        DEBUG_INT2(DebugLevel::Synchronization, "Buffer flip failed, retries remaining:", mTelemetry.AsyncRetriesLeft());
      else
        DEBUG_MSG(DebugLevel::Synchronization, "Buffer flip succeeded.");
    } 
    else {
      assert(mTelemetry.AsyncRetriesLeft() == 0);
      mTelemetry.FlipBuffers();
      DEBUG_MSG(DebugLevel::Synchronization, "Force flip due to retry limit exceeded.");
    }
  }
}


void SharedMemoryPlugin::UpdateTelemetryHelper(double const ticksNow, TelemInfoV01 const& info)
{
  mDelta = (ticksNow - mLastScoringUpdateTicks) / MICROSECONDS_IN_SECOND;
}


void SharedMemoryPlugin::UpdateScoringHelper(double const ticksNow, ScoringInfoV01 const& info)
{
  mLastScoringUpdateTicks = ticksNow;
#if 0
  static double scoringUpdatePrev = 0.0;
  auto const delta = mLastScoringUpdate - scoringUpdatePrev;
  scoringUpdatePrev = mLastScoringUpdate;

  auto pBuf = mpBufCurWrite;
  assert(!pBuf->mCurrentRead);

  DEBUG_FLOAT2(DebugLevel::Verbose, "Scoring ticks:", ticksNow);
  if (pBuf == mpBuf1)
    DEBUG_FLOAT2(DebugLevel::Timing, "Update Scoring Buffer 1:", delta / MICROSECONDS_IN_SECOND);
  else
    DEBUG_FLOAT2(DebugLevel::Timing, "Update Scoring Buffer 2:", delta / MICROSECONDS_IN_SECOND);

  pBuf->mDeltaTime = 0.0;

  // Update current player vehicle speeds, position and orientation
  if (info.mNumVehicles > 0) {
    assert(pBuf->mID == info.mVehicle[0].mID);

    // Since there's a gap between Telemetry and Scoring upates
    // update elapsed time to match most current value.
    pBuf->mElapsedTime = info.mCurrentET;

    // Below members have identical meaning in TelelmInfoV01 and VehicleScoringInfoV01
    // so update them.
    pBuf->mLapStartET = info.mVehicle[0].mLapStartET;
    pBuf->mLapNumber = info.mVehicle[0].mTotalLaps;

    // Update position and orientation.
    pBuf->mPos = { info.mVehicle[0].mPos.x, info.mVehicle[0].mPos.y, info.mVehicle[0].mPos.z };
    pBuf->mLocalVel = { info.mVehicle[0].mLocalVel.x, info.mVehicle[0].mLocalVel.y, info.mVehicle[0].mLocalVel.z };
    pBuf->mLocalAccel = { info.mVehicle[0].mLocalAccel.x, info.mVehicle[0].mLocalAccel.y, info.mVehicle[0].mLocalAccel.z };

    pBuf->mSpeed = sqrt((info.mVehicle[0].mLocalVel.x * info.mVehicle[0].mLocalVel.x) +
      (info.mVehicle[0].mLocalVel.y * info.mVehicle[0].mLocalVel.y) +
      (info.mVehicle[0].mLocalVel.z * info.mVehicle[0].mLocalVel.z));

    pBuf->mLocalRot = { info.mVehicle[0].mLocalRot.x, info.mVehicle[0].mLocalRot.y, info.mVehicle[0].mLocalRot.z };
    pBuf->mLocalRotAccel = { info.mVehicle[0].mLocalRotAccel.x, info.mVehicle[0].mLocalRotAccel.y, info.mVehicle[0].mLocalRotAccel.z };

    for (int row = 0; row < OriMat::NumRows; ++row)
      pBuf->mOri[row] = { info.mVehicle[0].mOri[row].x, info.mVehicle[0].mOri[row].y, info.mVehicle[0].mOri[row].z };
  }
#endif

  // Update Extended state.
  // TODO:
  //mExtStateTracker.FlushToBuffer(pBuf);
}


void SharedMemoryPlugin::SyncBuffers(bool telemetryOnly)
{
#if 0
  // Copy contents of the current read buffer to write buffer.
  auto pBuf = mpBufCurWrite;
  assert(!mpBufCurWrite->mCurrentRead);

  rF2State* pSrcBuf = nullptr;
  rF2State* pDstBuf = nullptr;
  if (pBuf == mpBuf1) {
    DEBUG_MSG(DebugLevel::Timing, telemetryOnly ? "Syncing write buffer 1 (telemetry)" : "Syncing write buffer 1");
    pSrcBuf = mpBuf2;
    pDstBuf = mpBuf1;
  }
  else if (pBuf == mpBuf2) {
    DEBUG_MSG(DebugLevel::Timing, telemetryOnly ? "Syncing write buffer 2 (telemetry)" : "Syncing write buffer 2");
    pSrcBuf = mpBuf1;
    pDstBuf = mpBuf2;
  }

  // For telemetry only case, copy read buffer into write buffer up to mSession variable.
  // For full sync case, copy whole buffer taking into account how many active vehicles are in there.
  auto const bytes = telemetryOnly 
    ? offsetof(rF2State, mSession) 
    : offsetof(rF2State, mVehicles) + pSrcBuf->mNumVehicles * sizeof(rF2VehScoringInfo);

  memcpy(pDstBuf, pSrcBuf, bytes);
  pDstBuf->mCurrentRead = false;
  assert(pSrcBuf->mCurrentRead);

  if (!telemetryOnly)
    assert((rF2State::MAX_VSI_SIZE - pSrcBuf->mNumVehicles) * sizeof(rF2VehScoringInfo) + bytes == sizeof(rF2State));
#endif
}


void SharedMemoryPlugin::ScoringTraceBeginUpdate()
{
  auto ticksNow = 0.0;
  if (SharedMemoryPlugin::msDebugOutputLevel >= DebugLevel::Timing) {
    ticksNow = TicksNow();
    auto const delta = ticksNow - mLastScoringUpdateTicks;

    if (mScoring.mpCurWriteBuf == mScoring.mpBuf1)
      DEBUG_FLOAT2(DebugLevel::Timing, "SCORING - Begin Update: Buffer 1.  Delta since last update:", delta / MICROSECONDS_IN_SECOND);
    else
      DEBUG_FLOAT2(DebugLevel::Timing, "SCORING - Begin Update: Buffer 2.  Delta since last update:", delta / MICROSECONDS_IN_SECOND);

    char msg[512] = {};
    sprintf(msg, "Scoring ET:%f  Telemetry ET:%f", mLastScoringUpdateET, mLastTelemetryUpdateET);
    DEBUG_MSG(DebugLevel::Timing, msg);
  }

  mLastScoringUpdateTicks = ticksNow;
}


void SharedMemoryPlugin::UpdateScoring(ScoringInfoV01 const& info)
{
  if (mIsMapped) {
    mScoringNumVehicles = info.mNumVehicles;
    mLastScoringUpdateET = info.mCurrentET;

    ScoringTraceBeginUpdate();

    if (mTelemetry.RetryPending()) {
      DEBUG_MSG(DebugLevel::Synchronization, "Telemetry force flip due to retry pending.");
      mTelemetry.FlipBuffers();
    }

    // Below apparently never happens, but let's keep it in case there's a regression in the game.
    if (mLastScoringUpdateET > mLastTelemetryUpdateET)
      DEBUG_MSG(DebugLevel::Warnings, "WARNING: Scoring update is ahead of telemetry.");

    memcpy(&(mScoring.mpCurWriteBuf->mScoringInfo), &info, sizeof(rF2ScoringInfo));

    for (int i = 0; i < info.mNumVehicles; ++i)
      memcpy(&(mScoring.mpCurWriteBuf->mVehicles[i]), &(info.mVehicle[i]), sizeof(rF2VehicleScoring));

    mScoring.FlipBuffers();

    // Update extended state.
    mExtStateTracker.ProcessScoringUpdate(info);

#if 0
    auto const ticksNow = TicksNow();

    // Make sure we're updating latest buffer (pickup telemetry update).
    if (!mRetryFlip) // If flip wasn't successful, that means write buffer is already up to date.
      SyncBuffers(true /*telemetryOnly*/);
    else
      DEBUG_MSG(DebugLevel::Synchronization, "Skipped initial write buffer sync due to retry mode.");

    // Update extended state.
    mExtStateTracker.ProcessScoringUpdate(info);

    // Update write buffer with scoring info.
    UpdateScoringHelper(ticksNow, info);
    
    // Make it available to readers.
    FlipBuffers(BufferType::Scoring);

    // Update write buffer with scoring info (to keep scoring info between buffers in sync).
    SyncBuffers(false /*telemetryOnly*/);
#endif
  }

  WriteScoringInternals(info);
}

bool SharedMemoryPlugin::WantsToDisplayMessage(MessageInfoV01& msgInfo)
{
  // Looks like this is write only API, can't read current text in MC
  /*
  msgInfo.mText[0] = 'H';
  msgInfo.mText[1] = 'e';
  msgInfo.mText[2] = 'l';
  msgInfo.mText[3] = 'l';
  msgInfo.mText[4] = 'o';
  msgInfo.mText[5] = '\0';
  msgInfo.mDestination = 0;
  DEBUG_MSG(DebugLevel::Errors, msgInfo.mText);
  */
  return false;
}

void SharedMemoryPlugin::ThreadStarted(long type)
{
  DEBUG_MSG(DebugLevel::Errors, type == 0 ? "Multimedia thread started" : "Simulation thread started");

}

void SharedMemoryPlugin::ThreadStopping(long type)
{
  DEBUG_MSG(DebugLevel::Errors, type == 0 ? "Multimedia thread stopped" : "Simulation thread stopped");
}

// Called roughly every 300ms.
//double ticksPrev = 0.0;
bool SharedMemoryPlugin::AccessTrackRules(TrackRulesV01& info)
{
/*  auto const ticksNow = TicksNow();

  DEBUG_FLOAT2(DebugLevel::Errors, "Update Rules:", (ticksNow - ticksPrev) / MICROSECONDS_IN_SECOND);
  ticksPrev = ticksNow;

  DEBUG_MSG(DebugLevel::Errors, info.mMessage);
  DEBUG_INT2(DebugLevel::Errors, "Slot 1 ", info.mParticipant[0].mID);
  DEBUG_MSG(DebugLevel::Errors, info.mParticipant[0].mMessage);
  DEBUG_INT2(DebugLevel::Errors, "Frozen Order", info.mParticipant[0].mFrozenOrder);
  DEBUG_INT2(DebugLevel::Errors, "Formation place", info.mParticipant[0].mPlace);
  DEBUG_INT2(DebugLevel::Errors, "Slot 2 ", info.mParticipant[1].mID);
  DEBUG_MSG(DebugLevel::Errors, info.mParticipant[1].mMessage);
  DEBUG_INT2(DebugLevel::Errors, "Frozen Order", info.mParticipant[1].mFrozenOrder);
  DEBUG_INT2(DebugLevel::Errors, "Formation place", info.mParticipant[1].mPlace);*/

  return false;
}

bool SharedMemoryPlugin::AccessPitMenu(PitMenuV01& info)
{
  //DEBUG_MSG(DebugLevel::Errors, "Info");
  //DEBUG_MSG(DebugLevel::Errors, info.mChoiceString);

  return false;
}

////////////////////////////////////////////
// Config, files and debugging output helpers.
////////////////////////////////////////////
void SharedMemoryPlugin::LoadConfig()
{
  char wd[MAX_PATH] = {};
  GetCurrentDirectory(MAX_PATH, wd);

  auto iniPath = lstrcatA(wd, SharedMemoryPlugin::CONFIG_FILE_REL_PATH);

  auto outputLvl = GetPrivateProfileInt("config", "debugOutputLevel", 0, iniPath);
  if (outputLvl > DebugLevel::Verbose)
    outputLvl = 0;

  msDebugOutputLevel = static_cast<DebugLevel>(outputLvl);
  if (msDebugOutputLevel > 0)
    remove(SharedMemoryPlugin::DEBUG_OUTPUT_FILENAME);  // Remove previous output.

  msDebugISIInternals = GetPrivateProfileInt("config", "debugISIInternals", 0, iniPath) != 0;

  DEBUG_MSG2(DebugLevel::Verbose, "Loaded config from:", iniPath);
}

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

  if (SharedMemoryPlugin::msDebugFile != nullptr) {
    va_start(argList, format);
    vfprintf(SharedMemoryPlugin::msDebugFile, format, argList);
    va_end(argList);
  }

  // Flush periodically for low volume messages.
  static __int64 lastFlushTicks = 0LL;
  auto const ticksNow = GetTickCount64();
  if ((ticksNow - lastFlushTicks) / MILLISECONDS_IN_SECOND > DEBUG_IO_FLUSH_PERIOD_SECS) {
    fflush(SharedMemoryPlugin::msDebugFile);
    lastFlushTicks = ticksNow;
  }
}

void SharedMemoryPlugin::WriteTelemetryInternals(TelemInfoV01 const& info)
{
  if (!SharedMemoryPlugin::msDebugISIInternals)
    return;

  // Use the incoming data, for now I'll just write some of it to a file to a) make sure it
  if (SharedMemoryPlugin::msIsiTelemetryFile == nullptr) {
    SharedMemoryPlugin::msIsiTelemetryFile = _fsopen(SharedMemoryPlugin::INTERNALS_TELEMETRY_FILENAME, "a", _SH_DENYNO);
    setvbuf(SharedMemoryPlugin::msIsiTelemetryFile, nullptr, _IOFBF, SharedMemoryPlugin::BUFFER_IO_BYTES);
  }

  // is working, and b) explain the coordinate system a little bit (see header for more info)
  auto fo = SharedMemoryPlugin::msIsiTelemetryFile;
  if (fo != nullptr)
  {
    // Delta time is variable, as we send out the info once per frame
    fprintf(fo, "DT=%.4f  ET=%.4f\n", info.mDeltaTime, info.mElapsedTime);
    fprintf(fo, "Lap=%d StartET=%.3f\n", info.mLapNumber, info.mLapStartET);
    fprintf(fo, "Vehicle=%s\n", info.mVehicleName);
    fprintf(fo, "Track=%s\n", info.mTrackName);
    fprintf(fo, "Pos=(%.3f,%.3f,%.3f)\n", info.mPos.x, info.mPos.y, info.mPos.z);

    // Forward is roughly in the -z direction (although current pitch of car may cause some y-direction velocity)
    fprintf(fo, "LocalVel=(%.2f,%.2f,%.2f)\n", info.mLocalVel.x, info.mLocalVel.y, info.mLocalVel.z);
    fprintf(fo, "LocalAccel=(%.1f,%.1f,%.1f)\n", info.mLocalAccel.x, info.mLocalAccel.y, info.mLocalAccel.z);

    // Orientation matrix is left-handed
    fprintf(fo, "[%6.3f,%6.3f,%6.3f]\n", info.mOri[0].x, info.mOri[0].y, info.mOri[0].z);
    fprintf(fo, "[%6.3f,%6.3f,%6.3f]\n", info.mOri[1].x, info.mOri[1].y, info.mOri[1].z);
    fprintf(fo, "[%6.3f,%6.3f,%6.3f]\n", info.mOri[2].x, info.mOri[2].y, info.mOri[2].z);
    fprintf(fo, "LocalRot=(%.3f,%.3f,%.3f)\n", info.mLocalRot.x, info.mLocalRot.y, info.mLocalRot.z);
    fprintf(fo, "LocalRotAccel=(%.2f,%.2f,%.2f)\n", info.mLocalRotAccel.x, info.mLocalRotAccel.y, info.mLocalRotAccel.z);

    // Vehicle status
    fprintf(fo, "Gear=%d RPM=%.1f RevLimit=%.1f\n", info.mGear, info.mEngineRPM, info.mEngineMaxRPM);
    fprintf(fo, "Water=%.1f Oil=%.1f\n", info.mEngineWaterTemp, info.mEngineOilTemp);
    fprintf(fo, "ClutchRPM=%.1f\n", info.mClutchRPM);

    // Driver input
    fprintf(fo, "UnfilteredThrottle=%.1f%%\n", 100.0 * info.mUnfilteredThrottle);
    fprintf(fo, "UnfilteredBrake=%.1f%%\n", 100.0 * info.mUnfilteredBrake);
    fprintf(fo, "UnfilteredSteering=%.1f%%\n", 100.0 * info.mUnfilteredSteering);
    fprintf(fo, "UnfilteredClutch=%.1f%%\n", 100.0 * info.mUnfilteredClutch);

    // Filtered input
    fprintf(fo, "FilteredThrottle=%.1f%%\n", 100.0 * info.mFilteredThrottle);
    fprintf(fo, "FilteredBrake=%.1f%%\n", 100.0 * info.mFilteredBrake);
    fprintf(fo, "FilteredSteering=%.1f%%\n", 100.0 * info.mFilteredSteering);
    fprintf(fo, "FilteredClutch=%.1f%%\n", 100.0 * info.mFilteredClutch);

    // Misc
    fprintf(fo, "SteeringShaftTorque=%.1f\n", info.mSteeringShaftTorque);
    fprintf(fo, "Front3rdDeflection=%.3f Rear3rdDeflection=%.3f\n", info.mFront3rdDeflection, info.mRear3rdDeflection);

    // Aerodynamics
    fprintf(fo, "FrontWingHeight=%.3f FrontRideHeight=%.3f RearRideHeight=%.3f\n", info.mFrontWingHeight, info.mFrontRideHeight, info.mRearRideHeight);
    fprintf(fo, "Drag=%.1f FrontDownforce=%.1f RearDownforce=%.1f\n", info.mDrag, info.mFrontDownforce, info.mRearDownforce);

    // Other
    fprintf(fo, "Fuel=%.1f ScheduledStops=%d Overheating=%d Detached=%d\n", info.mFuel, info.mScheduledStops, info.mOverheating, info.mDetached);
    fprintf(fo, "Dents=(%d,%d,%d,%d,%d,%d,%d,%d)\n", info.mDentSeverity[0], info.mDentSeverity[1], info.mDentSeverity[2], info.mDentSeverity[3],
      info.mDentSeverity[4], info.mDentSeverity[5], info.mDentSeverity[6], info.mDentSeverity[7]);
    fprintf(fo, "LastImpactET=%.1f Mag=%.1f, Pos=(%.1f,%.1f,%.1f)\n", info.mLastImpactET, info.mLastImpactMagnitude,
      info.mLastImpactPos.x, info.mLastImpactPos.y, info.mLastImpactPos.z);

    // Wheels
    for (long i = 0; i < 4; ++i)
    {
      const TelemWheelV01 &wheel = info.mWheel[i];
      fprintf(fo, "Wheel=%s\n", (i == 0) ? "FrontLeft" : (i == 1) ? "FrontRight" : (i == 2) ? "RearLeft" : "RearRight");
      fprintf(fo, " SuspensionDeflection=%.3f RideHeight=%.3f\n", wheel.mSuspensionDeflection, wheel.mRideHeight);
      fprintf(fo, " SuspForce=%.1f BrakeTemp=%.1f BrakePressure=%.3f\n", wheel.mSuspForce, wheel.mBrakeTemp, wheel.mBrakePressure);
      fprintf(fo, " ForwardRotation=%.1f Camber=%.3f\n", -wheel.mRotation, wheel.mCamber);
      fprintf(fo, " LateralPatchVel=%.2f LongitudinalPatchVel=%.2f\n", wheel.mLateralPatchVel, wheel.mLongitudinalPatchVel);
      fprintf(fo, " LateralGroundVel=%.2f LongitudinalGroundVel=%.2f\n", wheel.mLateralGroundVel, wheel.mLongitudinalGroundVel);
      fprintf(fo, " LateralForce=%.1f LongitudinalForce=%.1f\n", wheel.mLateralForce, wheel.mLongitudinalForce);
      fprintf(fo, " TireLoad=%.1f GripFract=%.3f TirePressure=%.1f\n", wheel.mTireLoad, wheel.mGripFract, wheel.mPressure);
      fprintf(fo, " TireTemp(l/c/r)=%.1f/%.1f/%.1f\n", wheel.mTemperature[0], wheel.mTemperature[1], wheel.mTemperature[2]);
      fprintf(fo, " Wear=%.3f TerrainName=%s SurfaceType=%d\n", wheel.mWear, wheel.mTerrainName, wheel.mSurfaceType);
      fprintf(fo, " Flat=%d Detached=%d\n", wheel.mFlat, wheel.mDetached);
    }

    // Compute some auxiliary info based on the above
    TelemVect3 forwardVector = { -info.mOri[0].z, -info.mOri[1].z, -info.mOri[2].z };
    TelemVect3    leftVector = { info.mOri[0].x,  info.mOri[1].x,  info.mOri[2].x };

    // These are normalized vectors, and remember that our world Y coordinate is up.  So you can
    // determine the current pitch and roll (w.r.t. the world x-z plane) as follows:
    const double pitch = atan2(forwardVector.y, sqrt((forwardVector.x * forwardVector.x) + (forwardVector.z * forwardVector.z)));
    const double  roll = atan2(leftVector.y, sqrt((leftVector.x *    leftVector.x) + (leftVector.z *    leftVector.z)));
    const double radsToDeg = 57.296;
    fprintf(fo, "Pitch = %.1f deg, Roll = %.1f deg\n", pitch * radsToDeg, roll * radsToDeg);

    const double metersPerSec = sqrt((info.mLocalVel.x * info.mLocalVel.x) +
      (info.mLocalVel.y * info.mLocalVel.y) +
      (info.mLocalVel.z * info.mLocalVel.z));
    fprintf(fo, "Speed = %.1f KPH, %.1f MPH\n\n", metersPerSec * 3.6, metersPerSec * 2.237);

    // Close file
    fclose(fo);
  }
}


void SharedMemoryPlugin::WriteScoringInternals(ScoringInfoV01 const& info)
{
  if (!SharedMemoryPlugin::msDebugISIInternals)
    return;

  // Note: function is called twice per second now (instead of once per second in previous versions)
  if (SharedMemoryPlugin::msIsiScoringFile == nullptr) {
    SharedMemoryPlugin::msIsiScoringFile = _fsopen(SharedMemoryPlugin::INTERNALS_SCORING_FILENAME, "a", _SH_DENYNO);
    setvbuf(SharedMemoryPlugin::msIsiScoringFile, nullptr, _IOFBF, SharedMemoryPlugin::BUFFER_IO_BYTES);
  }

  auto fo = SharedMemoryPlugin::msIsiScoringFile;
  if (fo != nullptr)
  {
    // Print general scoring info
    fprintf(fo, "TrackName=%s\n", info.mTrackName);
    fprintf(fo, "Session=%d NumVehicles=%d CurET=%.3f\n", info.mSession, info.mNumVehicles, info.mCurrentET);
    fprintf(fo, "EndET=%.3f MaxLaps=%d LapDist=%.1f\n", info.mEndET, info.mMaxLaps, info.mLapDist);

    // Note that only one plugin can use the stream (by enabling scoring updates) ... sorry if any clashes result
    fprintf(fo, "START STREAM\n");
    const char *ptr = info.mResultsStream;
    while (*ptr != '\0')
      fputc(*ptr++, fo);
    fprintf(fo, "END STREAM\n");

    // New version 2 stuff
    fprintf(fo, "GamePhase=%d YellowFlagState=%d SectorFlags=(%d,%d,%d)\n", info.mGamePhase, info.mYellowFlagState,
      info.mSectorFlag[0], info.mSectorFlag[1], info.mSectorFlag[2]);
    fprintf(fo, "InRealtime=%d StartLight=%d NumRedLights=%d\n", info.mInRealtime, info.mStartLight, info.mNumRedLights);
    fprintf(fo, "PlayerName=%s PlrFileName=%s\n", info.mPlayerName, info.mPlrFileName);
    fprintf(fo, "DarkCloud=%.2f Raining=%.2f AmbientTemp=%.1f TrackTemp=%.1f\n", info.mDarkCloud, info.mRaining, info.mAmbientTemp, info.mTrackTemp);
    fprintf(fo, "Wind=(%.1f,%.1f,%.1f) MinPathWetness=%.2f MaxPathWetness=%.2f\n", info.mWind.x, info.mWind.y, info.mWind.z, info.mMinPathWetness, info.mMaxPathWetness);

    // Print vehicle info
    for (long i = 0; i < info.mNumVehicles; ++i)
    {
      VehicleScoringInfoV01 &vinfo = info.mVehicle[i];
      fprintf(fo, "Driver %d: %s\n", i, vinfo.mDriverName);
      fprintf(fo, " ID=%d Vehicle=%s\n", vinfo.mID, vinfo.mVehicleName);
      fprintf(fo, " Laps=%d Sector=%d FinishStatus=%d\n", vinfo.mTotalLaps, vinfo.mSector, vinfo.mFinishStatus);
      fprintf(fo, " LapDist=%.1f PathLat=%.2f RelevantTrackEdge=%.2f\n", vinfo.mLapDist, vinfo.mPathLateral, vinfo.mTrackEdge);
      fprintf(fo, " Best=(%.3f, %.3f, %.3f)\n", vinfo.mBestSector1, vinfo.mBestSector2, vinfo.mBestLapTime);
      fprintf(fo, " Last=(%.3f, %.3f, %.3f)\n", vinfo.mLastSector1, vinfo.mLastSector2, vinfo.mLastLapTime);
      fprintf(fo, " Current Sector 1 = %.3f, Current Sector 2 = %.3f\n", vinfo.mCurSector1, vinfo.mCurSector2);
      fprintf(fo, " Pitstops=%d, Penalties=%d\n", vinfo.mNumPitstops, vinfo.mNumPenalties);

      // New version 2 stuff
      fprintf(fo, " IsPlayer=%d Control=%d InPits=%d LapStartET=%.3f\n", vinfo.mIsPlayer, vinfo.mControl, vinfo.mInPits, vinfo.mLapStartET);
      fprintf(fo, " Place=%d VehicleClass=%s\n", vinfo.mPlace, vinfo.mVehicleClass);
      fprintf(fo, " TimeBehindNext=%.3f LapsBehindNext=%d\n", vinfo.mTimeBehindNext, vinfo.mLapsBehindNext);
      fprintf(fo, " TimeBehindLeader=%.3f LapsBehindLeader=%d\n", vinfo.mTimeBehindLeader, vinfo.mLapsBehindLeader);
      fprintf(fo, " Pos=(%.3f,%.3f,%.3f)\n", vinfo.mPos.x, vinfo.mPos.y, vinfo.mPos.z);

      // Forward is roughly in the -z direction (although current pitch of car may cause some y-direction velocity)
      fprintf(fo, " LocalVel=(%.2f,%.2f,%.2f)\n", vinfo.mLocalVel.x, vinfo.mLocalVel.y, vinfo.mLocalVel.z);
      fprintf(fo, " LocalAccel=(%.1f,%.1f,%.1f)\n", vinfo.mLocalAccel.x, vinfo.mLocalAccel.y, vinfo.mLocalAccel.z);

      // Orientation matrix is left-handed
      fprintf(fo, " [%6.3f,%6.3f,%6.3f]\n", vinfo.mOri[0].x, vinfo.mOri[0].y, vinfo.mOri[0].z);
      fprintf(fo, " [%6.3f,%6.3f,%6.3f]\n", vinfo.mOri[1].x, vinfo.mOri[1].y, vinfo.mOri[1].z);
      fprintf(fo, " [%6.3f,%6.3f,%6.3f]\n", vinfo.mOri[2].x, vinfo.mOri[2].y, vinfo.mOri[2].z);
      fprintf(fo, " LocalRot=(%.3f,%.3f,%.3f)\n", vinfo.mLocalRot.x, vinfo.mLocalRot.y, vinfo.mLocalRot.z);
      fprintf(fo, " LocalRotAccel=(%.2f,%.2f,%.2f)\n", vinfo.mLocalRotAccel.x, vinfo.mLocalRotAccel.y, vinfo.mLocalRotAccel.z);
    }

    // Delimit sections
    fprintf(fo, "\n");

    // Close file
    fclose(fo);
  }
}
