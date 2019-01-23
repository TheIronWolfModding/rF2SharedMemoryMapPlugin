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

  mpCurrPitSpeedLimit = reinterpret_cast<float*>(FindPatternForPointerInMemory(module,
    reinterpret_cast<unsigned char*>("\x57\x48\x83\xEC\x20\xF3\x0F\x10\x2D\x35\x9C\xEF\x00\x48\x8B\xF1"),
    "xxxxxxxxx????xxx", 9u));

  if (mpCurrPitSpeedLimit == nullptr) {
    DEBUG_MSG(DebugLevel::Errors, "ERROR: Failed to resolve speed limit pointer.");
    return false;
  }

  auto const endTicks = TicksNow();

  if (SharedMemoryPlugin::msDebugOutputLevel >= DebugLevel::DevInfo) {
    // Normal scan ~9ms, failed scan ~35ms (debug).
    DEBUG_FLOAT2(DebugLevel::DevInfo, "Scan time seconds: ", (endTicks - startTicks) / MICROSECONDS_IN_SECOND);

    auto const addr1 = reinterpret_cast<char*>(reinterpret_cast<uintptr_t>(::GetModuleHandle(nullptr)) + 0x14BA0C0);
    auto const addr2 = *reinterpret_cast<char**>(reinterpret_cast<uintptr_t>(::GetModuleHandle(nullptr)) + 0x14B8738);
    auto const addr3 = reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(::GetModuleHandle(nullptr)) + 0x149A46C);

    DEBUG_ADDR2(DebugLevel::DevInfo, "A1", mpStatusMessage);
    DEBUG_ADDR2(DebugLevel::DevInfo, "A11", addr1);
    DEBUG_ADDR2(DebugLevel::DevInfo, "O1", reinterpret_cast<uintptr_t>(mpStatusMessage) - reinterpret_cast<uintptr_t>(module));
    DEBUG_ADDR2(DebugLevel::DevInfo, "O11", 0x14BA0C0uLL);

    DEBUG_ADDR2(DebugLevel::DevInfo, "A2", *mppMessageCenterMessages);
    DEBUG_ADDR2(DebugLevel::DevInfo, "A21", addr2);
    DEBUG_ADDR2(DebugLevel::DevInfo, "O2", reinterpret_cast<uintptr_t>(mppMessageCenterMessages) - reinterpret_cast<uintptr_t>(module));
    DEBUG_ADDR2(DebugLevel::DevInfo, "O21", 0x14B8738uLL);

    DEBUG_ADDR2(DebugLevel::DevInfo, "A3", mpCurrPitSpeedLimit);
    DEBUG_ADDR2(DebugLevel::DevInfo, "A31", addr3);
    DEBUG_ADDR2(DebugLevel::DevInfo, "O3", reinterpret_cast<uintptr_t>(mpCurrPitSpeedLimit) - reinterpret_cast<uintptr_t>(module));
    DEBUG_ADDR2(DebugLevel::DevInfo, "O31", 0x149A46CuLL);
  }

  return true;
}


bool DirectMemoryReader::Read(rF2Extended& extended)
{
  if (mpStatusMessage == nullptr || mppMessageCenterMessages == nullptr || mpCurrPitSpeedLimit == nullptr) {
    assert(false && "DMR not available, should not call.");
    return false;
  }

  strcpy_s(extended.mStatusMessage, mpStatusMessage);
  if (strcmp(mPrevStatusMessage, extended.mStatusMessage) != 0) {
    DEBUG_MSG2(DebugLevel::DevInfo, "Status message updated: ", extended.mStatusMessage);

    strcpy_s(mPrevStatusMessage, extended.mStatusMessage);
    extended.mTicksStatusMessageUpdated = ::GetTickCount64();
  }

  auto const pBegin = *mppMessageCenterMessages;
  if (pBegin == nullptr) {
    DEBUG_MSG2(DebugLevel::DevInfo, "No message array pointer assigned.", extended.mStatusMessage);
    return true;  // Retry next time or fail?  Have counter for N failures?
  }

  char msgBuff[rF2MappedBufferHeader::MAX_STATUS_MSG_LEN];

  auto seenSplit = false;
  auto pCurr = pBegin + 0xC0 * 0x2F + 0x68;
  auto const pPastEnd = pBegin + 0xC0 * 0x30;
  for (int i = 0;
    i < 0x30 && pCurr >= pBegin;
    ++i) {
    assert(pCurr >= pBegin && pCurr < pPastEnd);

    if (*pCurr != '\0') {
      if (*pCurr == ' ') {  // some messages are split, they begin with space though.
        pCurr -= 0xC0;
        seenSplit = true;
        continue;
      }

      if (seenSplit) {
        strcpy_s(msgBuff, pCurr);

        auto j = i - 1;
        auto pAhead = pCurr + 0xC0;
        assert(pAhead >= pBegin && pAhead < pPastEnd);

        while (j >= 0
          && *pAhead == ' '
          && *pCurr != '\0'
          && pAhead < pPastEnd) {
          assert(pAhead >= pBegin && pAhead < pPastEnd);
          assert(j >= 0 && j < 0x30);

          strcat_s(msgBuff, pAhead);

          --j;
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

bool DirectMemoryReader::ReadOnNewSession(rF2Extended& extended) const
{
  if (mpStatusMessage == nullptr || mppMessageCenterMessages == nullptr || mpCurrPitSpeedLimit == nullptr) {
    assert(false && "DMR not available, should not call.");
    return false;
  }

  extended.mCurrentPitSpeedLimit = *mpCurrPitSpeedLimit;
  DEBUG_FLOAT2(DebugLevel::DevInfo, "Current pit speed limit: ", extended.mCurrentPitSpeedLimit);

  return true;
}

