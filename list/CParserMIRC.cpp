#include "List.h"
#include "CParserMIRC.h"


int IdentifyCParserMIRC (uint8 *Data)
{
    guard
    {
        uint8 *Copy;
        int Rating = 0;

        Copy = new uint8[513];
        StreamCopy (Copy, Data, 512);
        Copy[512] = 0;

        if (strstr ((char *)Copy, "Session Start: ") != NULL  ||
            strstr ((char *)Copy, "Now talking in ") != NULL)
        {
            Rating = 7;
        }

        delete Copy;

        // Also, we consider it a mIRC file if it starts with ^C followed by a # (as per JLsoft's request)
        if (Data[0] == 3  &&  isdigit(Data[1]))
            Rating += 3;

        return (Rating);
    } unguard;
}


CParser2 *CreateCParserMIRC (width_t MaxWidth, bool WordWrapping, uint32 TabSize, cFormat DefaultFormat, uint16 UID)
{
    guard
    {
        return ((CParser2 *)new CParserMIRC (MaxWidth, WordWrapping, TabSize, DefaultFormat, UID));
    } unguard;
}



CParserMIRC::CParserMIRC (width_t MaxWidth, bool WordWrapping, uint32 TabSize, cFormat DefaultFormat, uint16 UID)
: CParser2 (MaxWidth, WordWrapping, TabSize, DefaultFormat, UID)
{
    guard
    {
        CurrentColor = DefFormat;

        State = 0;

        UseBold = false;
        UseColor = false;
        UseReverse = false;
        UseUnderline = false;

        ColorTable[0]  = 0xffffff;
        ColorTable[1]  = 0x000000;
        ColorTable[2]  = 0x7f0000;
        ColorTable[3]  = 0x009300;
        ColorTable[4]  = 0x0000ff;
        ColorTable[5]  = 0x00007f;
        ColorTable[6]  = 0x9c009c;
        ColorTable[7]  = 0x007ffc;
        ColorTable[8]  = 0x00ffff;
        ColorTable[9]  = 0x00fc00;
        ColorTable[10] = 0x939300;
        ColorTable[11] = 0xffff00;
        ColorTable[12] = 0xfc0000;
        ColorTable[13] = 0xff00ff;
        ColorTable[14] = 0x7f7f7f;
        ColorTable[15] = 0xd2d2d2;

        return;
    } unguard;
}


CParserMIRC::~CParserMIRC ()
{
    guard
    {
        return;
    } unguard;
}


bool CParserMIRC::UseUnicodeRendering (void)
{
    guard
    {
        return (false);
    } unguard;
}


CParser2 *CParserMIRC::Clone (void)
{
    guard
    {
        return (CreateCParserMIRC (MaxWidth, WordWrapping, TabSize, DefFormat, UID));
    } unguard;
}


void CParserMIRC::ProcessByte (uint8 Byte)
{
    guard
    {
        // We use a Deterministic Finite Automaton to parse color control codes
        switch (State)
        {
            case 0:
                switch (Byte)
                {
                    // mIRC Control codes
                    case 2: // bold, Ctrl+B
                        UseBold = !UseBold;
                        ChangeFormat (ProcessFormat (CurrentColor));
                        break;

                    case 22:
                        UseReverse = !UseReverse;
                        ChangeFormat (ProcessFormat (CurrentColor));
                        break;

                    case 15: // ordinary, Ctrl+O
                        CurrentColor = DefFormat;
                        UseBold = false;
                        UseColor = false;
                        UseReverse = false;
                        UseUnderline = false;
                        ChangeFormat (ProcessFormat (CurrentColor));
                        break;

                    case 0x1f: // eat CTRL+U for underline
                        UseUnderline = !UseUnderline;
                        ChangeFormat (ProcessFormat (CurrentColor));
                        break;

                    case 3: // color, Ctrl+K
                        State = 1;
                        break;

                    // We "eat" \r characters
                    case '\r':
                        break;

                    case '\n':
                        ProcessByte (15); // ctrl+o = force ordinary
                        // fall-through to default case

                    default:
                        CurChar.Char = Byte;
                        EmitChar ();
                        break;
                }

                break;

            case 1: // have seen a ^k (well, ^c really)
                if (isdigit(Byte))
                {
                    d1a = Byte - '0';
                    State = 2;
                }
                else
                {   // ^C + non-digit = no color
                    State = 0;
                    UseColor = false;
                    CurrentColor = DefFormat;
                    ChangeFormat (ProcessFormat (CurrentColor));
                    ProcessByte (Byte);
                }

                break;

            case 2: // have seen ^k, digit
                if (isdigit(Byte))
                {
                    d1b = Byte - '0';
                    State = 3;
                }
                else
                if (Byte == ',')
                {
                    CurrentColor.Foreground = GetColor (d1a);
                    State = 4;
                }
                else
                {
                    CurrentColor.Foreground = GetColor (d1a);
                    UseColor = true;
                    State = 0;
                    ChangeFormat (CurrentColor);
                    ProcessByte (Byte);
                }

                break;

            case 3: // have seen ^k, digit, digit
                if (Byte == ',')
                {
                    CurrentColor.Foreground = GetColor (d1b + (d1a * 10));
                    State = 4;
                }
                else
                {
                    CurrentColor.Foreground = GetColor (d1b + (d1a * 10));
                    UseColor = true;                
                    State = 0;
                    ChangeFormat (CurrentColor);
                    ProcessByte (Byte);
                }

                break;
     
            case 4: // have seen ^k, digit, [digit], comma
                if (isdigit(Byte))
                {
                    d2a = Byte - '0';
                    State = 5;
                }
                else
                {
                    UseColor = true;
                    State = 0;
                    ChangeFormat (CurrentColor);
                    ProcessByte (Byte);
                }
                break;

            case 5: // have seen ^k, digit, [digit], comma, digit
                if (isdigit(Byte))
                {
                    d2b = Byte - '0';
                    State = 6;
                }
                else
                {
                    CurrentColor.Background = GetColor (d2a);
                    UseColor = true;
                    State = 0;
                    ChangeFormat (CurrentColor);
                    ProcessByte (Byte);
                }
                break;

            case 6: // have seen ^k, digit, [digit], comma, digit, digit
                CurrentColor.Background = GetColor (d2b + (d2a * 10));
                UseColor = true;
                State = 0;
                ChangeFormat (CurrentColor);
                ProcessByte (Byte);
                break;
        }

        return;
    } unguard;
}


cFormat CParserMIRC::ProcessFormat (cFormat In)
{
    guard
    {
        if (UseColor)
            In = CurrentColor;

        if (UseBold)
        {
            int Red, Green, Blue;

            Red = In.Foreground & 0xff;
            Green = (In.Foreground & 0xff00) >> 8;
            Blue = (In.Foreground & 0xff0000) >> 16;

            // To simulate "bold" we double the color intensity, saturated to 255
            Red *= 2;
            Green *= 2;
            Blue *= 2;

            Red = min (255, Red);
            Green = min (255, Green);
            Blue = min (255, Blue);

            In.Foreground = Red + (Green << 8) + (Blue << 16);
        }

        if (UseReverse)
            swap (In.Foreground, In.Background);

        if (UseUnderline)
            In.Underline = true;

        return (In);
    } unguard;
}


COLORREF CParserMIRC::GetColor (int Indice)
{
    guard
    {
        if (Indice < 0  ||  Indice > 15)
            return (0);

        return (ColorTable[Indice]);
    } unguard;
}


void CParserMIRC::Reset (bool StartDependent)
{
    guard
    {
        CurrentColor = DefFormat;
        State = 0;
        UseBold = false;
        UseReverse = false;
        UseUnderline = false;
        UseColor = false;
        CParser2::Reset (StartDependent);
        return;
    } unguard;
}


