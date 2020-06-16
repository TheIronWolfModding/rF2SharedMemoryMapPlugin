#include "pch.h"
#include "CppUnitTest.h"
#include <direct.h>
#include <iostream>
#include <fstream>
using namespace std;

#define UNITTEST  // Make private methods and data available to unit test
// Perhaps not necessary if a client created?
#include "rFactor2SharedMemoryMap.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

#define SMP_VERSION 3720
#define TEST_NAME_IN_DEBUG(name) \
    DEBUG_MSG(DebugLevel::DevInfo, "**** FROM UNIT TEST " ## name ## " ****")

namespace UnitTestStartup // Just test the startup code
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
      TEST_NAME_IN_DEBUG("Test_Startup");
      smp_obj.Startup(SMP_VERSION);
    }
  };
}

namespace UnitTestMethods // startup code tested, now use it to test the rest
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

  TEST_CLASS(PrivateMethods)
  {
  public:

    TEST_METHOD(TestMethod1st)
    { // 1st test to establish format, doesn't test anyting
      TEST_NAME_IN_DEBUG("TestMethod1st");
      SharedMemoryPlugin::ExtendedStateTracker::ExtendedStateTracker();
    }
  };

  TEST_CLASS(PublicMethods)
  {
  public:

    TEST_METHOD(Test_GetPluginName)
    {
      //char* name = GetPluginName();
    }

    TEST_METHOD(Test_HasHardwareInputs)
    {
      double RetVal;

      TEST_NAME_IN_DEBUG("Test_HasHardwareInputs");
      bool ret = smp_obj.HasHardwareInputs();
      Assert::IsTrue(ret);
    }

    TEST_METHOD(Test_CheckHWControl)
    {
      double RetVal;

      TEST_NAME_IN_DEBUG("Test_CheckHWControl");
      bool ret = smp_obj.CheckHWControl("Headlights", RetVal);
      Assert::IsTrue(ret);
    }

    TEST_METHOD(Test_UpdateGraphics)
    {
      GraphicsInfoV01 GraphicsInfo;
      GraphicsInfo.mAmbientBlue = 0;
      TEST_NAME_IN_DEBUG("Test_UpdateGraphics");
      smp_obj.UpdateGraphics(GraphicsInfo);
    }

    TEST_METHOD(Test_SetPhysicsOptions)
    {
      PhysicsOptionsV01 PhysicsOptions;
      PhysicsOptions.mAIControl = 1;
      TEST_NAME_IN_DEBUG("Test_SetPhysicsOptions");
      smp_obj.SetPhysicsOptions(PhysicsOptions);
    }

    TEST_METHOD(Test_UpdateTelemetry)
    {
      TelemInfoV01 info;
      TEST_NAME_IN_DEBUG("Test_UpdateTelemetry");
      smp_obj.UpdateTelemetry(info);
      SharedMemoryPlugin::msDebugISIInternals = false; // fprintf goes bang if called twice
      smp_obj.UpdateTelemetry(info);
    }

    TEST_METHOD(TestAccessPitMenu)
    {
      PitMenuV01 info;
      info.mCategoryIndex = 1;
      info.mChoiceIndex = 1;
      strcpy_s(info.mCategoryName, sizeof(info.mCategoryName), "PIT MENU 1");
      strcpy_s(info.mChoiceString, sizeof(info.mChoiceString), "CHOICE 1");
      TEST_NAME_IN_DEBUG("TestAccessPitMenu");
      smp_obj.AccessPitMenu(info);
      Assert::IsTrue(smp_obj.mPitMenu.mpBuff->changed);
    }

    TEST_METHOD(TestAccessPitMenu_Timing)
    {
      PitMenuV01 info;
      info.mCategoryIndex = 2;
      info.mChoiceIndex = 2;
      strcpy_s(info.mCategoryName, sizeof(info.mCategoryName), "PIT MENU 2");
      strcpy_s(info.mChoiceString, sizeof(info.mChoiceString), "CHOICE 2");
      TEST_NAME_IN_DEBUG("TestAccessPitMenu_Timing");
      // Hit it several times to test the timing
      smp_obj.AccessPitMenu(info);
      SharedMemoryPlugin::msDebugISIInternals = false; // fprintf goes bang if called twice
      smp_obj.AccessPitMenu(info);
      smp_obj.AccessPitMenu(info);
      smp_obj.AccessPitMenu(info);
      smp_obj.AccessPitMenu(info);
    }

    TEST_METHOD(TestISIinternals)
    {
      CustomVariableV01 var;

      // Set debug level to capture everything
      strcpy_s(var.mCaption, sizeof(var.mCaption), "DebugISIInternals");
      var.mCurrentSetting = static_cast<long>(1);
      smp_obj.AccessCustomVariable(var);

      const char *FILENAME = "Userdata\\Log\\RF2SMMP_InternalsPitMenuOutput.txt";
      remove(FILENAME);

      PitMenuV01 info;
      info.mCategoryIndex = 3;
      info.mChoiceIndex = 3;
      strcpy_s(info.mCategoryName, sizeof(info.mCategoryName), "PIT MENU 3");
      strcpy_s(info.mChoiceString, sizeof(info.mChoiceString), "CHOICE 3");
      TEST_NAME_IN_DEBUG("TestAccessPitMenu_Timing");
      smp_obj.AccessPitMenu(info);
#ifdef write_RF2SMMP_InternalsPitMenuOutput_disabled
      // Writing twice causes a crash??? smp_obj.AccessPitMenu(info);
      ifstream RF2SMMP_InternalsPitMenuOutput(FILENAME);
      Assert::IsTrue(RF2SMMP_InternalsPitMenuOutput.is_open());
#endif
    }
  };
}

