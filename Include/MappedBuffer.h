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

  // Write buffer constructor.
  MappedBuffer(char const* mmFileName)
    : MM_FILE_NAME(mmFileName)
    , READ_BUFFER_SUPPORTED_LAYOUT_VERSION(0L)
  {}

  // Read buffer constructor.
  MappedBuffer(char const* mmFileName, long mLayoutVersion)
    : MM_FILE_NAME(mmFileName)
    , READ_BUFFER_SUPPORTED_LAYOUT_VERSION(mLayoutVersion)
  {
    memset(&mReadBuff, 0, sizeof(BuffT));
  }

  ~MappedBuffer()
  {
    ReleaseResources();
  }

  bool Initialize(bool mapGlobally)
  {
    assert(!mMapped);
    mhMap = MapMemoryFile(MM_FILE_NAME, mapGlobally, mpMappedView, mpWriteBuffVersionBlock, mpWriteBuff);
    if (mhMap == nullptr) {
      DEBUG_MSG(DebugLevel::Errors, DebugSource::General, "Failed to map file");
      ReleaseResources();

      return false;
    }

    assert(mpMappedView != nullptr);
    assert(mpWriteBuffVersionBlock != nullptr);
    assert(mpWriteBuff != nullptr);

    // Minimal risk here that this will get accessed before mMapped == true, but who cares.
    memset(mpMappedView, 0, sizeof(rF2MappedBufferVersionBlock) + sizeof(BuffT));

    mMapped = true;

    return true;
  }

  void BeginUpdate()
  {
    if (!mMapped) {
      DEBUG_MSG(DebugLevel::Errors, DebugSource::General, "Accessing unmapped buffer.");
      return;
    }

    if (READ_BUFFER_SUPPORTED_LAYOUT_VERSION != 0L) {
      DEBUG_MSG(DebugLevel::Warnings, DebugSource::General, "BeginUpdate: skipping as it does not apply to Read buffer");
      return;
    }

    // Fix up out of sync situation.
    if (mpWriteBuffVersionBlock->mVersionUpdateBegin != mpWriteBuffVersionBlock->mVersionUpdateEnd) {
      if (Utils::IsFlagOn(SharedMemoryPlugin::msDebugOutputLevel, DebugLevel::Synchronization)) {
        DEBUG_MSG(DebugLevel::Synchronization, DebugSource::MappedBufferSource, "BeginUpdate: versions out of sync.  Version Begin:%ld  End:%ld",
          mpWriteBuffVersionBlock->mVersionUpdateBegin, mpWriteBuffVersionBlock->mVersionUpdateEnd);
      }
      ::InterlockedExchange(&mpWriteBuffVersionBlock->mVersionUpdateEnd, mpWriteBuffVersionBlock->mVersionUpdateBegin);
    }

    ::InterlockedIncrement(&mpWriteBuffVersionBlock->mVersionUpdateBegin);
  }

  void EndUpdate()
  {
    if (!mMapped) {
      DEBUG_MSG(DebugLevel::Errors, DebugSource::General, "Accessing unmapped buffer.");
      return;
    }

    if (READ_BUFFER_SUPPORTED_LAYOUT_VERSION != 0L) {
      DEBUG_MSG(DebugLevel::Warnings, DebugSource::General, "EndUpdate: skipping as it does not apply to Read buffer");
      return;
    }

    ::InterlockedIncrement(&mpWriteBuffVersionBlock->mVersionUpdateEnd);

    // Fix up out of sync situation.
    if (mpWriteBuffVersionBlock->mVersionUpdateBegin != mpWriteBuffVersionBlock->mVersionUpdateEnd) {
      if (Utils::IsFlagOn(SharedMemoryPlugin::msDebugOutputLevel, DebugLevel::Synchronization)) {
        DEBUG_MSG(DebugLevel::Synchronization, DebugSource::MappedBufferSource, "EndUpdate: versions out of sync.  Version Begin:%ld  End:%ld",
          mpWriteBuffVersionBlock->mVersionUpdateBegin, mpWriteBuffVersionBlock->mVersionUpdateEnd);
      }
      ::InterlockedExchange(&mpWriteBuffVersionBlock->mVersionUpdateBegin, mpWriteBuffVersionBlock->mVersionUpdateEnd);
    }
  }

  void ClearState(BuffT const* pInitialContents)
  {
    if (!mMapped)
      return;

    if (READ_BUFFER_SUPPORTED_LAYOUT_VERSION != 0L) {
      DEBUG_MSG(DebugLevel::Warnings, DebugSource::General, "ClearState: skipping as it does not apply to Read buffer");
      return;
    }

    BeginUpdate();

    if (pInitialContents != nullptr)
      memcpy(mpWriteBuff, pInitialContents, sizeof(BuffT));
    else
      memset(mpWriteBuff, 0, sizeof(BuffT));

    EndUpdate();
  }

  /////////////////////////////////////////////////////////////////
  // Read buffer support

  bool VerifyBusyOrUnchanged(unsigned long versionUpdateBegin, unsigned long versionUpdateEnd)
  {
    // Check busy or out of sync situation.
    if (versionUpdateBegin != versionUpdateEnd) {
      if (Utils::IsFlagOn(SharedMemoryPlugin::msDebugOutputLevel, DebugLevel::Synchronization)) {
        DEBUG_MSG(DebugLevel::Synchronization, DebugSource::MappedBufferSource, "VerifyBusyOrUnchanged: versions out of sync.  Version Begin:%ld  End:%ld",
          versionUpdateBegin, versionUpdateEnd);
      }
      return true;
    }
    
    // Is it new?
    if (mReadLastVersionUpdateBegin != versionUpdateBegin)
      return false;
   
    return true;
  }

  // Returns true if buffer is valid and updated since last read.
  bool ReadUpdate()
  {
    if (!mMapped) {
      DEBUG_MSG(DebugLevel::Errors, DebugSource::General, "Accessing unmapped buffer.");
      return false;
    }

    rF2MappedBufferVersionBlock versionBegin;
    memcpy(&versionBegin, mpWriteBuffVersionBlock, sizeof(rF2MappedBufferVersionBlock));

    if (VerifyBusyOrUnchanged(versionBegin.mVersionUpdateBegin, versionBegin.mVersionUpdateEnd)) {
      DEBUG_MSG(DebugLevel::Synchronization, DebugSource::MappedBufferSource, "Skipping read buffer update.");
      return false;
    }

    memcpy(&mReadBuff, mpWriteBuff, sizeof(BuffT));

    rF2MappedBufferVersionBlock versionEnd;
    memcpy(&versionEnd, mpWriteBuffVersionBlock, sizeof(rF2MappedBufferVersionBlock));

    if (VerifyBusyOrUnchanged(versionBegin.mVersionUpdateBegin, versionEnd.mVersionUpdateEnd))
      return false;

    mReadLastVersionUpdateBegin = versionBegin.mVersionUpdateBegin;
    return true;
  }


  void ReleaseResources()
  {
    mMapped = false;

    // Unmap view and close all handles.
    if (mpMappedView != nullptr
      && !::UnmapViewOfFile(mpMappedView)) {
      DEBUG_MSG(DebugLevel::Errors, DebugSource::General, "Failed to unmap mapped buffer view");
      SharedMemoryPlugin::TraceLastWin32Error();
    }

    mpMappedView = nullptr;
    mpWriteBuff = nullptr;
    mpWriteBuffVersionBlock = nullptr;

    // Note: we didn't ever close this apparently before V3, oops.
    if (mhMap != nullptr
      && !::CloseHandle(mhMap)) {
      DEBUG_MSG(DebugLevel::Errors, DebugSource::General, "Failed to close mapped file handle.");
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
    auto isDedicatedServer = false;
    if (strstr(moduleName, "Dedicated.exe") == nullptr)
      strcpy_s(mappingName, fileName);  // Regular client use.
    else {
      // Dedicated server use.  Append processId for dedicated server to allow multiple instances.
      char pid[8] = {};
      sprintf(pid, "%ld", ::GetCurrentProcessId());

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

      auto ret = ConvertStringSecurityDescriptorToSecurityDescriptor(
        "D:P(A;OICI;GA;;;SY)(A;OICI;GA;;;BA)(A;OICI;GA;;;WD)",
        SDDL_REVISION_1,
        &security.lpSecurityDescriptor,
        nullptr);

      auto onExit = Utils::MakeScopeGuard([&]() {
        ::LocalFree(security.lpSecurityDescriptor);
      });

      if (!ret) {
        DEBUG_MSG(DebugLevel::Errors, DebugSource::General, "Failed to create security descriptor for mapping: '%s'", mappingName);
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
      DEBUG_MSG(DebugLevel::Errors, DebugSource::General, "Failed to create file mapping for file: '%s'", mappingName);
      SharedMemoryPlugin::TraceLastWin32Error();
      return nullptr;
    }

    if (::GetLastError() == ERROR_ALREADY_EXISTS)
      DEBUG_MSG(DebugLevel::Warnings, DebugSource::General, "File mapping already exists for file: '%s'", mappingName);

    pMappedView = ::MapViewOfFile(
      hMap,
      FILE_MAP_ALL_ACCESS,
      0 /*dwFileOffsetHigh*/,
      0 /*dwFileOffsetLow*/,
      sizeof(rF2MappedBufferVersionBlock) + sizeof(BuffT));

    if (pMappedView == nullptr) {
      DEBUG_MSG(DebugLevel::Errors, DebugSource::General, "Failed to map buffer view.");
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
    typedef BuffT BufferType;

    rF2MappedBufferVersionBlock* mpWriteBuffVersionBlock = nullptr;
    BuffT* mpWriteBuff = nullptr;

    // TODO: re-think.
    // Read buffer support.  Read buffer support is hacked up incorrectly, I will revisit it at some point.

    // Local read buffer copy:
    BuffT mReadBuff;

    unsigned long mReadLastVersionUpdateBegin = 0uL;

  private:
    LPVOID mpMappedView = nullptr;
    char const* const MM_FILE_NAME = nullptr;
    HANDLE mhMap = nullptr;
    bool mMapped = false;

    // If 0, it means this is write mode buffer.
    long const READ_BUFFER_SUPPORTED_LAYOUT_VERSION;
};