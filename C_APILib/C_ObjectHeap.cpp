#include "C_ObjectHeap.h"

#define MEMPTR MemoryHeap
#include "C_Thunks.h"

C_ObjectHeap::C_ObjectHeap ()
{
	InitSuccess = FALSE;

	InitShObject (&ObjHeapLock, TRUE);

	ObjHeap = NULL;
	ObjHeapLength = 0;

	MemoryHeap = NULL;
	MemoryHeap = new C_Memory (0);

	UnlockShObject (&ObjHeapLock);

	InitSuccess = TRUE;
}


C_ObjectHeap::~C_ObjectHeap ()
{
	LockShObject (&ObjHeapLock);

	for (DWORD i = 0; i < ObjHeapLength; i++)
	{
		if (ObjHeap[i] != NULL)
		{
			DeleteObject (i, TRUE);
		}
	}

	UnlockShObject (&ObjHeapLock);

	free (ObjHeap);
	delete MemoryHeap;

	DestroyShObject (&ObjHeapLock);
}


BOOL C_ObjectHeap::IsObjIndexValid (DWORD Index)
{
	BOOL ReturnVal;

	if (InitSuccess != TRUE)
		return (0);

	LockObjHeap ();

	if (Index > (ObjHeapLength - 1))
		ReturnVal = FALSE;
	else
		ReturnVal = TRUE;

	UnlockObjHeap ();

	return (ReturnVal);
}


DWORD C_ObjectHeap::GetTotalMemUsage (void)
{
	DWORD Total;
	
	if (InitSuccess != TRUE)
		return (0);
		
	Total = sizeof(C_ObjectHeap);
	Total += MemoryHeap->GetTotalHeapSize();

//	LockObjHeap ();
	for (DWORD i = 0; i < ObjHeapLength; i++)
	{
		if (ObjHeap[i] != NULL)
		{
            DWORD Mem = 0;
            try
            {
			    Mem = ObjHeap[i]->GetMemUsage();
            }

            catch (...)
            {
                // do jack
            }

            Total += Mem;
		}
	}
//	UnlockObjHeap ();

	return (Total);
}


BOOL C_ObjectHeap::SuspendHeapObjects (void)
{
	if (!InitSuccess)
		return (FALSE);

	//LockObjHeap ();
	for (DWORD i = 0; i < ObjHeapLength; i++)
	{
		if (ObjHeap[i] != NULL)
		{
            try
            {
			    ObjHeap[i]->SuspendObject ();
            }

            catch (...)
            {
                // do jack :(
            }
		}
	}
	//UnlockObjHeap ();

	return (TRUE);
}


BOOL C_ObjectHeap::ResumeHeapObjects (void)
{
	if (!InitSuccess)
		return (FALSE);

	//LockObjHeap ();
	for (DWORD i = 0; i < ObjHeapLength; i++)
	{
		if (ObjHeap[i] != NULL)
		{
            try
            {
			    ObjHeap[i]->ResumeObject ();
            }

            catch (...)
            {
                // do jack :(
            }
		}
	}
	//UnlockObjHeap ();

	return (TRUE);
}


BOOL C_ObjectHeap::AddObject (C_Object *Object, DWORD *Index)
{
	BOOL ReturnVal = FALSE;

	if (InitSuccess != TRUE)
		return (FALSE);

	if (Object == NULL  ||  Index == NULL)
		return (FALSE);

	LockObjHeap ();
	ObjHeap = (C_Object **) realloc (ObjHeap, sizeof(C_Object *) * (ObjHeapLength+1));
	ObjHeap[ObjHeapLength] = Object;

	if (Index)
	{
		*Index = ObjHeapLength;
	}

	ObjHeapLength++;
	ReturnVal = TRUE;

	UnlockObjHeap ();

	return (ReturnVal);
}


BOOL C_ObjectHeap::DeleteObject (DWORD Index, BOOL Delete)
{
	BOOL ReturnVal = FALSE;

	if (InitSuccess != TRUE)
		return (FALSE);

	if (Index > (ObjHeapLength - 1))
		return (FALSE);

	LockObjHeap ();

	if (ObjHeap[Index] != NULL)
	{
		if (Delete) 
		{
            ObjHeap[Index]->Lock();
			ObjHeap[Index]->SetDeleteFlag (TRUE);
            // for some reason consframe crashes sometimes when txt or tetris is
            // running and you smack the X to quit, or shutdown otherwise
            try
            {
			    delete ObjHeap[Index];
            }
            catch (...)
            {

            }
		}

		ObjHeap[Index] = NULL;
		ReturnVal = TRUE;
	}

	ObjHeap[Index] = NULL;

	UnlockObjHeap ();

	return (ReturnVal);
}


BOOL C_ObjectHeap::GetHeapCPUTimes (LPFILETIME lpCreationTime, 
	                                LPFILETIME lpExitTime, 
				            	    LPFILETIME lpKernelTime, 
					                LPFILETIME lpUserTime)
{
	FILETIME Creation;
	FILETIME ExitTime;
	FILETIME KernelTime;
	FILETIME UserTime;

	memset (&Creation,   0, sizeof (FILETIME));
	memset (&ExitTime,   0, sizeof (FILETIME));
	memset (&KernelTime, 0, sizeof (FILETIME));
	memset (&UserTime,   0, sizeof (FILETIME));

//	LockObjHeap ();

	for (DWORD i = 0; i < ObjHeapLength; i++)
	{
		if (ObjHeap[i] != NULL)
		{
			ULONG64 *Cast;

            try
            {
			    if (ObjHeap[i]->GetCPUTimes (&Creation, &ExitTime, &KernelTime, &UserTime))
			    {
    				Cast = (ULONG64 *)lpCreationTime;
    				(*Cast) += *((ULONG64 *)&Creation);

				    Cast = (ULONG64 *)lpExitTime;
				    (*Cast) += *((ULONG64 *)&ExitTime);

				    Cast = (ULONG64 *)lpKernelTime;
				    (*Cast) += *((ULONG64 *)&KernelTime);

				    Cast = (ULONG64 *)lpUserTime;
				    (*Cast) += *((ULONG64 *)&UserTime);
			    }
            }

            catch (...)
            {
                // do jack
            }
			 
		}
	}

//	UnlockObjHeap ();

	return (TRUE);
}

