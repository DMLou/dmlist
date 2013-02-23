/*****************************************************************************

  In the About box, we show a neat logo picture, copyright info, and
  some "shout outs." If we are running in Windows XP we also show the URL
  and e-mail address as clickable links.

*****************************************************************************/

#define _WIN32_WINNT 0x0501 
#define WINVER 0x0501

#include "AboutBox.h"
#include "resource.h"
#include "commctrl.h"
#include "ShellApi.h"
#include "BuildNumber.h"
#include <sstream>

#pragma warning(disable:4312)

string GetLinkText (string LinkText, string LinkName, bool Link)
{
    if (!Link)
        return (LinkText);
    else
    {
        stringstream ss;

        ss << "<A ID=\"" << LinkName << "\">" << LinkText << "</A>";
        return (ss.str());
    }
}

string GetCPUText (void)
{
    stringstream CPUText;

    CPUText << "Instruction path: " /*<< CPUCount << "x "*/;

    if (HaveSSE2)
        CPUText << "SIMD SSE2 (Pentium 4/Xeon/Opteron)";
    else
    if (HaveSSE)
        CPUText << "SIMD SSE (Pentium III/Athlon XP)";
    else
    if (HaveMMX)
        CPUText << "SIMD MMX (Pentium MMX/II/Athlon)";
    else
        CPUText << "Scalar x86 (486, Pentium)";

    return (CPUText.str());
}

string AboutText (bool Links)
{
    stringstream s;

    s << GlobalInfo.GetValString ("FullName") << " Build " << ListXPBuildNumber << "\n"
      << "Copyright 2002, 2003 Rick Brewster\n"
      << "Download at: " << GetLinkText (GlobalInfo.GetValString ("Website"), "homepage", Links) << "\n"
      << "\n"
      << "If you have a question, a suggestion, or if you just want to send me a "
      << "message, my e-mail is " << GetLinkText (GlobalInfo.GetValString("E-mail"), "email", Links) << ". "
      << "Thanks to Andrew Kent (aka Hunam) for the domain and website hosting, "
      << "and DrConway for extensive testing.\n"
      << "\n"
      << "This software uses the PCRE library for regular expression handling (" << GetLinkText (GlobalInfo.GetValString ("Credits.PCREPage"), "pcrepage", Links) 
      << ").\n"
      << "\n"
      << "AVL Search Tree code adapted from the Insitute of Applied Iconoclasm (" << GetLinkText (GlobalInfo.GetValString ("Credits.AVLPage"), "avlpage", Links) << ").\n"
      << "\n"
      << "Original icon artwork by Christhian of thirdangel design (" << GetLinkText (GlobalInfo.GetValString ("Credits.IconPage"), "iconpage", Links) << ").\n"
      << "\n"
      << GetCPUText ();

    return (s.str());
}

string AboutTextStandard (void)
{
    return AboutText (false);
}

string AboutTextLink (void)
{
    return AboutText (true);
}

//#define ABOUTTEXT_STD ABOUTTEXT(LISTHOMEPAGE, LISTEMAIL)
//#define ABOUTTEXT_LNK ABOUTTEXT("<A ID=\"homepage\">" LISTHOMEPAGE "</A>", "<A ID=\"email\">" LISTEMAIL "</A>")


typedef struct
{
    HWND AboutSysLink;
} AboutContext;


typedef BOOL (WINAPI *InitCCExFnType) (LPINITCOMMONCONTROLSEX);


void GotoURL (HWND Hwnd, string PageRef)
{
    stringstream cmd;

    cmd << "\"" 
        << GlobalInfo.GetValString (PageRef) 
        << "\"";

    RunShell (Hwnd, cmd.str().c_str());
    return;
}

// Convert ANSI string to UNICODE
void StrCpyA2W (wchar_t *Dst, char *Src)
{
    wchar_t *d;
    char *s;

    d = Dst;
    s = Src;

    while (*s != '\0')
    {
        *d = (wchar_t) *s;
        d++;
        s++;
    }

    *d = 0;

    return;
}


// Inits common controls to enable the SysLink class. Assumes COM has been initialized with CoInitialize
bool InitSysLink (void)
{
    HMODULE ComDLL;
    InitCCExFnType InitCCExFn;
    INITCOMMONCONTROLSEX ICCex;
    BOOL Result;

    ComDLL = LoadLibrary ("Comctl32.dll");

    if (ComDLL == NULL)
        return (false);

    InitCCExFn = (InitCCExFnType) GetProcAddress (ComDLL, "InitCommonControlsEx");

    if (InitCCExFn == NULL)
    {
        FreeLibrary (ComDLL);
        return (false);
    }

    ICCex.dwSize = sizeof (ICCex);
    ICCex.dwICC = ICC_LINK_CLASS;

    Result = InitCCExFn (&ICCex);

    FreeLibrary (ComDLL);

    if (Result == TRUE)
        return (true);

    return (false);
}


INT_PTR CALLBACK AboutBoxDialog (HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam)
{
    AboutContext *Context;
    stringstream CPUText;
    string AboutText;

    Context = (AboutContext *) GetWindowLongPtr (Hwnd, GWLP_USERDATA);

    switch (Msg)
    {
        case WM_INITDIALOG:
            Context = new AboutContext;
            Context->AboutSysLink = NULL;

            AboutText = AboutTextStandard();
            SendDlgItemMessage (Hwnd, IDC_LISTXPABOUTTEXT, WM_SETTEXT, NULL, (LPARAM) AboutText.c_str());

            // Try for syslink class. If we can use it, then we hide the normal text box and 
            // overlay the SysLink control over the top of it.
            if (InitSysLink())
            {
                WINDOWPLACEMENT Placement;
                string SysLinkTxt;
                wstring SysLinkTxtW;

                SysLinkTxt = AboutTextLink();

                //StrCpyA2W (SysLinkTxtW, SysLinkTxt);
                transform (SysLinkTxt.begin(), SysLinkTxt.end(),
                           back_inserter(SysLinkTxtW), typecast_fn<char,wchar_t>());

                Placement.length = sizeof (Placement);
                GetWindowPlacement (GetDlgItem (Hwnd, IDC_LISTXPABOUTTEXT), &Placement);

                Context->AboutSysLink = CreateWindowW (WC_LINK, 
                                                       SysLinkTxtW.c_str(), 
                                                       WS_TABSTOP | WS_VISIBLE | WS_CHILD, 
                                                       0, 0, 
                                                       1, 1, 
                                                       Hwnd, 
                                                       NULL, NULL, NULL);

                if (Context->AboutSysLink != NULL)
                {
                    ShowWindow (GetDlgItem (Hwnd, IDC_LISTXPABOUTTEXT), SW_HIDE);
                    SetWindowPlacement (Context->AboutSysLink, &Placement);
                    ShowWindow (Context->AboutSysLink, SW_SHOW);
                }
            }

            CenterWindow (Hwnd);
            return (TRUE);

        case WM_SYSCOMMAND:
            switch (WParam)
            {
                case SC_CLOSE:
                    if (Context != NULL)
                    {
                        delete Context;
                        SetWindowLongPtr (Hwnd, GWLP_USERDATA, NULL);
                    }

                    EndDialog (Hwnd, 0);
                    return (0);
            }

            return (0);

        case WM_COMMAND:
            switch (LOWORD(WParam))
            {
                case IDCANCEL:
                case ID_CANCEL:
                case ID_OK:
                    if (Context != NULL)
                    {
                        delete Context;
                        SetWindowLongPtr (Hwnd, GWLP_USERDATA, NULL);
                    }

                    EndDialog (Hwnd, 0);
                    return (0);
            }

            return (1);

        case WM_NOTIFY:
            int idCtrl = (int) WParam;
            LPNMHDR lpnmh = (LPNMHDR) LParam;

            switch (lpnmh->code)
            {
                case NM_RETURN:
                case NM_CLICK:
                    PNMLINK Link = (PNMLINK) LParam;

                    if (0 == wcscmp (Link->item.szID, L"homepage"))
                        GotoURL (Hwnd, "Website");
                    else
                    if (0 == wcscmp (Link->item.szID, L"pcrepage"))
                        GotoURL (Hwnd, "Credits.PCREPage");
                    else
                    if (0 == wcscmp (Link->item.szID, L"avlpage"))
                        GotoURL (Hwnd, "Credits.AVLPage");
                    else
                    if (0 == wcscmp (Link->item.szID, L"iconpage"))
                        GotoURL (Hwnd, "Credits.IconPage");
                    else
                    if (0 == wcscmp (Link->item.szID, L"email"))
                    {
                        stringstream cmd;

                        cmd << "\"mailto:" 
                            << GlobalInfo.GetValString("E-mail") 
                            << "?subject=" 
                            << GlobalInfo.GetValString("ProgramName") 
                            << " " 
                            << GlobalInfo.GetValString("Version") 
                            << "\"";

                        RunShell (Hwnd, cmd.str().c_str());
                    }

                    break;
            }

            return (0);

    }

    return (FALSE);
}

