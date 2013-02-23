#ifndef _CFILELINESDB_H
#define _CFILELINESDB_H


#include "CLinesDB.h"
#include "List.h"
#include "CFastFile.h"
#include "TreeList.h"
#include "CParser2.h"
#include "CScannerThread.h"


class CScannerThread;


class CFileLinesDB : public CLinesDB
{
public:
    typedef struct
    {
        uint64 Offset;
        bool Dependent;
    } SeekPoint;

public:
    CFileLinesDB (CFastFile *TheFile,
                  CParser2 *Parser,
                  width_t LineWidth, 
                  uint32 SeekGranularityLog2, 
                  //uint64 MaxCacheSize,
                  uint32 CacheChunks, // this many 'chunks' of size (2^SeekGranularityLog2) will be maintained; ala cachelines
                  uint32 TabSize);

    ~CFileLinesDB ();

    CParser2 *GetParser (void)
    {
        guard
        {
            return (Parser);
        } unguard;
    }

    // Scans file starting at the given line number; returns false if
    // scanning is already in progress
    bool ScanFromLine (uint64 Line, CSTCompletionCallback CallbackFn = NULL, void *CallbackContext = NULL);

    // Returns a pointer to the data stored in the line cache
    bool GetLine (uint64 Line, CParser2::TextLine **TextLine);    // gives you a pointer to the data, DO NOT DEALLOCATE IT

    bool GetLine (uint64 Line, CParser2::TextLine **TextLine, int Depth);

    // Returns a copy of the data stored in the line cache, already formatted for rendering
    bool GetLineFormatted (uint64 Line, cTextString *RichText);

    // Returns a copy of the wide character data stored in the line cache
    bool GetLineUnicode (uint64 Line, wstring *Text);

    // Returns a copy of the character data stored in the line cache, converted from Unicode to ASCII
    bool GetLineASCII (uint64 Line, string *Text);

    void RemoveCacheElement (uint64 Line);
    void ClearCache (void);

    vector<SeekPoint> &GetSeekPoints (void)
    {
        guard
        {
            return (SeekPoints);
        } unguard;
    }

    // Line "width" in here refers to BYTE WIDTH, not display/character width

    bool SearchLineCache (uint64 Line, CParser2::TextLine **DataResult);
    bool IsLineInCache (uint64 Line);
    void SetMaxWidth (width_t NewMaxWidth);
    uint32 GetTabSize (void);
    width_t GetMaxWidth (void);

    void AddSeekPoint (uint64 Line, uint64 SeekPoint, bool Dependent);
    bool GetSeekPoint (uint64 Line, uint64 &SeekResult);
    width_t GetLineWidth (uint64 Line, bool *Dependence = NULL);
    bool GetLineDependence (uint64 Line);

    // Signals the background scanner thread to STFU
    void StopScanningThread (void);

    //
    bool SeekPointToLine (uint64 SeekPoint, uint64 &LineResult);

    // Just use GetSeekPoint; this function isn't really needed.
    //bool LineToSeekPoint (uint64 Line, uint64 &SeekPointResult);

    uint32 GetSeekGranularity (void)
    {
        guard
        {
            return (1 << SeekGranularityLog2);
        } unguard;
    }

    uint32 GetSeekGranLog2 (void)
    {
        guard
        {
            return (SeekGranularityLog2);
        } unguard;
    }

    double GetAverageCharsPerLine (void)
    {
        guard
        {
            return (AvgCharsPerLine);
        } unguard;
    }

    void SetAverageCharsPerLine (double NewAvg)
    {
        guard
        {
            AvgCharsPerLine = NewAvg;
            return;
        } unguard;
    }

    uint32 GetSeekCacheMemUsage (void)
    {
        guard
        {
            return (SeekPoints.size() * sizeof (SeekPoint));
        } unguard;
    }

    uint32 GetSeekCacheLength (void)
    {
        guard
        {
            return (SeekPoints.size());
        } unguard;
    }

    uint32 GetTextCacheMemUsage (void)
    {
        guard
        {
            Lock ();

            vector<CParser2::TextLine **> dvec (TextCache.GetDataVector());
            uint32 Usage = 0;
            int i, j;

            for (i = 0; i < dvec.size(); i++)
            {
                CParser2::TextLine **Array;

                Array = dvec[i];
                for (j = 0; j < (1 << SeekGranularityLog2); j++)
                {
                    uint32 bytes;

                    bytes = Array[j]->Bytes;
                    Usage += bytes;
                }
            }

            Usage += TextCache.GetMemUsage ();

            Unlock ();
            return (Usage);
        } unguard;
    }

    uint32 GetTextCacheChunks (void)
    {
        guard
        {
            return (TextCache.GetLength());
        } unguard;
    }

    // Iterator class and interface
    class db_iterator : public iterator <random_access_iterator_tag, CParser2::TextLine *>
    {
    public:
        db_iterator (db_iterator &copy_me)
            : parent (copy_me.parent),
              line (copy_me.line)
        {
            return;
        }

        db_iterator (CFileLinesDB *parent_init, uint64 line_init = 0)
            : parent (parent_init),
              line (line_init)
        {
            return;
        }

        uint64 get_line (void)
        {
            return (line);
        }

        void set_line (uint64 new_line)
        {
            line = new_line;
            return;
        }

        value_type operator* (void) throw (runtime_error)
        {
            CParser2::TextLine *Return;

            if (!parent->GetLine (line, &Return))
                throw runtime_error ("runtime_error: could not get requested line # (operator*)");

            return (Return);
        }

        value_type operator[] (difference_type offset)
        {
            CParser2::TextLine *Return;

            if (!parent->GetLine (line + offset, &Return))
                throw runtime_error ("runtime_error: could not get requested line # (operator[])");

            return (Return);
        }

        db_iterator &operator= (const db_iterator &rhs)
        {
            parent = rhs.parent;
            line = rhs.line;
            return (*this);
        }

        bool operator== (const db_iterator &rhs)
        {
            return (parent == rhs.parent &&
                    line == rhs.line);
        }

        bool operator!= (const db_iterator &rhs)
        {
            return (parent != rhs.parent ||
                    line != rhs.line);
        }

        db_iterator &operator++ (void)
        {
            line++;
            return (*this);
        }

        db_iterator operator++ (int)
        {
            uint64 old_line;

            old_line = line;
            line++;
            return (db_iterator (parent, old_line));
        }

        db_iterator operator+ (distance_type dist)
        {
            return (db_iterator (parent, line + dist));
        }

        db_iterator &operator+= (distance_type dist)
        {
            line += dist;
            return (*this);
        }

        db_iterator &operator-- (void)
        {
            line--;
            return (*this);
        }

        db_iterator operator-- (int)
        {
            uint64 old_line;

            old_line = line;
            line--;
            return (db_iterator (parent, old_line));
        }

        db_iterator operator- (distance_type dist)
        {
            return (db_iterator (parent, line - dist));
        }

        db_iterator &operator-= (distance_type dist)
        {
            line -= dist;
            return (*this);
        }

    private:
        CFileLinesDB *parent;
        uint64 line;
    };

    db_iterator begin (void)
    {
        return (db_iterator (this, 0));
    }

    db_iterator end (void)
    {
        return (db_iterator (this, GetTotalLines ()));
    }

private:
    CParser2 *Parser;

    // Paged linewidth cache system
    uint32 CacheChunks;
    vector<SeekPoint> SeekPoints;       // We directly store *every* Nth seek point. 
    uint32 SeekGranularityLog2;      // ... where N = 2^SeekGranularity = 1<<SeekGranularity
    uint32 SeekGranularityModAnd;    // To perform a % operation, do & by this number
    uint32 MaxWidth;                 // What is the longest line in the file, in characters?

    // Each item is an array of ChunkSize elements
    TreeList<uint64, CParser2::TextLine **> TextCache;

    // This function will pull in the associated cache element in to the seekpoint heap
    void FillWidthCacheLine (uint64 Line);

    // Allocates an array of the appropriate size and gives it to you
    // All CParser2::TextLine elements are set to NULL
    CParser2::TextLine **AllocateCacheLine (void);

    // Frees an array of the appropriate size; properly frees every non-NULL CParser2::TextLine element as well
    void FreeCacheLine (CParser2::TextLine **Array);

    uint64 LastComputedSeekLine;
    uint64 LastComputedSeekPoint;

    // Cache the most recent TextCache item
    uint64 LastTCElement;
    CParser2::TextLine **LastTCArray;

    uint32 TabSize;
    uint32 LineCacheMemUsage;
    //uint64 CacheSize;
    double AvgCharsPerLine;
    CScannerThread *ScannerThread;
};


#endif // _CFILELINESDB_H
