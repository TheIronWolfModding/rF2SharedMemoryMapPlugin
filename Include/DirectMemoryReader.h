/*
Author: The Iron Wolf (vleonavicius@hotmail.com)
Website: thecrewchief.org
*/

#pragma once

class DirectMemoryReader
{
public:
  DirectMemoryReader() {}

  bool Initialize();
  bool Read(rF2Extended& extended);
  bool ReadOnNewSession(rF2Extended& extended) const; 
  bool ReadOnFCY(rF2Extended& extended);

  bool IsSCRPluginEnabled() const { return mSCRPluginEnabled; }
  long GetSCRPluginDoubleFileType() const { return mSCRPluginDoubleFileType; }

private:
  void ReadSCRPluginConfig();
  void ReadSCRPluginConfigValues(char* const pluginConfig);

private:
  char* mpStatusMessage = nullptr;
  char** mppMessageCenterMessages = nullptr;
  float* mpCurrPitSpeedLimit = nullptr;
  char* mpSCRInstructionMessage = nullptr;

  char mPrevStatusMessage[rF2MappedBufferHeader::MAX_STATUS_MSG_LEN];
  char mPrevLastHistoryMessage[rF2MappedBufferHeader::MAX_STATUS_MSG_LEN];
  char mPrevSCRInstructionMessage[rF2MappedBufferHeader::MAX_STATUS_MSG_LEN];

  bool mSCRPluginEnabled = false;
  long mSCRPluginDoubleFileType = -1L;
};