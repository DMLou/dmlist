#include "List.h"
#include <algorithm>
#include <shlwapi.h>
#include <Commdlg.h>
#include <mmsystem.h>
#include <shellapi.h>
#include <fstream>
#include "CFastFile.h"
#include "CLinesDB.h"
#include "CFileLinesDB.h"
#include "CHexLinesDB.h"
#include "CJobQueue.h"
#include "Settings.h"
#include "resource.h"
#include "../libs/pcre/src/pcre.h"
#include "CSearchLiteral.h"
#include "CSearchKMP.h"
#include "CSearchRegex.h"
#include "CSearchBoolean.h"
#include "HelpText.h"
#include "Hive.h"
#include "NoticeBox.h"
#include "Updates.h"


#pragma comment (lib,"winmm.lib")


#define NORMALSLEEP 10
#define REMOTESLEEP 100

// Get rid of std::min and std::max polluting the namespace
// This is a temporary hack for now. Will clean up better later.
#undef min
#undef max

// Lookup table to avoid using tolower(), which is a function with
// external linkage that can be replaced with this simple table lookup.
// Initialized in ListThread
wchar_t ToLowerTable[65536];
wchar_t IdentityTable[65536];


void OpenInNewWindow (HWND Hwnd, const string &FileName)
{
    char EXEName[2048];

    GetModuleFileName (GetModuleHandle (NULL), EXEName, sizeof (EXEName));
    ShellExecute (Hwnd, "open", EXEName, FileName.c_str(), ".", SW_SHOWNORMAL);
    return;
}


CFastFile *OpenFile (ListThreadContext *LC, const string &FileName)
{
    guard
    {
        CFastFile *ReturnFile;
        uint64 FileSize;
        CFastFile *TempFile;

        // Get the file size
        TempFile = new CFastFile (FileName,
                                LC->SettingsV2.GetValUint32 ("BlockSizeLog2"),
                                1024 * LC->SettingsV2.GetValUint32("FileCacheSizeKB"),
                                GENERIC_READ,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                OPEN_EXISTING,
                                0,
                                bool(LC->SettingsV2.GetValUint32("Prefetching")));

        if (TempFile->GetLastError() != CFastFile::NoError)
            return (TempFile);

        FileSize = TempFile->GetFileSize();
        delete TempFile;

        // Get the actual file stuffs!
        ReturnFile = new CFastFile (FileName, 
                                    LC->SettingsV2.GetValUint32 ("BlockSizeLog2"),
                                    1024 * LC->SettingsV2.GetValUint32("FileCacheSizeKB"),
                                    GENERIC_READ, 
                                    FILE_SHARE_READ | FILE_SHARE_WRITE, 
                                    OPEN_EXISTING, 
                                    FILE_FLAG_SEQUENTIAL_SCAN |
                                        ((LC->SettingsV2.GetValUint32("AllowNonCached")  &&  
                                            FileSize >= LC->SettingsV2.GetValUint64("NonCachedMinSizeKB") * 1024) ? 
                                                FILE_FLAG_NO_BUFFERING : 0),
                                    bool(LC->SettingsV2.GetValUint32("Prefetching")));

        return (ReturnFile);
    } unguard;
}


bool HasFileChanged (CFastFile *File)
{
    guard
    {
        if (File->GetFileSize() != File->GetRealFileSize()  ||
            (File->GetFileTime().dwHighDateTime != File->GetRealFileTime().dwHighDateTime  &&
            File->GetFileTime().dwLowDateTime != File->GetRealFileTime().dwLowDateTime))
        {
            return (true);
        }

        return (false);
    } unguard;
}


// Parser system management
typedef struct
{
    string Name;
    CreateCParser Creator;
    IdentifyCParser Identifier;
} ParserInfo;


vector<ParserInfo> ParserList;


#include "CParserASCII.h"
#include "CParserBinary.h"
#include "CParserUnicode.h"
#include "CParserMIRC.h"
#include "CParser2.h"


void InitParserSystem (void)
{
    guard
    {
        ParserInfo Info;

        ParserList.clear ();

        Info.Name = "Text";
        Info.Creator = CreateCParserASCII;
        Info.Identifier = IdentifyCParserASCII;
        ParserList.push_back (Info);

        Info.Name = "Binary";
        Info.Creator = CreateCParserBinary;
        Info.Identifier = IdentifyCParserBinary;
        ParserList.push_back (Info);

        Info.Name = "Unicode Text";
        Info.Creator = CreateCParserUnicode;
        Info.Identifier = IdentifyCParserUnicode;
        ParserList.push_back (Info);

        Info.Name = "mIRC log file";
        Info.Creator = CreateCParserMIRC;
        Info.Identifier = IdentifyCParserMIRC;
        ParserList.push_back (Info);

        return;
    } unguard;
}


int GetParserCount (void)
{
    guard
    {
        return (ParserList.size());
    } unguard;
}


string GetParserName (int Indice)
{
    guard
    {
        if (Indice >= ParserList.size())
            return (string("n/a"));

        return (ParserList[Indice].Name);
    } unguard;
}


CParser2 *CreateParser (ListThreadContext *LC, int Indice)
{
    guard
    {
        return (ParserList[Indice].Creator (LC->FileContext->MaxWidth,
                                            bool(LC->SettingsV2.GetValUint32("WrapText")),
                                            LC->SettingsV2.GetValUint32("TabSize"),
                                            CMakeCFormat (
                                                LC->SettingsV2.GetValUint32("TextColor.Foreground"),
                                                LC->SettingsV2.GetValUint32("TextColor.Background")),
                                            (uint16)Indice));
    } unguard;
}


int IdentifyParser (int Indice, uint8 *Data)
{
    guard
    {
        return (ParserList[Indice].Identifier (Data));
    } unguard;
}


CLinesDB *GetLinesDB (ListThreadContext *LC, bool Hex)
{
    guard
    {
        if (Hex)
            return ((CLinesDB *) new CHexLinesDB (LC->FileContext->File, 
                                                  LC->SettingsV2.GetValUint32 ("HexUseConstantWidth") ? LC->SettingsV2.GetValUint32 ("HexConstantWidth") : LC->Screen->GetConsWidth(), 
                                                  1024,
                                                  1 << LC->SettingsV2.GetValUint32 ("HexWordSizeLog2"),
                                                  LC->SettingsV2.GetValUint32 ("HexLittleEndian")));
        else
            return ((CLinesDB *) new CFileLinesDB (LC->FileContext->File, 
                                                   LC->FileContext->Parser, 
                                                   LC->FileContext->MaxWidth, 
                                                   LC->SettingsV2.GetValUint32 ("SeekGranularityLog2"),
                                                   LC->SettingsV2.GetValUint32 ("CacheChunks"),
                                                   LC->SettingsV2.GetValUint32 ("TabSize")));
    } unguard;
}


// Context is a pointer to a ListThreadContext structure
void DoneTailingCallback (bool Success, void *Context)
{
    guard
    {
        ListThreadContext *LC;

        LC = (ListThreadContext *) Context;
        LC->FileContext->NormalLinesDB->ClearCache ();

        if (LC->SettingsV2.GetValUint32 ("TailingJumpToEnd") == 1)
            AddSimpleCommand (LC, LCMD_INPUT_GO_END);

        AddSimpleCommand (LC, LCMD_UPDATE_INFO);
        AddSimpleCommand (LC, LCMD_UPDATE_TEXT);

        return;
    } unguard;
}


void ResetLinesDBs (ListThreadContext *LC, bool ForceReset, bool FlushIOCache = false)
{
    guard
    {
        uint64 HexSeek;
        bool SetHexLine = false;
        uint16 NewMaxWidth;

        if (LC->FileContext->WrapText)
            NewMaxWidth = LC->Screen->GetMaxX() + 1;
        else
            NewMaxWidth = CLINESDB_MAXWIDTH;

        LC->FileContext->MaxWidth = NewMaxWidth;

        LC->FileContext->File->ResetFileSize ();
        LC->FileContext->File->ResetFileTime ();

        if (FlushIOCache)
            LC->FileContext->File->ResetCache ();

        // If we're in hex mode we want to make sure we keep the same data on the screen ...
        if (LC->FileContext->HexLinesDB != NULL  &&  LC->FileContext->HexMode)
        {
            HexSeek = LC->FileContext->HexLinesDB->LineToSeekPoint (LC->FileContext->CurrentLine);
            SetHexLine = true;
        }

        // Hex DB needs to be reset if Hex.DisplayWidth != NewDisplayWidth,
        // or if WordSizeLog2 != Settings->WordSizeLog2
        if (LC->FileContext->HexLinesDB == NULL  || 
            ForceReset  ||  
            LC->FileContext->HexLinesDB->GetDisplayWidth() != LC->Screen->GetConsWidth() ||
            LC->FileContext->HexLinesDB->GetWordSizeLog2() != LC->SettingsV2.GetValUint32 ("HexWordSizeLog2"))
        {
            if (LC->FileContext->HexLinesDB != NULL)
                delete LC->FileContext->HexLinesDB;

            LC->FileContext->HexLinesDB = (CHexLinesDB *) GetLinesDB (LC, true);
        }

        // Normal DB needs to be reset if Norm.MaxWidth != LC->Settings.MaxWidth
        // And if the normal DB needs to be reset, then the parser needs to be reset
        if (LC->FileContext->NormalLinesDB == NULL  || 
            ForceReset  ||  
            LC->FileContext->NormalLinesDB->GetMaxWidth() != LC->FileContext->MaxWidth)
        {
            SetParser (LC, LC->FileContext->ParserIndice);

            if (LC->FileContext->NormalLinesDB != NULL)
                delete LC->FileContext->NormalLinesDB;

            LC->FileContext->NormalLinesDB = (CFileLinesDB *) GetLinesDB (LC, false);

            if (!LC->FileContext->HexMode)
            {
                LC->FileContext->CurrentLine = 0;
                LC->UpdateInfo = true;
                LC->UpdateText = true;
                DisableHighlight (LC);
            }
        }

        // Set ActiveLinesDB appropriately
        if (LC->FileContext->HexMode)
            LC->FileContext->ActiveLinesDB = LC->FileContext->HexLinesDB;
        else
            LC->FileContext->ActiveLinesDB = LC->FileContext->NormalLinesDB;

        //
        if (SetHexLine)
        {
            uint64 NewHexLine;
            NewHexLine = LC->FileContext->HexLinesDB->SeekPointToLine (HexSeek);
            LC->CmdQueue.SendCommandUrgentAsync (CCommand (LCMD_SET_LINE, LODWORD(NewHexLine), HIDWORD(NewHexLine), 0, 0, NULL));
            AddSimpleCommand (LC, LCMD_UPDATE_TEXT);
        }

        LC->FileContext->OneTimeDoneScanning = false;
        return;
    } unguard;
}


void ParseFileName (ListThreadContext *LC, string &FileName)
{
    guard
    {
        // Remove surrounding quotes.
        if (*(FileName.begin()) == '"')
        {
            FileName.erase (FileName.begin());
        }

        if (*(FileName.end() - 1) == '"')
        {
            FileName.erase (FileName.end() - 1);
        }

        return;
    } unguard;
}


char *AddCommas (char *Result, uint64 Number)
{
    guard
    {
        char  Temp[256];
	    int   TempLen;
	    char *p = NULL;
	    int   AddCommas = 0;
	    char *StrPosResult = NULL;
	    char *StrPosOrig = NULL;

	    // we get the string form of the number, then we count down w/ AddCommas
	    // while copying the string from Temp1 to Result. when AddCommas % 3  == 1,
	    // slap in a commas as well, before the #.
	    sprintf (Temp, "%I64u", Number);
	    AddCommas = TempLen = strlen (Temp);
	    StrPosOrig   = Temp;
	    StrPosResult = Result;

	    while (AddCommas)
	    {
		    if ((AddCommas % 3) == 0  &&  AddCommas != TempLen) // avoid stuff like ",345"
		    {
			    *StrPosResult = ',';
			    StrPosResult++;
		    }

		    *StrPosResult = *StrPosOrig;
		    StrPosResult++;
		    StrPosOrig++;

		    *StrPosResult = 0;

		    AddCommas--;
	    }

	    return (Result);
    } unguard;
}


uint64 GetCenterLine (ListThreadContext *LC)
{
    guard
    {
        return (LC->FileContext->CurrentLine + (LC->Screen->GetMaxConsY() / 2) - 1);
    } unguard;
}


uint64 GetTopLine (ListThreadContext *LC)
{
    guard
    {
        return (LC->FileContext->CurrentLine);
    } unguard;
}


uint64 GetBottomLine (ListThreadContext *LC)
{
    guard
    {
        return (LC->FileContext->CurrentLine + LC->Screen->GetMaxConsY() - 2);
    } unguard;
}


// Takes a console X,Y coordinate and translates that into the line/column
// within the file. Returns false if the conversion is not possible (i.e.
// X lies past the end of a line or something)
bool ScreenXYtoLineCol (ListThreadContext *LC, int X, int Y, uint64 &LineResult, uint32 &ColResult)
{
    guard
    {
        uint64 Line;
        uint64 Column;
        CParser2::TextLine *Text;

        // First convert X,Y into Line,Column
        if (Y < 1  ||  Y > LC->Screen->GetMaxConsY() - 1)
            return (false);

        Line = Y - 1; // -1 to account for infobar at top
        Line += LC->FileContext->CurrentLine;
        Column = X;

        if (!LC->FileContext->NormalLinesDB->GetLine (Line, &Text))
            return (false);

        if (Text == NULL)
            return (false);

        if (Column >= Text->Chars)
            return (false);

        LineResult = Line;
        ColResult = Column;
        return (true);
    } unguard;
}

// Given a line # and a character offset, this function will return the proper line,char
// pair that you really want.
// Ok that doesn't really work very well as a description.
// So here's an example.
// Let's say you want "line 0, offset 500"
// But line 0 is only 40 characters long
// Well, then we'll look for "line 1, offset 460" (500-40=460)
// and recurse (well, loop) until we find the 500th character after the start of line 0
bool LineOffsetConversion (ListThreadContext *LC, uint64 Line, uint64 Offset, uint64 &NewLine, uint64 &NewOffset)
{
    guard
    {
        CParser2::TextLine *Text;
        uint64 TotalLines;

        TotalLines = LC->FileContext->NormalLinesDB->GetTotalLines ();

        while (Line < TotalLines)
        {
            if (!LC->FileContext->NormalLinesDB->GetLine (Line, &Text))
                return (false);

            if (Offset < Text->Chars)
            {   // Done!
                NewLine = Line;
                NewOffset = Offset;
                return (true);
            }

            Offset -= Text->Chars;
            Line++;
        }

        return (false);
    } unguard;
}


void DrawInfoLines (ListThreadContext *LC)
{
    guard
    {
        cText C;
        cFormat InfoColor;
        char TopLeft[1024];
        int TopLeftLen;
        char TopRight[1024];
        int TopRightLen;
        char SpinText[1024];
        int SpinTextLen;
        char CommaBuf1[256];
        char CommaBuf2[256];
        char CommaBuf3[256];
        char CommaBuf4[256];
        int i;
        int MaxTLLen;
        BOOL OldAR;

        OldAR = LC->Screen->GetAutoRefresh ();
        LC->Screen->SetAutoRefresh (FALSE);

        InfoColor = CMakeCFormat (LC->SettingsV2.GetValUint32 ("InfoColor.Foreground"),
                                  LC->SettingsV2.GetValUint32 ("InfoColor.Background"));

        LC->Screen->SetForeground (InfoColor.Foreground);
        LC->Screen->SetBackground (InfoColor.Background);
        LC->Screen->SetUnderline (false);

        C.Format.Foreground = InfoColor.Foreground;
        C.Format.Background = InfoColor.Background;
        C.Format.Underline = false;
        C.Refresh = true;
        C.Char = ' ';

        // Top line
        for (i = 0; i <= LC->Screen->GetMaxX(); i++)
            LC->Screen->Poke (i, 0) = C;

        // Build top-left string
        TopLeftLen = sprintf (TopLeft, 
                            "%c%s", 
                            HasFileChanged (LC->FileContext->File) ? '*' : ' ',
                            LC->FileContext->FileName.c_str());

        TopRightLen = sprintf (TopRight,
            "%s:%s / %s:%s %c%c%c%c%c%c ",
            AddCommas(CommaBuf1, LC->FileContext->CurrentLine + 1),
            AddCommas(CommaBuf2, LC->FileContext->CurrentColumn + 1),
            AddCommas(CommaBuf3, LC->FileContext->ActiveLinesDB->GetTotalLines()),
            AddCommas(CommaBuf4, LC->FileContext->ActiveLinesDB->GetLongestLineWidth() ? LC->FileContext->ActiveLinesDB->GetLongestLineWidth() : 0),
            (bool(LC->SettingsV2.GetValUint32("AnimatedSearch")) ? 'a' : '_'),
            (LC->FileContext->HexMode ? 'h' : '_'),
            (LC->JunkFilter ? 'j' : '_'),
            (LC->FileContext->Tailing ? 't' : '_'),
            (LC->Screen->GetGDIOutput()->IsUsingUnicode() ? 'u' : '_'),
            (LC->FileContext->WrapText ? 'w' : '_'));

        // Build the percentage/spinny text string
        if (LC->FileContext->NormalLinesDB->IsDoneScanning())
        {
            SpinTextLen = 0;
            SpinText[0] = '\0';
        }
        else
        {
            SpinTextLen = sprintf (SpinText, 
                                "(%.1f%%) %c",
                                std::min(100.0f, std::max(0.0f, LC->FileContext->NormalLinesDB->GetScanPercent())),
                                (("/-\\|")[(GetTickCount() / 64) % 4])); // Oh gosh, most evil line of code ever :)
        }

        // Now we wish to start print it all, yo

        // Figure out maximum length that TopLeft can be
        MaxTLLen = LC->Screen->GetConsWidth() - TopRightLen - SpinTextLen - 2;

        // If the filename text is too long, chop of the last 3 chars and replace them with ...
        if (TopLeftLen > MaxTLLen)
        {
            TopLeft[MaxTLLen] = '\0';
            TopLeft[MaxTLLen - 1] = '.';
            TopLeft[MaxTLLen - 2] = '.';
            TopLeft[MaxTLLen - 3] = '.';
        }

        // Now print it!
        LC->Screen->GoToXY (0, 0);
        LC->Screen->WriteText (TopLeft);
        LC->Screen->GoToXY (LC->Screen->GetMaxConsX() - TopRightLen - SpinTextLen, 0);
        LC->Screen->WriteText (TopRight);
        LC->Screen->WriteText (SpinText);

        // Bottom line
        LC->Screen->SetForeground (InfoColor.Foreground);
        LC->Screen->SetBackground (InfoColor.Background);
        LC->Screen->SetUnderline (false);

        for (i = 0; i <= LC->Screen->GetMaxX(); i++)
            LC->Screen->Poke (i, LC->Screen->GetMaxY()) = C;

        LC->Screen->GoToXY (1, LC->Screen->GetMaxY());

        LC->Screen->WriteTextf ("%s ", LC->InfoText.c_str());
        //AddCommand (LC, LCMD_SET_INFO_TEXT, 0, 0, 0, 0, NULL);

        if (LC->WaitForScan)
        {
            LC->Screen->WriteText ("(Waiting for file scan to finish) ");
        }
        else
        if (!LC->FileContext->NormalLinesDB->IsDoneScanning())
        {
            LC->Screen->WriteText ("(Scanning file) ");
        }

        // Search info pertains to NormalLinesDB, not Active/HexLinesDB
        if (LC->FileContext->DoingSearch)
        {
            LC->Screen->WriteTextf ("(Searching: %.1lf%%/%s/%s) ", 
                100.0 * (double(LC->FileContext->NextLineToSearch + 1) / double(LC->FileContext->NormalLinesDB->GetTotalLines())),
                AddCommas (CommaBuf1, LC->FileContext->NextLineToSearch + 1), 
                AddCommas (CommaBuf2, LC->FileContext->NormalLinesDB->GetTotalLines()));
        }
        else
        {
            LC->Screen->WriteTextf ("Next search from line %s.", AddCommas (CommaBuf1, LC->FileContext->NextLineToSearch + 1));

            if ((LC->Screen->GetConsWidth() - LC->Screen->GetAbsXPos()) <= 38)
            {
                LC->Screen->WriteTextf (" (%s/%s/%s)",
                    (LC->FileContext->SearchDirection == +1) ? "d" : "u",
                    (LC->FileContext->SearchType == SEARCHTYPE_LITERAL) ? "l" :
                    (LC->FileContext->SearchType == SEARCHTYPE_REGEX) ? "r" : "b",
                    LC->FileContext->SearchCaseSensitive ? "s" : "i");
            }
            else
            {            
                LC->Screen->WriteTextf (" (%s, %s, %s)", 
                    (LC->FileContext->SearchDirection == +1) ? "down" : "up",
                    (LC->FileContext->SearchType == SEARCHTYPE_LITERAL) ? "literal" :
                    (LC->FileContext->SearchType == SEARCHTYPE_REGEX) ? "regex" : "boolean",
                    LC->FileContext->SearchCaseSensitive ? "case sensitive" : "case insensitive");
            }
        }

        // It's advisable to only enable one of these stat trackers at a time, btw :)

    #ifdef CLINESDB_STATS
        LC->Screen->WriteTextf (" (T:%I64u, H:%I64u, M:%I64u, L:%I64u)", 
            LC->FileContext->ActiveLinesDB->GetStats().TotalLineReads,
            LC->FileContext->ActiveLinesDB->GetStats().CacheHits,
            LC->FileContext->ActiveLinesDB->GetStats().CacheMisses,
            LC->FileContext->ActiveLinesDB->GetStats().Locks);

        AddSimpleCommand (LCMD_UPDATE_INFO);
    #endif

    #ifdef CFASTFILE_STATS
        DWORD TotalTicks;
        double Rate; // bytes/second

        TotalTicks = GetTickCount() - LC->FileContext->File->GetStats().BirthTick;
        Rate = double(LC->FileContext->File->GetStats().DataRead) / (double(TotalTicks) / 1000.0);
        Rate /= 1024; // KB/second
        LC->Screen->WriteTextf (" (%.2lfKB/s, %I64u bytes, %ub cache)", Rate, LC->FileContext->File->GetStats().DataRead, LC->FileContext->File->GetCacheSize());

    #endif

    #ifdef SCAN_BENCH
        DWORD TimeDelta;
        double ScanRate; // lines/sec

        TimeDelta = GetTickCount() - LC->ScanBenchTimeStamp;
        ScanRate = double(LC->FileContext->NormalLinesDB->GetTotalLines()) / (double(TimeDelta) / 1000.0);
        LC->Screen->WriteTextf (" %.2lf lines/sec ", ScanRate);
    #endif

        LC->Screen->SetAutoRefresh (OldAR);
        LC->Screen->RefreshXY (0, 0, LC->Screen->GetMaxX(), 0);
        LC->Screen->RefreshXY (0, LC->Screen->GetMaxY(), LC->Screen->GetMaxX(), LC->Screen->GetMaxY());

        UpdateScrollBars (LC);

        return;
    } unguard;
}


void DrawTextBody (ListThreadContext *LC, int FirstLine, int LineCount)
{
    guard
    {
        int i;
        int OldAR;
        cFormat TextColor;
        bool IgnoreColors;

        IgnoreColors = bool(LC->SettingsV2.GetValUint32 ("IgnoreParsedColors"));

        if (LineCount == -1)
            LineCount = LC->Screen->GetMaxY() - 1;

        TextColor = CMakeCFormat (LC->SettingsV2.GetValUint32 ("TextColor.Foreground"),
                                  LC->SettingsV2.GetValUint32 ("TextColor.Background"));

        LC->Screen->SetForeground (TextColor.Foreground);
        LC->Screen->SetBackground (TextColor.Background);
        LC->Screen->SetUnderline (false);

        OldAR = LC->Screen->GetAutoRefresh();
        LC->Screen->SetAutoRefresh(FALSE);

        // Unicode?
        if (LC->FileContext->Parser->UseUnicodeRendering())
            LC->Screen->GetGDIOutput()->SwitchToUnicode();
        else
            LC->Screen->GetGDIOutput()->SwitchToANSI();

        // File info?
        if (LC->DisplayInfo)
        {
            cText C;
            char CommaBuf1[256];
            char CommaBuf2[256];
            char CommaBuf3[256];
            uint64 Usage;
            uint64 TotalUsage;

            C.Char = ' ';
            C.Format.Foreground = TextColor.Foreground;
            C.Format.Background = TextColor.Background;
            C.Format.Underline = false;
            C.Refresh = true;

            for (int y = 1 + FirstLine; y < 1 + FirstLine + LineCount; y++)
            {
                for (int x = 0; x <= LC->Screen->GetMaxX(); x++)
                {
                    LC->Screen->Poke (x, y) = C;
                }
            }

            LC->Screen->SetForeground (TextColor.Foreground);
            LC->Screen->SetBackground (TextColor.Background);
            LC->Screen->SetUnderline (false);

            LC->Screen->GoToXY (0, 1);
            LC->Screen->WriteTextf (" " LISTTITLE " " LISTVERSION " by Rick Brewster (rbrewster@wsu.edu) (" __DATE__ " @ " __TIME__ ")\n");
            LC->Screen->WriteTextf ("\n");
            LC->Screen->WriteTextf (" File Information\n");
            LC->Screen->WriteTextf (" ----------------\n");
            LC->Screen->WriteTextf (" Filename: %s\n", LC->FileContext->FileName.c_str());
            LC->Screen->WriteTextf (" Size:     %s bytes / %s KB / %s MB\n", 
                AddCommas (CommaBuf1, LC->FileContext->File->GetFileSize()), 
                AddCommas (CommaBuf2, LC->FileContext->File->GetFileSize() / uint64(1024)), 
                AddCommas (CommaBuf3, LC->FileContext->File->GetFileSize() / uint64(1048576)));

            LC->Screen->WriteTextf ("           %s%s text lines / %s hex lines\n", 
                AddCommas (CommaBuf1, LC->FileContext->NormalLinesDB->GetTotalLines()), 
                LC->FileContext->NormalLinesDB->IsDoneScanning() ? "" : "+",
                AddCommas (CommaBuf2, LC->FileContext->HexLinesDB->GetTotalLines()));

            LC->Screen->WriteTextf ("           %.2lf average characters per line\n", LC->FileContext->NormalLinesDB->GetAverageCharsPerLine());

            LC->Screen->WriteTextf ("\n");
            LC->Screen->WriteTextf (" Memory Usage (approximate)\n");
            LC->Screen->WriteTextf (" --------------------------\n");

            TotalUsage = 0;

            Usage = LC->FileContext->NormalLinesDB->GetSeekCacheMemUsage();
            LC->Screen->WriteTextf (" Seek point cache:          %s bytes for %s seek points\n", 
                AddCommas (CommaBuf2, Usage),
                AddCommas (CommaBuf1, LC->FileContext->NormalLinesDB->GetSeekCacheLength())); 
            TotalUsage += Usage;

            Usage = LC->FileContext->NormalLinesDB->GetTextCacheMemUsage();
            LC->Screen->WriteTextf (" Text cache:                %s bytes for %s text chunks\n", 
                AddCommas (CommaBuf2, Usage),
                AddCommas (CommaBuf1, LC->FileContext->NormalLinesDB->GetTextCacheChunks())); 
            TotalUsage += Usage;

            Usage = LC->FileContext->HexLinesDB->GetHexLineCacheMemUsage();
            LC->Screen->WriteTextf (" Hex mode line cache size:  %s bytes for %s lines of text\n",
                AddCommas (CommaBuf1, Usage),
                AddCommas (CommaBuf2, LC->FileContext->HexLinesDB->GetHexLineCacheLength()));
            TotalUsage += Usage;

            Usage = LC->FileContext->File->GetCacheMemUsage();

            if (!LC->FileContext->NormalLinesDB->IsDoneScanning())
            {
                Usage += LC->FileContext->File->GetMaxCacheMemUsage ();
            }

            LC->Screen->WriteTextf (" File I/O cache:            %s bytes (%s byte block size)\n",
                AddCommas (CommaBuf1, Usage),
                AddCommas (CommaBuf2, LC->FileContext->File->GetCacheBlockSize()));

            TotalUsage += Usage;

            LC->Screen->WriteTextf (" Total (approx.):           %s bytes\n",
                AddCommas (CommaBuf1, TotalUsage));

            LC->Screen->GoToXY (0, 0);

            if (!LC->FileContext->NormalLinesDB->IsDoneScanning())
                AddSimpleCommand (LC, LCMD_UPDATE_TEXT);

            if (LC->LessVisUpdates)
                Sleep (REMOTESLEEP);
            else
                Sleep (NORMALSLEEP);

            if (LC->Screen->GetConsHeight() < 25)
            {
                DrawInfoLines(LC);
            }
        }
        else
        // Help info?
        if (LC->DisplayHelp)
        {
            cText C;

            C.Char = ' ';
            C.Format.Foreground = TextColor.Foreground;
            C.Format.Background = TextColor.Background;
            C.Format.Underline = false;
            C.Refresh = true;

            LC->Screen->SetForeground (TextColor.Foreground);
            LC->Screen->SetBackground (TextColor.Background);
            LC->Screen->SetUnderline (false);

            LC->Screen->ClearXY (0, 1, LC->Screen->GetMaxX(), LC->Screen->GetMaxY() - 1);
            LC->Screen->GoToXY (0, 1);
            LC->Screen->WriteText (ListHelpHeader);
            LC->Screen->WriteTextf (" (Page %d of %d)                                                               \n", LC->DisplayHelp, ListHelpPageCount);
            LC->Screen->WriteText (ListHelpPages[LC->DisplayHelp - 1]);
            LC->Screen->GoToXY (0, 0);

            if (LC->Screen->GetConsHeight() < 25)
            {
                DrawInfoLines(LC);
            }
        }
        else
        // Normal text rendering
        {
            uint64 HighlightLine;
            uint64 CurrentColumn = LC->FileContext->CurrentColumn;
            CLinesDB *LinesDB = LC->FileContext->ActiveLinesDB;

            // If we're in hex mode, translate highlight line
            if (LC->FileContext->HexMode)
                NormalLineToHexLine (LC, LC->FileContext->HighlightLine, HighlightLine);
            else
                HighlightLine = LC->FileContext->HighlightLine;

            //for (i = FirstLine; i < FirstLine + LineCount; i++)
            for (i = FirstLine + LineCount - 1; i >= FirstLine; i--)
            {
                //cTextString Text;
                CParser2::TextLine *Text = NULL;
                bool FreeText = true;
                uint64 LineToDraw;
                vector<CParser2::FormatExtent>::const_iterator fit;
                int ScreenLine;
                bool Result;
                int j;

                LineToDraw = i + LC->FileContext->CurrentLine;
                ScreenLine = i + 1;

                if (LineToDraw >= LinesDB->GetTotalLines())
                {
                    Result = false; // simulate a "couldn't read this line from the file"
                    Text = BlankTextLine ();
                    FreeText = true;
                }
                else
                {
                    Result = LinesDB->GetCopyOfLine (LineToDraw, &Text);
                    FreeText = true;
                }

                if (!Result)
                {
                    if (!LinesDB->IsDoneScanning())
                        AddSimpleCommand (LC, LCMD_UPDATE_TEXT);

                    if (Text == NULL)
                    {
                        Text = BlankTextLine ();
                        FreeText = true;
                    }
                }

                // First draw the characters (i.e. apply Char)
                for (j = 0; j <= LC->Screen->GetMaxX(); j++)
                {
                    int indice;
                    int scolumn;
                    cChar c;

                    // Compute indice of Text->Text[] we should load from
                    indice = j + int(CurrentColumn);

                    // Is this in bounds?
                    if (indice < Text->Chars)
                        c = cChar(Text->Text[indice]); // yes, load from Text->Text[]
                    else
                        c = cChar(' ');               // no, draw a space

                    if (c == L'\n'  ||  c == L'\r'  ||  c == L'\0')
                        c = cChar(' ');

                    // Junk filter?
                    if (LC->JunkFilter)
                        c &= ~0x80; // turn off 7th bit.

                    // Draw!
                    cText &T = LC->Screen->Poke (j, ScreenLine);
                    T.Char = c;
                    T.Format = TextColor;
                    T.Refresh = true;
                }

                // Next, apply the color from Text->Formatting
                if (Text != NULL && !IgnoreColors)
                {
                    for (fit = Text->Formatting.begin(); fit != Text->Formatting.end(); ++fit)
                    {
                        int tbegin;  // left text indice for color
                        int tend; // right text indice for color

                        // Compute left/right string positions
                        tbegin = fit->Begin;
                        tend = tbegin + fit->Length;

                        // Translate to left/right screen coordinates
                        tbegin -= int(CurrentColumn);
                        tend -= int(CurrentColumn);

                        // Clip
                        if (tbegin > LC->Screen->GetMaxConsX())
                            continue;

                        if (tend <= 0)
                            continue;

                        if (tend > LC->Screen->GetConsWidth())
                        {
                            tend = LC->Screen->GetConsWidth();
                            if (tbegin >= tend)
                                continue;
                        }

                        if (tbegin < 0)
                        {
                            tbegin = 0;
                            if (tend <= tbegin)
                                continue;
                        }

                        // Otherwise, draw
                        for (j = tbegin; j < tend; j++)
                        {
                            cText &T = LC->Screen->Poke (j, ScreenLine);
                            T.Format = fit->Format;
                            T.Refresh = true;
                        }
                    }
                }

                // Do we highlight this line? If so, invert the whole line
                if (LC->FileContext->Highlight &&
                    LineToDraw == HighlightLine &&
                    1 == LC->SettingsV2.GetValUint32 ("Highlight.Line"))
                {
                    for (j = 0; j <= LC->Screen->GetMaxConsX(); j++)
                    {
                        cText &T = LC->Screen->Poke (j, ScreenLine);
                        T.Format.Foreground = 0x00ffffff - T.Format.Foreground;
                        T.Format.Background = 0x00ffffff - T.Format.Background;
                        T.Refresh = true;
                    }                        
                }

                bool DoHL = false;
                CSearch::MatchExtentGroup Extents;

                // Highlight search terms, if needed
                if (LC->FileContext->Highlight && 
                    LineToDraw == HighlightLine &&
                    !LC->FileContext->HexMode &&
                    1 == LC->SettingsV2.GetValUint32 ("Highlight.Terms"))
                {
                    DoHL = true;
                    Extents = LC->FileContext->HighlightSecondary;
                }

                if (DoHL)
                {
                    CSearch::MatchExtentGroup::iterator it;
                    int scrleft;
                    int scrright;
                    cFormat hfmt;

                    hfmt = CMakeCFormat (LC->SettingsV2.GetValUint32 ("Highlight.Foreground"),
                                         LC->SettingsV2.GetValUint32 ("Highlight.Background"));

                    if (LC->SettingsV2.GetValUint32 ("Highlight.Underline") == 1)
                        hfmt.Underline = true;

                    // Set up clipping min/max: scrleft/right correspond to the left-most and
                    // right-most column indices on screen
                    scrleft = LC->FileContext->CurrentColumn;
                    scrright = std::min(Text->Chars, static_cast<uint32_t>(scrleft + LC->Screen->GetMaxConsX()));

                    for (it = Extents.begin(); it != Extents.end(); ++it)
                    {
                        uint64 line;
                        uint64 begin;
                        uint64 end;
                        uint64 offset;

                        line = LineToDraw;
                        begin = it->first;
                        end = begin + it->second;

                        for (offset = begin; offset < end; offset++)
                        {
                            uint64 rline;
                            uint64 roffset;

                            if (LineOffsetConversion (LC, line, offset, rline, roffset))
                            {
                                uint64 x;
                                uint64 y;

                                x = roffset - LC->FileContext->CurrentColumn;
                                y = (rline - LC->FileContext->CurrentLine) + 1;

                                if (x >= 0  &&  y >= 1  &&
                                    x <= LC->Screen->GetMaxConsX() && 
                                    y < LC->Screen->GetMaxConsY())
                                {
                                    LC->Screen->PokeFormat (x, y) = hfmt;
                                }
                            }
                        }
                    }
                }

                /*
                if (LC->FileContext->UserIsLineMarked(LineToDraw)  ||
                    (LC->ShowMarkZone  &&  LineToDraw == GetCenterLine (LC)))
                {
                    for (j = 0; j <= LC->Screen->GetMaxX(); j++)
                    {
                        cFormat &Char = LC->Screen->PokeFormat (j, ScreenLine);
                        Char.Underline = !Char.Underline;
                    }
                }
                */

                if (FreeText)
                    DeleteTextLine (Text);
            }
        }

        LC->Screen->Refresh();
        LC->Screen->SetAutoRefresh (OldAR);

        return;
    } unguard;
}


void HandleVScroll (ListThreadContext *LC)
{
    guard
    {
        if (LC->DoVScroll)
        {
            if (LC->DisplayHelp  ||  LC->DisplayInfo  ||  LC->ShowMarkZone)
            {   // Don't scroll stuff if we're displaying the help info.
                LC->UpdateText = true;
                LC->UpdateInfo = true;
                LC->DoVScroll = false;
                LC->VScrollAccumulate = 0;
            }
            else
            {
                if (abs(LC->VScrollAccumulate) > LC->Screen->GetMaxY() - 1)
                    LC->UpdateText = true; // Just do a full redraw.
                else
                if (LC->VScrollAccumulate != 0)
                {
                    LC->Screen->ScrollRegion (0, LC->VScrollAccumulate, 0, 1, LC->Screen->GetMaxX(), LC->Screen->GetMaxY() - 1);

                    if (LC->VScrollAccumulate < 0)
                    {   // scroll up
                        DrawTextBody (LC, LC->Screen->GetMaxY() - 1 - abs(LC->VScrollAccumulate), abs(LC->VScrollAccumulate));
                    }
                    else 
                    {   // scroll down
                        DrawTextBody (LC, 0, abs(LC->VScrollAccumulate));
                    }
                }

                UpdateScrollBars (LC);

                LC->DoVScroll = false;
                LC->VScrollAccumulate = 0;
                LC->UpdateText = false;
            }
        }

        return;
    } unguard;
}


void UpdateScrollBars (ListThreadContext *LC)
{
    guard
    {
        // Vertical scrollbar
        // But disable it if we have less then (LC->Screen->GetMaxY() - 1) lines.
        ShowScrollBar (LC->Screen->GetHWND(), SB_VERT, TRUE);
        if (LC->FileContext->ActiveLinesDB->GetTotalLines() < (LC->Screen->GetMaxY() - 1))
            EnableScrollBar (LC->Screen->GetHWND(), SB_VERT, ESB_DISABLE_BOTH);
        else
        {
            SCROLLINFO ScrollInfo;

            ScrollInfo.cbSize = sizeof (ScrollInfo);
            ScrollInfo.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
            ScrollInfo.nMax = LC->FileContext->ActiveLinesDB->GetTotalLines();
            ScrollInfo.nMin = 1;
            ScrollInfo.nPage = LC->Screen->GetMaxY() - 1;
            ScrollInfo.nPos = LC->FileContext->CurrentLine + 1;

            EnableScrollBar (LC->Screen->GetHWND(), SB_VERT, ESB_ENABLE_BOTH);
            SetScrollInfo (LC->Screen->GetHWND(), SB_VERT, &ScrollInfo, TRUE);
        }

        // Horizontal scrollbar
        ShowScrollBar (LC->Screen->GetHWND(), SB_HORZ, TRUE);
        if ((LC->FileContext->ActiveLinesDB->GetLongestLineWidth() - 1) <= LC->Screen->GetConsWidth())
            EnableScrollBar (LC->Screen->GetHWND(), SB_HORZ, ESB_DISABLE_BOTH);
        else
        {
            SCROLLINFO ScrollInfo;

            ScrollInfo.cbSize = sizeof (ScrollInfo);
            ScrollInfo.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
            ScrollInfo.nMax = LC->FileContext->ActiveLinesDB->GetLongestLineWidth() - 1;
            ScrollInfo.nMin = 1;
            ScrollInfo.nPage = LC->Screen->GetConsWidth();
            ScrollInfo.nPos = LC->FileContext->CurrentColumn + 1;

            EnableScrollBar (LC->Screen->GetHWND(), SB_HORZ, ESB_ENABLE_BOTH);
            SetScrollInfo (LC->Screen->GetHWND(), SB_HORZ, &ScrollInfo, TRUE);
        }

        return;
    } unguard;
}

string GetLineInput (ListThreadContext *LC, string Prompt)
{
    guard
    {
        int x1, y1, x2, y2;
        char Buffer[16384];

        LC->CmdQueue.Disable ();

        x1 = LC->Screen->GetWinX1 ();
        y1 = LC->Screen->GetWinY1 ();
        x2 = LC->Screen->GetWinX2 ();
        y2 = LC->Screen->GetWinY2 ();

        LC->Screen->SetWindow (0, LC->Screen->GetMaxConsY(), LC->Screen->GetMaxConsX(), LC->Screen->GetMaxConsY());
        LC->Screen->SetForeground (LC->SettingsV2.GetValUint32("InfoColor.Foreground"));
        LC->Screen->SetBackground (LC->SettingsV2.GetValUint32("InfoColor.Background"));
        LC->Screen->ClearWin ();

        // Set the window this way so that if they type a lot it will expand the typing box
        // to fill as much of the screen as necessary
        LC->Screen->SetWindow (0, 1, LC->Screen->GetMaxConsX(), LC->Screen->GetMaxConsY());

        LC->Screen->GoToXY (0, LC->Screen->GetMaxY());
        LC->Screen->WriteTextf ("%s", Prompt.c_str());
        LC->Screen->ReadText (Buffer, sizeof (Buffer) - 1, FALSE, TRUE);
        LC->Screen->SetWindow (x1, y1, x2, y2);

        LC->CmdQueue.Enable ();

        AddSimpleCommand (LC, LCMD_UPDATE_INFO);
        AddSimpleCommand (LC, LCMD_UPDATE_TEXT);
        AddCommand (LC, LCMD_SET_INFO_TEXT, 0, 0, 0, 0, NULL);

        return (string(Buffer));
    } unguard;
}

void AddSimpleCommand (ListThreadContext *LC, uint32 Command)
{
    guard
    {
        LC->CmdQueue.SendCommandAsync (CCommand (Command, 0, 0, 0, 0, NULL));
        return;
    } unguard;
}

void AddCommand (ListThreadContext *LC, uint32 Command, uint32 Parm1, uint32 Parm2, uint32 Parm3, uint32 Parm4, void *Ptr)
{
    guard
    {
        LC->CmdQueue.SendCommandAsync (CCommand (Command, Parm1, Parm2, Parm3, Parm4, Ptr));
        return;
    } unguard;
}

void AddSimpleCommandUrgent (ListThreadContext *LC, uint32 Command)
{
    guard
    {
        LC->CmdQueue.SendCommandUrgentAsync (CCommand (Command, 0, 0, 0, 0, NULL));
        return;
    } unguard;
}

void AddCommandUrgent (ListThreadContext *LC, uint32 Command, uint32 Parm1, uint32 Parm2, uint32 Parm3, uint32 Parm4, void *Ptr)
{
    guard
    {
        LC->CmdQueue.SendCommandUrgentAsync (CCommand (Command, Parm1, Parm2, Parm3, Parm4, Ptr));
        return;
    } unguard;
}

void ReplaceSimpleCommand (ListThreadContext *LC, uint32 Command)
{
    guard
    {
        LC->CmdQueue.ReplaceCommandAsync (Command, CCommand (Command, 0, 0, 0, 0, NULL));
        return;
    } unguard;
}

void ReplaceCommand (ListThreadContext *LC, uint32 Command, uint32 Parm1, uint32 Parm2, uint32 Parm3, uint32 Parm4, void *Ptr)
{
    guard
    {
        LC->CmdQueue.ReplaceCommandAsync (Command, CCommand (Command, Parm1, Parm2, Parm3, Parm4, Ptr));
        return;
    } unguard;
}


// A return value of false means that the hex line corresponds to a line
// in the "normal" lines DB that isn't available yet (or is invalid?)
bool HexLineToNormalLine (ListThreadContext *LC, uint64 HexLine, uint64 &NormalLineResult)
{
    guard
    {
        uint64 SeekPoint;
        uint64 Line;

        SeekPoint = LC->FileContext->HexLinesDB->LineToSeekPoint (HexLine);

        if (!LC->FileContext->NormalLinesDB->SeekPointToLine (SeekPoint, Line))
            return (false);

        NormalLineResult = Line;
        return (true);
    } unguard;
}


bool NormalLineToHexLine (ListThreadContext *LC, uint64 NormalLine, uint64 &HexLineResult)
{
    guard
    {
        uint64 SeekPoint;

        if (!LC->FileContext->NormalLinesDB->GetSeekPoint (NormalLine, SeekPoint))
            return (false);

        HexLineResult = LC->FileContext->HexLinesDB->SeekPointToLine (SeekPoint);
        return (true);
    } unguard;
}


void ToggleHexMode (ListThreadContext *LC)
{
    guard
    {
        uint64 CurrentLine;
        //uint64 HighlightLine;

        CurrentLine = LC->FileContext->CurrentLine;
        //HighlightLine = LC->FileContext->HighlightLine;

        LC->FileContext->HexMode = !LC->FileContext->HexMode;
        ResetLinesDBs (LC, false);

        if (LC->FileContext->HexMode)
        {   // We are switching into hex mode.
            if (!NormalLineToHexLine (LC, CurrentLine, CurrentLine))
                CurrentLine = 0;

            //if (!NormalLineToHexLine (LC, HighlightLine, HighlightLine))
                //DisableHighlight (LC);
        }
        else
        {   // We are switching into normal mode
            if (!HexLineToNormalLine (LC, CurrentLine, CurrentLine))
                CurrentLine = 0;

            //if (!HexLineToNormalLine (LC, HighlightLine, HighlightLine))
                //DisableHighlight (LC);
        }

        AddCommand (LC, LCMD_SET_LINE, LODWORD(CurrentLine), HIDWORD(CurrentLine), 0, 0, NULL);
        //LC->FileContext->HighlightLine = HighlightLine;

        LC->FileContext->CurrentColumn = 0;
        AddSimpleCommand (LC, LCMD_UPDATE_INFO);
        AddSimpleCommand (LC, LCMD_UPDATE_TEXT);

        return;
    } unguard;
}

bool HandleInput (ListThreadContext *LC)
{    
    guard
    {
    #define CONTROL(k) (k - 'A' + 1)
        CIKey Char;
        string TempString;
        bool AddCheckScanning = false;
        bool Return = false;

        if (!LC->Screen->KeyPressed())
            return (false);

        while (LC->Screen->KeyPressed())
        {
            Char = LC->Screen->GetChar ();
            switch (tolower(Char))
            {
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    AddCommand (LC, LCMD_SET_PARSER, Char - '1', 0, 0, 0, NULL);
                    break;

                case 1: // Ctrl+A = toggle always on top
                    AddCommand (LC, LCMD_SET_ALWAYS_ON_TOP, LC->AlwaysOnTop ? 0 : 1, 0, 0, 0, NULL);
                    break;

                //case 9: // Ctrl+I
                case CONTROL('I'):
                    AddSimpleCommand (LC, LCMD_TOGGLE_INFO_DISPLAY);
                    break;

                case 'g': // g = toggle search type
                    AddSimpleCommand (LC, LCMD_SEARCH_TOGGLE_TYPE);
                    break;

                //case 18: // ctrl+r = ruler
                    /*
                case CONTROL('R'):
                    AddSimpleCommand (LC, LCMD_TOGGLE_RULER);
                    break;
                    */

                case CONTROL('Y'):
                    AddSimpleCommand (LC, LCMD_TOGGLE_TRANSPARENCY);
                    break;

                //case 23: // ctrl+w = quit!
                case CONTROL('W'):
                    AddSimpleCommand (LC, LCMD_QUIT);
                    break;

                //case 15: // CTRL+O = new file
                case CONTROL('O'):
                    AddSimpleCommand (LC, LCMD_INPUT_OPEN);
                    break;

                //case 14: // CTRL+N = new file in new window
                case CONTROL('N'):
                    AddSimpleCommand (LC, LCMD_INPUT_OPEN_NEW);
                    break;

                case CONTROL('T'): // toggle tailing
                    AddSimpleCommand (LC, LCMD_TOGGLE_TAILING);
                    break;

                case CONTROL('E'): // edit file
                    AddSimpleCommand (LC, LCMD_EDIT);
                    break;

                case ' ': // space = page down
                    AddSimpleCommand (LC, LCMD_INPUT_PAGE_DOWN);
                    break;

                case '*':
                    AddSimpleCommand (LC, LCMD_TOGGLE_JUNK_FILTER);
                    break;

                // User-defined bookmarks
                /*
                case 'u':
                case 'j':
                case 'm':
                    {
                        uint64 Line;
                        
                        switch (tolower(Char))
                        {
                            case 'u':
                                Line = GetTopLine (LC);
                                break;

                            case 'j':
                                Line = GetBottomLine (LC);
                                break;

                            case 'm':
                                Line = GetCenterLine (LC);
                                break;
                        }

                        AddCommand (LC, LCMD_TOGGLE_USER_BOOKMARK, LODWORD(Line), HIDWORD(Line), 0, 0, NULL);
                        AddSimpleCommand (LC, LCMD_UPDATE_TEXT);
                    }
                    break;

                case '/':
                    AddSimpleCommand (LC, LCMD_TOGGLE_SHOW_MARK_ZONE);
                    break;
                */

                case 'e': // edit search string
                    int k;

                    if (LC->FileContext->DoingSearch) // don't let them edit the search while we're in the middle of searching
                        break;

                    AddCommand (LC, LCMD_SET_INFO_TEXT, 0, 0, 0, 0, NULL);

                    for (k = 0; k < LC->FileContext->SearchSubstring.length(); k++)
                        LC->Screen->AddKeyToQ (LC->FileContext->SearchSubstring[k]);

                    // Home key
                    LC->Screen->AddKeyToQ (27);
                    LC->Screen->AddKeyToQ ('[');
                    LC->Screen->AddKeyToQ ('H');

                    TempString = GetLineInput (LC, "Enter search string: ");

                    if (TempString.length() > 0)
                    {
                        LC->FileContext->HaveDoneSearchBefore = true; // so we can hit 'n' or 'N' and have it work
                        SetSearchPattern (LC, TempString.c_str());
                    }

                    AddSimpleCommand (LC, LCMD_UPDATE_INFO);
                    break;

                case 'h': // toggle help
                    AddSimpleCommand (LC, LCMD_TOGGLE_HEX_DISPLAY);
                    AddSimpleCommand (LC, LCMD_UPDATE_TEXT);
                    AddSimpleCommand (LC, LCMD_UPDATE_INFO);
                    break;

                case 6: // search using settings
                    if (LC->FileContext->DoingSearch)
                        break;

                    AddCommand (LC, LCMD_SET_INFO_TEXT, 0, 0, 0, 0, NULL);

                    TempString = GetLineInput (LC, "Search for: ");
                    if (TempString.length() > 0)
                    {
                        LC->FileContext->DoingSearch = true;
                        SetSearchPattern (LC, TempString.c_str());
                        // The search will now be performed in the main while() loop of the ListThread function
                    }

                    AddSimpleCommand (LC, LCMD_UPDATE_INFO);
                    break;

                case '|': // case-sensitive search (throwback to old school List)
                    AddCommand (LC, LCMD_SET_INFO_TEXT, 0, 0, 0, 0, NULL);

                    TempString = GetLineInput (LC, "Search for: ");
                    if (TempString.length() > 0)
                    {
                        LC->FileContext->DoingSearch = true;
                        LC->FileContext->SearchCaseSensitive = true;
                        SetSearchPattern (LC, TempString.c_str());
                        // The search will now be performed in the main while() loop of the ListThread function
                    }

                    AddSimpleCommand (LC, LCMD_UPDATE_INFO);
                    break;

                    /*
                case ']' : // case-sensitive filter
                    AddCommand (LC, LCMD_SET_INFO_TEXT, 0, 0, 0, 0, NULL);

                    TempString = GetLineInput (LC, "Filter for: ");

                    if (TempString.length() > 0)
                    {
                        LC->FileContext->DoingSearch = true;
                        LC->FileContext->SearchCaseSensitive = false;
                        SetSearchPattern (LC, TempString.c_str());
                        LC->FileContext->DoFilter = true;
                        LC->FileContext->FilterStream = new wofstream ("c:/temp/lxpipe.tmp");
                        OpenInNewWindow (LC->Screen->GetHWND(), "c:\\temp\\lxpipe.tmp?tail=on?_TailingJumpToEnd=0");
                    }

                    AddSimpleCommand (LC, LCMD_UPDATE_INFO);
                    break;
                    */

                case '\\': // case-insensitive search (throwback to old school List)
                    AddCommand (LC, LCMD_SET_INFO_TEXT, 0, 0, 0, 0, NULL);

                    TempString = GetLineInput (LC, "Search for: ");
                    if (TempString.length() > 0)
                    {
                        LC->FileContext->DoingSearch = true;
                        LC->FileContext->SearchCaseSensitive = false;
                        SetSearchPattern (LC, TempString.c_str());
                        // The search will now be performed in the main while() loop of the ListThread function
                    }

                    AddSimpleCommand (LC, LCMD_UPDATE_INFO);
                    break;

                case 'r': // reset search to beginning of file
                    AddSimpleCommand (LC, LCMD_SEARCH_RESET);
                    break;

                case 'c': // toggle case-sensitive search
                    AddSimpleCommand (LC, LCMD_SEARCH_TOGGLE_MATCH_CASE);
                    break;

                    // tag: make this a LCMD_*
                case 't': // reset next search to current line
                    if (!LC->FileContext->DoingSearch)
                    {
                        if (LC->FileContext->HexMode)
                        {
                            if (!HexLineToNormalLine (LC, LC->FileContext->CurrentLine, LC->FileContext->NextLineToSearch))
                                LC->FileContext->NextLineToSearch = 0;
                        }
                        else
                        {
                            LC->FileContext->NextLineToSearch = LC->FileContext->CurrentLine;
                        }
                    }

                    AddCommand (LC, LCMD_SET_INFO_TEXT, 0, 0, 0, 0, NULL);
                    AddSimpleCommand (LC, LCMD_UPDATE_INFO);
                    break;

                case 'd': // toggle search direction
                    AddSimpleCommand (LC, LCMD_SEARCH_TOGGLE_DIRECTION);
                    break;

                case 'k':
                    AddSimpleCommand (LC, LCMD_SETTINGS_LOAD);
                    break;

                case 'l':
                    AddCommand (LC, LCMD_SEARCH_SET_TYPE, SEARCHTYPE_LITERAL, 0, 0, 0, NULL);
                    break;

                case 'b':
                    AddCommand (LC, LCMD_SEARCH_SET_TYPE, SEARCHTYPE_BOOLEAN, 0, 0, 0, NULL);
                    break;

                case 'x':
                    AddCommand (LC, LCMD_SEARCH_SET_TYPE, SEARCHTYPE_REGEX, 0, 0, 0, NULL);
                    break;

                case 'q':
                    LC->FileContext->NormalLinesDB->ClearCache ();
                    AddSimpleCommand (LC, LCMD_UPDATE_TEXT);
                    break;

                case '+':
                    {
                        string InputText;
                        uint64 OldLine;
                        uint64 NewLine;
                        sint64 LineDelta;

                        InputText = GetLineInput (LC, "Enter # of lines to skip forward: ");
                        LineDelta = _atoi64 (InputText.c_str());

                        OldLine = LC->FileContext->CurrentLine;
                        NewLine = (unsigned)((signed)OldLine + LineDelta);

                        AddCommand (LC, LCMD_SET_LINE, LODWORD(NewLine), HIDWORD(NewLine), 0, 0, NULL);
                    }

                    AddCommand (LC, LCMD_SET_INFO_TEXT, 0, 0, 0, 0, NULL);
                    break;

                case '-':
                    {
                        string InputText;
                        uint64 OldLine;
                        uint64 NewLine;
                        sint64 LineDelta;

                        InputText = GetLineInput (LC, "Enter # of lines to skip forward: ");
                        LineDelta = _atoi64 (InputText.c_str());

                        OldLine = LC->FileContext->CurrentLine;
                        NewLine = (unsigned)((signed)OldLine - LineDelta);

                        AddCommand (LC, LCMD_SET_LINE, LODWORD(NewLine), HIDWORD(NewLine), 0, 0, NULL);
                    }

                    AddCommand (LC, LCMD_SET_INFO_TEXT, 0, 0, 0, 0, NULL);
                    break;

                case CONTROL('G'):
                case '#': // input line # to jump to
                    {
                        string InputText;
                        uint64 LineNumber;

                        InputText = GetLineInput (LC, "Enter line number to jump to: ");
                        LineNumber = _atoi64 (InputText.c_str());

                        if (LineNumber == 0)
                            LineNumber = 1;

                        if ((LineNumber - 1) > LC->FileContext->ActiveLinesDB->GetTotalLines())
                            LineNumber = LC->FileContext->ActiveLinesDB->GetTotalLines() - 1;

                        LineNumber--; // cardinal --> ordinal conversion
                        AddCommand (LC, LCMD_SET_LINE, LODWORD(LineNumber), HIDWORD(LineNumber), 0, 0, NULL);
                    }

                    AddCommand (LC, LCMD_SET_INFO_TEXT, 0, 0, 0, 0, NULL);
                    break;

                case 's': // perform search using parameters
                    AddSimpleCommand (LC, LCMD_SEARCH_FIND_NEXT);
                    break;

                case 'n': // find next occurence of text. throwback to old school List
                        // note that this key does not heed the LC->SearchDirection setting
                        // and, in fact, resets it
                        // n = down, shift+n = up
                    if (!LC->FileContext->HaveDoneSearchBefore)
                        break;

                    // Also, if we are switching directions, then we ought to adjust what the 'next'
                    // line to search is.
                    if (Char == 'N') // if Shift+N, then search up instead of down
                        AddCommand (LC, LCMD_SEARCH_SET_DIRECTION, 0, 0, 0, 0, NULL);
                    else
                        AddCommand (LC, LCMD_SEARCH_SET_DIRECTION, 1, 0, 0, 0, NULL);

                    AddSimpleCommand (LC, LCMD_SEARCH_FIND_NEXT);
                    break;

                case CONTROL('Z'): // Reset/reread file
                    AddSimpleCommand (LC, LCMD_FILE_RESET);
                    break;

                case 'w': // toggle line wrapping
                    AddSimpleCommand (LC, LCMD_TOGGLE_LINE_WRAPPING);
                    break;

                case 'a': // toggle animated search
                    AddSimpleCommand (LC, LCMD_SEARCH_TOGGLE_ANIMATED);
                    break;

                case 27: // escape sequences
                    Char = LC->Screen->GetChar ();
                    switch (Char)
                    {
                        case 27:
                            if (LC->DisplayHelp > 0  ||  LC->DisplayInfo)
                            {
                                if (LC->DisplayHelp > 0)
                                    AddSimpleCommand (LC, LCMD_HELP_DISPLAY_TOGGLE);

                                if (LC->DisplayInfo)
                                    AddSimpleCommand (LC, LCMD_TOGGLE_INFO_DISPLAY);
                            }
                            else
                            if (LC->FileContext->DoingSearch == true)
                            {
                                AddSimpleCommand (LC, LCMD_SEARCH_CANCEL);
                            }
                            else
                            {   // If they press ESC twice quick enough, then quit
                                if (GetTickCount() - LC->EscTimestamp < ESCTIMEOUT)
                                    AddSimpleCommand (LC, LCMD_QUIT);
                                else
                                {
                                    LC->EscTimestamp = GetTickCount();
                                    AddSimpleCommand (LC, LCMD_UPDATE_INFO);
                                }
                            }

                            AddCommand (LC, LCMD_SET_INFO_TEXT, 0, 0, 0, 0, NULL);
                            break;

                        case '[':
                            Char = LC->Screen->GetChar ();
                            switch (Char)
                            {
                                case 'A': // up
                                    AddSimpleCommand (LC, LCMD_INPUT_LINE_UP);
                                    AddCommand (LC, LCMD_SET_INFO_TEXT, 0, 0, 0, 0, NULL);
                                    break;

                                case 'B': // down
                                    AddSimpleCommand (LC, LCMD_INPUT_LINE_DOWN);
                                    AddCommand (LC, LCMD_SET_INFO_TEXT, 0, 0, 0, 0, NULL);
                                    break;

                                case 'D': // left
                                    AddSimpleCommand (LC, LCMD_INPUT_COLUMN_LEFT);
                                    AddCommand (LC, LCMD_SET_INFO_TEXT, 0, 0, 0, 0, NULL);
                                    break;

                                case 'C': // right
                                    AddSimpleCommand (LC, LCMD_INPUT_COLUMN_RIGHT);
                                    AddCommand (LC, LCMD_SET_INFO_TEXT, 0, 0, 0, 0, NULL);
                                    break;

                                case 'G': // page down
                                    AddSimpleCommand (LC, LCMD_INPUT_PAGE_DOWN);
                                    AddCommand (LC, LCMD_SET_INFO_TEXT, 0, 0, 0, 0, NULL);
                                    break;

                                case 'I': // page up
                                    AddSimpleCommand (LC, LCMD_INPUT_PAGE_UP);
                                    AddCommand (LC, LCMD_SET_INFO_TEXT, 0, 0, 0, 0, NULL);
                                    break;

                                case 'H': // home
                                    AddSimpleCommand (LC, LCMD_INPUT_GO_HOME);
                                    AddCommand (LC, LCMD_SET_INFO_TEXT, 0, 0, 0, 0, NULL);
                                    break;

                                case 'F': // end
                                    AddSimpleCommand (LC, LCMD_INPUT_GO_END);
                                    AddCommand (LC, LCMD_SET_INFO_TEXT, 0, 0, 0, 0, NULL);
                                    break;

                                case 'M': // F1
                                    AddSimpleCommand (LC, LCMD_HELP_DISPLAY_NEXT_PAGE);
                                    break;

                                case 'O': // F3
                                    AddSimpleCommand (LC, LCMD_SEARCH_FIND_NEXT);
                                    break;
                            }

                            break;
                    }
            }
        }

        Return = LC->Screen->KeyPressed();

        if (AddCheckScanning)
            AddSimpleCommand (LC, LCMD_CHECK_SCANNING);

        return (Return);
#undef CONTROL
    } unguard;
}


void HandleCommands (ListThreadContext *LC)
{
    guard
    {
        CCommand Command;
        bool AddCheckScanning = false;
        bool Result;

        while (LC->CmdQueue.ReceiveCommandAsync (&Command))
        {
            if (Command.Message == LCMD_LOOP_BREAK)
                break;

            switch (Command.Message)
            {
                case LCMD_QUIT:
                    LC->WantToQuit = true;
                    break;

                case LCMD_SETTINGS_LOAD:
                    //LoadSettingsV2 (LC);
                    //ApplySettingsV2 (LC);
                    break;

                case LCMD_SETTINGS_SAVE:
                    SaveSettingsV2 (LC);
                    break;

                case LCMD_FILE_RESET:
                    LC->FileContext->CurrentLine = 0;

                    if (LC->FileContext->WrapText)
                        LC->FileContext->MaxWidth = LC->Screen->GetMaxX() + 1;
                    else
                        LC->FileContext->MaxWidth = CLINESDB_MAXWIDTH;

                    ResetLinesDBs (LC, true, true);
                    LC->FileContext->File->Seek (0);

                    AddSimpleCommand (LC, LCMD_UPDATE_TEXT);
                    AddSimpleCommand (LC, LCMD_UPDATE_INFO);
                    LC->FileContext->NextLineToSearch = 0;
                    AddCommand (LC, LCMD_SET_INFO_TEXT, 0, 0, 0, 0, NULL);
                    break;

                case LCMD_SET_TAILING:
                    if (Command.Parm1 == 0)
                        LC->FileContext->Tailing = false;
                    else
                    {
                        if (LC->SettingsV2.GetValUint32("DisableTailWarning") == 0)
                        {
                            bool Result;

                            NoticeBox (LC->Screen->GetHWND(),
                                       LC->SettingsV2.GetValString ("_TailingNotice").c_str(),
                                       "Tailing",
                                       bool(LC->SettingsV2.GetValUint32("DisableTailWarning")),
                                       &Result);

                            LC->SettingsV2.SetValUint32 ("DisableTailWarning", Result ? 1 : 0);
                            AddSimpleCommand (LC, LCMD_SETTINGS_SAVE);
                        }

                        LC->FileContext->Tailing = true;
                    }

                    AddSimpleCommand (LC, LCMD_UPDATE_INFO);
                    break;

                case LCMD_TOGGLE_TAILING:
                    AddCommand (LC, LCMD_SET_TAILING, LC->FileContext->Tailing ? 0 : 1, 0, 0, 0, NULL);
                    break;

                case LCMD_TOGGLE_TRANSPARENCY:
                    LC->SettingsV2.SetValUint32 ("Transparent", (LC->SettingsV2.GetValUint32 ("Transparent") == 1) ? 0 : 1);
                    AddSimpleCommand (LC, LCMD_APPLY_TRANSPARENCY);
                    break;

                case LCMD_SET_TRANSPARENCY:
                    LC->SettingsV2.SetValUint32 ("Transparent", (Command.Parm1 == 0) ? 0 : 1);
                    AddSimpleCommand (LC, LCMD_APPLY_TRANSPARENCY);
                    break;

                case LCMD_SET_ALPHA_LEVEL:
                    LC->SettingsV2.SetValUint32 ("AlphaLevel", Command.Parm1);
                    AddSimpleCommand (LC, LCMD_APPLY_TRANSPARENCY);
                    break;

                case LCMD_APPLY_TRANSPARENCY:
                    SetWindowAlphaValues (LC->Screen->GetHWND(), 
                        LC->SettingsV2.GetValUint32 ("Transparent"), 
                        LC->SettingsV2.GetValUint32 ("AlphaLevel"));

                    break;


                case LCMD_TOGGLE_RULER:
                    LC->DisplayRuler = !LC->DisplayRuler;
                    AddSimpleCommand (LC, LCMD_UPDATE_INFO);
                    break;

                case LCMD_TOGGLE_USER_BOOKMARK:
                    LC->FileContext->UserToggleMark ((uint64)Command.Parm1 + ((uint64)Command.Parm2 << 32));
                    AddSimpleCommand (LC, LCMD_UPDATE_TEXT);
                    break;

                case LCMD_TOGGLE_SHOW_MARK_ZONE:
                    LC->ShowMarkZone = !LC->ShowMarkZone;
                    AddSimpleCommand (LC, LCMD_UPDATE_TEXT);
                    break;

                case LCMD_SET_LINE:
                    LC->FileContext->CurrentLine = (uint64)Command.Parm1 + ((uint64)Command.Parm2 << 32);
                    AddSimpleCommand (LC, LCMD_UPDATE_INFO);
                    AddSimpleCommand (LC, LCMD_UPDATE_TEXT);
                    break;

                case LCMD_SET_COLUMN:
                    LC->FileContext->CurrentColumn = Command.Parm1;
                    AddSimpleCommand (LC, LCMD_UPDATE_INFO);
                    AddSimpleCommand (LC, LCMD_UPDATE_TEXT);
                    break;

                case LCMD_SET_PARSER:
                    if (Command.Parm1 != LC->FileContext->ParserIndice) 
                    {
                        if (Command.Parm1 >= GetParserCount())
                            break;

                        LC->FileContext->ParserIndice = Command.Parm1;
                        ResetLinesDBs (LC, true);
                        AddSimpleCommand (LC, LCMD_FILE_RESET);
                    }

                    SetListTitle (LC);
                    AddSimpleCommand (LC, LCMD_UPDATE_TEXT);
                    AddSimpleCommand (LC, LCMD_UPDATE_INFO);
                    break;

                case LCMD_CHECK_SCANNING: // Secret code for "check to see if done scanning, if so go to end of file"
                    if (LC->FileContext->ActiveLinesDB->IsDoneScanning() == false)
                        AddCheckScanning = true;
                    else
                        AddSimpleCommand (LC, LCMD_INPUT_GO_END);

                    break;

                case LCMD_SEARCH_RESET:
                    if (!LC->FileContext->DoingSearch)
                        LC->FileContext->NextLineToSearch = 0;

                    DisableHighlight (LC);
                    AddSimpleCommand (LC, LCMD_UPDATE_INFO);
                    AddSimpleCommand (LC, LCMD_UPDATE_TEXT);
                    AddCommand (LC, LCMD_SET_INFO_TEXT, 0, 0, 0, 0, NULL);
                    break;

                case LCMD_SEARCH_TOGGLE_MATCH_CASE:
                    AddCommand (LC, LCMD_SEARCH_SET_MATCH_CASE, LC->FileContext->SearchCaseSensitive ? 0 : 1, 0, 0, 0, NULL);
                    break;

                case LCMD_SEARCH_SET_MATCH_CASE:
                    if (!LC->FileContext->DoingSearch)
                        LC->FileContext->SearchCaseSensitive = bool(Command.Parm1);

                    AddSimpleCommand (LC, LCMD_UPDATE_INFO);
                    AddCommand (LC, LCMD_SET_INFO_TEXT, 0, 0, 0, 0, NULL);
                    break;

                case LCMD_SEARCH_TOGGLE_DIRECTION:
                    if (LC->FileContext->SearchDirection == -1)
                        AddCommandUrgent (LC, LCMD_SEARCH_SET_DIRECTION, 1, 0, 0, 0, NULL);
                    else
                        AddCommandUrgent (LC, LCMD_SEARCH_SET_DIRECTION, 0, 0, 0, 0, NULL);

                    AddCommand (LC, LCMD_SET_INFO_TEXT, 0, 0, 0, 0, NULL);
                    break;

                case LCMD_SEARCH_SET_DIRECTION: // 0 = up, 1 = down
                    if (!LC->FileContext->DoingSearch)
                    {
                        int OldDirection;
                        int NewDirection;
                        bool Toggling;

                        OldDirection = LC->FileContext->SearchDirection;
                        NewDirection = signed(Command.Parm1 * 2) - 1; // 1 -> 1, 0 -> -1, which is what we want

                        // If we are toggling the search direction we need to adjust where we do our next search
                        if (OldDirection != NewDirection)
                        {
                            if (NewDirection == -1)
                            {
                            // Switching from foward- to reverse-searching
                                if (LC->FileContext->NextLineToSearch <= 1) // i.e. subtracting would cause underflow
                                    LC->FileContext->NextLineToSearch = 0; //LC->FileContext->NormalLinesDB->GetTotalLines(); // say "ok searched past end of file"
                                else
                                    LC->FileContext->NextLineToSearch -= 2;
                            }
                            else
                            // Switching from reverse- to forward-searching
                            {
                                LC->FileContext->NextLineToSearch = min (LC->FileContext->NextLineToSearch + 2,
                                                                         LC->FileContext->NormalLinesDB->GetTotalLines());
                            }
                        }

                        LC->FileContext->SearchDirection = NewDirection;
                        AddSimpleCommand (LC, LCMD_UPDATE_INFO);
                    }

                    AddCommand (LC, LCMD_SET_INFO_TEXT, 0, 0, 0, 0, NULL);
                    break;

                case LCMD_SEARCH_FIND_NEXT:
                    if (LC->FileContext->SearchSubstring.length() == 0)
                        break; // no search string? no search!

                    // Make sure they haven't reached the end of the file
                    if (LC->FileContext->NextLineToSearch == LC->FileContext->NormalLinesDB->GetTotalLines())
                        AddCommand (LC, LCMD_SET_INFO_TEXT, 0, 0, 0, 0, "Searched past end of file (press R to reset)");
                    else
                        LC->FileContext->DoingSearch = true;

                    /*
                    LC->FileContext->DoingSearch = true;

                    if ((LC->FileContext->SearchDirection == +1  &&  LC->FileContext->NextLineToSearch < LC->FileContext->NormalLinesDB->GetTotalLines()) ||
                        (LC->FileContext->SearchDirection == -1  &&  LC->FileContext->NextLineToSearch > 0))
                    {
                        LC->FileContext->NextLineToSearch += LC->FileContext->SearchDirection;
                    }
                    */

                    AddCommand (LC, LCMD_SET_INFO_TEXT, 0, 0, 0, 0, NULL);
                    break;

                case LCMD_SEARCH_TOGGLE_ANIMATED:
                    AddCommand (LC, LCMD_SEARCH_SET_ANIMATED, 
                        LC->SettingsV2.GetValUint32("AnimatedSearch") ? 0 : 1, 0, 0, 0, NULL);

                    AddSimpleCommand (LC, LCMD_UPDATE_INFO);
                    AddSimpleCommand (LC, LCMD_SETTINGS_SAVE);
                    AddCommand (LC, LCMD_SET_INFO_TEXT, 0, 0, 0, 0, NULL);
                    break;

                case LCMD_SEARCH_SET_ANIMATED:
                    LC->SettingsV2.SetValUint32 ("AnimatedSearch", Command.Parm1 ? 1 : 0);
                    AddSimpleCommand (LC, LCMD_UPDATE_INFO);
                    AddSimpleCommand (LC, LCMD_SETTINGS_SAVE);
                    AddCommand (LC, LCMD_SET_INFO_TEXT, 0, 0, 0, 0, NULL);
                    break;

                case LCMD_SEARCH_CANCEL:
                    AddSimpleCommand (LC, LCMD_UPDATE_INFO);
                    LC->FileContext->DoingSearch = false;
                    AddCommand (LC, LCMD_SET_INFO_TEXT, 0, 0, 0, 0, NULL);
                    break;

                case LCMD_SEARCH_TOGGLE_TYPE:
                    if (LC->FileContext->DoingSearch)
                        break; // Don't let them change the search type while we're searching

                    switch (LC->FileContext->SearchType)
                    {
                        case SEARCHTYPE_LITERAL:
                            AddCommand (LC, LCMD_SEARCH_SET_TYPE, SEARCHTYPE_BOOLEAN, 0, 0, 0, NULL);
                            break;

                        case SEARCHTYPE_BOOLEAN:
                            AddCommand (LC, LCMD_SEARCH_SET_TYPE, SEARCHTYPE_REGEX, 0, 0, 0, NULL);
                            break;

                        case SEARCHTYPE_REGEX:
                            AddCommand (LC, LCMD_SEARCH_SET_TYPE, SEARCHTYPE_LITERAL, 0, 0, 0, NULL);
                            break;
                    }
                    break;

                case LCMD_SEARCH_SET_TYPE:
                    if (LC->FileContext->DoingSearch)
                        break; // Don't let them change the search type while we're searching

                    LC->FileContext->SearchType = Command.Parm1;
                    SetSearchPattern (LC, LC->FileContext->SearchSubstring.c_str());
                    AddSimpleCommand (LC, LCMD_UPDATE_INFO);
                    AddCommand (LC, LCMD_SET_INFO_TEXT, 0, 0, 0, 0, NULL);
                    break; 

                case LCMD_INPUT_OPEN:
                    Result = ChooseFile (LC->Screen->GetHWND(), NewFileName);
                    if (Result)
                    {
                        UseNewFileName = true;
                        AddSimpleCommand (LC, LCMD_QUIT);
                    }

                    AddCommand (LC, LCMD_SET_INFO_TEXT, 0, 0, 0, 0, NULL);
                    break;

                case LCMD_INPUT_OPEN_NEW:
                    {
                        bool Result;
                        string FileName;

                        // tag
                        Result = ChooseFile (LC->Screen->GetHWND(), FileName, "Open in New Window");

                        if (Result)
                        {
                            SaveSettingsV2 (LC);
                            OpenInNewWindow (LC->Screen->GetHWND(), FileName);
                        }
                    }

                    AddCommand (LC, LCMD_SET_INFO_TEXT, 0, 0, 0, 0, NULL);
                    break;

                case LCMD_OPEN_FILE:
                    // tag
                    // I really don't like this implementation :(
                    NewFileName = string((char *)Command.Ptr);
                    free (Command.Ptr);
                    UseNewFileName = true;
                    AddSimpleCommand (LC, LCMD_QUIT);

                    AddCommand (LC, LCMD_SET_INFO_TEXT, 0, 0, 0, 0, NULL);
                    break;

                case LCMD_TOGGLE_LINE_WRAPPING:
                    AddCommand (LC, LCMD_SET_LINE_WRAPPING, LC->FileContext->WrapText ? 0 : 1, 0, 0, 0, NULL);
                    break;

                case LCMD_SET_LINE_WRAPPING:
                    LC->FileContext->WrapText = bool(Command.Parm1);
                    LC->SettingsV2.SetValUint32 ("WrapText", LC->FileContext->WrapText ? 1 : 0);
                    AddSimpleCommand (LC, LCMD_SETTINGS_SAVE);
                    LC->FileContext->CurrentColumn = 0;
                    AddCommand (LC, LCMD_SET_INFO_TEXT, 0, 0, 0, 0, NULL);
                    ResetLinesDBs (LC, false);
                    break;

                case LCMD_SET_TABSIZE:
                    if (Command.Parm1 != LC->FileContext->NormalLinesDB->GetTabSize())
                    {
                        LC->SettingsV2.SetValUint32 ("TabSize", Command.Parm1);
                        AddSimpleCommand (LC, LCMD_SETTINGS_SAVE);
                        AddCommand (LC, LCMD_SET_INFO_TEXT, 0, 0, 0, 0, NULL);
                        ResetLinesDBs (LC, true, false);
                    }
                    break;
                
                case LCMD_TOGGLE_INFO_DISPLAY:
                    LC->DisplayInfo = !LC->DisplayInfo;
                    AddSimpleCommand (LC, LCMD_UPDATE_TEXT);
                    break;

                case LCMD_TOGGLE_HEX_DISPLAY:
                    ToggleHexMode (LC);
                    AddSimpleCommand (LC, LCMD_UPDATE_TEXT);
                    break;

                case LCMD_SET_HEX_WORDSIZELOG2:
                    LC->SettingsV2.SetValUint32 ("HexWordSizeLog2", Command.Parm1);
                    AddSimpleCommand (LC, LCMD_SETTINGS_SAVE);
                    ResetLinesDBs (LC, false);
                    AddSimpleCommand (LC, LCMD_UPDATE_TEXT);
                    break;

                case LCMD_HELP_DISPLAY_TOGGLE:
                    if (LC->DisplayHelp > 0)
                        LC->DisplayHelp = 0;
                    else
                        LC->DisplayHelp = 1;
                      
                    AddSimpleCommand (LC, LCMD_UPDATE_TEXT);
                    break;

                case LCMD_HELP_DISPLAY_NEXT_PAGE:
                    LC->DisplayHelp = (1 + LC->DisplayHelp) % (1 + ListHelpPageCount);
                    AddSimpleCommand (LC, LCMD_UPDATE_TEXT);
                    break;

                case LCMD_TOGGLE_JUNK_FILTER:
                    LC->JunkFilter = !LC->JunkFilter;
                    AddSimpleCommand (LC, LCMD_UPDATE_TEXT);
                    AddSimpleCommand (LC, LCMD_UPDATE_INFO);
                    break;

                case LCMD_INPUT_LINE_UP:
                    if (LC->FileContext->CurrentLine != 0)
                    {
                        LC->FileContext->CurrentLine--;

                        // Use hardware scrolling!
                        LC->DoVScroll = true;
                        LC->VScrollAccumulate++;
                    }

                    // We don't send a LCMD_UPDATE_TEXT because the status of LC->DoVScroll will handle that
                    AddSimpleCommand (LC, LCMD_UPDATE_INFO);
                    LC->WaitForScan = false;
                    AddCheckScanning = false;
                    break;

                case LCMD_INPUT_LINE_DOWN:
                    if (LC->FileContext->CurrentLine + 1 < LC->FileContext->ActiveLinesDB->GetTotalLines())
                    {
                        LC->FileContext->CurrentLine++;

                        // Use hardware scrolling!
                        LC->DoVScroll = true;
                        LC->VScrollAccumulate--;
                    }

                    // We don't send a LCMD_UPDATE_TEXT because the status of LC->DoVScroll will handle that
                    AddSimpleCommand (LC, LCMD_UPDATE_INFO);
                    LC->WaitForScan = false;
                    AddCheckScanning = false;
                    break;

                case LCMD_INPUT_PAGE_LEFT:
                    if (LC->FileContext->CurrentColumn > LC->Screen->GetConsWidth())
                        LC->FileContext->CurrentColumn -= LC->Screen->GetConsWidth();
                    else
                        LC->FileContext->CurrentColumn = 0;

                    LC->WaitForScan = false;
                    AddSimpleCommand (LC, LCMD_UPDATE_TEXT);
                    AddSimpleCommand (LC, LCMD_UPDATE_INFO);
                    AddCheckScanning = false;
                    break;

                case LCMD_INPUT_PAGE_RIGHT:
                    if (LC->FileContext->HexMode)
                        LC->FileContext->CurrentColumn = 0;
                    else
                    if (LC->FileContext->CurrentColumn + LC->Screen->GetConsWidth() < LC->FileContext->ActiveLinesDB->GetLongestLineWidth() - LC->Screen->GetConsWidth() - 1)
                        LC->FileContext->CurrentColumn += LC->Screen->GetConsWidth();
                    else
                        LC->FileContext->CurrentColumn = LC->FileContext->ActiveLinesDB->GetLongestLineWidth() - LC->Screen->GetConsWidth() - 1;

                    AddSimpleCommand (LC, LCMD_UPDATE_TEXT);
                    AddSimpleCommand (LC, LCMD_UPDATE_INFO);
                    LC->WaitForScan = false;
                    AddCheckScanning = false;
                    break;

                case LCMD_INPUT_COLUMN_LEFT:
                    if (LC->FileContext->WrapText)
                        break;

                    if (LC->FileContext->CurrentColumn > 0)
                        LC->FileContext->CurrentColumn -= 1;

                    LC->WaitForScan = false;
                    AddSimpleCommand (LC, LCMD_UPDATE_TEXT);
                    AddSimpleCommand (LC, LCMD_UPDATE_INFO);
                    AddCheckScanning = false;
                    break;

                case LCMD_INPUT_COLUMN_RIGHT:
                    if (LC->FileContext->WrapText  ||  LC->FileContext->HexMode)
                        break;

                    if (LC->FileContext->CurrentColumn + 1 < LC->FileContext->ActiveLinesDB->GetLongestLineWidth() - LC->Screen->GetConsWidth())
                        LC->FileContext->CurrentColumn++;

                    AddSimpleCommand (LC, LCMD_UPDATE_TEXT);
                    AddSimpleCommand (LC, LCMD_UPDATE_INFO);
                    LC->WaitForScan = false;
                    AddCheckScanning = false;
                    break;

                case LCMD_INPUT_PAGE_UP:
                    if (LC->FileContext->CurrentLine > (LC->Screen->GetMaxY() - 1))
                        LC->FileContext->CurrentLine -= LC->Screen->GetMaxY() - 1;
                    else
                        LC->FileContext->CurrentLine = 0;

                    LC->WaitForScan = false;
                    AddSimpleCommand (LC, LCMD_UPDATE_TEXT);
                    AddSimpleCommand (LC, LCMD_UPDATE_INFO);
                    AddCheckScanning = false;
                    break;

                case LCMD_INPUT_PAGE_DOWN:
                    LC->FileContext->CurrentLine += LC->Screen->GetMaxY() - 1;
                    LC->WaitForScan = false;
                    AddSimpleCommand (LC, LCMD_UPDATE_TEXT);
                    AddSimpleCommand (LC, LCMD_UPDATE_INFO);
                    AddCheckScanning = false;
                    break;

                case LCMD_INPUT_GO_HOME:
                    LC->FileContext->CurrentLine = 0;
                    LC->FileContext->CurrentColumn = 0;
                    LC->WaitForScan = false;
                    AddSimpleCommand (LC, LCMD_UPDATE_TEXT);
                    AddSimpleCommand (LC, LCMD_UPDATE_INFO);
                    AddCheckScanning = false;
                    break;

                case LCMD_INPUT_GO_END:
                    if (!LC->FileContext->ActiveLinesDB->IsDoneScanning())
                    {
                        LC->WaitForScan = true;
                        AddSimpleCommand (LC, LCMD_UPDATE_TEXT);
                        AddSimpleCommand (LC, LCMD_UPDATE_INFO);
                        AddCheckScanning = true;
                    }
                    else
                    {
                        if (LC->FileContext->ActiveLinesDB->GetTotalLines() > LC->Screen->GetMaxY() - 1)
                            LC->FileContext->CurrentLine = LC->FileContext->ActiveLinesDB->GetTotalLines() - 
                                (LC->Screen->GetMaxY() - 1); 
                
                        AddSimpleCommand (LC, LCMD_UPDATE_TEXT);
                        AddSimpleCommand (LC, LCMD_UPDATE_INFO);
                    }
                    break;

                case LCMD_SET_INFO_TEXT:
                    if (Command.Ptr == NULL)
                        LC->InfoText = "";
                    else
                    {
                        LC->InfoText = string ((char *)Command.Ptr);
                        free (Command.Ptr);
                    }
                    break;

                case LCMD_UPDATE_INFO:
                    LC->UpdateInfo = true;
                    break;

                case LCMD_UPDATE_TEXT:
                    LC->UpdateText = true;
                    break;

                case LCMD_SET_ALWAYS_ON_TOP:
                    LC->AlwaysOnTop = bool(Command.Parm1);
                    ApplyAlwaysOnTop (LC->Screen->GetHWND(), LC->AlwaysOnTop);
                    break;

                case LCMD_FILE_CHANGED:
                    // If tailing is enabled and the file has changed, run a tail update!
                    if (LC->FileContext->Tailing  &&
                        LC->FileContext->NormalLinesDB->IsDoneScanning())
                    {
                        uint64 StartSeek;
                        uint64 StartLine;
                        uint32 Backtrack;
                        CFileContext *CFC;

                        CFC = LC->FileContext;

                        if (CFC->File->GetFileSize() > 0)
                            StartSeek = CFC->File->GetFileSize() - 1; // retrieve *!old!* file size
                        else
                            StartSeek = 0;

                        Backtrack = LC->SettingsV2.GetValUint32 ("TailingBacktrackKB");

                        if (StartSeek >= ((uint64)Backtrack * 1024))
                            StartSeek -= (uint64)Backtrack * 1024;
                        else
                            StartSeek = 0;

                        if (!CFC->NormalLinesDB->SeekPointToLine (StartSeek, StartLine))
                            StartLine = 0;

                        if (StartLine != 0)
                            StartLine--;

                        CFC->OneTimeDoneScanning = false;
                        CFC->NormalLinesDB->ScanFromLine (StartLine,
                                                          DoneTailingCallback,
                                                          (void *)LC);
                    }

                    break;

                case LCMD_UPDATE_AVAILABLE:
                    {
                        UpdateInfo *Info;

                        Info = (UpdateInfo *) Command.Ptr;
                        BugUserAboutUpdate (LC->Screen->GetHWND(), LC, Info->NewVersion, Info->Message);

                        delete Info;
                    }

                    break;

                case LCMD_EDIT:
                    int SER;
                    
#pragma warning(disable:4311)
                    SER = (int) ShellExecute (LC->Screen->GetHWND(), 
                                              "open", 
                                              LC->SettingsV2.GetValString ("EditProgram").c_str(), 
                                              (string (" \"") + LC->FileContext->FileName + string ("\"")).c_str(), 
                                              ".", 
                                              SW_SHOWNORMAL);

                    if (SER <= 32)
                    {
                        string ErrorText;

                        ErrorText = tostring ("Unable to launch '")
                                  + tostring (LC->SettingsV2.GetValString ("EditProgram"))
                                  + tostring ("' to edit the file '")
                                  + tostring (LC->FileContext->FileName)
                                  + tostring ("'.\n\nThe error code is: '");

                        switch (SER)
                        {
                            default:
                                ErrorText += "Unknown error code";
                                break;

                            case ERROR_FILE_NOT_FOUND:
                                ErrorText += "The specified file was not found";
                                break;

                            case ERROR_PATH_NOT_FOUND:
                                ErrorText += "The specified path was not found";
                                break;

                            case ERROR_BAD_FORMAT:
                                ErrorText += "The .exe file is invalid (non-Win32® .exe or error in .exe image)";
                                break;
                            
                            case SE_ERR_ACCESSDENIED:  
                                ErrorText += "The operating system denied access to the specified file";  
                                break;

                            case SE_ERR_ASSOCINCOMPLETE:  
                                ErrorText += "The file name association is incomplete or invalid";
                                break; 

                            case SE_ERR_DDEBUSY:  
                                ErrorText += "The DDE transaction could not be completed because other DDE transactions were being processed";
                                break; 

                            case SE_ERR_DDEFAIL:  
                                ErrorText += "The DDE transaction failed";
                                break; 

                            case SE_ERR_DDETIMEOUT:  
                                ErrorText += "The DDE transaction could not be completed because the request timed out";
                                break; 

                            case SE_ERR_DLLNOTFOUND:  
                                ErrorText += "The specified dynamic-link library was not found";
                                break;  

                                /*
                            case SE_ERR_FNF:  
                                ErrorText += "The specified file was not found";
                                break; 
                                */

                            case SE_ERR_NOASSOC:  
                                ErrorText += "There is no application associated with the given file name extension. This error will also be returned if you attempt to print a file that is not printable";
                                break; 

                            case SE_ERR_OOM:  
                                ErrorText += "There was not enough memory to complete the operation";
                                break;

                                /*
                            case SE_ERR_PNF:  
                                ErrorText += "The specified path was not found";
                                break;
                                */

                            case SE_ERR_SHARE:  
                                ErrorText += "A sharing violation occurred";
                                break;
                        }

                        ErrorText += tostring (" (")
                                  +  tostring (SER)
                                  +  tostring (").'");

                        MessageBox (LC->Screen->GetHWND(), ErrorText.c_str(), "Error", MB_OK);
                    }

                    break;

                case LCMD_RESIZEXY:
                    {
                        bool ChangeWidth = false;
                        bool ChangeHeight = false;
                        uint32 NewWidth;
                        uint32 NewHeight;
                        ListResizeInfo *Info;
                        BOOL OldAR;

                        Info = (ListResizeInfo *) Command.Ptr;

                        NewWidth = Info->NewWidth;
                        NewHeight = Info->NewHeight;

                        NewWidth = std::max(80u, NewWidth);
                        NewHeight = std::max(10u, NewHeight);

                        OldAR = LC->Screen->GetAutoRefresh ();
                        LC->Screen->SetAutoRefresh (FALSE);

                        if (NewWidth != LC->Screen->GetConsWidth())
                            ChangeWidth = true;

                        if (NewHeight != LC->Screen->GetConsHeight())
                            ChangeHeight = true;

                        LC->Screen->GetGDIOutput()->SetPlus1FudgeX (Info->FudgeX);
                        LC->Screen->GetGDIOutput()->SetPlus1FudgeY (Info->FudgeY);

                        if (Info->Maximizing)
                            LC->SettingsV2.SetValUint32 ("Maximized", 1);
                        else
                        {
                            LC->SettingsV2.SetValUint32 ("Maximized", 0);
                            LC->SettingsV2.SetValUint32 ("Width", NewWidth);
                            LC->SettingsV2.SetValUint32 ("Height", NewHeight);
                        }

                        LC->Screen->ResizeXY (NewWidth, NewHeight, false);

                        DrawInfoLines (LC);
                        DrawTextBody (LC, 0, -1);

                        LC->CmdQueue.BumpCommandAsync (LCMD_UPDATE_INFO, CCommand (LCMD_UPDATE_INFO, 0, 0, 0, 0, NULL));
                        LC->CmdQueue.BumpCommandAsync (LCMD_UPDATE_TEXT, CCommand (LCMD_UPDATE_TEXT, 0, 0, 0, 0, NULL));
                        SetListTitle (LC);

                        if (ChangeWidth && (LC->FileContext->WrapText || LC->FileContext->HexMode))
                            ResetLinesDBs (LC, false);

                        ApplyAlwaysOnTop (LC->Screen->GetHWND(), LC->AlwaysOnTop);

                        LC->Screen->SetAutoRefresh (OldAR);
                        delete Info;
                        AddSimpleCommand (LC, LCMD_SETTINGS_SAVE);
                    }

                    break;
            }
        }

        if (AddCheckScanning)
            AddSimpleCommand (LC, LCMD_CHECK_SCANNING);

        return;
    } unguard;
}


// Deletes a CParser2::TextLine
void DeleteTextLine (CParser2::TextLine *Line)
{
    guard
    {
        delete Line->Text;
        Line->Text = NULL;
        delete Line;
        return;
    } unguard;
}


CParser2::TextLine *BlankTextLine (void)
{
    guard
    {
        CParser2::TextLine *Return;

        Return = new CParser2::TextLine;
        Return->Bytes = 0;
        Return->Chars = 0;
        Return->Dependent = false;
        Return->LastChar = CParser2::InvalidChar;
        Return->Text = new wchar_t[1];
        Return->Text[0] = L'\0';

        return (Return);
    } unguard;
}


// Returns a copy of a CParser2::TextLine object. Be sure to delete it when done.
CParser2::TextLine *CopyTextLine (CParser2::TextLine *CopyMe, bool CopyFormatting)
{
    guard
    {
        CParser2::TextLine *Return;

        Return = new CParser2::TextLine;
        Return->Bytes = CopyMe->Bytes;
        Return->Chars = CopyMe->Chars;
        Return->Dependent = CopyMe->Dependent;

        if (CopyFormatting) 
            Return->Formatting = CopyMe->Formatting;

        Return->LastChar = CopyMe->LastChar;
        Return->Text = new wchar_t[Return->Chars + 1];
        StreamCopy (Return->Text, CopyMe->Text, sizeof(wchar_t) * Return->Chars);
        Return->Text[Return->Chars] = L'\0';

        return (Return);
    } unguard;
}


// Pastes two CParser2::TextLines together and returns a pointer to a new
// structure. Be sure to delete it when done.
// Does not copy formatting, because we only use this in the search code, which doesn't care about that.
// If either Left or Right are NULL, they are assumed to be 0-byte strings instead
CParser2::TextLine *PasteTwoLines (CParser2::TextLine *Left, CParser2::TextLine *Right)
{
    guard
    {
        CParser2::TextLine *Result;

        if (Left == NULL  &&  Right != NULL)
            return (CopyTextLine (Right));

        if (Left != NULL  &&  Right == NULL)
            return (CopyTextLine (Left));

        if (Left == NULL  &&  Right == NULL)
            return (BlankTextLine ());

        Result = new CParser2::TextLine;
        Result->Bytes = Left->Bytes + Right->Bytes;
        Result->Chars = Left->Chars + Right->Chars;
        Result->Dependent = Left->Dependent;
        Result->LastChar = Right->LastChar;
        Result->Text = new wchar_t[Result->Chars + 1];
        StreamCopy (Result->Text, Left->Text, Left->Chars * sizeof (wchar_t));
        StreamCopy (Result->Text + Left->Chars, Right->Text, Right->Chars * sizeof (wchar_t));
        Result->Text[Result->Chars] = L'\0';

        return (Result);
    } unguard;
}

// Accumulates a line:
// if line #Start does not end with the EOL character, then GetLineRange will search
// for the first line that does end with the EOL character and place that line # into
// LastResult. Otherwise LastResult=Start.
// If the last line couldn't be found (i.e. it is outside of [Start,End)) then
// false is returned. Otherwise true is returned.
// GetLineRange will only search through [Start,End)
bool GetLineRange (ListThreadContext *LC, uint64 Start, uint64 End, uint64 &LastResult) throw (runtime_error)
{
    guard
    {
        CParser2::TextLine *Line;
        bool ret = false;

        while (Start < End)
        {
            if (!LC->FileContext->NormalLinesDB->GetLine (Start, &Line))
                guard_throw (runtime_error ("runtime_error: GetLine returned false to GetLineRange"));

            if (Line == NULL)
                guard_throw (runtime_error ("runtime_error: GetLine returned NULL to GetLineRange"));

            if ((Line->Chars > 0  &&  Line->Text[Line->Chars - 1] == '\n') ||
                !(Start + 1 < End))
            {
                ret = true;
                break;
            }

            Start++;
        }

        LastResult = Start;
        return (ret);
    } unguard;
}

// Gets a line and does necessary error handling
CParser2::TextLine *GetLine (ListThreadContext *LC, uint64 Line) throw (runtime_error)
{
    guard
    {
        CParser2::TextLine *Return;

        if (!LC->FileContext->NormalLinesDB->GetLine (Line, &Return))
            guard_throw (runtime_error ("runtime_error: CFileLinesDB::GetLine returned false to GetLine"));

        if (Return == NULL)
            guard_throw (runtime_error ("runtime_error: CFileLinesDB::GetLine returned NULL to GetLine"));

        return (Return);
    } unguard;
}

// Concatenates the given line range into one line that is returned
// If only a single line # is passed in (i.e. First == Last) then the
// line should NOT be freed, as it is a pointer to the entry in the
// CFileLinesDB database. This will be indicated by a proper value in
// FreeableResult upon exit.
CParser2::TextLine *AccumulateLines (ListThreadContext *LC, uint64 First, uint64 Last, bool &FreeableResult)
{
    guard
    {
        CParser2::TextLine *Accum = NULL; // accumulator
        uint64 Line;

        if (First == Last)
        {
            CParser2::TextLine *Return = NULL;

            Return = GetLine (LC, First);
            FreeableResult = false;
            return (Return);
        }

        Accum = BlankTextLine ();
        for (Line = First; Line <= Last; Line++)
        {
            CParser2::TextLine *One;
            CParser2::TextLine *Paste;

            One = GetLine (LC, Line);
            Paste = PasteTwoLines (Accum, One);
            DeleteTextLine (Accum);
            Accum = Paste;
        }

        FreeableResult = true;
        return (Accum);
    } unguard;
}

// Searches exactly 1 line of text
bool SearchLine (ListThreadContext *LC, uint64 Line, CSearch::MatchExtentGroup *ExtentResult)
{
    guard
    {
        bool Found;
        CParser2::TextLine *Text;

        Text = GetLine (LC, Line);
        Found = LC->FileContext->SearchMachine->MatchPattern (Text->Text, Text->Chars, ExtentResult);

        // Do not free Text since it is straight from CFileLinesDB
        return (Found);
    } unguard;
}

// Searches up to 1 line of 'text' (see note) within [Start,End)
// If something like word wrap is enabled, 1 'line' actually extends over multiple
// lines. Thus, EndResult will be the value of the line past what we searched
// (i.e. if you have a loop, this should be the value of your loop index for the
// next loop iteration)
// If the text is found, true will be returned and the line that the text was found in
// (in a range from [Start,EndResult)) will be placed into FoundResult
// i.e. a line of text may 'wrap' from [n,n+m) and will be searched as one line, but
// the 
// Typical case: EndResult = Start+1
// Extreme case: EndResult = End
// BTW: Pinpoint is used 'internally'. Leave it alone when calling this function!
//      It is used when pinpointing the line of text that matched our pattern.
//      It helps to avoid catatonic (in fact, nearly O(n!) performance) slowdown
//      If Pinpoint is true, then FoundResult will not be changed.
bool SearchLineV (ListThreadContext *LC, uint64 Start, uint64 End, uint64 &EndResult, uint64 &FoundResult, CSearch::MatchExtentGroup *ExtentResult, bool Pinpoint)
    throw (runtime_error)
{
    guard
    {
        uint64 Last;
        bool FreeMe;
        bool Found;
        CParser2::TextLine *Text;

        GetLineRange (LC, Start, End, Last); // error handling?
        Text = AccumulateLines (LC, Start, Last, FreeMe);

        // Search accumulated line of text
        Found = LC->FileContext->SearchMachine->MatchPattern (Text->Text, Text->Chars, ExtentResult);

        // Did we find the text!? If so, we need to figure out exactly which line it was on
        // To do this, we actually call ourself recursively on a smaller and smaller text
        // range. Basically we search the ranges [First,Last] through [Last,Last] (i.e.
        // [First+n,Last] for n=[1,Last-First]) and the first one that gives us a negative
        // result is the one past where the text occurred
        if (Found && Pinpoint)
        {
            uint64 i;
            bool Pinpointed = false;

            FoundResult = Last;

            if (Start == Last)
                Pinpointed = true;
            else
            for (i = Start; i <= Last; i++)
            {
                uint64 EndR;
                uint64 FoundR;

                if (!SearchLineV (LC, i, Last + 1, EndR, FoundR, ExtentResult, false))
                {   // aha, we have pinpointed it to the line i-1
                    Pinpointed = true;
                    FoundResult = i - 1;
                    break;
                }
            }
        }

        // Deallocate memory if necessary
        if (FreeMe)
            DeleteTextLine (Text);

        EndResult = Last + 1;
        return (Found);
    } unguard;
}

// Searches the given range of lines for the text you want. Searches [Start,End)
// Returns the line # that the text occurred in, or End if it wasn't found
uint64 SearchRange (ListThreadContext *LC, uint64 Start, const uint64 End, CSearch::MatchExtentGroup *ExtentResult)
{
    guard
    {
        uint32 Total = LC->FileContext->NormalLinesDB->GetTotalLines ();

        while (Start < End)
        {
            uint64 NextStart;
            uint64 MatchLine;
            bool Found;

            // Note: We use Total instead of End because it is possible that a line could wrap
            // and go past End, in which case it would never actually match our search string
            // in the correct circumstances.
            Found = SearchLineV (LC, Start, /*End*/ Total, NextStart, MatchLine, ExtentResult);

            if (Found)
                return (MatchLine);

            Start = NextStart;
        }

        return (End);
    } unguard;
}


// Searches the given range of lines in reverse order. Searches [Start,End)
// starting with End-1 and finishing with Start.
// Returns the line # that the text occurred in, or End if it wasn't found.
uint64 SearchRangeRev (ListThreadContext *LC, uint64 Start, const uint64 End, CSearch::MatchExtentGroup *ExtentResult)
{
    guard
    {
        uint64 Begin;
        uint32 Total = LC->FileContext->NormalLinesDB->GetTotalLines ();

        Begin = End - 1;

        if (Begin > End) // underflow from unsigned 0 -> ~0
            return (End);

        while (Begin >= Start)
        {
            uint64 MatchLine;
            uint64 NextStart; // not used, really; value is throw away
            bool Found;

            Found = SearchLineV (LC, Begin, /*End*/ Total, NextStart, MatchLine, ExtentResult);

            if (Found)
                return (MatchLine);

            Begin--;

            if (Begin > End)  // underflow from unsigned 0 -> ~0
                return (End);
        }

        return (End);
    } unguard;
}


void DisableHighlight (ListThreadContext *LC)
{
    guard
    {
        LC->FileContext->Highlight = false;
        LC->FileContext->HighlightSecondary.clear ();
        return;
    } unguard;
}


void SetHighlight (ListThreadContext *LC, bool Enable, uint64 Line, const CSearch::MatchExtentGroup &Extents)
{
    guard
    {
        LC->FileContext->Highlight = Enable;
        LC->FileContext->HighlightLine = Line;
        LC->FileContext->HighlightSecondary = Extents;
        return;
    } unguard;
}


// Brings the requested line in view
// The line number given corresponds to the ACTIVE LinesDB context
// If the given line # is already in view, nothing will be done.
// If hex-view is turned on, then the line # will be converted to the
// appropriate hex-view line
void JumpToLine (ListThreadContext *LC, uint64 JumpHere, bool Center)
{
    guard
    {
        if (LC->FileContext->HexMode)
            NormalLineToHexLine (LC, JumpHere, JumpHere);

        if ((JumpHere < LC->FileContext->CurrentLine) || 
            (JumpHere > (LC->FileContext->CurrentLine + LC->Screen->GetMaxConsY() - 2)))
        {   // The line is not in view!
            uint64 JumpTo;

            if (JumpHere < (LC->Screen->GetMaxConsY() - 2))
            {   // If it's the first page of the file, jump to line #0
                JumpTo = 0;
            }
            else
            {
                if (Center)
                    JumpTo = JumpHere - (LC->Screen->GetMaxConsY() / 2) + 1;
                else
                    JumpTo = JumpHere;
            }

            AddCommand (LC, LCMD_SET_LINE, LODWORD(JumpTo), HIDWORD(JumpTo), 0, 0, NULL);
            AddSimpleCommand (LC, LCMD_UPDATE_TEXT);
        }
    } unguard;    
}


string GetShortFileName (string LongFileName)
{
    guard
    {
        string::iterator p;
        string::iterator begin;

        //char *p;
        //char *begin;

        begin = LongFileName.begin ();
        p = --LongFileName.end();

        //begin = (char *)LongFileName.c_str();
        //p = begin + LongFileName.length();

	    while (p >= begin && *p != '\\') 
            p--;

        p++;

        return (string (p, LongFileName.end()));
        //return (string (p));
    } unguard;
}


// Returns true if we are running in a remote session with Terminal Services
// or Remote Desktop Connection
typedef BOOL (WINAPI *PidToSidFunction) (DWORD, DWORD *);
bool AreWeRemote (void)
{
    guard
    {
        HMODULE Module = NULL;
        PidToSidFunction PidToSid = NULL;
        DWORD Pid = 0;
        DWORD Sid = 0;
        bool ReturnVal = false;

        Module = LoadLibrary ("kernel32.dll");

        if (Module == NULL)
            return (false);

        PidToSid = (PidToSidFunction) GetProcAddress (Module, "ProcessIdToSessionId");

        if (PidToSid == NULL)
            ReturnVal = false;
        else
        {   
            Pid = GetCurrentProcessId ();

            if (!PidToSid (Pid, &Sid))
                ReturnVal = false;
            else
            if (Sid == 0)
                ReturnVal = false;
            else
                ReturnVal = true;
        }

        FreeLibrary (Module);
        return (ReturnVal);
    } unguard;
}


void SetListTitle (ListThreadContext *LC)
{
    guard
    {
        char Title[1024];
        string ShortName;

        LC->FileContext->PrintFileName = LC->FileContext->FileName; //GetShortFileName(LC.FileName);
        if (LC->FileContext->PrintFileName.length() > (LC->Screen->GetMaxX() - 45))
        {
            LC->FileContext->PrintFileName.resize ((LC->Screen->GetMaxX() - 48));
            LC->FileContext->PrintFileName += "...";
        }

        ShortName = GetShortFileName (LC->FileContext->FileName);

        sprintf (Title, "%s - %s (%ux%u, %s)", ShortName.c_str(), LISTTITLE " " LISTVERSION, 
            LC->Screen->GetConsWidth(), 
            LC->Screen->GetConsHeight(),
            GetParserName (LC->FileContext->ParserIndice).c_str());

        SetWindowText (LC->Screen->GetHWND(), Title);

        return;
    } unguard;
}


// Move to CFileContext?
void SetParser (ListThreadContext *LC, int ParserIndice)
{
    guard
    {
        if (LC->FileContext->Parser != NULL)
            delete LC->FileContext->Parser;

        LC->FileContext->ParserIndice = ParserIndice;
        LC->FileContext->Parser = CreateParser (LC, LC->FileContext->ParserIndice);
        return;
    } unguard;
}


int DetectParser (ListThreadContext *LC)
{
    guard
    {
        uint8 Buffer[512];
        int *Ratings;
        int i;
        int Best;
        int Count;
        uint64 OldSeek;

        ZeroMemory (Buffer, sizeof (Buffer));
        LC->FileContext->File->Lock ();
        OldSeek = LC->FileContext->File->GetFilePos ();
        LC->FileContext->File->Seek (0);
        LC->FileContext->File->ReadData (Buffer, sizeof (Buffer), NULL);
        LC->FileContext->File->Seek (OldSeek);
        LC->FileContext->File->Unlock ();

        Count = GetParserCount();
        Ratings = new int[Count];

        // Get the parsers' ratings
        for (i = 0; i < Count; i++)
            Ratings[i] = IdentifyParser (i, Buffer);

        // Now find the "best" one
        Best = 0;
        for (i = 0; i < Count; i++)
        {
            if (Ratings[i] > Best)
                Best = i;
        }

        // Now create it!
        delete Ratings;
        return (Best);
    } unguard;
}


void SetSearchPattern (ListThreadContext *LC, const char *NewPattern)
{
    guard
    {
        wchar_t *PatternCopy;

        if (LC->FileContext->SearchMachine != NULL)
            delete LC->FileContext->SearchMachine;

        switch (LC->FileContext->SearchType)
        {
            case SEARCHTYPE_LITERAL:
                LC->FileContext->SearchMachine = new CSearchKMP (LC->FileContext->SearchCaseSensitive);
                break;

            case SEARCHTYPE_BOOLEAN:
                LC->FileContext->SearchMachine = new CSearchBoolean (LC->FileContext->SearchCaseSensitive);
                break;

            case SEARCHTYPE_REGEX:
                LC->FileContext->SearchMachine = new CSearchRegex (LC->FileContext->SearchCaseSensitive);
                break;
        }

        PatternCopy = StrDupA2W (NewPattern);
        if (!LC->FileContext->SearchMachine->CompileSearch (PatternCopy))
        {
            LC->FileContext->HaveDoneSearchBefore = false;
            LC->FileContext->DoingSearch = false;
            AddCommand (LC, LCMD_SET_INFO_TEXT, 0, 0, 0, 0, strdup (LC->FileContext->SearchMachine->GetLastError().c_str()));    
        }

        delete PatternCopy;

        LC->FileContext->SearchSubstring = string (NewPattern);
        return;
    } unguard;
}


#if 0
// Searches all lines, [Start,End), and places the matching line numbers into the given container
// Also sets FoundAny to true if any were found, or false if not
template<class iter>
void SearchRangeAll (ListThreadContext *LC, iter Results, uint64 Start, uint64 End, bool &FoundAny)
{
    guard
    {
        while (Start < End)
        {
            uint64 NextStart;
            uint64 MatchLine;
            bool Found;

            Found = SearchLineV (LC, Start, End, NextStart, MatchLine);

            if (Found)
            {
                FoundAny = true;
                *iter = MatchLine;
                ++iter;
            }

            Start = NextStart;
        }

        return (End);
    } unguard;
}

// Searches are done in the range [searchjob.first,searchjob.second)
typedef pair<uint64,uint64> searchjob;


class CSearchWorkerThread : public CThread
{
public:
    CSearchWorkerThread (ListThreadContext *LC_init,
                         bool Reverse_init,
                         bool &Found_init,
                         CJobQueue<searchjob> &JobQueue_init)
        : LC (LC_init),
          Reverse (Reverse_init),
          Found (Found_init),
          JobQueue (JobQueue_init),
          terminate (false),
          done_signal (false),
          Matched (false)
    {
        guard
        {

        } unguard;
    }

    ~CSearchWorkerThread ()
    {
        guard
        {
            Finalize ();
            return;
        } unguard;
    }

    bool ThreadFunction (void)
    {
        guard
        {
            while (!terminate)
            {
                searchjob job;
                CSearch::MatchExtentGroup Extents;

                JobQueue.WaitForJob ();

                if (JobQueue.GetJob (job))
                {
                    uint64 Result;

                    if (Reverse)
                        Result = SearchRangeRev (LC, job->first, job->end, &Extents);
                    else
                        Result = SearchRange (LC, job->first, job->end, &Extents);

                    // Found a match?
                    if (Result != job->end)
                    {
                        Matched = true;
                        MatchLine = Result;
                        MatchHighlights = Extents;
                        Found = true;

                        // Quit now -- don't do ANY more searching!
                        break;
                    }
                } // if
            } // while

            done_signal.Signal ();
            return (false);
        } unguard;
    }

    void GetMatchInfo (bool &Success, uint64 &Line, CSearch::MatchExtentGroup &Highlights)
    {
        guard
        {
            Success = Matched;

            if (Success)
            {
                Line = MatchLine;
                Highlights = MatchHighlights;
            }

            return;
        } unguard;
    }

    // Tell it to stop!
    void Finalize (void)
    {
        guard
        {
            terminate = true;
            done_signal.WaitUntilSignaled ();
            return;
        } unguard;
    }

private:
    ListThreadContext *LC;
    volatile bool &Found;
    CJobQueue<searchjob> &JobQueue;
    bool Reverse;
    volatile bool terminate; // signal us to stop!
    CSignal done_signal; // signal that we've stopped

    // Used to hold result of a successful search
    bool Matched;
    uint64 MatchLine;
    CSearch::MatchExtentGroup MatchHighlights;
};


// Once started, this will search from the given start line # in the given direction
// and will stop as soon as it finds a matching line. In the background.
class CSearchManager : public CThread
{
public:
    CSearchManager (ListThreadContext *LC_init, 
                    uint64 Start_init, 
                    bool Reverse_init, 
                    int WorkersCount_init)
        : LC (LC_init),
          Start (Start_init),
          Reverse (Reverse_init),
          WorkersCount (Workers_init)
    {
        guard
        {
            JobSize = LC->SettingsV2.GetValUint32 ("SearchLinesPerJob");
            return;
        } unguard;
    }

    ~CSearchManager ()
    {
        guard
        {
            terminate = true;
            done_signal.WaitUntilSignaled ();
            return;
        } unguard;
    }

    void ThreadFunction (void)
    {
        guard
        {
            /*
    CSearchWorkerThread (ListThreadContext *LC_init,
                         bool Reverse_init,
                         CSignal &Found_init,
                         CJobQueue<searchjob> &JobQueue_init)
                         */

            CSearchWorkerThread **Threads;
            volatile bool FoundSignal = false;
            CJobQueue<searchjob> JobQueue;
            uint64 LastJob; // which job ordinal did we add last?
                            // job[i] = searchjob<begin + i*JobSize,
                            //                    begin + (i+1)*JobSize>
                            // job[LastJob-1] = searchjob<begin + (LastJob-1)*JobSize, end>
            searchjob job;
            uint64 begin = Start;
            uint64 nextstart = begin;
            int i;

            LastJob = 0;

            // Init thread array
            Threads = new CSearchWorkerThread[WorkersCount];

            for (i = 0; i < WorkersCount; i++)
                Threads[i] = new CSearchWorkerThread (LC, Reverse, FoundSignal, JobQueue);

            // Add initial jobs
            for (i = 0; i < WorkersCount * 2; i++)
            {
                uint64 total;

                job.first = nextstart;
                job.second = job.first + JobSize;
                total = LC->FileContext->NormalLinesDB->GetTotalLines();

                if (job.second > total)
                    job.second = total;

                nextstart = job.second;
                JobQueue.AddJob (job);
            }

            // Let them go!
            for (i = 0; i < WorkersCount; i++)
                Threads[i]->RunThread ();

            // Conditions to wait:
            // 1. JobQueue is not empty
            // 2. 
            while (!terminate)
            {

            } 

            // We're done. Either we found something or we reached the EOF.
            for (i = 0; i < WorkersCount; i++)
                Threads[i]->Finalize ();

            if (FoundSignal)
            {
                // Go through and find the earliest search match
                // (or "last" search match if doing reverse)
                uint64 MaxLine;
                CSearch::MatchExtentGroup MaxHighlight;

                if (Reverse)
                    MaxLine = 0;
                else
                    MaxLine = 0xffffffffffffffff;

                for (i = 0; i < WorkersCount; i++)
                {
                    bool Matched;
                    uint64 MatchLine;
                    CSearch::MatchExtentGroup MatchHighlights;

                    Threads[i]->GetMatchInfo (Matched, MatchLine, MatchHighlights);

                    if (Matched)
                    {
                        if (Reverse ? (MatchLine >= MaxLine) : (MatchLine <= MaxLine))
                        {
                            MaxLine = MatchLine;
                            MaxHighlight = MatchHighlights;
                        }
                    }
                }
            }

        } unguard;
    }

private:
    uint64 JobSize;




    bool terminate;
    CSignal done_signal;
    int WorkersCount;
    uint64 Start;
    bool Reverse;
    ListThreadContext *LC;
};


// Like HandleSearching(), but manages a background pool of threads that search
// Unlike HandleSearching, this one does not discontinue searching when it returns
void HandleSearchingMT (ListThreadContext *LC)
{
    
}

#endif

void HandleSearching (ListThreadContext *LC)
{
    guard
    {
        DWORD Start;
        int Count = 0;
        const uint64 LinesPerJob = LC->SettingsV2.GetValUint32 ("SearchLinesPerJob");
        uint64 TotalLinesCache;
        uint64 OldNextLine;
        CSearch::MatchExtentGroup Extents;

        OldNextLine = LC->FileContext->NextLineToSearch;
        LC->FileContext->HaveDoneSearchBefore = true;
        LC->FileContext->NormalLinesDB->Lock ();
        TotalLinesCache = LC->FileContext->NormalLinesDB->GetTotalLines ();

    #ifdef SEARCH_BENCH
        uint64 StartLine;
        uint64 EndLine;
        DWORD StartTime;
        DWORD EndTime;

        StartTime = timeGetTime();
        StartLine = LC->FileContext->NextLineToSearch;
    #endif

        // Prefetching
        if (1 == LC->SettingsV2.GetValUint32 ("Prefetching"))
        {
            uint64 PrefetchSeekPoint;
            uint64 PrefetchLine;
            uint32 SeekGranularity;

            SeekGranularity = 1 << LC->SettingsV2.GetValUint32 ("SeekGranularityLog2");

            PrefetchLine = LC->FileContext->NextLineToSearch;

            if (PrefetchLine < SeekGranularity && LC->FileContext->SearchDirection == -1)
                PrefetchLine = 0;
            else
                PrefetchLine += SeekGranularity * LC->FileContext->SearchDirection;

            if (LC->FileContext->NormalLinesDB->GetSeekPoint (PrefetchLine, PrefetchSeekPoint))
                LC->FileContext->File->Prefetch (PrefetchSeekPoint);
        }

        //LC->UpdateInfo = true;
        AddSimpleCommand (LC, LCMD_UPDATE_INFO);

        // We run our search for 125 lines / 20ms at a time (whichever is longer)
        // Every 125 lines of searching, we check our watch and
        // if we've been searching for 20ms or more, we break out
        // and do other normal processing
        // Note that is LessVisUpdates is true, then we check every 100ms, not 20ms
        // Or if we're benchmarking.
        Start = timeGetTime ();

        while (true)
        {
            uint64 Begin;
            uint64 End;
            uint64 MatchingLine;
            uint64 NextBegin; // set up where the next search job should start at
            bool TheEnd = false; // did we reach The End of the file?
            bool Found;

            // Set up range of lines to search, where the next search should start, then do search

            // Reverse searching
            if (LC->FileContext->SearchDirection == -1)
            {
                End = LC->FileContext->NextLineToSearch + 1;

                if (End < LinesPerJob)
                {   // We are searching through a range that ends at the beginning of the file
                    // THus, once we are done with this search we will have searched as far as
                    // we can, and so we are at "the end"
                    Begin = 0;
                    NextBegin = 0;
                    TheEnd = true;
                }
                else
                {   // But in this case that is not true.
                    Begin = End - LinesPerJob;
                    NextBegin = Begin - 1;
                    TheEnd = false;
                }

                // Perform search
                MatchingLine = SearchRangeRev (LC, Begin, End, &Extents);

                if (MatchingLine != End)
                {
                    if (MatchingLine != 0)
                    {   // Oh, I guess we didn't search all the way to the 'end' ("beginning" :) of
                        // the file. 
                        NextBegin = MatchingLine - 1;
                        TheEnd = false;
                    }
                    else
                    {
                        NextBegin = 0; //TotalLinesCache;
                        TheEnd = true;
                    }
                }

                if (MatchingLine == End  &&  Begin == 0)
                {
                    NextBegin = 0;
                    TheEnd = true;
                }
            }
            //if (LC->FileContext->SearchDirection == +1)
            // Foward searching
            else
            {
                Begin = LC->FileContext->NextLineToSearch;
                End = Begin + LinesPerJob;

                if (End > TotalLinesCache)
                {
                    End = TotalLinesCache;
                    NextBegin = TotalLinesCache - 1;
                    TheEnd = true;
                }
                else
                {
                    NextBegin = End + 1;
                    TheEnd = false;
                }

                // Perform search
                MatchingLine = SearchRange (LC, Begin, End, &Extents);

                if (MatchingLine != End)
                {
                    if (MatchingLine != TotalLinesCache - 1)
                        NextBegin = MatchingLine + 1;
                    else
                    {
                        NextBegin = TotalLinesCache;
                        TheEnd = true;
                    }
                }
            }

            Found = (MatchingLine != End);
            LC->FileContext->NextLineToSearch = NextBegin;

            // If we found the line, we probably want to jump to, and Highlight, that line
            // We also want to turn off searching!
            if (Found)
            {
                if (LC->FileContext->HexMode)
                    NormalLineToHexLine (LC, MatchingLine, MatchingLine);

                JumpToLine (LC, MatchingLine, true);
                SetHighlight (LC, true, MatchingLine, Extents);
                LC->FileContext->DoingSearch = false;
                AddSimpleCommand (LC, LCMD_UPDATE_TEXT);

                // Then break out of our while(true) loop.
                break;
            }
            else
            // Invariant: we did not find a matching line of text
            // If we reached The End, then say "Haha, could not find your stuffs!"
            if (TheEnd)
            {
                if (LC->FileContext->NormalLinesDB->IsDoneScanning() || LC->FileContext->SearchDirection == -1)
                {
                    LC->FileContext->DoingSearch = false;
                    AddCommand (LC, LCMD_SET_INFO_TEXT, 0, 0, 0, 0, strdup ("Could not find search text."));
                    AddSimpleCommand (LC, LCMD_UPDATE_INFO);
                }

                break;
            }

            // Have we been searching too long!?
            if ((timeGetTime() - Start) > 
#ifdef SEARCH_BENCH
                REMOTESLEEP)
#else
                (LC->LessVisUpdates ? REMOTESLEEP : NORMALSLEEP))
#endif
            {
                break;
            }
        }

        LC->FileContext->NormalLinesDB->Unlock ();

        // Our we doing an ANIMATED SEARCH!?
        if (LC->SettingsV2.GetValUint32 ("AnimatedSearch") == 1)
            JumpToLine (LC, LC->FileContext->NextLineToSearch, true);

    #ifdef SEARCH_BENCH
        EndTime = timeGetTime();
        EndLine = LC->FileContext->NextLineToSearch;

        char Text[1000];

        sprintf (Text, "%.2lf lines/sec", 1000.0 * double(EndLine - StartLine) / double(EndTime - StartTime));
        AddCommand (LC, LCMD_SET_INFO_TEXT, 0, 0, 0, 0, strdup (Text));
    #endif

        return;
    } unguard;
}


bool is_on (const string &val)
{
    string l;

    transform (val.begin(), val.end(), back_inserter(l), tolower);

    if (l == "0"  ||  l == "no"   ||  l == "off"  ||  l == "false")
        return (false);

    if (l == "1"  ||  l == "yes"  ||  l == "on"   ||  l == "true")
        return (true);

    return (false);
}

bool is_off (const string &val)
{
    string l;

    transform (val.begin(), val.end(), back_inserter(l), tolower);

    if (l == "0"  ||  l == "no"   ||  l == "off"  ||  l == "false")
        return (true);

    if (l == "1"  ||  l == "yes"  ||  l == "on"   ||  l == "true")
        return (false);

    return (false);
}

// Applies one option
void ApplyOption (ListThreadContext *LC, Option option)
{
    string &tag = option.first;
    string &val = option.second;
    string tag_l; // lowercase copy of tag
    string val_l; // lowercase copy of val

    transform (tag.begin(), tag.end(), back_inserter(tag_l), tolower);
    transform (val.begin(), val.end(), back_inserter(val_l), tolower);

    if (tag_l == "tail"  ||  tag_l == "tailing")
    {   // Tailing
        if (is_on (val))
        {
            AddCommand (LC, LCMD_SET_TAILING, 1, 0, 0, 0, NULL);
            AddSimpleCommand (LC, LCMD_INPUT_GO_END);
        }
    }
    else
    if (tag_l == "wrap"  ||  tag_l == "wrapping")
    {
        if (is_on (val))
            AddCommand (LC, LCMD_SET_LINE_WRAPPING, 1, 0, 0, 0, NULL);
        else
        if (is_off (val))
            AddCommand (LC, LCMD_SET_LINE_WRAPPING, 0, 0, 0, 0, NULL);
    }
    else
    if (tag_l == "searchtype")
    {   // Set search type
        if (val == tostring(SEARCHTYPE_LITERAL)  ||  val_l == "literal"  ||  val_l == "l")
            AddCommand (LC, LCMD_SEARCH_SET_TYPE, SEARCHTYPE_LITERAL, 0, 0, 0, NULL);
        else
        if (val == tostring(SEARCHTYPE_BOOLEAN)  ||  val_l == "boolean"  ||  val_l == "bool"    ||  val_l == "b")
            AddCommand (LC, LCMD_SEARCH_SET_TYPE, SEARCHTYPE_BOOLEAN, 0, 0, 0, NULL);
        else
        if (val == tostring(SEARCHTYPE_REGEX)    ||  val_l == "regex"    ||  val_l == "regexp"  ||  val_l == "r"  ||  val_l == "x")
            AddCommand (LC, LCMD_SEARCH_SET_TYPE, SEARCHTYPE_REGEX,   0, 0, 0, NULL);
    }
    else
    if (tag_l == "setsearch")
    {   // Set the search pattern
        SetSearchPattern (LC, val.c_str());
        LC->FileContext->HaveDoneSearchBefore = true;
    }
    else
    if (tag_l == "searchcase")
    {   // Set the search case sensitivity
        if (is_off (val))
            AddCommand (LC, LCMD_SEARCH_SET_MATCH_CASE, 0, 0, 0, 0, NULL);
        else
        if (is_on (val))
            AddCommand (LC, LCMD_SEARCH_SET_MATCH_CASE, 1, 0, 0, 0, NULL);
    }
    else
    if (tag_l == "dosearch")
    {   // Sets the search pattern AND initiates the search
        ApplyOption (LC, Option ("setsearch", val));
        LC->FileContext->DoingSearch = true;
    }
    else
    // Set the value of any variable: does not use lowercase form of tag
    if (tag.length() > 0  &&  tag[0] == '_')
    {
        VarAnonExt Default;
        string Name (++tag.begin(), tag.end());
        
        Default = LC->SettingsV2.GetVar (Name);
        LC->SettingsV2.SetVal (Name, val);
        LC->SettingsV2.AddVolatile (Name);
        ApplySettingsV2 (LC, LC->SettingsV2);
    }

    return;
}


void ApplyOptions (ListThreadContext *LC, const OptionList &Options)
{
    OptionList::const_iterator it;

    for_each (Options.begin(), Options.end(), bind1st(ptr_fun(ApplyOption), LC));
    return;
}


DWORD WINAPI _ListThread (LPVOID Context)
{
    guard
    {
        int i;
        ListThreadContext *LC;
        ListStartContext *LSC;
        OptionList Options;

        timeBeginPeriod (1);

        //srand (timeGetTime());

        LSC = (ListStartContext *) Context;
        LC = (ListThreadContext *) LSC->LTC;
        LC->LSC = LSC;

        LC->Screen = LC->LSC->Screen;
        LC->FileContext = new CFileContext;
        LC->WantToQuit = false;

        LC->VScrollAccumulate = 0;
        LC->UpdateCount = 0;
        LC->WaitForScan = false;
        LC->UpdateInfo = true;
        LC->InfoText = "";
        LC->UpdateText = true;
        LC->DisplayHelp = false;
        LC->DisplayInfo = false;
        LC->DisplayRuler = false;
        LC->EscTimestamp = GetTickCount() - ESCTIMEOUT - 1;
        LC->JunkFilter = false;
        LC->AlwaysOnTop = false;
        LC->ShowMarkZone = false;
        LC->LessVisUpdates = false;
        LC->SizeCheckInterval = 1000; // simple value for now until it is loaded from var
        LC->SizeCheckTimestamp = GetTickCount();

        // Init Hive info
        SetHiveEnabled (bool(LC->SettingsV2.GetValUint32 ("HiveEnabled")));
        SetHiveCompression (bool(LC->SettingsV2.GetValUint32 ("HiveCompression")));
        SetHivePath (LC->SettingsV2.GetValString ("HivePath"));

        // Initialize our tolower() replacement table
        for (i = 0; i < 65536; i++)
        {
            ToLowerTable[i] = towlower((wchar_t)i);
            IdentityTable[i] = (wchar_t)i;
        }

        LC->FileContext->WrapText = bool(LC->SettingsV2.GetValUint32("WrapText"));
        if (LC->FileContext->WrapText)
            LC->FileContext->MaxWidth = LC->Screen->GetMaxX() + 1;
        else
            LC->FileContext->MaxWidth = CLINESDB_MAXWIDTH;

        LC->Screen->SetConsoleExceptions (FALSE);
        LC->Screen->ResizeQ (1000);

        // Set the background brush
        /*
        SetClassLongPtr (LC->Screen->GetHWND(), GCLP_HBRBACKGROUND, 
            (LONG_PTR) CreateSolidBrush (LC->SettingsV2.GetValUint32("TextColor.Background"))); //(LC->Settings.TextColor.Background));
            */

        LSC->DoneInit = true;

        while (LSC->DoneInit == true)
            Sleep (1);

        LSC->DoneInit = true;

        // Figure out what file we want to open here.
        LC->FileContext->FileName = ParseForOptionsAndTag (LC->LSC->CommandLine).first;
        ParseFileName (LC, LC->FileContext->FileName);

        // Get our options
        Options = ParseForOptionsAndTag (LC->LSC->CommandLine).second;

        // Append global options
        copy (LC->LSC->GlobalOptions->begin(),
              LC->LSC->GlobalOptions->end(),
              back_inserter(Options));

        // We will apply these options/commands later

        // Set window text
        SetListTitle (LC);

    #ifdef SCAN_BENCH
        LC->ScanBenchTimeStamp = GetTickCount ();
    #endif

        // Create file-class instance
        LC->FileContext->File = OpenFile (LC, LC->FileContext->FileName);

        // Error?
        if (LC->FileContext->File == NULL  ||
            LC->FileContext->File->GetLastError() != CFastFile::NoError)
        {
            string Reason;
            DWORD WinError;
            char ErrorName[1024];

            // Figure out why
            WinError = LC->FileContext->File->GetLastWin32Error ();

            FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM,
                           0,
                           WinError,
                           0,
                           ErrorName,
                           sizeof(ErrorName),
                           NULL);

            MessageBox (LC->Screen->GetHWND(), 
                (string("Could not open file: ") + LC->FileContext->FileName + string("\n\n") + string("Reason: ") + string(ErrorName)).c_str(),
                LISTTITLE " File Open Error", MB_OK | MB_ICONERROR);

            timeEndPeriod (1);
            delete LC->FileContext->File;
            LC->Screen->SetWindowProcContext (NULL);

            ExitThread (1);
        }

        // Init parser
        InitParserSystem ();

        if (LC->SettingsV2.GetValUint32("DetectParsers"))
            LC->FileContext->ParserIndice = DetectParser (LC);
        else
            LC->FileContext->ParserIndice = 0;

        LC->FileContext->Parser = CreateParser (LC, LC->FileContext->ParserIndice);

        // Open the line database and let it scanz0r the filez0r
        ResetLinesDBs (LC, true);

        SetListTitle (LC);

        // Determine if we should update less often
        LC->LessVisUpdates = AreWeRemote ();

        // Last minute application of certain settings
        ApplySettingsV2 (LC, LC->SettingsV2);

        if (LC->SettingsV2.GetValUint32 ("Maximized") == 1 || LC->SettingsV2.GetValUint32 ("AlwaysMaximized") == 1)
        {

        }
        else
        {
            ShowWindow (LC->Screen->GetHWND(), LSC->ShowCmd);
        }

        // Continually print the status of stuff.
        DWORD CountDown = 3;
        while (true)
        {
            HANDLE WaitEvents[2];

            // For the first 10 updates, redraw the whole screen no matter what
            if (LC->UpdateCount < 10)
            {
                AddSimpleCommand (LC, LCMD_UPDATE_TEXT);
                AddSimpleCommand (LC, LCMD_UPDATE_INFO);
            }

            // On the 4th update, apply our options.
            // This works around something such as doing "command | listxp ?" (stdin piping)
            // and having the window set itself to a weird size and having it think it is maximized
            if (LC->UpdateCount == 4 && Options.size() > 0)
            {
                ApplyOptions (LC, Options);
                ApplySettingsV2 (LC, LC->SettingsV2);
            }

            // On the 5th update, check for updates
            if (LC->UpdateCount == 5)
                CheckForUpdate (LC);

            WaitEvents[0] = LC->Screen->GetInputSignalObject ();
            WaitEvents[1] = LC->CmdQueue.GetSignal ();

            if (CountDown != 0)
                CountDown--;

            if (!LC->FileContext->OneTimeDoneScanning)
                AddSimpleCommand (LC, LCMD_UPDATE_INFO);

            if (!LC->FileContext->DoingSearch  &&  
                LC->FileContext->NormalLinesDB->IsDoneScanning()  &&  
                CountDown == 0)
            {
                WaitForMultipleObjects (2, WaitEvents, FALSE, 1000);
                CountDown = 3;
            }

            LC->UpdateCount++;

            HandleCommands (LC);
            
            // Do some hardware scrolling?
            HandleVScroll (LC);

            if (LC->FileContext->NormalLinesDB->IsDoneScanning() == false)
            {
                LC->UpdateInfo = true;

                if ((LC->FileContext->CurrentLine + LC->Screen->GetMaxY()) >= LC->FileContext->ActiveLinesDB->GetTotalLines())
                    LC->UpdateText = true;
            }

            if (LC->FileContext->NormalLinesDB->IsDoneScanning() == true  &&  LC->FileContext->OneTimeDoneScanning == false)
            {
                LC->FileContext->OneTimeDoneScanning = true;
                LC->UpdateInfo = true;
                LC->UpdateText = true;
            }

            if (LC->UpdateInfo)
            {
                LC->UpdateInfo = false;
                DrawInfoLines (LC);
            }

            if (LC->UpdateText)
            {
                LC->UpdateText = false;
                DrawTextBody (LC, 0, -1);
                UpdateScrollBars (LC);
            }

            while (HandleInput (LC))
            {
                ;
            }

            // Do search handling
            if (LC->FileContext->DoingSearch)
                HandleSearching (LC);

            // If we're done scanning, then we can't be waiting for the scan to complete
            // Only the "Normal" LinesDB actually scans; we don't check HexLinesDB
            if (LC->WaitForScan && LC->FileContext->NormalLinesDB->IsDoneScanning())
                LC->WaitForScan = false;

            // Update whatever was drawn this iteration
            LC->Screen->GoToXY (0, 0);
            LC->Screen->Refresh ();

            // Make sure we don't go past the vertical end of the file ever
            if (LC->FileContext->CurrentLine >= LC->FileContext->ActiveLinesDB->GetTotalLines()  &&  LC->FileContext->ActiveLinesDB->GetTotalLines() > 0)
            {
                LC->FileContext->CurrentLine = LC->FileContext->ActiveLinesDB->GetTotalLines() - 1;
                LC->UpdateText = false;
            }

            // Update the scrollbars and then maybe get some sleep.
            UpdateScrollBars (LC);

            if (LC->UpdateCount > 20 && !LC->FileContext->NormalLinesDB->IsDoneScanning())
            {
                if (LC->LessVisUpdates)
                    Sleep (REMOTESLEEP);
                else
                    Sleep (NORMALSLEEP);
            }

            if (LC->WantToQuit)
                break;
        }
        
        timeEndPeriod (1);

        //SaveSettings (LC); // save settings to the registry
        //LC->SettingsV2.SaveVarsToReg ();
        SaveSettingsV2 (LC);
        LC->Screen->SetWindowProcContext (NULL);

        delete LC->FileContext->Parser;

        if (LC->FileContext->SearchMachine != NULL)
            delete LC->FileContext->SearchMachine;

        delete LC->FileContext->NormalLinesDB;
        delete LC->FileContext->HexLinesDB;
        delete LC->FileContext->File;
        delete LC->FileContext;
        PostMessage (LC->Screen->GetHWND(), WM_REALLY_QUIT, 0, 0);

        ExitThread (0);
    } unguard;
}


DWORD WINAPI ListThread (LPVOID Context)
{
    DWORD ret = 0;

    /*
#ifdef NDEBUG
    try
#endif
    {
#ifdef NDEBUG
        try
#endif
    */
        {
            ret = _ListThread (Context);
        }

    /*
#ifdef NDEBUG
        catch (exception &ex)
        {
            throw;
        }

        catch (...)
        {
            throw exception ("(unknown)");
        }
#endif
    }

#ifdef NDEBUG
    catch (exception &ex)
    {
        MessageBox (NULL, ex.what(), "Unhandled C++ Exception", MB_OK);
        ExitProcess (1);
    }
#endif
    */

    return (ret);
}

