#include <windows.h>
#include <string>
#include <new>
#include "ListMisc.h"


char *StrDupW2A (const wchar_t *Src)
{
    guard
    {
        char *Return;
        char *d;
        wchar_t *s;

        Return = new char[wcslen (Src) + 1];
        s = (wchar_t *)Src;
        d = Return;

        while (*s != '\0')
        {
            *d = (char) *s;
            d++;
            s++;
        }

        *d = '\0';
        return (Return);
    } unguard;
}


wchar_t *StrDupA2W (const char *Src)
{
    guard
    {
        wchar_t *Return;
        wchar_t *d;
        char *s;

        Return = new wchar_t[strlen (Src) + 1];
        s = (char *)Src;
        d = Return;

        while (*s != '\0')
        {
            *d = (wchar_t) *s;
            d++;
            s++;
        }

        *d = L'\0';
        return (Return);
    } unguard;
}


// Given a 64-bit integer representing a quantity in bytes, this will
// decide whether you should print a number in bytes, kilobytes, megabytes,
// or gigabytes
const char *GetScaleName (const uint64 Bytes,
                          const char *BytesName,
                          const char *KBName,
                          const char *MBName,
                          const char *GBName) // what name to use: "bytes", "KB", etc
{
    guard
    {
        if (Bytes > 1024)
            return (KBName);

        if (Bytes > 1024 * 1024)
            return (MBName);

        if (Bytes > 1024 * 1024 * 1024)
            return (GBName);

        return (BytesName);
    } unguard;
}


int GetScaleDiv (const uint64 Bytes) // what to divide by
{
    guard
    {
        if (Bytes > 1024)
            return (1024);

        if (Bytes > 1024 * 1024)
            return (1024 * 1024);

        if (Bytes > 1024 * 1024 * 1024)
            return (1024 * 1024 * 1024);

        return (1);
    } unguard;
}


uint64 GetTime64 (void)
{
    guard
    {
        SYSTEMTIME SLocalTime;
        FILETIME FLocalTime;

        ZeroMemory (&SLocalTime, sizeof (SLocalTime));
        ZeroMemory (&FLocalTime, sizeof (FLocalTime));
        GetSystemTime (&SLocalTime);
        SystemTimeToFileTime (&SLocalTime, &FLocalTime);
        
        return (uint64(FLocalTime.dwLowDateTime) + (uint64(FLocalTime.dwHighDateTime) << 32));
    } unguard;
}


// Returns an integer telling the difference between two times, in milliseconds
sint64 TimeDiffMS (uint64 lhs, uint64 rhs)
{
    guard
    {
        sint64 diff;

        diff = (sint64)lhs - (sint64)rhs;
        diff /= 10000;

        return (diff);
    } unguard;
}


sint64 TimeDiffDays (uint64 &lhs, uint64 &rhs)
{
    guard
    {
        return (TimeDiffMS (lhs, rhs) / (1000 * 60 * 60 * 24));
    } unguard;
}


// Takes a string and separates ?'d parameters from the target
// i.e. the string:
//     access.log?tailing=1?height=50?width=80
// is separated into two components. The first is the string "access.log"
// The second is a vector of pair<string,string> such that:
// [0]=<"tailing","1">, [1]=<"height","50">, [2]=<"width", "80">
// From here you can parse the options as necessary.
// Note, it is perfectly legal to start immediately with a question mark,
// and in this case the initial string will simply be blank.


// Given a string of the form:
// tag=value
// Returns a pair of strings corresponding to tag and value
// i.e.
// retvalue.first = tag
// retvalue.second = value
// Otherwise returns blank strings.
Option ParseForOption (string::const_iterator begin, string::const_iterator end)
{
    guard
    {
        Option ret;
        string::const_iterator eq;

        eq = find (begin, end, '=');

        if (eq != end)
        {   // We found an equal sign!
            ret.first.assign (begin, eq);
            ret.second.assign (eq + 1, end);
        }

        return (ret);
    } unguard;
}

// Parses option tags out of a list.
// Grammar is:
//     t = ?tag=value
//     l = eps | t l
// So, given a string of the form:
//     ?tag0=value0[?tag1=value1[?tag2=value2...]]
// Returns a vector such as:
//     retvalue = { (tag0,value0), (tag1,value1), (tag2,value2) }
OptionList ParseForOptions (string::const_iterator begin, string::const_iterator end)
{
    guard
    {
        OptionList ret;

        if ((end - begin) > 0)
        {
            if (*begin == '?')
            {
                string::const_iterator q;
                Option op;

                q = find (begin + 1, end, '?');
                ret.push_back (ParseForOption (begin + 1, q));
                OptionList inductive (ParseForOptions (q, end));
                copy (inductive.begin(), inductive.end(), back_inserter (ret));
            }
        }

        return (ret);
    } unguard;        
}

OptionsAndTag ParseForOptionsAndTag (const string &Source)
{
    guard
    {
        OptionsAndTag ret;
        string::const_iterator it;

        // Empty string?
        if (Source.length() != 0)
        {   // No. Proceed.
            // Axiom: Source.length() >= 1
            // Acquire the first source string by searching for the first question mark
            it = find (Source.begin(), Source.end(), '?');

            ret.first.assign (Source.begin(), it);

            if (it != Source.begin())
                ret.second = ParseForOptions (it, Source.end());
        }

        return (ret);
    } unguard;
}

// Takes an option list of the form:
//    list = { (tag0,val0), (tag1,val1), (tag2,val2), ..., (tagN,valN) }
// And returns a string of the form:
//    ?tag0=val0?tag1=val1?tag2=val2?...?tagN=valN
string OptionListToString (const OptionList &list)
{
    guard
    {
        string ret ("");

        for (OptionList::const_iterator it = list.begin(); it != list.end(); ++it)
        {
            ret.push_back ('?');
            ret.append (it->first);
            ret.push_back ('=');
            ret.append (it->second);
        }

        return (ret);
    } unguard;
}


uint8 *search (uint8 *begin, uint8 *end, char *str)
{
    uint8 *sbeg;
    uint8 *send;
    uint8 *ret;

    sbeg = (uint8 *)str;
    send = (uint8 *)(str + strlen(str));
    ret = search (begin, end, sbeg, send);

    if (ret == end)
        ret = NULL;

    return ((uint8 *)ret);
}
