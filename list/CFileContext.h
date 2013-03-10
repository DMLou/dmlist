/*****************************************************************************

    CFileContext

    Maintains context for the currently open file.

*****************************************************************************/


#ifndef _CFILECONTEXT_H
#define _CFILECONTEXT_H


#include <map>
#include "DynamicHeap.h"


// Keeps track of which characters on the line are especially marked
class CLineMarking : public std::vector<bool>
{
public:
    void Mark (int Pos)
    {
        guard
        {
            if (int(size()) <= Pos)
                resize (Pos + 1);

            (*this)[Pos] = true;
            return;
        } unguard;
    }

    void Unmark (int Pos)
    {
        guard
        {
            if (int(size()) <= Pos)
                resize (Pos + 1);

            (*this)[Pos] = false;
            return;
        } unguard;
    }

    bool IsMarked (int Pos)
    {
        guard
        {
            if (int(size()) <= Pos)
                return (false);

            return ((*this)[Pos]);
        } unguard;
    }
};


// Keeps track of which lines are marked, and then for each line stores a CLineMarking instance
class CMarkedLines : public std::map<uint64, CLineMarking>
{
public:
    void AddLine (uint64 NewLine, CLineMarking &NewData)
    {
        guard
        {
            insert(std::pair<uint64, CLineMarking>(NewLine, NewData));
        } unguard;
    }

    void RemoveLine (uint64 OldLine)
    {
        guard
        {
            erase(OldLine);
        } unguard;
    }

    bool IsLinePresent (uint64 Line)
    {
        guard
        {
            iterator found = find(Line);
            return (found != end());
        } unguard;
    }

    CLineMarking *GetLineBitmap (uint64 Line)
    {
        guard
        {
            iterator found = find(Line);
            if (found == end())
            {
                return NULL;
            }
            else
            {
                return &(found->second);
            }
        } unguard;
    }

    bool MarkRange (uint64 Line, int PosFirst, int PosSecond)
    {
        guard
        {
            CLineMarking *Marker;
            int i;

            Marker = GetLineBitmap (Line);

            if (Marker == NULL)
                return (false);

            for (i = PosFirst; i <= PosSecond; i++)
                Marker->Mark (i);

            return (true);
        } unguard;
    }

    bool UnmarkRange (uint64 Line, int PosFirst, int PosSecond)
    {
        guard
        {
            CLineMarking *Marker;
            int i;

            Marker = GetLineBitmap (Line);

            if (Marker == NULL)
                return (false);

            for (i = PosFirst; i <= PosSecond; i++)
                Marker->Unmark (i);

            return (true);
        } unguard;
    }

private:

};


// The current context for a single file that is being viewed
// Includes settings that can be changed per open file but
// are still saved in the registry if only one file is open
// (This is so you can "open" multiple files at once ...)
class CFileContext
{
public:
    CFileContext ();
    virtual ~CFileContext ();

    // User bookmarks/highlighting: you can only do a full highlight of any given line
    bool UserIsLineMarked (uint64 Line)
    {
        guard
        {
            return (MarkedLines.IsLinePresent (Line));
        } unguard;
    }

    void UserMarkLine (uint64 Line)
    {
        guard
        {
            MarkedLines.AddLine (Line, CLineMarking());
            return;
        } unguard;
    }

    void UserUnmarkLine (uint64 Line)
    {
        guard
        {
            MarkedLines.RemoveLine (Line);
            return;
        } unguard;
    }

    void UserToggleMark (uint64 Line)
    {
        guard
        {
            if (UserIsLineMarked(Line))
                UserUnmarkLine (Line);
            else
                UserMarkLine (Line);

            return;
        } unguard;
    }

private:
    CMarkedLines MarkedLines;

public: 
    bool WrapText;

    string FileName;
    CFastFile *File;
    CHexLinesDB *HexLinesDB;
    CFileLinesDB *NormalLinesDB;
    CLinesDB *ActiveLinesDB; // the one that is currently being displayed
    string PrintFileName;
    int ParserIndice;
    CParser2 *Parser;

    bool Tailing;
    bool HexMode;

    __declspec(align(8)) uint64 MaxWidth;
    __declspec(align(8)) uint64 CurrentLine; // which line is at the top of the screen?
    __declspec(align(8)) uint64 CurrentColumn;
    bool   Highlight;
    uint64 HighlightLine;
    CSearch::MatchExtentGroup HighlightSecondary; // chars within the highlight to do a secondary highlight on
    bool OneTimeDoneScanning; // update title once more after done scanning

    // Searching functions
    bool DoingSearch;
    bool HaveDoneSearchBefore;
    int SearchType;               // one of the SEARCHTYPE_* #defines listed above
    CSearch *SearchMachine;
    string SearchSubstring;       // the last pattern compiled
    uint64 NextLineToSearch;      // if this is set to NormalLinesDB->GetTotalLines(), then the search has gone past the end of the file
                                  // Note: it is set to 0 when SearchDirection==-1
    int SearchDirection; // +1 for down, -1 for up
    bool SearchCaseSensitive;
};


#endif // _CFILECONTEXT_H