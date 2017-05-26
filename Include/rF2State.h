/*
Definition of rF2State structure and related types.

Author: The Iron Wolf (vleonavicius@hotmail.com)


Description:
  rF2State is the structure that maps game state into memory mapped file.  It is 
  essentially a combination of ISI's Internals Plugin #7 structures defined in InternalsPlugin.hpp.
  I've kept comments to reflect relationships to the ISI types.

  rF2State uses different x64 default 16 byte pack.  I've also added 3D math helpers to make code more readable.

  Parts of types different from ISI types are tagged with comments:
    - MM_NEW - added members
    - MM_NOT_USED - present in ISI type, but not in mapped type
*/

#pragma once

// Use 4 to match ISI pack.
#pragma pack(push, 4)
#pragma warning(disable : 4121)   // Alignment sensitivity (ISI sets 4 byte pack)

// 0 Before session has begun
// 1 Reconnaissance laps (race only)
// 2 Grid walk-through (race only)
// 3 Formation lap (race only)
// 4 Starting-light countdown has begun (race only)
// 5 Green flag
// 6 Full course yellow / safety car
// 7 Session stopped
// 8 Session over
enum class rF2GamePhase
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
enum class rF2YellowFlagState
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
enum class rF2SurfaceType
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
enum class rF2Sector
{
  Sector3 = 0,
  Sector1 = 1,
  Sector2 = 2
};

// 0=none, 1=finished, 2=dnf, 3=dq
enum class rF2FinishStatus
{
  None = 0,
  Finished = 1,
  Dnf = 2,
  Dq = 3
};

// who's in control: -1=nobody (shouldn't get this), 0=local player, 1=local AI, 2=remote, 3=replay (shouldn't get this)
enum class rF2Control {
  Nobody = -1,
  Player = 0,
  AI = 1,
  Remote = 2,
  Replay = 3
};

// wheel info (front left, front right, rear left, rear right)
enum class rF2WheelIndex {
  FrontLeft = 0,
  FrontRight = 1,
  RearLeft = 2,
  RearRight = 3
};

// 0=none, 1=request, 2=entering, 3=stopped, 4=exiting
enum class rF2PitState {
  None = 0,
  Request = 1,
  Entering = 2,
  Stopped = 3,
  Exiting = 4
};

// primary flag being shown to vehicle (currently only 0=green or 6=blue)
enum class rF2PrimaryFlag {
  Green = 0,
  Blue = 6
};

// 0 = do not count lap or time, 1 = count lap but not time, 2 = count lap and time
enum class rF2CountLapFlag {
  DoNotCountLap = 0,
  CountLapButNotTime = 1,
  CountLapAndTime = 2
};

// 0=disallowed, 1=criteria detected but not allowed quite yet, 2=allowed
enum class rF2RearFlapLegalStatus {
  Disallowed = 0,
  DetectedButNotAllowedYet = 1,
  Alllowed = 2
};

// 0=off 1=ignition 2=ignition+starter
enum class rF2IgnitionStarterStatus {
  Off = 0,
  Ignition = 1,
  IgnitionAndStarter = 2
};


struct rF2Vec3
{
  double x, y, z;

  void Set(const double a, const double b, const double c) { x = a; y = b; z = c; }

  // Allowed to reference as [0], [1], or [2], instead of .x, .y, or .z, respectively
  double& operator[](long i) { return((&x)[i]); }
  const double& operator[](long i) const { return((&x)[i]); }
};


/////////////////////////////////////
// Based on TelemWheelV01
////////////////////////////////////
struct rF2Wheel
{
  double mSuspensionDeflection;  // meters
  double mRideHeight;            // meters
  double mSuspForce;             // pushrod load in Newtons
  double mBrakeTemp;             // Celsius
  double mBrakePressure;         // currently 0.0-1.0, depending on driver input and brake balance; will convert to true brake pressure (kPa) in future

  double mRotation;              // radians/sec
  double mLateralPatchVel;       // lateral velocity at contact patch
  double mLongitudinalPatchVel;  // longitudinal velocity at contact patch
  double mLateralGroundVel;      // lateral velocity at contact patch
  double mLongitudinalGroundVel; // longitudinal velocity at contact patch
  double mCamber;                // radians (positive is left for left-side wheels, right for right-side wheels)
  double mLateralForce;          // Newtons
  double mLongitudinalForce;     // Newtons
  double mTireLoad;              // Newtons

  double mGripFract;             // an approximation of what fraction of the contact patch is sliding
  double mPressure;              // kPa (tire pressure)
  double mTemperature[3];        // Kelvin (subtract 273.15 to get Celsius), left/center/right (not to be confused with inside/center/outside!)
  double mWear;                  // wear (0.0-1.0, fraction of maximum) ... this is not necessarily proportional with grip loss
  char mTerrainName[16];         // the material prefixes from the TDF file
  unsigned char mSurfaceType;    // 0=dry, 1=wet, 2=grass, 3=dirt, 4=gravel, 5=rumblestrip, 6=special
  bool mFlat;                    // whether tire is flat
  bool mDetached;                // whether wheel is detached

  double mVerticalTireDeflection;// how much is tire deflected from its (speed-sensitive) radius
  double mWheelYLocation;        // wheel's y location relative to vehicle y location
  double mToe;                   // current toe angle w.r.t. the vehicle

  double mTireCarcassTemperature;       // rough average of temperature samples from carcass (Kelvin)
  double mTireInnerLayerTemperature[3]; // rough average of temperature samples from innermost layer of rubber (before carcass) (Kelvin)

  unsigned char mExpansion[24];  // for future use
};

//////////////////////////////////////////////////////////////////////////////////////////
// Identical to TelemInfoV01, except where noted by MM_NEW/MM_NOT_USED comments.
//////////////////////////////////////////////////////////////////////////////////////////
struct rF2VehicleTelemetry
{
  // Time
  long mID;                      // slot ID (note that it can be re-used in multiplayer after someone leaves)
  double mDeltaTime;             // time since last update (seconds)
  double mElapsedTime;           // game session time
  long mLapNumber;               // current lap number
  double mLapStartET;            // time this lap was started
  char mVehicleName[64];         // current vehicle name
  char mTrackName[64];           // current track name

  // Position and derivatives
  rF2Vec3 mPos;                  // world position in meters
  rF2Vec3 mLocalVel;             // velocity (meters/sec) in local vehicle coordinates
  rF2Vec3 mLocalAccel;           // acceleration (meters/sec^2) in local vehicle coordinates

                                 // Orientation and derivatives
  rF2Vec3 mOri[3];               // rows of orientation matrix (use TelemQuat conversions if desired), also converts local
                                 // vehicle vectors into world X, Y, or Z using dot product of rows 0, 1, or 2 respectively
  rF2Vec3 mLocalRot;             // rotation (radians/sec) in local vehicle coordinates
  rF2Vec3 mLocalRotAccel;        // rotational acceleration (radians/sec^2) in local vehicle coordinates

  // Vehicle status
  long mGear;                    // -1=reverse, 0=neutral, 1+=forward gears
  double mEngineRPM;             // engine RPM
  double mEngineWaterTemp;       // Celsius
  double mEngineOilTemp;         // Celsius
  double mClutchRPM;             // clutch RPM

  // Driver input
  double mUnfilteredThrottle;    // ranges  0.0-1.0
  double mUnfilteredBrake;       // ranges  0.0-1.0
  double mUnfilteredSteering;    // ranges -1.0-1.0 (left to right)
  double mUnfilteredClutch;      // ranges  0.0-1.0

  // Filtered input (various adjustments for rev or speed limiting, TC, ABS?, speed sensitive steering, clutch work for semi-automatic shifting, etc.)
  double mFilteredThrottle;      // ranges  0.0-1.0
  double mFilteredBrake;         // ranges  0.0-1.0
  double mFilteredSteering;      // ranges -1.0-1.0 (left to right)
  double mFilteredClutch;        // ranges  0.0-1.0

  // Misc
  double mSteeringShaftTorque;   // torque around steering shaft (used to be mSteeringArmForce, but that is not necessarily accurate for feedback purposes)
  double mFront3rdDeflection;    // deflection at front 3rd spring
  double mRear3rdDeflection;     // deflection at rear 3rd spring

  // Aerodynamics
  double mFrontWingHeight;       // front wing height
  double mFrontRideHeight;       // front ride height
  double mRearRideHeight;        // rear ride height
  double mDrag;                  // drag
  double mFrontDownforce;        // front downforce
  double mRearDownforce;         // rear downforce

  // State/damage info
  double mFuel;                  // amount of fuel (liters)
  double mEngineMaxRPM;          // rev limit
  unsigned char mScheduledStops; // number of scheduled pitstops
  bool  mOverheating;            // whether overheating icon is shown
  bool  mDetached;               // whether any parts (besides wheels) have been detached
  bool  mHeadlights;             // whether headlights are on
  unsigned char mDentSeverity[8];// dent severity at 8 locations around the car (0=none, 1=some, 2=more)
  double mLastImpactET;          // time of last impact
  double mLastImpactMagnitude;   // magnitude of last impact
  rF2Vec3 mLastImpactPos;        // location of last impact

  // Expanded
  double mEngineTorque;          // current engine torque (including additive torque) (used to be mEngineTq, but there's little reason to abbreviate it)
  long mCurrentSector;           // the current sector (zero-based) with the pitlane stored in the sign bit (example: entering pits from third sector gives 0x80000002)
  unsigned char mSpeedLimiter;   // whether speed limiter is on
  unsigned char mMaxGears;       // maximum forward gears
  unsigned char mFrontTireCompoundIndex;   // index within brand
  unsigned char mRearTireCompoundIndex;    // index within brand
  double mFuelCapacity;          // capacity in liters
  unsigned char mFrontFlapActivated;       // whether front flap is activated
  unsigned char mRearFlapActivated;        // whether rear flap is activated
  unsigned char mRearFlapLegalStatus;      // 0=disallowed, 1=criteria detected but not allowed quite yet, 2=allowed
  unsigned char mIgnitionStarter;          // 0=off 1=ignition 2=ignition+starter

  char mFrontTireCompoundName[18];         // name of front tire compound
  char mRearTireCompoundName[18];          // name of rear tire compound

  unsigned char mSpeedLimiterAvailable;    // whether speed limiter is available
  unsigned char mAntiStallActivated;       // whether (hard) anti-stall is activated
  unsigned char mUnused[2];                //
  float mVisualSteeringWheelRange;         // the *visual* steering wheel range

  double mRearBrakeBias;                   // fraction of brakes on rear
  double mTurboBoostPressure;              // current turbo boost pressure if available
  float mPhysicsToGraphicsOffset[3];       // offset from static CG to graphical center
  float mPhysicalSteeringWheelRange;       // the *physical* steering wheel range

  // Future use
  unsigned char mExpansion[152];           // for future use (note that the slot ID has been moved to mID above)

  // keeping this at the end of the structure to make it easier to replace in future versions
  rF2Wheel mWheel[4];                      // wheel info (front left, front right, rear left, rear right)
};


//////////////////////////////////////////////////////////////////////////////////////////
// Identical to ScoringInfoV01, except where noted by MM_NEW/MM_NOT_USED comments.
//////////////////////////////////////////////////////////////////////////////////////////
struct rF2ScoringInfo
{
  char mTrackName[64];           // current track name
  long mSession;                 // current session (0=testday 1-4=practice 5-8=qual 9=warmup 10-13=race)
  double mCurrentET;             // current time
  double mEndET;                 // ending time
  long  mMaxLaps;                // maximum laps
  double mLapDist;               // distance around track
  // MM_NOT_USED
  //char *mResultsStream;          // results stream additions since last update (newline-delimited and NULL-terminated)
  // MM_NEW
#ifdef _AMD64_
  unsigned char pointer1[8];
#else
  unsigned char pointer1[4];
#endif

  long mNumVehicles;             // current number of vehicles

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

  signed char mSectorFlag[3];      // whether there are any local yellows at the moment in each sector (not sure if sector 0 is first or last, so test)
  unsigned char mStartLight;       // start light frame (number depends on track)
  unsigned char mNumRedLights;     // number of red lights in start sequence
  bool mInRealtime;                // in realtime as opposed to at the monitor
  char mPlayerName[32];            // player name (including possible multiplayer override)
  char mPlrFileName[64];           // may be encoded to be a legal filename

                                   // weather
  double mDarkCloud;               // cloud darkness? 0.0-1.0
  double mRaining;                 // raining severity 0.0-1.0
  double mAmbientTemp;             // temperature (Celsius)
  double mTrackTemp;               // temperature (Celsius)
  rF2Vec3 mWind   ;                // wind speed
  double mMinPathWetness;          // minimum wetness on main path 0.0-1.0
  double mMaxPathWetness;          // maximum wetness on main path 0.0-1.0

                                   // Future use
  unsigned char mExpansion[256];

  // MM_NOT_USED
  // keeping this at the end of the structure to make it easier to replace in future versions
  // VehicleScoringInfoV01 *mVehicle; // array of vehicle scoring info's
  // MM_NEW
#ifdef _AMD64_
  unsigned char pointer2[8];
#else
  unsigned char pointer2[4];
#endif
};


//////////////////////////////////////////////////////////////////////////////////////////
// Identical to VehicleScoringInfoV01, except where noted by MM_NEW/MM_NOT_USED comments.
//////////////////////////////////////////////////////////////////////////////////////////
struct rF2VehicleScoring
{
  long mID;                      // slot ID (note that it can be re-used in multiplayer after someone leaves)
  char mDriverName[32];          // driver name
  char mVehicleName[64];         // vehicle name
  short mTotalLaps;              // laps completed
  signed char mSector;           // 0=sector3, 1=sector1, 2=sector2 (don't ask why)
  signed char mFinishStatus;     // 0=none, 1=finished, 2=dnf, 3=dq
  double mLapDist;               // current distance around track
  double mPathLateral;           // lateral position with respect to *very approximate* "center" path
  double mTrackEdge;             // track edge (w.r.t. "center" path) on same side of track as vehicle

  double mBestSector1;           // best sector 1
  double mBestSector2;           // best sector 2 (plus sector 1)
  double mBestLapTime;           // best lap time
  double mLastSector1;           // last sector 1
  double mLastSector2;           // last sector 2 (plus sector 1)
  double mLastLapTime;           // last lap time
  double mCurSector1;            // current sector 1 if valid
  double mCurSector2;            // current sector 2 (plus sector 1) if valid
                                 // no current laptime because it instantly becomes "last"

  short mNumPitstops;            // number of pitstops made
  short mNumPenalties;           // number of outstanding penalties
  bool mIsPlayer;                // is this the player's vehicle

  signed char mControl;          // who's in control: -1=nobody (shouldn't get this), 0=local player, 1=local AI, 2=remote, 3=replay (shouldn't get this)
  bool mInPits;                  // between pit entrance and pit exit (not always accurate for remote vehicles)
  unsigned char mPlace;          // 1-based position
  char mVehicleClass[32];        // vehicle class

                                 // Dash Indicators
  double mTimeBehindNext;        // time behind vehicle in next higher place
  long mLapsBehindNext;          // laps behind vehicle in next higher place
  double mTimeBehindLeader;      // time behind leader
  long mLapsBehindLeader;        // laps behind leader
  double mLapStartET;            // time this lap was started

                                 // Position and derivatives
  rF2Vec3 mPos;                  // world position in meters
  rF2Vec3 mLocalVel;             // velocity (meters/sec) in local vehicle coordinates
  rF2Vec3 mLocalAccel;           // acceleration (meters/sec^2) in local vehicle coordinates

                                 // Orientation and derivatives
  rF2Vec3 mOri[3];               // rows of orientation matrix (use TelemQuat conversions if desired), also converts local
                                 // vehicle vectors into world X, Y, or Z using dot product of rows 0, 1, or 2 respectively
  rF2Vec3 mLocalRot;             // rotation (radians/sec) in local vehicle coordinates
  rF2Vec3 mLocalRotAccel;        // rotational acceleration (radians/sec^2) in local vehicle coordinates

                                 // tag.2012.03.01 - stopped casting some of these so variables now have names and mExpansion has shrunk, overall size and old data locations should be same
  unsigned char mHeadlights;     // status of headlights
  unsigned char mPitState;       // 0=none, 1=request, 2=entering, 3=stopped, 4=exiting
  unsigned char mServerScored;   // whether this vehicle is being scored by server (could be off in qualifying or racing heats)
  unsigned char mIndividualPhase;// game phases (described below) plus 9=after formation, 10=under yellow, 11=under blue (not used)

  long mQualification;           // 1-based, can be -1 when invalid

  double mTimeIntoLap;           // estimated time into lap
  double mEstimatedLapTime;      // estimated laptime used for 'time behind' and 'time into lap' (note: this may changed based on vehicle and setup!?)

  char mPitGroup[24];            // pit group (same as team name unless pit is shared)
  unsigned char mFlag;           // primary flag being shown to vehicle (currently only 0=green or 6=blue)
  bool mUnderYellow;             // whether this car has taken a full-course caution flag at the start/finish line
  unsigned char mCountLapFlag;   // 0 = do not count lap or time, 1 = count lap but not time, 2 = count lap and time
  bool mInGarageStall;           // appears to be within the correct garage stall

  unsigned char mUpgradePack[16];  // Coded upgrades

                                   // Future use
                                   // tag.2012.04.06 - SEE ABOVE!
  unsigned char mExpansion[60];  // for future use
};


//////////////////////////////////////////////////////////////////////////////////////////
// Identical to PhysicsOptionsV01, except where noted by MM_NEW/MM_NOT_USED comments.
//////////////////////////////////////////////////////////////////////////////////////////
struct rF2PhysicsOptions
{
  unsigned char mTractionControl;  // 0 (off) - 3 (high)
  unsigned char mAntiLockBrakes;   // 0 (off) - 2 (high)
  unsigned char mStabilityControl; // 0 (off) - 2 (high)
  unsigned char mAutoShift;        // 0 (off), 1 (upshifts), 2 (downshifts), 3 (all)
  unsigned char mAutoClutch;       // 0 (off), 1 (on)
  unsigned char mInvulnerable;     // 0 (off), 1 (on)
  unsigned char mOppositeLock;     // 0 (off), 1 (on)
  unsigned char mSteeringHelp;     // 0 (off) - 3 (high)
  unsigned char mBrakingHelp;      // 0 (off) - 2 (high)
  unsigned char mSpinRecovery;     // 0 (off), 1 (on)
  unsigned char mAutoPit;          // 0 (off), 1 (on)
  unsigned char mAutoLift;         // 0 (off), 1 (on)
  unsigned char mAutoBlip;         // 0 (off), 1 (on)

  unsigned char mFuelMult;         // fuel multiplier (0x-7x)
  unsigned char mTireMult;         // tire wear multiplier (0x-7x)
  unsigned char mMechFail;         // mechanical failure setting; 0 (off), 1 (normal), 2 (timescaled)
  unsigned char mAllowPitcrewPush; // 0 (off), 1 (on)
  unsigned char mRepeatShifts;     // accidental repeat shift prevention (0-5; see PLR file)
  unsigned char mHoldClutch;       // for auto-shifters at start of race: 0 (off), 1 (on)
  unsigned char mAutoReverse;      // 0 (off), 1 (on)
  unsigned char mAlternateNeutral; // Whether shifting up and down simultaneously equals neutral

                                   // tag.2014.06.09 - yes these are new, but no they don't change the size of the structure nor the address of the other variables in it (because we're just using the existing padding)
  unsigned char mAIControl;        // Whether player vehicle is currently under AI control
  unsigned char mUnused1;          //
  unsigned char mUnused2;          //

  float mManualShiftOverrideTime;  // time before auto-shifting can resume after recent manual shift
  float mAutoShiftOverrideTime;    // time before manual shifting can resume after recent auto shift
  float mSpeedSensitiveSteering;   // 0.0 (off) - 1.0
  float mSteerRatioSpeed;          // speed (m/s) under which lock gets expanded to full
};


struct rF2MappedBufferHeader
{
  static int const MAX_MAPPED_VEHICLES = 128;

  bool mCurrentRead;                 // True indicates buffer is safe to read under mutex.
};


struct rF2Telemetry : public rF2MappedBufferHeader
{
  long mNumVehicles;             // current number of vehicles

  rF2VehicleTelemetry mVehicles[rF2MappedBufferHeader::MAX_MAPPED_VEHICLES];
};


struct rF2Scoring : public rF2MappedBufferHeader
{
  rF2ScoringInfo mScoringInfo;
  rF2VehicleScoring mVehicles[rF2MappedBufferHeader::MAX_MAPPED_VEHICLES];
};


struct rF2Extended : public rF2MappedBufferHeader
{
  char mVersion[8];                            // API version
  bool is64bit;                                // Is 64bit plugin?

  // Damage tracking:
  double mMaxImpactMagnitude;                 // Max impact magnitude.  Tracked on every telemetry update, and reset on visit to pits or Session restart.
  double mAccumulatedImpactMagnitude;         // Accumulated impact magnitude.  Tracked on every telemetry update, and reset on visit to pits or Session restart.

  // Physics options (updated on session start):
  rF2PhysicsOptions mPhysics;

  // Function call based flags:
  bool mInRealtimeFC;                         // in realtime as opposed to at the monitor (reported via last EnterRealtime/ExitRealtime calls).
  bool mMultimediaThreadStarted;              // multimedia thread started (reported via ThreadStarted/ThreadStopped calls).
  bool mSimulationThreadStarted;              // simulation thread started (reported via ThreadStarted/ThreadStopped calls).
};


static_assert(sizeof(rF2VehicleTelemetry) == sizeof(TelemInfoV01), "rF2VehicleTelemetry and TelemInfoV01 structures are out of sync");
static_assert(sizeof(rF2ScoringInfo) == sizeof(ScoringInfoV01), "rF2ScoringInfo and ScoringInfoV01 structures are out of sync");
static_assert(sizeof(rF2VehicleScoring) == sizeof(VehicleScoringInfoV01), "rF2VehicleScoring and VehicleScoringInfoV01 structures are out of sync");
static_assert(sizeof(rF2PhysicsOptions) == sizeof(PhysicsOptionsV01), "rF2PhysicsOptions and PhysicsOptionsV01 structures are out of sync");

#pragma pack(pop)
