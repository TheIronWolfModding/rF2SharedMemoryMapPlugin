/*
Definition of the PluginHost class which currently hosts StockCarRules plugin but could
possibly be generalized in the future.

Author: The Iron Wolf (vleonavicius@hotmail.com)
Website: thecrewchief.org
*/

#pragma once

#pragma warning(push)
#pragma warning(disable : 4263)   // UpdateGraphics virtual incorrect signature
#pragma warning(disable : 4264)   // UpdateGraphics virtual incorrect signature
#pragma warning(disable : 4121)   // Alignment sensitivity (ISI sets 4 byte pack)
#pragma warning(disable : 4100)   // Unreferenced params
#include "InternalsPlugin.hpp"
#pragma warning(pop)

class PluginHost : public InternalsPluginV07
{
public:
  PluginHost() {}
  ~PluginHost() { Cleanup(); }

  void Initialize(bool hostStockCarRules);
  void Cleanup();
  bool IsStockCarRulesPluginHosted() const { return mInitialized; }

  ////////////////////////////////////////////////////
  // InternalsPluginV01 (InternalsPlugin)
  ///////////////////////////////////////////////////
  void Startup(long version) override;    // game startup
  void Shutdown() override;               // game shutdown

  void EnterRealtime() override;          // entering realtime (where the vehicle can be driven)
  void ExitRealtime() override;           // exiting realtime

  void StartSession() override;           // session has started
  void EndSession() override;             // session has ended

  // GAME OUTPUT
  void UpdateTelemetry(TelemInfoV01 const& info) override;

  // SCORING OUTPUT
  void UpdateScoring(ScoringInfoV01 const& info) override; // update plugin with scoring info (approximately five times per second)

  // MESSAGE BOX INPUT
  bool WantsToDisplayMessage(MessageInfoV01& msgInfo) override; // set message and return true

  // ADDITIONAL GAMEFLOW NOTIFICATIONS
  void ThreadStarted(long type) override; // called just after a primary thread is started (type is 0=multimedia or 1=simulation)
  void ThreadStopping(long type) override;  // called just before a primary thread is stopped (type is 0=multimedia or 1=simulation)

  bool AccessTrackRules(TrackRulesV01& info) override; // current track order passed in; return true if you want to change it (note: this will be called immediately after UpdateScoring() when appropriate)

  // PIT MENU INFO (currently, the only way to edit the pit menu is to use this in conjunction with CheckHWControl())
  bool AccessPitMenu(PitMenuV01& info) override; // currently, the return code should always be false (because we may allow more direct editing in the future)

  void SetPhysicsOptions(PhysicsOptionsV01& options) override;

  // SCORING CONTROL (only available in single-player or on multiplayer server)
  bool AccessMultiSessionRules(MultiSessionRulesV01& info); // current internal rules passed in; return true if you want to change them

  void SetEnvironment(const EnvironmentInfoV01 &info) override; // may be called whenever the environment changes

private:
  char* GetFileContents(char const* const filePath);
  bool IsPluginDisabled(char* const configFileContents, char const* const pluginDllName, char** pluginConfig);
  bool ForwardPluginConfig(char* const pluginConfig, InternalsPluginV07& plugin);

private:
  bool mInitialized = false;

  HMODULE mhModuleSCRPlugin = nullptr;
  InternalsPluginV07* mStockCarRulesPlugin = nullptr;

  using PluginDestoryFunc = void (*)(PluginObject*);
  PluginDestoryFunc mpfnDestroyPluginObject = nullptr;
};