/*****************************************************************************

    CMutexRW

    Multiple readers, single writer mutex object
    Currently implemented as a normal mutex.

*****************************************************************************/


#ifndef _CMUTEXRW_H
#define _CMUTEXRW_H


#include "CMutex.h"


class CMutexRW
{
public:
    // If Locked is true then the mutex is created with initial ownership in write mode
    CMutexRW (bool LockedWrite = false)
        : Mutex (LockedWrite)
    {
        guard
        {
            return;
        } unguard;
    }

    ~CMutexRW ()
    {
        guard
        {
            return;
        } unguard;
    }

    void LockRead (void) const
    {
        guard
        {
            Mutex.Lock ();
            return;
        } unguard;
    }

    void UnlockRead (void) const
    {
        guard
        {
            Mutex.Unlock ();
            return;
        } unguard;
    }

    void LockWrite (void) const
    {
        guard
        {
            Mutex.Lock ();
            return;
        } unguard;
    }

    void UnlockWrite (void) const
    {
        guard
        {
            Mutex.Unlock ();
            return;
        } unguard;
    }

private:
    mutable CMutex Mutex;
};


#endif // _CMUTEXRW_H