//#define WINVER 0x0500


#include "C_ShObj.h"
#include <WinBase.h>


BOOL InitShObject (ShObj *Object, BOOL Locked)
{

#ifdef SHOBJ_USE_MUTEX
    CreateMutex (NULL, Locked, NULL);
#endif

#ifdef SHOBJ_USE_CS
    InitializeCriticalSection (&Object->CriticalSection);
#endif

	Object->LockCount   = 0;
	Object->Locked      = FALSE;
	Object->Initialized = TRUE;
	Object->Data        = NULL;

#ifdef SHOBJ_USE_CS
    if (Locked)
        LockShObject (Object);
#endif

	return (TRUE);
}


BOOL DestroyShObject (ShObj *Object)
{
    LockShObject (Object);

#ifdef SHOBJ_USE_MUTEX
    CloseHandle (Object->Mutex);
#endif

#ifdef SHOBJ_USE_CS
    DeleteCriticalSection (&Object->CriticalSection);
#endif

	Object->Initialized = FALSE;
    return (TRUE);
}


BOOL LockShObject (ShObj *Object)
{
    if (Object->Initialized == FALSE)
        return (FALSE);

    Object->Locked = TRUE;

#ifdef SHOBJ_USE_MUTEX
    WaitForSingleObject (Object->Mutex, INFINITE);
#endif

#ifdef SHOBJ_USE_CS
    EnterCriticalSection (&Object->CriticalSection);
#endif

    Object->LockCount += 1;

    return (TRUE);
}


BOOL UnlockShObject (ShObj *Object)
{
    if (Object->Initialized == FALSE)
        return (FALSE);

	Object->LockCount -= 1;
	if (Object->LockCount <= 0)
	{
		Object->LockCount = 0;
		Object->Locked = FALSE;
	}

#ifdef SHOBJ_USE_MUTEX
    ReleaseMutex (Object->Mutex);
#endif

#ifdef SHOBJ_USE_CS
    LeaveCriticalSection (&Object->CriticalSection);
#endif

	return (TRUE);
}

