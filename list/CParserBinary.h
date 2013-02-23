/*****************************************************************************

  CParserBinary

  Gives a more literal display of a file as opposed to CParserASCII.

*****************************************************************************/


#ifndef _CPARSERBINARY_H
#define _CPARSERBINARY_H


#include "CParser2.h"


extern int IdentifyCParserBinary (uint8 *Data);


class CParserBinary : public CParser2
{
protected:
    CParserBinary (width_t MaxWidth, 
                   bool WordWrapping, 
                   uint32 TabSize, 
                   cFormat DefaultFormat, 
                   uint16 UID);

    extern friend CParser2 *CreateCParserBinary (width_t MaxWidth, 
                                                 bool WordWrapping,
                                                 uint32 TabSize, 
                                                 cFormat DefaultFormat, 
                                                 uint16 UID);

public:
    ~CParserBinary ();

    bool UseUnicodeRendering (void);
    void ProcessByte (uint8 Byte);
    CParser2 *Clone (void);

private:

};


#endif // _CPARSERASCII_H

