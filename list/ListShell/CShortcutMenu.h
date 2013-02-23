/******************************************************************************

  CShortcutMenu

  Implements the shortcut menu for CLSID_ListXPShortcutMenu

******************************************************************************/

#ifndef _CSHORTCUTMENU_H
#define _CSHORTCUTMENU_H


#include <Shlobj.h>


#define IDM_LIST 0


class CShortcutMenu : public IShellExtInit, 
                      public IContextMenu
{
public:
    CShortcutMenu ();
    ~CShortcutMenu ();

    //IUnknown methods
    STDMETHODIMP QueryInterface(REFIID, LPVOID*);
    STDMETHODIMP_(DWORD) AddRef();
    STDMETHODIMP_(DWORD) Release();

    // IShellExtInit methods
    STDMETHOD (Initialize) (LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID);

    // IContextMenu methods    
    STDMETHOD (GetCommandString) (UINT_PTR idCmd, UINT uFlags, UINT *pwReserved, LPSTR pszName, UINT cchMax);
    STDMETHOD (InvokeCommand) (LPCMINVOKECOMMANDINFO pici);
    STDMETHOD (QueryContextMenu) (HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);

protected:
    DWORD RefCount;

private:
    TCHAR FileName[MAX_PATH * 2]; // the filename to open, initialized in IShellExtInit::Initialize()
    IDataObject *DataObject;
    LPCITEMIDLIST PIDLFolder;
    bool AddMenuItem; // only add a menu item if they want to open 1 item
};


#endif // _CSHORTCUTMENU_H