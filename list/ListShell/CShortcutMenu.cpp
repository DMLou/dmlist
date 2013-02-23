#pragma warning (disable: 4530)
#include "CShortcutMenu.h"
#include "ListShell.h"
#include "../ListRegistry.h"
#include <shlwapi.h>
#pragma comment(lib,"shlwapi.lib")
#include "GUID.h"


/*
#include <stdio.h>
#include <stdlib.h>

#define SHOWMESSAGE(msg,title) \
    { \
        MessageBox (NULL, msg, title, MB_OK); \
    }

#define SHOWREFCOUNT(fname) \
    { \
        char Text[1024]; \
        sprintf (Text, "RefCount: %d, GlobalRefCount: %d", RefCount, GlobalRefCount); \
        SHOWMESSAGE (Text, fname); \
    }
*/

#define SHOWMESSAGE(msg,title)
#define SHOWREFCOUNT(fname)



CShortcutMenu::CShortcutMenu ()
{
    RefCount = 1;
    GlobalRefCount++;
    DataObject = NULL;
    FileName[0] = '\0';
    AddMenuItem = false;
    SHOWREFCOUNT("CShortCutMenu()");
    return;
}


CShortcutMenu::~CShortcutMenu ()
{
    GlobalRefCount--;

    if (DataObject != NULL)
        DataObject->Release ();

    SHOWREFCOUNT("~CShortCutMenu()");
    return; 
}   


// IUnknown implementation
STDMETHODIMP CShortcutMenu::QueryInterface (REFIID riid, LPVOID *ppReturn)
{
    *ppReturn = NULL;

    if (IsEqualCLSID (riid, IID_IUnknown))
        *ppReturn = this;

    if (IsEqualCLSID (riid, IID_IShellExtInit))
        *ppReturn = (IShellExtInit *) this;

    if (IsEqualCLSID (riid, IID_IContextMenu))
        *ppReturn = (IContextMenu *) this;

    if (*ppReturn != NULL)
    {
        (*(LPUNKNOWN *)ppReturn)->AddRef ();
        return (S_OK);
    }

    return (E_NOINTERFACE);
}


STDMETHODIMP_(DWORD) CShortcutMenu::AddRef ()
{
    RefCount++;
    SHOWREFCOUNT("AddRef()");
    return (RefCount);
}


STDMETHODIMP_(DWORD) CShortcutMenu::Release ()
{
    RefCount--;

    SHOWREFCOUNT("Release()");

    if (RefCount == 0)
    {
        SHOWMESSAGE("deleting","word");
        delete this;
        return (0);
    }

    return (RefCount);
}


// IShellExtInit implementation
STDMETHODIMP CShortcutMenu::Initialize (LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hKeyProgID)
{
    //Store the PIDL.
    if (pidlFolder != NULL)
        PIDLFolder = pidlFolder;
	
    // If Initialize has already been called, release the old IDataObject pointer.
    if (DataObject != NULL)
	{ 
        DataObject->Release(); 
        DataObject = NULL;
    }
	 
    // If a data object pointer was passed in, save it and extract the file name. 
    if (pdtobj != NULL) 
    { 
        STGMEDIUM   StgMedium;
        FORMATETC   fe = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, 
		                   TYMED_HGLOBAL };
        UINT        Count;
        char        Name[MAX_PATH];

        DataObject = pdtobj; 
        DataObject->AddRef(); 

        if (SUCCEEDED (DataObject->GetData (&fe, &StgMedium)))
        {
            // Get the file name from the CF_HDROP.
            Count = DragQueryFile ((HDROP)StgMedium.hGlobal, (UINT)-1, NULL, 0);

            if (Count != 0)
            {
                DragQueryFile((HDROP)StgMedium.hGlobal, 0, Name, sizeof(FileName));
                lstrcpy (FileName, "\"");
                lstrcat (FileName, Name);
                lstrcat (FileName, "\"");
            }

            ReleaseStgMedium (&StgMedium);

            if (Count == 1)
                AddMenuItem = true;
            else
                AddMenuItem = false;
        }

        DataObject->Release ();
        DataObject = NULL;
    }   

    return (S_OK); 
}


// IContextMenu implementation
STDMETHODIMP CShortcutMenu::GetCommandString (UINT_PTR idCmd, UINT uFlags, UINT *pwReserved, LPSTR pszName, UINT cchMax)
{
    HRESULT ReturnVal;

    ReturnVal = E_INVALIDARG;

    switch (idCmd)
    {
        case IDM_LIST:
            switch (uFlags)
            {
                case GCS_HELPTEXT:
                    lstrcpy (pszName, "This will load the file with ListXP");
                    ReturnVal = NOERROR;
                    break;

                case GCS_VERB:
                    lstrcpy (pszName, "List");
                    ReturnVal = NOERROR;
                    break;

                case GCS_VALIDATE:
                    ReturnVal = NOERROR;
                    break;
            }

            break;
    }

    return (ReturnVal);
}


STDMETHODIMP CShortcutMenu::InvokeCommand (LPCMINVOKECOMMANDINFO pico)
{
    switch (LOWORD (pico->lpVerb))
    {   // Called when the user clicks our verb.
        case IDM_LIST:
            {   
                HKEY Key;
                LONG Result;
                char EXEString[2048];
                int Length;
                DWORD Type;
                int SER;

                // We do not use ListXP's VarList[Ext] class because that would add a lot to our DLL size!
                Result = RegCreateKeyEx (LISTREGROOTKEY, LISTREGSUBKEY, 0, NULL, REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS, NULL, &Key, NULL);

                if (Result == ERROR_SUCCESS)
                {
                    Type = REG_SZ;

                    if (ERROR_SUCCESS != RegQueryValueEx (Key, "LastEXEPath", NULL, &Type, NULL, (unsigned long *)&Length))
                    {   // The entry doesn't exist so we'll use our own current directory + listxp.exe ... maybe it'll work!
                        char EXEName[1024];
                        char *p;

                        GetModuleFileName (GlobalInstance, EXEName, sizeof (EXEName));

                        p = EXEName + lstrlen (EXEName) - 1;

                        while (*p != '\\'  &&  p != EXEName)
                            p--;

                        if (p == EXEName)
                            p = NULL;

                        if (p != NULL)
                            *p = '\0';

                        lstrcpy (EXEString, EXEName);
                        lstrcat (EXEString, "\\listxp.exe");
                    }
                    else
                    {
                        RegQueryValueEx (Key, "LastEXEPath", NULL, &Type, (unsigned char *)EXEString, (unsigned long *)&Length);
                    }

                    RegCloseKey (Key);

                    SER = (int) ShellExecute (pico->hwnd, "open", EXEString, FileName, ".", SW_SHOWNORMAL);

                    // Was there an error running list? Well maybe they deleted listxp.exe ... ask if
                    // they want to remove ListXP's presence from the registry
                    if (SER < 32)
                    {   
                        // However, some error codes do not result from listxp.exe being deleted ...
                        if (SER == ERROR_FILE_NOT_FOUND  ||  
                            SER == ERROR_PATH_NOT_FOUND  ||
                            SER == SE_ERR_FNF            ||
                            SER == SE_ERR_PNF)
                        {
                            int MBResult;

                            MBResult = MessageBox (pico->hwnd, 
                                "It appears that the ListXP executable has been deleted.\n"
                                "Would you like to remove all ListXP registry entries?",
                                "ListXP Uninstall",
                                MB_YESNO | MB_ICONQUESTION);

                            if (MBResult == IDYES)
                            {   // Delete the following registry keys:
                                // HKEY_CURRENT_USER\SOFTWARE\ListXP
                                // HKEY_CLASSES_ROOT\CLSID\{Our GUID}
                                // HKEY_CLASSES_ROOT\*\shellex\ContextMenuHandlers\ListXP
                                SHDeleteKey (LISTREGROOTKEY, LISTREGSUBKEY);
                                SHDeleteKey (HKEY_CLASSES_ROOT, "CLSID\\" CLSID_LISTXPSHORTCUTMENU_GUIDSTRING);
                                SHDeleteKey (HKEY_CLASSES_ROOT, "*\\shellex\\ContextMenuHandlers\\ListXP");

                                MessageBox (pico->hwnd,
                                    "ListXP's registry settings and shell integration have been removed.",
                                    "Done",
                                    MB_OK);
                            }
                        }                        
                    }
                }
            }

            break;

        default:
            return (E_INVALIDARG);
    }

    return (NOERROR);
}


STDMETHODIMP CShortcutMenu::QueryContextMenu (HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    // Do not modify the default verb actions!
    if (!(uFlags & CMF_DEFAULTONLY)  &&  AddMenuItem)
    {
        InsertMenu (hMenu, indexMenu, MF_STRING | MF_BYPOSITION, idCmdFirst + IDM_LIST, "&List");
        return (MAKE_HRESULT (SEVERITY_SUCCESS, 0, USHORT(IDM_LIST + 1)));
    }

    return (MAKE_HRESULT (SEVERITY_SUCCESS, 0, USHORT(0)));
}



