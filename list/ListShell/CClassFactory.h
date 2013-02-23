/*****************************************************************************

  CClassFactory

  Class to create instances of the COM objects required to implement the
  ListXP shortcut menu handler

*****************************************************************************/


#ifndef _CCLASSFACTORY_H
#define _CCLASSFACTORY_H


#include <windows.h>


class CClassFactory : public IClassFactory
{
public:
    CClassFactory (CLSID ClassID);
    ~CClassFactory ();

    // IUnknown methods
    STDMETHODIMP QueryInterface (REFIID riid, void **ppvObject);
    STDMETHODIMP_(DWORD) AddRef ();
    STDMETHODIMP_(DWORD) Release ();

    // IClassFactory methods
    STDMETHODIMP CreateInstance (IUnknown *pUnkOuter, REFIID riid, void **ppvObject);
    STDMETHODIMP LockServer (BOOL fLock);

    void Constructor (CLSID ClassID);
    void Destructor (void);

protected:
    DWORD RefCount;

private:
    CLSID ClassIDObject;
};


#endif // _CCLASSFACTORY_H