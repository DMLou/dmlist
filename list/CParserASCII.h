/*****************************************************************************

  CParserASCII

  ASCII text parser implementation of CParser class. The difference between
  this and CParserBinary is that this class expands tab characters and
  ignore \r characters.

*****************************************************************************/


#ifndef _CPARSERASCII_H
#define _CPARSERASCII_H


#include "CParser2.h"


extern int IdentifyCParserASCII (uint8 *Data);


class CParserASCII : public CParser2
{
protected:
    CParserASCII (width_t MaxWidth, 
                  bool WordWrapping, 
                  uint32 TabSize, 
                  cFormat DefaultFormat, 
                  uint16 UID);

    extern friend CParser2 *CreateCParserASCII (width_t MaxWidth, 
                                                bool WordWrapping,
                                                uint32 TabSize, 
                                                cFormat DefaultFormat, 
                                                uint16 UID);

public:
    ~CParserASCII ();

    bool UseUnicodeRendering (void);
    void ProcessByte (uint8 Byte);
    CParser2 *Clone (void);

private:

};


#endif // _CPARSERASCII_H

