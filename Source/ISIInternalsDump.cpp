#include "rFactor2SharedMemoryMap.hpp"


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
    fprintf(fo, "Lap=%ld StartET=%.3f\n", info.mLapNumber, info.mLapStartET);
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
    fprintf(fo, "Gear=%ld RPM=%.1f RevLimit=%.1f\n", info.mGear, info.mEngineRPM, info.mEngineMaxRPM);
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
    fprintf(fo, "Session=%ld NumVehicles=%ld CurET=%.3f\n", info.mSession, info.mNumVehicles, info.mCurrentET);
    fprintf(fo, "EndET=%.3f MaxLaps=%ld LapDist=%.1f\n", info.mEndET, info.mMaxLaps, info.mLapDist);

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
      fprintf(fo, "Driver %ld: %s\n", i, vinfo.mDriverName);
      fprintf(fo, " ID=%ld Vehicle=%s\n", vinfo.mID, vinfo.mVehicleName);
      fprintf(fo, " Laps=%d Sector=%d FinishStatus=%d\n", vinfo.mTotalLaps, vinfo.mSector, vinfo.mFinishStatus);
      fprintf(fo, " LapDist=%.1f PathLat=%.2f RelevantTrackEdge=%.2f\n", vinfo.mLapDist, vinfo.mPathLateral, vinfo.mTrackEdge);
      fprintf(fo, " Best=(%.3f, %.3f, %.3f)\n", vinfo.mBestSector1, vinfo.mBestSector2, vinfo.mBestLapTime);
      fprintf(fo, " Last=(%.3f, %.3f, %.3f)\n", vinfo.mLastSector1, vinfo.mLastSector2, vinfo.mLastLapTime);
      fprintf(fo, " Current Sector 1 = %.3f, Current Sector 2 = %.3f\n", vinfo.mCurSector1, vinfo.mCurSector2);
      fprintf(fo, " Pitstops=%d, Penalties=%d\n", vinfo.mNumPitstops, vinfo.mNumPenalties);

      // New version 2 stuff
      fprintf(fo, " IsPlayer=%d Control=%d InPits=%d LapStartET=%.3f\n", vinfo.mIsPlayer, vinfo.mControl, vinfo.mInPits, vinfo.mLapStartET);
      fprintf(fo, " Place=%d VehicleClass=%s\n", vinfo.mPlace, vinfo.mVehicleClass);
      fprintf(fo, " TimeBehindNext=%.3f LapsBehindNext=%ld\n", vinfo.mTimeBehindNext, vinfo.mLapsBehindNext);
      fprintf(fo, " TimeBehindLeader=%.3f LapsBehindLeader=%ld\n", vinfo.mTimeBehindLeader, vinfo.mLapsBehindLeader);
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

void SharedMemoryPlugin::WritePitMenuInternals(PitMenuV01 const& info)
{
  if (!SharedMemoryPlugin::msDebugISIInternals)
    return;

  if (SharedMemoryPlugin::msIsiPitMenuFile == nullptr) {
    SharedMemoryPlugin::msIsiPitMenuFile = _fsopen(SharedMemoryPlugin::INTERNALS_PITMENU_FILENAME, "a", _SH_DENYNO);
    setvbuf(SharedMemoryPlugin::msIsiPitMenuFile, nullptr, _IOFBF, SharedMemoryPlugin::BUFFER_IO_BYTES);
  }

  auto fo = SharedMemoryPlugin::msIsiPitMenuFile;
  if (fo != nullptr)
  {
    fprintf(fo, "mCategoryIndex=%ld  ", info.mCategoryIndex);
    fprintf(fo, "mCategoryName='%s'\n", info.mCategoryName);
    fprintf(fo, "mChoiceIndex=%ld  ", info.mChoiceIndex);
    fprintf(fo, "mChoiceString='%s'\n", info.mChoiceString);
    // Close file
    DEBUG_MSG(DebugLevel::DevInfo, "WritePitMenuInternals");
    fclose(fo);
  }

}
