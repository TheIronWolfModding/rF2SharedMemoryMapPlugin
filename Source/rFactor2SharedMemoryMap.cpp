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
#include <stdio.h>                              // for sample output
#include <stdlib.h>
#include <assert.h>
#include <cstddef>                              // offsetof

#define PLUGIN_VERSION_MAJOR "0.5"
#define PLUGIN_VERSION_MINOR "0.0"
#define PLUGIN_NAME_AND_VERSION "rFactor 2 Shared Memory Map Plugin - v" PLUGIN_VERSION_MAJOR
#define SHARED_MEMORY_VERSION PLUGIN_VERSION_MAJOR "." PLUGIN_VERSION_MINOR

static double const MILLISECONDS_IN_SECOND = 1000.0;
static double const MICROSECONDS_IN_MILLISECOND = 1000.0;
static double const MICROSECONDS_IN_SECOND = MILLISECONDS_IN_SECOND * MICROSECONDS_IN_MILLISECOND;

DebugLevel SharedMemoryPlugin::msDebugOutputLevel = DebugLevel::Off;
bool SharedMemoryPlugin::msDebugISIInternals = false;
int SharedMemoryPlugin::msMillisRefresh = 32;
DWORD SharedMemoryPlugin::msMillisMutexWait = 1;
char const* const SharedMemoryPlugin::MM_FILE_NAME1 = "$rFactor2SMMPBuffer1$";
char const* const SharedMemoryPlugin::MM_FILE_NAME2 = "$rFactor2SMMPBuffer2$";
char const* const SharedMemoryPlugin::MM_FILE_ACCESS_MUTEX = R"(Global\$rFactor2SMMPMutex)";
char const* const SharedMemoryPlugin::CONFIG_FILE_REL_PATH = R"(\UserData\player\rf2smmp.ini)";  // Relative to rF2 root.
char const* const SharedMemoryPlugin::INTERNALS_TELEMETRY_FILENAME = "RF2SMMP_InternalsTelemetryOutput.txt";
char const* const SharedMemoryPlugin::INTERNALS_SCORING_FILENAME = "RF2SMMP_InternalsScoringOutput.txt";
char const* const SharedMemoryPlugin::DEBUG_OUTPUT_FILENAME = "RF2SMMP_DebugOutput.txt";
int const SharedMemoryPlugin::MAX_ASYNC_RETRIES = 1;

// plugin information
extern "C" __declspec(dllexport)
const char * __cdecl GetPluginName() { return PLUGIN_NAME_AND_VERSION; }

extern "C" __declspec(dllexport)
PluginObjectType __cdecl GetPluginType() { return(PO_INTERNALS); }

extern "C" __declspec(dllexport)
int __cdecl GetPluginVersion() { return(7); } // InternalsPluginV01 functionality (if you change this return value, you must derive from the appropriate class!)

extern "C" __declspec(dllexport)
PluginObject * __cdecl CreatePluginObject() { return((PluginObject *) new SharedMemoryPlugin); }

extern "C" __declspec(dllexport)
void __cdecl DestroyPluginObject(PluginObject *obj) { delete((SharedMemoryPlugin *)obj); }

// NOTE: too much logging will kill performance.  Can be improved with buffering.
// This is hell on earth, but I do not want to add additional dependencies needed for STL right now.
// Be super careful with those, there's no type safety or checks of any kind.
#define DEBUG_MSG(lvl, msg) WriteDebugMsg(lvl, "%s : %s\n", __FUNCTION__, msg)
#define DEBUG_MSG2(lvl, msg, msg2) WriteDebugMsg(lvl, "%s : %s %s\n", __FUNCTION__, msg, msg2)
#define DEBUG_INT2(lvl, msg, intValue) WriteDebugMsg(lvl, "%s : %s %d\n", __FUNCTION__, msg, intValue)
#define DEBUG_FLOAT2(lvl, msg, floatValue) WriteDebugMsg(lvl, "%s : %s %f\n", __FUNCTION__, msg, floatValue)
#define DEBUG_MSG3(lvl, msg, msg2, msg3) WriteDebugMsg(lvl, "%s : %s %s %s\n", __FUNCTION__, msg, msg2, msg3)

//////////////////////////////////////
// SharedMemoryPlugin class
//////////////////////////////////////
HANDLE SharedMemoryPlugin::MapMemoryFile(char const* const fileName, rF2State*& pBuf) const
{
  char tag[256] = {};
  strcpy_s(tag, fileName);

  char exe[1024] = {};
  GetModuleFileName(nullptr, exe, sizeof(exe));

  char pid[8] = {};
  sprintf(pid, "%d", GetCurrentProcessId());

  // append processId for dedicated server to allow multiple instances
  // TODO: Verify for rF2.
  if (strstr(exe, "Dedicated.exe") != nullptr) 
    strcat(tag, pid);

  // init handle and try to create, read if existing
  HANDLE hMap = INVALID_HANDLE_VALUE;
  hMap = CreateFileMapping(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof(rF2State), TEXT(tag));
  if (hMap == nullptr) {
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
      hMap = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, TEXT(tag));
      if (hMap == nullptr) {
        return nullptr;
      }
    }
  }

  pBuf = static_cast<rF2State*>(MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(rF2State)));
  if (pBuf == nullptr) {
    // failed to map memory buffer
    CloseHandle(hMap);
    return nullptr;
  }

  return hMap;
}

void SharedMemoryPlugin::Startup(long version)
{
  // Read configuration .ini if there's one.
  LoadConfig();

  char temp[80] = {};
  sprintf(temp, "-STARTUP- (version %.3f)", (float)version / 1000.0f);
  WriteToAllExampleOutputFiles("w", temp);

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

  mIsMapped = true;
  if (mIsMapped) {
    ClearState();

    DEBUG_MSG(DebugLevel::Errors, "Files mapped successfully");

    if (SharedMemoryPlugin::msDebugOutputLevel != DebugLevel::Off) {
      auto size = static_cast<int>(sizeof(rF2State));
      char sizeSz[20] = {};
      _itoa_s(size, sizeSz, 10);
      DEBUG_MSG3(DebugLevel::Errors, "Size of state buffers:", sizeSz, "bytes each.");
    }
  }

  return;
}


void SharedMemoryPlugin::Shutdown()
{
  WriteToAllExampleOutputFiles("a", "-SHUTDOWN-");

  DEBUG_MSG(DebugLevel::Errors, "Shutting down");

  ClearState();

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

  mpBuf1 = nullptr;
  mpBuf2 = nullptr;
  mhMap1 = nullptr;
  mhMap2 = nullptr;
  mhMutex = nullptr;
  mpBufCurWrite = nullptr;
  mIsMapped = false;
}

void SharedMemoryPlugin::ClearTimings()
{
  mLastTelUpdate = 0.0;
  mLastScoringUpdate = 0.0;
  mDelta = 0.0;
}

void SharedMemoryPlugin::ClearState()
{
  if (mIsMapped) {
    mRetryFlip = false;
    mRetriesLeft = 0;

    auto const ret = WaitForSingleObject(mhMutex, SharedMemoryPlugin::msMillisMutexWait);

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
  }

  ClearTimings();
  mScoringInfo = {};
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


void SharedMemoryPlugin::EnterRealtime()
{
  // start up timer every time we enter realtime
  mET = 0.0;
  WriteToAllExampleOutputFiles("a", "---ENTERREALTIME---");

  mInRealtime = true;
}


void SharedMemoryPlugin::ExitRealtime()
{
  WriteToAllExampleOutputFiles("a", "---EXITREALTIME---");

  mInRealtime = false;
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


void SharedMemoryPlugin::UpdateTelemetry(TelemInfoV01 const& info)
{
  if (mIsMapped) {
     // Update mMaxImactMagnitude.
     mpBufCurWrite->mMaxImpactMagnitude = max(mpBufCurWrite->mMaxImpactMagnitude, info.mLastImpactMagnitude);

    // Delta will have to be capped to some max value (refresh rate)?
    auto const ticksNow = TicksNow();
    auto const delta = ticksNow - mLastTelUpdate;

    // It looks like rF2 calls this function every ~10ms.
    // Subtracting 5 from desired refresh rate makes it refresh closer to desired
    // moment on the average.
    if (delta >= ((SharedMemoryPlugin::msMillisRefresh - 5) * MICROSECONDS_IN_MILLISECOND)) {
      mLastTelUpdate = ticksNow;

      // Update write buffer with telemetry.
      UpdateTelemetryHelper(ticksNow, info);

      // Try making write buffer the read buffer.
      TryFlipBuffers();

      // If flip failed (some client holds lock), retry on next call.
      if (mRetryFlip) {
        mRetriesLeft = MAX_ASYNC_RETRIES;
        DEBUG_INT2(DebugLevel::Synchronization, "Buffer flip failed, retries left:", mRetriesLeft);
      }
    }
    else if (mRetryFlip) {
      if (mRetriesLeft > 0) {
        // If we're in retry mode and we still have retries allowed, try again
        TryFlipBuffers();
      
        if (mRetryFlip) {
          --mRetriesLeft;
          DEBUG_INT2(DebugLevel::Synchronization, "Buffer flip failed, retries left:", mRetriesLeft);
        }
        else 
          DEBUG_MSG(DebugLevel::Synchronization, "Buffer flip retry succeeded.");
      }
      else {
        // Otherwise, update synchronously.
        DEBUG_MSG(DebugLevel::Synchronization, "Retries failed, updating synchronously.");
        FlipBuffers();
      }
    }
  }

  WriteTelemetryInternals(info);
}


void SharedMemoryPlugin::FlipBuffersHelper()
{
  // Handle fucked up case:
  if (mpBuf1->mCurrentRead == mpBuf2->mCurrentRead) {
    mpBuf1->mCurrentRead = true;
    mpBuf2->mCurrentRead = false;
    DEBUG_MSG(DebugLevel::Errors, "ERROR: - Buffers out of sync.");
  }

  // Pick previous read buffer.
  mpBufCurWrite = mpBuf1->mCurrentRead ? mpBuf1 : mpBuf2;

  // Switch the read and write buffers.
  mpBuf1->mCurrentRead = !mpBuf1->mCurrentRead;
  mpBuf2->mCurrentRead = !mpBuf2->mCurrentRead;

  assert(!mpBufCurWrite->mCurrentRead);
}


void SharedMemoryPlugin::FlipBuffers()
{
  // This update will wait.  Clear the retry variables.
  mRetryFlip = false;
  mRetriesLeft = 0;

  auto const ret = WaitForSingleObject(mhMutex, SharedMemoryPlugin::msMillisMutexWait);

  FlipBuffersHelper();

  if (ret == WAIT_OBJECT_0)
    ReleaseMutex(mhMutex);
  else if (ret == WAIT_TIMEOUT)
    DEBUG_MSG(DebugLevel::Warnings, "WARNING: - Timed out while waiting on mutex.");
  else
    DEBUG_MSG(DebugLevel::Errors, "ERROR: - wait on mutex failed.");
}

void SharedMemoryPlugin::TryFlipBuffers()
{
  // Do not wait on mutex if it is held.
  auto const ret = WaitForSingleObject(mhMutex, 0);
  if (ret == WAIT_TIMEOUT) {
    mRetryFlip = true;
    return;
  }

  // We have the lock.  Clear retry variables.
  mRetryFlip = false;
  mRetriesLeft = 0;

  // Do the actual flip.
  FlipBuffersHelper();

  if (ret == WAIT_OBJECT_0)
    ReleaseMutex(mhMutex);
}


void SharedMemoryPlugin::UpdateTelemetryHelper(double const ticksNow, TelemInfoV01 const& info)
{
  mDelta = (ticksNow - mLastScoringUpdate) / MICROSECONDS_IN_SECOND;

  auto pBuf = mpBufCurWrite;
  assert(!mpBufCurWrite->mCurrentRead);

  DEBUG_FLOAT2(DebugLevel::Verbose, "Telemetry ticks:", ticksNow);
  if (pBuf == mpBuf1)
    DEBUG_FLOAT2(DebugLevel::Timing, "Update Telemetry Buffer 1:", mDelta);
  else
    DEBUG_FLOAT2(DebugLevel::Timing, "Update Telemetry Buffer 2:", mDelta);

  //
  // TelemInfoV01
  //

  // Time
  pBuf->mID = info.mID;
  pBuf->mDeltaTime = mDelta;
  pBuf->mElapsedTime = info.mElapsedTime;
  pBuf->mLapNumber = info.mLapNumber;
  pBuf->mLapStartET = info.mLapStartET;
  
  strcpy_s(pBuf->mVehicleName, info.mVehicleName);
  strcpy_s(pBuf->mTrackName, info.mTrackName);

  // Position and derivatives
  pBuf->mPos = { info.mPos.x, info.mPos.y, info.mPos.z };
  pBuf->mLocalVel = { info.mLocalVel.x, info.mLocalVel.y, info.mLocalVel.z };
  pBuf->mLocalAccel = { info.mLocalAccel.x, info.mLocalAccel.y, info.mLocalAccel.z };

  pBuf->mSpeed = sqrt((info.mLocalVel.x * info.mLocalVel.x) +
    (info.mLocalVel.y * info.mLocalVel.y) +
    (info.mLocalVel.z * info.mLocalVel.z));

  // Orientation and derivatives
  for (int row = 0; row < OriMat::NumRows; ++row) {
    pBuf->mOri[row] = { info.mOri[row].x, info.mOri[row].y, info.mOri[row].z };
  }
  
  pBuf->mLocalRot = { info.mLocalRot.x, info.mLocalRot.y, info.mLocalRot.z };
  pBuf->mLocalRotAccel = { info.mLocalRotAccel.x, info.mLocalRotAccel.y, info.mLocalRotAccel.z };
  
  // Vehicle status
  pBuf->mGear = info.mGear;
  pBuf->mEngineRPM = info.mEngineRPM;
  pBuf->mEngineWaterTemp = info.mEngineWaterTemp;
  pBuf->mEngineOilTemp = info.mEngineOilTemp;
  pBuf->mClutchRPM = info.mClutchRPM;

  // Driver input
  pBuf->mUnfilteredThrottle = info.mUnfilteredThrottle;
  pBuf->mUnfilteredBrake = info.mUnfilteredBrake;
  pBuf->mUnfilteredSteering = info.mUnfilteredSteering;
  pBuf->mUnfilteredClutch = info.mUnfilteredClutch;

  // Filtered input
  pBuf->mFilteredThrottle = info.mFilteredThrottle;
  pBuf->mFilteredBrake = info.mFilteredBrake;
  pBuf->mFilteredSteering = info.mFilteredSteering;
  pBuf->mFilteredClutch = info.mFilteredClutch;

  // Misc
  pBuf->mSteeringShaftTorque = info.mSteeringShaftTorque;
  pBuf->mFront3rdDeflection = info.mFront3rdDeflection;
  pBuf->mRear3rdDeflection = info.mRear3rdDeflection;

  // Aerodynamics
  pBuf->mFrontWingHeight = info.mFrontWingHeight;
  pBuf->mFrontRideHeight = info.mFrontRideHeight;
  pBuf->mRearRideHeight = info.mRearRideHeight;
  pBuf->mDrag = info.mDrag;
  pBuf->mFrontDownforce = info.mFrontDownforce;
  pBuf->mRearDownforce = info.mRearDownforce;

  // State/damage info
  pBuf->mFuel= info.mFuel;
  pBuf->mEngineMaxRPM = info.mEngineMaxRPM;
  pBuf->mScheduledStops = info.mScheduledStops;
  pBuf->mOverheating = info.mOverheating;
  pBuf->mDetached = info.mDetached;
  pBuf->mHeadlights = info.mHeadlights;
  memcpy(pBuf->mDentSeverity, info.mDentSeverity, sizeof(pBuf->mDentSeverity));
  pBuf->mLastImpactET = info.mLastImpactET;
  pBuf->mLastImpactMagnitude = info.mLastImpactMagnitude;
  pBuf->mLastImpactPos = { info.mLastImpactPos.x, info.mLastImpactPos.y, info.mLastImpactPos.z };

  // Expanded
  pBuf->mEngineTorque = info.mEngineTorque;
  pBuf->mCurrentSector = info.mCurrentSector;
  pBuf->mSpeedLimiter = info.mSpeedLimiter;
  pBuf->mMaxGears = info.mMaxGears;
  pBuf->mFrontTireCompoundIndex = info.mFrontTireCompoundIndex;
  pBuf->mRearTireCompoundIndex = info.mRearTireCompoundIndex;
  pBuf->mFuelCapacity = info.mFuelCapacity;
  pBuf->mFrontFlapActivated = info.mFrontFlapActivated;
  pBuf->mRearFlapActivated = info.mRearFlapActivated;
  pBuf->mRearFlapLegalStatus = info.mRearFlapLegalStatus;
  pBuf->mIgnitionStarter = info.mIgnitionStarter;

  memcpy(pBuf->mFrontTireCompoundName, info.mFrontTireCompoundName, sizeof(pBuf->mFrontTireCompoundName));
  memcpy(pBuf->mRearTireCompoundName, info.mRearTireCompoundName, sizeof(pBuf->mRearTireCompoundName));

  pBuf->mSpeedLimiterAvailable = info.mSpeedLimiterAvailable;
  pBuf->mAntiStallActivated = info.mAntiStallActivated;
  pBuf->mVisualSteeringWheelRange = info.mVisualSteeringWheelRange;

  pBuf->mRearBrakeBias = info.mRearBrakeBias;
  pBuf->mTurboBoostPressure = info.mTurboBoostPressure;
  memcpy(pBuf->mPhysicsToGraphicsOffset, info.mPhysicsToGraphicsOffset, sizeof(pBuf->mPhysicsToGraphicsOffset));
  pBuf->mPhysicalSteeringWheelRange = info.mPhysicalSteeringWheelRange;

  // TelemWheelV01
  for (int i = 0; i < 4; ++i) {
    pBuf->mWheels[i].mSuspensionDeflection = info.mWheel[i].mSuspensionDeflection;
    pBuf->mWheels[i].mRideHeight = info.mWheel[i].mRideHeight;
    pBuf->mWheels[i].mSuspForce = info.mWheel[i].mSuspForce;
    pBuf->mWheels[i].mBrakeTemp = info.mWheel[i].mBrakeTemp;
    pBuf->mWheels[i].mBrakePressure = info.mWheel[i].mBrakePressure;

    pBuf->mWheels[i].mRotation = info.mWheel[i].mRotation;
    pBuf->mWheels[i].mLateralPatchVel = info.mWheel[i].mLateralPatchVel;
    pBuf->mWheels[i].mLongitudinalPatchVel = info.mWheel[i].mLongitudinalPatchVel;
    pBuf->mWheels[i].mLateralGroundVel = info.mWheel[i].mLateralGroundVel;
    pBuf->mWheels[i].mLongitudinalGroundVel = info.mWheel[i].mLongitudinalGroundVel;
    pBuf->mWheels[i].mCamber = info.mWheel[i].mCamber;
    pBuf->mWheels[i].mLateralForce = info.mWheel[i].mLateralForce;
    pBuf->mWheels[i].mLongitudinalForce = info.mWheel[i].mLongitudinalForce;
    pBuf->mWheels[i].mTireLoad = info.mWheel[i].mTireLoad;

    pBuf->mWheels[i].mGripFract = info.mWheel[i].mGripFract;
    pBuf->mWheels[i].mPressure = info.mWheel[i].mPressure;
    memcpy(pBuf->mWheels[i].mTemperature, info.mWheel[i].mTemperature, sizeof(pBuf->mWheels[i].mTemperature));
    pBuf->mWheels[i].mWear = info.mWheel[i].mWear;
    strcpy_s(pBuf->mWheels[i].mTerrainName, info.mWheel[i].mTerrainName);
    pBuf->mWheels[i].mSurfaceType = info.mWheel[i].mSurfaceType;
    pBuf->mWheels[i].mFlat = info.mWheel[i].mFlat;
    pBuf->mWheels[i].mDetached = info.mWheel[i].mDetached;

    pBuf->mWheels[i].mVerticalTireDeflection = info.mWheel[i].mVerticalTireDeflection;
    pBuf->mWheels[i].mWheelYLocation = info.mWheel[i].mWheelYLocation;
    pBuf->mWheels[i].mToe = info.mWheel[i].mToe;

    pBuf->mWheels[i].mTireCarcassTemperature = info.mWheel[i].mTireCarcassTemperature;
    memcpy(pBuf->mWheels[i].mTireInnerLayerTemperature, info.mWheel[i].mTireInnerLayerTemperature, sizeof(pBuf->mWheels[i].mTireInnerLayerTemperature));
  }

  double interTicksBegin = 0.0;
  // NOTE: makes sense only with QPC
  if (msDebugOutputLevel >= DebugLevel::Perf)
    interTicksBegin = TicksNow();

  //
  // Interpolation of scoring info
  //
  // In rF2 scoring update happens roughly every 200ms,
  // so only interpolate if we're roughly within that time delta.
  // Longer delta means a pause or a hiccup, and interpolation will go
  // through the roof if there are no bounds.
  //
  if (mDelta > 0.0 && mDelta < 0.22) {
    // ScoringInfoV01
    pBuf->mCurrentET = mScoringInfo.mCurrentET + mDelta;
    pBuf->mInRealtime = mInRealtime;

    for (int i = 0; i < rF2State::MAX_VSI_SIZE; ++i) {
      if (i < mScoringInfo.mNumVehicles) {
        // nlerp between begin and end orientations.
        auto const nlerpQuat = rF2Quat::Nlerp(
          mScoringInfo.mVehicles[i].mOriQuatBegin,
          mScoringInfo.mVehicles[i].mOriQuatEnd,
          mDelta);

        // Calucalte estimated offset.
        rF2Vec3 offset = mScoringInfo.mVehicles[i].mLocalVelEnd * mDelta;
        offset.Rotate(nlerpQuat);

        // Calculate estimated new position.
        pBuf->mVehicles[i].mPos = mScoringInfo.mVehicles[i].mPos + offset;

        // Calculate estimated orientation, speed and lap distance.
        auto const euler = nlerpQuat.EulerFromQuat();

        // Don't know why I need to negate here to match rF1 math, can be mistake either here or there.
        pBuf->mVehicles[i].mYaw = -euler.yaw;
        pBuf->mVehicles[i].mPitch = euler.pitch;
        pBuf->mVehicles[i].mRoll = euler.roll;

        // Calculate estimated speed and advance
        auto const localVelEst = mScoringInfo.mVehicles[i].mLocalVel + (mScoringInfo.mVehicles[i].mLocalAccel * mDelta);

        // Verify.
        pBuf->mVehicles[i].mSpeed = sqrt((localVelEst.x * localVelEst.x) + (localVelEst.y * localVelEst.y) + (localVelEst.z * localVelEst.z));
        pBuf->mVehicles[i].mLapDist = mScoringInfo.mVehicles[i].mLapDist - localVelEst.z * mDelta;

        continue;
      }
    }
    
    if (msDebugOutputLevel >= DebugLevel::Perf) {
      static double interTicksTotal = 0.0;
      static int interUpdates = 1;

      double const interTicks = TicksNow() - interTicksBegin;
      interTicksTotal += interTicks;
      DEBUG_FLOAT2(DebugLevel::Perf, "Interpolation time elapsed: ", interTicks);
      DEBUG_FLOAT2(DebugLevel::Perf, "Avg interpolation time: ", interTicksTotal / interUpdates);

      ++interUpdates;
    }
  }
}


void SharedMemoryPlugin::UpdateScoringHelper(double const ticksNow, ScoringInfoV01 const& info)
{
  mLastScoringUpdate = ticksNow;

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

  ////////////////////////////////////////////////////
  // Update internal state (needed for interpolation).
  ////////////////////////////////////////////////////
  mScoringInfo.mCurrentET = info.mCurrentET;
  mScoringInfo.mNumVehicles = info.mNumVehicles;
  strcpy(mScoringInfo.mPlrFileName, info.mPlrFileName);
  for (int i = 0; i < info.mNumVehicles; ++i) {
    mScoringInfo.mVehicles[i].mLapDist = info.mVehicle[i].mLapDist;
    mScoringInfo.mVehicles[i].mPos = { info.mVehicle[i].mPos.x, info.mVehicle[i].mPos.y, info.mVehicle[i].mPos.z };
    mScoringInfo.mVehicles[i].mLocalVel = { info.mVehicle[i].mLocalVel.x, info.mVehicle[i].mLocalVel.y, info.mVehicle[i].mLocalVel.z };
    mScoringInfo.mVehicles[i].mLocalAccel = { info.mVehicle[i].mLocalAccel.x, info.mVehicle[i].mLocalAccel.y, info.mVehicle[i].mLocalAccel.z };

    // Calculate nlerp begin/end quaternions for this vehicle.
    rF2Quat& oriQuatBegin = mScoringInfo.mVehicles[i].mOriQuatBegin;
    oriQuatBegin.ConvertMatToQuat(info.mVehicle[i].mOri);
    oriQuatBegin.Normalize();

    // Smoothing factors help with keeping average delta between real (telemetry) positions
    // and interpolated positions smaller, and were found experimentally.
    // Small accceleration factors are probably helpful due to acceleration not being linear (m/s^2).
    static auto const RF2_SHARED_MEMORY_ROT_ACC_SMOOTH_FACTOR = 0.01;
    static auto const RF2_SHARED_MEMORY_VEL_ACC_SMOOTH_FACTOR = 0.03;
    static auto const RF2_SHARED_MEMORY_ROT_SMOOTH_FACTOR = 0.3;

    // Calculate estimated local rotation and speed in one second.
    rF2Vec3 localRot(info.mVehicle[i].mLocalRot);
    rF2Vec3 const localRotAccel(info.mVehicle[i].mLocalRotAccel);
    localRot += localRotAccel * RF2_SHARED_MEMORY_ROT_ACC_SMOOTH_FACTOR;

    rF2Vec3 localVel(info.mVehicle[i].mLocalVel);
    rF2Vec3 const localAccel(info.mVehicle[i].mLocalAccel);
    localVel += localAccel * RF2_SHARED_MEMORY_VEL_ACC_SMOOTH_FACTOR;

    // Save end velocity for offset calculation during interpolation.
    mScoringInfo.mVehicles[i].mLocalVelEnd = localVel;

    // Calculate estimated world rotation in one second.
    rF2Vec3 wRot = localRot * RF2_SHARED_MEMORY_ROT_SMOOTH_FACTOR;
    wRot.Rotate(oriQuatBegin);

    // Calculate estimated rotation quaternion.
    rF2Quat rotQuat;
    rotQuat.ConvertEulerToQuat(wRot.x, wRot.y, wRot.z);
    rotQuat.Normalize();

    // Caluclate nlerp end quaternion.
    rF2Quat& oriQuatEnd = mScoringInfo.mVehicles[i].mOriQuatEnd;
    oriQuatEnd = oriQuatBegin;
    oriQuatEnd *= rotQuat;
    oriQuatEnd.Normalize();
  }

  //////////////////////////////////
  // ScoringInfoV01
  //////////////////////////////////
  pBuf->mSession = info.mSession;
  pBuf->mCurrentET = info.mCurrentET;
  pBuf->mEndET = info.mEndET;
  pBuf->mMaxLaps = info.mMaxLaps;
  pBuf->mLapDist = info.mLapDist;
  pBuf->mNumVehicles = info.mNumVehicles;

  pBuf->mGamePhase = info.mGamePhase;
  pBuf->mYellowFlagState = info.mYellowFlagState;

  memcpy(pBuf->mSectorFlag, info.mSectorFlag, sizeof(pBuf->mSectorFlag));
  pBuf->mStartLight = info.mStartLight;
  pBuf->mNumRedLights = info.mNumRedLights;
  pBuf->mInRealtime = mInRealtime;
  strcpy_s(pBuf->mPlayerName, info.mPlayerName);
  strcpy_s(pBuf->mPlrFileName, info.mPlrFileName);

  // Weather
  pBuf->mDarkCloud = info.mDarkCloud;
  pBuf->mRaining = info.mRaining;
  pBuf->mAmbientTemp = info.mAmbientTemp;
  pBuf->mTrackTemp = info.mTrackTemp;
  pBuf->mWind = { info.mWind.x, info.mWind.y, info.mWind.z };
  pBuf->mMinPathWetness = info.mMinPathWetness;
  pBuf->mMaxPathWetness = info.mMaxPathWetness;

  // Physics options received via SetPhysicsOptions.
  pBuf->mInvulnerable = mInvulnerable;

  ///////////////////////////////////////
  // VehicleScoringInfoV01
  ///////////////////////////////////////
  for (int i = 0; i < info.mNumVehicles; ++i) {
    pBuf->mVehicles[i].mID = info.mVehicle[i].mID;
    strcpy_s(pBuf->mVehicles[i].mDriverName, info.mVehicle[i].mDriverName);
    strcpy_s(pBuf->mVehicles[i].mVehicleName, info.mVehicle[i].mVehicleName);
    pBuf->mVehicles[i].mTotalLaps = info.mVehicle[i].mTotalLaps;
    pBuf->mVehicles[i].mSector = info.mVehicle[i].mSector;
    pBuf->mVehicles[i].mFinishStatus = info.mVehicle[i].mFinishStatus;
    pBuf->mVehicles[i].mLapDist = info.mVehicle[i].mLapDist;
    pBuf->mVehicles[i].mPathLateral = info.mVehicle[i].mPathLateral;
    pBuf->mVehicles[i].mTrackEdge = info.mVehicle[i].mTrackEdge;

    pBuf->mVehicles[i].mBestSector1 = info.mVehicle[i].mBestSector1;
    pBuf->mVehicles[i].mBestSector2 = info.mVehicle[i].mBestSector2;
    pBuf->mVehicles[i].mBestLapTime = info.mVehicle[i].mBestLapTime;
    pBuf->mVehicles[i].mLastSector1 = info.mVehicle[i].mLastSector1;
    pBuf->mVehicles[i].mLastSector2 = info.mVehicle[i].mLastSector2;
    pBuf->mVehicles[i].mLastLapTime = info.mVehicle[i].mLastLapTime;
    pBuf->mVehicles[i].mCurSector1 = info.mVehicle[i].mCurSector1;
    pBuf->mVehicles[i].mCurSector2 = info.mVehicle[i].mCurSector2;

    pBuf->mVehicles[i].mNumPitstops = info.mVehicle[i].mNumPitstops;
    pBuf->mVehicles[i].mNumPenalties = info.mVehicle[i].mNumPenalties;
    pBuf->mVehicles[i].mIsPlayer = info.mVehicle[i].mIsPlayer;

    pBuf->mVehicles[i].mControl = info.mVehicle[i].mControl;
    pBuf->mVehicles[i].mInPits = info.mVehicle[i].mInPits;
    pBuf->mVehicles[i].mPlace = info.mVehicle[i].mPlace;
    strcpy_s(pBuf->mVehicles[i].mVehicleClass, info.mVehicle[i].mVehicleClass);

    // Dash Indicators
    pBuf->mVehicles[i].mTimeBehindNext = info.mVehicle[i].mTimeBehindNext;
    pBuf->mVehicles[i].mLapsBehindNext = info.mVehicle[i].mLapsBehindNext;
    pBuf->mVehicles[i].mTimeBehindLeader = info.mVehicle[i].mTimeBehindLeader;
    pBuf->mVehicles[i].mLapsBehindLeader = info.mVehicle[i].mLapsBehindLeader;
    pBuf->mVehicles[i].mLapStartET = info.mVehicle[i].mLapStartET;

    // Position and derivatives
    pBuf->mVehicles[i].mPos = { info.mVehicle[i].mPos.x, info.mVehicle[i].mPos.y, info.mVehicle[i].mPos.z };

#ifdef DEBUG_INTERPOLATION
    pBuf->mVehicles[i].mPosScoring = { info.mVehicle[i].mPos.x, info.mVehicle[i].mPos.y, info.mVehicle[i].mPos.z };
    pBuf->mVehicles[i].mLocalVel = { info.mVehicle[i].mLocalVel.x, info.mVehicle[i].mLocalVel.y, info.mVehicle[i].mLocalVel.z };
    pBuf->mVehicles[i].mLocalAccel = { info.mVehicle[i].mLocalAccel.x, info.mVehicle[i].mLocalAccel.y, info.mVehicle[i].mLocalAccel.z };
    pBuf->mVehicles[i].mLocalRot = { info.mVehicle[i].mLocalRot.x, info.mVehicle[i].mLocalRot.y, info.mVehicle[i].mLocalRot.z };
    pBuf->mVehicles[i].mLocalRotAccel = { info.mVehicle[i].mLocalRotAccel.x, info.mVehicle[i].mLocalRotAccel.y, info.mVehicle[i].mLocalRotAccel.z };
    pBuf->mVehicles[i].mOri[RowX] = { info.mVehicle[i].mOri[RowX].x, info.mVehicle[i].mOri[RowX].y, info.mVehicle[i].mOri[RowX].z };
    pBuf->mVehicles[i].mOri[RowY] = { info.mVehicle[i].mOri[RowY].x, info.mVehicle[i].mOri[RowY].y, info.mVehicle[i].mOri[RowY].z };
    pBuf->mVehicles[i].mOri[RowZ] = { info.mVehicle[i].mOri[RowZ].x, info.mVehicle[i].mOri[RowZ].y, info.mVehicle[i].mOri[RowZ].z };
#endif

    pBuf->mVehicles[i].mYaw = atan2(info.mVehicle[i].mOri[RowZ].x, info.mVehicle[i].mOri[RowZ].z);
    pBuf->mVehicles[i].mPitch = atan2(-info.mVehicle[i].mOri[RowY].z,
      sqrt(info.mVehicle[i].mOri[RowX].z * info.mVehicle[i].mOri[RowX].z +
        info.mVehicle[i].mOri[RowZ].z * info.mVehicle[i].mOri[RowZ].z));
    pBuf->mVehicles[i].mRoll = atan2(info.mVehicle[i].mOri[RowY].x,
      sqrt(info.mVehicle[i].mOri[RowX].x * info.mVehicle[i].mOri[RowX].x +
        info.mVehicle[i].mOri[RowZ].x * info.mVehicle[i].mOri[RowZ].x));
    pBuf->mVehicles[i].mSpeed = sqrt((info.mVehicle[i].mLocalVel.x * info.mVehicle[i].mLocalVel.x) +
      (info.mVehicle[i].mLocalVel.y * info.mVehicle[i].mLocalVel.y) +
      (info.mVehicle[i].mLocalVel.z * info.mVehicle[i].mLocalVel.z));

    pBuf->mVehicles[i].mHeadlights = info.mVehicle[i].mHeadlights;
    pBuf->mVehicles[i].mPitState = info.mVehicle[i].mPitState;
    pBuf->mVehicles[i].mServerScored = info.mVehicle[i].mServerScored;
    pBuf->mVehicles[i].mIndividualPhase = info.mVehicle[i].mIndividualPhase;

    pBuf->mVehicles[i].mQualification = info.mVehicle[i].mQualification;

    pBuf->mVehicles[i].mTimeIntoLap = info.mVehicle[i].mTimeIntoLap;
    pBuf->mVehicles[i].mEstimatedLapTime = info.mVehicle[i].mEstimatedLapTime;

    memcpy(pBuf->mVehicles[i].mPitGroup, info.mVehicle[i].mPitGroup, sizeof(pBuf->mVehicles[i].mPitGroup));
    pBuf->mVehicles[i].mFlag = info.mVehicle[i].mFlag;
    pBuf->mVehicles[i].mUnderYellow = info.mVehicle[i].mUnderYellow;
    pBuf->mVehicles[i].mCountLapFlag = info.mVehicle[i].mCountLapFlag;
    pBuf->mVehicles[i].mInGarageStall = info.mVehicle[i].mInGarageStall;
  }
}


void SharedMemoryPlugin::SyncBuffers(bool telemetryOnly)
{
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
}

void SharedMemoryPlugin::UpdateScoring(ScoringInfoV01 const& info)
{
  if (mIsMapped) {
    auto const ticksNow = TicksNow();

    // Make sure we're updating latest buffer (pickup telemetry update).
    if (!mRetryFlip) // If flip wasn't successful, that means write buffer is already up to date.
      SyncBuffers(true /*telemetryOnly*/);
    else
      DEBUG_MSG(DebugLevel::Synchronization, "Skipped initial write buffer sync due to retry mode.");

    // Update write buffer with scoring info.
    UpdateScoringHelper(ticksNow, info);
    
    // Make it available to readers.
    FlipBuffers();

    // Update write buffer with scoring info (to keep scoring info between buffers in sync).
    SyncBuffers(false /*telemetryOnly*/);
  }

  WriteScoringInternals(info);
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
  auto fo = fopen(SharedMemoryPlugin::DEBUG_OUTPUT_FILENAME, "a");
  if (fo != nullptr) {
    va_start(argList, format);
    vfprintf(fo, format, argList);
    va_end(argList);
    fclose(fo);
  }
}

void SharedMemoryPlugin::WriteTelemetryInternals(TelemInfoV01 const& info)
{
  if (!SharedMemoryPlugin::msDebugISIInternals)
    return;

  // Use the incoming data, for now I'll just write some of it to a file to a) make sure it
  // is working, and b) explain the coordinate system a little bit (see header for more info)
  auto fo = fopen(SharedMemoryPlugin::INTERNALS_TELEMETRY_FILENAME, "a");
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
  auto fo = fopen(SharedMemoryPlugin::INTERNALS_SCORING_FILENAME, "a");
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
