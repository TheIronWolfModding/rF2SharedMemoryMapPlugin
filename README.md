# rFactor 2 Internals Shared Memory Map Plugin

This plugin mirrors exposed rFactor 2 internal state into shared memory buffers.  Essentially, this is direct memcpy of rFactor 2 internals.

Reading shared memory allows creating external tools running outside of rFactor 2 and written in languages other than C++ (C# sample is included).  It also allows keeping CPU time impact in rFactor 2 plugin threads to the minimum.

This plugin is carefully implemented with an intent of becoming a shared component for reading rF2 internals.  It can handle any number of clients without slowing down rF2 plugin thread.  A lot of work was done to ensure it is as efficient and reliable as possible.

#### This work is based on:
  * rF2 Internals Plugin sample #7 by ISI/S397 found at: https://www.studio-397.com/modding-resources/
#### Was inspired by:
  * rF1 Shared Memory Map Plugin by Dan Allongo found at: https://github.com/dallongo/rFactorSharedMemoryMap

## Features
Plugin offers optional weak synchronization by using version variables on each of the buffers.  Plugin is also capable of hosting StockCarRules.dll plugin as a hack/workaround for accessing SCR set values.

Plugin is built using VS 2015 Community Edition, targeting VC12 (VS 2013) runtime, since rF2 comes with VC12 redist.

## Refresh Rates:
* Telemetry - 50FPS.
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
  * Basic:   Most clients (HUDs, Dashes, visualizers) won't need synchronization, see `rF2SMMonitor.MainForm.MappedBuffer<>.GetMappedDataUnsynchronized` for sample implementation.
  * Advanced:  If you would like to make sure you're not 
  reading a torn (partially overwritten) frame, see `rF2SMMonitor.MainForm.MappedBuffer<>.GetMappedData` for sample implementation.

## Distribution and reuse
You are allowed to include this .dll with your distribution, as long as it is:
* Free
* Readme is included
* You had my permission via email

Please also be aware, that Crew Chief will always ship with the latest version of the .dll and will overwrite .dll to match its version.  I do not expect compatibility to break without game changing its model, aside from rF2Extended buffer, which contains stuff not directly exposed by the game.  Every time layout of memory changes, second digit in Plugin version is incremented.

## Support this project
If you would like to support this project, you can donate [here.](http://thecrewchief.org/misc.php?do=donate)

# Release history

02/16/2018 - v3.0.0.0

  Rework Plugin to replace mutexes with buffer version tracking mechanism.  Update Monitor to match new way of reading data.

01/24/2018 - v2.4.0.0

  Plugin:
  * Expose Stock Car Rules plugin DoubleFileType setting.
  * Rearrange rF2Extended to better expose hosted plugin variables.

  Monitor:
  * Improve Rolling Start detection when no SC present.
  * Correctly calculate distance to the SC.
  * Add Frozen Order sub-phases for Rolling start without SC, while assigned pole/pole row.

01/02/2018 - v2.3.1.2

  Plugin:
  * Allow hosting of StockCarRules.dll plugin.  Capture mMessage members filled out by SCR plugin and pass them out via SM.
  * Remove .ini file config and move to standard rF2 plugin config via CustomPluginVariables.json.

  Monitor:
  * Minor fixes and changes to support SCR plugin hosting feature.

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
  * Extended monitor to display more information
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