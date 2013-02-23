/*****************************************************************************

    CMutex

    Note: #definfe SAFE_MUTEX if you want a mutex that adheres strictly to
    lock/unlock behavior requirements. Without this #define, CMutex will
    try to avoid calling the OS-level Lock/Unlock facilities as much as
    possible. i.e. if the current thread has already locked this mutex, it
    will not be locked again. A counter will keep track of the lock/unlock
    nesting depth, and the mutex will only be unlocked when the counter
    reaches zero.

*****************************************************************************/


#ifndef _CMUTEX_H
#define _CMUTEX_H


#include "../C_APILib/C_ShObj.h"
#include "SmartLock.h"
//#define SAFE_CMUTEX
#include "guard.h"


class CMutex
{
public:
    typedef SmartLock<CMutex> AutoLock;

    CMutex (bool Locked = false)
    {
        guard
        {
            InitShObject (&Mutex, TRUE);
            OwnerThread = GetCurrentThreadId();

            if (!Locked)
                Unlock ();

            return;
        } unguard;
    }

    virtual ~CMutex ()
    {
        guard
        {
            DestroyShObject (&Mutex);
            return;
        } unguard;
    }

    void Lock (void)
    {
        guard
        {
    #ifndef SAFE_CMUTEX
            DWORD TID = GetCurrentThreadId();

            if (OwnerThread != TID)
            {
    #endif
                LockShObject (&Mutex);
    #ifndef SAFE_CMUTEX
                OwnerThread = TID;
            }
            else
            {
                Mutex.LockCount++;
            }
    #endif

            return;
        } unguard;
    }

    void Unlock (void)
    {
        guard
        {
    #ifndef SAFE_CMUTEX
            Mutex.LockCount--;

            if (Mutex.LockCount == 0)
            {
                OwnerThread = 0;
    #endif
                UnlockShObject (&Mutex);
    #ifndef SAFE_CMUTEX
            }
    #endif
            return;
        } unguard;
    }

private:
    volatile DWORD OwnerThread;

    ShObj Mutex;
};


#endif // _CMUTEX_H