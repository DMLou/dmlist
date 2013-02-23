/*****************************************************************************

  CSearchRegex

  Regular expression pattern matching

*****************************************************************************/


#ifndef _CSEARCHREGEX_H
#define _CSEARCHREGEX_H


#include "CSearch.h"
#include "../libs/pcre/src/pcre.h"
#include "../libs/pcre/src/pcreposix.h"


class CSearchRegex : public CSearch
{
public:
    CSearchRegex (bool MatchCase);
    ~CSearchRegex ();

    bool CompileSearch (const wchar_t *Pattern);
    bool MatchPattern (const wchar_t *String, const int StringLength, MatchExtentGroup *ExtentsResult);
    MatchExtentGroup GetMatchExtents (const wchar_t *String, const int StringLength);

protected:
    regex_t Regex;
};


#endif // _CSEARCHREGEX_H

