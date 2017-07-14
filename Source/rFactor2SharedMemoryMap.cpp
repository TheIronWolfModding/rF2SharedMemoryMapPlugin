/*
Implementation of rFactor 2 internal state mapping into shared memory buffers.

Author: The Iron Wolf (vleonavicius@hotmail.com)
Website: thecrewchief.org


Acknowledgements:
  This work is based on:
    - rF2 Internals Plugin sample #7 by ISI/S397 found at: https://www.studio-397.com/modding-resources/
    - rF1 Shared Memory Map Plugin by Dan Allongo found at: https://github.com/dallongo/rFactorSharedMemoryMap


Shared resources:
  This plugin uses double buffering and mutex to allow optional synchronized access to rF2 exposed internal state.
  Shared resources use the following naming convention:
    - $rFactor2SMMP_<BUFFER_TYPE>Buffer1$
    - $rFactor2SMMP_<BUFFER_TYPE>Buffer2$
    - Global\$rFactor2SMMP_<BUFFER_TYPE>Mutex - mutex for optional weak synchronization (see Synchronization below)

  where <BUFFER_TYPE> is one of the following:
    * Telemetry - mapped view of rF2Telemetry structure
    * Scoring - mapped view of rF2Scoring structure
    * Extended - mapped view of rF2Extended structure

  Those types are (with few exceptions) exact mirror of ISI structures, plugin constantly memcpy'es them from game to memory mapped files.


State updates (buffer flips, see Double Buffering):
  Telemetry - updated every 10ms, but in practice only every other update contains updated data, so real update rate is around 50FPS.
  Scoring - every 200ms (5FPS)
  Extended - every 200ms or on tracked function call.

  Plugin does not add artificial delays, except:
    - telemetry updates with same game time are skipped
    - if telemetry mutex is signaled, telemetry buffer update is skipped


Telemetry state:
  rF2 calls UpdateTelemetry for each vehicle.  Plugin tries to guess when all vehicles received an update, and only after that flip is attempted (see Double Buffering).


Extended state:
  Exposed extended state consists of the two parts:

  * Non periodically updated game state:
      Physics settings updates and various callback based properties are tracked.

  * Heuristic data exposed as an attempt to compensate for values not currently available from the game:
      Damage state is tracked, since game provides no accumulated damage data.  Tracking happens on _every_ telemetry/scoring
      update for full precision.
      
  See SharedMemoryPlugin::ExtendedStateTracker struct for details.


Double Buffering:
  Plugin maps each exposed structure into two memory mapped files.  Buffers are written to alternatively.
  rF2MappedBufferHeaders::mCurrentRead indicates last updated buffer.

  Buffers are flipped after each update (see State Updates) except for telemetry state buffers.

  Telemetry buffer flip is designed so that we try to avoid waiting on the mutex if it is signaled.  There are SharedMemoryPlugin::MAX_ASYNC_RETRIES
  (three currently) attempts before wait will happen.  Retries only happen on telemetry frame completion, or new frame start.


Synchronization:
  Important: do not use synchronization if your application:
    - queries for data at high rate (50ms or smaller gaps)
    - does not need consistent view of the whole buffer.  Typically, Dashboards,  varios visualizers do not need such views,
      because partially correct data will be overritten by next frame.  Abusing synchronization might cause game FPS drops.

  A lot of effort was done to ensure minimal impact on the rF2.  Therefore, using mutex does not guarantee that buffer
  won't be overwritten. While mutex is exposed for synchronized access, plugin tries to minimize wait time by retrying
  during telemetry updates (~90FPS) and only waiting for 1ms max during scoring updates (and on fourth telemetry flip retry), 
  before forcefully flipping buffers.  Also, if 1ms elapses on synchronized flip, buffer will be overwritten anyway.


Configuration file:
  Optional configuration file is supported (primarily for debugging purposes).
  See SharedMemoryPlugin::LoadConfig.


Sample consumption:
  For sample C# client, see Monitor\rF2SMMonitor\rF2SMMonitor\MainForm.cs
*/
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
{}


void SharedMemoryPlugin::Startup(long version)
{
  // Read configuration .ini if there's one.
  LoadConfig();

  char temp[80] = {};
  sprintf(temp, "-STARTUP- (version %.3f)", (float)version / 1000.0f);
  WriteToAllExampleOutputFiles("w", temp);

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

  ClearState();

  DEBUG_MSG(DebugLevel::Errors, "Files mapped successfully");
  if (SharedMemoryPlugin::msDebugOutputLevel != DebugLevel::Off) {
    char sizeSz[20] = {};
    auto size = static_cast<int>(sizeof(rF2Telemetry));
    _itoa_s(size, sizeSz, 10);
    DEBUG_MSG3(DebugLevel::Errors, "Size of telemetry buffers:", sizeSz, "bytes each.");

    assert(sizeof(rF2Telemetry) == offsetof(rF2Telemetry, mVehicles[rF2MappedBufferHeader::MAX_MAPPED_VEHICLES]));

    sizeSz[0] = '\0';
    size = static_cast<int>(sizeof(rF2Scoring));
    _itoa_s(size, sizeSz, 10);
    DEBUG_MSG3(DebugLevel::Errors, "Size of scoring buffers:", sizeSz, "bytes each.");

    assert(sizeof(rF2Scoring) == offsetof(rF2Scoring, mVehicles[rF2MappedBufferHeader::MAX_MAPPED_VEHICLES]));

    sizeSz[0] = '\0';
    size = static_cast<int>(sizeof(rF2Extended));
    _itoa_s(size, sizeSz, 10);
    DEBUG_MSG3(DebugLevel::Errors, "Size of extended buffers:", sizeSz, "bytes each.");
  }
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

  mTelemetry.ClearState(nullptr /*pInitialContents*/);
  mTelemetry.ReleaseResources();

  mScoring.ClearState(nullptr /*pInitialContents*/);
  mScoring.ReleaseResources();

  mExtended.ClearState(nullptr /*pInitialContents*/);
  mExtended.ReleaseResources();

  mIsMapped = false;
}

void SharedMemoryPlugin::ClearTimingsAndCounters()
{
  mTelemetryFrameCompleted = false;

  mLastTelemetryUpdateMillis = 0.0;
  mLastTelemetryVehicleAddedMillis = 0.0;
  mLastScoringUpdateMillis = 0.0;

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

  // Certain members of extended state persist between restarts/sessions.
  // So, clear the state but pass persisting state as initial state.
  mExtStateTracker.ClearState();
  mExtended.ClearState(&(mExtStateTracker.mExtended));

  ClearTimingsAndCounters();
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


void SharedMemoryPlugin::UpdateInRealtimeFC(bool inRealTime)
{
  if (!mIsMapped)
    return;

  DEBUG_MSG(DebugLevel::Synchronization, inRealTime ? "Entering Realtime" : "Exiting Realtime");

  mExtStateTracker.mExtended.mInRealtimeFC = inRealTime;
  memcpy(mExtended.mpCurrWriteBuff, &(mExtStateTracker.mExtended), sizeof(rF2Extended));
  mExtended.FlipBuffers();
}


void SharedMemoryPlugin::EnterRealtime()
{
  // start up timer every time we enter realtime
  WriteToAllExampleOutputFiles("a", "---ENTERREALTIME---");

  UpdateInRealtimeFC(true /*inRealtime*/);
}


void SharedMemoryPlugin::ExitRealtime()
{
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


void SharedMemoryPlugin::TelemetryTraceSkipUpdate(TelemInfoV01 const& info, double deltaET) const
{
  if (SharedMemoryPlugin::msDebugOutputLevel >= DebugLevel::Timing) {
    char msg[512] = {};
    sprintf(msg, "TELEMETRY - Skipping update due to no changes in the input data.  Delta ET: %f  New ET: %f  Prev ET:%f  mID(new):%d", deltaET, info.mElapsedTime, mLastTelemetryUpdateET, info.mID);
    DEBUG_MSG(DebugLevel::Timing, msg);

    // We flip only on mElapsedTime change, so on skip we need to compare to the current write buffer.
    // Below assumes that we begin skip on the first vehicle, which is not guaranteed.  However, that's ok
    // since this code is diagnostic.
    auto const prevBuff = mTelemetry.mpCurrWriteBuff;
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
    sprintf(msg, "TELEMETRY - Begin Update: Buffer %s.  ET:%f  ET delta:%f  Time delta since last update:%f",
      mTelemetry.mpCurrWriteBuff == mTelemetry.mpBuff1 ? "1" : "2", telUpdateET, deltaET, delta / MICROSECONDS_IN_SECOND);
    
    DEBUG_MSG(DebugLevel::Timing, msg);
  }

  mLastTelemetryUpdateMillis = ticksNow;
}


void SharedMemoryPlugin::TelemetryTraceVehicleAdded(TelemInfoV01 const& info) 
{
  if (SharedMemoryPlugin::msDebugOutputLevel == DebugLevel::Verbose) {
    // If retry is pending, previous data is in write buffer, otherwise it is in read buffer.
    auto const prevBuff = mTelemetry.RetryPending() ? mTelemetry.mpCurrWriteBuff : mTelemetry.mpCurrReadBuff;
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
    sprintf(msg, "TELEMETRY - End Update.  Telemetry chain update took %f:  Vehicles in chain: %d", deltaSysTimeMicroseconds / MICROSECONDS_IN_SECOND, numVehiclesInChain);

    DEBUG_MSG(DebugLevel::Timing, msg);
  }
}


void SharedMemoryPlugin::TelemetryFlipBuffers()
{
  if (mLastTelemetryUpdateET > 0.0 && mLastTelemetryUpdateET <= mLastScoringUpdateET) {
    // At the ET when both Scoring and Telemetry are updated, Scoring updates are coming in between telemetry updates with same ET.
    // We want scoring and telemetry to be close and not let telemetry update drag behind.
    DEBUG_MSG(DebugLevel::Timing, "TELEMETRY - Force flip due to: mLastTelemetryUpdateET <= mLastScoringUpdateET.");
    mTelemetry.FlipBuffers();
  }
  else if (mTelemetry.AsyncRetriesLeft() > 0) {
    auto const retryPending = mTelemetry.RetryPending();
    // Otherwise, try buffer flip.
    mTelemetry.TryFlipBuffers();

    // Print msg about buffer flip failure or success.
    if (mTelemetry.RetryPending())
      DEBUG_INT2(DebugLevel::Synchronization, "TELEMETRY - Buffer flip failed, retries remaining:", mTelemetry.AsyncRetriesLeft());
    else {
      if (retryPending)
        DEBUG_MSG(DebugLevel::Synchronization, "TELEMETRY - Buffer flip succeeded on retry.");
      else
        DEBUG_MSG(DebugLevel::Timing, "TELEMETRY - Buffer flip succeeded.");
    }
  }
  else {
    // Force flip if no more retries are left
    assert(mTelemetry.AsyncRetriesLeft() == 0);
    DEBUG_MSG(DebugLevel::Synchronization, "TELEMETRY - Force flip due to retry limit exceeded.");
    mTelemetry.FlipBuffers();
  }
}


void SharedMemoryPlugin::TelemetryCompleteFrame()
{
  mTelemetry.mpCurrWriteBuff->mNumVehicles = mCurrTelemetryVehicleIndex;
  mTelemetry.mpCurrWriteBuff->mBytesUpdatedHint = static_cast<int>(offsetof(rF2Telemetry, mVehicles[mTelemetry.mpCurrWriteBuff->mNumVehicles]));

  TelemetryTraceEndUpdate(mTelemetry.mpCurrWriteBuff->mNumVehicles);

  // Try flipping the buffers.
  TelemetryFlipBuffers();
}


/*
rF2 sends telemetry updates for each vehicle.  The problem is that we do not know when all vehicles received an update.
Below I am trying to flip buffers per-frame, where "frame" means all vehicles received telemetry update.

I am detecting frame by checking time distance between mElapsedTime.  It appears that rF2 sends vehicle telemetry every 2ms
(every 1ms really, but most of the time contents are duplicated).  As a consquence, we do flip every 2ms (50FPS).

Note that sometimes mElapsedTime for player vehicle is slightly ahead of the rest of vehicles (but never more than 2ms, most often being 0.25ms).

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
  if (abs(deltaET) >= 0.0199)  // Apparently, rF2 telemetry update step is 2ms.
    isNewFrame = true;
  else {
    // Sometimes, player vehicle telemetry is updated more frequently than other vehicles.  What that means is that ET of player is
    // ahead of other vehicles.  This creates torn frames, and is a problem especially in online due to player
    // vehicle not having predefined position in a chain.
    // Current solution is to detect when 2ms step happens, which means that we effectively limit refresh
    // to 50FPS (seems to be what game's doing anyway).
    
    // We need to pick min ET for the frame because one of the vehicles in a frame might be slightly ahead of the rest.
    mLastTelemetryUpdateET = min(mLastTelemetryUpdateET, info.mElapsedTime);
  }

  if (isNewFrame
    || mCurrTelemetryVehicleIndex >= rF2MappedBufferHeader::MAX_MAPPED_VEHICLES) {
    // This is the new frame.  End the previous frame if it is still open:
    if (!mTelemetryFrameCompleted)
      TelemetryCompleteFrame();
    else if (mTelemetry.RetryPending()) {  // Frame already complete, retry if flip is pending.
      DEBUG_MSG(DebugLevel::Synchronization, "TELEMETRY - Retry pending buffer flip on new telemetry frame.");
      TelemetryFlipBuffers();
    }

    // Begin the new frame.  Reset tracking variables.
    TelemetryTraceBeginUpdate(info.mElapsedTime, deltaET);
    
    memset(mParticipantTelemetryUpdated, 0, sizeof(mParticipantTelemetryUpdated));
    mTelemetryFrameCompleted = false;
    mCurrTelemetryVehicleIndex = 0;

    // Update telemetry frame Elapsed Time.
    mLastTelemetryUpdateET = info.mElapsedTime;
  }

  if (mTelemetryFrameCompleted)
    return;  // Nothing to do.

  // See if we are in a cycle.
  auto const partiticpantIndex = min(max(info.mID, 0L), SharedMemoryPlugin::MAX_PARTICIPANT_SLOTS - 1);
  auto const alreadyUpdated = mParticipantTelemetryUpdated[partiticpantIndex];

  if (!alreadyUpdated) {
    if (mCurrTelemetryVehicleIndex >= rF2MappedBufferHeader::MAX_MAPPED_VEHICLES) {
      DEBUG_MSG(DebugLevel::Errors, "TELEMETRY - Exceeded maximum of allowed mapped vehicles.");
      return;
    }

    // Update extended state for this vehicle.
    // Since I do not want to miss impact data, and it is not accumulated in any way
    // I am aware of in rF2 internals, process on every telemetr update.
    mExtStateTracker.ProcessTelemetryUpdate(info);

    // Mark participant as updated
    assert(mParticipantTelemetryUpdated[partiticpantIndex] == false);
    mParticipantTelemetryUpdated[partiticpantIndex] = true;

    TelemetryTraceVehicleAdded(info);

    // Write vehicle telemetry.
    memcpy(&(mTelemetry.mpCurrWriteBuff->mVehicles[mCurrTelemetryVehicleIndex]), &info, sizeof(rF2VehicleTelemetry));
    ++mCurrTelemetryVehicleIndex;
  }
  else {
    // The chain is complete.  Try flipping early.
    if (!mTelemetryFrameCompleted) {
      TelemetryTraceSkipUpdate(info, deltaET);

      TelemetryCompleteFrame();

      mTelemetryFrameCompleted = true;
    }
  }
}


void SharedMemoryPlugin::ScoringTraceBeginUpdate()
{
  auto ticksNow = 0.0;
  if (SharedMemoryPlugin::msDebugOutputLevel >= DebugLevel::Timing) {
    ticksNow = TicksNow();
    auto const delta = ticksNow - mLastScoringUpdateMillis;

    if (mScoring.mpCurrWriteBuff == mScoring.mpBuff1)
      DEBUG_FLOAT2(DebugLevel::Timing, "SCORING - Begin Update: Buffer 1.  Delta since last update:", delta / MICROSECONDS_IN_SECOND);
    else
      DEBUG_FLOAT2(DebugLevel::Timing, "SCORING - Begin Update: Buffer 2.  Delta since last update:", delta / MICROSECONDS_IN_SECOND);

    char msg[512] = {};
    sprintf(msg, "SCORING - Scoring ET:%f  Telemetry ET:%f", mLastScoringUpdateET, mLastTelemetryUpdateET);
    DEBUG_MSG(DebugLevel::Timing, msg);
  }

  mLastScoringUpdateMillis = ticksNow;
}


void SharedMemoryPlugin::UpdateScoring(ScoringInfoV01 const& info)
{
  WriteScoringInternals(info);

  if (!mIsMapped)
    return;

  mLastScoringUpdateET = info.mCurrentET;

  ScoringTraceBeginUpdate();

  if (mTelemetry.RetryPending()) {
    // If this happens often, we need to change something, because forced flip is coming very soon
    // after scoring update anyway.
    DEBUG_MSG(DebugLevel::Synchronization, "SCORING - Force telemetry flip due to retry pending.");
    mTelemetry.FlipBuffers();
  }

  // Below apparently never happens, but let's keep it in case there's a regression in the game.
  // So far, this appears to only happen on session end, when telemetry is already zeroed out.
  if (mLastScoringUpdateET > mLastTelemetryUpdateET)
    DEBUG_MSG(DebugLevel::Warnings, "WARNING: Scoring update is ahead of telemetry.");

  memcpy(&(mScoring.mpCurrWriteBuff->mScoringInfo), &info, sizeof(rF2ScoringInfo));

  if (info.mNumVehicles >= rF2MappedBufferHeader::MAX_MAPPED_VEHICLES)
    DEBUG_MSG(DebugLevel::Errors, "ERROR: Scoring exceeded maximum of allowed mapped vehicles.");

  auto const numScoringVehicles = min(info.mNumVehicles, rF2MappedBufferHeader::MAX_MAPPED_VEHICLES);
  for (int i = 0; i < numScoringVehicles; ++i)
    memcpy(&(mScoring.mpCurrWriteBuff->mVehicles[i]), &(info.mVehicle[i]), sizeof(rF2VehicleScoring));

  mScoring.mpCurrWriteBuff->mBytesUpdatedHint = static_cast<int>(offsetof(rF2Scoring, mVehicles[numScoringVehicles]));

  mScoring.FlipBuffers();

  // Update extended state.
  mExtStateTracker.ProcessScoringUpdate(info);
  memcpy(mExtended.mpCurrWriteBuff, &(mExtStateTracker.mExtended), sizeof(rF2Extended));
  mExtended.FlipBuffers();
}


// Invoked periodically.
bool SharedMemoryPlugin::WantsToDisplayMessage(MessageInfoV01& /*msgInfo*/)
{
  // Looks like this is write only API, can't read current text in MC
  return false;
}


void SharedMemoryPlugin::UpdateThreadState(long type, bool starting)
{
  (type == 0 ? mExtStateTracker.mExtended.mMultimediaThreadStarted : mExtStateTracker.mExtended.mSimulationThreadStarted)
    = starting;

  if (!mIsMapped)
    return;

  memcpy(mExtended.mpCurrWriteBuff, &(mExtStateTracker.mExtended), sizeof(rF2Extended));
  mExtended.FlipBuffers();
}


void SharedMemoryPlugin::ThreadStarted(long type)
{
  DEBUG_MSG(DebugLevel::Synchronization, type == 0 ? "Multimedia thread started" : "Simulation thread started");
  UpdateThreadState(type, true /*starting*/);
}

void SharedMemoryPlugin::ThreadStopping(long type)
{
  DEBUG_MSG(DebugLevel::Synchronization, type == 0 ? "Multimedia thread stopped" : "Simulation thread stopped");
  UpdateThreadState(type, false /*starting*/);
}


// Called roughly every 300ms.
bool SharedMemoryPlugin::AccessTrackRules(TrackRulesV01& /*info*/)
{
  return false;
}

// Invoked periodically.
bool SharedMemoryPlugin::AccessPitMenu(PitMenuV01& /*info*/)
{
  return false;
}


void SharedMemoryPlugin::SetPhysicsOptions(PhysicsOptionsV01& options)
{
  DEBUG_MSG(DebugLevel::Timing, "PHYSICS - Updated.");
  memcpy(&(mExtStateTracker.mExtended.mPhysics), &options, sizeof(rF2PhysicsOptions));
  memcpy(mExtended.mpCurrWriteBuff, &(mExtStateTracker.mExtended), sizeof(rF2Extended));
  mExtended.FlipBuffers();
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

  // Form time string.
  char timeBuff[20] = {};
  time_t now = time(nullptr);
  tm timeLocal = {};
  (errno_t)localtime_s(&timeLocal, &now);
  strftime(timeBuff, 20, "%H:%M:%S", &timeLocal);

  fprintf(SharedMemoryPlugin::msDebugFile, "%s  TID:0x%04x  ", timeBuff, GetCurrentThreadId());
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