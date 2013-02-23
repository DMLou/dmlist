#include "Updates.h"
#include "ListDefaults.h"
#include "List.h"
#include "ListMisc.h"
#include "CCmdQueue.h"
#include "CThread.h"
#include "wininet.h"
#include "resource.h"
//#pragma comment(lib,"wininet.lib")
#pragma warning(disable:4312)

typedef struct
{
    ListThreadContext *LC;
    string UpdateMessage;
    string LatestVersion;
} UpdatesBoxContext;

typedef BOOL (WINAPI *FP_InternetCloseHandle) (HINTERNET);
typedef HINTERNET (WINAPI *FP_InternetOpenA) (LPCTSTR, DWORD, LPCTSTR, LPCTSTR, DWORD);
typedef HINTERNET (WINAPI *FP_InternetOpenUrlA) (HINTERNET, LPCTSTR, LPCTSTR, DWORD, DWORD, DWORD_PTR);
typedef BOOL (WINAPI *FP_InternetReadFile) (HINTERNET, LPVOID, DWORD, LPDWORD);

typedef struct
{
    FP_InternetCloseHandle InternetCloseHandle;
    FP_InternetOpenA InternetOpenA;
    FP_InternetOpenUrlA InternetOpenUrlA;
    FP_InternetReadFile InternetReadFile;
    HMODULE WinInetDLL;
    int RefCount;
} WinInetFunctions;


bool LoadWinInet (WinInetFunctions &Result)
{
    Result.WinInetDLL = LoadLibrary ("wininet.dll");

    if (Result.WinInetDLL == NULL)
        return false;

    Result.InternetCloseHandle = (FP_InternetCloseHandle) GetProcAddress (Result.WinInetDLL, "InternetCloseHandle");
    Result.InternetOpenA = (FP_InternetOpenA) GetProcAddress (Result.WinInetDLL, "InternetOpenA");
    Result.InternetOpenUrlA = (FP_InternetOpenUrlA) GetProcAddress (Result.WinInetDLL, "InternetOpenUrlA");
    Result.InternetReadFile = (FP_InternetReadFile) GetProcAddress (Result.WinInetDLL, "InternetReadFile");

    if (Result.InternetCloseHandle == NULL ||
        Result.InternetOpenA == NULL ||
        Result.InternetOpenUrlA == NULL ||
        Result.InternetReadFile == NULL)
    {
        FreeLibrary (Result.WinInetDLL);
        return false;
    }

    return true;
}

bool FreeWinInet (WinInetFunctions &FreeMe)
{
    return (bool(FreeLibrary (FreeMe.WinInetDLL)));
}    


string FixNewlines (const string &input)
{
    string ret;
    string::const_iterator it;

    for (it = input.begin(); it != input.end(); ++it)
    {
        if (*it == '\n')
            ret.push_back ('\r');

        ret.push_back (*it);
    }

    return (ret);
}


INT_PTR CALLBACK UpdatesBox (HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam)
{
    guard
    {
        UpdatesBoxContext *UC;
        string Message;
        string::iterator sit;

        UC = (UpdatesBoxContext *) GetWindowLongPtr (Hwnd, GWLP_USERDATA);

        switch (Msg)
        {
            case WM_INITDIALOG:
                bool UpdatesEnabled;

                CenterWindow (Hwnd);
                SetWindowLongPtr (Hwnd, GWLP_USERDATA, (LONG_PTR)LParam);
                UC = (UpdatesBoxContext *) GetWindowLongPtr (Hwnd, GWLP_USERDATA);

                Message = FixNewlines (UC->UpdateMessage);

                SetWindowText (GetDlgItem (Hwnd, IDC_LATESTVERSION), UC->LatestVersion.c_str());
                SetWindowText (GetDlgItem (Hwnd, IDC_YOURVERSION), GlobalInfo.GetValString ("Version").c_str());
                SetWindowText (GetDlgItem (Hwnd, IDC_UPDATEMESSAGE), Message.c_str());

                UpdatesEnabled = bool(UC->LC->SettingsV2.GetValUint32("UpdatesEnabled"));
                SendDlgItemMessage (Hwnd, IDC_DISABLEUPDATES, BM_SETCHECK, 
                     UpdatesEnabled ? BST_UNCHECKED : BST_CHECKED, 0);

                if (!UpdatesEnabled)
                    SetWindowText (GetDlgItem (Hwnd, IDC_REMINDLATER), "Close");

                break;

            case WM_COMMAND:
                switch (LOWORD(WParam))
                {
                    case IDC_DISABLEUPDATES:
                        switch (SendDlgItemMessage (Hwnd, IDC_DISABLEUPDATES, BM_GETCHECK, 0, 0))
                        {
                            default:
                            case BST_UNCHECKED:
                                SetWindowText (GetDlgItem (Hwnd, IDC_REMINDLATER), "Remind Me Later");
                                break;

                            case BST_CHECKED:
                                SetWindowText (GetDlgItem (Hwnd, IDC_REMINDLATER), "Close");
                                break;
                        }

                        break;

                    case IDC_GOTOWEBSITE:
                        RunShell (Hwnd, GlobalInfo.GetValString("Website").c_str()); // security badness!?!?!?!

                    case IDCANCEL:
                    case ID_CANCEL:
                    case IDC_REMINDLATER:
                        UC->LC->SettingsV2.SetValUint32 ("UpdatesEnabled", uint32(BST_UNCHECKED == SendDlgItemMessage (Hwnd, IDC_DISABLEUPDATES, BM_GETCHECK, 0, 0)));
                        EndDialog (Hwnd, 0);
                        break;
                }

                break;
        }

        return (FALSE);
    } unguard;
}


// This is from my IDL project.
bool DownloadFile (WinInetFunctions &WinInet, HINTERNET HInternet, const char *Url, char **Output, DWORD *ByteSize)
{
    guard
    {
        HANDLE WebPage;
        char TempBuffer[4096];
        char *Buffer = NULL;
        DWORD WriteHere = 0;
        DWORD AmtRead;
        DWORD BufferSize = 0;

        printf ("Downloading %s ", Url);
        WebPage = WinInet.InternetOpenUrlA (HInternet, Url, NULL, NULL, INTERNET_FLAG_RELOAD, NULL);
        
        if (WebPage == NULL)
        {
            printf ("failed\n");
            return (false);
        }

        Buffer = (char *) malloc (1);
        WriteHere = 0;
        BufferSize = 1;

        while (true)
        {
            BOOL Result;

            Result = WinInet.InternetReadFile (WebPage, TempBuffer, sizeof (TempBuffer), &AmtRead);

            if (Result != TRUE || AmtRead == 0)
                break;
            else
            {
                BufferSize += AmtRead;
                Buffer = (char *) realloc (Buffer, BufferSize);
                StreamCopy (Buffer + WriteHere, TempBuffer, AmtRead);
                WriteHere += AmtRead;
                printf (".");
            }
        }

        Buffer[BufferSize - 1] = '\0'; // null terminateify the string
        printf (" (%u bytes)\n", BufferSize);
        WinInet.InternetCloseHandle (WebPage);
        *Output = Buffer;
        *ByteSize = BufferSize;

        return (true);
    } unguard;
}

// Return value is the contents of the URL
string GetInternetFile (const string &Name)
{
    guard
    {
        HINTERNET Internet;
        DWORD Size;
        char *Buffer;
        string ReturnString;
        WinInetFunctions WinInet;

        if (!LoadWinInet (WinInet))
            return (string(""));

        Internet = WinInet.InternetOpenA ("ListXP Update Checker", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, NULL);

        if (!DownloadFile (WinInet, Internet, Name.c_str(), &Buffer, &Size))
        {
            WinInet.InternetCloseHandle (Internet);
            return (string(""));
        }

        ReturnString = string(Buffer);
        free (Buffer);
        FreeWinInet (WinInet);
        return (ReturnString);
    } unguard;
}


stringvec ParseForLines (string &Text)
{
    guard
    {
        stringvec ReturnList;
        string Line;
        string::iterator it;

        Line = string("");

        for (it = Text.begin(); it != Text.end(); ++it)
        {
            switch (*it)
            {
                case '\n':
                case '\r':
                    ReturnList.push_back (Line);
                    Line = string ("");
                    break;

                default:
                    Line.append (1, *it);
                    break;
            }
        }

        return (ReturnList);
    } unguard;
}


string GetSubStr (string &src, int start, int maxlen)
{
    guard
    {
        string::iterator sit;
        string Return ("");
        int length = 0;

        for (sit = src.begin() + start; sit < src.end()  &&  length < maxlen; ++sit, ++length)
            Return.append (1, *sit);

        return (Return);
    } unguard;
}


bool GetUpdateInfo (string &LatestVersion, string &UpdateMessage)
{
    guard
    {
        bool ReturnVal = false;
        stringvec Text;
        stringvec::iterator it;
        string FileName;

        FileName = GlobalInfo.GetValString("Website") + string("/") + GlobalInfo.GetValString("UpdatesFile");
        Text = ParseForLines (GetInternetFile (FileName));

        for (it = Text.begin(); it < Text.end(); it++)
        {
            string &Line = *it;

            if (Line.find ("LatestVersion=") == 0)
            {
                LatestVersion = Line.substr (14, 128);
                //LatestVersion = GetSubStr (Line, 14, 128);
                ReturnVal = true;
            }
            else
            // In the message, we use ` as a line break
            // NOTE: Reason we use "Message2" is because in v0.90 I found a BUG (see FixNewlines)
            // in my code for handling \n --> \r\n conversion.
            // So old versions will always get "Message=..." (some very short string without any
            // newlines)
            if (Line.find ("Message2=") == 0)
            {
                UpdateMessage = Line.substr (9, 1024);
                replace (UpdateMessage.begin(), UpdateMessage.end(), '`', '\n');
            }
        }

        return (ReturnVal);
    } unguard;
}


void BugUserAboutUpdate (HWND Parent, ListThreadContext *LC, const string &NewVersion, const string &UpdateMessage)
{
    guard
    {
        UpdatesBoxContext Context;

        Context.LatestVersion = NewVersion;
        Context.UpdateMessage = UpdateMessage;
        Context.LC = LC;

        DialogBoxParam (GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_UPDATE), Parent, UpdatesBox, (LPARAM)&Context);

        return;
    } unguard;
}


class CUpdateThread : public CThread
{
public:
    CUpdateThread (ListThreadContext *LC, bool DoNotify)
    {
        guard
        {
            CUpdateThread::LC = LC;
            CUpdateThread::DoNotify = DoNotify;
            FoundUpdate = false;
            return;
        } unguard;
    }

    bool ThreadFunction (void)
    {
        guard
        {
            if (GetUpdateInfo (LatestVersion, Message))
            {
                if (LatestVersion != GlobalInfo.GetValString ("Version"))
                {
                    if (DoNotify)
                    {
                        UpdateInfo *Info;

                        Info = new UpdateInfo;
                        Info->Message = Message;
                        Info->NewVersion = LatestVersion;
                        LC->CmdQueue.SendCommandAsync (CCommand (LCMD_UPDATE_AVAILABLE, 0, 0, 0, 0, (void *)Info));
                    }

                    FoundUpdate = true;
                }

                LC->SettingsV2.SetValUint64 ("UpdatesLastChecked", GetTime64());
            }

            return (true);
        } unguard;
    }

    bool DoNotify;
    bool FoundUpdate;
    string Message;
    string LatestVersion;

private:
    ListThreadContext *LC;
};


void CheckForUpdate (ListThreadContext *LC, 
                     bool OverrideEnable, 
                     bool Async, 
                     bool *FoundUpdateResult,
                     UpdateInfo *InfoResult)
{
    guard
    {
        uint64 Now;
        uint64 Then;

        if (!OverrideEnable  &&  LC->SettingsV2.GetValUint32 ("UpdatesEnabled") == 0)
        {
            if (FoundUpdateResult != NULL)
                *FoundUpdateResult = false;

            return; // no checking!
        }

        Then = LC->SettingsV2.GetValUint64 ("UpdatesLastChecked");
        Now = GetTime64 ();

        if (OverrideEnable  ||
            TimeDiffDays (Now, Then) > LC->SettingsV2.GetValUint32 ("UpdatesCheckInterval"))
        {
            CUpdateThread *UpdateThread;

            // update checking!
            if (!OverrideEnable && LC->SettingsV2.GetValUint32 ("UpdatesFirstTime") == 1)
            {
                int Result;

                Result = MessageBox (
                    LC->Screen->GetHWND(),
                    "ListXP is about to check to see if an update is available. This only\n"
                    "involves downloading a small text file from the ListXP website.\n"
                    "\n"
                    "Do you want to want to allow this? Clicking 'No' will disable this feature.",
                    "Auto-Notification of Updates",
                    MB_YESNO);

                LC->SettingsV2.SetValUint32 ("UpdatesFirstTime", 0);

                if (Result == IDNO)
                {
                    if (FoundUpdateResult != NULL)
                        *FoundUpdateResult = false;

                    LC->SettingsV2.SetValUint32 ("UpdatesEnabled", 0);
                    return; // do not check
                }
            }

            UpdateThread = new CUpdateThread (LC, Async);

            if (Async)
            {
                if (!UpdateThread->RunThread ())
                    delete UpdateThread;
            }
            else
            {
                UpdateThread->ThreadFunction ();

                if (FoundUpdateResult != NULL)
                    *FoundUpdateResult = UpdateThread->FoundUpdate;

                if (InfoResult != NULL)
                {
                    InfoResult->Message = UpdateThread->Message;
                    InfoResult->NewVersion = UpdateThread->LatestVersion;
                }

                delete UpdateThread;
            }
        }
        else
        {
            if (FoundUpdateResult != NULL)
                *FoundUpdateResult = false;
        }

        return;
    } unguard;
}
