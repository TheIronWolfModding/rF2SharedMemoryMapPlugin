#include "pch.h"
#include "CppUnitTest.h"
#include <direct.h>
#include "rFactor2SharedMemoryMap.hpp"
#include "MappedBuffer.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

#define SMP_VERSION 3720
#define TEST_NAME_IN_DEBUG(name) \
    DEBUG_MSG(DebugLevel::DevInfo, "**** FROM LIVE UNIT TEST " ## name ## " ****")


namespace TestLiveStartup // Just test the startup code
{
  TEST_CLASS(UnitTest)
  {
  public:

    TEST_METHOD(init)
    {
      // Run once when the module is loaded.

      // Create Userdata\Log folder for debug files
      // found in VC12\x64\UnitTest\Userdata\Log\RF2SMMP_DebugOutput.txt
      _mkdir("Userdata");
      _mkdir("Userdata\\Log");

    }
    TEST_METHOD(Test_Startup)
    {
    }
  };
}

namespace TestLiveHWControl //
{

  TEST_MODULE_INITIALIZE(init)
  {
    // Run once when the module is loaded.

    // Create Userdata\Log folder for debug files
    // found in VC12\x64\Debug\Userdata\Log\RF2SMMP_DebugOutput.txt
    _mkdir("Userdata");
    _mkdir("Userdata\\Log");

    SharedMemoryPlugin::msDebugISIInternals = true;

    TEST_NAME_IN_DEBUG("INITIALIZE");
  }

  TEST_MODULE_CLEANUP(methodName)
  {
    // Run once when the module is unloaded.
    // Could delete Userdata\Log folder but then wouldn't be able to read them!
  }

  /////////////////////////////////////////////////////////////////////////////
  //
  // Tests

  TEST_CLASS(ClientHWControl)
  {
  public:

    TEST_METHOD(Connect)
    { //
      MappedBuffer<rF2HWControl> clientHWControl(SharedMemoryPlugin::MM_HWCONTROL_FILE_NAME);
      rF2HWControl info;
      TEST_NAME_IN_DEBUG("Connect");

      if (!clientHWControl.Initialize(SharedMemoryPlugin::msDedicatedServerMapGlobally)) {
        DEBUG_MSG(DebugLevel::Errors, "Failed to initialize client HWControl mapping");
        return;
      }
      clientHWControl.ReleaseResources();
    }

    TEST_METHOD(ConnectAndWrite)
    { //
      MappedBuffer<rF2HWControl> clientHWControl(SharedMemoryPlugin::MM_HWCONTROL_FILE_NAME);
      rF2HWControl info;
      double fRetVal;
      TEST_NAME_IN_DEBUG("ConnectAndWrite");

      if (!clientHWControl.Initialize(SharedMemoryPlugin::msDedicatedServerMapGlobally)) {
        DEBUG_MSG(DebugLevel::Errors, "Failed to initialize client HWControl mapping");
        return;
      }

      strcpy(info.mControlName, "ToggleMFDB");
      info.mfRetVal = 1.0f;

      clientHWControl.BeginUpdate();
      memcpy(clientHWControl.mpWriteBuff, &info, sizeof(rF2HWControl));
      clientHWControl.EndUpdate();
      Sleep(1000);

    }

    TEST_METHOD(ConnectAndWritePitMenuToggle)
    { //
      MappedBuffer<rF2HWControl> clientHWControl(SharedMemoryPlugin::MM_HWCONTROL_FILE_NAME);
      rF2HWControl info;
      double fRetVal;
      bool res;
      TEST_NAME_IN_DEBUG("ConnectAndWritePitMenuToggle");

      if (!clientHWControl.Initialize(SharedMemoryPlugin::msDedicatedServerMapGlobally)) {
        DEBUG_MSG(DebugLevel::Errors, "Failed to initialize client HWControl mapping");
        return;
      }

      strcpy(info.mControlName, "ToggleMFDB");
      info.mfRetVal = 1.0f;
      clientHWControl.BeginUpdate();
      memcpy(clientHWControl.mpWriteBuff, &info, sizeof(rF2HWControl));
      clientHWControl.EndUpdate();
      Sleep(100);

      // Send release
      strcpy(info.mControlName, "ToggleMFDB");
      info.mfRetVal = 0.0f;
      clientHWControl.BeginUpdate();
      memcpy(clientHWControl.mpWriteBuff, &info, sizeof(rF2HWControl));
      clientHWControl.EndUpdate();
      Sleep(100);

    }

    TEST_METHOD(ConnectAndWriteIncrementValue)
    { //
      MappedBuffer<rF2HWControl> clientHWControl(SharedMemoryPlugin::MM_HWCONTROL_FILE_NAME);
      rF2HWControl info;
      double fRetVal;
      bool res;
      TEST_NAME_IN_DEBUG("ConnectAndWriteIncrementValue");

      if (!clientHWControl.Initialize(SharedMemoryPlugin::msDedicatedServerMapGlobally)) {
        DEBUG_MSG(DebugLevel::Errors, "Failed to initialize client HWControl mapping");
        return;
      }

      strcpy(info.mControlName, "PitMenuIncrementValue");
      info.mfRetVal = 1.0f;
      clientHWControl.BeginUpdate();
      memcpy(clientHWControl.mpWriteBuff, &info, sizeof(rF2HWControl));
      clientHWControl.EndUpdate();
      Sleep(100);

      // Send release
      strcpy(info.mControlName, "PitMenuIncrementValue");
      info.mfRetVal = 0.0f;
      clientHWControl.BeginUpdate();
      memcpy(clientHWControl.mpWriteBuff, &info, sizeof(rF2HWControl));
      clientHWControl.EndUpdate();
      Sleep(100);

    }

    TEST_METHOD(ConnectAndWriteDecrementValue)
    { //
      MappedBuffer<rF2HWControl> clientHWControl(SharedMemoryPlugin::MM_HWCONTROL_FILE_NAME);
      rF2HWControl info;
      double fRetVal;
      bool res;
      TEST_NAME_IN_DEBUG("ConnectAndWriteDecrementValue");

      if (!clientHWControl.Initialize(SharedMemoryPlugin::msDedicatedServerMapGlobally)) {
        DEBUG_MSG(DebugLevel::Errors, "Failed to initialize client HWControl mapping");
        return;
      }

      strcpy(info.mControlName, "PitMenuDecrementValue");
      info.mfRetVal = 1.0f;
      clientHWControl.BeginUpdate();
      memcpy(clientHWControl.mpWriteBuff, &info, sizeof(rF2HWControl));
      clientHWControl.EndUpdate();
      Sleep(100);

      // Send release
      strcpy(info.mControlName, "PitMenuDecrementValue");
      info.mfRetVal = 0.0f;
      clientHWControl.BeginUpdate();
      memcpy(clientHWControl.mpWriteBuff, &info, sizeof(rF2HWControl));
      clientHWControl.EndUpdate();
      Sleep(100);

    }

    TEST_METHOD(ConnectAndWriteMenuUp)
    { //
      MappedBuffer<rF2HWControl> clientHWControl(SharedMemoryPlugin::MM_HWCONTROL_FILE_NAME);
      rF2HWControl info;
      double fRetVal;
      bool res;
      TEST_NAME_IN_DEBUG("ConnectAndWriteMenuUp");

      if (!clientHWControl.Initialize(SharedMemoryPlugin::msDedicatedServerMapGlobally)) {
        DEBUG_MSG(DebugLevel::Errors, "Failed to initialize client HWControl mapping");
        return;
      }

      strcpy(info.mControlName, "PitMenuUp");
      info.mfRetVal = 1.0f;
      clientHWControl.BeginUpdate();
      memcpy(clientHWControl.mpWriteBuff, &info, sizeof(rF2HWControl));
      clientHWControl.EndUpdate();
      Sleep(100);

      // Send release
      strcpy(info.mControlName, "PitMenuUp");
      info.mfRetVal = 0.0f;
      clientHWControl.BeginUpdate();
      memcpy(clientHWControl.mpWriteBuff, &info, sizeof(rF2HWControl));
      clientHWControl.EndUpdate();
      Sleep(100);

    }

    TEST_METHOD(ConnectAndWritePitMenuCompleteSequence)
    { //
      MappedBuffer<rF2HWControl> clientHWControl(SharedMemoryPlugin::MM_HWCONTROL_FILE_NAME);
      rF2HWControl info;
      double fRetVal;
      bool res;
      TEST_NAME_IN_DEBUG("ConnectAndWritePitMenuCompleteSequence");

      if (!clientHWControl.Initialize(SharedMemoryPlugin::msDedicatedServerMapGlobally)) {
        DEBUG_MSG(DebugLevel::Errors, "Failed to initialize client HWControl mapping");
        return;
      }

      Sleep(5000); // Give time to switch to rFactor
      strcpy(info.mControlName, "ToggleMFDB");
      info.mfRetVal = 1.0f;
      clientHWControl.BeginUpdate();
      memcpy(clientHWControl.mpWriteBuff, &info, sizeof(rF2HWControl));
      clientHWControl.EndUpdate();
      Sleep(1000);

      // Send release
      strcpy(info.mControlName, "ToggleMFDB");
      info.mfRetVal = 0.0f;
      clientHWControl.BeginUpdate();
      memcpy(clientHWControl.mpWriteBuff, &info, sizeof(rF2HWControl));
      clientHWControl.EndUpdate();
      Sleep(1000);

      strcpy(info.mControlName, "PitMenuIncrementValue");
      info.mfRetVal = 1.0f;
      clientHWControl.BeginUpdate();
      memcpy(clientHWControl.mpWriteBuff, &info, sizeof(rF2HWControl));
      clientHWControl.EndUpdate();
      Sleep(1000);

      // Send release
      strcpy(info.mControlName, "PitMenuIncrementValue");
      info.mfRetVal = 0.0f;
      clientHWControl.BeginUpdate();
      memcpy(clientHWControl.mpWriteBuff, &info, sizeof(rF2HWControl));
      clientHWControl.EndUpdate();
      Sleep(1000);
#if false
      strcpy(info.mControlName, "PitMenuUp");
      info.mfRetVal = 1.0f;
      clientHWControl.BeginUpdate();
      memcpy(clientHWControl.mpWriteBuff, &info, sizeof(rF2HWControl));
      clientHWControl.EndUpdate();
      Sleep(1000);

      // Send release
      strcpy(info.mControlName, "PitMenuUp");
      info.mfRetVal = 0.0f;
      clientHWControl.BeginUpdate();
      memcpy(clientHWControl.mpWriteBuff, &info, sizeof(rF2HWControl));
      clientHWControl.EndUpdate();
      Sleep(1000);
#endif
    }

  };
}