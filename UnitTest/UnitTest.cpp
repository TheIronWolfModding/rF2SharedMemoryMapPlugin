#include "pch.h"
#include "CppUnitTest.h"
#include <direct.h>
#define UNITTEST
#include "rFactor2SharedMemoryMap.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

#define SMP_VERSION 3720

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

namespace UnitTestMethods // startup code tested, now use it to test the rest
{
  SharedMemoryPlugin smp_obj;

  TEST_MODULE_INITIALIZE(init)
  {
    // Run once when the module is loaded.
    CustomVariableV01 var;

    // Create Userdata\Log folder for debug files
    _mkdir("Userdata");
    _mkdir("Userdata\\Log");

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
    TEST_METHOD(Test_CheckHWControl)
    {
      double RetVal;

      bool ret = smp_obj.CheckHWControl("Headlights", RetVal);
      Assert::IsTrue(ret);
    }

    TEST_METHOD(Test_UpdateGraphics)
    {
      GraphicsInfoV01 GraphicsInfo;
      GraphicsInfo.mAmbientBlue = 0;
      smp_obj.UpdateGraphics(GraphicsInfo);
    }

    TEST_METHOD(Test_SetPhysicsOptions)
    {
      PhysicsOptionsV01 PhysicsOptions;
      PhysicsOptions.mAIControl = 1;
      smp_obj.SetPhysicsOptions(PhysicsOptions);
    }

    TEST_METHOD(Test_UpdateTelemetry)
    {
      TelemInfoV01 info;
      smp_obj.UpdateTelemetry(info);
    }

    TEST_METHOD(TestAccessPitMenu)
    {
      PitMenuV01 info;
      info.mCategoryIndex = 1;
      info.mChoiceIndex = 1;
      strcpy_s(info.mCategoryName, sizeof(info.mCategoryName), "PIT MENU 1");
      strcpy_s(info.mChoiceString, sizeof(info.mChoiceString), "CHOICE 1");
      smp_obj.AccessPitMenu(info);
    }

  };
}
