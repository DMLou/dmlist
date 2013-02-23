/*****************************************************************************

  CSearchLiteral

  Literal pattern matching

*****************************************************************************/


#ifndef _CSEARCHLITERAL_H
#define _CSEARCHLITERAL_H


#include "CSearch.h"


class CSearchLiteral : public CSearch
{
public:
    CSearchLiteral (bool MatchCase);
    ~CSearchLiteral ();

    bool CompileSearch (const wchar_t *Pattern);
    bool MatchPattern (const wchar_t *String, const int StringLength, MatchExtentGroup *ExtentsResult);
    MatchExtentGroup GetMatchExtents (const wchar_t *String, const int StringLength);

protected:
    std::wstring SubString;
};


#endif // _CSEARCHLITERAL_H
