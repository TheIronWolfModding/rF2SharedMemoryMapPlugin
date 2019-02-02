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
  mpStatusMessage = reinterpret_cast<char*>(Utils::FindPatternForPointerInMemory(module,
     reinterpret_cast<unsigned char*>("\x74\x23\x48\x8D\x15\x5D\x31\xF5\x00\x48\x2B\xD3"),
     "xxxxx????xxx", 5u));

  if (mpStatusMessage == nullptr) {
    DEBUG_MSG(DebugLevel::Errors, "ERROR: Failed to resolve status message.");
    return false;
  }

  mppMessageCenterMessages = reinterpret_cast<char**>(Utils::FindPatternForPointerInMemory(module,
    reinterpret_cast<unsigned char*>("\x48\x8B\x05\xAA\xD1\xFF\x00\xC6\x80\xB8\x25\x00\x00\x01"),
    "xxx????xxxxxxx", 3u));

  if (mppMessageCenterMessages == nullptr) {
    DEBUG_MSG(DebugLevel::Errors, "ERROR: Failed to resolve message array.");
    return false;
  }

  mpCurrPitSpeedLimit = reinterpret_cast<float*>(Utils::FindPatternForPointerInMemory(module,
    reinterpret_cast<unsigned char*>("\x57\x48\x83\xEC\x20\xF3\x0F\x10\x2D\x35\x9C\xEF\x00\x48\x8B\xF1"),
    "xxxxxxxxx????xxx", 9u));

  if (mpCurrPitSpeedLimit == nullptr) {
    DEBUG_MSG(DebugLevel::Errors, "ERROR: Failed to resolve speed limit pointer.");
    return false;
  }

  ReadSCRPluginConfig();

  if (mSCRPluginEnabled) {
    DEBUG_MSG(DebugLevel::DevInfo, "Initializing Rules Instruction message.");
    mpRulesInstructionMessage = reinterpret_cast<char*>(Utils::FindPatternForPointerInMemory(module,
      reinterpret_cast<unsigned char*>("\x48\x83\xEC\x28\xE8\xB7\xC1\xF7\xFF\xE8\xB2\x2D\x15\x00\x48\x8D\x0D\x73\xA2\x07\x01\xE8\xA6\x8B\x08\x00\x88\x05\x8C\xA2\x07\x01"),
      "xxxxx????x????xxx????x????xx????", 17u));

    if (mpRulesInstructionMessage == nullptr) {
      DEBUG_MSG(DebugLevel::Errors, "ERROR: Failed to resolve LSI message pointer.");
      return false;
    }
  }

  mpRulesInstructionMessage += 0x1D0uLL;

  auto const endTicks = TicksNow();

  if (SharedMemoryPlugin::msDebugOutputLevel >= DebugLevel::DevInfo) {
    // TODO: REMEASURE Normal scan ~9ms, failed scan ~35ms (debug).
    DEBUG_FLOAT2(DebugLevel::DevInfo, "Scan time seconds: ", (endTicks - startTicks) / MICROSECONDS_IN_SECOND);

    auto const addr1 = reinterpret_cast<char*>(reinterpret_cast<uintptr_t>(::GetModuleHandle(nullptr)) + 0x14C6110uLL);
    auto const addr2 = *reinterpret_cast<char**>(reinterpret_cast<uintptr_t>(::GetModuleHandle(nullptr)) + 0x14C4780uLL);
    auto const addr3 = reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(::GetModuleHandle(nullptr)) + 0x14A64ACuLL);
    auto const addr4 = reinterpret_cast<char*>(reinterpret_cast<uintptr_t>(::GetModuleHandle(nullptr)) + 0x14C49D8uLL);

    DEBUG_ADDR2(DebugLevel::DevInfo, "A1", mpStatusMessage);
    DEBUG_ADDR2(DebugLevel::DevInfo, "A11", addr1);
    DEBUG_ADDR2(DebugLevel::DevInfo, "O1", reinterpret_cast<uintptr_t>(mpStatusMessage) - reinterpret_cast<uintptr_t>(module));
    DEBUG_ADDR2(DebugLevel::DevInfo, "O11", 0x14C6110uLL);

    DEBUG_ADDR2(DebugLevel::DevInfo, "A2", *mppMessageCenterMessages);
    DEBUG_ADDR2(DebugLevel::DevInfo, "A21", addr2);
    DEBUG_ADDR2(DebugLevel::DevInfo, "O2", reinterpret_cast<uintptr_t>(mppMessageCenterMessages) - reinterpret_cast<uintptr_t>(module));
    DEBUG_ADDR2(DebugLevel::DevInfo, "O21", 0x14C4780uLL);

    DEBUG_ADDR2(DebugLevel::DevInfo, "A3", mpCurrPitSpeedLimit);
    DEBUG_ADDR2(DebugLevel::DevInfo, "A31", addr3);
    DEBUG_ADDR2(DebugLevel::DevInfo, "O3", reinterpret_cast<uintptr_t>(mpCurrPitSpeedLimit) - reinterpret_cast<uintptr_t>(module));
    DEBUG_ADDR2(DebugLevel::DevInfo, "O31", 0x14A64ACuLL);

    if (mSCRPluginEnabled) {
      DEBUG_ADDR2(DebugLevel::DevInfo, "A4", mpRulesInstructionMessage);
      DEBUG_ADDR2(DebugLevel::DevInfo, "A41", addr4);
      DEBUG_ADDR2(DebugLevel::DevInfo, "O4", reinterpret_cast<uintptr_t>(mpRulesInstructionMessage) - reinterpret_cast<uintptr_t>(module));
      DEBUG_ADDR2(DebugLevel::DevInfo, "O41", 0x14C49D8uLL);
    }
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

bool DirectMemoryReader::ReadOnFCY(rF2Extended& extended)
{
  if (!mSCRPluginEnabled) {
    assert(false && "SCR Plugin not enabled, should not call.");
    return false;
  }

  if (mpStatusMessage == nullptr || mppMessageCenterMessages == nullptr || mpCurrPitSpeedLimit == nullptr || mpRulesInstructionMessage == nullptr) {
    assert(false && "DMR not available, should not call.");
    return false;
  }

  if (mpRulesInstructionMessage[0] == '\0')
    return true;

  strcpy_s(extended.mRulesInstructionMessage, mpRulesInstructionMessage);
  if (strcmp(mPrevRulesInstructionMessage, extended.mRulesInstructionMessage) != 0) {
    DEBUG_MSG2(DebugLevel::DevInfo, "Rules Instruction message updated: ", extended.mRulesInstructionMessage);

    strcpy_s(mPrevRulesInstructionMessage, extended.mRulesInstructionMessage);
    extended.mTicksRulesInstructionMessageUpdated = ::GetTickCount64();
  }

  return true;
}


void DirectMemoryReader::ReadSCRPluginConfig()
{
  char wd[MAX_PATH] = {};
  ::GetCurrentDirectory(MAX_PATH, wd);

  auto const configFilePath = lstrcatA(wd, R"(\UserData\player\CustomPluginVariables.JSON)");

  auto configFileContents = Utils::GetFileContents(configFilePath);
  if (configFileContents == nullptr) {
    DEBUG_MSG(DebugLevel::Errors, "Failed to load CustomPluginVariables.JSON file");
    return;
  }

  auto onExit = Utils::MakeScopeGuard([&]() {
    delete[] configFileContents;
  });

  ReadSCRPluginConfigValues(configFileContents);
}


void DirectMemoryReader::ReadSCRPluginConfigValues(char* const configFileContents)
{
  // See if plugin is enabled:
  auto curLine = strstr(configFileContents, "StockCarRules.dll");
  while (curLine != nullptr) {
    // Cut off next line from the current text.
    auto const nextLine = strstr(curLine, "\r\n");
    if (nextLine != nullptr)
      *nextLine = '\0';

    auto onExitOrNewIteration = Utils::MakeScopeGuard([&]() {
      // Restore the original line.
      if (nextLine != nullptr)
        *nextLine = '\r';
    });

    auto const closingBrace = strchr(curLine, '}');
    if (closingBrace != nullptr) {
      // End of {} for a plugin.
      return;
    }

    if (!mSCRPluginEnabled) {
      // Check if plugin is enabled.
      auto const enabled = strstr(curLine, " \" Enabled\":1");
      if (enabled != nullptr)
        mSCRPluginEnabled = true;
    }

    if (mSCRPluginDoubleFileType == -1L) {
      auto const dft = strstr(curLine, " \"DoubleFileType\":");
      if (dft != nullptr) {
        char value[2] = {};
        value[0] = *(dft + sizeof("\"DoubleFileType\":"));
        mSCRPluginDoubleFileType = atol(value);
      }
    }

    if (mSCRPluginEnabled && mSCRPluginDoubleFileType != -1L)
      return;

    curLine = nextLine != nullptr ? (nextLine + 2 /*skip \r\n*/) : nullptr;
  }

  // If we're here, consider SCR plugin as not enabled.
  mSCRPluginEnabled = false;

  return;
}
