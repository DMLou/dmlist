#include "CLinesDB.h"


CLinesDB::CLinesDB (CFastFile *TheFile)
{
    guard
    {
        File = TheFile;
        TotalLines = 0;
        StopScanning = false;
        DoneScanning = false;
        LongestLineWidth = 0;

    #ifdef CLINESDB_STATS
        Stats.CacheHits = 0;
        Stats.CacheMisses = 0;
        Stats.Locks = 0;
        Stats.TotalLineReads = 0;
    #endif

        return;
    } unguard;
}


CLinesDB::~CLinesDB ()
{
    guard
    {
        return;
    } unguard;
}


uint64 CLinesDB::GetTotalLines (void)
{
    guard
    {
        uint64 Return;

        Lock ();
        Return = TotalLines;
        Unlock ();

        return (Return);
    } unguard;
}


void CLinesDB::SetTotalLines (uint64 NewTotalLines)
{
    guard
    {
        Lock ();
        TotalLines = NewTotalLines;
        Unlock ();
        return;
    } unguard;
}


uint16 CLinesDB::GetLongestLineWidth (void)
{
    guard
    {
        return (LongestLineWidth);
    } unguard;
}


CFastFile *CLinesDB::GetFile (void)
{
    guard
    {
        return (File);
    } unguard;
}


bool CLinesDB::IsDoneScanning (void)
{
    guard
    {
        return (DoneScanning);
    } unguard;
}


void CLinesDB::SetDoneScanning (bool NewDS)
{
    guard
    {
        DoneScanning = NewDS;
        return;
    } unguard;
}


bool CLinesDB::PleaseStopScanning (void)
{
    guard
    {
        return (StopScanning);
    } unguard;
}


void CLinesDB::SetScanPercent (float NewPercent)
{
    guard
    {
        ScanPercent = NewPercent;
        return;
    } unguard;
}


float CLinesDB::GetScanPercent (void)
{
    guard
    {
        return (ScanPercent);
    } unguard;
}


void CLinesDB::SetLongestLineWidth (uint16 NewLW)
{
    guard
    {
        LongestLineWidth = NewLW;
        return;
    } unguard;
}
