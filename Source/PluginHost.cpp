#include "PluginHost.h"
#include "rFactor2SharedMemoryMap.hpp"


void PluginHost::Initialize(bool hostStockCarRules)
{
  if (!hostStockCarRules)
    return;  // Single plugin for now, nothing to do if not requested.

  char wd[MAX_PATH] = {};
  GetCurrentDirectory(MAX_PATH, wd);

  auto const scrDllPath = lstrcatA(wd, R"(\Bin64\Plugins\StockCarRules.dll)");

  // Load the SCR plugin if requested.
  mhModuleSCRPlugin = ::LoadLibraryEx(scrDllPath, nullptr /*reserved*/, 0 /*dwFlags*/);
  if (mhModuleSCRPlugin == nullptr) {
    DEBUG_MSG(DebugLevel::Errors, "Failed to load StockCarRules.dll library");
    SharedMemoryPlugin::TraceLastWin32Error();
    return;
  }

  do
  {
#pragma warning(push)
#pragma warning(disable : 4191)  // Allow casts to procedures other than FARPROC.

    auto const pfnGetPluginName = reinterpret_cast<char* (*)()>(::GetProcAddress(mhModuleSCRPlugin, "GetPluginName"));
    if (pfnGetPluginName == nullptr) {
      DEBUG_MSG(DebugLevel::Errors, "Function GetPluginName not found.");
      break;
    }

    auto const pfnGetPluginType = reinterpret_cast<PluginObjectType(*)()>(::GetProcAddress(mhModuleSCRPlugin, "GetPluginType"));
    if (pfnGetPluginType == nullptr) {
      DEBUG_MSG(DebugLevel::Errors, "Function GetPluginType not found.");
      break;
    }

    auto const pfnGetPluginVersion = reinterpret_cast<int(*)()>(::GetProcAddress(mhModuleSCRPlugin, "GetPluginVersion"));
    if (pfnGetPluginVersion == nullptr) {
      DEBUG_MSG(DebugLevel::Errors, "Function GetPluginVersion not found.");
      break;
    }

    auto const pfnCreatePluginObject = reinterpret_cast<PluginObject* (*)()>(::GetProcAddress(mhModuleSCRPlugin, "CreatePluginObject"));
    if (pfnCreatePluginObject == nullptr) {
      DEBUG_MSG(DebugLevel::Errors, "Function CreatePluginObject not found.");
      break;
    }

    auto const pfnDestroyPluginObject = reinterpret_cast<void(*)(PluginObject*)>(::GetProcAddress(mhModuleSCRPlugin, "DestroyPluginObject"));
    if (pfnDestroyPluginObject == nullptr) {
      DEBUG_MSG(DebugLevel::Errors, "Function DestroyPluginObject not found.");
      break;
    }

#pragma warning(pop)  // Allow casts to procedures other than FARPROC.

    auto const pluginType = pfnGetPluginType();
    if (pluginType != PO_INTERNALS) {
      DEBUG_INT2(DebugLevel::Errors, "Unexpected plugin type.", pluginType);
      break;
    }

    auto const pluginVersion = pfnGetPluginVersion();
    if (pluginVersion != 7) {
      DEBUG_INT2(DebugLevel::Errors, "Unexpected plugin version.", pluginVersion);
      break;
    }

    mStockCarRulesPlugin = static_cast<InternalsPluginV07*>(pfnCreatePluginObject());
    if (mStockCarRulesPlugin == nullptr) {
      DEBUG_MSG(DebugLevel::Errors, "Failed to create plugin object.");
      break;
    }

    auto const pluginName = pfnGetPluginName();
    DEBUG_MSG2(DebugLevel::Errors, "Successfully loaded StockCarRules.dll plugin.  Name:", pluginName);

    mInitialized = true;
  } while (false);
  
  if (!mInitialized
    && ::FreeLibrary(mhModuleSCRPlugin) != 1) {
    DEBUG_MSG(DebugLevel::Errors, "Failed to unload StockCarRules.dll library");
    SharedMemoryPlugin::TraceLastWin32Error();
  }

  if (!mInitialized)
    return;

  // Pass the SCR plugin parameters.  For now, just hardcode.
  CustomVariableV01 cvar;
  strcpy(cvar.mCaption, "AllowFrozenAdjustments");
  cvar.mCurrentSetting = 25;
  mStockCarRulesPlugin->AccessCustomVariable(cvar);

  strcpy(cvar.mCaption, "AdjustUntilYellowFlagState");
  cvar.mCurrentSetting = 4;
  mStockCarRulesPlugin->AccessCustomVariable(cvar);

  strcpy(cvar.mCaption, "AllowFrozenAdjustments");
  cvar.mCurrentSetting = 1;
  mStockCarRulesPlugin->AccessCustomVariable(cvar);

  strcpy(cvar.mCaption, "DoubleFileType");
  cvar.mCurrentSetting = 2;
  mStockCarRulesPlugin->AccessCustomVariable(cvar);

  strcpy(cvar.mCaption, "Logging");
  cvar.mCurrentSetting = 1;
  mStockCarRulesPlugin->AccessCustomVariable(cvar);

  strcpy(cvar.mCaption, "LuckyDogFreePass");
  cvar.mCurrentSetting = 1;
  mStockCarRulesPlugin->AccessCustomVariable(cvar);

  strcpy(cvar.mCaption, "WaveArounds");
  cvar.mCurrentSetting = 1;
  mStockCarRulesPlugin->AccessCustomVariable(cvar);

  strcpy(cvar.mCaption, "YellowLapsMinimum");
  cvar.mCurrentSetting = 6;
  mStockCarRulesPlugin->AccessCustomVariable(cvar);

  strcpy(cvar.mCaption, "YellowLapsRandom");
  cvar.mCurrentSetting = 0;
  mStockCarRulesPlugin->AccessCustomVariable(cvar);
}


void PluginHost::Startup(long version)
{
  if (!mInitialized)
    return;

  assert(mStockCarRulesPlugin != nullptr);
  mStockCarRulesPlugin->Startup(version);
}

void PluginHost::Shutdown()
{
  if (!mInitialized)
    return;

  assert(mStockCarRulesPlugin != nullptr);
  mStockCarRulesPlugin->Shutdown();
}

void PluginHost::EnterRealtime()
{
  if (!mInitialized)
    return;

  assert(mStockCarRulesPlugin != nullptr);
  mStockCarRulesPlugin->EnterRealtime();
}

void PluginHost::ExitRealtime()
{
  if (!mInitialized)
    return;

  assert(mStockCarRulesPlugin != nullptr);
  mStockCarRulesPlugin->ExitRealtime();
}

void PluginHost::StartSession()
{
  if (!mInitialized)
    return;

  assert(mStockCarRulesPlugin != nullptr);
  mStockCarRulesPlugin->StartSession();
}

void PluginHost::EndSession()
{
  if (!mInitialized)
    return;

  assert(mStockCarRulesPlugin != nullptr);
  mStockCarRulesPlugin->EndSession();
}

void PluginHost::UpdateTelemetry(TelemInfoV01 const& info)
{
  if (!mInitialized)
    return;

  assert(mStockCarRulesPlugin != nullptr);
  mStockCarRulesPlugin->UpdateTelemetry(info);
}

void PluginHost::UpdateScoring(ScoringInfoV01 const& info)
{
  if (!mInitialized)
    return;

}

bool PluginHost::WantsToDisplayMessage(MessageInfoV01 & msgInfo)
{
  if (!mInitialized)
    return false;

  return false;
}

void PluginHost::ThreadStarted(long type)
{
}

void PluginHost::ThreadStopping(long type)
{
}

bool PluginHost::AccessTrackRules(TrackRulesV01 & info)
{
  if (!mInitialized)
    return false;

  return false;
}

bool PluginHost::WantsPitMenuAccess()
{
  if (!mInitialized)
    return false;

  return false;
}

bool PluginHost::AccessPitMenu(PitMenuV01 & info)
{
  if (!mInitialized)
    return false;

  return false;
}

void PluginHost::SetEnvironment(const EnvironmentInfoV01& info)
{
  if (!mInitialized)
    return;
}
