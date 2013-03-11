#include "List.h"
#include "ListMisc.h"
#include "PreferencesV2.h"
#include "resource.h"
#include "commctrl.h"
#include "commdlg.h"
#include "Settings.h"
#include "Hive.h" 
#include "Updates.h"
#include <iostream>
#include <streambuf>
#include <cstdint>

#pragma warning(disable:4312)
// debugging
//#define ENABLE_PREFS_LOGGING

#ifdef ENABLE_PREFS_LOGGING

#include <fstream>
#include <stack>

#pragma message ("Preferences logging is enabled!")
ofstream _prefslog ("c:/temp/prefsv2.log");

class __prefslog
{
public:
    __prefslog (ostream &stream_init)
        : stream (stream_init),
          indent (0)
    {
        guard
        {
            return;
        } unguard;
    }

    template<class T>
    ostream &operator<< (const T &rhs)
    {
        guard
        {
            stream << string(max (0, indent) * 2, ' ');
            stream << rhs;
            return (stream);
        } unguard;
    }

    __prefslog &enter (const string &name)
    {
        guard
        {
            (*this) << "Entering: " << name << endl;
            indent++;
            trace.push (name);
            return (*this);
        } unguard;
    }

    ostream &enter ()
    {
        guard
        {
            (*this) << "Entering: ";
            trace.push ("(anon)");
            return (stream);
        } unguard;
    }

    __prefslog &leave (void)
    {
        guard
        {
            indent--;

            if (indent < 0)
                indent = 0;

            (*this) << "Leaving: " << trace.top () << endl;
            trace.pop ();
            return (*this);
        } unguard;
    }

private:
    ostream &stream;
    stack<string> trace;
    int indent;
};

__prefslog prefslog (_prefslog);

#else
// blank

#pragma message ("No preferences logging")

class null_streambuf 
    : public basic_streambuf <char, char_traits<char> >
{
public:
    null_streambuf ()
        : basic_streambuf <char, char_traits<char> > ()
    {
        return;
    }
};

ostream cnul (new null_streambuf());

class __prefslog
{ 
public:
    __prefslog ()
    {
        guard
        {
            return;
        } unguard;
    }

    template<class T>
    ostream &operator<< (const T &rhs)
    {
        guard
        {
            return (cnul);
        } unguard;
    }

    ostream &enter (void)
    {
        guard
        {
            return (cnul);
        } unguard;
    }

    __prefslog &enter (const string &name)
    {
        guard
        {
            return (*this);
        } unguard;
    }

    __prefslog &leave (void)
    {
        guard
        {
            return (*this);
        } unguard;
    }
};

__prefslog prefslog;

#endif

static int CALLBACK BuildFontList (const ENUMLOGFONTEX *lpelfe,
                                   const NEWTEXTMETRICEX *lpntme,
                                   DWORD FontType,         
                                   LPARAM lParam)
{
    stringvec &fontNames = *((stringvec *)lParam);

    if (((lpelfe->elfLogFont.lfPitchAndFamily & 0x3) == FIXED_PITCH)  &&
        ((lpelfe->elfLogFont.lfCharSet == OEM_CHARSET) || (lpelfe->elfLogFont.lfCharSet == ANSI_CHARSET))
        )
    {
        string name = (char *)lpelfe->elfFullName;

        if (name[0] != '@' && (fontNames.end() == find (fontNames.begin(), fontNames.end(), name)))
        {
            fontNames.push_back (name);
        }
    }

    return (1);
}


stringvec GetFontList (HDC hdc)
{
    if (::GetVersion() > 0x80000000)
    {   // HACK: Win9x doesn't seem to like the way I enumerate fonts ... ?!?!?!
        stringvec fontNames;

        fontNames.push_back ("Terminal");
        fontNames.push_back ("Courier");
        fontNames.push_back ("Courier New");
        fontNames.push_back ("Lucida Console");
        fontNames.push_back ("FixedSys");

        return (fontNames);
    }
    else
    {
        stringvec fontNames;
        LOGFONT logFont;
        int result;

        ZeroMemory (&logFont, sizeof(logFont));
        logFont.lfCharSet = DEFAULT_CHARSET;
        logFont.lfFaceName[0] = '\0';
        logFont.lfPitchAndFamily = FIXED_PITCH | FF_DECORATIVE | FF_DONTCARE | FF_MODERN | FF_ROMAN | FF_SCRIPT | FF_SWISS;

        result = EnumFontFamiliesExA (hdc, &logFont, (FONTENUMPROC)BuildFontList, (LPARAM)&fontNames, 0);
        return (fontNames);
    }
}


class SectionInfo;
class PrefsContext;


INT_PTR CALLBACK GenericSectionProc (HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam);


typedef enum
{
    ControlButton,
    ControlCheckBox,
    ControlComboBox,
    ControlEditAndSpin,  // ID = edit, ID2 = spin
    ControlSpin,
    ControlText,         // represents a static text control that is simply loaded from a variable; not editable directly
    ControlVarOnly,      // doesn't actually have a control associated with it, this is simple here so you can associate a variable with a properties page for the purposes of the Defaults button

    ControlOther,
    ControlInvalid
} ControlType;

template<class ST>
ST &operator<< (ST &stream, const ControlType &ctl)
{
    guard
    {
        switch (ctl)
        {
    #define CASENAME(n) case n: stream << #n; break;
        CASENAME (ControlButton)
        CASENAME (ControlCheckBox)
        CASENAME (ControlComboBox)
        CASENAME (ControlEditAndSpin)  
        CASENAME (ControlSpin)
        CASENAME (ControlText)         
        CASENAME (ControlVarOnly)      
        CASENAME (ControlOther)
        CASENAME (ControlInvalid)
    #undef CTLNAME

        default:
            stream << "(unknown)";
            break;
        }

        return (stream);
    } unguard;
}

// Special function called on button click events
typedef enum
{
    EventClicked,
    EventEdited,
    EventRefresh,
    EventApply,
    EventInit,
    EventDestroy,

} ControlEvent;

template<class ST>
ST &operator<< (ST &stream, const ControlEvent &ctl)
{
    guard
    {
        switch (ctl)
        {
        #define CASENAME(n) case n: stream << #n; break;
            CASENAME (EventClicked)
            CASENAME (EventEdited)
            CASENAME (EventRefresh)
            CASENAME (EventApply)  
            CASENAME (EventInit)
            CASENAME (EventDestroy)         
        #undef CASENAME

        default:
            stream << "(unknown)";
            break;
        }

        return (stream);
    } unguard;
}



// Called when the button is clicked
typedef void (*ControlEventProc) (PrefsContext *Context, int Sec, int Ctl, ControlEvent Event);


// Section picker functions.
void    PickerInit (PrefsContext *Context);
void    PickerSetSection (PrefsContext *Context, int Index);
bool    PickerFindVariable (PrefsContext *Context, const string &Name, int &SectionIndexResult, int &ControlIndexResult);
bool    PickerSetVariable (PrefsContext *Context, const string &Name, const string &Value);
LRESULT PickerSendControlMessage (PrefsContext *Context, int Section, int Control, UINT Msg, WPARAM WParam, LPARAM LParam);
void    PickerSendControlEvent (PrefsContext *Context, int Section, int Control, ControlEvent Event);
void    PickerBroadcastEvent (PrefsContext *Context, ControlEvent Event);
bool    PickerSendVariableMessage (PrefsContext *Context, const string &Name, UINT Msg, WPARAM WParam, LPARAM LParam, LRESULT &LResult);
bool    PickerFindID (PrefsContext *Context, WORD IDorID2, int &SectionIndexResult, int &ControlIndexResult);
bool    PickerFindVarForID (PrefsContext *Context, WORD IDorID2, string &VarNameResult);


// Functions for working with a delimited attribute list, (such as FontName's 'validlist' attribute)
int DAGetCount (string &List, char Delimiter = '^');
string DAGetString (string &List, int Which, char Delimiter = '^');


// Returns -1 if the string is not in the list, or the ordinal of where the string is (which can be used with DAGetString)
int DAIsInList (string &List, const string &FindMe, char Delimiter = '^');


// Adds the given string to the list, if it is not already there
bool DAInsert (string &List, const string &AddMe, char Delimiter = '^');


//
void HiveBrowseButtonProc      (PrefsContext *Context, int Sec, int Ctl, ControlEvent Event);
void HiveCompressionButtonProc (PrefsContext *Context, int Sec, int Ctl, ControlEvent Event);
void EditBrowseButtonProc      (PrefsContext *Context, int Sec, int Ctl, ControlEvent Event);
void AlphaSpinButtonProc       (PrefsContext *Context, int Sec, int Ctl, ControlEvent Event);
void PickColorButtonProc       (PrefsContext *Context, int Sec, int Ctl, ControlEvent Event);
void HiveClearButtonProc       (PrefsContext *Context, int Sec, int Ctl, ControlEvent Event);
void HiveStatsTextProc         (PrefsContext *Context, int Sec, int Ctl, ControlEvent Event);
void ShellIntButtonProc        (PrefsContext *Context, int Sec, int Ctl, ControlEvent Event);
void UpdateNowButtonProc       (PrefsContext *Context, int Sec, int Ctl, ControlEvent Event);
void UpdateDaysLeftTextProc    (PrefsContext *Context, int Sec, int Ctl, ControlEvent Event);
void FontNameComboBoxProc      (PrefsContext *Context, int Sec, int Ctl, ControlEvent Event);


// Used to associate a variable name to one or two control IDs
class ControlInfo
{
public:
    ControlInfo (string _VarName, ControlType _Type, WORD _ID, WORD _ID2, ControlEventProc _Proc)
    {
        guard
        {
            VarName = _VarName;
            Type = _Type;
            ID = _ID;
            ID2 = _ID2;
            Proc = _Proc;
            return;
        } unguard;
    }

    ControlInfo (const ControlInfo &Info)
    {
        guard
        {
            VarName = Info.VarName;
            Type = Info.Type;
            ID = Info.ID;
            ID2 = Info.ID2;
            Proc = Info.Proc;
            return;
        } unguard;
    }

    friend ostream &operator<< (ostream &stream, const ControlInfo &rhs)
    {
        guard
        {
            stream << "ControlInfo ("
                << "var='" << rhs.VarName << "'"
                << " type=" << rhs.Type
                << " ID=" << rhs.ID
                << " ID2=" << rhs.ID2
                << " Proc=" << rhs.Proc
                << ")";

            return (stream);
        } unguard;
    }

    friend ostream &operator<< (ostream &stream, const vector<ControlInfo> &rhs)
    {
        guard
        {
            vector<ControlInfo>::const_iterator it;

            stream << "vector<ControlInfo> { "
                << "size=" << rhs.size() << " ";

            for (it = rhs.begin(); it != rhs.end(); ++it)
            {
                stream << *it;
                if (it != --rhs.end())
                    stream << ", ";
            }

            stream << " }";
            return (stream);
        } unguard;
    }

    string VarName;
    ControlType Type;
    WORD ID;
    WORD ID2;
    ControlEventProc Proc;
};


// Used to associate a section name, dialog procedure, and window handle
class SectionInfo
{
public:
    SectionInfo (string _Name, WORD _ID, DLGPROC _Proc, HWND _Hwnd, ControlInfo *_Controls, bool _Active = true)
    {
        guard
        {
            int i;

            Name = _Name;
            ID   = _ID;
            Proc = _Proc;
            Hwnd = _Hwnd;
            Active = _Active;
            
            if (_Controls != NULL)
            {
                i = 0;
                while (_Controls[i].Type != ControlInvalid)
                {
                    Controls.push_back (_Controls[i]);
                    i++;
                }
            }

            return;
        } unguard;
    }

    SectionInfo (const SectionInfo &Info)
    {
        guard
        {
            Name   = Info.Name;
            Active = Info.Active;
            ID     = Info.ID;
            Proc   = Info.Proc;
            Hwnd   = Info.Hwnd;
            Controls = Info.Controls;
            return;
        } unguard;
    }

    SectionInfo &operator= (const SectionInfo &rhs)
    {
        guard
        {
            if (this != &rhs)
            {
                Name   = rhs.Name;
                Active = rhs.Active;
                ID     = rhs.ID;
                Proc   = rhs.Proc;
                Hwnd   = rhs.Hwnd;
                Controls = rhs.Controls;
            }

            return (*this);
        } unguard;
    }

    friend ostream &operator<< (ostream &stream, const SectionInfo &rhs)
    {
        guard
        {
            stream << "SectionInfo ("
                << "name='" << rhs.Name << "'"
                << " Active=" << (rhs.Active ? "true" : "false")
                << " ID=" << rhs.ID
                << " Proc=" << rhs.Proc
                << " Hwnd=" << rhs.Hwnd
                << " Controls=" << rhs.Controls
                << ")";

            return (stream);
        } unguard;
    }

    friend ostream &operator<< (ostream &stream, const vector<SectionInfo> &rhs)
    {
        guard
        {
            vector<SectionInfo>::const_iterator it;

            stream << "vector<SectionInfo> { "
                << "size=" << rhs.size() << " ";

            for (it = rhs.begin(); it != rhs.end(); ++it)
            {
                stream << *it;
                if (it != --rhs.end())
                    stream << ", ";
            }

            stream << " }";
            return (stream);
        } unguard;
    }

    string  Name;
    WORD    ID;
    DLGPROC Proc;
    HWND    Hwnd;
    vector<ControlInfo> Controls;
    bool    Active; // when set to true, the dialog procedure will handle all messages,
                    // otherwise it ignores those that would change the values of controls
};


ControlInfo DisplayControls[] =
{
    ControlInfo ("Transparent",          ControlCheckBox,    IDC_TRANSPARENT,     NULL,                AlphaSpinButtonProc),
    ControlInfo ("AlphaLevel",           ControlEditAndSpin, IDC_ALPHALEVEL,      IDC_ALPHALEVEL_SPIN, AlphaSpinButtonProc),
    ControlInfo ("FontName",             ControlComboBox,    IDC_FONTNAME,        NULL,                FontNameComboBoxProc),
    ControlInfo ("FontSize",             ControlEditAndSpin, IDC_FONTSIZE,        IDC_FONTSIZE_SPIN,   NULL),
    ControlInfo ("AlwaysMaximized",      ControlCheckBox,    IDC_ALWAYSMAXIMIZED, NULL,                NULL), 
    ControlInfo ("ColorList",            ControlComboBox,    IDC_COLORLIST,       NULL,                NULL),
    ControlInfo ("",                     ControlButton,      IDC_PICKCOLOR,       NULL,                PickColorButtonProc),
    ControlInfo ("Highlight.Foreground", ControlVarOnly,     NULL,                NULL,                NULL),
    ControlInfo ("Highlight.Background", ControlVarOnly,     NULL,                NULL,                NULL),
    ControlInfo ("TextColor.Foreground", ControlVarOnly,     NULL,                NULL,                NULL),
    ControlInfo ("TextColor.Background", ControlVarOnly,     NULL,                NULL,                NULL),
    ControlInfo ("InfoColor.Foreground", ControlVarOnly,     NULL,                NULL,                NULL),
    ControlInfo ("InfoColor.Background", ControlVarOnly,     NULL,                NULL,                NULL),
    ControlInfo ("_DisplayExplain",      ControlText,        IDC_DISPLAYINFO,     NULL,                NULL),

    ControlInfo ("", ControlInvalid, NULL, NULL, NULL)
};


ControlInfo SearchingControls[] =
{
    ControlInfo ("Highlight.Underline", ControlCheckBox, IDC_HIGHLIGHT_UNDERLINE,        NULL, NULL),
    ControlInfo ("Highlight.Line",      ControlCheckBox, IDC_HIGHLIGHT_ENTIRE_LINE,      NULL, NULL),
    ControlInfo ("Highlight.Terms",     ControlCheckBox, IDC_HIGHLIGHT_INDIVIDUAL_TERMS, NULL, NULL),
    ControlInfo ("AnimatedSearch",       ControlCheckBox,    IDC_ANIMATEDSEARCH,  NULL,                NULL),
    ControlInfo ("_SearchingExplain", ControlText, IDC_SEARCHING_EXPLAIN,                NULL, NULL),

    ControlInfo ("", ControlInvalid, NULL, NULL, NULL)
};


ControlInfo WindowsControls[] =
{
    ControlInfo ("",               ControlCheckBox,    IDC_SHELLINTEGRATION,  NULL, ShellIntButtonProc),
    ControlInfo ("_RMBExplain",    ControlText,        IDC_RMBEXPLAIN,        NULL, NULL),
    ControlInfo ("EditProgram",    ControlText,        IDC_EDITPROGRAM,       NULL, NULL),
    ControlInfo ("",               ControlButton,      IDC_CHANGEEDITPROGRAM, NULL, EditBrowseButtonProc),
    ControlInfo ("_EditExplain",   ControlText,        IDC_EDITEXPLAIN,       NULL, NULL),

    ControlInfo ("", ControlInvalid, NULL, NULL, NULL)
};


ControlInfo ParsingControls[] =
{
    ControlInfo ("DetectParsers",      ControlCheckBox,    IDC_DETECTPARSERS,        NULL,             NULL),
    ControlInfo ("WrapText",           ControlCheckBox,    IDC_WRAPTEXT,             NULL,             NULL),
    ControlInfo ("IgnoreParsedColors", ControlCheckBox,    IDC_IGNORE_PARSED_COLORS, NULL,             NULL),
    ControlInfo ("TabSize",            ControlEditAndSpin, IDC_TABSIZE,              IDC_TABSIZE_SPIN, NULL),

    ControlInfo ("", ControlInvalid, NULL, NULL, NULL)
};


ControlInfo TailingControls[] =
{
    ControlInfo ("TailingInterval",     ControlEditAndSpin, IDC_TAILING_INTERVAL,      IDC_TAILING_INTERVAL_SPIN,  NULL),
    ControlInfo ("TailingIntervalUnit", ControlComboBox,    IDC_TAILING_INTERVAL_UNIT, NULL,                       NULL),
    ControlInfo ("TailingBacktrackKB",  ControlEditAndSpin, IDC_TAILING_BACKTRACK,     IDC_TAILING_BACKTRACK_SPIN, NULL),
    ControlInfo ("TailingForceUpdate",  ControlCheckBox,    IDC_TAILING_FORCE_UPDATE,  NULL,                       NULL),
    ControlInfo ("TailingJumpToEnd",    ControlCheckBox,    IDC_TAILING_JUMP_TO_END,   NULL,                       NULL),
    ControlInfo ("DisableTailWarning",  ControlCheckBox,    IDC_DISABLETAILWARNING,    NULL,                       NULL),
    ControlInfo ("_TailingExplain",     ControlText,        IDC_TAILINGEXPLAIN,        NULL,                       NULL),

    ControlInfo ("", ControlInvalid, NULL, NULL, NULL)
};


ControlInfo CachingControls[] =
{
    ControlInfo ("AllowNonCached",      ControlCheckBox,    IDC_ALLOWNONCACHED,      NULL,                         NULL),
    ControlInfo ("NonCachedMinSizeKB",  ControlEditAndSpin, IDC_NONCACHEDMINSIZE,    IDC_NONCACHEDMINSIZE_SPIN,    NULL),
    ControlInfo ("FileCacheSizeKB",     ControlEditAndSpin, IDC_FILECACHESIZE,       IDC_FILECACHESIZE_SPIN,       NULL),
    ControlInfo ("BlockSizeLog2",       ControlEditAndSpin, IDC_BLOCKSIZELOG2,       IDC_BLOCKSIZELOG2_SPIN,       NULL),
    ControlInfo ("CacheChunks",         ControlEditAndSpin, IDC_CACHECHUNKS,         IDC_CACHECHUNKS_SPIN,         NULL),
    ControlInfo ("SeekGranularityLog2", ControlEditAndSpin, IDC_SEEKGRANULARITYLOG2, IDC_SEEKGRANULARITYLOG2_SPIN, NULL), 
    ControlInfo ("Prefetching",         ControlCheckBox,    IDC_PREFETCHING,         NULL,                         NULL),
    ControlInfo ("_CachingExplain",     ControlText,        IDC_CACHINGEXPLAIN,      NULL,                         NULL),

    ControlInfo ("", ControlInvalid, NULL, NULL, NULL)
};


ControlInfo HiveControls[] =
{
    ControlInfo ("HiveEnabled",     ControlCheckBox,    IDC_HIVEENABLED,      NULL,                   NULL),
    ControlInfo ("HivePath",        ControlText,        IDC_HIVEPATH,         NULL,                   NULL),
    ControlInfo ("HiveCompression", ControlCheckBox,    IDC_HIVE_COMPRESSION, NULL,                   HiveCompressionButtonProc),
    ControlInfo ("HiveMinSizeKB",   ControlEditAndSpin, IDC_HIVEMINSIZEKB,    IDC_HIVEMINSIZEKB_SPIN, NULL),
    ControlInfo ("_HiveExplain",    ControlText,        IDC_HIVEINFOTEXT,     NULL,                   NULL),
    ControlInfo ("",                ControlButton,      IDC_HIVEBROWSE,       NULL,                   HiveBrowseButtonProc),
    ControlInfo ("",                ControlButton,      IDC_HIVECLEAR,        NULL,                   HiveClearButtonProc),
    ControlInfo ("",                ControlText,        IDC_HIVESTATS,        NULL,                   HiveStatsTextProc),

    ControlInfo ("", ControlInvalid, NULL, NULL, NULL)
};


ControlInfo UpdatesControls[] =
{
    ControlInfo ("_UpdatesExplain",      ControlText,        IDC_UPDATESEXPLAIN,       NULL,                          NULL),
    ControlInfo ("UpdatesEnabled",       ControlCheckBox,    IDC_UPDATESENABLED,       NULL,                          NULL),
    ControlInfo ("UpdatesCheckInterval", ControlEditAndSpin, IDC_UPDATESCHECKINTERVAL, IDC_UPDATESCHECKINTERVAL_SPIN, NULL),
    ControlInfo ("",                     ControlButton,      IDC_UPDATENOW,            NULL,                          UpdateNowButtonProc),
    ControlInfo ("",                     ControlText,        IDC_UPDATESDAYSLEFT,      NULL,                          UpdateDaysLeftTextProc),

    ControlInfo ("", ControlInvalid, NULL, NULL, NULL)
};


SectionInfo Sections[] = 
{
    SectionInfo ("Display",           IDD_PREFS_DISPLAY,   GenericSectionProc, NULL, DisplayControls),
    SectionInfo ("Searching",         IDD_PREFS_SEARCHING, GenericSectionProc, NULL, SearchingControls),
    SectionInfo ("Parsing",           IDD_PREFS_PARSING,   GenericSectionProc, NULL, ParsingControls),
    SectionInfo ("Tailing",           IDD_PREFS_TAILING,   GenericSectionProc, NULL, TailingControls),
    SectionInfo ("Windows",           IDD_PREFS_WINDOWS,   GenericSectionProc, NULL, WindowsControls),
    SectionInfo ("Caching",           IDD_PREFS_CACHING,   GenericSectionProc, NULL, CachingControls),
    SectionInfo ("Index Hive",        IDD_PREFS_HIVE,      GenericSectionProc, NULL, HiveControls),
    SectionInfo ("Updates",           IDD_PREFS_UPDATES,   GenericSectionProc, NULL, UpdatesControls),

    SectionInfo ("", NULL, NULL, NULL, NULL)
};


class PrefsContext
{
public:
    string InitialPage;
    ListThreadContext *LC;
    HWND MainHwnd;
    vector<SectionInfo> Sections;
    VarListExt Settings;
    VarListExt BackupSettings;
    int CurrentSection;
};


class SectionsContext
{
public:
    PrefsContext *PC;
    int Index; // PC->Sections[Index]
};


void HiveBrowseButtonProc (PrefsContext *Context, int Sec, int Ctl, ControlEvent Event)
{
    guard
    {
        string NewDir;
        int s, c;

        prefslog.enter ("HiveBrowseButtonProc");
        switch (Event)
        {
            case EventClicked:
                prefslog.enter ("EventClicked");
                if (ChooseDirectory (Context->MainHwnd, 
                                    NewDir, 
                                    (char *)Context->Settings.GetValString ("HivePath").c_str(), 
                                    "Choose Hive Location"))
                {
                    PickerSetVariable (Context, "HivePath", NewDir);
                }

                if (PickerFindID (Context, IDC_HIVESTATS, s, c))
                    PickerSendControlEvent (Context, s, c, EventClicked);

                prefslog.leave ();
                break;
        }

        prefslog.leave ();
        return;
    } unguard;
}


void HiveCompressionButtonProc (PrefsContext *Context, int Sec, int Ctl, ControlEvent Event)
{
    guard
    {
        prefslog.enter ("HiveCompressionButtonProc");

        switch (Event)
        {
            case EventInit:
                if (!IsHiveCompressionSupported())
                    EnableWindow (GetDlgItem (Context->Sections[Sec].Hwnd, IDC_HIVE_COMPRESSION), FALSE);

                break;
        }

        prefslog.leave ();
        return;
    } unguard;
}


vector<WIN32_FIND_DATA> GetFileNames (string FileSpec = "*.*", DWORD Attributes = 0xffffffff)
{
    guard
    {
        prefslog.enter ("GetFileNames");

        WIN32_FIND_DATA FindData;
        HANDLE FindHandle;
        BOOL Result;
        vector<WIN32_FIND_DATA> Return;

        FindHandle = FindFirstFile (FileSpec.c_str(), &FindData);

        if (FindHandle == INVALID_HANDLE_VALUE)
            Result = FALSE;
        else
            Result = TRUE;

        while (Result == TRUE)
        {
            if (FindData.cFileName[0] != '.'  &&  FindData.dwFileAttributes & Attributes)
                Return.push_back (FindData);

            Result = FindNextFile (FindHandle, &FindData);

        };

        prefslog << "returning vector of " << Return.size() << " files" << endl;
        prefslog << "Result == " << Result << endl;
        prefslog.leave ();
        return (Return);
    } unguard;
}


void GetHiveData (PrefsContext *Context, uint64 &Bytes, vector<WIN32_FIND_DATA> &Files, string &Directory)
{
    guard
    {
        int i;

        prefslog.enter ("GetHiveData");
        Directory = Context->Settings.GetValString ("HivePath");
        Files = GetFileNames (Directory + string ("\\*." INDEXEXT));
        Bytes = 0;

        for (i = 0; i < Files.size(); i++)
            Bytes += (uint64)Files[i].nFileSizeLow + ((uint64)Files[i].nFileSizeHigh << 32);

        prefslog << "File count:  " << Files.size() << endl;
        prefslog << "Total bytes: " << Bytes << endl;
        prefslog.leave ();
        return;
    } unguard;
}


typedef struct HSTContext
{
    PrefsContext *PC;
    int Sec;
    int Ctl;
} HSTContext;


DWORD WINAPI HiveStatsTextThread (LPVOID Context)
{
    guard
    {
        HSTContext *HST = (HSTContext *) Context;
        vector<WIN32_FIND_DATA> Files;
        uint64 Bytes;
        string DirName;
        char Text[1024];
        char CommaBuf[256];

        PickerSendControlMessage (HST->PC, HST->Sec, HST->Ctl, WM_SETTEXT, 0, (LPARAM) "...");
        GetHiveData (HST->PC, Bytes, Files, DirName);

        sprintf (Text, "There %s %u index file%s totalling %s %s in the hive.", 
            Files.size() == 1 ? "is" : "are",
            Files.size(), 
            Files.size() == 1 ? "" : "s", 
            AddCommas(CommaBuf, Bytes / GetScaleDiv(Bytes)),
            GetScaleName(Bytes));

        PickerSendControlMessage (HST->PC, HST->Sec, HST->Ctl, WM_SETTEXT, 0, (LPARAM) (LPCTSTR) Text);
        delete HST;
        return (0);
    } unguard;
}


void HiveStatsTextProc (PrefsContext *Context, int Sec, int Ctl, ControlEvent Event)
{
    guard
    {
        HSTContext *HST;
        DWORD ThreadID;

        prefslog.enter ("HiveStatsTextProc");
        switch (Event)
        {
            case EventInit:
                prefslog.enter ("EventInit");
                PickerSendControlEvent (Context, Sec, Ctl, EventClicked);
                prefslog.leave ();
                break;

            case EventClicked:
                prefslog.enter ("EventClicked");
                HST = new HSTContext;
                HST->PC = Context;
                HST->Sec = Sec;
                HST->Ctl = Ctl;
                CreateThread (NULL, 4096, HiveStatsTextThread, (LPVOID)HST, 0, &ThreadID);
                prefslog.leave ();
                break;
        }

        prefslog.leave ();
        return;
    } unguard;
}


void HiveClearButtonProc (PrefsContext *Context, int Sec, int Ctl, ControlEvent Event)
{
    guard
    {
        string Path;
        int Result;
        vector<WIN32_FIND_DATA> Files;
        uint64 Bytes;
        char CommaBuf[256];
        int i;

        prefslog.enter ("HiveClearButtonProc");
        Path = Context->Settings.GetValString ("HivePath");
        Files = GetFileNames ((Path + string ("\\*.") + string (INDEXEXT)).c_str());

        // Count how many bytes they'll save by deleting the files
        Bytes = 0;

        for (i = 0; i < Files.size(); i++)
            Bytes += (uint64)Files[i].nFileSizeLow + ((uint64)Files[i].nFileSizeHigh << 32);

        // Decide whether to print bytes, kilobytes, megabytes

        switch (Event)
        {
            case EventClicked:
                prefslog.enter ("EventClicked");
                Result = MessageBox (Context->Sections[Sec].Hwnd,
                    (string("Clicking 'Yes' will delete all files with the ")
                    + string(INDEXEXT)
                    + string(" extension in the following directory:\n\t")
                    + Path
                    + string("\nThis operation will save " )
                    + string(AddCommas(CommaBuf, Bytes / GetScaleDiv(Bytes)))
                    + string(" ") 
                    + string(GetScaleName(Bytes)) 
                    + string (" ")
                    + string ("of disk space.")).c_str(),
                    "Confirmation",
                    MB_YESNO);

                if (Result == IDYES)
                {
                    WIN32_FIND_DATA FindData;
                    HANDLE FindHandle;
                    int count = 0;
                    BOOL Result;

                    FindHandle = FindFirstFile ((Path + string ("\\*.") + string(INDEXEXT)).c_str(), &FindData);
                    
                    if (FindHandle == INVALID_HANDLE_VALUE)
                        Result = FALSE;
                    else
                        Result = TRUE;

                    while (Result == TRUE)
                    {
                        count++;
                        DeleteFile ((Path + string("\\") + string(FindData.cFileName)).c_str());
                        Result = FindNextFile (FindHandle, &FindData);
                    }

                    MessageBox (Context->Sections[Sec].Hwnd,
                                (string("Deleted ") 
                                + string(AddCommas(CommaBuf,count)) 
                                + string(" index file") 
                                + string(count == 1 ? "" : "s")).c_str(),
                                "Finished",
                                MB_OK);

                    FindClose (FindHandle);
                }

                // Update the info text
                int s, c;
                if (PickerFindID (Context, IDC_HIVESTATS, s, c))
                    PickerSendControlEvent (Context, s, c, EventClicked);

                prefslog.leave ();
                break;
        }

        prefslog.leave ();
        return;
    } unguard;
}

void EditBrowseButtonProc (PrefsContext *Context, int Sec, int Ctl, ControlEvent Event)
{
    guard
    {
        string NewFile;

        prefslog.enter ("EditBrowseButtonProc");
        switch (Event)
        {
            case EventClicked:
                prefslog.enter ("EventClicked");
                if (ChooseFile (Context->MainHwnd, 
                                NewFile, 
                                "Choose External Editor",
                                "Executables\0*.exe\0"))
                {
                    PickerSetVariable (Context, "EditProgram", NewFile);
                }

                prefslog.leave ();
                break;
        }

        prefslog.leave ();
        return;
    } unguard;
}


void AlphaSpinButtonProc (PrefsContext *Context, int Sec, int Ctl, ControlEvent Event)
{
    guard
    {
        prefslog.enter ("AlphaSpinButtonProc");
        switch (Event)
        {
            case EventInit:
            case EventDestroy:
            case EventClicked:
                prefslog.enter (tostring (Event));

                if (!IsWindowAlphaAllowed())
                {
                    EnableWindow (GetDlgItem (Context->Sections[Sec].Hwnd, IDC_TRANSPARENT), FALSE);
                    EnableWindow (GetDlgItem (Context->Sections[Sec].Hwnd, IDC_ALPHALEVEL), FALSE);
                    EnableWindow (GetDlgItem (Context->Sections[Sec].Hwnd, IDC_ALPHALEVEL_SPIN), FALSE);
                }
                
                SetWindowAlphaValues (GetWindow (Context->MainHwnd, GW_OWNER), 
                                      bool(Context->Settings.GetValUint32 ("Transparent")),
                                      Context->Settings.GetValUint32 ("AlphaLevel"));

                prefslog.leave ();
                break;
        }

        prefslog.leave ();
        return;
    } unguard;
}


void PickColorButtonProc (PrefsContext *Context, int Sec, int Ctl, ControlEvent Event)
{
    guard
    {
        prefslog.enter ("PickColorButtonProc");
        switch (Event)
        {
            case EventDestroy:
                prefslog.enter ("EventDestroy");
                Context->LC->SettingsV2.SetVar ("Highlight.Foreground", Context->BackupSettings.GetVar ("Highlight.Foreground"));
                Context->LC->SettingsV2.SetVar ("Highlight.Background", Context->BackupSettings.GetVar ("Highlight.Background"));
                Context->LC->SettingsV2.SetVar ("InfoColor.Foreground", Context->BackupSettings.GetVar ("InfoColor.Foreground"));
                Context->LC->SettingsV2.SetVar ("InfoColor.Background", Context->BackupSettings.GetVar ("InfoColor.Background"));
                Context->LC->SettingsV2.SetVar ("TextColor.Foreground", Context->BackupSettings.GetVar ("TextColor.Foreground"));
                Context->LC->SettingsV2.SetVar ("TextColor.Background", Context->BackupSettings.GetVar ("TextColor.Background"));
                AddSimpleCommand (Context->LC, LCMD_UPDATE_TEXT);
                AddSimpleCommand (Context->LC, LCMD_UPDATE_INFO);
                prefslog.leave ();
                break;

            case EventClicked:
                prefslog.enter ("EventClicked");
                CHOOSECOLOR CC;
                LONG Index;
                int scb, ccb;
                ValBinary CustomColorsVB;
                uint8 *CustomColors;
                int i;

                ZeroMemory (&CC, sizeof (CC));

                // Find the sec/ctl #s for the combo box
                if (!PickerFindID (Context, IDC_COLORLIST, scb, ccb))
                {
                    prefslog << "PickerFindID(...) == false" << endl;
                    prefslog.leave ();
                    break;
                }
                
                Index = PickerSendControlMessage (Context, scb, ccb, CB_GETCURSEL, 0, 0);

                if (Index == CB_ERR)
                {
                    prefslog << "Index == CB_ERR" << endl;
                    prefslog.leave ();
                    break;
                }

                CC.lStructSize = sizeof (CC);
                CC.hwndOwner = Context->Sections[Sec].Hwnd;
                CC.hInstance = NULL;

                prefslog << "User chose index " << Index << endl;

                switch (Index)
                {
                    default:
                    case 0: // Highlight.Background
                        CC.rgbResult = Context->Settings.GetValUint32 ("Highlight.Background"); 
                        break;

                    case 1: // Highlight.Foreground
                        CC.rgbResult = Context->Settings.GetValUint32 ("Highlight.Foreground"); 
                        break;

                    case 2: // InfoColor.Background
                        CC.rgbResult = Context->Settings.GetValUint32 ("InfoColor.Background"); 
                        break;

                    case 3: // InfoColor.Foreground
                        CC.rgbResult = Context->Settings.GetValUint32 ("InfoColor.Foreground"); 
                        break;

                    case 4: // TextColor.Background
                        CC.rgbResult = Context->Settings.GetValUint32 ("TextColor.Background"); 
                        break;

                    case 5: // TextColor.Foreground
                        CC.rgbResult = Context->Settings.GetValUint32 ("TextColor.Foreground"); 
                        break;
                }

                // Get the custom colors
                CustomColorsVB = Context->Settings.GetValBinary ("CustomColors");
                CustomColors = new uint8[64];
                ZeroMemory (CustomColors, 64);

                for (i = 0; i < min (64, CustomColorsVB.size()); i++)
                    CustomColors[i] = CustomColorsVB[i];

                CC.lpCustColors = (COLORREF *)CustomColors;
                CC.Flags = CC_ANYCOLOR | CC_FULLOPEN | CC_RGBINIT;
                CC.lCustData = NULL;
                CC.lpfnHook = NULL;
                CC.lpTemplateName = NULL;

                ChooseColor (&CC);

                Context->Settings.SetValBinary ("CustomColors", CustomColors, 64);
                delete CustomColors;
                
                switch (Index)
                {
                    default:
                    case 0:
                        Context->Settings.SetValUint32 ("Highlight.Background", CC.rgbResult);
                        Context->LC->SettingsV2.SetVar ("Highlight.Background", Context->Settings.GetVar ("Highlight.Background"));
                        break;

                    case 1:
                        Context->Settings.SetValUint32 ("Highlight.Foreground", CC.rgbResult);
                        Context->LC->SettingsV2.SetVar ("Highlight.Foreground", Context->Settings.GetVar ("Highlight.Foreground"));
                        break;

                    case 2:
                        Context->Settings.SetValUint32 ("InfoColor.Background", CC.rgbResult);
                        Context->LC->SettingsV2.SetVar ("InfoColor.Background", Context->Settings.GetVar ("InfoColor.Background"));
                        break;

                    case 3:
                        Context->Settings.SetValUint32 ("InfoColor.Foreground", CC.rgbResult);
                        Context->LC->SettingsV2.SetVar ("InfoColor.Foreground", Context->Settings.GetVar ("InfoColor.Foreground"));
                        break;

                    case 4:
                        Context->Settings.SetValUint32 ("TextColor.Background", CC.rgbResult);
                        Context->LC->SettingsV2.SetVar ("TextColor.Background", Context->Settings.GetVar ("TextColor.Background"));
                        break;

                    case 5:
                        Context->Settings.SetValUint32 ("TextColor.Foreground", CC.rgbResult);
                        Context->LC->SettingsV2.SetVar ("TextColor.Foreground", Context->Settings.GetVar ("TextColor.Foreground"));
                        break;
                }

                AddSimpleCommand (Context->LC, LCMD_UPDATE_INFO);
                AddSimpleCommand (Context->LC, LCMD_UPDATE_TEXT);
                prefslog.leave ();
                break;
        }

        prefslog.leave ();
        return;
    } unguard;
}

void ShellIntButtonProc (PrefsContext *Context, int Sec, int Ctl, ControlEvent Event)
{
    guard
    {
        bool Enabled;

        prefslog.enter ("ShellIntButtonProc");
        switch (Event)
        {
            case EventInit:
                prefslog.enter ("EventInit");
                Enabled = GetShellIntegration ();
                PickerSendControlMessage (Context, Sec, Ctl, BM_SETCHECK, Enabled ? BST_CHECKED : BST_UNCHECKED, 0);
                prefslog.leave ();
                break;

            case EventApply:
                prefslog.enter ("EventApply");
                Enabled = (BST_CHECKED == PickerSendControlMessage (Context, Sec, Ctl, BM_GETCHECK, 0, 0));

                if (Enabled != GetShellIntegration() &&
                    !SetShellIntegration (Enabled))
                {
                    MessageBox (Context->MainHwnd, 
                        (string("The shell integration could not be ") + string(Enabled ? "enabled" : "disabled") + string (". Please make sure you have the appropriate user permissions.")).c_str(),
                        "Error",
                        MB_OK | MB_ICONWARNING);
                }

                prefslog.leave ();
                break;
        }

        prefslog.leave ();
        return;
    } unguard;
}


void UpdateDaysLeftTextProc (PrefsContext *Context, int Sec, int Ctl, ControlEvent Event)
{
    guard
    {
        char Text[1024];
        uint64_t LastChecked;
        uint64_t Now;
        int64_t DaysLeft;

        prefslog.enter ("UpdateDaysLeftTextProc");
        switch (Event)
        {
            case EventRefresh:
            case EventApply:
            case EventInit:
                prefslog.enter (tostring (Event));
                Now = GetTime64 ();

                LastChecked = Context->Settings.GetValUint64 ("UpdatesLastChecked");
                DaysLeft = Context->Settings.GetValUint32 ("UpdatesCheckInterval") - TimeDiffDays (Now, LastChecked);

                DaysLeft = std::max(DaysLeft, 0LL);

                if (bool(Context->Settings.GetValUint32("UpdatesEnabled")))
                    sprintf (Text, "Updates will be checked for in approximately %I64d days.", DaysLeft);
                else
                    strcpy (Text, "");

                SetWindowText (GetDlgItem (Context->Sections[Sec].Hwnd, IDC_UPDATESDAYSLEFT), Text);
                prefslog.leave ();
                break;
        }

        prefslog.leave ();
        return;
    } unguard;
}


void UpdateNowButtonProc (PrefsContext *Context, int Sec, int Ctl, ControlEvent Event)
{
    guard
    {
        bool FoundUpdate;
        UpdateInfo Info;

        prefslog.enter ("UpdateNowButtonProc");
        switch (Event)
        {
            case EventClicked:
                prefslog.enter ("EventClicked");
                CheckForUpdate (Context->LC, true, false, &FoundUpdate, &Info);

                if (!FoundUpdate)
                {
                    prefslog << "No updates found" << endl;
                    MessageBox (Context->MainHwnd, "No updates are available.", "Updates", MB_OK);
                }
                else
                {
                    prefslog << "Update found: " << Info.NewVersion << endl;
                    prefslog << "Message: " << Info.Message << endl;

                    // Make sure that LC->SettingsV2 is up-to-date with the checkbox value
                    Context->LC->SettingsV2.SetValUint32 ("UpdatesEnabled", 
                        uint32(BST_CHECKED == SendDlgItemMessage (Context->Sections[Sec].Hwnd, IDC_UPDATESENABLED, BM_GETCHECK, 0, 0))); 

                    // Show them the dialog box!
                    BugUserAboutUpdate (Context->MainHwnd, Context->LC, Info.NewVersion, Info.Message);

                    // Make sure that the checkbox value reflects the value in LC->SettingsV
                    PickerSetVariable (Context, "UpdatesEnabled", 
                        Context->LC->SettingsV2.GetValString ("UpdatesEnabled"));
                }

                Context->Settings.SetValUint64 ("UpdatesLastChecked", GetTime64());
                PickerBroadcastEvent (Context, EventRefresh);

                prefslog.leave ();
                break;
        }

        prefslog.leave ();
        return;
    } unguard;
}


void FontNameComboBoxProc (PrefsContext *Context, int Sec, int Ctl, ControlEvent Event)
{
    guard
    {
        prefslog.enter ("FontNameComboBoxProc");

        switch (Event)
        {
            case EventInit:
                prefslog.enter ("EventInit");
                {
                    HDC hdc;
                    string CurrentFontName = Context->Settings.GetValString ("FontName");

                    hdc = GetDC (Context->MainHwnd);
                    stringvec FontNames = GetFontList (hdc);
                    sort (FontNames.begin(), FontNames.end());

                    ReleaseDC (Context->MainHwnd, hdc);

                    PickerSendControlMessage (Context, Sec, Ctl, CB_RESETCONTENT, 0, 0);

                    for (stringvec::iterator svit = FontNames.begin(); svit != FontNames.end(); ++svit)
                    {
                        LRESULT Index;

                        Index = PickerSendControlMessage (Context, Sec, Ctl, CB_ADDSTRING, 0, (LPARAM) (LPCTSTR) svit->c_str());                

                        if (*svit == CurrentFontName)
                            PickerSendControlMessage (Context, Sec, Ctl, CB_SETCURSEL, Index, 0);
                    }
                }            
                prefslog.leave ();
                break;
        }

        prefslog.leave ();
    } unguard;
}

// Functions for working with a delimited attribute list, (such as FontName's 'validlist' attribute)
int DAGetCount (string &List, char Delimiter)
{
    guard
    {
        int count;
        int i;

        if (List.length() == 0)
            return (0);

        for (i = 0, count = 1; i < List.length(); i++)
        {
            if (List[i] == Delimiter)
                count++;
        }

        return (count);
    } unguard;
}


string DAGetString (string &List, int Which, char Delimiter)
{
    guard
    {
        int ord;
        int start;
        int end;
        int i;
        string Result = string("");

        if (List.length() == 0)
            return ("");

        ord = 0;
        start = 0;
        end = 0;

        for (i = 0; i <= List.length(); i++)
        {
            if (i == List.length()  ||  List[i] == Delimiter)
            {
                end = i;

                if (ord == Which)
                {
                    Result.assign (List.begin() + start, List.begin() + end);
                    return (Result);
                }

                start = i + 1;
                ord++;
            }
        }

        return ("");
    } unguard;
}


int DAIsInList (string &List, const string &FindMe, char Delimiter)
{
    guard
    {
        int count;
        int i;

        count = DAGetCount (List, Delimiter);

        for (i = 0; i < count ;i++)
        {
            if (DAGetString (List, i, Delimiter) == FindMe)
                return (i);
        }

        return (-1);
    } unguard;
}


bool DAInsert (string &List, const string &AddMe, char Delimiter)
{
    guard
    {
        char DelStr[2];

        DelStr[0] = Delimiter;
        DelStr[1] = '\0';

        if (!DAIsInList (List, AddMe, Delimiter))
        {
            List += string(DelStr) + AddMe;
            return (true);
        }

        return (false);
    } unguard;
}


void PickerInit (PrefsContext *Context)
{
    guard
    {
        int i;
        LRESULT Index;
        WINDOWPLACEMENT Placement;
        int sec;
        int ctl;

        prefslog.enter ("PickerInit");

        Placement.length = sizeof (Placement);
        GetWindowPlacement (GetDlgItem(Context->MainHwnd, IDC_SUBALIGN), &Placement);
        SendDlgItemMessage (Context->MainHwnd, IDC_SECTION_PICKER, LB_RESETCONTENT, 0, 0);

        prefslog << "Section count: " << Context->Sections.size() << endl;
        for (i = 0; i < Context->Sections.size(); i++)
        {
            Index = SendDlgItemMessage (Context->MainHwnd, IDC_SECTION_PICKER, LB_ADDSTRING, 0, (LPARAM) (LPCTSTR) Sections[i].Name.c_str());
            SendDlgItemMessage (Context->MainHwnd, IDC_SECTION_PICKER, LB_SETITEMDATA, Index, (LPARAM) i);
            SetWindowPlacement (Context->Sections[i].Hwnd, &Placement);
            ShowWindow (Context->Sections[i].Hwnd, SW_HIDE);
        }
        prefslog << "Section windows created" << endl;

        SendDlgItemMessage (Context->MainHwnd, IDC_SECTION_PICKER, LB_SETCURSEL, 0, 0);
        PickerSetSection (Context, 0);

        // Find all the combo box controls and fill them up
        for (sec = 0; sec < Context->Sections.size(); sec++)
        {
            for (ctl = 0; ctl < Context->Sections[sec].Controls.size(); ctl++)
            {
                ControlInfo *CI;

                prefslog.enter () << "Initializing sec=" << sec << " ctl=" << ctl << endl;
                CI = &Context->Sections[sec].Controls[ctl];

                if (CI->Type == ControlComboBox)
                {
                    string List;
                    int count;
                    int j;

                    prefslog << "(it's a ControlComboBox)" << endl;
                    PickerSendControlMessage (Context, sec, ctl, CB_RESETCONTENT, 0, 0);

                    List = GlobalDefaults.GetVar(CI->VarName).Attributes.GetValString ("validlist");

                    // Add all items to the combo box, which are delimited by the ^ character in 'validlist'
                    count = DAGetCount (List);

                    for (j = 0; j < count; j++)
                    {
                        string Text;
                        LRESULT Index;

                        Text = DAGetString (List, j);
                        prefslog << "Adding string: " << Text << endl;
                        Index = PickerSendControlMessage (Context, sec, ctl, CB_ADDSTRING, 0, (LPARAM) (LPCTSTR) Text.c_str());
                        PickerSendControlMessage (Context, sec, ctl, CB_SETITEMDATA, Index, j);
                    }

                    PickerSendControlMessage (Context, sec, ctl, CB_SETCURSEL, 0, 0);
                }

                prefslog.leave ();
            }
        }

        // Set all the values for the variables
        prefslog.enter ("Initializing control values"); //
        vector<VarNamedExt> Vars;
        vector<VarNamedExt> DefVars;

        Vars = Context->Settings.GetVector ();
        DefVars = GlobalDefaults.GetVector ();

        prefslog << "There are " << Vars.size() << " vars, with " << DefVars.size() << " defaults" << endl;

        // Initially every control receives the default value
        prefslog << "Initializing to defaults" << endl;
        for (i = 0; i < DefVars.size(); i++)
            PickerSetVariable (Context, DefVars[i].Name, DefVars[i].Value);

        // And then we set all the values that have changed from the defaults
        prefslog << "Setting changed vars" << endl;
        for (i = 0; i < Vars.size(); i++)
            PickerSetVariable (Context, Vars[i].Name, Vars[i].Value);

        // And then we tell all the controls to perform any other initialization steps they want
        PickerBroadcastEvent (Context, EventInit);

        prefslog.leave (); //

        // And then we gray out the Apply button
        EnableWindow (GetDlgItem (Context->MainHwnd, ID_APPLY), FALSE);

        prefslog.leave ();
        return;
    } unguard;
}


void PickerSetSection (PrefsContext *Context, int Index)
{
    guard
    {
        int i;

        prefslog.enter ("PickerSetSection (int)");
        prefslog << "Setting section index #" << Index << endl;

        prefslog << "Hiding all pages" << endl;
        for (i = 0; i < Context->Sections.size(); i++)
            ShowWindow (Context->Sections[i].Hwnd, SW_HIDE);

        prefslog << "Showing correct page" << endl;
        ShowWindow (Context->Sections[Index].Hwnd, SW_SHOW);
        SetWindowText (GetDlgItem (Context->MainHwnd, IDC_SECTION_NAME), Context->Sections[Index].Name.c_str());
        SetWindowPos (GetDlgItem (Context->MainHwnd, IDC_SECTION_NAME), HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        Context->CurrentSection = Index;
        SendDlgItemMessage (Context->MainHwnd, IDC_SECTION_PICKER, LB_SETCURSEL, Index, 0);

        prefslog.leave ();
        return;
    } unguard;
}


void PickerSetSection (PrefsContext *Context, const string &Name)
{
    guard
    {
        int i;

        prefslog.enter ("PickerSetSection (string)");

        for (i = 0; i < Context->Sections.size(); i++)
            if (Name == Context->Sections[i].Name)
                PickerSetSection (Context, i);

        prefslog.leave ();
        return;
    } unguard;
}


// Returns the section and control indexes of a given variable
// returns true if the variable was found, with *Result filed in appropriately
bool PickerFindVariable (PrefsContext *Context, const string &Name, int &SectionIndexResult, int &ControlIndexResult)
{
    guard
    {
        int sec;
        int ctl;

        prefslog.enter ("PickerFindVariable");
        prefslog << "Looking for: " << Name << endl;

        for (sec = 0; sec < Context->Sections.size(); sec++)
        {
            for (ctl = 0; ctl < Context->Sections[sec].Controls.size(); ctl++)
            {
                if (Context->Sections[sec].Controls[ctl].VarName == Name)
                {
                    prefslog << "Found: sec=" << sec << " ctl=" << ctl << endl;
                    SectionIndexResult = sec;
                    ControlIndexResult = ctl;
                    prefslog.leave ();
                    return (true);
                }
            }
        }

        prefslog << "Could not find!" << endl;
        prefslog.leave ();
        return (false);
    } unguard;
}


// Returns the section and control index for a given win32 control ID (or ID2)
bool PickerFindID (PrefsContext *Context, WORD IDorID2, int &SectionIndexResult, int &ControlIndexResult)
{
    guard
    {
        int sec;
        int ctl;

        prefslog.enter ("PickerFindID");
        prefslog << "Looking for ID (or ID2) == " << IDorID2 << endl;

        for (sec = 0; sec < Context->Sections.size(); sec++)
        {
            for (ctl = 0; ctl < Context->Sections[sec].Controls.size(); ctl++)
            {
                if (Context->Sections[sec].Controls[ctl].ID  == IDorID2  ||
                    Context->Sections[sec].Controls[ctl].ID2 == IDorID2)
                {
                    prefslog << "Found: sec=" << sec << " ctl=" << ctl << endl;
                    SectionIndexResult = sec;
                    ControlIndexResult = ctl;
                    prefslog.leave ();
                    return (true);
                }
            }
        }

        prefslog << "Could not find!" << endl;
        prefslog.leave ();
        return (false);
    } unguard;
}


// Returns the variable name associated with a given win32 control ID (or ID2)
bool PickerFindVarForID (PrefsContext *Context, WORD IDorID2, string &VarNameResult)
{
    guard
    {
        int sec;
        int ctl;

        prefslog.enter ("PickerFindVarForID");
        prefslog << "Looking for ID (or ID2) == " << IDorID2 << endl;

        if (!PickerFindID (Context, IDorID2, sec, ctl))
        {
            prefslog << "Could not find!" << endl;
            prefslog.leave ();
            return (false);
        }

        VarNameResult = Context->Sections[sec].Controls[ctl].VarName;
        prefslog << "Var name = " << VarNameResult << endl;
        prefslog.leave ();
        return (true);
    } unguard;
}


bool PickerSetVariable (PrefsContext *Context, const string &Name, const string &Value)
{
    guard
    {
        int sec;
        int ctl;
        int ord;
        string Temp;
        LRESULT Index;
        VarAnonExt Attr;
        VarAnonExt Val;
        int order;
        char DispText[1024];

        prefslog.enter ("PickerSetVariable");
        prefslog << "Setting " << Name << " to " << Value << endl;

        if (Name == "")
        {
            prefslog << "Aborting: Name=\"\"" << endl;
            prefslog.leave ();
            return (false);
        }

        if (!PickerFindVariable (Context, Name, sec, ctl))
        {
            prefslog << "Aborting: Could not find sec/ctl for " << Name << endl;
            prefslog.leave ();
            return (false);
        }

        if (!Context->Sections[sec].Active)
        {
            prefslog << "Aborting: Section page is not active" << endl;
            prefslog.leave ();
            return (false);
        }

        Context->Settings.SetVal (Name, Value);
        Context->Settings.SetVarType (Name, GlobalDefaults.GetVar (Name).Type);

        Val = Context->Settings.GetVar (Name);
        prefslog << "Value is now: " << Val << endl;
        Attr = GlobalDefaults.GetVar (Name);
        prefslog << "Global default: " << Attr << endl;
        order = Attr.Attributes.GetValUint32 ("dispvalorder");

        // For order, 0=normal, 1=2^x
        if (order == 0)
            strcpy (DispText, Val.GetValString().c_str());
        else
        if (order == 1)
        {
            sint64 NumExp2;

            NumExp2 = (signed) Val.GetValUint64();
            NumExp2 = 1 << NumExp2;
            sprintf (DispText, "%I64d", NumExp2);
        }

        switch (Context->Sections[sec].Controls[ctl].Type)
        {
            case ControlText:
                prefslog.enter ("setting ControlText");
                PickerSendControlMessage (Context, sec, ctl, WM_SETTEXT, 0, (LPARAM) DispText);
                prefslog.leave ();
                break;

            case ControlCheckBox:
                prefslog.enter ("setting ControlCheckBox");
                PickerSendControlMessage (Context, sec, ctl, BM_SETCHECK,
                    Context->Settings.GetValUint32 (Name) ? BST_CHECKED : BST_UNCHECKED, 0);
                prefslog.leave ();
                break;

            case ControlEditAndSpin:
                prefslog.enter ("setting ControlEditAndSpin");
                PickerSendControlMessage (Context, sec, ctl, WM_SETTEXT, 0, (LPARAM) DispText);
                prefslog.leave ();
                break;

            case ControlComboBox:
                LRESULT Count;

                prefslog.enter ("setting ControlComboBox");

                // Make sure that Value is a valid choise
                Temp = GlobalDefaults.GetVar (Name).Attributes.GetValString ("validlist");
                ord = DAIsInList (Temp, Value);

                if (ord == -1)
                {
                    prefslog << "ord == -1" << endl;
                    prefslog.leave ();
                    return (false); // invalid!
                }
                
                // Search through the data in the combo box for one who's associated data (the data
                // win32 keeps track of) is equal to ord
                Count = PickerSendControlMessage (Context, sec, ctl, CB_GETCOUNT, 0, 0);
                Index = PickerSendControlMessage (Context, sec, ctl, CB_FINDSTRING, -1, (LPARAM)(LPCTSTR)DAGetString(Temp,ord).c_str());

                if (Index == CB_ERR)
                {
                    prefslog << "Index == CB_ERR" << endl;
                    prefslog.leave ();
                    return (false);
                }

                PickerSendControlMessage (Context, sec, ctl, CB_SETCURSEL, Index, 0);                        
                prefslog.leave ();
                break;
        }

        prefslog.leave ();
        return (true);
    } unguard;
}


LRESULT PickerSendControlMessage (PrefsContext *Context, int Section, int Control, UINT Msg, WPARAM WParam, LPARAM LParam)
{
    guard
    {
        LRESULT LResult;

        prefslog.enter ("PickerSendControlMessage");
        prefslog << "sec=" << Section
                 << " ctl=" << Control
                 << " msg=" << Msg 
                 << " w=" << WParam 
                 << " l=" << LParam 
                 << endl;

        LResult = SendDlgItemMessage (Context->Sections[Section].Hwnd, 
                                      Context->Sections[Section].Controls[Control].ID,
                                      Msg,
                                      WParam,
                                      LParam);

        prefslog << "LResult=" << LResult << endl;
        prefslog.leave ();
        return (LResult);
    } unguard;
}


void PickerSendControlEvent (PrefsContext *Context, int Section, int Control, ControlEvent Event)
{
    guard
    {
        ControlEventProc Proc;

        prefslog.enter ("PickerSendControlEvent");
        prefslog << "sec=" << Section 
                 << " ctl=" << Control
                 << " event=" << Event
                 << endl;

        Proc = Context->Sections[Section].Controls[Control].Proc;

        if (Proc != NULL)
            Proc (Context, Section, Control, Event);
        
        prefslog.leave ();
        return;
    } unguard;
}


bool PickerSendVariableMessage (PrefsContext *Context, const string &Name, UINT Msg, WPARAM WParam, LPARAM LParam, LRESULT &LResult)
{
    guard
    {
        int sec;
        int ctl;

        prefslog.enter ("PickerSendVariableMessage");
        prefslog << "name=" << Name
                 << " msg=" << Msg
                 << " w=" << WParam
                 << " l=" << LParam
                 << endl;

        if (!PickerFindVariable (Context, Name, sec, ctl))
        {
            prefslog << "Could not find!" << endl;
            prefslog.leave ();
            return (false);
        }

        LResult = PickerSendControlMessage (Context, sec, ctl, Msg, WParam, LParam);
        prefslog << "lresult=" << LResult << endl;
        prefslog.leave ();
        return (true);
    } unguard;
}


void PickerBroadcastEvent (PrefsContext *Context, ControlEvent Event)
{
    guard
    {
        int sec;
        int ctl;

        prefslog.enter ("PickerBroadcastEvent");
        prefslog << "event=" << Event << endl;

        for (sec = 0; sec < Context->Sections.size(); sec++)
        {
            for (ctl = 0; ctl < Context->Sections[sec].Controls.size(); ctl++)
            {
                ControlInfo *CI;

                CI = &Context->Sections[sec].Controls[ctl];
                if (CI->Proc != NULL)
                    CI->Proc (Context, sec, ctl, Event);
            }
        }

        prefslog.leave ();
        return;
    } unguard;
}


INT_PTR CALLBACK GenericSectionProc (HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam)
{
    guard
    {
        SectionsContext *SC = NULL;
        PrefsContext *PC = NULL;
        SectionInfo *SI = NULL;
        ListThreadContext *LC = NULL;
        int sec;
        int ctl;

        SC = (SectionsContext *) GetWindowLongPtr (Hwnd, GWLP_USERDATA);

        if (SC != NULL)
        {
            PC = SC->PC;
            SI = &PC->Sections[SC->Index];
            LC = PC->LC;
        }
        else
        {
            prefslog << "GenericSectionProc: SC == NULL" << endl;
        }

        switch (Msg)
        {
            case WM_INITDIALOG:
                prefslog.enter ("GenericSectionProc WM_INITDIALOG");
                SC = (SectionsContext *) LParam;
                SetWindowLongPtr (Hwnd, GWLP_USERDATA, (LONG_PTR) SC);
                PC = SC->PC;
                SI = &PC->Sections[SC->Index];;
                LC = PC->LC;
                prefslog.leave ();
                return (true);

            case WM_DESTROY:
                prefslog.enter ("GenericSectionProc WM_DESTROY");
                SetWindowLongPtr (Hwnd, GWLP_USERDATA, NULL);
                delete SC;
                prefslog.leave ();
                return (0);

            // ControlButton, ControlCheckBox
            case WM_COMMAND:
                int ControlID;
                int Event;

                prefslog.enter ("GenericSectionProc WM_COMMAND");
                ControlID = LOWORD (WParam);
                Event = HIWORD (WParam);
                prefslog << "ControlID=" << ControlID
                         << " Event=" << Event << endl;

                if (!SI->Active)
                {
                    prefslog << "SI->Active == false" << endl;
                    prefslog << "SI == " << SI << endl;
                    prefslog << "*SI = " << *SI << endl;
                    prefslog.leave ();
                    break;
                }

                if (PickerFindID (PC, ControlID, sec, ctl))
                {
                    ControlInfo *Info;

                    Info = &PC->Sections[sec].Controls[ctl];

                    switch (Info->Type)
                    {
                        case ControlCheckBox:
                            prefslog << "Type = ControlCheckBox" << endl;
                            EnableWindow (GetDlgItem (PC->MainHwnd, ID_APPLY), TRUE);

                            PickerSetVariable (PC, Info->VarName, PC->Settings.GetValUint32 (Info->VarName) ? "0" : "1");
                            if (Info->Proc != NULL)
                                Info->Proc (PC, sec, ctl, EventClicked);

                            break;

                        case ControlButton:
                            prefslog << "Type = ControlButton" << endl;
                            EnableWindow (GetDlgItem (PC->MainHwnd, ID_APPLY), TRUE);

                            if (Info->Proc != NULL)
                                Info->Proc (PC, sec, ctl, EventClicked);

                            break;

                        case ControlComboBox:
                            prefslog << "Type = ControlComboBox" << endl;
                            switch (Event)
                            {
                                case CBN_SELCHANGE:
                                    {
                                        LRESULT Index;
                                        int Length;
                                        char *Text;

                                        prefslog.enter ("CBN_SELCHANGE");
                                        EnableWindow (GetDlgItem (PC->MainHwnd, ID_APPLY), TRUE);

                                        Index = PickerSendControlMessage (PC, sec, ctl, CB_GETCURSEL, 0, 0);
                                        Length = PickerSendControlMessage (PC, sec, ctl, CB_GETLBTEXTLEN, Index, 0);
                                        Text = new char[Length + 1];
                                        PickerSendControlMessage (PC, sec, ctl, CB_GETLBTEXT, Index, (LPARAM) (LPTSTR) Text);
                                        PickerSetVariable (PC, Info->VarName, Text);
                                        delete Text;
                                        prefslog.leave ();
                                    }
                                    break;
                            }
                            break;

                        case ControlEditAndSpin:
                            prefslog << "Type = ControlEditAndSpin" << endl;
                            switch (Event)
                            {
                                case EN_CHANGE:
                                    prefslog.enter ("EN_CHANGE");
                                    EnableWindow (GetDlgItem (PC->MainHwnd, ID_APPLY), TRUE);
                                    prefslog.leave ();
                                    break;

                                case EN_KILLFOCUS:  
                                {   // Validate number what's in the edit control
                                    prefslog.enter ("EN_KILLFOCUS");
                                    sint64 Min;
                                    sint64 Max;
                                    sint64 Gran;
                                    int order;
                                    VarAnonExt Attr;
                                    VarAnon Val;
                                    char EditText[1024];

                                    EnableWindow (GetDlgItem (PC->MainHwnd, ID_APPLY), TRUE);

                                    Attr = GlobalDefaults.GetVar (Info->VarName);
                                    Min = Attr.Attributes.GetValUint64 ("min");
                                    Max = Attr.Attributes.GetValUint64 ("max");
                                    Gran = Attr.Attributes.GetValUint64 ("gran");
                                    order = Attr.Attributes.GetValUint32 ("dispvalorder");

                                    if (Gran == 0)
                                        Gran = 1;

                                    GetDlgItemText (Hwnd, ControlID, EditText, sizeof(EditText));

                                    if (order == 0)
                                        Val.SetValUint64 (_atoi64 (EditText));
                                    else
                                    if (order == 1)
                                        Val.SetValUint64 (IntLog2(_atoi64(EditText)));

                                    Val.SetValUint64 ((unsigned)(((signed)Val.GetValUint64() / Gran) * Gran));

                                    if ((signed)Val.GetValUint64() < Min)
                                        Val.SetValUint64 (Min);

                                    if ((signed)Val.GetValUint64() > Max)
                                        Val.SetValUint64 (Max);
                                    
                                    Val.SetType (Attr.GetType());
                                    PickerSetVariable (PC, Info->VarName, Val.GetVal());
                                    prefslog.leave ();
                                }
                                break;
                                    
                            }

                            break;
                    }            
                }

                prefslog.leave ();
                break;

            // ControlEditAndSpin
            case WM_NOTIFY:
			    LPNMHDR      Header;
			    LPNMUPDOWN   UpDown;
			    int          Code;
			    int          Delta;

                prefslog.enter ("WM_NOTIFY");

			    UpDown    = (LPNMUPDOWN) LParam;
			    Header    = (LPNMHDR)    LParam;
			    Code      = Header->code;
			    ControlID = (int) WParam;

                prefslog << "ControlID=" << ControlID
                         << " Code=" << Code << endl;

                if (!SI->Active)
                {
                    prefslog << "SI->Active == false" << endl;
                    prefslog << "SI == " << SI << endl;
                    prefslog << "*SI = " << *SI << endl;
                    prefslog.leave ();
                    break;
                }

                if (PickerFindID (PC, ControlID, sec, ctl))
                {
                    ControlInfo *Info;
                    VarAnonExt Attr;  
                    VarAnonExt Val;
                    sint64 Min;
                    sint64 Max;
                    sint64 Gran;
                    sint64 OldVal;
                    sint64 NewVal;

                    Info = &PC->Sections[sec].Controls[ctl];
                    Attr = GlobalDefaults.GetVar (Info->VarName);  // load attribute information
                    Val = PC->Settings.GetVar (Info->VarName);   // old value information
                    OldVal = (signed) Val.GetValUint64 ();
                    Min = (signed) Attr.Attributes.GetValUint64 ("min");
                    Max = (signed) Attr.Attributes.GetValUint64 ("max");
                    Gran = (signed) Attr.Attributes.GetValUint64 ("gran");

                    if (Gran == 0)
                        Gran = 1;
                    
          		    switch (Code)
                    {
                        case UDN_DELTAPOS:
                            prefslog.enter ("UDN_DELTAPOS");
                            EnableWindow (GetDlgItem (PC->MainHwnd, ID_APPLY), TRUE);

                            Delta = -UpDown->iDelta;
                            NewVal = ((OldVal / Gran) + Delta) * Gran;

                            if (NewVal < Min)
                                NewVal = Min;

                            if (NewVal > Max)
                                NewVal = Max;

                            Val.SetValUint64 ((signed)NewVal); 
                            PickerSetVariable (PC, Info->VarName, Val.Value);

                            if (Info->Proc != NULL)
                                Info->Proc (PC, sec, ctl, EventClicked);

                            prefslog.leave ();
                            break;
                    }
                }

                prefslog.leave ();
                break;
        }

        return (FALSE);
    } unguard;
}


INT_PTR CALLBACK PreferencesDialogV2 (HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam)
{
    guard
    {
        int i;
        PrefsContext *Context;

        Context = (PrefsContext *) GetWindowLongPtrA (Hwnd, GWLP_USERDATA);

        switch (Msg)
        {
            case WM_INITDIALOG:
                {
                    prefslog.enter ("PreferencesDialogV2 WM_INITDIALOG");

                    // Init our context
                    //Context = NULL;
                    //Context = new PrefsContext;
                    Context = (PrefsContext *) LParam;
                    Context->MainHwnd = Hwnd;
                    //Context->LC = (ListThreadContext *) LParam;
                    Context->Settings = Context->LC->SettingsV2; // make a copy of the settings so we don't work with them directly!
                    Context->BackupSettings = Context->LC->SettingsV2; // make another copy since the color buttons want to show their changes and must change LC->SettingsV2

                    for (i = 0; Sections[i].Proc != NULL; i++)
                    {
                        SectionsContext *SC;

                        Context->Sections.push_back (Sections[i]);

                        SC = new SectionsContext; // responsibility of child dialog to delete this when it receives WM_DESTROY
                        SC->PC = Context;
                        SC->Index = i;

                        Context->Sections[i].Active = true;

                        Context->Sections[i].Hwnd = CreateDialogParam (GetModuleHandle(NULL), 
                                                                       MAKEINTRESOURCE(Context->Sections[i].ID), 
                                                                       Hwnd, 
                                                                       Context->Sections[i].Proc, 
                                                                       (LPARAM)SC);

                        //Context->Sections[i].Active = true;
                    }

                    SetWindowLongPtr (Hwnd, GWLP_USERDATA, (LONG_PTR)Context);

                    // Other stuff
                    CenterWindow (Hwnd);
                    PickerInit (Context);

                    if (Context->Settings.GetValUint32 ("IPaid") == 1)
                        ShowWindow (GetDlgItem (Hwnd, IDC_GOTODONATE), SW_HIDE);

                    PickerSetSection (Context, Context->InitialPage);
                    prefslog << "Sections: [ " << Context->Sections << " ]" << endl;
                    prefslog.leave ();
                }

                return (TRUE);

            case WM_DESTROY:
                prefslog.enter ("PreferencesDialogV2 WM_DESTROY");
                SetWindowLongPtr (Hwnd, GWLP_USERDATA, NULL);
                prefslog.leave ();
                return (0);

            // If they press the X, simulate pressing the Cancel button
            case WM_SYSCOMMAND:
                prefslog.enter ("PreferencesDialogV2 WM_SYSCOMMAND");
                switch (WParam)
                {
                    case SC_CLOSE:
                        prefslog.enter ("SC_CLOSE");
                        SendDlgItemMessage (Hwnd, ID_CANCEL, BM_CLICK, 0, 0);
                        prefslog.leave ();
                        break;
                }

                prefslog.leave ();
                break;

            case WM_COMMAND:
                int ID;
                int Event;

                prefslog.enter ("PreferencesDialogV2 WM_COMMAND");
                ID = LOWORD (WParam);
                Event = HIWORD (WParam);

                switch (ID)
                {
                    case IDC_SECTION_PICKER:
                        prefslog.enter ("IDC_SECTION_PICKER");
                        switch (Event)
                        {
                            case LBN_SELCHANGE:
                                LRESULT SelIndex;

                                prefslog.enter ("LBN_SELCHANGE");
                                SelIndex = SendDlgItemMessage (Hwnd, IDC_SECTION_PICKER, LB_GETCURSEL, 0, 0);
                                PickerSetSection (Context, SendDlgItemMessage (Hwnd, IDC_SECTION_PICKER, LB_GETITEMDATA, SelIndex, 0));
                                prefslog.leave ();
                                break;
                        }
                        prefslog.leave ();
                        break;

                    case ID_DEFAULTS:
                        {
                            prefslog.enter ("ID_DEFAULTS");
                            vector<VarNamedExt> Vars;

                            Vars = GlobalDefaults.GetVector ();

                            for (i = 0; i < Vars.size(); i++)
                            {   // If this variable is on the currently selected section(page), then reset it
                                bool Found = false;
                                SectionInfo *Sec;

                                Sec = &Context->Sections[Context->CurrentSection];

                                for (int j = 0; j < Sec->Controls.size(); j++)
                                {
                                    if (Vars[i].Name == Sec->Controls[j].VarName)
                                        Found = true;
                                }

                                if (Found)
                                    PickerSetVariable (Context, Vars[i].Name, Vars[i].Value);
                            }

                            EnableWindow (GetDlgItem (Hwnd, ID_APPLY), TRUE);
                            prefslog.leave ();
                        }

                        break;

                    case ID_OK:
                        prefslog.enter ("ID_OK");
                        // Clicking OK is the same as pressing APPLY and then CANCEL
                        SendDlgItemMessage (Hwnd, ID_APPLY, BM_CLICK, 0, 0);
                        SendDlgItemMessage (Hwnd, ID_CANCEL, BM_CLICK, 0, 0);
                        prefslog.leave ();
                        break;

                    case ID_APPLY:
                        prefslog.enter ("ID_APPLY");
                        Context->BackupSettings = Context->Settings;
                        ApplySettingsV2 (Context->LC, Context->Settings);
                        SaveSettingsV2 (Context->LC);
                        SetFocus (Hwnd);
                        PickerBroadcastEvent (Context, EventApply);
                        EnableWindow (GetDlgItem (Hwnd, ID_APPLY), FALSE);
                        prefslog.leave ();
                        break;

                    case IDCANCEL:  // So that the ESC key works
                    case ID_CANCEL:
                        prefslog.enter ("ID_CANCEL");
                        Context->LC->SettingsV2 = Context->BackupSettings;
                        Context->Settings = Context->LC->SettingsV2;
                        PickerBroadcastEvent (Context, EventDestroy);

                        for (i = 0; i < Context->Sections.size(); i++)
                        {
                            DestroyWindow (Context->Sections[i].Hwnd);
                            Context->Sections[i].Hwnd = NULL;
                        }

                        delete Context;
                        prefslog.leave ();
                        EndDialog (Hwnd, NULL);
                        break;

                    case IDC_GOTODONATE: // Donate money via PayPal!
                        prefslog.enter ("IDC_GOTODONATE");
                        PickerSetSection (Context, "Donations");
                        prefslog.leave ();
                        break;
                }

                prefslog.leave ();
                break;
            
        }

        return (FALSE);
    } unguard;
}

void DoPreferences (HWND Parent, ListThreadContext *LC, const string &InitialPage)
{
    guard
    {
        PrefsContext *Context;

        prefslog.enter ("DoPreferences");
        prefslog << "Global defaults: " << GlobalDefaults << endl;
        prefslog << "Current vars: " << LC->SettingsV2 << endl;

        Context = new PrefsContext;
        Context->LC = LC;
        Context->InitialPage = InitialPage;

        DialogBoxParam (GetModuleHandle (NULL), MAKEINTRESOURCE(IDD_PREFSV2), Parent, PreferencesDialogV2, (LPARAM) Context);
        SaveSettingsV2 (LC);
        //delete Context; // done by PreferencesDialogV2
        
        prefslog << "New vars: " << LC->SettingsV2 << endl;
        prefslog.leave ();

        return;
    } unguard;
}
