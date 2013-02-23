/*****************************************************************************

	C_ShObj.h

  Mechanism for sharing an object between multiple threads which require
  mutually exclusive access. 

*****************************************************************************/

#ifndef C_SHOBJ_H
#define C_SHOBJ_H

#include <windows.h>
#include "C_Linkage.h"


#define SHOBJ_USE_CS
//#define SHOBJ_USE_MUTEX


typedef struct SharedObject
{
	volatile void  *Data;      // use this for whatever you want

	volatile BOOL   Locked;
	volatile int    LockCount;

	volatile BOOL   Initialized;

#ifdef WIN32

#ifdef SHOBJ_USE_MUTEX
	HANDLE Mutex;
#endif

#ifdef SHOBJ_USE_CS
    CRITICAL_SECTION CriticalSection;
#endif

#endif
} ShObj;


CAPILINK BOOL  InitShObject    (ShObj *Object, BOOL Locked);
CAPILINK BOOL  DestroyShObject (ShObj *Object);
CAPILINK BOOL  LockShObject    (ShObj *Object);
CAPILINK BOOL  UnlockShObject  (ShObj *Object);


#endif // C_SHOBJ_H
