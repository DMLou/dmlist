/*****************************************************************************

    Routines and classes for handling string searching / pattern matching

*****************************************************************************/


#ifndef _SEARCH_H
#define _SEARCH_H


// Normal "C-style" search functions
wchar_t *StringSearch    (const wchar_t *String, int StrLen, const wchar_t *SubString, int SubLen);
wchar_t *StringSearchI   (const wchar_t *String, int StrLen, const wchar_t *SubString, int SubLen);


// KMP search functions
class KMPContext
{
public:
    std::wstring Pattern;
    int *JumpVector;  // This will be allocated by KMPCompile and freed by KMPFree. 
                      // Don't allocate or free this yourself!
    int j;            // Used while performing the actual search
    int Result;       // This is the result after doing a KMPEvaluate or KMPEvaluateMany
                      // -1 means "not found", any other number indicates array indice
                      // into the given search string that the string occurred at
    wchar_t *Table;   // table to character translation table (IdentityTable or ToLowerTable)
    bool MatchCase;   // stores whether the KMP machine was compiled for case sensitivity or not
};


bool KMPCompile      (KMPContext *Result, const wchar_t *Pattern, const int Length, bool MatchCase);
void KMPFree         (KMPContext *FreeMe);
void KMPEvaluate     (KMPContext *Context, const wchar_t *String, const int Length);
void KMPEvaluateMany (KMPContext **Contexts, int KMPCount, const wchar_t *String, const int Length);


#endif // _SEARCH_H


