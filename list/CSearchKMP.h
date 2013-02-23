/*****************************************************************************

  CSearchKMP

  Literal pattern matching using the KMP algorithm

*****************************************************************************/


#ifndef _CSEARCHKMP_H
#define _CSEARCHKMP_H


#include "CSearch.h"
#include "Search.h"

class CSearchKMP : public CSearch
{
public:
    CSearchKMP (bool MatchCase);
    ~CSearchKMP ();

    bool CompileSearch (const wchar_t *Pattern);
    bool MatchPattern (const wchar_t *String, const int StringLength, MatchExtentGroup *ExtentsResult);
    void SetMatchCase (bool NewMC);
    MatchExtentGroup GetMatchExtents (const wchar_t *String, const int StringLength);

protected:
    KMPContext KMP;
};


#endif // _CSEARCHKMP_H
