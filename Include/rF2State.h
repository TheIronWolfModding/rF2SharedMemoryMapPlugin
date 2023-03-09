/*
Definition of mapped rF2 structures and related types.

Author: The Iron Wolf (vleonavicius@hotmail.com)
Website: thecrewchief.org

Description:
  This file contains structures that are written to memory mapped files.  Those
  essentially mirror ISI's Internals Plugin #7 structures defined in InternalsPlugin.hpp,
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

// 0=no change, 1=go active, 2=head for pits
enum class rF2SafetyCarInstruction {
  NoChange = 0,
  GoActive = 1,
  HeadForPits = 2
};


/////////////////////////////////////
// Based on TelemVect3
/////////////////////////////////////
struct rF2Vec3
{
  double x, y, z;
};
static_assert(sizeof(rF2Vec3) == sizeof(TelemVect3), "rF2Vec3 and TelemVect3 structures are out of sync");


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
  unsigned char mStaticUndeflectedRadius; // tire radius in centimeters

  double mVerticalTireDeflection;// how much is tire deflected from its (speed-sensitive) radius
  double mWheelYLocation;        // wheel's y location relative to vehicle y location
  double mToe;                   // current toe angle w.r.t. the vehicle

  double mTireCarcassTemperature;       // rough average of temperature samples from carcass (Kelvin)
  double mTireInnerLayerTemperature[3]; // rough average of temperature samples from innermost layer of rubber (before carcass) (Kelvin)

  unsigned char mExpansion[24];  // for future use
};
static_assert(sizeof(rF2Wheel) == sizeof(TelemWheelV01), "rF2Wheel and TelemWheelV01 structures are out of sync");

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

  double mBatteryChargeFraction; // Battery charge as fraction [0.0-1.0]

  // electric boost motor
  double mElectricBoostMotorTorque; // current torque of boost motor (can be negative when in regenerating mode)
  double mElectricBoostMotorRPM; // current rpm of boost motor
  double mElectricBoostMotorTemperature; // current temperature of boost motor
  double mElectricBoostWaterTemperature; // current water temperature of boost motor cooler if present (0 otherwise)
  unsigned char mElectricBoostMotorState; // 0=unavailable 1=inactive, 2=propulsion, 3=regeneration

  // Future use
  unsigned char mExpansion[111]; // for future use (note that the slot ID has been moved to mID above)


  // keeping this at the end of the structure to make it easier to replace in future versions
  rF2Wheel mWheels[4];                     // wheel info (front left, front right, rear left, rear right)
};
static_assert(sizeof(rF2VehicleTelemetry) == sizeof(TelemInfoV01), "rF2VehicleTelemetry and TelemInfoV01 structures are out of sync");

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
  // 9 Paused (tag.2015.09.14 - this is new, and indicates that this is a heartbeat call to the plugin)
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

  // multiplayer
  unsigned char mGameMode;         // 1 = server, 2 = client, 3 = server and client
  bool mIsPasswordProtected;       // is the server password protected
  unsigned short mServerPort;      // the port of the server (if on a server)
  unsigned long mServerPublicIP;   // the public IP address of the server (if on a server)
  long mMaxPlayers;                // maximum number of vehicles that can be in the session
  char mServerName[32];            // name of the server
  float mStartET;                  // start time (seconds since midnight) of the event

  double mAvgPathWetness;          // average wetness on main path 0.0-1.0

  // Future use
  unsigned char mExpansion[200];

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
static_assert(sizeof(rF2ScoringInfo) == sizeof(ScoringInfoV01), "rF2ScoringInfo and ScoringInfoV01 structures are out of sync");


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

  float mPitLapDist;             // location of pit in terms of lap distance

  float mBestLapSector1;         // sector 1 time from best lap (not necessarily the best sector 1 time)
  float mBestLapSector2;         // sector 2 time from best lap (not necessarily the best sector 2 time)

  // Future use
  // tag.2012.04.06 - SEE ABOVE!
  unsigned char mExpansion[48];  // for future use
};
static_assert(sizeof(rF2VehicleScoring) == sizeof(VehicleScoringInfoV01), "rF2VehicleScoring and VehicleScoringInfoV01 structures are out of sync");


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
static_assert(sizeof(rF2PhysicsOptions) == sizeof(PhysicsOptionsV01), "rF2PhysicsOptions and PhysicsOptionsV01 structures are out of sync");


//////////////////////////////////////////////////////////////////////////////////////////
// Identical to GraphicsInfoV02, except where noted by MM_NEW/MM_NOT_USED comments.
//////////////////////////////////////////////////////////////////////////////////////////
struct rF2GraphicsInfo
{
  rF2Vec3 mCamPos;              // camera position
  rF2Vec3 mCamOri[3];           // rows of orientation matrix (use TelemQuat conversions if desired), also converts local
  HWND mHWND;                   // app handle

  double mAmbientRed;
  double mAmbientGreen;
  double mAmbientBlue;

  long mID;                      // slot ID being viewed (-1 if invalid)

  // Camera types (some of these may only be used for *setting* the camera type in WantsToViewVehicle())
  //    0  = TV cockpit
  //    1  = cockpit
  //    2  = nosecam
  //    3  = swingman
  //    4  = trackside (nearest)
  //    5  = onboard000
  //       :
  //       :
  // 1004  = onboard999
  // 1005+ = (currently unsupported, in the future may be able to set/get specific trackside camera)
  long mCameraType;              // see above comments for possible values

  unsigned char mExpansion[128]; // for future use (possibly camera name)
};
static_assert(sizeof(rF2GraphicsInfo) == sizeof(GraphicsInfoV02), "rF2GraphicsInfo and GraphicsInfoV02 structures are out of sync");


//////////////////////////////////////////////////////////////////////////////////////////
// Identical to TrackRulesCommandV01, except where noted by MM_NEW/MM_NOT_USED comments.  Renamed to match plugin convention.
//////////////////////////////////////////////////////////////////////////////////////////
enum class rF2TrackRulesCommand
{
  AddFromTrack = 0,             // crossed s/f line for first time after full-course yellow was called
  AddFromPit,                   // exited pit during full-course yellow
  AddFromUndq,                  // during a full-course yellow, the admin reversed a disqualification
  RemoveToPit,                  // entered pit during full-course yellow
  RemoveToDnf,                  // vehicle DNF'd during full-course yellow
  RemoveToDq,                   // vehicle DQ'd during full-course yellow
  RemoveToUnloaded,             // vehicle unloaded (possibly kicked out or banned) during full-course yellow
  MoveToBack,                   // misbehavior during full-course yellow, resulting in the penalty of being moved to the back of their current line
  LongestTime,                  // misbehavior during full-course yellow, resulting in the penalty of being moved to the back of the longest line
  //------------------
  Maximum                       // should be last
};
static_assert(sizeof(rF2TrackRulesCommand) == sizeof(TrackRulesCommandV01), "rF2TrackRulesCommand and TrackRulesCommandV01 enums are out of sync");


//////////////////////////////////////////////////////////////////////////////////////////
// Identical to TrackRulesActionV01, except where noted by MM_NEW/MM_NOT_USED comments.
//////////////////////////////////////////////////////////////////////////////////////////
struct rF2TrackRulesAction
{
  // input only
  rF2TrackRulesCommand mCommand;        // recommended action
  long mID;                             // slot ID if applicable
  double mET;                           // elapsed time that event occurred, if applicable
};
static_assert(sizeof(rF2TrackRulesAction) == sizeof(TrackRulesActionV01), "rF2TrackRulesAction and TrackRulesActionV01 structs are out of sync");


//////////////////////////////////////////////////////////////////////////////////////////
// Identical to TrackRulesColumnV01, except where noted by MM_NEW/MM_NOT_USED comments.  Renamed to match plugin convention.
//////////////////////////////////////////////////////////////////////////////////////////
enum class rF2TrackRulesColumn
{
  LeftLane = 0,                  // left (inside)
  MidLefLane,                    // mid-left
  MiddleLane,                    // middle
  MidrRghtLane,                  // mid-right
  RightLane,                     // right (outside)
  //------------------
  MaxLanes,                      // should be after the valid static lane choices
  //------------------
  Invalid = MaxLanes,            // currently invalid (hasn't crossed line or in pits/garage)
  FreeChoice,                    // free choice (dynamically chosen by driver)
  Pending,                       // depends on another participant's free choice (dynamically set after another driver chooses)
  //------------------
  Maximum                        // should be last
};
static_assert(sizeof(rF2TrackRulesColumn) == sizeof(TrackRulesColumnV01), "rF2TrackRulesColumn and TrackRulesColumnV01 enums are out of sync");


//////////////////////////////////////////////////////////////////////////////////////////
// Identical to TrackRulesParticipantV01, except where noted by MM_NEW/MM_NOT_USED comments.
//////////////////////////////////////////////////////////////////////////////////////////
struct rF2TrackRulesParticipant
{
  // input only
  long mID;                             // slot ID
  short mFrozenOrder;                   // 0-based place when caution came out (not valid for formation laps)
  short mPlace;                         // 1-based place (typically used for the initialization of the formation lap track order)
  float mYellowSeverity;                // a rating of how much this vehicle is contributing to a yellow flag (the sum of all vehicles is compared to TrackRulesV01::mSafetyCarThreshold)
  double mCurrentRelativeDistance;      // equal to ( ( ScoringInfoV01::mLapDist * this->mRelativeLaps ) + VehicleScoringInfoV01::mLapDist )

  // input/output
  long mRelativeLaps;                   // current formation/caution laps relative to safety car (should generally be zero except when safety car crosses s/f line); this can be decremented to implement 'wave around' or 'beneficiary rule' (a.k.a. 'lucky dog' or 'free pass')
  rF2TrackRulesColumn mColumnAssignment;// which column (line/lane) that participant is supposed to be in
  long mPositionAssignment;             // 0-based position within column (line/lane) that participant is supposed to be located at (-1 is invalid)
  unsigned char mPitsOpen;              // whether the rules allow this particular vehicle to enter pits right now (input is 2=false or 3=true; if you want to edit it, set to 0=false or 1=true)
  bool mUpToSpeed;                      // while in the frozen order, this flag indicates whether the vehicle can be followed (this should be false for somebody who has temporarily spun and hasn't gotten back up to speed yet)
  bool mUnused[2];                      //
  double mGoalRelativeDistance;         // calculated based on where the leader is, and adjusted by the desired column spacing and the column/position assignments
  char mMessage[ 96 ];                  // a message for this participant to explain what is going on (untranslated; it will get run through translator on client machines)

  // future expansion
  unsigned char mExpansion[ 192 ];
};
static_assert(sizeof(rF2TrackRulesParticipant) == sizeof(TrackRulesParticipantV01), "rF2TrackRulesParticipant and TrackRulesParticipantV01 structs are out of sync");


//////////////////////////////////////////////////////////////////////////////////////////
// Identical to TrackRulesStageV01, except where noted by MM_NEW/MM_NOT_USED comments.  Renamed to match plugin convention.
//////////////////////////////////////////////////////////////////////////////////////////
enum class rF2TrackRulesStage
{
  FormationInit = 0,           // initialization of the formation lap
  FormationUpdate,             // update of the formation lap
  Normal,                      // normal (non-yellow) update
  CautionInit,                 // initialization of a full-course yellow
  CautionUpdate,               // update of a full-course yellow
  //------------------
  Maximum                      // should be last
};
static_assert(sizeof(rF2TrackRulesColumn) == sizeof(TrackRulesColumnV01), "rF2TrackRulesColumn and TrackRulesColumnV01 enums are out of sync");


//////////////////////////////////////////////////////////////////////////////////////////
// Identical to TrackRulesV01, except where noted by MM_NEW/MM_NOT_USED comments.
//////////////////////////////////////////////////////////////////////////////////////////
struct rF2TrackRules
{
  // input only
  double mCurrentET;                    // current time
  rF2TrackRulesStage mStage;            // current stage
  rF2TrackRulesColumn mPoleColumn;      // column assignment where pole position seems to be located
  long mNumActions;                     // number of recent actions

  // MM_NOT_USED
  // TrackRulesActionV01 *mAction;         // array of recent actions
  // MM_NEW
#ifdef _AMD64_
  unsigned char pointer1[8];
#else
  unsigned char pointer1[4];
#endif

  long mNumParticipants;                // number of participants (vehicles)

  bool mYellowFlagDetected;             // whether yellow flag was requested or sum of participant mYellowSeverity's exceeds mSafetyCarThreshold
  unsigned char mYellowFlagLapsWasOverridden; // whether mYellowFlagLaps (below) is an admin request (0=no 1=yes 2=clear yellow)

  bool mSafetyCarExists;                // whether safety car even exists
  bool mSafetyCarActive;                // whether safety car is active
  long mSafetyCarLaps;                  // number of laps
  float mSafetyCarThreshold;            // the threshold at which a safety car is called out (compared to the sum of TrackRulesParticipantV01::mYellowSeverity for each vehicle)
  double mSafetyCarLapDist;             // safety car lap distance
  float mSafetyCarLapDistAtStart;       // where the safety car starts from

  float mPitLaneStartDist;              // where the waypoint branch to the pits breaks off (this may not be perfectly accurate)
  float mTeleportLapDist;               // the front of the teleport locations (a useful first guess as to where to throw the green flag)

  // future input expansion
  unsigned char mInputExpansion[ 256 ];

  // input/output
  signed char mYellowFlagState;         // see ScoringInfoV01 for values
  short mYellowFlagLaps;                // suggested number of laps to run under yellow (may be passed in with admin command)

  long mSafetyCarInstruction;           // 0=no change, 1=go active, 2=head for pits
  float mSafetyCarSpeed;                // maximum speed at which to drive
  float mSafetyCarMinimumSpacing;       // minimum spacing behind safety car (-1 to indicate no limit)
  float mSafetyCarMaximumSpacing;       // maximum spacing behind safety car (-1 to indicate no limit)

  float mMinimumColumnSpacing;          // minimum desired spacing between vehicles in a column (-1 to indicate indeterminate/unenforced)
  float mMaximumColumnSpacing;          // maximum desired spacing between vehicles in a column (-1 to indicate indeterminate/unenforced)

  float mMinimumSpeed;                  // minimum speed that anybody should be driving (-1 to indicate no limit)
  float mMaximumSpeed;                  // maximum speed that anybody should be driving (-1 to indicate no limit)

  char mMessage[ 96 ];                  // a message for everybody to explain what is going on (which will get run through translator on client machines)

  // MM_NOT_USED
  // TrackRulesParticipantV01 *mParticipant;         // array of partipants (vehicles)
  // MM_NEW
#ifdef _AMD64_
  unsigned char pointer2[8];
#else
  unsigned char pointer2[4];
#endif

  // future input/output expansion
  unsigned char mInputOutputExpansion[ 256 ];
};
static_assert(sizeof(rF2TrackRules) == sizeof(TrackRulesV01), "rF2TrackRules and TrackRulesV01 structs are out of sync");


//////////////////////////////////////////////////////////////////////////////////////////
// Identical to MultiSessionParticipantV01, except where noted by MM_NEW/MM_NOT_USED comments.
//////////////////////////////////////////////////////////////////////////////////////////
struct rF2MultiSessionParticipant
{
  // input only
  long mID;                             // slot ID (if loaded) or -1 (if currently disconnected)
  char mDriverName[32];               // driver name
  char mVehicleName[64];              // vehicle name
  unsigned char mUpgradePack[16];     // coded upgrades

  float mBestPracticeTime;              // best practice time
  long mQualParticipantIndex;           // once qualifying begins, this becomes valid and ranks participants according to practice time if possible
  float mQualificationTime[4];        // best qualification time in up to 4 qual sessions
  float mFinalRacePlace[4];           // final race place in up to 4 race sessions
  float mFinalRaceTime[4];            // final race time in up to 4 race sessions

                                      // input/output
  bool mServerScored;                   // whether vehicle is allowed to participate in current session
  long mGridPosition;                   // 1-based grid position for current race session (or upcoming race session if it is currently warmup), or -1 if currently disconnected
                                        // long mPitIndex;
                                        // long mGarageIndex;

                                        // future expansion
  unsigned char mExpansion[128];
};
static_assert(sizeof(rF2MultiSessionParticipant) == sizeof(MultiSessionParticipantV01), "rF2MultiSessionParticipant and MultiSessionParticipantV01 structs are out of sync");


//////////////////////////////////////////////////////////////////////////////////////////
// Identical to MultiSessionRulesV01, except where noted by MM_NEW/MM_NOT_USED comments.
//////////////////////////////////////////////////////////////////////////////////////////
struct rF2MultiSessionRules
{
  // input only
  long mSession;                        // current session (0=testday 1-4=practice 5-8=qual 9=warmup 10-13=race)
  long mSpecialSlotID;                  // slot ID of someone who just joined, or -2 requesting to update qual order, or -1 (default/general)
  char mTrackType[32];                // track type from GDB
  long mNumParticipants;                // number of participants (vehicles)

                                        // input/output
  // MM_NOT_USED
  // MultiSessionParticipantV01 *mParticipant;       // array of partipants (vehicles)
  // MM_NEW
#ifdef _AMD64_
  unsigned char pointer1[8];
#else
  unsigned char pointer1[4];
#endif

  long mNumQualSessions;                // number of qualifying sessions configured
  long mNumRaceSessions;                // number of race sessions configured
  long mMaxLaps;                        // maximum laps allowed in current session (LONG_MAX = unlimited) (note: cannot currently edit in *race* sessions)
  long mMaxSeconds;                     // maximum time allowed in current session (LONG_MAX = unlimited) (note: cannot currently edit in *race* sessions)
  char mName[32];                     // untranslated name override for session (please use mixed case here, it should get uppercased if necessary)

                                      // future expansion
  unsigned char mExpansion[256];
};
static_assert(sizeof(rF2MultiSessionRules) == sizeof(MultiSessionRulesV01), "rF2MultiSessionRules and MultiSessionRulesV01 structs are out of sync");


//////////////////////////////////////////////////////////////////////////////////////////
// Identical to PitMenuV01, except where noted by MM_NEW/MM_NOT_USED comments.
//////////////////////////////////////////////////////////////////////////////////////////
struct rF2PitMenu
{
  long mCategoryIndex;                  // index of the current category
  char mCategoryName[32];               // name of the current category (untranslated)

  long mChoiceIndex;                    // index of the current choice (within the current category)
  char mChoiceString[32];               // name of the current choice (may have some translated words)
  long mNumChoices;                     // total number of choices (0 <= mChoiceIndex < mNumChoices)

  unsigned char mExpansion[256];        // for future use
};
static_assert(sizeof(rF2PitMenu) == sizeof(PitMenuV01), "rF2PitMenu and PitMenuV0 structs are out of sync");


//////////////////////////////////////////////////////////////////////////////////////////
// Identical to WeatherControlInfoV01, except where noted by MM_NEW/MM_NOT_USED comments.
//////////////////////////////////////////////////////////////////////////////////////////
struct rF2WeatherControlInfo
{
  // The current conditions are passed in with the API call. The following ET (Elapsed Time) value should typically be far
  // enough in the future that it can be interpolated smoothly, and allow clouds time to roll in before rain starts. In
  // other words you probably shouldn't have mCloudiness and mRaining suddenly change from 0.0 to 1.0 and expect that
  // to happen in a few seconds without looking crazy.
  double mET;                           // when you want this weather to take effect

  // mRaining[1][1] is at the origin (2013.12.19 - and currently the only implemented node), while the others
  // are spaced at <trackNodeSize> meters where <trackNodeSize> is the maximum absolute value of a track vertex
  // coordinate (and is passed into the API call).
  double mRaining[3][3];            // rain (0.0-1.0) at different nodes

  double mCloudiness;                   // general cloudiness (0.0=clear to 1.0=dark), will be automatically overridden to help ensure clouds exist over rainy areas
  double mAmbientTempK;                 // ambient temperature (Kelvin)
  double mWindMaxSpeed;                 // maximum speed of wind (ground speed, but it affects how fast the clouds move, too)

  bool mApplyCloudinessInstantly;       // preferably we roll the new clouds in, but you can instantly change them now
  bool mUnused1;                        //
  bool mUnused2;                        //
  bool mUnused3;                        //

  unsigned char mExpansion[508];      // future use (humidity, pressure, air density, etc.)
};
static_assert(sizeof(rF2WeatherControlInfo) == sizeof(WeatherControlInfoV01), "rF2WeatherControlInfo and WeatherControlInfoV01 structs are out of sync");


///////////////////////////////////////////
// Mapped wrapper structures
///////////////////////////////////////////

struct rF2MappedBufferVersionBlock
{
  unsigned long mVersionUpdateBegin;      // Incremented right before buffer is written to.
  unsigned long mVersionUpdateEnd;        // Incremented after buffer write is done.
};


struct rF2MappedBufferHeader
{
  static int const MAX_MAPPED_VEHICLES = 128;
};


struct rF2MappedBufferHeaderWithSize : public rF2MappedBufferHeader
{
  int mBytesUpdatedHint;              // How many bytes of the structure were written during the last update.
                                      // 0 means unknown (whole buffer should be considered as updated).
};


struct rF2Telemetry : public rF2MappedBufferHeaderWithSize
{
  long mNumVehicles;             // current number of vehicles

  rF2VehicleTelemetry mVehicles[rF2MappedBufferHeader::MAX_MAPPED_VEHICLES];
};


struct rF2Scoring : public rF2MappedBufferHeaderWithSize
{
  rF2ScoringInfo mScoringInfo;
  rF2VehicleScoring mVehicles[rF2MappedBufferHeader::MAX_MAPPED_VEHICLES];
};


struct rF2Rules : public rF2MappedBufferHeaderWithSize
{
  rF2TrackRules mTrackRules;

  rF2TrackRulesAction mActions[rF2MappedBufferHeader::MAX_MAPPED_VEHICLES];
  rF2TrackRulesParticipant mParticipants[rF2MappedBufferHeader::MAX_MAPPED_VEHICLES];
};


struct rF2MultiRules : public rF2MappedBufferHeaderWithSize
{
  rF2MultiSessionRules mMultiSessionRules;
  rF2MultiSessionParticipant mParticipants[rF2MappedBufferHeader::MAX_MAPPED_VEHICLES];
};


// Note: not versioned due to high referesh rate and no need for consistent buffer view.
struct rF2ForceFeedback : public rF2MappedBufferHeader
{
  double mForceValue;  // Current FFB value reported via InternalsPlugin::ForceFeedback.
};


// Note: not versioned due to high referesh rate and no need for consistent buffer view.
struct rF2Graphics : public rF2MappedBufferHeader
{
  rF2GraphicsInfo mGraphicsInfo;
};


struct rF2PitInfo : public rF2MappedBufferHeader
{
  rF2PitMenu mPitMenu;
};


struct rF2TrackedDamage
{
  double mMaxImpactMagnitude;                 // Max impact magnitude.  Tracked on every telemetry update, and reset on visit to pits or Session restart.
  double mAccumulatedImpactMagnitude;         // Accumulated impact magnitude.  Tracked on every telemetry update, and reset on visit to pits or Session restart.
};


struct rF2VehScoringCapture
{
  // VehicleScoringInfoV01 members:
  long mID;                      // slot ID (note that it can be re-used in multiplayer after someone leaves)
  unsigned char mPlace;
  bool mIsPlayer;
  signed char mFinishStatus;     // 0=none, 1=finished, 2=dnf, 3=dq
};


struct rF2SessionTransitionCapture
{
  // ScoringInfoV01 members:
  unsigned char mGamePhase;
  long mSession;

  // VehicleScoringInfoV01 members:
  long mNumScoringVehicles;
  rF2VehScoringCapture mScoringVehicles[rF2MappedBufferHeader::MAX_MAPPED_VEHICLES];
};


struct rF2Weather : public rF2MappedBufferHeader
{
  double mTrackNodeSize;
  rF2WeatherControlInfo mWeatherInfo;
};


struct rF2Extended : public rF2MappedBufferHeader
{
  static int const MAX_MAPPED_IDS = 512;
  static int const MAX_STATUS_MSG_LEN = 128;
  static int const MAX_RULES_INSTRUCTION_MSG_LEN = 96;

  char mVersion[12];                           // API version
  bool is64bit;                                // Is 64bit plugin?

  // Physics options (updated on session start):
  rF2PhysicsOptions mPhysics;

  // Damage tracking for each vehicle (indexed by mID % rF2Extended::MAX_MAPPED_IDS):
  rF2TrackedDamage mTrackedDamages[rF2Extended::MAX_MAPPED_IDS];

  // Function call based flags:
  bool mInRealtimeFC;                         // in realtime as opposed to at the monitor (reported via last EnterRealtime/ExitRealtime calls).
  bool mMultimediaThreadStarted;              // multimedia thread started (reported via ThreadStarted/ThreadStopped calls).
  bool mSimulationThreadStarted;              // simulation thread started (reported via ThreadStarted/ThreadStopped calls).

  bool mSessionStarted;                       // True if Session Started was called.
  ULONGLONG mTicksSessionStarted;             // Ticks when session started.
  ULONGLONG mTicksSessionEnded;               // Ticks when session ended.

  // FUTURE: It might be worth to keep the whole scoring capture as a separate double buffer instead of this.
  rF2SessionTransitionCapture mSessionTransitionCapture;  // Contains partial internals capture at session transition time.

  // Captured non-empty MessageInfoV01::mText message.
  char mDisplayedMessageUpdateCapture[sizeof(decltype(MessageInfoV01::mText))];

  // Direct Memory access stuff
  bool mDirectMemoryAccessEnabled;

  ULONGLONG mTicksStatusMessageUpdated;             // Ticks when status message was updated;
  char mStatusMessage[rF2Extended::MAX_STATUS_MSG_LEN];

  ULONGLONG mTicksLastHistoryMessageUpdated;        // Ticks when last message history message was updated;
  char mLastHistoryMessage[rF2Extended::MAX_STATUS_MSG_LEN];

  float mCurrentPitSpeedLimit;                      // speed limit m/s.

  bool mSCRPluginEnabled;                           // Is Stock Car Rules plugin enabled?
  long mSCRPluginDoubleFileType;                    // Stock Car Rules plugin DoubleFileType value, only meaningful if mSCRPluginEnabled is true.

  ULONGLONG mTicksLSIPhaseMessageUpdated;           // Ticks when last LSI phase message was updated.
  char mLSIPhaseMessage[rF2Extended::MAX_RULES_INSTRUCTION_MSG_LEN];

  ULONGLONG mTicksLSIPitStateMessageUpdated;        // Ticks when last LSI pit state message was updated.
  char mLSIPitStateMessage[rF2Extended::MAX_RULES_INSTRUCTION_MSG_LEN];

  ULONGLONG mTicksLSIOrderInstructionMessageUpdated;     // Ticks when last LSI order instruction message was updated.
  char mLSIOrderInstructionMessage[rF2Extended::MAX_RULES_INSTRUCTION_MSG_LEN];

  ULONGLONG mTicksLSIRulesInstructionMessageUpdated;     // Ticks when last FCY rules message was updated.  Currently, only SCR plugin sets that.
  char mLSIRulesInstructionMessage[rF2Extended::MAX_RULES_INSTRUCTION_MSG_LEN];

  long mUnsubscribedBuffersMask;                  // Currently active UnsbscribedBuffersMask value.  This will be allowed for clients to write to in the future, but not yet.

  bool mHWControlInputEnabled;                    // HWControl input buffer is enabled.
  bool mWeatherControlInputEnabled;               // Weather Control input buffer is enabled.
  bool mRulesControlInputEnabled;                 // Rules Control input buffer is enabled.
  bool mPluginControlInputEnabled;                // Plugin Control input buffer is enabled.
};


struct rF2MappedInputBufferHeader : public rF2MappedBufferHeader
{
  long mLayoutVersion;
};

struct rF2HWControl : public rF2MappedInputBufferHeader
{
  static int const MAX_HWCONTROL_NAME_LEN = 96;

  // Version supported by the _current_ plugin.
  static long const SUPPORTED_LAYOUT_VERSION = 1L;

  char mControlName[rF2HWControl::MAX_HWCONTROL_NAME_LEN];
  double mfRetVal;
};


struct rF2WeatherControl : public rF2MappedInputBufferHeader
{
  // Version supported by the _current_ plugin.
  static long const SUPPORTED_LAYOUT_VERSION = 1L;

  rF2WeatherControlInfo mWeatherInfo;
};


struct rF2RulesControl : public rF2MappedInputBufferHeader
{
  // Version supported by the _current_ plugin.
  static long const SUPPORTED_LAYOUT_VERSION = 1L;

  rF2TrackRules mTrackRules;

  rF2TrackRulesAction mActions[rF2MappedBufferHeader::MAX_MAPPED_VEHICLES];
  rF2TrackRulesParticipant mParticipants[rF2MappedBufferHeader::MAX_MAPPED_VEHICLES];
};


struct rF2PluginControl : public rF2MappedInputBufferHeader
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
