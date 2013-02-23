/*****************************************************************************

  CParserUnicode

  Unicode text parser implementation of CParser class
  To do: remove inheritance to CParserASCII in order to properly handle
  Unicode text.

*****************************************************************************/


#ifndef _CPARSERUNICODE_H
#define _CPARSERUNICODE_H


//#include "CParser.h"
#include "CParserASCII.h"


extern int IdentifyCParserUnicode (uint8 *Data);


class CParserUnicode : public CParser2
{
protected:
    CParserUnicode (width_t MaxWidth, 
                    bool WordWrapping, 
                    uint32 TabSize, 
                    cFormat DefaultFormat, 
                    uint16 UID);

    extern friend CParser2 *CreateCParserUnicode (width_t MaxWidth, 
                                                  bool WordWrapping,
                                                  uint32 TabSize, 
                                                  cFormat DefaultFormat, 
                                                  uint16 UID);

public:
    ~CParserUnicode ();

    bool UseUnicodeRendering (void);
    void ProcessByte (uint8 Byte);
    void Reset (bool StartDependent);
    CParser2 *Clone (void);

private:
    uint8 Low;
    uint8 High;
    int Offset;
};


#endif // _CPARSERUNICODE_H

