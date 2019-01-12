#include <windows.h>
#include "Utils.h"
#include <psapi.h>

uintptr_t* FindPatternForPointerInMemory(HMODULE module, unsigned const char* pattern, char const* mask, int bytedIntoPatternToFindOffset)
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


uintptr_t FindPattern(uintptr_t start, size_t length, unsigned const char* pattern, char const* mask)
{
  size_t maskPos = 0u;
  auto const maskLength = strlen(mask) - 1;

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
