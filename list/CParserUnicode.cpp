#include "List.h"
#include "CParserUnicode.h"


int IdentifyCParserUnicode (uint8 *Data)
{
    guard
    {
        int Rating = 0;

        // I've noticed that Unicode text files start with the word ff fe (0xfeff in little-endian)
        if (Data[0] == 0xff  &&  Data[1] == 0xfe)
        {
            Rating = 3;

            // And we are more sure if every other byte is a 0, although we only check the first couple.
            if (Data[3] == 0  &&  Data[5] == 0  &&  Data[7] == 0  &&  Data[9] == 0)
                Rating += 3;
        }

        return (Rating);
    } unguard;
}


CParser2 *CreateCParserUnicode (width_t MaxWidth, bool WordWrapping, uint32 TabSize, cFormat DefaultFormat, uint16 UID)
{
    guard
    {
        return ((CParser2 *)new CParserUnicode (MaxWidth, WordWrapping, TabSize, DefaultFormat, UID));
    } unguard;
}



CParserUnicode::CParserUnicode (width_t MaxWidth, bool WordWrapping, uint32 TabSize, cFormat DefaultFormat, uint16 UID)
: CParser2 (MaxWidth, WordWrapping, TabSize, DefaultFormat, UID)
{
    guard
    {
        Offset = 0;
        return;
    } unguard;
}


CParserUnicode::~CParserUnicode ()
{
    guard
    {
        return;
    } unguard;
}


bool CParserUnicode::UseUnicodeRendering (void)
{
    guard
    {
        return (true);
    } unguard;
}


CParser2 *CParserUnicode::Clone (void)
{
    guard
    {
        return ((CParser2 *)CreateCParserUnicode (MaxWidth, WordWrapping, TabSize, DefFormat, UID));
    } unguard;
}


void CParserUnicode::ProcessByte (uint8 Byte)
{
    guard
    {
        if ((Offset & 1) == 0)
            Low = Byte;
        else
        {
            High = Byte;

            CurChar.Char = (wchar_t)Low + ((wchar_t)High << 8);

            if (CurChar.Char == (wchar_t)0xfeff || CurChar.Char == L'\0')
                ; // ignore zeros, just like in CParserASCII. also ignore the 0xfeff char
            else
            if (CurChar.Char == L'\t')
                EmitTab ();
            else
            {
                //CurChar.Char = (wchar_t)Byte;

                if (CurChar.Char != L'\r')
                    EmitChar ();
            }
        }

        Offset++;
        return;
    } unguard;
}


void CParserUnicode::Reset (bool StartDependent)
{
    guard
    {
        Offset = 0;
        CParser2::Reset (StartDependent);
        return;
    } unguard;
}
