// Todo: convert to more (better) functional form

#include "CScannerThread.h"
#include "CFileLinesDB.h"
#include "CFastFile.h"
#include "Hive.h"
#include "CParser2.h"
#include "SmartLock.h"


//#define NO_BUFFER


CScannerThread::CScannerThread (CFileLinesDB *LinesDB, uint64 StartLine, CSTCompletionCallback Callback, void *Context)
{
    guard
    {
        CScannerThread::LinesDB = LinesDB;
        CScannerThread::StartLine = StartLine;
        CScannerThread::Callback = Callback;
        CScannerThread::CallbackContext = Context;
        return;
    } unguard;
}


CScannerThread::~CScannerThread ()
{
    guard
    {
        LinesDB->StopScanningThread ();

        while (LinesDB->IsDoneScanning() == false)
            Sleep (1);

        return;
    } unguard;
}


bool CScannerThread::ThreadFunction (void)
{
    guard
    {
        uint64 StartSeek;
        uint64 CurrentLine;
        uint64 SeekPos;
        uint64 TotalLineBreaks;
        uint64 BufferBlock;
        uint64 BlockSize;
        uint64 LastBlock;
        uint64 BufferSeek;
        uint64 BytesRead;
        uint8 *Buffer;
        int    Counter;
        int    cursor;
        int    PFDistance;
        int    i;
        bool   ReadResult;

        TotalLineBreaks = 0;
        Buffer = NULL;
        BufferBlock = 0;
        BufferSeek = 0;
        PFDistance = 8;
        HiveTotalLines = 0;
        HiveLongestLine = 0;
        DoHiveSave = true;
        SkipReading = false;

        //SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

        // Cached values
        TabSize = LinesDB->GetTabSize();
        MaxWidth = LinesDB->GetMaxWidth();
        TheFile = LinesDB->GetFile()->GetSecondView();
        Parser = LinesDB->GetParser()->Clone ();

        //
        if (StartLine == 0)
        {
            LinesDB->SetAverageCharsPerLine (0);
            LinesDB->SetLongestLineWidth (0);
        }

        if (TheFile->GetLastError() != CFastFile::ErrorType::NoError)
        {
            delete TheFile;
            delete Parser;

            if (Callback != NULL)
                Callback (false, CallbackContext);

            return false;
        }

        // Load hive data?
        LoadFromHive ();

        // Figure out what offset within the file to start reading from
        if (StartLine == 0)
            StartSeek = 0;
        else
        {
            if (!LinesDB->GetSeekPoint (StartLine, StartSeek))
            {   // flag error?
                delete TheFile;
                delete Parser;

                if (Callback != NULL)
                    Callback (false, CallbackContext);

                return (false);
            }
        }

        TheFile->Seek (StartSeek);

        Counter = 0;
        CurrentLine = StartLine;
        Parser->SetParseOnlyFlag (true);
        Parser->Reset (false);

        BlockSize = TheFile->GetCacheBlockSize ();
        BufferBlock = StartSeek / BlockSize;
        LastBlock = (TheFile->GetFileSize() + 1) / BlockSize;
        BufferSeek = BufferBlock * BlockSize;

    #ifndef NO_BUFFER
        TheFile->CacheBlock (BufferBlock);
        TheFile->ReadCacheBlockPtr (&Buffer, BufferBlock);
    #endif

        if (TheFile->GetFileSize() == 0)
        {
            ReadResult = false;
            DoHiveSave = false;
            BytesRead = 0;
        }
        else
        {
            ReadResult = true;

            if (BufferBlock == LastBlock)
                BytesRead = TheFile->GetFileSize() % BlockSize;
            else
                BytesRead = BlockSize;
        }

        // Prefetch
        for (i = 0; i <= PFDistance; i++)
            TheFile->Prefetch (BlockSize * (BufferBlock + i));

    #ifdef NO_BUFFER
        // non-buffered version
        DoHiveSave = false;
        if (!SkipReading)
        {
            while (!LinesDB->PleaseStopScanning())
            {
                uint8 Byte;
                bool Result;
                bool EndOfFile = false;
                bool DoUnlock = false;

                Result = TheFile->ReadByte (Byte);

                if (!Result)
                    break;
                else
                    EndOfFile = true;

                Parser->FeedByte (Byte);

                // Now process any lines that may have accumulated
                if (Parser->GetLineCount() > 0)
                {
                    LinesDB->Lock ();
                    DoUnlock = true;
                }

                while (Parser->GetLineCount() > 0)
                {
                    CParser2::TextLine *TL;

                    TL = Parser->GetFirstLine ();

                    LinesDB->AddSeekPoint (CurrentLine, SeekPos, TL->Dependent);
                    SeekPos += TL->Bytes;
                    LinesDB->SetLongestLineWidth (max (LinesDB->GetLongestLineWidth(), TL->Chars));

                    // Stats type information
                    Counter++;
                    if ((Counter & 63) == 0  ||  EndOfFile)
                    {
                        if (TheFile->GetFileSize() != 0)
                            LinesDB->SetScanPercent (100.0f * ((float)SeekPos / (float)TheFile->GetFileSize()));
                        else
                            LinesDB->SetScanPercent (100.0f);

                        LinesDB->SetAverageCharsPerLine ((double)SeekPos / (double)CurrentLine);
                        Sleep (0);
                    }

                    // Maintain total line count information
                    CurrentLine++;

                    if (CurrentLine > HiveTotalLines)
                        LinesDB->SetTotalLines (CurrentLine);

                    Parser->RemoveFirstLine (true);
                }

                if (DoUnlock)
                    LinesDB->Unlock ();
            }
        }

    #else // NO_BUFFER
        // buffered version
        cursor = StartSeek % BlockSize;
        SeekPos = StartSeek;

        while (!SkipReading && !LinesDB->PleaseStopScanning())
        {
            bool EndOfFile = false;
            bool Result = false;

            // Feed all the bytes to the parser
            while (cursor < BytesRead)
            {
                Parser->FeedByte (Buffer[cursor]);
                cursor++;
            }

            // Did we just finish with the last block from the file? If so, set the EOF flag and the HiveSave flag
            if (BytesRead < BlockSize  ||  BufferBlock == LastBlock)
            {
                DoHiveSave = true;
                EndOfFile = true;
            }

            // If we just processed the last block then we should flush
            if (EndOfFile)
                Parser->Flush ();

            // Now process any lines that may have accumulated
            LinesDB->Lock ();

            while (Parser->GetLineCount() > 0)
            {
                CParser2::TextLine *TL;

                TL = Parser->GetFirstLine ();

                LinesDB->AddSeekPoint (CurrentLine, SeekPos, TL->Dependent);
                SeekPos += TL->Bytes;
                // TODO: A little worried about the cast below. May want to switch to boost::numeric_cast eventually
                LinesDB->SetLongestLineWidth(std::max(LinesDB->GetLongestLineWidth(), static_cast<width_t>(TL->Chars)));

                // Stats type information
                Counter++;
                if ((Counter & 63) == 0  ||  EndOfFile)
                {
                    if (TheFile->GetFileSize() != 0)
                        LinesDB->SetScanPercent (100.0f * ((float)SeekPos / (float)TheFile->GetFileSize()));
                    else
                        LinesDB->SetScanPercent (100.0f);

                    LinesDB->SetAverageCharsPerLine ((double)SeekPos / (double)CurrentLine);
                    Sleep (0);
                }

                // Maintain total line count information
                CurrentLine++;

                if (CurrentLine > HiveTotalLines)
                    LinesDB->SetTotalLines (CurrentLine);

                Parser->RemoveFirstLine (true);
            }

            LinesDB->Unlock ();

            // Read the next block of data
            BufferBlock++;
            Result = TheFile->CacheBlock (BufferBlock); // tag: handle error condition
            Result &= TheFile->ReadCacheBlockPtr (&Buffer, BufferBlock); // tag: handle error condition

            if (BufferBlock != LastBlock)
                BytesRead = BlockSize;
            else
                BytesRead = TheFile->GetFileSize() % BlockSize;

            TheFile->Prefetch (BlockSize * (BufferBlock + PFDistance));

            cursor = 0;

            if (!Result) // read errors?!
            {
                if (BufferBlock < LastBlock)
                    DoHiveSave = false;
                    
                break;
            }
        }
    #endif

        if (LinesDB->PleaseStopScanning())
            DoHiveSave = false;

        // Save hive data ... 
        if (DoHiveSave)
            SaveToHive ();

        delete TheFile;
        delete Parser;

        LinesDB->SetScanPercent (100.0f);

        if (Callback != NULL)
            Callback (true, CallbackContext);

        LinesDB->SetDoneScanning (true);

        return (false);
    } unguard;
}


void CScannerThread::LoadFromHive (void)
{
    guard
    {
        if (IsHiveEnabled())
        {
            HiveData FileData;
            HiveData LoadedData;

            if (FillInHiveHashData (TheFile->GetWin32Handle(), 
                                    TabSize, 
                                    MaxWidth, 
                                    LinesDB->GetParser()->GetUID(), 
                                    LinesDB->GetSeekGranLog2(),
                                    &FileData)   
                &&
                FillInHiveHashData (TheFile->GetWin32Handle(), 
                                    TabSize, 
                                    MaxWidth, 
                                    LinesDB->GetParser()->GetUID(), 
                                    LinesDB->GetSeekGranLog2(),
                                    &LoadedData) 
                &&
                FillInHiveSigData  (TheFile->GetWin32Handle(), &FileData))       
            {
                if (LoadHiveData (&LoadedData))
                {   // LoadedData and FileData are now completely filled in
                    // Now we must determine whether to actually use the data
                    if (AreFilesEquivalent (&FileData, &LoadedData))
                    {   // All signs point to "YES"
                        int i;
                        uint32 SG = LinesDB->GetSeekGranularity ();

                        for (i = 0; i < LoadedData.SeekPoints.size(); i++)
                            LinesDB->AddSeekPoint (uint64(i) * SG, LoadedData.SeekPoints[i].Offset, LoadedData.SeekPoints[i].Dependent);

                        HiveTotalLines = LoadedData.Signature.TotalLines;
                        LinesDB->SetTotalLines (HiveTotalLines);
                        HiveLongestLine = LoadedData.Signature.LongestLine;
                        LinesDB->SetLongestLineWidth (HiveLongestLine);
                        LinesDB->SetAverageCharsPerLine (LoadedData.Signature.AvgCharsPerLine);
                        SkipReading = true;
                        DoHiveSave = false;
                    }
                }
            }
        }

        return;
    } unguard;
}


void CScannerThread::SaveToHive (void)
{
    guard
    {
        if (DoHiveSave  &&  
            IsHiveEnabled()  &&  
            TheFile->GetFileSize() > (GetHiveMinSize()))
        {
            HiveData Data;

            if (FillInHiveHashData (TheFile->GetWin32Handle(), 
                                    LinesDB->GetTabSize(), 
                                    LinesDB->GetMaxWidth(), 
                                    LinesDB->GetParser()->GetUID(), 
                                    LinesDB->GetSeekGranLog2(),
                                    &Data) &&
                FillInHiveSigData (TheFile->GetWin32Handle(), &Data))
            {
                // Fill in seekpoint data
                Data.SeekPoints = LinesDB->GetSeekPoints();
                Data.Signature.TotalLines = LinesDB->GetTotalLines();
                Data.Signature.LongestLine = LinesDB->GetLongestLineWidth();
                Data.Signature.AvgCharsPerLine = LinesDB->GetAverageCharsPerLine();
                SaveHiveData (&Data);
            }
        }

        return;
    } unguard;
}