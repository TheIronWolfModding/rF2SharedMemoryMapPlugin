# rFactor 2 Internals Shared Memory Plugin

This plugin mirrors exposed rFactor 2 internal state into shared memory buffers.  Essentially, this is direct memcpy of rFactor 2 internals (except for extended stuff implemented to workaround API limitations/bugs).  Plugin also allows some level of input back into the rFactor 2 API.

Reading and writing shared memory allows creating external tools running outside of rFactor 2 and written in languages other than C++ (C# sample is included).  It also allows keeping CPU time impact in rFactor 2 plugin threads to the minimum.

This plugin is carefully implemented with an intent of becoming a shared component for reading rF2 internals.  For read operations, it can handle any number of clients without slowing down rF2 plugin thread.  A lot of work was done to ensure it is as efficient and reliable as possible.

rFactor 2 API has some limitations/bugs, and plugin tries to deal with that by deriving some information: basic accumulated damage info, tracking session transitions and optionally reading game memory directly.

# Acknowledgements
##### This work is based on:
  * rF2 Internals Plugin sample #8 by ISI/S397 found at: https://www.studio-397.com/modding-resources/
##### Was inspired by:
  * rF1 Shared Memory Map Plugin by Dan Allongo found at: https://github.com/dallongo/rFactorSharedMemoryMap

### Authors: 
Vytautas Leonavičius
##### With contributions by:
- Morten Roslev: parts of DMR implementation and teaching me various memory reading techniques and helping me to stay sane
- Tony Whitley: pit info/HWControl prototyping/bug fixes/proofreading

## Download:
https://www.mediafire.com/file/s6ojcr9zrs6q9ls/rf2_sm_tools_3.7.15.1.zip/file

## Features
Plugin offers optional weak synchronization by using version variables on each of the output buffers.

Plugin is built using VS Community Edition, targeting VC12 (VS 2013) runtime, since rF2 comes with VC12 redist.

## Output Refresh Rates:
* Telemetry - 50FPS.
* Scoring - 5FPS.
* Rules - 3FPS.
* Multi Rules - on callback from a game, usually once a session and in between sessions.
* ForceFeedback - 400FPS.
* Graphics - 400FPS.
* Pit Info - 100FPS.
* Weather - 1FPS.
* Extended - 5FPS and on tracked callback by the game.

Note: `Graphics` and `Weather` are unsbscribed from by default.

## Input Buffers
Note to cheaters who dare to contact me with questions: none of this can be used to control vehicle.

Plugin supports sending input to the game (using rFactor 2 API).  Please use this stuff with extreme care - it can cause game freezes and unexpected behavior.  When working with the input buffers, make sure to set `DebugOutputLevel` to `15` and `DebugOutputSource` to `32767` and review `UserData\Log\RF2SMMP_DebugOutput.txt` for errors and warnings.

Monitor app includes sample code that uses Input buffers.

Note: designed with only one write client in mind.  Although multiple clients should be able to coexist fine, they will need to be mindful of buffer update clashes, plugin does not moderate that part.

### HWControl input
Allows sending restricted set of inputs to rF2.  Mostly useful for pit menu interaction.

### Weather Control input
Allows sending weather input.  This might be useful in keeping internet queries/thread synchronization out of rF2 plugin thread and inside of a standalone weather control app.

### Rules Control input
This is experimental control buffer.  It allows sending Rules input to the game.  The idea was that it might make developing custom rules plugin easier, by being able to change logic in an external app/script without rebuilding plugin/restarting rF2.  

However, this is experimental, because I am not positive this approach is going to "fly"/is reliable.  The reason for that is that when done inside of a plugin, rules are applied synchronously: game sends rules input, and allows updating that input in the same callback, meaning synchronously.  Using shared memory plugin this process is not synchronous: game sends rules in a function call, plugin picks up rules update from the input buffer in a separate function call, and will apply update next time game calls rule update function.  So there's a gap between game ouput rules and requested input rules.

I tried to minimize the impact of this processing gap by copying some of the latest rules state onto the updated state, but I do not know how reliably would that work.  That said, I am very passionate about autosport rules, so if you try to use this plugin for rules development and need help/changes (with the plugin part), feel free to reach out.

### Plugin Control input
Allows dynamically subscirbing to buffers that might've been unsubscribed by `CustomPluginVariables.json` configuration.  Also, allows enabling control input buffers.  The idea here is to allow client to turn missing functionality on if it is missing due to misconfiguration.

## Input Refresh Rates:
* HWControl - Read at 5FPS with 100ms boost to 50FPS once update is received.  Applied at 100FPS.
* WeatherControl - Read at 5FPS.  Applied at 1FPS.
* RulesControl - Read at 5FPS.  Applied at 3FPS.
* PluginControl - Read at 5FPS.  Applied on read.

Note: only `PluginControl` and `HWControl` buffers are enabled by default.  Other buffers can be enabled via `CustomPluginVariables.json` settings.

## Unsubscribing from the buffer updatdes
It is possible to configure which buffers get updated and which don't.  This is done via `UnsubscribedBuffersMask` value in the `CustomPluginVariables.json` file.  To specify buffers to unsubscribe from, add desired flag values up.

`Telemetry = 1,
Scoring = 2,
Rules = 4,
MultiRules = 8,
ForceFeedback = 16,
Graphics = 32,
PitInfo = 64,
Weather = 128
All = 255`

So, to unsubscribe from `Multi Rules` and `Graphics` buffers set `UnsubscribedBuffersMask` to 40 (8 + 32).

- Note: unsubscribing from `Extended` buffer updates is not supported.
- Note: usubscribing from `Scoring` will disable `Plugin Control` input.

## Limitations/Assumptions:
* Negative mID is not supported.
* Distance between max(mID) and min(mID) in a session cannot exceed 512.
* Max mapped vehicles: 128.
* Plugin assumes that delta Elapsed Time in a telemetry update frame cannot exceed 2ms (which effectively limits telemetry refresh rate to 50FPS).

## Monitor
Plugin comes with rF2SMMonitor program that shows how to access exposed internals from C# program.  It is also useful for visualization of shared memory contents and general understanding of rFactor 2 internals.

## Memory Buffer Uses
  * Basic:   Most clients (HUDs, Dashes, visualizers) won't need synchronization, see `rF2SMMonitor.MappedBuffer<>.GetMappedDataUnsynchronized` for sample implementation.
  * Advanced:  If you would like to make sure you're not 
  reading a torn (partially overwritten) frame, see `rF2SMMonitor.MappedBuffer<>.GetMappedData` for sample implementation.

## Dedicated server use
If ran in dedicated server process, each shared memory buffer name has server PID appended.  If DedicatedServerMapGlobally preference is set to 1, plugin will attempt creating shared memory buffers in the Global section.  Note that "Create Global Objects" permission is needed on user account running dedicated server.

## Distribution and reuse
You are allowed to include this .dll with your distribution, as long as it is:
* Free
* Readme is included
* You had my permission via email

Please also be aware, that Crew Chief will always ship with the latest version of the .dll and will overwrite .dll to match its version.  I do not expect compatibility to break without game changing its model, aside from `rF2Extended` buffer, which contains stuff not directly exposed by the game.  Every time layout of memory changes, either of the first two digits in the Plugin version is incremented, which means clients might need an update.  Monitor app is kept in sync with plugin.

## Current known clients
* Crew Chief: https://github.com/mrbelowski/CrewChiefV4 
* SimHub: https://github.com/zegreatclan/AssettoCorsaTools/wiki
* rFactor 2 Log Analyzer: https://forum.studio-397.com/index.php?threads/rfactor2-log-analyzer-ver-2-with-offline-and-league-championship-manager.48117/
* Sim Racing Studio tools: https://www.simracingstudio.com/download
* Second Monitor: https://github.com/Winzarten/SecondMonitor
* Realistic Gearshift: https://forum.studio-397.com/index.php?threads/realistic-gearshift-version-2.62996/

## Support this project
If you would like to support this project, you can donate [here.](http://thecrewchief.org/misc.php?do=donate)

# Release history

**25/03/2023 - v3.7.15.1**

This version corrects the updated InternalsPlugin.hpp

  Plugin:
  * Inserts DeltaBest into the `rF2Telemetry` buffer.

**09/03/2023 - v3.7.15.0**

This version just uses the updated InternalsPlugin.hpp

  Plugin:
  * Adds battery/electric motor items to the `rF2Telemetry` buffer.

**09/07/2020 - v3.7.14.2**

This version introduces input buffers.  See "Input Buffers" readme section for more info.

  Plugin:
  * Expose `rF2PitInfo` and `rF2Weather` buffers.  Special thanks for prototyping `AccessPitMenu` and `CheckHWControl` plugin functionality go to Tony Whitley.
  * Expose `rF2HWControl` input buffer, which allows sending limited number of inputs into the game.
  * Expose `rF2WeatherControl` input buffer, which allows changing weather conditions dynamically.
  * Expose `rF2PluginControl` input buffer.  Allows requesting additional plugin features dynamically.
  * Experimental: Expose `rF2RulesControl` input buffer, which allows sending rules input into the game.
  * Intenral: rework tracing and reduce code duplication.
  
Monitor:
* Updated to demo newly added features.

**11/10/2019 - v3.7.1.0**

  Plugin:
  * Expose `UsubscribedBuffersMask` via `rF2Extended::mUnsubscribedBuffersMask`.  This can be used by clients to validate UBM, and might be made client writable in the future.

**11/08/2019 - v3.7.0.0**

Plugin:

* Expose rF2Graphics (GraphicsInfoV02) buffer via $rFactor2SMMP_Graphics$.  Note that it is not subsribed to by default.
* It is now possible to configure which buffers get updated and which don't.  This is done via `UnsubscribedBuffersMask` value in the `CustomPluginVariables.json` file.  To specify buffers to unsubscribe from, add desired flag values up.

`Telemetry = 1,
Scoring = 2,
Rules = 4,
MultiRules = 8,
ForceFeedback = 16,
Graphics = 32`

So, to unsubscribe from `Multi Rules` and `Graphics` buffers set `UnsubscribedBuffersMask` to 40 (8 + 32).

**05/01/2019 - v3.6.0.0**

  Plugin:
  * Fix version string not behaving as expected (breaking rF2Extended change, Sorry!)
  * Reduce amount of string copies in DMA mode.

**03/29/2019 - v3.5.0.9**

  Plugin:
  * Expose LSI messages in DMA mode.
  * Clear out accumulated damage on return to Monitor.
  * Harden DMA mode against crashes.

  Monitor:
  * Implement Frozen Order detection based on LSI messages.  That's the only way to handle FO in the online sessions.

**02/08/2019 - v3.4.0.6**

  Plugin/Monitor:
  * Expose Stock Car Rules plugin FCY instruction message in DMA mode.
  * Deprecate SCR plugin hosting (breaking change).

**01/28/2019 - v3.3.0.6**

  Plugin/Monitor:
  * Expose Message Center messages, global status message and pit lane speed values via rF2Extended buffer.  This functionality if off by default and is enabled via "EnableDirectMemoryAccess" plugin variable.

**12/15/2018 - v3.2.0.0**

  Plugin/Monitor:
  * Update exposed field headers and C# marhsaling code to include fields found in Rules/Knockout Qualifying plugins.  See [this](https://github.com/TheIronWolfModding/rF2SharedMemoryMapPlugin/commit/eb5de36e7cf0ccce4335d93bc4c34471841ddecf?w=1) commit to see what has changed.

**09/04/2018 - v3.1.0.0**

  Plugin:
  * Exposed FFB information.  Add $rFactor2SMMP_ForceFeedback$ buffer to map rF2ForceFeedback structure.  Note that since it is a single value buffer, and it is updated at 400FPS, no synchronization is applied nor needed while reading it.

  Monitor:
  * Updated to expose FFB info.

**03/29/2018 - v3.0.1.0**

  Plugin:
  * Add DedicatedServerMapGlobally preference to allow mapping dedicated server shared memory buffers to global section, so that users running under other accounts can access them.  Note that "Create Global Objects" permission is needed on user account running dedicated server.

**02/21/2018 - v3.0.0.1**

  Hotfix for physics options not being captured.

**02/16/2018 - v3.0.0.0**

  Rework Plugin to replace mutexes with buffer version tracking mechanism.  Update Monitor to match new way of reading data.

**01/24/2018 - v2.4.0.0**

  Plugin:
  * Expose Stock Car Rules plugin DoubleFileType setting.
  * Rearrange rF2Extended to better expose hosted plugin variables.

  Monitor:
  * Improve Rolling Start detection when no SC present.
  * Correctly calculate distance to the SC.
  * Add Frozen Order sub-phases for Rolling start without SC, while assigned pole/pole row.

**01/02/2018 - v2.3.1.2**

  Plugin:
  * Allow hosting of StockCarRules.dll plugin.  Capture mMessage members filled out by SCR plugin and pass them out via SM.
  * Remove .ini file config and move to standard rF2 plugin config via CustomPluginVariables.json.

  Monitor:
  * Minor fixes and changes to support SCR plugin hosting feature.

**10/04/2017 - v2.2.1.0**

  Plugin:
  * Expose rF2MultiRules buffer
  * Expose rF2TrackRulesAction on rF2Rules
  * Add rF2SessionTransitionCapture to rF2Extended.  This allows tracking some state on session transition.
  * Add mSessionStarted, mTicksSessionStarted and mTicksSessionStarted to rF2Extended to help tracking session transitions.
  * Buffers are no longer cleared out on EndSession.  This is neccessary to capture data in StartSession.

  Monitor:
  * Add Frozen Order rules detection.

**9/01/2017 - v2.1.1.1**

  Plugin:
  * Fix crash in physics update that happens when buffer can't be mapped.
  * Expose TrackRules01 as rF2Rules buffer.
  * Improve Win32 error tracing.
  * Minor reliability improvements.

  Monitor:
  * Add rF2Rules tracking.
  * Force invariant culture on the app.

**7/22/2017 - v2.0.0.0**

  Complete redesign/rework to better reflect rFactor 2 internals read model.  Interpolation is removed, plugin is now essentially double buffered memcpy of rF2 internals.

**3/22/2017 - v1.1.0.1**

  Plugin:
  * Replaced rF2State::mInRealTime with mInRealTimeFC and mInRealTimeSU values, to distiguish between InRealtime state reported via ScoringUpdate, and via Enter/ExitRealtime calls.
  * rF2State::mCurrentET is no longer updated between Scoring Updates, and matches value last reported by the game.

  Monitor:
  * Extended monitor to display more information
  * Implemented correct "Best Split" time calculation logic.

**2/26/2017 - v1.0.0.1**

  Fixed synchronization of:
  * rF2State::mElapsedTime
  * rF2State::mLapStartET
  * rF2State::mLapNumber

  This eliminates the gap those values had between telemetry and scoring updates.

**01/31/2017 - v1.0.0.0**
  * Plugin: Added damage and invulnerability tracking
  * Monitor: Added phase and damage tracking and logging

**1/18/2017 v0.5.0.0 - Initial release**