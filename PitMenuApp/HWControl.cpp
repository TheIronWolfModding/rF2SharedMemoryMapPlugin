#include "pch.h"
#include "HWControl.h"
#include "rFactor2SharedMemoryMap.hpp"
#include "MappedBuffer.h"

//MappedBuffer<rF2HWControl> clientHWControl;

void HWControl::Initialize(void)
{
#if false
  MappedBuffer<rF2HWControl> clientHWControl(SharedMemoryPlugin::MM_HWCONTROL_FILE_NAME);
  if (!clientHWControl.Initialize(SharedMemoryPlugin::msDedicatedServerMapGlobally))
  {
    DEBUG_MSG(DebugLevel::Errors, "Failed to initialize client HWControl mapping");
    return;
  }
#endif
}
void HWControl::sendHWControl(char* controlName, double retVal)
{
#if false
  MappedBuffer<rF2HWControl> clientHWControl(SharedMemoryPlugin::MM_HWCONTROL_FILE_NAME);
  rF2HWControl info;

  strcpy(info.mControlName, controlName);
  info.mfRetVal = retVal;
  clientHWControl.BeginUpdate();
  memcpy(clientHWControl.mpBuff, &info, sizeof(rF2HWControl));
  clientHWControl.EndUpdate();
#endif
}

