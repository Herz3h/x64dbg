/**
 @file thread.cpp

 @brief Implements the thread class.
 */

#include "thread.h"
#include "memory.h"
#include "threading.h"

std::unordered_map<DWORD, THREADINFO> threadList;

void ThreadCreate(CREATE_THREAD_DEBUG_INFO* CreateThread)
{
    THREADINFO curInfo;
    memset(&curInfo, 0, sizeof(THREADINFO));

    curInfo.ThreadNumber = ThreadGetCount();
    curInfo.Handle = CreateThread->hThread;
    curInfo.ThreadId = ((DEBUG_EVENT*)GetDebugData())->dwThreadId;
    curInfo.ThreadStartAddress = (duint)CreateThread->lpStartAddress;
    curInfo.ThreadLocalBase = (duint)CreateThread->lpThreadLocalBase;

    // The first thread (#0) is always the main program thread
    if(curInfo.ThreadNumber <= 0)
        strcpy_s(curInfo.threadName, "Main Thread");

    // Modify global thread list
    EXCLUSIVE_ACQUIRE(LockThreads);
    threadList.insert(std::make_pair(curInfo.ThreadId, curInfo));
    EXCLUSIVE_RELEASE();

    // Notify GUI
    GuiUpdateThreadView();
}

void ThreadExit(DWORD ThreadId)
{
    EXCLUSIVE_ACQUIRE(LockThreads);

    // Erase element using native functions
    auto itr = threadList.find(ThreadId);

    if(itr != threadList.end())
        threadList.erase(itr);

    EXCLUSIVE_RELEASE();
    GuiUpdateThreadView();
}

void ThreadClear()
{
    // Clear the current array of threads
    EXCLUSIVE_ACQUIRE(LockThreads);
    threadList.clear();
    EXCLUSIVE_RELEASE();

    // Update the GUI's list
    GuiUpdateThreadView();
}

int ThreadGetCount()
{
    SHARED_ACQUIRE(LockThreads);
    return (int)threadList.size();
}

void ThreadGetList(THREADLIST* List)
{
    ASSERT_NONNULL(List);
    SHARED_ACQUIRE(LockThreads);

    //
    // This function converts a C++ std::unordered_map to a C-style THREADLIST[].
    // Also assume BridgeAlloc zeros the returned buffer.
    //
    List->count = (int)threadList.size();
    List->list = nullptr;

    if(List->count <= 0)
        return;

    // Allocate C-style array
    List->list = (THREADALLINFO*)BridgeAlloc(List->count * sizeof(THREADALLINFO));

    // Fill out the list data
    int index = 0;

    for(auto & itr : threadList)
    {
        HANDLE threadHandle = itr.second.Handle;

        // Get the debugger's active thread index
        if(threadHandle == hActiveThread)
            List->CurrentThread = index;

        memcpy(&List->list[index].BasicInfo, &itr.second, sizeof(THREADINFO));

        List->list[index].ThreadCip = GetContextDataEx(threadHandle, UE_CIP);
        List->list[index].SuspendCount = ThreadGetSuspendCount(threadHandle);
        List->list[index].Priority = ThreadGetPriority(threadHandle);
        List->list[index].WaitReason = ThreadGetWaitReason(threadHandle);
        List->list[index].LastError = ThreadGetLastErrorTEB(itr.second.ThreadLocalBase);
        index++;
    }
}

bool ThreadIsValid(DWORD ThreadId)
{
    SHARED_ACQUIRE(LockThreads);
    return threadList.find(ThreadId) != threadList.end();
}

bool ThreadGetTib(duint TEBAddress, NT_TIB* Tib)
{
    // Calculate offset from structure member
    TEBAddress += offsetof(TEB, Tib);

    memset(Tib, 0, sizeof(NT_TIB));
    return MemRead(TEBAddress, Tib, sizeof(NT_TIB));
}

bool ThreadGetTeb(duint TEBAddress, TEB* Teb)
{
    memset(Teb, 0, sizeof(TEB));
    return MemRead(TEBAddress, Teb, sizeof(TEB));
}

int ThreadGetSuspendCount(HANDLE Thread)
{
    //
    // Suspend a thread in order to get the previous suspension count
    // WARNING: This function is very bad (threads should not be randomly interrupted)
    //
    int suspendCount = (int)SuspendThread(Thread);

    if(suspendCount == -1)
        return 0;

    // Resume the thread's normal execution
    ResumeThread(Thread);

    return suspendCount;
}

THREADPRIORITY ThreadGetPriority(HANDLE Thread)
{
    return (THREADPRIORITY)GetThreadPriority(Thread);
}

THREADWAITREASON ThreadGetWaitReason(HANDLE Thread)
{
    UNREFERENCED_PARAMETER(Thread);

    // TODO: Implement this
    return _Executive;
}

DWORD ThreadGetLastErrorTEB(ULONG_PTR ThreadLocalBase)
{
    // Get the offset for the TEB::LastErrorValue and read it
    DWORD lastError = 0;
    duint structOffset = ThreadLocalBase + offsetof(TEB, LastErrorValue);

    MemRead(structOffset, &lastError, sizeof(DWORD));
    return lastError;
}

DWORD ThreadGetLastError(DWORD ThreadId)
{
    SHARED_ACQUIRE(LockThreads);

    if(threadList.find(ThreadId) != threadList.end())
        return ThreadGetLastErrorTEB(threadList[ThreadId].ThreadLocalBase);

    ASSERT_ALWAYS("Trying to get last error of a thread that doesn't exist!");
    return 0;
}

bool ThreadSetName(DWORD ThreadId, const char* Name)
{
    EXCLUSIVE_ACQUIRE(LockThreads);

    // Modifies a variable (name), so an exclusive lock is required
    if(threadList.find(ThreadId) != threadList.end())
    {
        if(!Name)
            Name = "";

        strcpy_s(threadList[ThreadId].threadName, Name);
        return true;
    }

    return false;
}

HANDLE ThreadGetHandle(DWORD ThreadId)
{
    SHARED_ACQUIRE(LockThreads);

    if(threadList.find(ThreadId) != threadList.end())
        return threadList[ThreadId].Handle;

    ASSERT_ALWAYS("Trying to get handle of a thread that doesn't exist!");
    return nullptr;
}

DWORD ThreadGetId(HANDLE Thread)
{
    SHARED_ACQUIRE(LockThreads);

    // Search for the ID in the local list
    for(auto & entry : threadList)
    {
        if(entry.second.Handle == Thread)
            return entry.first;
    }

    // Wasn't found, check with Windows
    typedef DWORD (WINAPI * GETTHREADID)(HANDLE hThread);
    static GETTHREADID _GetThreadId = (GETTHREADID)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "GetThreadId");
    return _GetThreadId ? _GetThreadId(Thread) : 0;
}

int ThreadSuspendAll()
{
    // SuspendThread does not modify any internal variables
    SHARED_ACQUIRE(LockThreads);

    int count = 0;
    for(auto & entry : threadList)
    {
        if(SuspendThread(entry.second.Handle) != -1)
            count++;
    }

    return count;
}

int ThreadResumeAll()
{
    // ResumeThread does not modify any internal variables
    SHARED_ACQUIRE(LockThreads);

    int count = 0;
    for(auto & entry : threadList)
    {
        if(ResumeThread(entry.second.Handle) != -1)
            count++;
    }

    return count;
}