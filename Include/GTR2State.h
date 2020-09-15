/*
Definition of mapped GTR2 structures and related types.

Author: The Iron Wolf (vleonavicius@hotmail.com)
Website: thecrewchief.org

Description:
  This file contains structures that are written to memory mapped files.  Those
  essentially mirror ISI's Internals Plugin structures defined in InternalsPlugin.hpp,
  except for pointer types, which are replaced with dummy char arrays.  Where game's
  structure contains pointer to the array that we'd like to expose, it is exposed as separate
  member variable with fixed array size.

  Those exposed structures are mostly memcpy'ed one to one from ISI types, so it is critical
  for layout, padding, pack and size to match exactly.

  I've kept comments to reflect relationships to the ISI types.

  Parts of types different from ISI types are tagged with comments:
    - MM_NEW - added members
    - MM_NOT_USED - present in ISI type, but not in mapped type
*/
#pragma once

// Use 4 to match ISI pack.
#pragma pack(push, 4)
#pragma warning(disable : 4121) // Alignment sensitivity (ISI sets 4 byte pack)

// 0 Before session has begun
// 1 Reconnaissance laps (race only)
// 2 Grid walk-through (race only)
// 3 Formation lap (race only)
// 4 Starting-light countdown has begun (race only)
// 5 Green flag
// 6 Full course yellow / safety car
// 7 Session stopped
// 8 Session over
enum class GTR2GamePhase
{
  Garage = 0,
  WarmUp = 1,
  GridWalk = 2,
  Formation = 3,
  Countdown = 4,
  GreenFlag = 5,
  FullCourseYellow = 6,
  SessionStopped = 7,
  SessionOver = 8
};

// Yellow flag states (applies to full-course only)
// -1 Invalid
//  0 None
//  1 Pending
//  2 Pits closed
//  3 Pit lead lap
//  4 Pits open
//  5 Last lap
//  6 Resume
//  7 Race halt (not currently used)
enum class GTR2YellowFlagState
{
  Invalid = -1,
  NoFlag = 0,
  Pending = 1,
  PitClosed = 2,
  PitLeadLap = 3,
  PitOpen = 4,
  LastLap = 5,
  Resume = 6,
  RaceHalt = 7
};

// 0=dry, 1=wet, 2=grass, 3=dirt, 4=gravel, 5=rumblestrip, 6=special
enum class GTR2SurfaceType
{
  Dry = 0,
  Wet = 1,
  Grass = 2,
  Dirt = 3,
  Gravel = 4,
  Kerb = 5,
  Special = 6
};

// 0=sector3, 1=sector1, 2=sector2 (don't ask why)
enum class GTR2Sector
{
  Sector3 = 0,
  Sector1 = 1,
  Sector2 = 2
};

// 0=none, 1=finished, 2=dnf, 3=dq
enum class GTR2FinishStatus
{
  None = 0,
  Finished = 1,
  Dnf = 2,
  Dq = 3
};

// who's in control: -1=nobody (shouldn't get this), 0=local player, 1=local AI, 2=remote, 3=replay (shouldn't get this)
enum class GTR2Control
{
  Nobody = -1,
  Player = 0,
  AI = 1,
  Remote = 2,
  Replay = 3
};

// wheel info (front left, front right, rear left, rear right)
enum class GTR2WheelIndex
{
  FrontLeft = 0,
  FrontRight = 1,
  RearLeft = 2,
  RearRight = 3
};

// 0=none, 1=request, 2=entering, 3=stopped, 4=exiting
enum class GTR2PitState
{
  None = 0,
  Request = 1,
  Entering = 2,
  Stopped = 3,
  Exiting = 4
};

// primary flag being shown to vehicle (currently only 0=green or 6=blue)
enum class GTR2PrimaryFlag
{
  Green = 0,
  Blue = 6
};

// 0 = do not count lap or time, 1 = count lap but not time, 2 = count lap and time
enum class GTR2CountLapFlag
{
  DoNotCountLap = 0,
  CountLapButNotTime = 1,
  CountLapAndTime = 2
};

// 0=disallowed, 1=criteria detected but not allowed quite yet, 2=allowed
enum class GTR2RearFlapLegalStatus
{
  Disallowed = 0,
  DetectedButNotAllowedYet = 1,
  Alllowed = 2
};

// 0=off 1=ignition 2=ignition+starter
enum class GTR2IgnitionStarterStatus
{
  Off = 0,
  Ignition = 1,
  IgnitionAndStarter = 2
};

// 0=no change, 1=go active, 2=head for pits
enum class GTR2SafetyCarInstruction
{
  NoChange = 0,
  GoActive = 1,
  HeadForPits = 2
};

/////////////////////////////////////
// Based on GTR2Vec3
/////////////////////////////////////
struct GTR2Vec3
{
  float x, y, z;
};
static_assert(sizeof(GTR2Vec3) == sizeof(TelemVect3), "GTR2Vec3 and TelemVect3 structures are out of sync");

/////////////////////////////////////
// Based on TelemWheelV2
////////////////////////////////////
struct GTR2Wheel
{
  float mRotation;             // radians/sec
  float mSuspensionDeflection; // meters
  float mRideHeight;           // meters
  float mTireLoad;             // Newtons
  float mLateralForce;         // Newtons
  float mGripFract;            // an approximation of what fraction of the contact patch is sliding
  float mBrakeTemp;            // Celsius
  float mPressure;             // kPa
  float mTemperature[3];       // Celsius, left/center/right (not to be confused with inside/center/outside!)

  float mWear;           // wear (0.0-1.0, fraction of maximum) ... this is not necessarily proportional with grip loss
  char mTerrainName[16]; // the material prefixes from the TDF file
  unsigned char mSurfaceType; // 0=dry, 1=wet, 2=grass, 3=dirt, 4=gravel, 5=rumblestrip
  bool mFlat;                 // whether tire is flat
  bool mDetached;             // whether wheel is detached

  // Future use
  unsigned char mExpansion[32];
};
static_assert(sizeof(GTR2Wheel) == sizeof(TelemWheelV2), "GTR2Wheel and TelemWheelV2 structures are out of sync");

//////////////////////////////////////////////////////////////////////////////////////////
// Identical to TelemInfoV2, except where noted by MM_NEW/MM_NOT_USED comments.
//////////////////////////////////////////////////////////////////////////////////////////
struct GTR2VehicleTelemetry
{
  // Time
  float mDeltaTime;      // time since last update (seconds)
  long mLapNumber;       // current lap number
  float mLapStartET;     // time this lap was started
  char mVehicleName[64]; // current vehicle name
  char mTrackName[64];   // current track name

  // Position and derivatives
  GTR2Vec3 mPos;        // world position in meters
  GTR2Vec3 mLocalVel;   // velocity (meters/sec) in local vehicle coordinates
  GTR2Vec3 mLocalAccel; // acceleration (meters/sec^2) in local vehicle coordinates

  // Orientation and derivatives
  GTR2Vec3 mOriX; // top row of orientation matrix (also converts local vehicle vectors into world X using dot product)
  GTR2Vec3 mOriY; // mid row of orientation matrix (also converts local vehicle vectors into world Y using dot product)
  GTR2Vec3 mOriZ; // bot row of orientation matrix (also converts local vehicle vectors into world Z using dot product)
  GTR2Vec3 mLocalRot;      // rotation (radians/sec) in local vehicle coordinates
  GTR2Vec3 mLocalRotAccel; // rotational acceleration (radians/sec^2) in local vehicle coordinates

  // Vehicle status
  long mGear;             // -1=reverse, 0=neutral, 1+=forward gears
  float mEngineRPM;       // engine RPM
  float mEngineWaterTemp; // Celsius
  float mEngineOilTemp;   // Celsius
  float mClutchRPM;       // clutch RPM

  // Driver input
  float mUnfilteredThrottle; // ranges  0.0-1.0
  float mUnfilteredBrake;    // ranges  0.0-1.0
  float mUnfilteredSteering; // ranges -1.0-1.0 (left to right)
  float mUnfilteredClutch;   // ranges  0.0-1.0

  // Misc
  float mSteeringArmForce; // force on steering arms

  // state/damage info
  float mFuel;                    // amount of fuel (liters)
  float mEngineMaxRPM;            // rev limit
  unsigned char mScheduledStops;  // number of scheduled pitstops
  bool mOverheating;              // whether overheating icon is shown
  bool mDetached;                 // whether any parts (besides wheels) have been detached
  unsigned char mDentSeverity[8]; // dent severity at 8 locations around the car (0=none, 1=some, 2=more)
  float mLastImpactET;            // time of last impact
  float mLastImpactMagnitude;     // magnitude of last impact
  GTR2Vec3 mLastImpactPos;        // location of last impact

  // Future use
  unsigned char mExpansion[64];

  // keeping this at the end of the structure to make it easier to replace in future versions
  TelemWheelV2 mWheel[4]; // wheel info (front left, front right, rear left, rear right)
};
static_assert(sizeof(GTR2VehicleTelemetry) == sizeof(TelemInfoV2),
              "GTR2VehicleTelemetry and TelemInfoV01 structures are out of sync");

//////////////////////////////////////////////////////////////////////////////////////////
// Identical to ScoringInfoV01, except where noted by MM_NEW/MM_NOT_USED comments.
//////////////////////////////////////////////////////////////////////////////////////////
struct GTR2ScoringInfo
{
  char mTrackName[64]; // current track name
  long mSession;       // current session
  float mCurrentET;    // current time
  float mEndET;        // ending time
  long mMaxLaps;       // maximum laps
  float mLapDist;      // distance around track

  // MM_NOT_USED
  // char *mResultsStream;          // results stream additions since last update (newline-delimited and
  // NULL-terminated)
  // MM_NEW
  unsigned char pointer1[4];

  long mNumVehicles; // current number of vehicles
                     // Game phases:
  // 0 Before session has begun
  // 1 Reconnaissance laps (race only)
  // 2 Grid walk-through (race only)
  // 3 Formation lap (race only)
  // 4 Starting-light countdown has begun (race only)
  // 5 Green flag
  // 6 Full course yellow / safety car
  // 7 Session stopped
  // 8 Session over
  unsigned char mGamePhase;

  // Yellow flag states (applies to full-course only)
  // -1 Invalid
  //  0 None
  //  1 Pending
  //  2 Pits closed
  //  3 Pit lead lap
  //  4 Pits open
  //  5 Last lap
  //  6 Resume
  //  7 Race halt (not currently used)
  signed char mYellowFlagState;

  signed char mSectorFlag[3];  // whether there are any local yellows at the moment in each sector (not sure if sector 0
                               // is first or last, so test)
  unsigned char mStartLight;   // start light frame (number depends on track)
  unsigned char mNumRedLights; // number of red lights in start sequence
  bool mInRealtime;            // in realtime as opposed to at the monitor
  char mPlayerName[32];        // player name (including possible multiplayer override)
  char mPlrFileName[64];       // may be encoded to be a legal filename

  // weather
  float mDarkCloud;      // cloud darkness? 0.0-1.0
  float mRaining;        // raining severity 0.0-1.0
  float mAmbientTemp;    // temperature (Celsius)
  float mTrackTemp;      // temperature (Celsius)
  GTR2Vec3 mWind;        // wind speed
  float mOnPathWetness;  // on main path 0.0-1.0
  float mOffPathWetness; // on main path 0.0-1.0

  // Future use
  unsigned char mExpansion[256];

  // MM_NOT_USED
  // VehicleScoringInfoV2 *mVehicle;  // array of vehicle scoring info's
  // MM_NEW
  unsigned char pointer2[4];
};
static_assert(sizeof(GTR2ScoringInfo) == sizeof(ScoringInfoV2),
              "GTR2ScoringInfo and ScoringInfoV01 structures are out of sync");

//////////////////////////////////////////////////////////////////////////////////////////
// Identical to VehicleScoringInfoV01, except where noted by MM_NEW/MM_NOT_USED comments.
//////////////////////////////////////////////////////////////////////////////////////////
struct GTR2VehicleScoring
{
  char mDriverName[32];      // driver name
  char mVehicleName[64];     // vehicle name
  short mTotalLaps;          // laps completed
  signed char mSector;       // 0=sector3, 1=sector1, 2=sector2 (don't ask why)
  signed char mFinishStatus; // 0=none, 1=finished, 2=dnf, 3=dq
  float mLapDist;            // current distance around track
  float mPathLateral;        // lateral position with respect to *very approximate* "center" path
  float mTrackEdge;          // track edge (w.r.t. "center" path) on same side of track as vehicle

  float mBestSector1; // best sector 1
  float mBestSector2; // best sector 2 (plus sector 1)
  float mBestLapTime; // best lap time
  float mLastSector1; // last sector 1
  float mLastSector2; // last sector 2 (plus sector 1)
  float mLastLapTime; // last lap time
  float mCurSector1;  // current sector 1 if valid
  float mCurSector2;  // current sector 2 (plus sector 1) if valid
  // no current laptime because it instantly becomes "last"

  short mNumPitstops;     // number of pitstops made
  short mNumPenalties;    // number of outstanding penalties
  bool mIsPlayer;         // is this the player's vehicle
  signed char mControl;   // who's in control: -1=nobody (shouldn't get this), 0=local player, 1=local AI, 2=remote,
                          // 3=replay (shouldn't get this)
  bool mInPits;           // between pit entrance and pit exit (not always accurate for remote vehicles)
  unsigned char mPlace;   // 1-based position
  char mVehicleClass[32]; // vehicle class

  // Dash Indicators
  float mTimeBehindNext;   // time behind vehicle in next higher place
  long mLapsBehindNext;    // laps behind vehicle in next higher place
  float mTimeBehindLeader; // time behind leader
  long mLapsBehindLeader;  // laps behind leader
  float mLapStartET;       // time this lap was started

  // Position and derivatives
  GTR2Vec3 mPos;        // world position in meters
  GTR2Vec3 mLocalVel;   // velocity (meters/sec) in local vehicle coordinates
  GTR2Vec3 mLocalAccel; // acceleration (meters/sec^2) in local vehicle coordinates

  // Orientation and derivatives
  GTR2Vec3 mOriX; // top row of orientation matrix (also converts local vehicle vectors into world X using dot product)
  GTR2Vec3 mOriY; // mid row of orientation matrix (also converts local vehicle vectors into world Y using dot product)
  GTR2Vec3 mOriZ; // bot row of orientation matrix (also converts local vehicle vectors into world Z using dot product)
  GTR2Vec3 mLocalRot;      // rotation (radians/sec) in local vehicle coordinates
  GTR2Vec3 mLocalRotAccel; // rotational acceleration (radians/sec^2) in local vehicle coordinates

  // Future use
  unsigned char mExpansion[128];
};
static_assert(sizeof(GTR2VehicleScoring) == sizeof(VehicleScoringInfoV2),
              "GTR2VehicleScoring and VehicleScoringInfoV01 structures are out of sync");

//////////////////////////////////////////////////////////////////////////////////////////
// Identical to GraphicsInfoV02, except where noted by MM_NEW/MM_NOT_USED comments.
//////////////////////////////////////////////////////////////////////////////////////////
struct GTR2GraphicsInfo
{
  GTR2Vec3 mCamPos; // camera position
  GTR2Vec3 mCamOri; // camera orientation
  HWND mHWND;       // app handle

  float mAmbientRed;
  float mAmbientGreen;
  float mAmbientBlue;
};
static_assert(sizeof(GTR2GraphicsInfo) == sizeof(GraphicsInfoV2),
              "GTR2GraphicsInfo and GraphicsInfoV2 structures are out of sync");

///////////////////////////////////////////
// Mapped wrapper structures
///////////////////////////////////////////

struct GTR2MappedBufferVersionBlock
{
  unsigned long mVersionUpdateBegin; // Incremented right before buffer is written to.
  unsigned long mVersionUpdateEnd;   // Incremented after buffer write is done.
};

struct GTR2MappedBufferHeader
{
  static int const MAX_MAPPED_VEHICLES = 128;
};

struct GTR2MappedBufferHeaderWithSize : public GTR2MappedBufferHeader
{
  int mBytesUpdatedHint; // How many bytes of the structure were written during the last update.
                         // 0 means unknown (whole buffer should be considered as updated).
};

struct GTR2Telemetry : public GTR2MappedBufferHeader
{
  GTR2VehicleTelemetry mPlayerTelemetry;
};

struct GTR2Scoring : public GTR2MappedBufferHeaderWithSize
{
  GTR2ScoringInfo mScoringInfo;
  GTR2VehicleScoring mVehicles[GTR2MappedBufferHeader::MAX_MAPPED_VEHICLES];
};

// Note: not versioned due to high referesh rate and no need for consistent buffer view.
struct GTR2ForceFeedback : public GTR2MappedBufferHeader
{
  double mForceValue; // Current FFB value reported via InternalsPlugin::ForceFeedback.
};

// Note: not versioned due to high referesh rate and no need for consistent buffer view.
struct GTR2Graphics : public GTR2MappedBufferHeader
{
  GTR2GraphicsInfo mGraphicsInfo;
};

struct GTR2TrackedDamage
{
  double mMaxImpactMagnitude; // Max impact magnitude.  Tracked on every telemetry update, and reset on visit to pits or
                              // Session restart.
  double mAccumulatedImpactMagnitude; // Accumulated impact magnitude.  Tracked on every telemetry update, and reset on
                                      // visit to pits or Session restart.
};

struct GTR2VehScoringCapture
{
  // VehicleScoringInfoV01 members:
  long mID; // slot ID (note that it can be re-used in multiplayer after someone leaves)
  unsigned char mPlace;
  bool mIsPlayer;
  signed char mFinishStatus; // 0=none, 1=finished, 2=dnf, 3=dq
};

struct GTR2SessionTransitionCapture
{
  // ScoringInfoV01 members:
  unsigned char mGamePhase;
  long mSession;

  // VehicleScoringInfoV01 members:
  long mNumScoringVehicles;
  GTR2VehScoringCapture mScoringVehicles[GTR2MappedBufferHeader::MAX_MAPPED_VEHICLES];
};

struct GTR2Extended : public GTR2MappedBufferHeader
{
  static int const MAX_MAPPED_IDS = 512;
  static int const MAX_STATUS_MSG_LEN = 128;
  static int const MAX_RULES_INSTRUCTION_MSG_LEN = 96;

  char mVersion[12]; // API version
  bool is64bit;      // Is 64bit plugin?

  // Damage tracking for each vehicle (indexed by mID % GTR2Extended::MAX_MAPPED_IDS):
  GTR2TrackedDamage mTrackedDamages[GTR2Extended::MAX_MAPPED_IDS];

  // Function call based flags:
  bool mInRealtimeFC; // in realtime as opposed to at the monitor (reported via last EnterRealtime/ExitRealtime calls).
  bool mMultimediaThreadStarted; // multimedia thread started (reported via ThreadStarted/ThreadStopped calls).
  bool mSimulationThreadStarted; // simulation thread started (reported via ThreadStarted/ThreadStopped calls).

  bool mSessionStarted;           // True if Session Started was called.
  ULONGLONG mTicksSessionStarted; // Ticks when session started.
  ULONGLONG mTicksSessionEnded;   // Ticks when session ended.

  // FUTURE: It might be worth to keep the whole scoring capture as a separate double buffer instead of this.
  GTR2SessionTransitionCapture
    mSessionTransitionCapture; // Contains partial internals capture at session transition time.

  // Direct Memory access stuff
  bool mDirectMemoryAccessEnabled;

  ULONGLONG mTicksStatusMessageUpdated; // Ticks when status message was updated;
  char mStatusMessage[GTR2Extended::MAX_STATUS_MSG_LEN];

  ULONGLONG mTicksLastHistoryMessageUpdated; // Ticks when last message history message was updated;
  char mLastHistoryMessage[GTR2Extended::MAX_STATUS_MSG_LEN];

  float mCurrentPitSpeedLimit; // speed limit m/s.

  bool mSCRPluginEnabled;        // Is Stock Car Rules plugin enabled?
  long mSCRPluginDoubleFileType; // Stock Car Rules plugin DoubleFileType value, only meaningful if mSCRPluginEnabled is
                                 // true.

  ULONGLONG mTicksLSIPhaseMessageUpdated; // Ticks when last LSI phase message was updated.
  char mLSIPhaseMessage[GTR2Extended::MAX_RULES_INSTRUCTION_MSG_LEN];

  ULONGLONG mTicksLSIPitStateMessageUpdated; // Ticks when last LSI pit state message was updated.
  char mLSIPitStateMessage[GTR2Extended::MAX_RULES_INSTRUCTION_MSG_LEN];

  ULONGLONG mTicksLSIOrderInstructionMessageUpdated; // Ticks when last LSI order instruction message was updated.
  char mLSIOrderInstructionMessage[GTR2Extended::MAX_RULES_INSTRUCTION_MSG_LEN];

  ULONGLONG mTicksLSIRulesInstructionMessageUpdated; // Ticks when last FCY rules message was updated.  Currently, only
                                                     // SCR plugin sets that.
  char mLSIRulesInstructionMessage[GTR2Extended::MAX_RULES_INSTRUCTION_MSG_LEN];

  long mUnsubscribedBuffersMask; // Currently active UnsbscribedBuffersMask value.  This will be allowed for clients to
                                 // write to in the future, but not yet.

  bool mHWControlInputEnabled;      // HWControl input buffer is enabled.
  bool mWeatherControlInputEnabled; // Weather Control input buffer is enabled.
  bool mRulesControlInputEnabled;   // Rules Control input buffer is enabled.
  bool mPluginControlInputEnabled;  // Plugin Control input buffer is enabled.
};

struct GTR2MappedInputBufferHeader : public GTR2MappedBufferHeader
{
  long mLayoutVersion;
};

struct GTR2HWControl : public GTR2MappedInputBufferHeader
{
  static int const MAX_HWCONTROL_NAME_LEN = 96;

  // Version supported by the _current_ plugin.
  static long const SUPPORTED_LAYOUT_VERSION = 1L;

  char mControlName[GTR2HWControl::MAX_HWCONTROL_NAME_LEN];
  float mfRetVal;
};

struct GTR2PluginControl : public GTR2MappedInputBufferHeader
{
  // Version supported by the _current_ plugin.
  static long const SUPPORTED_LAYOUT_VERSION = 1L;

  // Note: turning Scoring update on cannot be requested
  long mRequestEnableBuffersMask;
  bool mRequestHWControlInput;
  bool mRequestWeatherControlInput;
  bool mRequestRulesControlInput;
};

#pragma pack(pop)
