/*****************************************************************************

  CThread

  Class for abstracting away thread management
  To use:
  1. Inherit class and override ThreadFunction, implement new constructor
     and destructors as needed.
  2. Create instance of class.
  3. Call RunThread()
  
  Constructor runs *before* the thread starts execution.
  Thread starts executing *before* RunThread() returns.
  Destructor runs *before* the thread stops execution (unless it has already
  terminated), and when it has finished the thread is terminated. Optimally
  you will implement code using signals to gracefully stop the thread first.
  RunThread() will not spawn the thread again if it is already running
  ThreadFunction() should return true to cause a delete of the class instance
  (returning true makes it easy to create threads that cleanup by themself
  when they are done)

  All threads have built-in mutex functions, Lock() and Unlock(), as a result
  of deriving from CMutex.

*****************************************************************************/


#ifndef _CTHREAD_H
#define _CTHREAD_H


#include <windows.h>
#include "CMutex.h"


class CThread : public CMutex
{
public:
    CThread ();
    virtual ~CThread ();

    bool RunThread (void);
    bool IsRunning (void);

    virtual bool ThreadFunction (void) = NULL;

private:
    HANDLE ThreadHandle;
    DWORD ThreadID;
};


#endif // _CTHREAD_H