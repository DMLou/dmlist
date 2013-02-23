/*****************************************************************************

	C_Object

  The actual object. Inherit this class to make an 'object'

*****************************************************************************/

#ifndef C_OBJECT_H
#define C_OBJECT_H

#include "C_Linkage.h"
class CAPILINK C_Object;

#include <windows.h>
#include "C_ObjectHeap.h"
#include "C_Memory.h"
#include "C_ShObj.h"

// inherit this to make an 'object'
// you must alloc an object heap before one of these!
class CAPILINK C_Object
{
public:
	C_Object (C_ObjectHeap *ObjHeap);
	virtual ~C_Object ();

	virtual DWORD  GetMemUsage   (void);
	C_Memory      *GetMemoryHeap (void)  { return (MemoryHeap);  }
	DWORD          GetObjectIndex (void) { return (ObjectIndex); }

	BOOL GetMemHeapExists (void) { return (MemoryHeap != NULL); }

	BOOL GetCanDestroyMemHeap (void)   { return (CanDestroyMemHeap); }
	void SetCanDestroyMemHeap (BOOL c) { CanDestroyMemHeap = c; }

	BOOL DestroyMemHeap (void);
	BOOL CreateMemHeap (void);

	// This is a sort of message that's send when a PID is to be suspended.
	// the return value is the suspend count;
	virtual DWORD SuspendObject (void);
	virtual DWORD ResumeObject (void);
	BOOL    IsSuspended (void)  { return (SuspendCount == 0); }

	virtual BOOL GetCPUTimes (LPFILETIME lpCreationTime, 
				              LPFILETIME lpExitTime, 
						      LPFILETIME lpKernelTime, 
						      LPFILETIME lpUserTime)
	{ return (FALSE); }

    void Lock (void)
    {
        LockShObject (&ObjectLock);
    }

    void Unlock (void)
    {
        UnlockShObject (&ObjectLock);
    }

	// Ok, here's the story behind this. My goal is to make it so that an object
	// can be managed just by using new and delete, where it will call C_ObjectHeap::
	// AddObject and DeleteObject automatically. However, you should also be able
	// to use DeleteObject yourself, which will set the 'delete' flag within the
	// object being deleted so that when it deletes the object it won't call
	// DeleteObject automatically, which would cause a lockup no doubt.
	void      SetDeleteFlag (BOOL f) { Deleting = f; }

protected:
	BOOL          InitSuccess;
	BOOL          Deleting;
	BOOL	      CanDestroyMemHeap;
	DWORD         ObjectIndex;
	C_Memory     *MemoryHeap;
	C_ObjectHeap *ObjectHeap;
	ShObj         ObjectLock;

	DWORD         SuspendCount;
};


#endif // C_OBJECT_H