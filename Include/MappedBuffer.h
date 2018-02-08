/*
Definition of MappedDoubleBuffer<> class.

Author: The Iron Wolf (vleonavicius@hotmail.com)
Website: thecrewchief.org

Description:
  MappedDoubleBuffer<> class abstracts two memory mapped buffers that are used for reading
  and writing data in turns.  The idea is to allow clients to read one buffer
  (identified by mCurrentRead == true) while the other buffer is used for writing of the new data,
  when game reports it.  When buffers are flipped, mutex is acquired and mCurrentRead
  values are inverted for each buffer.

  Flip operation also supports retry mode, where flip is cancelled if mutex is signaled
  (to avoid blocking the thread).
*/
#pragma once

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

  bool Initialize()
  {
    assert(!mMapped);
    mhMap = MapMemoryFile(MM_FILE_NAME, mpMappedView, mpBuffVersionBlock, mpBuff);
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
    if (!mMapped) {
      return;
    }

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

    // Unmap views and close all handles.
    BOOL ret = TRUE;
    if (mpMappedView != nullptr) ret = ::UnmapViewOfFile(mpMappedView);
    if (!ret) {
      DEBUG_MSG(DebugLevel::Errors, "Failed to unmap mapped buffer view");
      SharedMemoryPlugin::TraceLastWin32Error();
    }

    mpMappedView = nullptr;
    mpBuff = nullptr;
    mpBuffVersionBlock = nullptr; 

    // Note: we didn't ever close this apparently before V3, oops.
    if (mhMap != nullptr && !::CloseHandle(mhMap)) {
      DEBUG_MSG(DebugLevel::Errors, "Failed to close mapped file handle.");
      SharedMemoryPlugin::TraceLastWin32Error();
    }

    mhMap = nullptr;
  }

private:
  MappedBuffer(MappedBuffer const&) = delete;
  MappedBuffer& operator=(MappedBuffer const&) = delete;

  HANDLE MapMemoryFile(char const* const fileName, LPVOID& pMappedView, rF2MappedBufferVersionBlock*& pBufVersionBlock, BuffT*& pBuf) const
  {
    char mappingName[256] = {};
    strcpy_s(mappingName, fileName);

    char moduleName[1024] = {};
    ::GetModuleFileNameA(nullptr, moduleName, sizeof(moduleName));

    char pid[8] = {};
    sprintf(pid, "%d", ::GetCurrentProcessId());

    // Append processId for dedicated server to allow multiple instances
    // TODO: Verify for rF2.
    if (strstr(moduleName, "Dedicated.exe") != nullptr)
      strcat(mappingName, pid);

    // Init handle and try to create, read if existing
    auto hMap = ::CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof(rF2MappedBufferVersionBlock) + sizeof(BuffT), mappingName);
    if (hMap == nullptr) {
      DEBUG_MSG2(DebugLevel::Errors, "Failed to create file mapping for file:", mappingName);
      SharedMemoryPlugin::TraceLastWin32Error();
      return nullptr;
    }
    
    if (::GetLastError() == ERROR_ALREADY_EXISTS)
      DEBUG_MSG2(DebugLevel::Warnings, "WARNING: File mapping already exists for file:", mappingName);

    pMappedView = ::MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0 /*dwFileOffsetHigh*/, 0 /*dwFileOffsetLow*/, sizeof(rF2MappedBufferVersionBlock) + sizeof(BuffT));
    if (pMappedView == nullptr) {
      DEBUG_MSG(DebugLevel::Errors, "Failed to map buffer view.");
      SharedMemoryPlugin::TraceLastWin32Error();
      return nullptr;
    }

    // Map the version block first.
    pBufVersionBlock = static_cast<rF2MappedBufferVersionBlock*>(mpMappedView);
    pBuf = reinterpret_cast<BuffT*>(static_cast<char*>(pMappedView) + sizeof(pBufVersionBlock));

    return hMap;
  }

  public:
    rF2MappedBufferVersionBlock* mpBuffVersionBlock = nullptr;
    BuffT* mpBuff = nullptr;

  private:
    LPVOID mpMappedView = nullptr;

  private:
    char const* const MM_FILE_NAME = nullptr;
    HANDLE mhMap = nullptr;
    bool mMapped = false;
};