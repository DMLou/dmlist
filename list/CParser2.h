/*****************************************************************************

  CParser2

  Revision 2 of the parsing subsystem.

*****************************************************************************/


#ifndef _CPARSER2_H
#define _CPARSER2_H


#include "ListTypes.h"
#include "../C_APILib/Console.h"
#include "List.h"
#include "StaticHeap.h"


extern int IdentifyCParserBase (uint8 *Data);


class CParser2
{
public:
    static const wchar_t InvalidChar = 0xffff;

    typedef struct
    {
        wchar_t Char;
        uint32 Bytes;
    } TextChar;

    typedef struct
    {
        wchar_t *Text;
        uint32 Bytes;
        uint32 Chars;

        // Used when we are in parse-only mode
        wchar_t LastChar;
    } TextWord;

    typedef struct
    {
        uint32 Begin;
        uint32 Length;
        cFormat Format;
    } FormatExtent;

    typedef struct
    {
        wchar_t *Text;
        uint32 Bytes;
        uint32 Chars;
        vector<FormatExtent> Formatting;
        bool Dependent; // is this line dependent on the previous line for formatting?

        // Used when we are in parse-only mode
        wchar_t LastChar;
    } TextLine;

protected:
    CParser2 (width_t MaxWidth,
              bool    WordWrapping,
              uint32  TabSize,
              cFormat DefaultFormat,
              uint16  UID);

public:
    extern friend CParser2 *CreateCParserBase (width_t MaxWidth,
                                               bool    WordWrapping,
                                               uint32  TabSize,
                                               cFormat DefaultFormat,
                                               uint16  UID);

    virtual ~CParser2 ();

    uint16 GetUID (void);
    void SetUID (uint16 NewUID);

    // Client interface
    virtual bool UseUnicodeRendering (void);

    // Creates another instance of this parser. DOES NOT duplicate any state information.
    virtual CParser2 *Clone (void);

    // Use this to feed data to the parser
    void FeedByte (uint8 Byte);

    bool GetParseOnlyFlag (void);

    // This call MUST be immediately followed by a call to Reset()!!!
    void SetParseOnlyFlag (bool NewPO);

    // Call this when you want to start over completely with processing
    // You can reset the parser so that it starts with the new line in a dependent state by
    // setting StartDependent to true.
    // If you want to switch the parser between ParseOnly and storage-allowed modes,
    // switch that state and them immediately call this function
    void Reset (bool StartDependent = false);

    //
    cFormat GetDefaultFormat (void);

    // Return value: How many lines have been processed that are waiting to be retrieved, 0 if none are ready
    int GetLineCount (void);

    // Get a pointer to the "oldest" line (i.e. the lowest line #)
    // Returns NULL if there are no lines ready
    TextLine *GetFirstLine (void);

    // Make a copy of a TextLine object
    void CopyTextLine (TextLine *Dst, const TextLine *Src);

    // Remove the oldest line
    // If you pass in false for Deallocate, then you are in charge of deallocating
    // both the TextLine->Text and the TextLine, with the delete operator
    void RemoveFirstLine (bool Deallocate);

    // Call this to force the current character/word/line to be emitted
    // This will flush ALL data that has been accumulated but not stored into the current line
    void Flush (void);

    // Inheritor interface
protected:
    // Is this character a separator or newline character that would
    // break up words or lines?
    bool IsSeparator (wchar_t Char);

    void EmitTab (void);

    // Default ProcessByte just emits every byte as a character
    virtual void ProcessByte (uint8 Byte);
    void EmitChar (void);
    void EmitWord (void);
    void EmitLine (bool NewLineIsDependent);

    // Will push the current format on to the format-extent stack, and let you start with a new formatting
    void ChangeFormat (cFormat &NewFormat);

    // Will directly set the current formatting
    void SetFormat (cFormat &NewFormat);

    // Will "glue" the formatting from the previous line into the new one
    // For example, if the last line was 40 chars but had 45 chars of formatting info,
    // because the last 5 chars wrapped on to the next line, this will take those last
    // 5 chars of formatting and place them on the new line
    void GlueFormatting (void);

protected:

    FormatExtent CurFmt;
    TextChar CurChar;
    TextWord CurWord;
    TextLine *CurLine;

    // Processed lines of text accumulate here
    deque<TextLine *> TextLines;

    uint32 MaxWidth;
    bool   WordWrapping;
    uint32 TabSize;
    bool   ParseOnly;
    uint16 UID;

    cFormat DefFormat;
};


extern void DeleteTextLine (CParser2::TextLine *Line);
extern CParser2::TextLine *BlankTextLine (void);
extern CParser2::TextLine *CopyTextLine (CParser2::TextLine *CopyMe, bool CopyFormatting = false);
extern CParser2::TextLine *PasteTwoLines (CParser2::TextLine *Left, CParser2::TextLine *Right);


#endif // _CPARSER_H