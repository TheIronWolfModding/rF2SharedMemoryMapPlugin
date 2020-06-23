#include "pch.h"
#include "CppUnitTest.h"
#include <direct.h>
#include "rFactor2SharedMemoryMap.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

#define SMP_VERSION 3720

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
