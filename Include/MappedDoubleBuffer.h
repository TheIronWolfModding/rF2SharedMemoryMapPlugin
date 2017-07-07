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
    mhMap1 = MapMemoryFile(MM_FILE_NAME1, mpBuff1);
    if (mhMap1 == nullptr) {
      DEBUG_MSG(DebugLevel::Errors, "Failed to map file 1");
      return false;
    }

    mhMap2 = MapMemoryFile(MM_FILE_NAME2, mpBuff2);
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
      memcpy(mpBuff1, pInitialContents, sizeof(BuffT));
      memcpy(mpBuff2, pInitialContents, sizeof(BuffT));
    }
    else {
      memset(mpBuff1, 0, sizeof(BuffT));
      memset(mpBuff2, 0, sizeof(BuffT));
    }

    mpBuff1->mCurrentRead = true;
    mpBuff2->mCurrentRead = false;

    mpCurrReadBuff = mpBuff1;
    mpCurrWriteBuff = mpBuff2;
    assert(mpCurrReadBuff->mCurrentRead);
    assert(!mpCurrWriteBuff->mCurrentRead);

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
    if (mpBuff1 != nullptr) ret = UnmapViewOfFile(mpBuff1);
    if (!ret) DEBUG_MSG(DebugLevel::Errors, "Failed to unmap buffer1");

    if (mpBuff2 != nullptr) ret = UnmapViewOfFile(mpBuff2);
    if (!ret) DEBUG_MSG(DebugLevel::Errors, "Failed to unmap buffer2");

    if (mhMap1 != nullptr) ret = CloseHandle(mhMap1);
    if (!ret) DEBUG_MSG(DebugLevel::Errors, "Failed to close map1 handle");

    if (mhMap2 != nullptr) ret = CloseHandle(mhMap2);
    if (!ret) DEBUG_MSG(DebugLevel::Errors, "Failed to close map2 handle");

    if (mhMutex != nullptr) ret = CloseHandle(mhMutex);
    if (!ret) DEBUG_MSG(DebugLevel::Errors, "Failed to close mutex handle");

    mpBuff1 = nullptr;
    mpBuff2 = nullptr;
    mhMap1 = nullptr;
    mhMap2 = nullptr;
    mhMutex = nullptr;
    mpCurrWriteBuff = nullptr;
    mpCurrReadBuff = nullptr;
  }

  void FlipBuffersHelper()
  {
    if (!mMapped) {
      assert(mMapped);
      DEBUG_MSG(DebugLevel::Errors, "Accessing unmapped buffer.");
      return;
    }

    // Handle fucked up case:
    if (mpBuff1->mCurrentRead == mpBuff2->mCurrentRead) {
      mpBuff1->mCurrentRead = true;
      mpBuff2->mCurrentRead = false;
      DEBUG_MSG(DebugLevel::Errors, "ERROR: - Buffers out of sync.");
    }

    // Update read buffer.
    assert(mpCurrReadBuff->mCurrentRead);
    assert(!mpCurrWriteBuff->mCurrentRead);
    mpCurrReadBuff = mpCurrWriteBuff;

    // Pick previous read buffer.
    mpCurrWriteBuff = mpBuff1->mCurrentRead ? mpBuff1 : mpBuff2;

    // Switch the read and write buffers.
    mpBuff1->mCurrentRead = !mpBuff1->mCurrentRead;
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
    BuffT* mpBuff1 = nullptr;
    BuffT* mpBuff2 = nullptr;

    BuffT* mpCurrWriteBuff = nullptr;
    BuffT* mpCurrReadBuff = nullptr;

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