#ifndef _LIST_H
#define _LIST_H


#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "ListTypes.h"
#include "ListMisc.h"
#include "../libs/pcre/src/pcre.h"
#include "../libs/pcre/src/pcreposix.h"
#include "../libs/gdicons/gdicons.h"
#include "../C_APILib/C_ShObj.h"
#include "Search.h"
#include "CSearch.h"
#include "VarList.h"
#include "CMutex.h"
#include "CParser2.h"


typedef vector<cText> cTextString;


#define HIBOX(a) MessageBox (NULL, a, "Hi", MB_OK)


#include "CCmdQueue.h"


#ifdef NDEBUG
    #ifdef LISTXP_CHECKED
        #define LISTTITLE   "ListXP (Checked)"
    #else
        #define LISTTITLE   "ListXP"
    #endif
#else
    #define LISTTITLE   "ListXP (Debug Build)"
#endif


//#define LISTHOMEPAGE "http://www.wsu.edu/~rolo/ListXP.html"
#define LISTHOMEPAGE "http://www.listxp.com"
#define LISTEMAIL    "rbrewster@wsu.edu"


#define LISTVER "1.0"
#define LISTSETTINGSVER "0.01"

// When we hit v1.0 we don't want to print this crap? We'll see.
#define LISTVERSION LISTVER
//#define LISTVERSION


// User must press ESC two times in this many milliseconds to actually quit
#define ESCTIMEOUT 1000


// Global CPU feature flags
extern bool HaveMMX;
extern bool HaveSSE;
extern bool HaveSSE2;
extern int  CPUCount;


extern int __stdcall WinMain (HINSTANCE HInstance, HINSTANCE HPrevInstance, LPSTR CmdLine, int ShowCmd);
extern DWORD WINAPI ListThread (LPVOID Context);
extern bool ChooseFile (HWND ParentHwnd, string &Result, string WindowName = "Open", const char *FileSpec = "");
extern bool ChooseDirectory (HWND ParentHwnd, string &Result, char *InitialDir, string WindowName);
extern void TurnOffTimer (HWND Hwnd);
extern void TurnOnTimer (HWND Hwnd);
extern void StreamCopy (void *Dst, void *Src, uint32 Bytes); // use this instead of memcpy
extern void CenterWindow (HWND Hwnd);
extern void RunShell (HWND Hwnd, const char *Command);
extern void ApplyAlwaysOnTop (HWND Hwnd, bool OnTop);
extern bool SetWindowAlphaValues (HWND Hwnd, bool Enable, uint8 Alpha);
extern bool IsWindowAlphaAllowed (void);
extern bool CanUseStreamingCopy (void);


extern string NewFileName;
extern volatile bool UseNewFileName;


class CFastFile;
class CLinesDB;
class CParser;
class ListThreadContext;


typedef struct
{
    char *CommandLine;
    OptionList *GlobalOptions; // options that *all* files will share!
    GDICons *Screen;
    void *LTC; // typecast to a ListThreadContext ... ListThread fills this out after 
               // being started so that WinMain can use the information.
    volatile bool DoneInit; // ListThread() sets this to true when it's ready for its creator to continue
    int ShowCmd;
} ListStartContext;


#define SEARCHTYPE_LITERAL 0 
#define SEARCHTYPE_BOOLEAN 1
#define SEARCHTYPE_REGEX   2


// File parser management
class CParser2;
typedef CParser2 *(*CreateCParser) (width_t, bool, uint32, cFormat, uint16);  // Function to create a CParser

typedef int (*IdentifyCParser) (uint8 *); // Function to aid in detecting the proper file parser to use
                                          // what we do is pass it the first 512 bytes of the file and
                                          // you return a value from 0 to 10 that says how "strongly" you
                                          // feel about being the parser for the file: 0 means "nah" and
                                          // 10 means "YES ME FOR SURE DAMMIT"

#define PARSER_MENUBASE 50000 // for HMENU handling, since we generate the Parsers menu at runtime

// Custom window procedure messages
#define WM_UPDATE_DISPLAY  (WM_USER + 2000)


extern void      InitParserSystem (void);
extern int       GetParserCount   (void);
extern string    GetParserName    (int Indice);
extern CParser2 *CreateParser     (ListThreadContext *LC, int Indice);
extern int       IdentifyParser   (int Indice, uint8 *Data);


typedef struct
{
    int NewWidth;
    int NewHeight;
    bool FudgeX;
    bool FudgeY;
    bool Maximizing; // if this is set to TRUE then the new width/height values will not be saved to the varlist
} ListResizeInfo;


// Passed in with LCMD_UPDATE_AVAILABLE message
class UpdateInfo
{
public:
    string NewVersion;
    string Message;
};

// This message is sent to the main message queue to say "yes, really quit!"
#define WM_REALLY_QUIT (WM_USER + 1)

// Messages that can be sent from ListMain to List

// Do a resize. Ptr = pointer to ListResizeInfo struct which will be delete-d automatically
#define LCMD_RESIZEXY                 (WM_USER + 256)

// None of these commands take any parameters for input
#define LCMD_CHECK_SCANNING           255

// Parm1-4 = not used, Ptr = cast to (char *) for filename, then deallocated using free()
#define LCMD_OPEN_FILE                213

#define LCMD_INPUT_PAGE_LEFT          254
#define LCMD_INPUT_PAGE_RIGHT         253
#define LCMD_INPUT_COLUMN_LEFT        252
#define LCMD_INPUT_COLUMN_RIGHT       251
#define LCMD_INPUT_PAGE_UP            250
#define LCMD_INPUT_PAGE_DOWN          249
#define LCMD_INPUT_LINE_UP            248
#define LCMD_INPUT_LINE_DOWN          247
#define LCMD_INPUT_GO_HOME            246
#define LCMD_INPUT_GO_END             245

// Turns help display on or off
#define LCMD_HELP_DISPLAY_TOGGLE      244

// Turns help display to the next help page
#define LCMD_HELP_DISPLAY_NEXT_PAGE   212

#define LCMD_TOGGLE_INFO_DISPLAY      243
#define LCMD_TOGGLE_HEX_DISPLAY       242

// Parm1 = log2(new_wordsize)
#define LCMD_SET_HEX_WORDSIZELOG2     201

#define LCMD_TOGGLE_LINE_WRAPPING     239
#define LCMD_TOGGLE_JUNK_FILTER       211
#define LCMD_TOGGLE_RULER             207

// Parm1 = 0 or 1
#define LCMD_SET_TRANSPARENCY         199

// Parm1 = [0,255] for alpha level
#define LCMD_SET_ALPHA_LEVEL          198

// simply applys transparent/alpha setting (for if you set those values manually through LC->SettingsV2 isntead of LCMD_SET_whatever)
#define LCMD_APPLY_TRANSPARENCY       197

#define LCMD_TOGGLE_TRANSPARENCY      196

// Parm1 = 0 for false, 1 for true
#define LCMD_SET_LINE_WRAPPING        214

#define LCMD_INPUT_OPEN               241
#define LCMD_INPUT_OPEN_NEW           240 

#define LCMD_QUIT                     100


// Parm1 = SEARCHTYPE_*, other parms unused
#define LCMD_SEARCH_SET_TYPE          238

// No parameters
#define LCMD_SEARCH_TOGGLE_TYPE       237 

// No parameters
#define LCMD_SEARCH_RESET             236

// No parameters
#define LCMD_SEARCH_TOGGLE_MATCH_CASE 235

// No parameters
#define LCMD_SEARCH_TOGGLE_ANIMATED   229

// Parm1 = 0 for false, 1 for true
#define LCMD_SEARCH_SET_ANIMATED      230

// Parm1 = 0 for false, 1 for true
#define LCMD_SEARCH_SET_MATCH_CASE    234

// No parameters
#define LCMD_SEARCH_CANCEL            233

// No paramters
#define LCMD_SEARCH_FIND_NEXT         232

// No parameters
#define LCMD_SEARCH_TOGGLE_DIRECTION  227

// Parm1 = 0 for up, 1 for down
#define LCMD_SEARCH_SET_DIRECTION     228

// No parameters
#define LCMD_FILE_RESET               231

// Parm1 = line number (lower 32-bits), Parm2 = line number (upper 32-bits)
// (use LODWORD and HIDWORD macros)
// Send ordinal values, i.e. [0,2^64-1]
// User is given cardinal values, i.e. [1,2^64]
// So if you ask user for a line #, subtract 1 and then send it.
#define LCMD_SET_LINE                 226

// Parm1 = column number
// Send ordinal values, i.e. [0,32767]
#define LCMD_SET_COLUMN               223

// No parameters
#define LCMD_SETTINGS_LOAD            225

// No parameters
#define LCMD_SETTINGS_SAVE            224

// Parm1 = parser indice
#define LCMD_SET_PARSER               220

// Ptr = pointer (char *) to string. This string will be free()'d!
// Or you can send NULL to set the text to blank
#define LCMD_SET_INFO_TEXT            219

// No parameters
#define LCMD_UPDATE_INFO              218

// No parameters
#define LCMD_UPDATE_TEXT              217

// No parameters
#define LCMD_LOOP_BREAK               216

// Parm1 = 0/1 for false/true
#define LCMD_SET_ALWAYS_ON_TOP        215

// Parm1 = line to toggle bookmark of (low 32-bits), Parm2 = high 32 bits
#define LCMD_TOGGLE_USER_BOOKMARK     210

// No parameters
#define LCMD_TOGGLE_SHOW_MARK_ZONE    209

// Parm1 = new tab size (1 through 64)
#define LCMD_SET_TABSIZE              208

// Parm1 = 0=off, 1=on
#define LCMD_SET_TAILING              205

// No parameters
#define LCMD_TOGGLE_TAILING           206

// Used to signal the main ListThread that the file has changed.
// No parameters
#define LCMD_FILE_CHANGED             204

// Used to signal that it has been determined that an update is available
// Ptr = cast to pointer to UpdateInfo class, which is deallocated using delete
#define LCMD_UPDATE_AVAILABLE         203

// No parameters
#define LCMD_EDIT                     202

// Parm1 = 0/1 for false/true for 2nd parameter to ResetLinesDBs()
#define LCMD_RESET_LINESDBS           200



#include "ListDefaults.h"

class CHexLinesDB;
class CFileLinesDB;

#include "CFileContext.h"


// Aww. Our "global" variables.
class ListThreadContext
{
public:
    ListStartContext *LSC;

    //ListSettings Settings;
    VarListExt SettingsV2;

    GDICons *Screen;
    CFileContext *FileContext;  // the current file context

    /*
    list<CFileContext *> AllContexts;
    int CurrentContext; // array indice
    */

    volatile bool WantToQuit;
    volatile bool WaitForScan;

    bool ShowMarkZone;

    string InfoText;  // one time info text display
    int UpdateCount; // just a general "how many times we gone through our update deal" thingy
    volatile bool UpdateInfo;  // top and bottom lines
    volatile bool UpdateText;  // body of text

    int DisplayHelp;  // 0 = no help, >0 = which page to display
    bool DisplayInfo;
    bool DisplayRuler;

    bool JunkFilter; // will filter <32 and >127 characters

    // Misc volatile settings
    bool AlwaysOnTop;
    bool LessVisUpdates; // update screen less often to improve performance in TS/RDC

    // Scrolling faster
    volatile int VScrollAccumulate;
    volatile bool DoVScroll;

    // Message queue between List.cpp and ListMain.cpp
    CCmdQueue CmdQueue;

    //SharedObject ListMutex;
    CMutex ListMutex;

    // You gotta hit ESC twice within 500ms to quit
    DWORD EscTimestamp;

    // Timestap the last time we checked the filesize
    DWORD SizeCheckTimestamp;

    // How much time must elapse before we check for a file size change?
    DWORD SizeCheckInterval;

    // Stats
//#define SEARCH_BENCH
#ifdef SEARCH_BENCH
    double SearchedLinesPerSec;
#endif

//#define SCAN_BENCH
#ifdef SCAN_BENCH
    DWORD ScanBenchTimeStamp;
#endif
};


extern wchar_t ToLowerTable[65536];   // ToLowerTable[i] = tolower(i)
extern wchar_t IdentityTable[65536];  // IdentityTable[i] = i


extern void OpenInNewWindow (HWND Hwnd, const string &FileName);
extern void ParseFileName (ListThreadContext *LC, string &FileName);
extern void DrawInfoLines (ListThreadContext *LC);
extern void DrawTextBody (ListThreadContext *LC, int FirstLine = 0, int LineCount = -1);
extern void UpdateScrollBars (ListThreadContext *LC);
extern string GetLineInput (ListThreadContext *LC, string Prompt);
extern bool HandleInput (ListThreadContext *LC);
extern void HandleCommands (ListThreadContext *LC);
extern bool DoSearch (ListThreadContext *LC);
extern string GetShortFileName (string LongFileName);
extern DWORD WINAPI ListThread (LPVOID Context);
extern void SetListTitle (ListThreadContext *LC);
extern void SetParser (ListThreadContext *LC, int ParserIndice);
extern void SetSearchPattern (ListThreadContext *LC, const char *NewPattern);
extern char *AddCommas (char *Result, uint64 Number);
extern bool GetLineRange (ListThreadContext *LC, uint64 Start, uint64 End, uint64 &LastResult) throw (runtime_error);
extern CParser2::TextLine *GetLine (ListThreadContext *LC, uint64 Line) throw (runtime_error);
extern CParser2::TextLine *AccumulateLines (ListThreadContext *LC, uint64 First, uint64 Last, bool &FreeableResult);
extern bool SearchLine (ListThreadContext *LC, uint64 Line, CSearch::MatchExtentGroup *ExtentResult);
extern bool SearchLineV (ListThreadContext *LC, uint64 Start, uint64 End, uint64 &EndResult, uint64 &FoundResult, CSearch::MatchExtentGroup *ExtentResult, bool Pinpoint = true) throw (runtime_error);
extern uint64 SearchRange (ListThreadContext *LC, uint64 Start, const uint64 End, CSearch::MatchExtentGroup *ExtentResult);
extern uint64 SearchRangeRev (ListThreadContext *LC, uint64 Start, const uint64 End, CSearch::MatchExtentGroup *ExtentResult);
extern void JumpToLine (ListThreadContext *LC, uint64 JumpHere, bool Center);
extern void DisableHighlight (ListThreadContext *LC);
extern void SetHighlight (ListThreadContext *LC, bool Enable, uint64 Line, const CSearch::MatchExtentGroup &Extents);
extern bool NormalLineToHexLine (ListThreadContext *LC, uint64 NormalLine, uint64 &HexLineResult);
extern bool HexLineToNormalLine (ListThreadContext *LC, uint64 HexLine, uint64 &NormalLineResult);

// Returns 'true' if the file has changed with regard to the cached file size and timestamp info that CFastFile keeps track of
extern bool HasFileChanged (CFastFile *File);

extern bool ScreenXYtoLineCol (ListThreadContext *LC, int X, int Y, uint64 &LineResult, uint32 &ColResult);


void AddSimpleCommand (ListThreadContext *LC, uint32 Command); // Sends CCommand(Command,0,0,0,0,NULL)
void AddCommand (ListThreadContext *LC, uint32 Command, uint32 Parm1, uint32 Parm2, uint32 Parm3, uint32 Parm4, void *Ptr);
void AddSimpleCommandUrgent (ListThreadContext *LC, uint32 Command); // Sends CCommand(Command,0,0,0,0,NULL)
void AddCommandUrgent (ListThreadContext *LC, uint32 Command, uint32 Parm1, uint32 Parm2, uint32 Parm3, uint32 Parm4, void *Ptr);
void ReplaceSimpleCommand (ListThreadContext *LC, uint32 Command); // Replaces
void ReplaceCommand (ListThreadContext *LC, uint32 Command, uint32 Parm1, uint32 Parm2, uint32 Parm3, uint32 Parm4, void *Ptr);


#endif // _LIST_H

