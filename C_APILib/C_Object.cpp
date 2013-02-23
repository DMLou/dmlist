#include "C_Object.h"

#define MEMPTR MemoryHeap
#include "C_Thunks.h"


C_Object::C_Object (C_ObjectHeap *ObjHeap)
{
	InitSuccess = FALSE;

	ObjectHeap = ObjHeap;
	MemoryHeap = NULL;

	InitShObject (&ObjectLock, FALSE);

	Deleting = FALSE;
	CanDestroyMemHeap = TRUE;
	InitSuccess = TRUE;
	SuspendCount = 0;

	if (ObjHeap != NULL)
        ObjHeap->AddObject (this, &ObjectIndex);
}


C_Object::~C_Object ()
{
	if (!InitSuccess) return;

	DestroyMemHeap ();

	// Make sure we get 'delete'd. So that we don't loop forever, when we
	// call DeleteObject, it will actually set 'Deleting' to TRUE
	if (!Deleting && ObjectHeap != NULL)
	{
		ObjectHeap->DeleteObject (ObjectIndex, FALSE);
	}

	LockShObject (&ObjectLock);
	DestroyShObject (&ObjectLock);
}


BOOL C_Object::CreateMemHeap (void)
{
	if (MemoryHeap != NULL)
		return (FALSE); // Memory heap already exists!

	MemoryHeap = new C_Memory (0);
	return (TRUE);
}


BOOL C_Object::DestroyMemHeap (void)
{
	if (MemoryHeap == NULL)
		return (FALSE); // Memory heap doesn't exist!

	if (!CanDestroyMemHeap)
		return (FALSE);

	delete MemoryHeap;
	MemoryHeap = NULL;

	return (TRUE);
}


DWORD C_Object::GetMemUsage ()
{ 
	DWORD ReturnVal;

	if (MemoryHeap == NULL)
		ReturnVal = sizeof(C_Object);
	else
		ReturnVal = sizeof(C_Object) + MemoryHeap->GetTotalHeapSize();

	return (ReturnVal);
}


DWORD C_Object::SuspendObject (void)
{
	DWORD ReturnVal;

	LockShObject (&ObjectLock);

	if (SuspendCount != 0xffffffff)
		SuspendCount++;

	ReturnVal = SuspendCount;

	UnlockShObject (&ObjectLock);

	return (ReturnVal);
}


DWORD C_Object::ResumeObject (void)
{
	DWORD ReturnVal;

	LockShObject (&ObjectLock);

	if (SuspendCount != 0)
		SuspendCount--;

	ReturnVal = SuspendCount;

	UnlockShObject (&ObjectLock);

	return (ReturnVal);
}
