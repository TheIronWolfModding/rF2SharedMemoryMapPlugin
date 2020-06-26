#include "pch.h"
#include "HWControl.h"
#include "rFactor2SharedMemoryMap.hpp"
#include "MappedBuffer.h"

MappedBuffer<rF2HWControl> clientHWControl(SharedMemoryPlugin::MM_HWCONTROL_FILE_NAME);
MappedBuffer<rF2PitInfo> mPitInfo(SharedMemoryPlugin::MM_PIT_INFO_FILE_NAME);

void HWControl::Initialize(void)
{
  if (!clientHWControl.Initialize(SharedMemoryPlugin::msDedicatedServerMapGlobally))
  {
    DEBUG_MSG(DebugLevel::Errors, "Failed to initialize client HWControl mapping");
    return;
  }
  if (!mPitInfo.Initialize(SharedMemoryPlugin::msDedicatedServerMapGlobally))
  {
    DEBUG_MSG(DebugLevel::Errors, "Failed to initialize client PitInfo mapping");
    return;
  }
  sendHWControl("ToggleMFDB", 1.0f);
  Sleep(100);
  sendHWControl("ToggleMFDB", 0.0f);
}
void HWControl::sendHWControl(char* controlName, double retVal)
{
  rF2HWControl info;
  rF2PitInfo pitInfo;
  char mCategoryName[32];
  rF2PitMenu *pInfo;

  strcpy(info.mControlName, controlName);
  info.mfRetVal = retVal;
  clientHWControl.BeginUpdate();
  memcpy(clientHWControl.mpBuff, &info, sizeof(rF2HWControl));
  clientHWControl.EndUpdate();

  mPitInfo.BeginUpdate();
  //pInfo = mPitInfo;// .mpBuff; // ->mPitMenu;
  //memcpy(pInfo, &(mPitInfo.mpBuff->mPitMenu), sizeof(rF2PitMenu));
  strcpy(mCategoryName, (*(rF2PitMenu*)&mPitInfo.mpBuff).mCategoryName);
  //mPitInfo.mpBuff->mPitMenu.mChoiceString;
  mPitInfo.EndUpdate();


}

