#include "rFactor2SharedMemoryMap.hpp"
#include <stdlib.h>
#include <cstddef>                              // offsetof
#include "DirectMemoryReader.h"
#include "Utils.h"

bool DirectMemoryReader::Initialize()
{
  __try {
    DEBUG_MSG(DebugLevel::DevInfo, "Initializing DMR.");

    auto const startTicks = TicksNow();

    auto const module = ::GetModuleHandle(nullptr);
    mpStatusMessage = reinterpret_cast<char*>(Utils::FindPatternForPointerInMemory(module,
      reinterpret_cast<unsigned char*>("\x74\x23\x48\x8D\x15\x5D\x31\xF5\x00\x48\x2B\xD3"),
      "xxxxx????xxx", 5u));

    if (mpStatusMessage == nullptr) {
      DEBUG_MSG(DebugLevel::Errors, "Failed to resolve status message.");
      return false;
    }

    mppMessageCenterMessages = reinterpret_cast<char**>(Utils::FindPatternForPointerInMemory(module,
      reinterpret_cast<unsigned char*>("\x48\x8B\x05\xAA\xD1\xFF\x00\xC6\x80\xB8\x25\x00\x00\x01"),
      "xxx????xxxxxxx", 3u));

    if (mppMessageCenterMessages == nullptr) {
      DEBUG_MSG(DebugLevel::Errors, "Failed to resolve message array.");
      return false;
    }

    mpCurrPitSpeedLimit = reinterpret_cast<float*>(Utils::FindPatternForPointerInMemory(module,
      reinterpret_cast<unsigned char*>("\x57\x48\x83\xEC\x20\xF3\x0F\x10\x2D\x35\x9C\xEF\x00\x48\x8B\xF1"),
      "xxxxxxxxx????xxx", 9u));

    if (mpCurrPitSpeedLimit == nullptr) {
      DEBUG_MSG(DebugLevel::Errors, "Failed to resolve speed limit pointer.");
      return false;
    }

    ReadSCRPluginConfig();

    mpLSIMessages = reinterpret_cast<char*>(Utils::FindPatternForPointerInMemory(module,
      reinterpret_cast<unsigned char*>("\x42\x88\x8C\x38\x0F\x02\x00\x00\x84\xC9\x75\xEB\x48\x8D\x15\x70\x3A\x05\x01\x48\x8D\x0D\xB1\x20\x05\x01\xE8"),
      "xxxxxxxxxxxxxxx????xxx????x", 22u));

    if (mpLSIMessages == nullptr) {
      DEBUG_MSG(DebugLevel::Errors, "Failed to resolve LSI message pointer.");
      return false;
    }

    auto const endTicks = TicksNow();

    if (Utils::IsFlagOn(SharedMemoryPlugin::msDebugOutputLevel, DebugLevel::DevInfo)) {
      // Successful scan: ~20ms
      DEBUG_MSG(DebugLevel::DevInfo, "Scan time seconds: %f", (endTicks - startTicks) / MICROSECONDS_IN_SECOND);

      auto const addr1 = reinterpret_cast<char*>(reinterpret_cast<uintptr_t>(::GetModuleHandle(nullptr)) + 0x14D4C20uLL);
      auto const addr2 = *reinterpret_cast<char**>(reinterpret_cast<uintptr_t>(::GetModuleHandle(nullptr)) + 0x14D31E0uLL);
      auto const addr3 = reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(::GetModuleHandle(nullptr)) + 0x14B4D0CuLL);
      auto const addr4 = reinterpret_cast<char*>(reinterpret_cast<uintptr_t>(::GetModuleHandle(nullptr)) + 0x14D3268uLL);

      DEBUG_MSG(DebugLevel::DevInfo, "A1 0x%p", mpStatusMessage);
      DEBUG_MSG(DebugLevel::DevInfo, "A11 0x%p", addr1);
      DEBUG_MSG(DebugLevel::DevInfo, "O1 0x%p", reinterpret_cast<uintptr_t>(mpStatusMessage) - reinterpret_cast<uintptr_t>(module));
      DEBUG_MSG(DebugLevel::DevInfo, "O11 0x%p", 0x14D4C20uLL);

      DEBUG_MSG(DebugLevel::DevInfo, "A2 0x%p", *mppMessageCenterMessages);
      DEBUG_MSG(DebugLevel::DevInfo, "A21 0x%p", addr2);
      DEBUG_MSG(DebugLevel::DevInfo, "O2 0x%p", reinterpret_cast<uintptr_t>(mppMessageCenterMessages) - reinterpret_cast<uintptr_t>(module));
      DEBUG_MSG(DebugLevel::DevInfo, "O21 0x%p", 0x14D31E0uLL);

      DEBUG_MSG(DebugLevel::DevInfo, "A3 0x%p", mpCurrPitSpeedLimit);
      DEBUG_MSG(DebugLevel::DevInfo, "A31 0x%p", addr3);
      DEBUG_MSG(DebugLevel::DevInfo, "O3 0x%p", reinterpret_cast<uintptr_t>(mpCurrPitSpeedLimit) - reinterpret_cast<uintptr_t>(module));
      DEBUG_MSG(DebugLevel::DevInfo, "O31 0x%p", 0x14B4D0CuLL);

      DEBUG_MSG(DebugLevel::DevInfo, "A4 0x%p", mpLSIMessages);
      DEBUG_MSG(DebugLevel::DevInfo, "A41 0x%p", addr4);
      DEBUG_MSG(DebugLevel::DevInfo, "O4 0x%p", reinterpret_cast<uintptr_t>(mpLSIMessages) - reinterpret_cast<uintptr_t>(module));
      DEBUG_MSG(DebugLevel::DevInfo, "O41 0x%p", 0x14D3268uLL);
    }
  }
  __except (::GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
    DEBUG_MSG(DebugLevel::Errors, "Exception while reading memory, disabling DMA.");
    return false;
  }

  return true;
}


bool DirectMemoryReader::Read(rF2Extended& extended)
{
  __try {
    if (mpStatusMessage == nullptr || mppMessageCenterMessages == nullptr || mpCurrPitSpeedLimit == nullptr) {
      assert(false && "DMR not available, should not call.");
      return false;
    }

    if (strncmp(mPrevStatusMessage, mpStatusMessage, rF2Extended::MAX_STATUS_MSG_LEN) != 0) {
      strcpy_s(extended.mStatusMessage, mpStatusMessage);
      strcpy_s(mPrevStatusMessage, extended.mStatusMessage);
      extended.mTicksStatusMessageUpdated = ::GetTickCount64();

      DEBUG_MSG(DebugLevel::DevInfo, "Status message updated: '%s'", extended.mStatusMessage);
    }

    auto const pBegin = *mppMessageCenterMessages;
    if (pBegin == nullptr) {
      DEBUG_MSG(DebugLevel::DevInfo, "No message array pointer assigned: '%s'", extended.mStatusMessage);
      return true;  // Retry next time or fail?  Have counter for N failures?
    }

    char msgBuff[rF2Extended::MAX_STATUS_MSG_LEN];

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

        auto const pMsg = !seenSplit ? pCurr : msgBuff;

        if (strncmp(mPrevLastHistoryMessage, pMsg, rF2Extended::MAX_STATUS_MSG_LEN) != 0) {
          strcpy_s(extended.mLastHistoryMessage, pMsg);
          strcpy_s(mPrevLastHistoryMessage, extended.mLastHistoryMessage);
          extended.mTicksLastHistoryMessageUpdated = ::GetTickCount64();

          if (!seenSplit)
            DEBUG_MSG(DebugLevel::DevInfo, "Last history message updated: '%s'", extended.mLastHistoryMessage);
          else
            DEBUG_MSG(DebugLevel::DevInfo, "Last history message updated (concatenated): '%s'", extended.mLastHistoryMessage);
        }

        break;
      }

      pCurr -= 0xC0;
    }
  }
  __except (::GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
    DEBUG_MSG(DebugLevel::Errors, "Exception while reading memory, disabling DMA.");
    return false;
  }


  return true;
}

bool DirectMemoryReader::ReadOnNewSession(rF2Extended& extended)
{
  __try {
    if (mpStatusMessage == nullptr || mppMessageCenterMessages == nullptr || mpCurrPitSpeedLimit == nullptr) {
      assert(false && "DMR not available, should not call.");
      return false;
    }

    ClearLSIValues(extended);

    extended.mCurrentPitSpeedLimit = *mpCurrPitSpeedLimit;
    DEBUG_MSG(DebugLevel::DevInfo, "Current pit speed limit: %f", extended.mCurrentPitSpeedLimit);
  }
  __except (::GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
    DEBUG_MSG(DebugLevel::Errors, "Excepction while reading memory, disabling DMA.");
    return false;
  }

  return true;
}

bool DirectMemoryReader::ReadOnLSIVisible(rF2Extended& extended)
{
  __try {
    if (mpStatusMessage == nullptr || mppMessageCenterMessages == nullptr || mpCurrPitSpeedLimit == nullptr || mpLSIMessages == nullptr) {
      assert(false && "DMR not available, should not call.");
      return false;
    }

    auto const pPhase = mpLSIMessages + 0x50uLL;
    if (pPhase[0] != '\0'
      && strncmp(mPrevLSIPhaseMessage, pPhase, rF2Extended::MAX_RULES_INSTRUCTION_MSG_LEN) != 0) {
      strcpy_s(extended.mLSIPhaseMessage, pPhase);
      strcpy_s(mPrevLSIPhaseMessage, extended.mLSIPhaseMessage);
      extended.mTicksLSIPhaseMessageUpdated = ::GetTickCount64();

      DEBUG_MSG(DebugLevel::DevInfo, "LSI Phase message updated: '%s'", extended.mLSIPhaseMessage);
    }

    auto const pPitState = mpLSIMessages + 0xD0uLL;
    if (pPitState[0] != '\0'
      && strncmp(mPrevLSIPitStateMessage, pPitState, rF2Extended::MAX_RULES_INSTRUCTION_MSG_LEN) != 0) {
      strcpy_s(extended.mLSIPitStateMessage, pPitState);
      strcpy_s(mPrevLSIPitStateMessage, extended.mLSIPitStateMessage);
      extended.mTicksLSIPitStateMessageUpdated = ::GetTickCount64();

      DEBUG_MSG(DebugLevel::DevInfo, "LSI Pit State message updated: '%s'", extended.mLSIPitStateMessage);
    }

    auto const pOrderInstruction = mpLSIMessages + 0x150uLL;
    if (pOrderInstruction[0] != '\0'
     && strncmp(mPrevLSIOrderInstructionMessage, pOrderInstruction, rF2Extended::MAX_RULES_INSTRUCTION_MSG_LEN) != 0) {
      strcpy_s(extended.mLSIOrderInstructionMessage, pOrderInstruction);
      strcpy_s(mPrevLSIOrderInstructionMessage, extended.mLSIOrderInstructionMessage);
      extended.mTicksLSIOrderInstructionMessageUpdated = ::GetTickCount64();

      DEBUG_MSG(DebugLevel::DevInfo, "LSI Order Instruction message updated: '%s'", extended.mLSIOrderInstructionMessage);
    }

    auto const pRulesInstruction = mpLSIMessages + 0x1D0uLL;
    if (mSCRPluginEnabled
      && pRulesInstruction[0] != '\0'
      && strncmp(mPrevLSIRulesInstructionMessage, pRulesInstruction, rF2Extended::MAX_RULES_INSTRUCTION_MSG_LEN) != 0) {
      strcpy_s(extended.mLSIRulesInstructionMessage, pRulesInstruction);
      strcpy_s(mPrevLSIRulesInstructionMessage, extended.mLSIRulesInstructionMessage);
      extended.mTicksLSIRulesInstructionMessageUpdated = ::GetTickCount64();

      DEBUG_MSG(DebugLevel::DevInfo, "LSI Rules Instruction message updated: '%s'", extended.mLSIRulesInstructionMessage);
    }
  }
  __except (::GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
  {
    DEBUG_MSG(DebugLevel::Errors, "Exception while reading memory, disabling DMA.");
    return false;
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
  mSCRPluginDoubleFileType = -1L;

  return;
}

void DirectMemoryReader::ClearLSIValues(rF2Extended& extended)
{
  DEBUG_MSG(DebugLevel::DevInfo, "Clearing LSI values.");

  mPrevLSIPhaseMessage[0] = '\0';
  extended.mLSIPhaseMessage[0] = '\0';
  extended.mTicksLSIPhaseMessageUpdated = ::GetTickCount64();

  mPrevLSIPitStateMessage[0] = '\0';
  extended.mLSIPitStateMessage[0] = '\0';
  extended.mTicksLSIPitStateMessageUpdated = ::GetTickCount64();

  mPrevLSIOrderInstructionMessage[0] = '\0';
  extended.mLSIOrderInstructionMessage[0] = '\0';
  extended.mTicksLSIOrderInstructionMessageUpdated = ::GetTickCount64();

  mPrevLSIRulesInstructionMessage[0] = '\0';
  extended.mLSIRulesInstructionMessage[0] = '\0';
  extended.mTicksLSIRulesInstructionMessageUpdated = ::GetTickCount64();
}
