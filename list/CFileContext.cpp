#include "List.h"
#include "CFileContext.h"


CFileContext::CFileContext ()
{
    guard
    {
        CurrentLine = 0;
        CurrentColumn = 0;
        Highlight = false;
        HighlightLine = 0;
        OneTimeDoneScanning = false;
        HaveDoneSearchBefore = false;
        SearchSubstring = "";
        NextLineToSearch = 0;
        SearchDirection = +1;
        SearchCaseSensitive = false;
        SearchMachine = NULL;
        DoingSearch = false;
        SearchType = SEARCHTYPE_LITERAL;
        HexMode = false;
        ActiveLinesDB = NULL;
        NormalLinesDB = NULL;
        HexLinesDB = NULL;
        Parser = NULL;
        File = NULL;
        Tailing = false;
//        DoFilter = false;
//        FilterStream = NULL;
        return;
    } unguard;
}


CFileContext::~CFileContext ()
{
    guard
    {
        return;
    } unguard;
}

