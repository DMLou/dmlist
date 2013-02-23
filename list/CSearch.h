/*****************************************************************************

  CSearch

  Class interface for handling pattern matching
  All strings searched are wide-strings, although error reporting is done
  via normal strings.

*****************************************************************************/


#ifndef _CSEARCH_H
#define _CSEARCH_H


class CSearch;
#include <string>
#include <utility>
#include <vector>
#include "guard.h"
//#include "List.h"


class CSearch
{
public:
    CSearch (bool MatchCase);
    virtual ~CSearch ();

    typedef std::pair<int,int> MatchExtent; // first # is start char, second # is char count (length)
    typedef std::vector<MatchExtent> MatchExtentGroup;

    // Compiles the given pattern into the search machine for further matching with MatchPattern
    virtual bool CompileSearch (const wchar_t *Pattern) = NULL;

    // Matches the pattern (compiled with CompileSearch) against String
    // If ExtentsGroup is non-NULL then the list of matching character extents is placed into it.
    virtual bool MatchPattern (const wchar_t *String, const int StringLength, MatchExtentGroup *ExtentsResult = NULL) = NULL;

    // This will return a vector of MatchExtents that tell you which parts of the line of
    // text match; this is so you can maybe highlight the terms on screen that match
    // Not required to return anything, though, even if the line does match your pattern
    virtual MatchExtentGroup GetMatchExtents (const wchar_t *String, const int StringLength) = NULL;
   
    void MatchGroupRLE (MatchExtentGroup &RLEme);

    virtual void SetMatchCase (bool NewMC)
    {
        guard
        {
            MatchCase = NewMC;
            return;
        } unguard;
    }

    virtual bool GetMatchCase (void)
    {
        guard
        {
            return (MatchCase);
        } unguard;
    }

    std::string GetLastError (void)
    {
        guard
        {
            return (LastError);
        } unguard;
    }

protected:
    // AddOneToMe[i].second++, for all i
    void AdjustMatchGroup (MatchExtentGroup &AddOneToMe);

    // Left = Left union Right. Assumes that Right has already been *adjusted*
    void MergeMatchGroups (MatchExtentGroup &Left, const MatchExtentGroup &Right);

    void SetLastError (std::string NewError)
    {
        guard
        {
            LastError = NewError;
            return;
        } unguard;
    }

    std::string LastError;
    bool MatchCase;
};


#endif // _CSEARCH_H
