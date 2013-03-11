#include "List.h"
#include "CParser2.h"


CParser2 *CreateCParserBase (width_t MaxWidth,
                             bool    LineWrapping,
                             uint32  TabSize,
                             cFormat DefaultFormat,
                             uint16  UID)
{
    guard
    {
        return (new CParser2 (MaxWidth, LineWrapping, TabSize, DefaultFormat, UID));
    } unguard;
}


int IdentifyCParserBase (uint8 *Data)
{
    guard
    {
        return (1);
    } unguard;
}


CParser2::CParser2 (width_t MaxWidth,
                    bool    WordWrapping,
                    uint32  TabSize,
                    cFormat DefaultFormat,
                    uint16  UID)
{
    guard
    {
        CParser2::MaxWidth = MaxWidth;
        CParser2::WordWrapping = WordWrapping;
        CParser2::TabSize = TabSize;
        CParser2::ParseOnly = false;
        CParser2::DefFormat = DefaultFormat;
        CParser2::UID = UID;

        CurChar.Bytes = 0;
        CurWord.Text = NULL;
        CurWord.Chars = 0;
        CurWord.Bytes = 0;
        CurLine = NULL;
        Reset ();

        return;
    } unguard;
}


CParser2::~CParser2 ()
{
    guard
    {
        Reset ();

        if (CurWord.Text != NULL)
        {
            delete CurWord.Text;
            CurWord.Text = NULL;
        }

        if (CurLine != NULL)
        {
            if (CurLine->Text != NULL)
                delete CurLine->Text;

            CurLine->Text = NULL;

            delete CurLine;
            CurLine = NULL;
        }

        return;
    } unguard;
}



uint16 CParser2::GetUID (void)  
{  
    guard
    {
        return (UID); 
    } unguard;
}


void CParser2::SetUID (uint16 NewUID)
{
    guard
    {
        UID = NewUID;
        return;
    } unguard;
}

// Client interface
bool CParser2::UseUnicodeRendering (void)
{
    guard
    {
        return (false);
    } unguard;
}

// Creates another instance of this parser. DOES NOT duplicate any state information.
CParser2 *CParser2::Clone (void)
{
    guard
    {
        return (new CParser2 (MaxWidth, WordWrapping, TabSize, DefFormat, UID));
    } unguard;
}

// Use this to feed data to the parser
void CParser2::FeedByte (uint8 Byte)
{
    guard
    {
        CurChar.Bytes++;
        ProcessByte (Byte);
        return;
    } unguard;
}

bool CParser2::GetParseOnlyFlag (void)
{
    guard
    {
        return (ParseOnly);
    } unguard;
}

// This call MUST be immediately followed by a call to Reset()!!!
void CParser2::SetParseOnlyFlag (bool NewPO)
{
    guard
    {
        if (NewPO != ParseOnly)
        {
            Reset ();
            ParseOnly = NewPO;
        }

        return;
    } unguard;
}

// Call this when you want to start over completely with processing
// You can reset the parser so that it starts with the new line in a dependent state by
// setting StartDependent to true.
// If you want to switch the parser between ParseOnly and storage-allowed modes,
// switch that state and them immediately call this function
void CParser2::Reset (bool StartDependent)
{
    guard
    {
        Flush ();

        while (GetLineCount() > 0)
            RemoveFirstLine (true);

        // Init CurChar
        CurChar.Bytes = 0;
        CurChar.Char = InvalidChar;

        // Init CurWord
        if (CurWord.Text == NULL)
        {
            if (!ParseOnly)
                CurWord.Text = new wchar_t[MaxWidth + 1];
        }

        CurWord.Bytes = 0;
        CurWord.Chars = 0;
        CurWord.LastChar = InvalidChar;

        // Init CurLine
        if (CurLine == NULL)
        {
            CurLine = new TextLine;
            CurLine->Text = NULL;
        }

        if (CurLine->Text == NULL)
        {
            if (!ParseOnly)
                CurLine->Text = new wchar_t[MaxWidth + 1];
        }

        CurLine->Bytes = 0;
        CurLine->Chars = 0;
        CurLine->Formatting.clear();
        CurLine->Dependent = StartDependent;
        CurLine->LastChar = InvalidChar;

        // Init CurFmt
        CurFmt.Begin = 0;
        CurFmt.Length = 0;
        CurFmt.Format = DefFormat;

        return;
    } unguard;
}

//
cFormat CParser2::GetDefaultFormat (void)
{
    guard
    {
        return (DefFormat);
    } unguard;
}

// Return value: How many lines have been processed that are waiting to be retrieved, 0 if none are ready
int CParser2::GetLineCount (void)
{
    guard
    {
        return (TextLines.size());
    } unguard;
}

// Get a pointer to the "oldest" line (i.e. the lowest line #)
// Returns NULL if there are no lines ready
CParser2::TextLine *CParser2::GetFirstLine (void)
{
    guard
    {
        if (TextLines.size() == 0)
        return (NULL);

    return (TextLines[0]);
    } unguard;
}

// Make a copy of a TextLine object
void CParser2::CopyTextLine (TextLine *Dst, const TextLine *Src)
{
    guard
    {
        Dst->Bytes = Src->Bytes;
        Dst->Chars = Src->Chars;
        Dst->Dependent = Src->Dependent;
        Dst->Formatting = Src->Formatting;
        Dst->LastChar = Src->LastChar;

        if (Dst->Text != NULL  &&  Src->Text != NULL)
            StreamCopy (Dst->Text, Src->Text, Dst->Chars * sizeof (wchar_t));

        return;
    } unguard;
}

// Remove the oldest line
// If you pass in false for Deallocate, then you are in charge of deallocating
// both the TextLine->Text and the TextLine, with the delete operator
void CParser2::RemoveFirstLine (bool Deallocate)
{
    guard
    {
        if (Deallocate)
        {
            TextLine *TL;

            TL = TextLines.front();

            if (TL->Text != NULL)
            {
                delete TL->Text;
                TL->Text = NULL;
            }

            delete TL;
        }

        TextLines.pop_front();
        return;
    } unguard;
}

// Call this to force the current character/word/line to be emitted
// This will flush ALL data that has been accumulated but not stored into the current line
void CParser2::Flush (void)
{
    guard
    {
        if (CurChar.Bytes > 0)
            EmitChar ();

        if (CurWord.Bytes > 0)
            EmitWord ();

        if (CurLine != NULL)
            if (CurLine->Bytes > 0)
                EmitLine (false);

        return;
    } unguard;
}


// Is this character a separator or newline character that would
// break up words or lines?
bool CParser2::IsSeparator (wchar_t Char)
{
    guard
    {
        if (Char == L' '  ||  Char == L'\t'  ||  Char == L'\n'  ||  Char == L'\0')
            return (true);

        return (false);
    } unguard;
}

void CParser2::EmitTab (void)
{
    guard
    {
        int LinePos;
        int Spaces;

        LinePos = CurLine->Chars + CurWord.Chars;
        Spaces = TabSize - (LinePos % TabSize);

        if (LinePos + Spaces >= MaxWidth)
            Spaces = MaxWidth - LinePos;

        while (Spaces > 0)
        {
            CurChar.Char = L' ';
            EmitChar ();
            Spaces--;
        }

        return;
    } unguard;
}

// Default ProcessByte just emits every byte as a character
void CParser2::ProcessByte (uint8 Byte)
{
    guard
    {
        if (Byte == '\0')
            Byte = ' ';

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

void CParser2::EmitChar (void)
{
    guard
    {
        if (WordWrapping)
        {   // Need to pre-emit the current word if:
            // 1. The current word's length is the maximum length of a line (Word.Chars == MaxWidth)
            if (CurWord.Chars == MaxWidth)
                EmitWord ();

            // Emit the character into the word accumulator and reset the byte-count in the character accumulator
            if (!ParseOnly)
                CurWord.Text[CurWord.Chars] = CurChar.Char;

            CurWord.LastChar = CurChar.Char;
            CurWord.Bytes += CurChar.Bytes;
            CurWord.Chars++;
            CurFmt.Length++;
            CurChar.Bytes = 0;

            // Need to post-emit the current word if:
            // 1. Character was a separator (space, tab, EOL)
            // 2. Word is now the maximum length of a line (Word.Chars == MaxWidth)
            if (IsSeparator(CurChar.Char)  ||
                CurWord.Chars == MaxWidth)
            {
                EmitWord ();
            }
        }
        else
        {   // Need to pre-emit the current line if;
            // 1. The current line's length is the maximum length of a line (Line.Chars == MaxWidth)
            if (CurLine->Chars == MaxWidth)
                EmitLine (true);

            // Emit the word straight into the line stream
            if (!ParseOnly)
                CurLine->Text[CurLine->Chars] = CurChar.Char;

            CurLine->LastChar = CurChar.Char;
            CurLine->Bytes += CurChar.Bytes;
            CurLine->Chars++;
            CurFmt.Length++;
            CurChar.Bytes = 0;

            // Need to post-emit the current line if:
            // 1. Character was an end of line character (\n)
            // 2. Line is now the maximum length of a line
            if (CurChar.Char == L'\n')
                EmitLine (false);
            else
            if (CurLine->Chars == MaxWidth)
                EmitLine (true);
        }

        return;
    } unguard;
}

void CParser2::EmitWord (void)
{
    guard
    {
        int i;

        // Need to pre-emit the current line if:
        // 1. Adding new word would overflow the line. 
        //    This causes the new line we're processing to have a formatting dependency.
        if (CurWord.Chars + CurLine->Chars > MaxWidth)
            EmitLine (true);

        // Copy the characters into the line accumulator
        if (!ParseOnly)
            for (i = 0; i < CurWord.Chars; i++)
                CurLine->Text[CurLine->Chars + i] = CurWord.Text[i];

        CurLine->LastChar = CurWord.LastChar;
        CurLine->Chars += CurWord.Chars;
        CurLine->Bytes += CurWord.Bytes;

        // Reset the word accumulator
        CurWord.Chars = 0;
        CurWord.Bytes = 0;

        // Need to post-emit the current line if:
        // 1. Line is full
        //    this causes the new line we're processing to have a formatting dependency.
        // 2. Last character we pasted into the line was an end-of-line character
        //    this new line does NOT have a formatting dependency
        if (CurLine->Chars == MaxWidth)
            EmitLine (true);
        else
        //if (CurLine->Text[CurLine->Chars - 1] == '\n')
        if (CurLine->LastChar == L'\n')
            EmitLine (false);

        return;
    } unguard;
}

void CParser2::EmitLine (bool NewLineIsDependent)
{
    guard
    {
        int nbegin;
        int nlen;

        if (!ParseOnly)
        {
            CurLine->Formatting.push_back (CurFmt);
            CurLine->Text[CurLine->Chars] = L'\0'; // null-terminate the wide string
        }

        TextLines.push_back (CurLine);

        // 'clip' CurFmt in case we don't reset it
        if (!ParseOnly)
        {
            nbegin = (signed)CurFmt.Begin - (signed)CurLine->Chars;
            nlen = (signed)CurFmt.Length;

            if (nbegin < 0)
            {
                nlen += nbegin;
                nbegin = 0;
            }

            if (nlen >= 0)
            {
                CurFmt.Begin = nbegin;
                CurFmt.Length = std::max(static_cast<uint32_t>(nlen), CurWord.Chars);
            }
            else
            {
                CurFmt.Begin = 0;
                CurFmt.Length = CurWord.Chars;
                CurFmt.Format = DefFormat;
            }
        }

        //
        CurLine = NULL;
        CurLine = new TextLine;
        CurLine->Text = NULL;
        CurLine->Bytes = 0;
        CurLine->Chars = 0;
        CurLine->Dependent = NewLineIsDependent;

        if (!ParseOnly)
            CurLine->Text = new wchar_t[MaxWidth + 1];
        else
            CurLine->Text = NULL;

        if (!ParseOnly)
        {
            if (!NewLineIsDependent)
            {
                CurFmt.Format = DefFormat;
                CurFmt.Begin = 0;
                CurFmt.Length = 0;
            }
            else
            {
                CurLine->Formatting.push_back (CurFmt);
                CurFmt.Begin = 0; 
                CurFmt.Length = CurWord.Chars;
                GlueFormatting ();
            }
        }

        return;
    } unguard;
}

// Will push the current format on to the format-extent stack, and let you start with a new formatting
void CParser2::ChangeFormat (cFormat &NewFormat)
{
    guard
    {
        if (!ParseOnly)
        {
            CurLine->Formatting.push_back (CurFmt);
            CurFmt.Begin = CurLine->Chars + CurWord.Chars;
            CurFmt.Length = 0;
            CurFmt.Format = NewFormat;
        }

        return;
    } unguard;
}

// Will directly set the current formatting
void CParser2::SetFormat (cFormat &NewFormat)
{
    guard
    {
        CurFmt.Format = NewFormat;
        return;
    } unguard;
}

// Will "glue" the formatting from the previous line into the new one
// For example, if the last line was 40 chars but had 45 chars of formatting info,
// because the last 5 chars wrapped on to the next line, this will take those last
// 5 chars of formatting and place them on the new line
void CParser2::GlueFormatting (void)
{
    guard
    {
        int i;
        TextLine *TL;

        TL = GetFirstLine ();

        if (TL == NULL)
            return;

        for (i = 0; i < TL->Formatting.size(); i++)
        {
            FormatExtent &Ext = TL->Formatting[i];
            FormatExtent FExt;
            int begin;
            int end;
            int nbegin;
            int nlength;

            begin = Ext.Begin;
            end = begin + Ext.Length;

            nbegin = begin - (signed)TL->Chars;
            nlength = Ext.Length;

            // Now "clip" it
            if (nbegin < 0)
            {
                nlength += nbegin;
                nbegin = 0;
            }

            if (nlength >= 0)
            {
                FExt.Begin = nbegin;
                FExt.Length = nlength;
                FExt.Format = Ext.Format;
                CurLine->Formatting.push_back (FExt);
            }
        }

        return;
    } unguard;
}
