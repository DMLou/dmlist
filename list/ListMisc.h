/*****************************************************************************

  ListMisc.h

  Miscellaneous utility functions

*****************************************************************************/


#ifndef _LISTMISC_H
#define _LISTMISC_H


#include "ListTypes.h"


template<class IT> 
static IT IntLog2 (IT Int)
{
    guard
    {
        IT Log;
        
        Log = -1;

        if (Int == 0)
            return (0);

        while (Int > 0)
        {
            Log++;
            Int >>= 1;
        }

        return (Log);
    } unguard;
}

extern char *StrDupW2A (const wchar_t *Src);
extern wchar_t *StrDupA2W (const char *Src);


// Given a 64-bit integer representing a quantity in bytes, this will
// decide whether you should print a number in bytes, kilobytes, megabytes,
// or gigabytes
const char *GetScaleName (const uint64 Bytes,
                          const char *BytesName = "bytes",
                          const char *KBName = "KB",
                          const char *MBName = "MB",
                          const char *GBName = "GB"); // what name to use: "bytes", "KB", etc

int GetScaleDiv  (const uint64 Bytes); // what to divide by


// Returns the string representation of any type that supports conversion via the << operator
template<class FromType>
string tostring (const FromType &from)
{
    guard
    {
        strstream stream;
        string result;

        stream << from;
        result.resize (stream.pcount());
        copy (stream.str(), stream.str() + stream.pcount(), result.begin());

        return (result);
    } unguard;
}


// Typecast functor: provides a function object that converts from one type to another
template<class FromType, class ToType>
class typecast_fn : public unary_function <FromType, ToType>
{
public:
    ToType operator() (const FromType &from) const
    {
        guard
        {
            return (ToType (from));
        } unguard;
    }
};


template<class ToType, class FromType>
ToType typecast (const FromType &from)
{
    guard
    {
        return (ToType (from));
    } unguard;
}


// Specialization of search<> for container objects
template<class CType>
typename CType::const_iterator search (const CType &searchme, const CType &matchme)
{
    guard
    {
        return (search (searchme.begin(), searchme.end(), matchme.begin(), matchme.end()));
    } unguard;
}


// Case-insensitive search<> using towlower
template<class CType>
typename CType::const_iterator searchi (const CType &searchme, const CType &matchme)
{
    CType copysearchme;
    CType copymatchme;

    transform (searchme.begin(), searchme.end(), back_inserter(copysearchme), towlower);
    transform (matchme.begin(), matchme.end(), back_inserter(copymatchme), towlower);

    return (searchme.begin() + (search (copysearchme, copymatchme) - copysearchme.begin()));
}

//
extern uint8 *search (uint8 *begin, uint8 *end, char *str);


// Returns a 64-bit integer represting the number of 100-nanosecond intervals since January 1, 1601 (UTC).
extern uint64 GetTime64 (void);
extern sint64 TimeDiffSec (uint64 lhs, uint64 rhs);
extern sint64 TimeDiffDays (uint64 &lhs, uint64 &rhs);


// Function for taking a filename+arglist and parsing it out
// Takes a string and separates ?'d parameters from the target
// i.e. the string:
//     access.log?tailing=1?height=50?width=80
// is separated into two components. The first is the string "access.log"
// The second is a vector of pair<string,string> such that:
// [0]=<"tailing","1">, [1]=<"height","50">, [2]=<"width", "80">
// From here you can parse the options as necessary.
// Note, it is perfectly legal to start immediately with a question mark,
// and in this case the initial string will simply be blank.
typedef pair<string,string> Option; // example, { "width", "80" }
typedef vector<Option> OptionList;  // example, [ { "width", "80" }, { "height", "50" }, ... ]
typedef pair<string,OptionList> OptionsAndTag;

extern OptionList ParseForOptions (string::const_iterator begin, string::const_iterator end);
extern OptionsAndTag ParseForOptionsAndTag (const string &Source);
extern string OptionListToString (const OptionList &list);


#endif // _LISTMISC_H