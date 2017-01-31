# rFactor 2 Internals Shared Memory Map Plugin

This plugin writes out telemetry and scoring updates into shared memory.  Reading shared memory allows creating  external tools running outside of rFactor 2 and written in languages other than C++ (C# sample is included).

#### This work is based on:
  * rF2 Internals Plugin sample #7 by ISI/S397 found at: https://www.studio-397.com/modding-resources/
  * rF1 Shared Memory Map Plugin by Dan Allongo found at: https://github.com/dallongo/rFactorSharedMemoryMap

## Features
Plugin uses double buffering and offers optional synchronization on global mutex.

Opponent positions in between updates are interpolated using quaternion nlerp.  Plugin has default refresh rate of  ~30FPS.

Plugin is build using VS 2015 Community Edition, targeting VC12 (VS 2013) runtime, since rF2 comes with VC12 redist.

## Monitor
Plugin comes with rF2SMMonitor program that is useful in visualizing of shared memory contents.

## Uses
  * Recommended: Simply copy rF2StateHeader part of the buffer, and check mCurrentRead variable.  If it's true, use this buffer, otherwise use the other buffer.  See `Monitor\rF2SMMonitor\rF2SMMonitor\MainForm.cs MainUpdate` method for example of use in C# (ignore mutex).
  * Synchronized: use mutex to make sure buffer is not overwritten. This use requires full understanding of how plugin works, and could cause FPS drop if not done right.  See `Monitor\rF2SMMonitor\rF2SMMonitor\MainForm.cs MainUpdate` method for example of use in C#
  * Basic: If 15FPS is enough, and you can tolerate partially overwritten buffer once in a while, simply read one buffer and don't bother with double buffering or mutex.

# Release history

01/31/2017 - v1.0.0.0
    Plugin: Added damage and invulnerability tracking
    Monitor: Added phase and damage tracking and logging


1/18/2017 v0.5.0.0 - Initial release
