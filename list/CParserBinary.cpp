#include "List.h"
#include "ListMisc.h"
#include "CParserBinary.h"


int IdentifyCParserBinary (uint8 *Data)
{
    guard
    {
        int i;
        int result;

        result = 0;

        for (i = 0; i < 512; i++)
        {
            if (Data[i] > 127)
            {
                result++; // the more high-ASCII we see, the stronger we want to parse this ... 
            }
            else
            {
                result--;
            }
        }

        result = max (0, result); // if less than 0, reset to 0

        // If it starts with "MZ" (executable tag) then we really want binary
        // Also, if it starts with "PK" (pkzip file) then we also really want binary
        // Other signatures are checked for as well
        if (
                (Data[0] == 'M'  &&  Data[1] == 'Z')            ||
                (Data[0] == 'P'  &&  Data[2] == 'K')            ||
                (NULL != search (Data, Data + 10, "PAGEDUMP"))  ||
                (NULL != search (Data, Data + 10, "JFIF"))      ||
                (NULL != search (Data, Data + 10, "GIF8"))      ||
                (NULL != search (Data, Data + 10, "%PDF"))      ||
                (NULL != search (Data, Data + 10, "ID3\x3"))    ||
                (NULL != search (Data, Data + 10, "RIFF"))
            )
        {
            result += 5;
        }

        // clamp result to [0,100]
        result = min (100, result); // max of 100
        result = max (0, result);  // min of 0

        return (result);
    } unguard;
}


CParser2 *CreateCParserBinary (width_t MaxWidth, bool WordWrapping, uint32 TabSize, cFormat DefaultFormat, uint16 UID)
{
    guard
    {
        return ((CParser2 *)new CParserBinary (MaxWidth, WordWrapping, TabSize, DefaultFormat, UID));
    } unguard;
}


CParserBinary::CParserBinary (width_t MaxWidth, bool WordWrapping, uint32 TabSize, cFormat DefaultFormat, uint16 UID)
: CParser2 (MaxWidth, WordWrapping, TabSize, DefaultFormat, UID)
{
    guard
    {
        return;
    } unguard;
}


CParserBinary::~CParserBinary ()
{
    return;
}


bool CParserBinary::UseUnicodeRendering (void)
{
    guard
    {
        return (false);
    } unguard;
}


CParser2 *CParserBinary::Clone (void)
{
    guard
    {
        return ((CParser2 *)CreateCParserBinary (MaxWidth, WordWrapping, TabSize, DefFormat, UID));
    } unguard;
}


void CParserBinary::ProcessByte (uint8 Byte)
{
    guard
    {
        if (Byte == '\0')
            Byte = ' ';

        CurChar.Char = (wchar_t)Byte;

        if (Byte != '\r')
        {
            EmitChar ();

            if (CurWord.Chars > 0)
                EmitWord ();
        }

        return;
    } unguard;
}


