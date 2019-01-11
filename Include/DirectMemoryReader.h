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
  void Read(rF2Extended& extended);

private:
  //uintptr_t 
  char* mpStatusMessage = nullptr;
  char* mpMessageCenterMessages = nullptr;

  char mPrevStatusMessage[rF2Extended::MAX_STATUS_MSG_LEN];
  char mPrevLastHistoryMessage[rF2Extended::MAX_STATUS_MSG_LEN];
};