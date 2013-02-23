#include "CThread.h"
#include <stdexcept>
#include "guard.h"


CThread::CThread ()
{
    guard
    {
        ThreadID = 0;
        ThreadHandle = NULL;
        return;
    } unguard;
}


CThread::~CThread ()
{
    guard
    {
        if (ThreadHandle != NULL)
            TerminateThread (ThreadHandle, 0xffffffff);

        CloseHandle (ThreadHandle);

        return;
    } unguard;
}


bool CThread::IsRunning (void)
{
    guard
    {
        DWORD ExitCode;

        GetExitCodeThread (ThreadHandle, &ExitCode);

        if (ExitCode == STILL_ACTIVE)
            return (true);

        return (false);
    } unguard;
}


DWORD WINAPI ThreadWrap (LPVOID Context)
{
    CThread *Thread;

    Thread = (CThread *)Context;

#ifdef NDEBUG
    try
#endif
    {
        if (Thread->ThreadFunction ())
            delete Thread;
    }

#ifdef NDEBUG
    catch (exception &ex)
    {
        MessageBox (NULL, ex.what(), "Unhandled C++ Exception in Win32 Thread", MB_OK);
        ExitProcess (1000);
    }
#endif

    return (0);
}


bool CThread::RunThread (void)
{
    guard
    {
        if (IsRunning())
            return (false);

        ThreadHandle = CreateThread (NULL, 4096, ThreadWrap, (void *)this, NULL, &ThreadID);

        if (ThreadHandle == NULL)
            return (false);

        return (true);
    } unguard;
}