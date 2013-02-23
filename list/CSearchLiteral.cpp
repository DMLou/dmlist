#include "CSearchLiteral.h"
#include "ListMisc.h"
#include "Search.h"
#include "List.h"
#include <shlwapi.h>


CSearchLiteral::CSearchLiteral (bool MatchCase)
: CSearch (MatchCase)
{
    guard
    {
        return;
    } unguard;
}


CSearchLiteral::~CSearchLiteral ()
{
    guard
    {
        return;
    } unguard;
}


bool CSearchLiteral::CompileSearch (const wchar_t *Pattern)
{
    guard
    {
        SubString = std::wstring (Pattern);
        return (true);
    } unguard;
}


bool CSearchLiteral::MatchPattern (const wchar_t *String, const int StringLength, MatchExtentGroup *ExtentResult)
{
    guard
    {
        bool Result;

        if (!MatchCase)
            Result = StringSearchI (String, StringLength, SubString.c_str(), SubString.length());
        else
            Result = StringSearch (String, StringLength, SubString.c_str(), SubString.length());

        if (Result && ExtentResult != NULL)
        {
            ExtentResult->clear ();
            *ExtentResult = GetMatchExtents (String, StringLength);
        }

        return (Result);
    } unguard;
}


CSearch::MatchExtentGroup CSearchLiteral::GetMatchExtents (const wchar_t *String, const int StringLength)
{
    guard
    {
        MatchExtentGroup extents;
        int matchindice;
        const wchar_t *p;
        const wchar_t *s;

        s = String;

        while (s < String + StringLength)
        {
            if (!MatchCase)
                p = StrStrIW (s, SubString.c_str());
            else
                p = StrStrW (s, SubString.c_str());

            if (p == NULL)
                s = String + StringLength;
            else
            {
                extents.push_back (make_pair (int(p - String), int(SubString.length())));
                s = p + 1;
            }
        }

        return (extents);
    } unguard;
}
