#include <windows.h>
#include <psapi.h>
#include "Utils.h"
#include "rFactor2SharedMemoryMap.hpp"

namespace Utils
{
uintptr_t* FindPatternForPointerInMemory(HMODULE module, unsigned char const* pattern, char const* mask, size_t bytedIntoPatternToFindOffset)
{
  MODULEINFO info = {};
  ::GetModuleInformation(::GetCurrentProcess(), module, &info, sizeof(MODULEINFO));
  auto addressAbsoluteRIP = FindPattern(reinterpret_cast<uintptr_t>(module), info.SizeOfImage, pattern, mask);
  if (addressAbsoluteRIP == 0uLL)
    return nullptr;

  addressAbsoluteRIP += bytedIntoPatternToFindOffset;
  auto const offsetFromRIP = LODWORD(*reinterpret_cast<uintptr_t*>(addressAbsoluteRIP) + 4uLL);
  return reinterpret_cast<uintptr_t*>(addressAbsoluteRIP + offsetFromRIP);
}


uintptr_t FindPattern(uintptr_t start, size_t length, unsigned char const* pattern, char const* mask)
{
  size_t maskPos = 0u;
  auto const maskLength = strlen(reinterpret_cast<char const*>(mask)) - 1;

  auto startAdress = start;
  for (auto currAddress = startAdress; currAddress < startAdress + length; ++currAddress) {
    if (*reinterpret_cast<unsigned char*>(currAddress) == pattern[maskPos] || mask[maskPos] == '?') {
      if (mask[maskPos + 1u] == '\0')
         return currAddress - maskLength;

      ++maskPos;
    } else
      maskPos = 0;
  }

  return 0uLL;
}


char* GetFileContents(char const* const filePath)
{
  FILE* fileHandle = nullptr;

  auto onExit = MakeScopeGuard(
    [&]() {
    if (fileHandle != nullptr) {
      auto ret = fclose(fileHandle);
      if (ret != 0)
        DEBUG_MSG(DebugLevel::Errors, DebugSource::General, "fclose() failed with: %d", ret);
    }
  });

  char* fileContents = nullptr;
  auto ret = fopen_s(&fileHandle, filePath, "rb");
  if (ret != 0) {
    DEBUG_MSG(DebugLevel::Errors, DebugSource::General, "fopen_s() failed with: %d", ret);
    return nullptr;
  }

  ret = fseek(fileHandle, 0, SEEK_END);
  if (ret != 0) {
    DEBUG_MSG(DebugLevel::Errors, DebugSource::General, "fseek() failed with: %d", ret);
    return nullptr;
  }

  auto const fileBytes = static_cast<size_t>(ftell(fileHandle));
  rewind(fileHandle);

  fileContents = new char[fileBytes + 1];
  auto elemsRead = fread(fileContents, fileBytes, 1 /*items*/, fileHandle);
  if (elemsRead != 1 /*items*/) {
    delete[] fileContents;
    fileContents = nullptr;
    DEBUG_MSG(DebugLevel::Errors, DebugSource::General, "fread() failed.");
    return nullptr;
  }

  fileContents[fileBytes] = 0;

  return fileContents;
}


}