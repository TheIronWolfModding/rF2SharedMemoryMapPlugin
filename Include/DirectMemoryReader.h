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

private:
  char* mpStatusMessage = nullptr;
  char** mppMessageCenterMessages = nullptr;

  char mPrevStatusMessage[rF2Extended::MAX_STATUS_MSG_LEN];
  char mPrevLastHistoryMessage[rF2Extended::MAX_STATUS_MSG_LEN];
};