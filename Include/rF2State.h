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

// Keep default x64 pack for maximum performance.  Space savings with pack(1) are less than 1.5k of 52k.
#pragma pack(push, 16)
/*#ifdef _X86_
#pragma pack(show)
#endif
#ifdef _AMD64_
#pragma pack(show)
#endif*/

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


// MM_NEW
struct Euler
{
  double yaw, pitch, roll;

  Euler() {}

  Euler(double yaw, double pitch, double roll) : yaw(yaw), pitch(pitch), roll(roll)
  {}
};


struct rF2Quat
{
  double w, x, y, z;

  // Convert this quaternion to a matrix
  void ConvertQuatToMat(TelemVect3 ori[3]) const
  {
    const double x2 = x + x;
    const double xx = x * x2;
    const double y2 = y + y;
    const double yy = y * y2;
    const double z2 = z + z;
    const double zz = z * z2;
    const double xz = x * z2;
    const double xy = x * y2;
    const double wy = w * y2;
    const double wx = w * x2;
    const double wz = w * z2;
    const double yz = y * z2;
    ori[0][0] = (double) 1.0 - (yy + zz);
    ori[0][1] = xy - wz;
    ori[0][2] = xz + wy;
    ori[1][0] = xy + wz;
    ori[1][1] = (double) 1.0 - (xx + zz);
    ori[1][2] = yz - wx;
    ori[2][0] = xz - wy;
    ori[2][1] = yz + wx;
    ori[2][2] = (double) 1.0 - (xx + yy);
  }

  // Convert a matrix to this quaternion
  void ConvertMatToQuat(const TelemVect3 ori[3])
  {
    const double trace = ori[0][0] + ori[1][1] + ori[2][2] + (double) 1.0;
    if (trace > 0.0625f)
    {
      const double sqrtTrace = sqrt(trace);
      const double s = (double) 0.5 / sqrtTrace;
      w = (double) 0.5 * sqrtTrace;
      x = (ori[2][1] - ori[1][2]) * s;
      y = (ori[0][2] - ori[2][0]) * s;
      z = (ori[1][0] - ori[0][1]) * s;
    }
    else if ((ori[0][0] > ori[1][1]) && (ori[0][0] > ori[2][2]))
    {
      const double sqrtTrace = sqrt((double) 1.0 + ori[0][0] - ori[1][1] - ori[2][2]);
      const double s = (double) 0.5 / sqrtTrace;
      w = (ori[2][1] - ori[1][2]) * s;
      x = (double) 0.5 * sqrtTrace;
      y = (ori[0][1] + ori[1][0]) * s;
      z = (ori[0][2] + ori[2][0]) * s;
    }
    else if (ori[1][1] > ori[2][2])
    {
      const double sqrtTrace = sqrt((double) 1.0 + ori[1][1] - ori[0][0] - ori[2][2]);
      const double s = (double) 0.5 / sqrtTrace;
      w = (ori[0][2] - ori[2][0]) * s;
      x = (ori[0][1] + ori[1][0]) * s;
      y = (double) 0.5 * sqrtTrace;
      z = (ori[1][2] + ori[2][1]) * s;
    }
    else
    {
      const double sqrtTrace = sqrt((double) 1.0 + ori[2][2] - ori[0][0] - ori[1][1]);
      const double s = (double) 0.5 / sqrtTrace;
      w = (ori[1][0] - ori[0][1]) * s;
      x = (ori[0][2] + ori[2][0]) * s;
      y = (ori[1][2] + ori[2][1]) * s;
      z = (double) 0.5 * sqrtTrace;
    }
  }

  // MM_NEW
  void Normalize()
  {
    static auto const RENORMALIZATION_THRESHOLD = 1e-6;

    double square = w * w + x * x + y * y + z * z;
    if (abs(square - 1.0) > RENORMALIZATION_THRESHOLD)
    {
      double magnitude = sqrt(square);
      w /= magnitude;
      x /= magnitude;
      y /= magnitude;
      z /= magnitude;
    }
  }

  // http://www.euclideanspace.com/maths/geometry/rotations/conversions/eulerToQuaternion/
  void ConvertEulerToQuat(double pitch, double yaw, double roll)
  {
    double c1 = cos(yaw * 0.5);
    double s1 = sin(yaw * 0.5);
    double c2 = cos(roll * 0.5);
    double s2 = sin(roll * 0.5);
    double c3 = cos(pitch * 0.5);
    double s3 = sin(pitch * 0.5);
    double c1c2 = c1 * c2;
    double s1s2 = s1 * s2;
    w = c1c2 * c3 - s1s2 * s3;
    x = c1c2 * s3 + s1s2 * c3;
    y = s1 * c2 * c3 + c1 * s2 * s3;
    z = c1 * s2 * c3 - s1 * c2 * s3;
  }


  rF2Quat& operator *=(rF2Quat const& rot)
  {
    //https://www.mathworks.com/help/aeroblks/quaternionmultiplication.html
    //t=q×r=t0+it1+jt2+kt3
    //t0=(r0q0−r1q1−r2q2−r3q3)
    //t1=(r0q1+r1q0−r2q3+r3q2)
    //t2=(r0q2+r1q3+r2q0−r3q1)
    //t3=(r0q3−r1q2+r2q1+r3q0)
    //
    // Meaning: rotate by q then by r.

    double r0 = rot.w;
    double r1 = rot.x;
    double r2 = rot.y;
    double r3 = rot.z;

    double q0 = w;
    double q1 = x;
    double q2 = y;
    double q3 = z;

    double t0 = r0*q0 - r1*q1 - r2*q2 - r3*q3;
    double t1 = r0*q1 + r1*q0 - r2*q3 + r3*q2;
    double t2 = r0*q2 + r1*q3 + r2*q0 - r3*q1;
    double t3 = r0*q3 - r1*q2 + r2*q1 + r3*q0;

    w = t0;
    x = t1;
    y = t2;
    z = t3;

    return *this;
  }

  rF2Quat const operator *(rF2Quat const& rhs)
  {
    rF2Quat result(*this);
    result *= rhs;
    return result;
  }

  Euler EulerFromQuat() const
  {
    double test = x * y + z * w;
    double yaw, pitch, roll;
    if (test > 0.499)
    { // singularity at north pole
      yaw = 2.0 * atan2(x, w);
      roll = M_PI / 2.0;
      pitch = 0.0;
      return Euler(yaw, roll, pitch);
    }
    else if (test < -0.499)
    { // singularity at south pole
      yaw = -2.0 * atan2(x, w);
      roll = -M_PI / 2.0;
      pitch = 0.0;
      return Euler(yaw, roll, pitch);
    }

    double sqx = x * x;
    double sqy = y * y;
    double sqz = z * z;
    yaw = atan2(2.0 * y * w - 2.0 * x * z, 1.0 - 2.0 * sqy - 2.0 * sqz);
    roll = asin(2.0 * test);
    pitch = atan2(2.0 * x * w - 2.0 * y * z, 1.0 - 2.0 * sqx - 2.0 * sqz);

    return Euler(yaw, roll, pitch);
  }

  static rF2Quat Nlerp(rF2Quat const& a, rF2Quat const& b, double const t)
  {
    double t1 = 1.0 - t;

    rF2Quat r;
    r.x = t1 * a.x + t * b.x;
    r.y = t1 * a.y + t * b.y;
    r.z = t1 * a.z + t * b.z;
    r.w = t1 * a.w + t * b.w;

    r.Normalize();
    return r;
  }

};


struct rF2Vec3
{
  double x, y, z;

  void Set(const double a, const double b, const double c) { x = a; y = b; z = c; }

  // Allowed to reference as [0], [1], or [2], instead of .x, .y, or .z, respectively
  double& operator[](long i) { return((&x)[i]); }
  const double& operator[](long i) const { return((&x)[i]); }

  // MM_NEW
  rF2Vec3()
  {}

  rF2Vec3(TelemVect3 const& other) : x(other.x), y(other.y), z(other.z) 
  {}

  rF2Vec3(double x, double y, double z) : x(x), y(y), z(z)
  {}

  rF2Vec3& operator *=(double const factor)
  {
    x *= factor;
    y *= factor;
    z *= factor;
    return *this;
  }

  rF2Vec3& operator +=(rF2Vec3 const& rhs)
  {
    x += rhs.x;
    y += rhs.y;
    z += rhs.z;
    return *this;
  }

  rF2Vec3 const operator *(double const factor) const
  {
    rF2Vec3 result(*this);
    result *= factor;
    return result;
  }

  rF2Vec3 const operator +(rF2Vec3 const& rhs) const
  {
    rF2Vec3 result(*this);
    result += rhs;
    return result;
  }

  void Rotate(rF2Quat const& r)
  {
    double num12 = r.x + r.x;
    double num2 = r.y + r.y;
    double num = r.z + r.z;
    double num11 = r.w * num12;
    double num10 = r.w * num2;
    double num9 = r.w * num;
    double num8 = r.x * num12;
    double num7 = r.x * num2;
    double num6 = r.x * num;
    double num5 = r.y * num2;
    double num4 = r.y * num;
    double num3 = r.z * num;
    double num15 = ((x * ((1.0 - num5) - num3)) + (y * (num7 - num9))) + (z * (num6 + num10));
    double num14 = ((x * (num7 + num9)) + (y * ((1.0 - num8) - num3))) + (z * (num4 - num11));
    double num13 = ((x * (num6 - num10)) + (y * (num4 + num11))) + (z * ((1.0 - num8) - num5));
    x = num15;
    y = num14;
    z = num13;
  }

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
///////////////////////////////////
// Based on VehicleScoringInfoV01
///////////////////////////////////
struct rF2VehScoringInfo
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
  // MM_NOT_USED
  //rF2Vec3 mLocalVel;             // velocity (meters/sec) in local vehicle coordinates
  //rF2Vec3 mLocalAccel;           // acceleration (meters/sec^2) in local vehicle coordinates

  // Orientation and derivatives
  // MM_NOT_USED
  //rF2Vec3 mOri[3];               // rows of orientation matrix (use TelemQuat conversions if desired), also converts local
                                   // vehicle vectors into world X, Y, or Z using dot product of rows 0, 1, or 2 respectively
  //rF2Vec3 mLocalRot;             // rotation (radians/sec) in local vehicle coordinates
  //rF2Vec3 mLocalRotAccel;        // rotational acceleration (radians/sec^2) in local vehicle coordinates

  // MM_NEW
  double mYaw;                    // rad, use (360-yaw*57.2978)%360 for heading in degrees
  double mPitch;                  // rad
  double mRoll;                   // rad
  double mSpeed;                  // meters/sec

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

#ifdef DEBUG_INTERPOLATION
  rF2Vec3 mPosScoring;
  rF2Vec3 mLocalVel;             // velocity (meters/sec) in local vehicle coordinates
  rF2Vec3 mLocalAccel;           // acceleration (meters/sec^2) in local vehicle coordinates

                                 // Orientation and derivatives
  rF2Vec3 mOri[3];               // rows of orientation matrix (use TelemQuat conversions if desired), also converts local
                                 // vehicle vectors into world X, Y, or Z using dot product of rows 0, 1, or 2 respectively

  rF2Vec3 mLocalRot;             // rotation (radians/sec) in local vehicle coordinates
  rF2Vec3 mLocalRotAccel;        // rotational acceleration (radians/sec^2) in local vehicle coordinates
#endif
};

enum OriMat
{
  RowX = 0,
  RowY,
  RowZ,
  NumRows
};

struct rF2StateHeader
{
  bool mCurrentRead;                 // True indicates buffer is safe to read under mutex.
};

// Our world coordinate system is left-handed, with +y pointing up.
// The local vehicle coordinate system is as follows:
//   +x points out the left side of the car (from the driver's perspective)
//   +y points out the roof
//   +z points out the back of the car
// Rotations are as follows:
//   +x pitches up
//   +y yaws to the right
//   +z rolls to the right
// Note that ISO vehicle coordinates (+x forward, +y right, +z upward) are
// right-handed.  If you are using that system, be sure to negate any rotation
// or torque data because things rotate in the opposite direction.  In other
// words, a -z velocity in rFactor is a +x velocity in ISO, but a -z rotation
// in rFactor is a -x rotation in ISO!!!

struct rF2State
{
  static int const MAX_VSI_SIZE = 128;

  bool mCurrentRead;             // True indicates buffer safe to read under mutex.
  char mVersion[8];              // API version

  ///////////////////////////////////////////////////////
  // TelemInfoV01
  ///////////////////////////////////////////////////////

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

  // MM_NEW
  double mSpeed;                 // meters/sec

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

  // MM_NEW
  double mMaxImpactMagnitude;    // Max impact magnitude.  Tracked on every telemetry call, and reset on visit to pits or Session restart.
  double mAccumulatedImpactMagnitude;  // Accumulated impact magnitude.  Tracked on every telemetry call, and reset on visit to pits or Session restart.

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
  float mVisualSteeringWheelRange;         // the *visual* steering wheel range

  double mRearBrakeBias;                   // fraction of brakes on rear
  double mTurboBoostPressure;              // current turbo boost pressure if available
  float mPhysicsToGraphicsOffset[3];       // offset from static CG to graphical center
  float mPhysicalSteeringWheelRange;       // the *physical* steering wheel range

  // Future use
  unsigned char mExpansionTelem[152]; // for future use (note that the slot ID has been moved to mID above)

                                 // keeping this at the end of the structure to make it easier to replace in future versions
  rF2Wheel mWheels[4];            // wheel info (front left, front right, rear left, rear right)

  ////////////////////////////////////////////////////////////
  // ScoringInfoV01
  ////////////////////////////////////////////////////////////
  // MM_NOT_USED Same as above
  // char mTrackName[64];           // current track name
  long mSession;                 // current session (0=testday 1-4=practice 5-8=qual 9=warmup 10-13=race)
  double mCurrentET;             // current time (at last ScoringUpdate)
  double mEndET;                 // ending time
  long  mMaxLaps;                // maximum laps
  double mLapDist;               // distance around track
  
  // MM_NOT_USED
  //char *mResultsStream;        // results stream additions since last update (newline-delimited and NULL-terminated)

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

  // MM_NEW
  bool mInRealtimeSU;              // in realtime as opposed to at the monitor (reported via ScoringUpdate)
  bool mInRealtimeFC;              // in realtime as opposed to at the monitor (reported via last EnterRealtime/ExitRealtime call)

  char mPlayerName[32];            // player name (including possible multiplayer override)
  char mPlrFileName[64];           // may be encoded to be a legal filename

  // Weather
  double mDarkCloud;               // cloud darkness? 0.0-1.0
  double mRaining;                 // raining severity 0.0-1.0
  double mAmbientTemp;             // temperature (Celsius)
  double mTrackTemp;               // temperature (Celsius)
  rF2Vec3 mWind;                   // wind speed
  double mMinPathWetness;          // minimum wetness on main path 0.0-1.0
  double mMaxPathWetness;          // maximum wetness on main path 0.0-1.0

  // MM_NEW
  unsigned char mInvulnerable;     // Indicates invulnerability 0 (off), 1 (on)

  // Future use
  unsigned char mExpansionScoring[256];
 
  // MM_NOT_USED (Use fixed size array)
  //VehicleScoringInfoV01 *mVehicle; // array of vehicle scoring info's

  rF2VehScoringInfo mVehicles[rF2State::MAX_VSI_SIZE];  // array of vehicle scoring info's
                                                        // NOTE: everything beyound mNumVehicles is trash.
};


#pragma pack(pop)
