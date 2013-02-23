#pragma warning (disable: 4530)
#include "CClassFactory.h"
#include "ListShell.h"
#include "GUID.h"
#include "CShortcutMenu.h"


int __cdecl memcmp (const void *b1, const void *b2, size_t length)
{
    char *c1 = (char *)b1;
    char *c2 = (char *)b2;

    while (length > 0)
    {
        if (*c1 > *c2)
            return (1);
        else
        if (*c1 < *c2)
            return (-1);

        c1++;
        c2++;
        length--;
    }

    return (0);
}


CClassFactory::CClassFactory (CLSID ClassID)
{
    Constructor (ClassID);
    return;
}


void CClassFactory::Constructor (CLSID ClassID)
{
    ClassIDObject = ClassID;
    RefCount = 1;
    GlobalRefCount++;
    return;
}


CClassFactory::~CClassFactory (void)
{
    Destructor ();
    return;
}


void CClassFactory::Destructor (void)
{
    GlobalRefCount--;
    return;
}


STDMETHODIMP CClassFactory::QueryInterface (REFIID riid, LPVOID *ppReturn)
{
    *ppReturn = NULL;

    if (IsEqualCLSID (riid, IID_IUnknown))
    {
        *ppReturn = this;
    }
    else
    if (IsEqualCLSID (riid, IID_IClassFactory))
    {
        *ppReturn = (IClassFactory *) this;
    }

    if (*ppReturn != NULL)
    {
        (*(LPUNKNOWN *)ppReturn)->AddRef ();
        return (S_OK);
    }

    return (E_NOINTERFACE);
}


STDMETHODIMP_(DWORD) CClassFactory::AddRef ()
{
    RefCount++;
    return (RefCount);
}


STDMETHODIMP_(DWORD) CClassFactory::Release ()
{
    RefCount--;

    if (RefCount == 0)
    {
        delete this;
        return (0);
    }

    return (RefCount);
}


STDMETHODIMP CClassFactory::CreateInstance (IUnknown *pUnkOuter, REFIID riid, void **ppvObject)
{
    void *Result;
    HRESULT HResult;

    *ppvObject = NULL;

    if (pUnkOuter != NULL)
        return (CLASS_E_NOAGGREGATION);

    if (IsEqualCLSID (ClassIDObject, CLSID_ListXPShortcutMenu))
    {
        CShortcutMenu *ShortcutMenu;

        ShortcutMenu = new CShortcutMenu;

        if (ShortcutMenu == NULL)
            return (E_OUTOFMEMORY);

        Result = (void *) ShortcutMenu;
    }

    if (Result != NULL)
    {
        HResult = ((LPUNKNOWN)Result)->QueryInterface (riid, ppvObject);
        ((LPUNKNOWN)Result)->Release ();
    }

    return (HResult);
}


STDMETHODIMP CClassFactory::LockServer (BOOL fLock)
{
    return (E_NOTIMPL);
}
