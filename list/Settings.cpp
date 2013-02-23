#include "Settings.h"
#include "Hive.h"
#include <shlwapi.h>
#include <shlobj.h>
#include "CFileLinesDB.h"
#pragma comment(lib,"shlwapi.lib")


// Applies settings from VarList&Settings, and then copies those settings into LC->SettingsV2
// Assumes variables are already validated to correct value ranges
void ApplySettingsV2 (ListThreadContext *LC, VarListExt &NewSettings)
{
    guard
    {
        VarListExt Settings;
        bool DoFileReset = false;
        uint32 Multiplier = 0;
        string Unit;

        // Essentially we will look for settings first in NewSettings, then in LC->SettingsV2, then in GlobalDefaults
        Settings = NewSettings;
        Settings.SetDefaultsList (&LC->SettingsV2);

        SetClassLongPtr (LC->Screen->GetHWND(), GCLP_HBRBACKGROUND, (LONG_PTR) CreateSolidBrush 
            (Settings.GetValUint32 ("TextColor.Background")));

        // SIMD?
        if (1 == Settings.GetValUint32 ("DisableSIMD"))
        {
            HaveMMX = false;
            HaveSSE = false;
            HaveSSE2 = false;
        }
        else
        {
            CanUseStreamingCopy ();
        }

        // Apply font
        LC->Screen->GetGDIOutput()->SetFont ((char *)Settings.GetValString ("FontName").c_str(), Settings.GetValUint32("FontSize"));
        LC->Screen->GetGDIOutput()->FitConsoleInParent ();

        // Apply tailing values
        Unit = Settings.GetValString("TailingIntervalUnit");
        if (Unit == "milliseconds")
            Multiplier = 1;
        else
        if (Unit == "seconds")
            Multiplier = 1000;
        else
        if (Unit == "minutes")
            Multiplier = 1000 * 60;
        else
        if (Unit == "hours")
            Multiplier = 1000 * 60 * 60;
        else
        if (Unit == "days")
            Multiplier = 1000 * 60 * 60 * 24;

        LC->SizeCheckInterval = Multiplier * Settings.GetValUint32 ("TailingInterval");

        // Maximized?
        if (Settings.GetValUint32 ("Maximized") == 1  ||  Settings.GetValUint32 ("AlwaysMaximized") == 1)
        {
            ShowWindow (LC->Screen->GetHWND(), SW_SHOWMAXIMIZED);
        }
        else
        if (Settings.GetValUint32 ("Width") != LC->Screen->GetConsWidth()  ||  
            Settings.GetValUint32 ("Height") != LC->Screen->GetConsHeight())
        {
            ListResizeInfo *Info = new ListResizeInfo;

            Info->NewWidth = Settings.GetValUint32 ("Width");
            Info->NewHeight = Settings.GetValUint32 ("Height");
            Info->FudgeX = false;
            Info->FudgeY = false;
            Info->Maximizing = false;

            LC->CmdQueue.ReplaceCommandUrgentAsync (LCMD_RESIZEXY, CCommand (LCMD_RESIZEXY, 0, 0, 0, 0, (void *)Info));
            LC->Screen->GetGDIOutput()->FitConsoleInParent ();
        }

        if ((Settings.GetValUint32("WrapText") == 1  &&  LC->FileContext->WrapText == false) ||
            (Settings.GetValUint32("WrapText") == 0  &&  LC->FileContext->WrapText == true))
        {
            LC->CmdQueue.SendCommandAsync (CCommand (LCMD_SET_LINE_WRAPPING, 
                Settings.GetValUint32 ("WrapText"), 0, 0, 0, NULL));
        }

        if (Settings.GetValUint32("TabSize") != LC->FileContext->NormalLinesDB->GetTabSize())
        {
            LC->CmdQueue.SendCommandAsync (CCommand (LCMD_SET_TABSIZE,
                Settings.GetValUint32 ("TabSize"), 0, 0, 0, NULL));
        }

        SetWindowAlphaValues (LC->Screen->GetHWND(), 
            Settings.GetValUint32 ("Transparent"), 
            Settings.GetValUint32 ("AlphaLevel"));

        // File cache settings
    #define COMPRESET(name,type) \
        if (Settings.GetVal##type (name) != LC->SettingsV2.GetVal##type (name)) \
        { \
            LC->SettingsV2.SetVal##type (name, Settings.GetVal##type (name)); \
            DoFileReset = true; \
        }

        COMPRESET("LineCacheSize",Uint32);
        COMPRESET("SeekGranularityLog2",Uint32);

        // Others
        SetHivePath (Settings.GetValString ("HivePath"));
        SetHiveEnabled (bool(Settings.GetValUint32 ("HiveEnabled")));
        SetHiveMinSize (Settings.GetValUint64 ("HiveMinSizeKB") * 1024);
        SetHiveCompression (bool(LC->SettingsV2.GetValUint32 ("HiveCompression")));

        LC->SettingsV2.Union (Settings);

        if (DoFileReset)
            LC->CmdQueue.ReplaceCommandAsync (LCMD_FILE_RESET, CCommand (LCMD_FILE_RESET, 0, 0, 0, 0, NULL));

        AddSimpleCommand (LC, LCMD_UPDATE_TEXT);
        AddSimpleCommand (LC, LCMD_UPDATE_INFO);

        return;
    } unguard;
}


void SaveSettingsV2 (ListThreadContext *LC)
{
    guard
    {
        RECT rect;

        LC->SettingsV2.SetValUint32 ("Maximized", IsZoomed (LC->Screen->GetHWND()) ? 1 : 0);

        if (LC->SettingsV2.GetValUint32 ("Maximized") == 0)
        {
            if (GetWindowRect(LC->Screen->GetHWND(), &rect))
            {
                LC->SettingsV2.SetValString ("X", tostring(rect.left));
                LC->SettingsV2.SetValString ("Y", tostring(rect.top));
            }
        }

        LC->SettingsV2.SaveVarsToReg ();
        return;
    } unguard;
}


void SetToDefaultsV2 (VarList &Settings, VarList &Defaults)
{
    guard
    {
        Settings.Union (Defaults);
        return;
    } unguard;
}


// Returns true or false depending on if shell integration is enabled or not
bool GetShellIntegration (void)
{
    guard
    {
        HKEY Key;
        LONG Result;
        bool ReturnVal;

        Result = RegOpenKeyEx (HKEY_CLASSES_ROOT, "*\\shellex\\ContextMenuHandlers\\ListXP", 0, KEY_READ, &Key);

        if (Result == ERROR_SUCCESS)
            ReturnVal = true;
        else
            ReturnVal = false;

        RegCloseKey (Key);
        return (ReturnVal);
    } unguard;
}


#include "ListShellDLL.h"
void DumpShellDLL (const char *FileName)
{
    guard
    {
        FILE *out;

        out = fopen (FileName, "wb");
        if (out != NULL)
        {
            fwrite (ListShellDLLFile, 1, ListShellDLLFileSize, out);
            fclose (out);
        }

        return;
    } unguard;
}


bool SetRegistryStringValue (HKEY RootKey, const char *SubKeyName, const char *ValueName, const char *StringValue)
{
    guard
    {
        HKEY Key;
        LONG Result;

        Result = RegCreateKeyEx (RootKey, SubKeyName, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &Key, NULL);

        if (Result != ERROR_SUCCESS)
            return (false);

        RegSetValueEx (Key, ValueName, 0, REG_SZ, (const BYTE *)StringValue, strlen(StringValue) + 1);
        RegCloseKey (Key);
        return (true);    
    } unguard;
}


#include "ListShell\GUID.h"
#define SHELLDLLNAME "ListShell.dll"


bool SetShellIntegration (bool Enabled)
{
    guard
    {
        bool ReturnVal;

        if (Enabled)
        {
            HKEY Key;
            LONG Result;
            char String[2048];
            char EXEName[1024];
            char DLLPath[2048];
            char *p;

            GetModuleFileName (GetModuleHandle (NULL), EXEName, sizeof (EXEName));
            p = strrchr (EXEName, '\\');
            *p = '\0';
            sprintf (DLLPath, "%s\\" SHELLDLLNAME "", EXEName);
            DumpShellDLL (DLLPath);
            //sprintf (DLLPath, "\"%s\\" SHELLDLLNAME "\"", EXEName);

            ReturnVal = 
                SetRegistryStringValue (HKEY_CLASSES_ROOT, "CLSID\\" CLSID_LISTXPSHORTCUTMENU_GUIDSTRING, "", "ListXP")
                && SetRegistryStringValue (HKEY_CLASSES_ROOT, "CLSID\\" CLSID_LISTXPSHORTCUTMENU_GUIDSTRING "\\InProcServer32", "", DLLPath)
                && SetRegistryStringValue (HKEY_CLASSES_ROOT, "CLSID\\" CLSID_LISTXPSHORTCUTMENU_GUIDSTRING "\\InProcServer32", "ThreadingModel", "Apartment")
                && SetRegistryStringValue (HKEY_CLASSES_ROOT, "ListXP\\ShellEx\\ContextMenuHandlers", "", CLSID_LISTXPSHORTCUTMENU_GUIDSTRING)
                && SetRegistryStringValue (HKEY_CLASSES_ROOT, "*\\shellex\\ContextMenuHandlers\\ListXP", "", CLSID_LISTXPSHORTCUTMENU_GUIDSTRING);
        }
        else
        {
            DWORD R1, R2, R3;

            R1 = SHDeleteKey (HKEY_CLASSES_ROOT, "CLSID\\" CLSID_LISTXPSHORTCUTMENU_GUIDSTRING);
            R2 = SHDeleteKey (HKEY_CLASSES_ROOT, "ListXP\\ShellEx\\ContextMenuHandlers");
            R3 = SHDeleteKey (HKEY_CLASSES_ROOT, "*\\shellex\\ContextMenuHandlers\\ListXP");

            if (R1 == ERROR_SUCCESS  &&  R1 == R2  &&  R2 == R3)
                ReturnVal = true;
            else
                ReturnVal = false;
        }

        SHChangeNotify (SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
        return (ReturnVal);
    } unguard;
}


