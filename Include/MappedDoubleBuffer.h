#pragma once

template <typename BuffT>
class MappedDoubleBuffer
{
public:

  MappedDoubleBuffer(
    int maxRetries
    , char const* mmFileName1
    , char const* mmFileName2
    , char const* mmMutexName) 
    : MAX_RETRIES(maxRetries)
    , MM_FILE_NAME1(mmFileName1)
    , MM_FILE_NAME2(mmFileName2)
    , MM_FILE_ACCESS_MUTEX(mmMutexName)
  {}

  ~MappedDoubleBuffer()
  {
    ReleaseResources();
  }

  bool Initialize()
  {
    assert(!mMapped);
    mhMap1 = MapMemoryFile(MM_FILE_NAME1, mpBuf1);
    if (mhMap1 == nullptr) {
      DEBUG_MSG(DebugLevel::Errors, "Failed to map file 1");
      return false;
    }

    mhMap2 = MapMemoryFile(MM_FILE_NAME2, mpBuf2);
    if (mhMap2 == nullptr) {
      DEBUG_MSG(DebugLevel::Errors, "Failed to map file 2");
      return false;
    }

    mhMutex = CreateMutex(nullptr, FALSE, MM_FILE_ACCESS_MUTEX);
    if (mhMutex == nullptr) {
      DEBUG_MSG(DebugLevel::Errors, "Failed to create mutex");
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

    mRetryPending = false;
    mAsyncRetriesLeft = MAX_RETRIES;

    auto ret = WaitForSingleObject(mhMutex, SharedMemoryPlugin::msMillisMutexWait);

    if (pInitialContents != nullptr) {
      memcpy(mpBuf1, pInitialContents, sizeof(BuffT));
      memcpy(mpBuf2, pInitialContents, sizeof(BuffT));
    }
    else {
      memset(mpBuf1, 0, sizeof(BuffT));
      memset(mpBuf2, 0, sizeof(BuffT));
    }

    mpBuf1->mCurrentRead = true;
    mpBuf2->mCurrentRead = false;

    mpCurReadBuf = mpBuf1;
    mpCurWriteBuf = mpBuf2;
    assert(mpCurReadBuf->mCurrentRead);
    assert(!mpCurWriteBuf->mCurrentRead);

    if (ret == WAIT_OBJECT_0)
      ReleaseMutex(mhMutex);
    else if (ret == WAIT_TIMEOUT)
      DEBUG_MSG(DebugLevel::Warnings, "WARNING: - Timed out while waiting on mutex.");
    else
      DEBUG_MSG(DebugLevel::Errors, "ERROR: - wait on mutex failed.");
  }

  void ReleaseResources()
  {
    // Unmap views and close all handles.
    BOOL ret = TRUE;
    if (mpBuf1 != nullptr) ret = UnmapViewOfFile(mpBuf1);
    if (!ret) DEBUG_MSG(DebugLevel::Errors, "Failed to unmap buffer1");

    if (mpBuf2 != nullptr) ret = UnmapViewOfFile(mpBuf2);
    if (!ret) DEBUG_MSG(DebugLevel::Errors, "Failed to unmap buffer2");

    if (mhMap1 != nullptr) ret = CloseHandle(mhMap1);
    if (!ret) DEBUG_MSG(DebugLevel::Errors, "Failed to close map1 handle");

    if (mhMap2 != nullptr) ret = CloseHandle(mhMap2);
    if (!ret) DEBUG_MSG(DebugLevel::Errors, "Failed to close map2 handle");

    if (mhMutex != nullptr) ret = CloseHandle(mhMutex);
    if (!ret) DEBUG_MSG(DebugLevel::Errors, "Failed to close mutex handle");

    mpBuf1 = nullptr;
    mpBuf2 = nullptr;
    mhMap1 = nullptr;
    mhMap2 = nullptr;
    mhMutex = nullptr;
    mpCurWriteBuf = nullptr;
    mpCurReadBuf = nullptr;
  }

  void FlipBuffersHelper()
  {
    if (!mMapped) {
      assert(mMapped);
      DEBUG_MSG(DebugLevel::Errors, "Accessing unmapped buffer.");
      return;
    }

    // Handle fucked up case:
    if (mpBuf1->mCurrentRead == mpBuf2->mCurrentRead) {
      mpBuf1->mCurrentRead = true;
      mpBuf2->mCurrentRead = false;
      DEBUG_MSG(DebugLevel::Errors, "ERROR: - Buffers out of sync.");
    }

    // Update read buffer.
    assert(mpCurReadBuf->mCurrentRead);
    assert(!mpCurWriteBuf->mCurrentRead);
    mpCurReadBuf = mpCurWriteBuf;

    // Pick previous read buffer.
    mpCurWriteBuf = mpBuf1->mCurrentRead ? mpBuf1 : mpBuf2;

    // Switch the read and write buffers.
    mpBuf1->mCurrentRead = !mpBuf1->mCurrentRead;
    mpBuf2->mCurrentRead = !mpBuf2->mCurrentRead;

    assert(!mpCurWriteBuf->mCurrentRead);
    assert(mpCurReadBuf->mCurrentRead);
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

    auto const ret = WaitForSingleObject(mhMutex, SharedMemoryPlugin::msMillisMutexWait);

    FlipBuffersHelper();

    if (ret == WAIT_OBJECT_0)
      ReleaseMutex(mhMutex);
    else if (ret == WAIT_TIMEOUT)
      DEBUG_MSG(DebugLevel::Warnings, "WARNING: - Timed out while waiting on mutex.");
    else
      DEBUG_MSG(DebugLevel::Errors, "ERROR: - wait on mutex failed.");
  }

  void TryFlipBuffers()
  {
    if (!mMapped) {
      assert(mMapped);
      DEBUG_MSG(DebugLevel::Errors, "Accessing unmapped buffer.");
      return;
    }

    // Do not wait on mutex if it is held.
    auto const ret = WaitForSingleObject(mhMutex, 0);
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

    if (ret == WAIT_OBJECT_0)
      ReleaseMutex(mhMutex);
  }

  int AsyncRetriesLeft() const { return mAsyncRetriesLeft; }
  int RetryPending() const { return mRetryPending; }

private:
  MappedDoubleBuffer(MappedDoubleBuffer const&) = delete;
  MappedDoubleBuffer& operator=(MappedDoubleBuffer const&) = delete;

  HANDLE MapMemoryFile(char const* const fileName, BuffT*& pBuf) const
  {
    char tag[256] = {};
    strcpy_s(tag, fileName);

    char exe[1024] = {};
    GetModuleFileName(nullptr, exe, sizeof(exe));

    char pid[8] = {};
    sprintf(pid, "%d", GetCurrentProcessId());

    // Append processId for dedicated server to allow multiple instances
    // TODO: Verify for rF2.
    if (strstr(exe, "Dedicated.exe") != nullptr)
      strcat(tag, pid);

    // Init handle and try to create, read if existing
    HANDLE hMap = INVALID_HANDLE_VALUE;
    hMap = CreateFileMapping(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof(BuffT), TEXT(tag));
    if (hMap == nullptr) {
      if (GetLastError() == ERROR_ALREADY_EXISTS) {
        hMap = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, TEXT(tag));
        if (hMap == nullptr) {
          return nullptr;
        }
      }
    }

    pBuf = static_cast<BuffT*>(MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(BuffT)));
    if (pBuf == nullptr) {
      // Failed to map memory buffer
      CloseHandle(hMap);
      return nullptr;
    }

    return hMap;
  }

  public:
    // Flip between 2 buffers.  Clients should read the one with mCurrentRead == true.
    BuffT* mpBuf1 = nullptr;
    BuffT* mpBuf2 = nullptr;

    BuffT* mpCurWriteBuf = nullptr;
    BuffT* mpCurReadBuf = nullptr;

  private:
    int const MAX_RETRIES;
    char const* const MM_FILE_NAME1;
    char const* const MM_FILE_NAME2;
    char const* const MM_FILE_ACCESS_MUTEX;

    HANDLE mhMutex = nullptr;
    HANDLE mhMap1 = nullptr;
    HANDLE mhMap2 = nullptr;

    bool mRetryPending = false;
    int mAsyncRetriesLeft = 0;

    bool mMapped = false;
};