/*
Implementation of the PluginHost class which currently hosts StockCarRules plugin but could
possibly be generalized in the future.

Class simply forwards rF2 Internals model calls to the plugin loaded using LoadLibraryEx.  Minimal validation
is performed in order to make sure hosted plugin is of V7 type and is currently disabled.

Hosted plugin's custom variables from CustomPluginVariables.json are fowarded.

Author: The Iron Wolf (vleonavicius@hotmail.com)
Website: thecrewchief.org
*/

#include "PluginHost.h"
#include "rFactor2SharedMemoryMap.hpp"
#include "Utils.h"
#include "errno.h"

void PluginHost::Initialize(bool hostStockCarRules)
{
  if (!hostStockCarRules)
    return;  // Single plugin for now, nothing to do if not requested.

  char wd[MAX_PATH] = {};
  ::GetCurrentDirectory(MAX_PATH, wd);

  auto const configFilePath = lstrcatA(wd, R"(\UserData\player\CustomPluginVariables.JSON)");

  auto configFileContents = GetFileContents(configFilePath);
  if (configFileContents == nullptr) {
    DEBUG_MSG(DebugLevel::Errors, "Failed to load CustomPluginVariables.JSON file");
    return;
  }

  auto onExit = MakeScopeGuard([&]() {
    delete[] configFileContents;
  });

  char* pluginConfig = nullptr;
  if (!IsPluginDisabled(configFileContents, "StockCarRules.dll", &pluginConfig) || pluginConfig == nullptr) {
    DEBUG_MSG(DebugLevel::Errors, "Error: StockCarRules.dll plugin is not disabled in CustomPluginVariables.JSON");
    return;
  }

  // Load the SCR plugin if requested.
  ::GetCurrentDirectory(MAX_PATH, wd);

  auto const scrDllPath = lstrcatA(wd, R"(\Bin64\Plugins\StockCarRules.dll)");

  mhModuleSCRPlugin = ::LoadLibraryEx(scrDllPath, nullptr /*reserved*/, 0 /*dwFlags*/);
  if (mhModuleSCRPlugin == nullptr) {
    DEBUG_MSG(DebugLevel::Errors, "Failed to load StockCarRules.dll library");
    SharedMemoryPlugin::TraceLastWin32Error();
    return;
  }

  auto onFailure = MakeScopeGuard(
    [&]() {
    if (!mInitialized
      && ::FreeLibrary(mhModuleSCRPlugin) != 1) {
      DEBUG_MSG(DebugLevel::Errors, "Failed to unload StockCarRules.dll library");
      SharedMemoryPlugin::TraceLastWin32Error();
    }
  });

#pragma warning(push)
#pragma warning(disable : 4191)  // Allow casts to procedures other than FARPROC.

  auto const pfnGetPluginName = reinterpret_cast<char* (*)()>(::GetProcAddress(mhModuleSCRPlugin, "GetPluginName"));
  if (pfnGetPluginName == nullptr) {
    DEBUG_MSG(DebugLevel::Errors, "Function GetPluginName not found.");
    return;
  }

  auto const pfnGetPluginType = reinterpret_cast<PluginObjectType(*)()>(::GetProcAddress(mhModuleSCRPlugin, "GetPluginType"));
  if (pfnGetPluginType == nullptr) {
    DEBUG_MSG(DebugLevel::Errors, "Function GetPluginType not found.");
    return;
  }

  auto const pfnGetPluginVersion = reinterpret_cast<int(*)()>(::GetProcAddress(mhModuleSCRPlugin, "GetPluginVersion"));
  if (pfnGetPluginVersion == nullptr) {
    DEBUG_MSG(DebugLevel::Errors, "Function GetPluginVersion not found.");
    return;
  }

  auto const pfnCreatePluginObject = reinterpret_cast<PluginObject* (*)()>(::GetProcAddress(mhModuleSCRPlugin, "CreatePluginObject"));
  if (pfnCreatePluginObject == nullptr) {
    DEBUG_MSG(DebugLevel::Errors, "Function CreatePluginObject not found.");
    return;
  }

  mpfnDestroyPluginObject = reinterpret_cast<PluginHost::PluginDestoryFunc>(::GetProcAddress(mhModuleSCRPlugin, "DestroyPluginObject"));
  if (mpfnDestroyPluginObject == nullptr) {
    DEBUG_MSG(DebugLevel::Errors, "Function DestroyPluginObject not found.");
    return;
  }

#pragma warning(pop)  // Allow casts to procedures other than FARPROC.

  auto const pluginType = pfnGetPluginType();
  if (pluginType != PO_INTERNALS) {
    DEBUG_INT2(DebugLevel::Errors, "Unexpected plugin type.", pluginType);
    return;
  }

  auto const pluginVersion = pfnGetPluginVersion();
  if (pluginVersion != 7) {
    DEBUG_INT2(DebugLevel::Errors, "Unexpected plugin version.", pluginVersion);
    return;
  }

  mStockCarRulesPlugin = static_cast<InternalsPluginV07*>(pfnCreatePluginObject());
  if (mStockCarRulesPlugin == nullptr) {
    DEBUG_MSG(DebugLevel::Errors, "Failed to create plugin object.");
    return;
  }

  DEBUG_MSG(DebugLevel::CriticalInfo, "Forwarding StockCarRules.dll plugin configuration:");
  if (!ForwardPluginConfig(pluginConfig, *mStockCarRulesPlugin)) {
    DEBUG_MSG(DebugLevel::Errors, "Failed to forward StockCarRules.dll plugin configuration.");
    return;
  }

  auto const pluginName = pfnGetPluginName();
  DEBUG_MSG2(DebugLevel::CriticalInfo, "Successfully loaded StockCarRules.dll plugin.  Name:", pluginName);

  mInitialized = true;
  onFailure.Dismiss();
}


void PluginHost::Cleanup()
{
  if (!mInitialized) {
    assert(mhModuleSCRPlugin == nullptr);
    assert(mStockCarRulesPlugin == nullptr);
    return;
  }

  assert(mhModuleSCRPlugin != nullptr);
  assert(mStockCarRulesPlugin != nullptr);
  assert(mpfnDestroyPluginObject != nullptr);

  if (mpfnDestroyPluginObject != nullptr)
    mpfnDestroyPluginObject(mStockCarRulesPlugin);

  mStockCarRulesPlugin = nullptr;

  if (::FreeLibrary(mhModuleSCRPlugin) != 1) {
    DEBUG_MSG(DebugLevel::Errors, "Failed to unload StockCarRules.dll library");
    SharedMemoryPlugin::TraceLastWin32Error();
  }

  mhModuleSCRPlugin = nullptr;

  mInitialized = false;
  DEBUG_MSG(DebugLevel::CriticalInfo, "Successfully unloaded StockCarRules.dll plugin.");
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
  if (mStockCarRulesPlugin->WantsTelemetryUpdates() == 0)
    return;

  mStockCarRulesPlugin->UpdateTelemetry(info);
}


void PluginHost::UpdateScoring(ScoringInfoV01 const& info)
{
  if (!mInitialized)
    return;

  assert(mStockCarRulesPlugin != nullptr);
  if (!mStockCarRulesPlugin->WantsScoringUpdates())
    return;

  mStockCarRulesPlugin->UpdateScoring(info);
}


bool PluginHost::WantsToDisplayMessage(MessageInfoV01& msgInfo)
{
  if (!mInitialized)
    return false;

  assert(mStockCarRulesPlugin != nullptr);
  return mStockCarRulesPlugin->WantsToDisplayMessage(msgInfo);
}


void PluginHost::ThreadStarted(long type)
{
  if (!mInitialized)
    return;

  assert(mStockCarRulesPlugin != nullptr);
  return mStockCarRulesPlugin->ThreadStarted(type);
}


void PluginHost::ThreadStopping(long type)
{
  if (!mInitialized)
    return;

  assert(mStockCarRulesPlugin != nullptr);
  return mStockCarRulesPlugin->ThreadStopping(type);
}


bool PluginHost::AccessTrackRules(TrackRulesV01& info)
{
  if (!mInitialized)
    return false;

  assert(mStockCarRulesPlugin != nullptr);
  return mStockCarRulesPlugin->AccessTrackRules(info);
}


bool PluginHost::AccessPitMenu(PitMenuV01& info)
{
  if (!mInitialized)
    return false;

  assert(mStockCarRulesPlugin != nullptr);
  if (!mStockCarRulesPlugin->WantsPitMenuAccess())
    return false;

  return mStockCarRulesPlugin->AccessPitMenu(info);
}


void PluginHost::SetPhysicsOptions(PhysicsOptionsV01& options)
{
  if (!mInitialized)
    return;

  assert(mStockCarRulesPlugin != nullptr);
  mStockCarRulesPlugin->SetPhysicsOptions(options);
}


bool PluginHost::AccessMultiSessionRules(MultiSessionRulesV01 & info)
{
  if (!mInitialized)
    return false;

  assert(mStockCarRulesPlugin != nullptr);
  return mStockCarRulesPlugin->AccessMultiSessionRules(info);
}


void PluginHost::SetEnvironment(const EnvironmentInfoV01& info)
{
  if (!mInitialized)
    return;

  assert(mStockCarRulesPlugin != nullptr);
  mStockCarRulesPlugin->SetEnvironment(info);
}


// Horrible stuff to avoid STL.  I don't know if it's worth it anymore.... hold, I just wanted to hoooold
char* PluginHost::GetFileContents(char const* const filePath)
{
  FILE* fileHandle = nullptr;

  auto onExit = MakeScopeGuard(
    [&]() {
    if (fileHandle != nullptr) {
      auto ret = fclose(fileHandle);
      if (ret != 0) {
        DEBUG_INT2(DebugLevel::Errors, "fclose() failed with:", ret);
      }
    }
  });

  char* fileContents = nullptr;
  auto ret = fopen_s(&fileHandle, filePath, "rb");
  if (ret != 0) {
    DEBUG_INT2(DebugLevel::Errors, "fopen_s() failed with:", ret);
    return nullptr;
  }

  ret = fseek(fileHandle, 0, SEEK_END);
  if (ret != 0) {
    DEBUG_INT2(DebugLevel::Errors, "fseek() failed with:", ret);
    return nullptr;
  }

  auto const fileBytes = static_cast<size_t>(ftell(fileHandle));
  rewind(fileHandle);

  fileContents = new char[fileBytes + 1];
  auto elemsRead = fread(fileContents, fileBytes, 1 /*items*/, fileHandle);
  if (elemsRead != 1 /*items*/) {
    delete[] fileContents;
    fileContents = nullptr;
    DEBUG_MSG(DebugLevel::Errors, "fread() failed.");
    return nullptr;
  }

  fileContents[fileBytes] = 0;

  return fileContents;
}


bool PluginHost::IsPluginDisabled(char* const configFileContents, char const* const pluginDllName, char** pluginConfig)
{
  // See if plugin is enabled:
  *pluginConfig = strstr(configFileContents, pluginDllName);
  auto curLine = *pluginConfig;
  while (curLine != nullptr) {
    // Cut off next line from the current text.
    auto const nextLine = strstr(curLine, "\r\n");
    if (nextLine != nullptr)
      *nextLine = '\0';

    auto onExitOrNewIteration = MakeScopeGuard(
      [&]() {
      // Restore the original line.
      if (nextLine != nullptr)
        *nextLine = '\r';
    });

    auto const closingBrace = strchr(curLine, '}');
    if (closingBrace != nullptr) {
      // End of {} for a plugin.
      return false;
    }

    // Check if plugin is disabled.
    auto const enabled = strstr(curLine, " \" Enabled\":0");
    if (enabled != nullptr)
      return true;

    curLine = nextLine != nullptr ? (nextLine + 2 /*skip \r\n*/) : nullptr;
  }

  return false;
}


bool PluginHost::ForwardPluginConfig(char* const pluginConfig, InternalsPluginV07& plugin)
{
  // Parse all the params:
  auto curLine = pluginConfig;
  while (curLine != nullptr) {
    // Cut off next line from the current text.
    auto const nextLine = strstr(curLine, "\r\n");
    if (nextLine != nullptr)
      *nextLine = '\0';

    auto onExitOrNewIteration = MakeScopeGuard(
      [&]() {
      // Restore the original line.
      if (nextLine != nullptr)
        *nextLine = '\r';
    });

    auto const closingBrace = strchr(curLine, '}');
    if (closingBrace != nullptr)
      return true;  // End of {} for a plugin.

    // Find opening "
    auto const openQuote = strchr(curLine, '"');
    if (openQuote == nullptr) {
      DEBUG_MSG2(DebugLevel::Errors, "Failed to locate opening quoute in line:", curLine);
      return false;
    }

    // Get variable name
    int const MAX_VAR_LEN = sizeof(decltype(CustomVariableV01::mCaption));
    char variable[MAX_VAR_LEN];
    auto pch = openQuote + 1;
    auto varIdx = 0;
    auto gotVariable = true;
    while (*pch != '\0' && *pch != '\"') {
      if (*pch == '{') {  // Special case for config opening.
        gotVariable = false;
        break;
      }

      if (varIdx > MAX_VAR_LEN - 2) {
        DEBUG_MSG2(DebugLevel::Errors, "Exceeded max variable length in line:", curLine);
        return false;
      }

      variable[varIdx++] = *pch++;
    }

    auto value = 0L;
    // Get value:
    if (gotVariable && *pch == '\"' && *++pch != '\0' && *pch == ':' && *++pch != '\0') {
      assert(varIdx < MAX_VAR_LEN);
      variable[varIdx] = '\0';

      int const MAX_VALUE_LEN = 7;
      char number[MAX_VALUE_LEN];
      auto valueIdx = 0;
      while (*pch != '\0' && *pch != '\r' && *pch != ',' && isdigit(*pch)) {
        if (valueIdx > MAX_VALUE_LEN - 2) {
          DEBUG_MSG2(DebugLevel::Errors, "Exceeded max number length in line:", curLine);
          return false;
        }

        number[valueIdx++] = *pch++;
      }

      assert(valueIdx < MAX_VALUE_LEN);
      number[valueIdx] = '\0';

      value = atol(number);
      if (value == 0
        && (errno == EINVAL || errno == ERANGE)) {
        DEBUG_MSG2(DebugLevel::Errors, "Failed to convert line value part to number:", curLine);
        return false;
      }
    }

    if (gotVariable) {
      if (strcmp(variable, " Enabled") == 0)
        value = 1;  // Fake enable just in case.
      else if (strcmp(variable, "DoubleFileType") == 0)
        mStockCarRulesPlugin_DoubleFileType = value;  // Capture DoubleFileType;

      CustomVariableV01 cvar;
      strcpy(cvar.mCaption, variable);
      cvar.mCurrentSetting = value;
      plugin.AccessCustomVariable(cvar);
      DEBUG_INT2(DebugLevel::CriticalInfo, variable, value);
    }

    curLine = nextLine != nullptr ? (nextLine + 2 /*skip \r\n*/) : nullptr;
  }

  return false;
}
