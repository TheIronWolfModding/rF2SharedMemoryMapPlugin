#include "pch.h"
#include "HWControl.h"
#include "rFactor2SharedMemoryMap.hpp"
#include "MappedBuffer.h"

//MappedBuffer<rF2HWControl> clientHWControl;

void HWControl::Initialize(void){
  MappedBuffer<rF2HWControl> clientHWControl(SharedMemoryPlugin::MM_HWCONTROL_FILE_NAME);
  if (!clientHWControl.Initialize(SharedMemoryPlugin::msDedicatedServerMapGlobally)) {
    DEBUG_MSG(DebugLevel::Errors, "Failed to initialize client HWControl mapping");
    return;
  }
}
void HWControl::sendHWControl(char* controlName, double retVal) {  
}

