/*
Various Utilities, helpers etc.

Author: The Iron Wolf (vleonavicius@hotmail.com)
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
