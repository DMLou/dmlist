#ifndef _CHEXLINESDB_H
#define _CHEXLINESDB_H


#include "List.h"
#include "CFastFile.h"
#include "CLinesDB.h"
#include "TreeList.h"


class CHexLinesDB : public CLinesDB
{
public:
    CHexLinesDB (CFastFile *TheFile, width_t TextWidth, uint64 MaxCacheSize, uint8 WordSize, bool LittleEndian);
    ~CHexLinesDB ();

    bool GetLine (uint64 Line, CParser2::TextLine **TextLine);
    width_t GetLineWidth (uint64 Line);

    int GetWordSizeLog2 (void)
    {
        if (WordSize == 1)
            return (0);
        else
        if (WordSize == 2)
            return (1);
        else
        if (WordSize == 4)
            return (2);
        else
        if (WordSize == 8)
            return (3);
        else
            return (-1);
    }

    uint64 SeekPointToLine (uint64 SeekPoint)
    {
        guard
        {
            return (SeekPoint / (HexWordsPerRow * WordSize));
        } unguard;
    }

    uint64 LineToSeekPoint (uint64 Line)
    {
        guard
        {
            return (Line * (HexWordsPerRow * WordSize));
        } unguard;
    }

    width_t GetDisplayWidth (void) // returns the display width for which this instance was initialized with
    {
        guard
        {
            return (DisplayWidth);
        } unguard;
    }

    int GetHexLineCacheLength (void)
    {
        guard
        {
            return (HexLineCache.GetLength());
        } unguard;
    }

    uint32 GetHexLineCacheMemUsage (void)
    {
        guard
        {
            return (HexLineCache.GetMemUsage() + 
                (HexLineCache.GetLength() * (sizeof(CParser2::TextLine *) + (DisplayWidth * sizeof (cText)))));
        } unguard;
    }

private:
    width_t DisplayWidth;
    width_t TextWidth;
    int WordSize; // how many bytes for each word
    bool LittleEndian;
    uint32 HexWordsPerRow;
    bool   LargeOffset;
    uint8 *HexReadBuffer;

    TreeList<uint64, CParser2::TextLine *> HexLineCache;
};


#endif // _CLINESDB_H