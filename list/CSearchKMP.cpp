#include "CSearchKMP.h"
#include "guard.h"
#include <shlwapi.h>
#include "ListMisc.h"



CSearchKMP::CSearchKMP (bool MatchCase)
: CSearch (MatchCase)
{
    guard
    {
        KMP.j = 0;
        KMP.JumpVector = NULL;
        KMP.Pattern = std::wstring (L"");
        KMP.Result = -1;
        KMPCompile (&KMP, L"blah", 4, MatchCase);
        return;
    } unguard;
}


CSearchKMP::~CSearchKMP ()
{
    guard
    {
        KMPFree (&KMP);
        return;
    } unguard;
}


bool CSearchKMP::CompileSearch (const wchar_t *Pattern)
{
    guard
    {
        KMPFree (&KMP);
        KMPCompile (&KMP, Pattern, wcslen(Pattern), MatchCase);
        return (true);
    } unguard;
}


bool CSearchKMP::MatchPattern (const wchar_t *String, const int StringLength, MatchExtentGroup *ExtentResult)
{
    guard
    {
        bool Result;

        KMPEvaluate (&KMP, String, StringLength);

        Result = (KMP.Result != -1);

        if (Result && ExtentResult != NULL)
        {
            ExtentResult->clear ();
            *ExtentResult = GetMatchExtents (String, StringLength);
        }

        return (Result);
    } unguard;
}


void CSearchKMP::SetMatchCase (bool NewMC)
{
    guard
    {
        wchar_t *copy;

        copy = wcsdup (KMP.Pattern.c_str());
        CompileSearch (copy);
        free (copy);
        CSearch::SetMatchCase (NewMC);
        return;
    } unguard;
}



CSearch::MatchExtentGroup CSearchKMP::GetMatchExtents (const wchar_t *String, const int StringLength)
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
                p = StrStrIW (s, KMP.Pattern.c_str());
            else
                p = StrStrW (s, KMP.Pattern.c_str());

            if (p == NULL)
                s = String + StringLength;
            else
            {
                extents.push_back (make_pair (int(p - String), int(KMP.Pattern.length())));
                s = p + 1;
            }
        }

        return (extents);
    } unguard;
}
