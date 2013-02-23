#include "CSearchRegex.h"
#include "Search.h"
#include "List.h"


CSearchRegex::CSearchRegex (bool MatchCase)
: CSearch (MatchCase)
{
    guard
    {
        memset (&Regex, 0, sizeof (Regex));
        return;
    } unguard;
}


CSearchRegex::~CSearchRegex ()
{
    guard
    {
        regfree (&Regex);
        return;
    } unguard;
}


bool CSearchRegex::CompileSearch (const wchar_t *Pattern)
{
    guard
    {
        int result;
        char *PatternCopy;

        PatternCopy = StrDupW2A (Pattern);

        regfree (&Regex);
        result = regcomp (&Regex, PatternCopy, MatchCase ? 0 : REG_ICASE);

        delete PatternCopy;

        if (result != 0)
        {
            SetLastError ("Could not compile regular expression");
            return (false);
        }

        return (true);
    } unguard;
}


bool CSearchRegex::MatchPattern (const wchar_t *String, const int StringLength, MatchExtentGroup *ExtentResult)
{
    guard
    {
        int regresult;
        bool Result;
        char *ASCIICopy;

        ASCIICopy = StrDupW2A (String);

        regresult = regexec (&Regex, ASCIICopy, 0, NULL, 0);
        delete ASCIICopy;
        Result = (regresult == 0);

        if (Result && ExtentResult != NULL)
        {
            ExtentResult->clear ();
            *ExtentResult = GetMatchExtents (String, StringLength);
        }

        return (Result);
    } unguard;
}


CSearch::MatchExtentGroup CSearchRegex::GetMatchExtents (const wchar_t *String, const int StringLength)
{
    guard
    {
        int result;
        char *ASCIICopy;
        MatchExtent extent;
        MatchExtentGroup ret;
        MatchExtentGroup ind;
        regmatch_t *regmatches;
        int i;

        if (StringLength != 0)
        {
            ind = GetMatchExtents (String + 1, StringLength - 1);
            AdjustMatchGroup (ind);
        }

        ASCIICopy = StrDupW2A (String);

        regmatches = new regmatch_t[1024]; // magic number: we will work with a maximum of 1024 extent matches
        for (i = 0; i < 1024; i++)
        {
            regmatches[i].rm_eo = 0;
            regmatches[i].rm_so = 0;
        }

        result = regexec (&Regex, ASCIICopy, 1024, regmatches, 0);
        delete ASCIICopy;

        for (i = 0; i < 1024; i++)
        {
            extent.first = regmatches[i].rm_so;
            extent.second = regmatches[i].rm_eo - regmatches[i].rm_so;

            if (extent.second != 0)
                ret.push_back (extent);
        }

        delete regmatches;
        MergeMatchGroups (ret, ind);

        return (ret);        
    } unguard;
}
