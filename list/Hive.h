/*****************************************************************************

    Hive.h

    Functions for maintaining the ListXP index hive.
    This caches, on disk, the seek point cache for any file opened and allows
    you to open files faster assuming they have not changed since the last
    time you opened them.

    The hive is usually kept in ~/Application Data/ListXP

*****************************************************************************/


#ifndef _HIVE_H
#define _HIVE_H


#include "List.h"
#include "CFileLinesDB.h"


#define HASHREVISION 5
#define INDEXEXT     "LXP"


#pragma pack (push,1)
typedef struct
{   // This information is used when creating the filename for the index hive file
    typedef struct
    {
        DWORD  FileIndexHigh;
        DWORD  FileIndexLow;
        DWORD  VolumeSerial;
        uint8 TabSize;
        uint8 ParserOrdinal;
        uint8 SeekGranLog2;
        uint16 MaxLineWidth;
        uint16 HashRevision;
    } FileHashType;

    FileHashType Hash;

    // This data is saved with an easy fwrite() command to the beginning of the index hive file
    // Fill in before calling SaveHiveData
    typedef struct
    {   //
        FILETIME CreationTime;
        FILETIME LastWriteTime;
        uint64 FileSize;

        // You must fill these in manually
        uint64 TotalLines;  // total # of lines!
        uint32 LongestLine; // longest line, in characters
        double AvgCharsPerLine;
    } FileSignatureType;

    FileSignatureType Signature;

    // The data we are most concerned with: fill this in before SaveHiveData
    // Something like this works well:  HiveData.SeekPoints = CFileLinesDB->GetSeekPoints()
    vector<CFileLinesDB::SeekPoint> SeekPoints;

} HiveData;
#pragma pack (pop)


// Fills in the Hash part of the HiveData structure. Call this BEFORE
// saving with SaveHiveData.
extern bool FillInHiveHashData (HANDLE FileHandle, 
                                uint16 TabSize, 
                                uint16 MaxWidth, 
                                uint16 ParserOrdinal,
                                uint16 SeekGranLog2,
                                HiveData *Data);


// Fills in the following parts of the Signature portion of the HiveData structure:
// CreationTime, LastWriteTime, FileSize
// Does NOT fill in TotalLines
// Do this before saving with SaveHiveData.
extern bool FillInHiveSigData (HANDLE FileHandle, HiveData *Data);


// Returns true if the files are deemed equivalent, otherwise false.
// "Equivalent" is defined as relying on the same seekpoint data
// (in English, that means they are the same file and you can use the index hive file's data!)
// This is done by comparing the Hash and the Signature, except for the TotalLines and LongestLine
// (because often you don't know the TotalLines of one of these when you compare them)
extern bool AreFilesEquivalent (HiveData *LHS, HiveData *RHS);


// Loads the hive data associated with the hash data in DataResult
// You must have the Hash filled in for DataResult before calling this function
// If the index file doesn't exist, false is returned. Otherwise the data
// is loaded and the true is returned.
extern bool LoadHiveData (HiveData *DataResult);


// Saves the given index data into the hive.
// Data must be completely filled in before calling this function
extern bool SaveHiveData (const HiveData *Data);


// Determines what the default hive folder location should be
extern bool GetDefaultHivePath (string &LocationResult);


// Initializes the hive to use within the program
extern bool SetHivePath (const string &HivePath);


// Only save files to the hive that are at least this size (MinSize is in bytes)
// SaveHiveData does not enforce this; you must enforce it yourself
extern void SetHiveMinSize (uint64 MinSize);


// Self-explanatory
extern uint64 GetHiveMinSize (void);


// Globals to avoid needed to pass the ListContext structure to the CFileLinesDB scanner thread
extern void SetHiveEnabled (bool OnOrOff);
extern bool IsHiveEnabled (void);

// Returns true or false depending on whether hive compression is supported.
// THis is merely dependent on whether the OS supports it (i.e. WinVer = NT/2K/XP)
extern bool IsHiveCompressionSupported (void);
extern void SetHiveCompression (bool OnOrOff);
extern bool IsHiveCompressionEnabled (void);


#endif // _HIVE_H

