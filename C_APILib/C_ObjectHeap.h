/*****************************************************************************

	C_ObjectHandler

  Provides a method for almost any "object" to be cleanly created and
  destroyed and to avoid memory leaks! An example: Say you're running
  HAL and root kills your process. Normally the files 'megahal.brn',etc.
  would still be open, but this provides a way to close those files
  automatically when the process exits or is killed.

  Instances of this class are meant to be used within the C_Program
  class. The instances are reffered to as "object heaps." 

*****************************************************************************/

#ifndef C_OBJECTHANDLER_H
#define C_OBJECTHANDLER_H

#include "C_Linkage.h"
class CAPILINK C_ObjectHeap;

#include "C_Memory.h"
#include "C_Object.h"
#include "C_ShObj.h"


class CAPILINK C_ObjectHeap
{
public:
	C_ObjectHeap ();
	~C_ObjectHeap ();

	// returns TRUE/FALSE success and also returns index in *Index so you can
	// DeleteObject () later
	BOOL AddObject       (C_Object *Object, DWORD *Index);
	BOOL DeleteObject    (DWORD Index, BOOL Delete);
	BOOL IsObjIndexValid (DWORD Index);

	DWORD GetObjectTotal (void) { return (ObjHeapLength); }

	void LockObjHeap (void)   { LockShObject (&ObjHeapLock);   }
	void UnlockObjHeap (void) { UnlockShObject (&ObjHeapLock); }

	BOOL SuspendHeapObjects (void);
	BOOL ResumeHeapObjects (void);

	BOOL  GetHeapCPUTimes (LPFILETIME lpCreationTime, 
		                   LPFILETIME lpExitTime, 
					       LPFILETIME lpKernelTime, 
					       LPFILETIME lpUserTime);

	DWORD     GetTotalMemUsage (void);
	C_Memory *GetMemoryHeap (void)    { return (MemoryHeap); }

protected:
	C_Object **ObjHeap;
	DWORD      ObjHeapLength;
	ShObj      ObjHeapLock;

	BOOL       InitSuccess;
	C_Memory  *MemoryHeap;
};


#endif // C_OBJECTHANDLER_H