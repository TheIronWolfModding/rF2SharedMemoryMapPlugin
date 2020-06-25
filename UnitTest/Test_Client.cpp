#include "pch.h"
#include "CppUnitTest.h"
#include <direct.h>
#include "rFactor2SharedMemoryMap.hpp"
#include "MappedBuffer.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

#define SMP_VERSION 3720
#define TEST_NAME_IN_DEBUG(name) \
    DEBUG_MSG(DebugLevel::DevInfo, "**** FROM CLIENT UNIT TEST " ## name ## " ****")


namespace TestClientStartup // Just test the startup code
{
  TEST_CLASS(UnitTest)
  {
  public:

    SharedMemoryPlugin smp_obj;

    TEST_METHOD(init)
    {
      // Run once when the module is loaded.
      CustomVariableV01 var;

      // Create Userdata\Log folder for debug files
      // found in VC12\x64\UnitTest\Userdata\Log\RF2SMMP_DebugOutput.txt
      _mkdir("Userdata");
      _mkdir("Userdata\\Log");

      // Set debug level to capture everything
      strcpy_s(var.mCaption, sizeof(var.mCaption), "DebugOutputLevel");
      var.mCurrentSetting = static_cast<long>(DebugLevel::Verbose);
      smp_obj.AccessCustomVariable(var);

    }
    TEST_METHOD(Test_Startup)
    {
      // GO!
      smp_obj.Startup(SMP_VERSION);
    }
  };
}

namespace TestHWControl //
{
  SharedMemoryPlugin smp_obj;

  TEST_MODULE_INITIALIZE(init)
  {
    // Run once when the module is loaded.
    CustomVariableV01 var;

    // Create Userdata\Log folder for debug files
    // found in VC12\x64\Debug\Userdata\Log\RF2SMMP_DebugOutput.txt
    _mkdir("Userdata");
    _mkdir("Userdata\\Log");

    SharedMemoryPlugin::msDebugISIInternals = true;

    TEST_NAME_IN_DEBUG("INITIALIZE");
    // Set debug level to capture everything
    strcpy_s(var.mCaption, sizeof(var.mCaption), "DebugOutputLevel");
    var.mCurrentSetting = static_cast<long>(DebugLevel::Verbose);
    smp_obj.AccessCustomVariable(var);

    // GO!
    smp_obj.Startup(SMP_VERSION);
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

      //smp_obj.Startup(SMP_VERSION);

      if (!clientHWControl.Initialize(SharedMemoryPlugin::msDedicatedServerMapGlobally)) {
        DEBUG_MSG(DebugLevel::Errors, "Failed to initialize client HWControl mapping");
        return;
      }
      smp_obj.Shutdown();
      clientHWControl.ReleaseResources();
    }

    TEST_METHOD(ConnectAndWrite)
    { //
      MappedBuffer<rF2HWControl> clientHWControl(SharedMemoryPlugin::MM_HWCONTROL_FILE_NAME);
      rF2HWControl info;
      double fRetVal;
      TEST_NAME_IN_DEBUG("ConnectAndWrite");

      smp_obj.Startup(SMP_VERSION);

      if (!clientHWControl.Initialize(SharedMemoryPlugin::msDedicatedServerMapGlobally)) {
        DEBUG_MSG(DebugLevel::Errors, "Failed to initialize client HWControl mapping");
        return;
      }

      strcpy(info.mControlName, "ToggleMFDB");
      info.mfRetVal = 1.0f;

      clientHWControl.BeginUpdate();
      memcpy(clientHWControl.mpBuff, &info, sizeof(rF2HWControl));
      clientHWControl.EndUpdate();

      bool res = smp_obj.CheckHWControl("ToggleMFDB", fRetVal);
      Assert::IsTrue(res);

      smp_obj.Shutdown();
    }

    TEST_METHOD(ConnectAndWriteMultiple)
    { //
      MappedBuffer<rF2HWControl> clientHWControl(SharedMemoryPlugin::MM_HWCONTROL_FILE_NAME);
      rF2HWControl info;
      double fRetVal;
      bool res;
      TEST_NAME_IN_DEBUG("ConnectAndWriteMultiple");

      smp_obj.Startup(SMP_VERSION);

      if (!clientHWControl.Initialize(SharedMemoryPlugin::msDedicatedServerMapGlobally)) {
        DEBUG_MSG(DebugLevel::Errors, "Failed to initialize client HWControl mapping");
        return;
      }

      strcpy(info.mControlName, "ToggleMFDB");
      info.mfRetVal = 1.0f;
      clientHWControl.BeginUpdate();
      memcpy(clientHWControl.mpBuff, &info, sizeof(rF2HWControl));
      clientHWControl.EndUpdate();

      res = smp_obj.CheckHWControl("ToggleMFDB", fRetVal);
      Assert::IsTrue(res);

      // Dealt with so calling it again has no effect
      res = smp_obj.CheckHWControl("ToggleMFDB", fRetVal);
      Assert::IsFalse(res);

      // Send release
      strcpy(info.mControlName, "ToggleMFDB");
      info.mfRetVal = 0.0f;
      clientHWControl.BeginUpdate();
      memcpy(clientHWControl.mpBuff, &info, sizeof(rF2HWControl));
      clientHWControl.EndUpdate();

      res = smp_obj.CheckHWControl("ToggleMFDB", fRetVal);
      Assert::IsTrue(res);

      // Dealt with so calling it again has no effect
      res = smp_obj.CheckHWControl("ToggleMFDB", fRetVal);
      Assert::IsFalse(res);

      // Send it but rFactor is checking other controls
      strcpy(info.mControlName, "ToggleMFDB");
      info.mfRetVal = 1.0f;
      clientHWControl.BeginUpdate();
      memcpy(clientHWControl.mpBuff, &info, sizeof(rF2HWControl));
      clientHWControl.EndUpdate();

      // Ignored when asking about ToggleMFDA
      res = smp_obj.CheckHWControl("ToggleMFDA", fRetVal);
      Assert::IsFalse(res);
      // Matches when asking about ToggleMFDB
      res = smp_obj.CheckHWControl("ToggleMFDB", fRetVal);
      Assert::IsTrue(res);
      
      smp_obj.Shutdown();
    }

  };
}