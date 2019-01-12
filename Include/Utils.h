/*
Various Utilities, helpers etc.

Authors: The Iron Wolf (vleonavicius@hotmail.com)
         Sparten (Morten Roslev) Pattern Finder code

Website: thecrewchief.org
*/

#pragma once

// Lazy man's ScopeGuard.
template <typename Lambda>
class ScopeGuard
{
public:
  ScopeGuard(Lambda const& scopeExitLambda) : mScopeExitLambda(scopeExitLambda)
  {}

  ~ScopeGuard()
  {
    if (!mCommitted)
      mScopeExitLambda();
  }

  void Dismiss()
  {
    mCommitted = true;
  }

private:
  bool mCommitted = false;
  Lambda mScopeExitLambda;
};

template <typename Lambda>
ScopeGuard<Lambda> MakeScopeGuard(Lambda const& l)
{
  return ScopeGuard<Lambda>(l);
};

#define LOWORD(_dw)     ((WORD)(((DWORD_PTR)(_dw)) & 0xffff))
#define HIWORD(_dw)     ((WORD)((((DWORD_PTR)(_dw)) >> 16) & 0xffff))
#define LODWORD(_qw)    ((DWORD)(_qw))
#define HIDWORD(_qw)    ((DWORD)(((_qw) >> 32) & 0xffffffff))

/// <summary>
/// Searches for the first pattern in the memory region.
/// </summary>
/// <param name="start">The start address of the memory region to scan.</param>
/// <param name="length">The length of the memory region.</param>
/// <param name="pattern">The pattern (Example: "\x12\xAB\x34")</param>
/// <param name="mask">The mask (Example: "x?x")</param>
/// <returns>The address of the found pattern or -1 if the pattern was not found.</returns>
uintptr_t FindPattern(uintptr_t start, size_t length, unsigned const char* pattern, char const* mask);

/// <summary>
/// Searches for the first pattern in the module.
/// </summary>
/// <param name="module">The module to scan.</param>
/// <param name="pattern">The pattern (Example: "\x12\xAB\x34")</param>
/// <param name="mask">The mask (Example: "x?x")</param>
/// <param name="bytedIntoPatternToFindOffset">nr bytes into found address to add to get the pointer offset (Example: -?+?)</param>
/// <returns>The address of the found pattern or -1 if the pattern was not found.</returns>
uintptr_t* FindPatternForPointerInMemory(HMODULE module, unsigned const char* pattern, char const* mask, int bytedIntoPatternToFindPointer);