#ifndef _CLINESDB_H
#define _CLINESDB_H


#include "List.h"
#include "CFastFile.h"
//#include "CParser.h"
#include "CMutex.h"


// We use a 16-bit unsigned word to keep track of line widths,
// to save memory.
#define CLINESDB_MAXWIDTH 0x7ffe


extern DWORD WINAPI ScanFileThread (LPVOID Context);


// Comment out to disable statistics tracking and reporting
//#define CLINESDB_STATS


class CLinesDB
{
public:
    CLinesDB (CFastFile *TheFile);

    virtual ~CLinesDB ();

    __forceinline void Lock (void)
    {
        guard
        {
            Mutex.Lock ();

    #ifdef CLINESDB_STATS
            Stats.Locks++;
    #endif
            return;
        } unguard;
    }

    __forceinline void Unlock (void)
    {
        guard
        {
            Mutex.Unlock ();
            return;
        } unguard;
    }

    // Inheritors need to implement this!
    // Returns a copy of the data
    //virtual bool GetLineFormatted (uint64 Line, cTextString *RichText) = NULL;

    // Returns a pointer to the data which you must NOT free or modify.
    virtual bool GetLine (uint64 Line, CParser2::TextLine **TextLine) = NULL;

    virtual bool GetCopyOfLine (uint64 Line, CParser2::TextLine **TextLine)
    {
        bool result;
        CParser2::TextLine *TheText = NULL;

        Lock ();

        result = GetLine (Line, &TheText);

        if (result)
            *TextLine = CopyTextLine (TheText, true);

        Unlock ();
        return (result);
    }

    //
    uint64 GetTotalLines (void);
    void SetTotalLines (uint64 NewTotalLines);
    CFastFile *GetFile (void);

    bool IsDoneScanning (void);
    void SetDoneScanning (bool NewDS);
    bool PleaseStopScanning (void);

    void SetScanPercent (float NewPercent);
    float GetScanPercent (void);

    width_t GetLongestLineWidth (void);
    void SetLongestLineWidth (uint16 NewLW);

protected:
    float ScanPercent;

    CFastFile *File;
    width_t LongestLineWidth; // keep track of the longest line's width

    uint64 TotalLines;
    bool StopScanning;
    bool DoneScanning;

    //    SharedObject Mutex; 
    CMutex Mutex;

#ifdef CLINESDB_STATS
    // Stats
public:
    typedef struct
    {
        uint64 TotalLineReads; // how many line requests?
        uint64 CacheMisses;
        uint64 CacheHits;
        uint64 Locks;          
    } DBStats;

    DBStats &GetStats (void)
    {
        guard
        {
            return (Stats);
        } unguard;
    }

protected:
    DBStats Stats;
#endif
};


#endif // _CLINESDB_H