/*
Definition of MappedBuffer<> class.

Author: The Iron Wolf (vleonavicius@hotmail.com)
Website: thecrewchief.org

Description:
  MappedBuffer<> class abstracts memory mapped buffer and version block that can be used
  to detect torn frames.  In mapped memory, BuffT is preceeded by rF2MappedBufferVersionBlock.

  rF2MappedBufferVersionBlock variables allow users who need to ensure consistent buffer view to check if
  buffer is being written to.  mVersionUpdateBegin and mVersionUpdateEnd version block variables should 
  be equal and differ only while buffer (mpBuff) contents are updated.

  MappedBuffer<> client code has to call BeginUpdate before updating buffer (mpBuff) contents and
  EndUpdate once they're done.
*/
#pragma once
#include <sddl.h>
#include "Utils.h"

template <typename BuffT>
class MappedBuffer
{
public:

  MappedBuffer(char const* mmFileName) 
    : MM_FILE_NAME(mmFileName)
  {}

  ~MappedBuffer()
  {
    ReleaseResources();
  }

  bool Initialize(bool mapGlobally)
  {
    assert(!mMapped);
    mhMap = MapMemoryFile(MM_FILE_NAME, mapGlobally, mpMappedView, mpBuffVersionBlock, mpBuff);
    if (mhMap == nullptr) {
      DEBUG_MSG(DebugLevel::Errors, "Failed to map file");
      ReleaseResources();

      return false;
    }

    assert(mpMappedView != nullptr);
    assert(mpBuffVersionBlock != nullptr);
    assert(mpBuff != nullptr);

    // Minimal risk here that this will get accessed before mMapped == true, but who cares.
    memset(mpMappedView, 0, sizeof(rF2MappedBufferVersionBlock) + sizeof(BuffT));

    mMapped = true;

    return true;
  }

  void BeginUpdate()
  {
    if (!mMapped) {
      DEBUG_MSG(DebugLevel::Errors, "Accessing unmapped buffer.");
      return;
    }

    // Fix up out of sync situation.
    if (mpBuffVersionBlock->mVersionUpdateBegin != mpBuffVersionBlock->mVersionUpdateEnd) {
      if (SharedMemoryPlugin::msDebugOutputLevel >= DebugLevel::Synchronization) {
        char msg[512] = {};

        sprintf(msg, "BeginUpdate: versions out of sync.  Version Begin:%d  End:%d",
          mpBuffVersionBlock->mVersionUpdateBegin, mpBuffVersionBlock->mVersionUpdateEnd);

        DEBUG_MSG(DebugLevel::Synchronization, msg);
      }
      ::InterlockedExchange(&mpBuffVersionBlock->mVersionUpdateEnd, mpBuffVersionBlock->mVersionUpdateBegin);
    }

    ::InterlockedIncrement(&mpBuffVersionBlock->mVersionUpdateBegin);
  }

  void EndUpdate()
  {
    if (!mMapped) {
      DEBUG_MSG(DebugLevel::Errors, "Accessing unmapped buffer.");
      return;
    }

    ::InterlockedIncrement(&mpBuffVersionBlock->mVersionUpdateEnd);

    // Fix up out of sync situation.
    if (mpBuffVersionBlock->mVersionUpdateBegin != mpBuffVersionBlock->mVersionUpdateEnd) {
      if (SharedMemoryPlugin::msDebugOutputLevel >= DebugLevel::Synchronization) {
        char msg[512] = {};

        sprintf(msg, "EndUpdate: versions out of sync.  Version Begin:%d  End:%d",
          mpBuffVersionBlock->mVersionUpdateBegin, mpBuffVersionBlock->mVersionUpdateEnd);

        DEBUG_MSG(DebugLevel::Synchronization, msg);
      }
      ::InterlockedExchange(&mpBuffVersionBlock->mVersionUpdateBegin, mpBuffVersionBlock->mVersionUpdateEnd);
    }
  }

  void ClearState(BuffT const* pInitialContents)
  {
    if (!mMapped)
      return;

    BeginUpdate();

    if (pInitialContents != nullptr)
      memcpy(mpBuff, pInitialContents, sizeof(BuffT));
    else
      memset(mpBuff, 0, sizeof(BuffT));

    EndUpdate();
  }

  void ReleaseResources()
  {
    mMapped = false;

    // Unmap view and close all handles.
    if (mpMappedView != nullptr 
      && !::UnmapViewOfFile(mpMappedView)) {
      DEBUG_MSG(DebugLevel::Errors, "Failed to unmap mapped buffer view");
      SharedMemoryPlugin::TraceLastWin32Error();
    }

    mpMappedView = nullptr;
    mpBuff = nullptr;
    mpBuffVersionBlock = nullptr; 

    // Note: we didn't ever close this apparently before V3, oops.
    if (mhMap != nullptr
      && !::CloseHandle(mhMap)) {
      DEBUG_MSG(DebugLevel::Errors, "Failed to close mapped file handle.");
      SharedMemoryPlugin::TraceLastWin32Error();
    }

    mhMap = nullptr;
  }

private:
  MappedBuffer(MappedBuffer const&) = delete;
  MappedBuffer& operator=(MappedBuffer const&) = delete;

  HANDLE MapMemoryFile(char const* const fileName, bool dedicatedServerMapGlobally, LPVOID& pMappedView, rF2MappedBufferVersionBlock*& pBufVersionBlock, BuffT*& pBuf) const
  {
    char moduleName[1024] = {};
    ::GetModuleFileNameA(nullptr, moduleName, sizeof(moduleName));

    char mappingName[MAX_PATH] = {};
    auto isDedicatedServer = true;
    if (strstr(moduleName, "Dedicated.exe") == nullptr)
      strcpy_s(mappingName, fileName);  // Regular client use.
    else {
      // Dedicated server use.  Append processId for dedicated server to allow multiple instances.
      char pid[8] = {};
      sprintf(pid, "%d", ::GetCurrentProcessId());

      if (dedicatedServerMapGlobally)
        sprintf(mappingName, "Global\\%s%s", fileName, pid);
      else
        sprintf(mappingName, "%s%s", fileName, pid);

      isDedicatedServer = true;
    }

    HANDLE hMap = INVALID_HANDLE_VALUE;
    if (!isDedicatedServer  // Regular client use.
      || (isDedicatedServer && !dedicatedServerMapGlobally)) {  // Dedicated, but no global mapping requested.
      // Init handle and try to create, read if existing.
      hMap = ::CreateFileMappingA(
        INVALID_HANDLE_VALUE,
        nullptr  /*lpFileMappingAttributes*/,
        PAGE_READWRITE,
        0  /*dwMaximumSizeLow*/,
        sizeof(rF2MappedBufferVersionBlock) + sizeof(BuffT),
        mappingName);
    }
    else {
      assert(isDedicatedServer);
      assert(dedicatedServerMapGlobally);

      SECURITY_ATTRIBUTES security = {};
      security.nLength = sizeof(security);

      /*SECURITY_ATTRIBUTES security = {};
      SECURITY_DESCRIPTOR secDesc = {};

      if (::InitializeSecurityDescriptor(&secDesc, SECURITY_DESCRIPTOR_REVISION)
        && ::SetSecurityDescriptorDacl(&secDesc, TRUE, static_cast<PACL>(0), FALSE))
      {
        security.nLength = sizeof(security);
        security.lpSecurityDescriptor = &secDesc;
        security.bInheritHandle = TRUE;
      }
      else {
          DEBUG_MSG2(DebugLevel::Errors, "Failed to create security descriptor for mapping:", mappingName);
          SharedMemoryPlugin::TraceLastWin32Error();
          ::LocalFree(security.lpSecurityDescriptor);
          return nullptr;
      }*/

      auto ret = ConvertStringSecurityDescriptorToSecurityDescriptor(
        "D:P(A;OICI;GA;;;SY)(A;OICI;GA;;;BA)(A;OICI;GA;;;WD)",
        SDDL_REVISION_1,
        &security.lpSecurityDescriptor,
        nullptr);

      auto onExit = MakeScopeGuard([&]() {
        ::LocalFree(security.lpSecurityDescriptor);
      });

      if (!ret) {
        DEBUG_MSG2(DebugLevel::Errors, "Failed to create security descriptor for mapping:", mappingName);
        SharedMemoryPlugin::TraceLastWin32Error();
        return nullptr;
      }

      // Init handle and try to create, read if existing
      hMap = ::CreateFileMappingA(
        INVALID_HANDLE_VALUE,
        &security,
        PAGE_READWRITE,
        0  /*dwMaximumSizeLow*/,
        sizeof(rF2MappedBufferVersionBlock) + sizeof(BuffT),
        mappingName);
    }
    
    if (hMap == nullptr) {
      DEBUG_MSG2(DebugLevel::Errors, "Failed to create file mapping for file:", mappingName);
      SharedMemoryPlugin::TraceLastWin32Error();
      return nullptr;
    }
    
    if (::GetLastError() == ERROR_ALREADY_EXISTS)
      DEBUG_MSG2(DebugLevel::Warnings, "WARNING: File mapping already exists for file:", mappingName);

    pMappedView = ::MapViewOfFile(
      hMap,
      FILE_MAP_ALL_ACCESS,
      0 /*dwFileOffsetHigh*/,
      0 /*dwFileOffsetLow*/,
      sizeof(rF2MappedBufferVersionBlock) + sizeof(BuffT));

    if (pMappedView == nullptr) {
      DEBUG_MSG(DebugLevel::Errors, "Failed to map buffer view.");
      SharedMemoryPlugin::TraceLastWin32Error();
      return nullptr;
    }

    // Set pointers up.
    pBufVersionBlock = static_cast<rF2MappedBufferVersionBlock*>(mpMappedView);
    pBuf = reinterpret_cast<BuffT*>(static_cast<char*>(pMappedView) + sizeof(pBufVersionBlock));
    assert((reinterpret_cast<char*>(pBufVersionBlock) + sizeof(pBufVersionBlock)) == reinterpret_cast<char*>(pBuf));

    return hMap;
  }

  public:
    rF2MappedBufferVersionBlock* mpBuffVersionBlock = nullptr;
    BuffT* mpBuff = nullptr;

  private:
    LPVOID mpMappedView = nullptr;
    char const* const MM_FILE_NAME = nullptr;
    HANDLE mhMap = nullptr;
    bool mMapped = false;
};