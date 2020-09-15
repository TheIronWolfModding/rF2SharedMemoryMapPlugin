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
  bool Read(GTR2Extended& extended);
  bool ReadOnNewSession(GTR2Extended& extended);
  bool ReadOnLSIVisible(GTR2Extended& extended);

  bool IsSCRPluginEnabled() const { return mSCRPluginEnabled; }
  long GetSCRPluginDoubleFileType() const { return mSCRPluginDoubleFileType; }
  void ClearLSIValues(GTR2Extended& extended);

private:
  void ReadSCRPluginConfig();
  void ReadSCRPluginConfigValues(char* const pluginConfig);

private:
  char* mpStatusMessage = nullptr;
  char** mppMessageCenterMessages = nullptr;
  float* mpCurrPitSpeedLimit = nullptr;
  char* mpLSIMessages = nullptr;

  char mPrevStatusMessage[GTR2Extended::MAX_STATUS_MSG_LEN];
  char mPrevLastHistoryMessage[GTR2Extended::MAX_STATUS_MSG_LEN];
  char mPrevLSIPhaseMessage[GTR2Extended::MAX_RULES_INSTRUCTION_MSG_LEN];
  char mPrevLSIPitStateMessage[GTR2Extended::MAX_RULES_INSTRUCTION_MSG_LEN];
  char mPrevLSIOrderInstructionMessage[GTR2Extended::MAX_RULES_INSTRUCTION_MSG_LEN];
  char mPrevLSIRulesInstructionMessage[GTR2Extended::MAX_RULES_INSTRUCTION_MSG_LEN];

  bool mSCRPluginEnabled = false;
  long mSCRPluginDoubleFileType = -1L;
};