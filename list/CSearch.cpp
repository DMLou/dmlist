#include "CSearch.h"
#include "guard.h"
#include <iterator>
#include <algorithm>


using namespace std;


CSearch::CSearch (bool MatchCase)
{
    guard
    {
        CSearch::MatchCase = MatchCase;
        return;
    } unguard;
}


CSearch::~CSearch ()
{
    guard
    {
        return;
    } unguard;
}


void CSearch::AdjustMatchGroup (MatchExtentGroup &AddOneToMe)
{
    guard
    {
        MatchExtentGroup::iterator it;

        for (it = AddOneToMe.begin(); it != AddOneToMe.end(); ++it)
            ++(it->first);

        return;
    } unguard;
}

// Consolidates sequential, continuous extents within RLEMe
// Example: If it contains the two extents (0,2) and (2,4), they will be consolidated into (0,6)
void CSearch::MatchGroupRLE (MatchExtentGroup &RLEme)
{
    guard
    {
        MatchExtentGroup::iterator r;
        MatchExtentGroup::iterator w;

        r = ++RLEme.begin();
        w = RLEme.begin();

        while (r != RLEme.end())
        {
            if (r->first == w->first + w->second)
                w->second += r->second;
            else
            {
                ++w;
                *w = *r;
            }

            ++r;
        }

        ++w;

        if (w != RLEme.end())
            RLEme.erase (w, RLEme.end());

    } unguard;
}

// Left = Left union Right
// Note: This function is not meant to be a general purpose merger of extents
//       It merely works on the assumption that Left and Right contain extents
//       that always overlap. Our main goal is to eliminate extents from Right
//       that are already in Left
void CSearch::MergeMatchGroups (MatchExtentGroup &Left, const MatchExtentGroup &Right)
{
    guard
    {
        MatchExtentGroup::iterator lit;
        MatchExtentGroup::const_iterator rit;

        lit = Left.begin();
        rit = Right.begin();

        while (lit != Left.end()  &&  rit != Right.end())
        {
            int lbegin;
            int lend;
            int rbegin;
            int rend;

            lbegin = lit->first;
            lend = lbegin + lit->second;
            rbegin = rit->first;
            rend = rbegin + rit->second;

            // l-----------------l
            // ----r--------r----- => throw away r
            if (rbegin >= lbegin  &&  rend <= lend)
            {
                ++rit;
            }
            else
            // l-------------------l
            // ------r-----------------r  => lend = rend
            if (rbegin >= lbegin  &&  
                rbegin <= lend     && 
                rend > lend)
            {
                lend = rend;
                ++rit;
                lit->second = lend - lbegin;
            }
            else
            // l--------------------l
            // -----------------------r------r  => go to next l, repeat loop
            if (rbegin > lbegin)
            {
                ++lit;
            }
            else
            {
                ++lit;
                ++rit;
            }
        }

        if (rit != Right.end())
            copy (rit, Right.end(), back_inserter (Left));

//        copy (Right.begin(), Right.end(), back_inserter (Left));

    } unguard;
}
