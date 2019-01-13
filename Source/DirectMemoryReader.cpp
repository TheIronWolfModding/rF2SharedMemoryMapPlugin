#include "rFactor2SharedMemoryMap.hpp"
#include <stdlib.h>
#include <cstddef>                              // offsetof
#include "DirectMemoryReader.h"
#include "Utils.h"

bool DirectMemoryReader::Initialize()
{
  DEBUG_MSG(DebugLevel::DevInfo, "Initializing DMR.");
  
  auto const startTicks = TicksNow();
  
  auto const module = ::GetModuleHandle(nullptr);
  mpStatusMessage = reinterpret_cast<char*>(FindPatternForPointerInMemory(module,
     reinterpret_cast<unsigned char*>("\x74\x23\x48\x8D\x15\x5D\x31\xF5\x00\x48\x2B\xD3"),
     "xxxxx????xxx", 5u));

  if (mpStatusMessage == nullptr) {
    DEBUG_MSG(DebugLevel::Errors, "ERROR: Failed to resolve status message.");
    return false;
  }

  mppMessageCenterMessages = reinterpret_cast<char**>(FindPatternForPointerInMemory(module,
    reinterpret_cast<unsigned char*>("\x48\x8B\x05\xAA\xD1\xFF\x00\xC6\x80\xB8\x25\x00\x00\x01"),
    "xxx????xxxxxxx", 3u));

  if (mppMessageCenterMessages == nullptr) {
    DEBUG_MSG(DebugLevel::Errors, "ERROR: Failed to resolve message array.");
    return false;
  }

  auto const endTicks = TicksNow();

  if (SharedMemoryPlugin::msDebugOutputLevel >= DebugLevel::DevInfo) {
    // Normal scan ~9ms, failed scan ~35ms (debug).
    DEBUG_FLOAT2(DebugLevel::DevInfo, "Scan time seconds: ", (endTicks - startTicks) / MICROSECONDS_IN_SECOND);

    auto const addr1 = reinterpret_cast<char*>(reinterpret_cast<uintptr_t>(::GetModuleHandle(nullptr)) + 0x14BA0C0);
    auto const addr2 = *reinterpret_cast<char**>(reinterpret_cast<uintptr_t>(::GetModuleHandle(nullptr)) + 0x14B8738);

    DEBUG_ADDR2(DebugLevel::DevInfo, "A1", mpStatusMessage);
    DEBUG_ADDR2(DebugLevel::DevInfo, "A11", addr1);
    DEBUG_ADDR2(DebugLevel::DevInfo, "O1", reinterpret_cast<uintptr_t>(mpStatusMessage) - reinterpret_cast<uintptr_t>(module));
    DEBUG_ADDR2(DebugLevel::DevInfo, "O11", 0x14BA0C0uLL);

    DEBUG_ADDR2(DebugLevel::DevInfo, "A2", *mppMessageCenterMessages);
    DEBUG_ADDR2(DebugLevel::DevInfo, "A21", addr2);
    DEBUG_ADDR2(DebugLevel::DevInfo, "O2", reinterpret_cast<uintptr_t>(mppMessageCenterMessages) - reinterpret_cast<uintptr_t>(module));
    DEBUG_ADDR2(DebugLevel::DevInfo, "O21", 0x14B8738uLL);
  }

  return true;
}


bool DirectMemoryReader::Read(rF2Extended& extended)
{
  strcpy_s(extended.mStatusMessage, mpStatusMessage);
  if (strcmp(mPrevStatusMessage, extended.mStatusMessage) != 0) {
    DEBUG_MSG2(DebugLevel::DevInfo, "Status message updated: ", extended.mStatusMessage);

    strcpy_s(mPrevStatusMessage, extended.mStatusMessage);
    extended.mTicksStatusMessageUpdated = ::GetTickCount64();
  }

  if (*mppMessageCenterMessages == nullptr) {
    DEBUG_MSG2(DebugLevel::DevInfo, "No message array pointer assigned.", extended.mStatusMessage);
    return true;  // Retry next time or fail?
  }

  char msgBuff[rF2MappedBufferHeader::MAX_STATUS_MSG_LEN] = {};

  auto seenSplit = false;
  auto pCurr = *mppMessageCenterMessages + 0xC0 * 0x2F + 0x68;
  for (int i = 0; i < 0x30; ++i) {
    if (*pCurr != '\0') {
      if (*pCurr == ' ') {  // some messages are split, they begin with space though.
        DEBUG_MSG2(DebugLevel::DevInfo, "Found split line msg: ", pCurr);
        pCurr -= 0xC0;
        seenSplit = true;
        continue;
      }

      if (seenSplit) {
        strcpy_s(msgBuff, pCurr);

        auto j = i + 1;
        auto pAhead = pCurr + 0xC0;
        while (j < 0x30 && *pAhead == ' ' && *pCurr != '\0') {
          // TODO: remove this logging, excess
          DEBUG_MSG2(DebugLevel::DevInfo, "Curr msg: ", msgBuff);
          strcat_s(msgBuff, pAhead);
          DEBUG_MSG2(DebugLevel::DevInfo, "Concatenated: ", msgBuff);

          ++j;
          pAhead += 0xC0;
        }
      }

      if (!seenSplit)
        strcpy_s(extended.mLastHistoryMessage, pCurr);
      else
        strcpy_s(extended.mLastHistoryMessage, msgBuff);

      if (strcmp(mPrevLastHistoryMessage, extended.mLastHistoryMessage) != 0) {
        if (!seenSplit)
          DEBUG_MSG2(DebugLevel::DevInfo, "Last history message updated: ", extended.mLastHistoryMessage);
        else
          DEBUG_MSG2(DebugLevel::DevInfo, "Last history message updated (concatenated): ", extended.mLastHistoryMessage);

        strcpy_s(mPrevLastHistoryMessage, extended.mLastHistoryMessage);
        extended.mTicksLastHistoryMessageUpdated = ::GetTickCount64();
      }

      break;
    }

    pCurr -= 0xC0;
  }

  return true;
}
