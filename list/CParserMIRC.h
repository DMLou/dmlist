/*****************************************************************************

  CParserMIRC

  MIRC log files text parser implementation of CParser class

*****************************************************************************/


#ifndef _CPARSERMIRC_H
#define _CPARSERMIRC_H


#include "CParser2.h"


extern int IdentifyCParserMIRC (uint8 *Data);


class CParserMIRC : public CParser2
{
protected:
    CParserMIRC (width_t MaxWidth, 
                 bool WordWrapping, 
                 uint32 TabSize, 
                 cFormat DefaultFormat, 
                 uint16 UID);

    extern friend CParser2 *CreateCParserMIRC (width_t MaxWidth, 
                                               bool WordWrapping,
                                               uint32 TabSize, 
                                               cFormat DefaultFormat, 
                                               uint16 UID);

public:
    ~CParserMIRC ();

    bool UseUnicodeRendering (void);
    void ProcessByte (uint8 Byte);
    CParser2 *Clone (void);
    void Reset (bool StartDependent);

private:
    cFormat ProcessFormat (cFormat In); // uses the following flags to "process" the given color
    COLORREF GetColor (int Indice);

    cText TempChar; // used by FeedByte

    bool UseBold;
    bool UseColor;
    bool UseReverse;
    bool UseUnderline; // not currently supported by Console class, but we parse it anyway
    cFormat CurrentColor;
    int State; // FA state

    int d1a, d1b;
    int d2a, d2b;

    COLORREF ColorTable[16];
};


#endif // _CPARSERMIRC_H

