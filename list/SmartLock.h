/*****************************************************************************

  SmartLock

  Defines an object that is "smart" in that it will automatically unlock a
  mutex (or any object supporting Lock() and Unlock()) when it goes out of
  scope. Simply create this object and use it for doing Lock/Unlock operations.

  The object is created in a completely unlocked state.

*****************************************************************************/

#ifndef _SMARTLOCK_H
#define _SMARTLOCK_H


template<class Mutex>
class SmartLock
{
public:
    SmartLock (Mutex *ParentMutex)
    {
        guard
        {
            TheMutex = ParentMutex;
            StillLocked = 0;
            return;
        } unguard;
    }

    ~SmartLock ()
    {
        guard
        {
            while (StillLocked > 0)
                TheMutex->Unlock ();

            return;
        } unguard;
    }

    void Lock (void)
    {
        guard
        {
            TheMutex->Lock ();
            StillLocked++;
            return;
        } unguard;
    }

    void Unlock (void)
    {
        guard
        {
            if (StillLocked > 0)
            {
                TheMutex->Unlock ();
                StillLocked--;
            }

            return;
        } unguard;
    }

private:
    int StillLocked;
    Mutex *TheMutex;
};


#endif