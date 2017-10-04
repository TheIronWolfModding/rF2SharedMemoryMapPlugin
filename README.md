# rFactor 2 Internals Shared Memory Map Plugin

This plugin mirrors exposed rFactor 2 internal state into shared memory buffers.  Essentially, this is direct memcpy of rFactor 2 internals.

Reading shared memory allows creating  external tools running outside of rFactor 2 and written in languages other than C++ (C# sample is included).  It also allows keeping CPU time impact in rFactor 2 process to the minimum.

#### This work is based on:
  * rF2 Internals Plugin sample #7 by ISI/S397 found at: https://www.studio-397.com/modding-resources/
#### Was inspired by:
  * rF1 Shared Memory Map Plugin by Dan Allongo found at: https://github.com/dallongo/rFactorSharedMemoryMap

## Features
Plugin uses double buffering and offers optional weak synchronization on global mutexes.

Plugin is built using VS 2015 Community Edition, targeting VC12 (VS 2013) runtime, since rF2 comes with VC12 redist.

## Refresh Rates:
* Telemetry - 50FPS (provided there's no mutex contention).
* Scoring - 5FPS.
* Rules - 3FPS.
* Multi Rules - on callback from a game, usually once a session and in between sessions.
* Extended - 5FPS and on tracked callback by the game.

## Limitations/Assumptions:
* Negative mID is not supported.
* Distance between max(mID) and min(mID) in a session cannot exceed 512.
* Max mapped vehicles: 128.
* Plugin assumes that delta Elapsed Time in a telemetry update frame cannot exceed 2ms (which effectively limits telemetry refresh rate to 50FPS).

## Monitor
Plugin comes with rF2SMMonitor program that shows how to access exposed internals from C# program.  It is also useful for visualization of shared memory contents and general understanding of rFactor 2 internals.

## Memory Buffer Uses
  * Recommended: Simply copy rF2StateHeader part of the buffer, and check mCurrentRead variable.  If it's true, use this buffer, otherwise use the other buffer.  See `Monitor\rF2SMMonitor\rF2SMMonitor\MainForm.cs MainUpdate` and `MappedDoubleBuffer.GetMappedData/MappedDoubleBuffer.GetMappedDataPartial` methods for example of use in C# (ignore mutex).
  * Synchronized: use mutex to make sure buffer is not overwritten (this is best effort activity, not a guarantee.  See comnents in C++ code for exact details). Generally, _do not use this method if you are visualizing rF2 internals_ and not doing any analysis that requires buffer to be complete.  Example: Crew Chief will not be happy if there are two copies of the vehicles in the buffer, but it does not matter in most other cases.  This use requires full understanding of how plugin works, and could cause FPS drop if not done right.  See `Monitor\rF2SMMonitor\rF2SMMonitor\MainForm.cs MainUpdate` and `MappedDoubleBuffer.GetMappedData/MappedDoubleBuffer.GetMappedDataPartial` methods for example of use in C#
  * Basic: If half refresh rate is enough, and you can tolerate partially overwritten buffer once in a while, simply read one buffer and don't bother with double buffering or mutex.

## Support this project
If you would like to support this project, you can donate [here.](http://thecrewchief.org/misc.php?do=donate)

# Release history

10/04/2017 - v2.2.1.0

  Plugin:
  * Expose rF2MultiRules buffer
  * Expose rF2TrackRulesAction on rF2Rules
  * Add rF2SessionTransitionCapture to rF2Extended.  This allows tracking some state on session transition.
  * Add mSessionStarted, mTicksSessionStarted and mTicksSessionStarted to rF2Extended to help tracking session transitions.
  * Buffers are no longer cleared out on EndSession.  This is neccessary to capture data in StartSession.

  Monitor:
  * Add Frozen Order rules detection.

9/01/2017 - v2.1.1.1

  Plugin:
  * Fix crash in physics update that happens when buffer can't be mapped.
  * Expose TrackRules01 as rF2Rules buffer.
  * Improve Win32 error tracing.
  * Minor reliability improvements.

  Monitor:
  * Add rF2Rules tracking.
  * Force invariant culture on the app.

7/22/2017 - v2.0.0.0

  Complete redesign/rework to better reflect rFactor 2 internals read model.  Interpolation is removed, plugin is now essentially double buffered memcpy of rF2 internals.

3/22/2017 - v1.1.0.1

  Plugin:
  * Replaced rF2State::mInRealTime with mInRealTimeFC and mInRealTimeSU values, to distiguish between InRealtime state reported via ScoringUpdate, and via Enter/ExitRealtime calls.
  * rF2State::mCurrentET is no longer updated between Scoring Updates, and matches value last reported by the game.

  Monitor:
  * Extended monitor to dislay more information
  * Implemented correct "Best Split" time calculation logic.

2/26/2017 - v1.0.0.1

  Fixed synchronization of:
  * rF2State::mElapsedTime
  * rF2State::mLapStartET
  * rF2State::mLapNumber

  This eliminates the gap those values had between telemetry and scoring updates.

01/31/2017 - v1.0.0.0
  * Plugin: Added damage and invulnerability tracking
  * Monitor: Added phase and damage tracking and logging


1/18/2017 v0.5.0.0 - Initial release