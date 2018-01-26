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
    : MM_FILE_NAME(mmFileName1)
  {}

  ~MappedBuffer()
  {
    ReleaseResources();
  }

  bool Initialize()
  {
    assert(!mMapped);
    mhMap = MapMemoryFile(MM_FILE_NAME, mpBuff);
    if (mhMap == nullptr) {
      DEBUG_MSG(DebugLevel::Errors, "Failed to map file");
      return false;
    }

    mMapped = true;

    return true;
  }

  void ClearState(BuffT const* pInitialContents)
  {
    if (!mMapped) {
      assert(mMapped);
      DEBUG_MSG(DebugLevel::Errors, "Accessing unmapped buffer.");
      return;
    }

    // TODO: Exchange or increment?
    ::InterlockedExchange(&mpBuff->mVersionUpdateBegin, 0);

    if (pInitialContents != nullptr)
      memcpy(mpBuff, pInitialContents, sizeof(BuffT));
    else
      memset(mpBuff, 0, sizeof(BuffT));

    ::InterlockedExchange(&mpBuff->mVersionUpdateEnd, 0);
  }

  void ReleaseResources()
  {
    mMapped = false;

    // Unmap views and close all handles.
    BOOL ret = TRUE;
    if (mpBuff != nullptr) ret = ::UnmapViewOfFile(mpBuff);
    if (!ret) {
      DEBUG_MSG(DebugLevel::Errors, "Failed to unmap buffer");
      SharedMemoryPlugin::TraceLastWin32Error();
    }

    mpBuff = nullptr;
  }
  /*
  void FlipBuffersHelper()
  {
    if (!mMapped) {
      assert(mMapped);
      DEBUG_MSG(DebugLevel::Errors, "Accessing unmapped buffer.");
      return;
    }

    // Handle fucked up case:
    if (mpBuff->mCurrentRead == mpBuff2->mCurrentRead) {
      mpBuff->mCurrentRead = true;
      mpBuff2->mCurrentRead = false;
      DEBUG_MSG(DebugLevel::Errors, "ERROR: - Buffers out of sync.");
    }

    // Update read buffer.
    assert(mpCurrReadBuff->mCurrentRead);
    assert(!mpCurrWriteBuff->mCurrentRead);
    mpCurrReadBuff = mpCurrWriteBuff;

    // Pick previous read buffer.
    mpCurrWriteBuff = mpBuff->mCurrentRead ? mpBuff : mpBuff2;

    // Switch the read and write buffers.
    mpBuff->mCurrentRead = !mpBuff->mCurrentRead;
    mpBuff2->mCurrentRead = !mpBuff2->mCurrentRead;

    assert(!mpCurrWriteBuff->mCurrentRead);
    assert(mpCurrReadBuff->mCurrentRead);
  }


  void FlipBuffers()
  {
    if (!mMapped) {
      assert(mMapped);
      DEBUG_MSG(DebugLevel::Errors, "Accessing unmapped buffer.");
      return;
    }

    // This update will wait.  Clear the retry variables.
    mRetryPending = false;
    mAsyncRetriesLeft = MAX_RETRIES;

    auto const ret = ::WaitForSingleObject(mhMutex, SharedMemoryPlugin::msMillisMutexWait);

    FlipBuffersHelper();

    if (ret == WAIT_OBJECT_0) {
      if (!::ReleaseMutex(mhMutex)) {
        DEBUG_MSG(DebugLevel::Errors, "Failed to release mutex.");
        SharedMemoryPlugin::TraceLastWin32Error();
      }
    }
    else if (ret == WAIT_TIMEOUT)
      DEBUG_MSG(DebugLevel::Warnings, "WARNING: - Timed out while waiting on mutex.");
    else {
      DEBUG_MSG(DebugLevel::Errors, "ERROR: - wait on mutex failed.");
      SharedMemoryPlugin::TraceLastWin32Error();
    }
  }

  void TryFlipBuffers()
  {
    if (!mMapped) {
      assert(mMapped);
      DEBUG_MSG(DebugLevel::Errors, "Accessing unmapped buffer.");
      return;
    }

    // Do not wait on mutex if it is held.
    auto const ret = ::WaitForSingleObject(mhMutex, 0);
    if (ret == WAIT_TIMEOUT) {
      mRetryPending = true;
      --mAsyncRetriesLeft;
      return;
    }

    // We have the lock.  Clear retry variables.
    mRetryPending = false;
    mAsyncRetriesLeft = MAX_RETRIES;

    // Do the actual flip.
    FlipBuffersHelper();

    if (ret == WAIT_OBJECT_0) {
      if (!::ReleaseMutex(mhMutex)) {
        DEBUG_MSG(DebugLevel::Errors, "Failed to release mutex.");
        SharedMemoryPlugin::TraceLastWin32Error();
      }
    }
  }

  int AsyncRetriesLeft() const { return mAsyncRetriesLeft; }
  int RetryPending() const { return mRetryPending; }*/

private:
  MappedBuffer(MappedBuffer const&) = delete;
  MappedBuffer& operator=(MappedBuffer const&) = delete;

  HANDLE MapMemoryFile(char const* const fileName, BuffT*& pBuf) const
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
    auto hMap = ::CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof(BuffT), mappingName);
    if (hMap == nullptr) {
      DEBUG_MSG2(DebugLevel::Errors, "Failed to create file mapping for file:", mappingName);
      SharedMemoryPlugin::TraceLastWin32Error();
      return nullptr;
    }
    
    if (::GetLastError() == ERROR_ALREADY_EXISTS)
      DEBUG_MSG2(DebugLevel::Warnings, "WARNING: File mapping already exists for file:", mappingName);

    pBuf = static_cast<BuffT*>(::MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(BuffT)));
    if (pBuf == nullptr) {
      SharedMemoryPlugin::TraceLastWin32Error();

      // Failed to map memory buffer.
      if (!::CloseHandle(hMap)) {
        DEBUG_MSG(DebugLevel::Errors, "Failed to close mapped file handle.");
        SharedMemoryPlugin::TraceLastWin32Error();
      }

      return nullptr;
    }

    return hMap;
  }

  public:
    BuffT* mpBuff = nullptr;

  private:
    char const* const MM_FILE_NAME;
    HANDLE mhMap = nullptr;
    bool mMapped = false;
};