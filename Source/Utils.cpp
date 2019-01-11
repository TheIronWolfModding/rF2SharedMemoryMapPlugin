#include <windows.h>
#include "Utils.h"

#include <psapi.h>

uintptr_t FindPattern(HMODULE module, unsigned const char* pattern, char const* mask)
{
  MODULEINFO info = {};
  ::GetModuleInformation(::GetCurrentProcess(), module, &info, sizeof(MODULEINFO));

  return FindPattern(reinterpret_cast<uintptr_t>(module), info.SizeOfImage, pattern, mask);
}

uintptr_t* FindPatternForPointerInMemory(HMODULE module, unsigned const char* pattern, char const* mask, int bytedIntoPatternToFindOffset)
{
  MODULEINFO info = {};
  ::GetModuleInformation(GetCurrentProcess(), module, &info, sizeof(MODULEINFO));
  auto address = FindPattern(reinterpret_cast<uintptr_t>(module), info.SizeOfImage, pattern, mask);
  if (address == 0uLL)
    return nullptr;

  address += bytedIntoPatternToFindOffset;
  return reinterpret_cast<uintptr_t*>(address + LODWORD(*(uintptr_t*) address + 4));
}

uintptr_t FindPattern(uintptr_t start, size_t length, unsigned const char* pattern, char const* mask)
{
  size_t pos = 0u;
  auto maskLength = strlen(mask) - 1;

  auto startAdress = start;
  for (auto it = startAdress; it < startAdress + length; ++it) {
    if (*reinterpret_cast<unsigned char*>(it) == pattern[pos] || mask[pos] == '?') {
      if (mask[pos + 1] == '\0')
        return it - maskLength;

      pos++;
    } else
      pos = 0;
  }

  return 0uLL;
}
