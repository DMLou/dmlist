// ListShell.cpp : Defines the entry point for the DLL application.
//


#pragma warning (disable: 4530)
#include <windows.h>
#include <comcat.h>
#include "ListShell.h"


// GUID stuff
#pragma data_seg(".text")
#define INITGUID
#include <initguid.h>
#include <shlguid.h>
#include "GUID.h"
#pragma data_seg()


#include "CClassFactory.h"


// Global variables
HINSTANCE GlobalInstance;
UINT GlobalRefCount = 0;


extern "C" BOOL WINAPI DllMain (HINSTANCE HInstance, DWORD ReasonForCall, LPVOID Reserved)
{
	switch (ReasonForCall)
	{
	    case DLL_PROCESS_ATTACH:
            GlobalInstance = HInstance;
            break;

	    case DLL_THREAD_ATTACH:
            break;

	    case DLL_THREAD_DETACH:
            break;

	    case DLL_PROCESS_DETACH:
		    break;
	}

    return (TRUE);
}


STDAPI DllCanUnloadNow ()
{
    if (GlobalRefCount == 0)
        return (S_OK);

    return (S_FALSE);
}


STDAPI DllGetClassObject (REFCLSID ReqCLSID, REFIID ReqIID, LPVOID *ObjectResult)
{
    CClassFactory *Factory;
    HRESULT HResult;

    *ObjectResult = NULL;

    if (!IsEqualCLSID (ReqCLSID, CLSID_ListXPShortcutMenu))
        return (CLASS_E_CLASSNOTAVAILABLE);

    Factory = new CClassFactory (ReqCLSID);
    Factory->Constructor (ReqCLSID);

    if (Factory == NULL)
        return (E_OUTOFMEMORY);

    HResult = Factory->QueryInterface (ReqIID, ObjectResult);
    Factory->Release ();

    return (HResult);
}

