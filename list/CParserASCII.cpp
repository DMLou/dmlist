#include "List.h"
#include "CParserASCII.h"

int IdentifyCParserASCII (uint8 *Data)
{
    guard
    {
        int ret = 1;
        uint8 *DataEnd = Data + 512;

        // If we find newlines, then we really want to parse as ASCII
        uint8 findme1[2];
        uint8 findme2[2];

        findme1[0] = 0xd;
        findme1[1] = 0xa;

        findme2[0] = 0xa;
        findme2[1] = 0xd;

        // If we find a \n\r or \r\n pair, increase our affinity for this parser by 4
        if (NULL != search (Data, DataEnd, findme1, findme1 + 2)  ||
            NULL != search (Data, DataEnd, findme2, findme2 + 2))
        {
            ret += 3;
        }
        else
        // If we find just a \n or \r, then increase our affinity by just 2
        if (DataEnd != find (Data, DataEnd, char(0xd))  ||
            DataEnd != find (Data, DataEnd, char(0xa)))
        {
            ret += 2;
        }

        return (ret);
    } unguard;
}

CParser2 *CreateCParserASCII (width_t MaxWidth, bool WordWrapping, uint32 TabSize, cFormat DefaultFormat, uint16 UID)
{
    guard
    {
        return ((CParser2 *)new CParserASCII (MaxWidth, WordWrapping, TabSize, DefaultFormat, UID));
    } unguard;
}

CParserASCII::CParserASCII (width_t MaxWidth, bool WordWrapping, uint32 TabSize, cFormat DefaultFormat, uint16 UID)
: CParser2 (MaxWidth, WordWrapping, TabSize, DefaultFormat, UID)
{
    guard
    {
        return;
    } unguard;
}

CParserASCII::~CParserASCII ()
{
    guard
    {
        return;
    } unguard;
}

void CParserASCII::ProcessByte (uint8 Byte)
{
    guard
    {
        if (Byte == '\0')
            return;

        if (Byte == '\t')
            EmitTab ();
        else
        {
            CurChar.Char = (wchar_t)Byte;

            if (Byte != '\r')
                EmitChar ();
        }

        return;
    } unguard;
}

bool CParserASCII::UseUnicodeRendering (void)
{
    guard
    {
        return (false);
    } unguard;
}

CParser2 *CParserASCII::Clone (void)
{
    guard
    {
        return ((CParser2 *)CreateCParserASCII (MaxWidth, WordWrapping, TabSize, DefFormat, UID));
    } unguard;
}

