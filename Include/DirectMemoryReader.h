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

private:
  char* mpStatusMessage = nullptr;
  char** mppMessageCenterMessages = nullptr;
  float* mpCurrPitSpeedLimit = nullptr;

  char mPrevStatusMessage[rF2MappedBufferHeader::MAX_STATUS_MSG_LEN];
  char mPrevLastHistoryMessage[rF2MappedBufferHeader::MAX_STATUS_MSG_LEN];
};