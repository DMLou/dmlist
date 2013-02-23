#include "CFastFile.h"


CFastFile::CFastFile (string FileName,
                      uint32 BlockSizeLog2,
                      uint32 CacheSize, 
                      DWORD DesiredAccess, 
                      DWORD ShareMode, 
                      DWORD CreationDisposition, 
                      DWORD FlagsAndAttributes, 
                      bool EnablePrefetching)
: Mutex(true)
{
    guard
    {
        OwnerThread = GetCurrentThreadId();
        SetLastError (NoError);
        LastWin32Error = ERROR_SUCCESS;
        OIOCount = 0;
        CacheBlocks = NULL;
        FileHandle = INVALID_HANDLE_VALUE;
        OIOHandle = INVALID_HANDLE_VALUE;
        Prefetching = EnablePrefetching;

        CFastFile::DesiredAccess = DesiredAccess;
        CFastFile::ShareMode = ShareMode;
        CFastFile::CreationDisposition = CreationDisposition;
        CFastFile::FlagsAndAttributes = FlagsAndAttributes;
        CFastFile::BlockSizeLog2 = BlockSizeLog2;
        BlockSize = 1 << BlockSizeLog2;
        BlockSizeModAnd = BlockSize - 1;

        FileSize = 0;
        FilePos = 0;
        CFastFile::FileName = FileName;

        // Get our function pointer for SetFilePointerEx, if possible
        Kernel32DLL = LoadLibrary ("kernel32.dll");
        _SetFilePointerEx = (SFPExPointer) GetProcAddress (Kernel32DLL, "SetFilePointerEx");

        // Open the file
        FileHandle = CreateFile (FileName.c_str(), DesiredAccess, ShareMode,
            NULL, CreationDisposition, FlagsAndAttributes, NULL);

        // If we got INVALID_HANDLE_VALUE then the file couldn't be opened. D'oh.
        if (FileHandle == INVALID_HANDLE_VALUE)
        {
            SetLastError (OpenError);
            Unlock ();
            return;
        }

        // We must be in Windows NT/2000/XP to use overlapped I/O for disk files.
        if (::GetVersion() < 0x80000000)
        {
            OIOHandle = CreateFile (FileName.c_str(), DesiredAccess, ShareMode,
                NULL, CreationDisposition, FlagsAndAttributes | FILE_FLAG_OVERLAPPED, NULL);
        }

        // Get file size
        FileSize = GetRealFileSize();

        // Get file timestamp
        FileTime = GetRealFileTime();

        //CacheBlockSize = SectorSize;

        // Set up cache based on the *requested* cache size. Round up; that is, if they ask for a 6K cache,
        // give them an 8K cache (assuming CacheBlockSize is 4K ...)
        CacheBlockCount = ((CacheSize + BlockSizeModAnd) >> BlockSizeLog2);

        // And initialize the mawfaw cache!
        CacheBlocks = NULL;
        ResetCache ();

        // Rewind to beginning.
        RealSeek (0);

        // Release the flood gates.
        Unlock ();
        return;
    } unguard;
}


CFastFile::~CFastFile ()
{
    guard
    {
        Lock ();

        WaitForOIOCompletion ();

        // Deallocate memory used by the cache
        if (CacheBlocks != NULL)
        {
            while (!CacheBlocks->IsEmpty())
                EjectOldestCacheBlock (true, NULL);

            delete CacheBlocks;
        }

        // Free misc. Win32 handles
        FreeLibrary (Kernel32DLL);

        if (FileHandle != INVALID_HANDLE_VALUE)
            CloseHandle (FileHandle);

        if (OIOHandle != INVALID_HANDLE_VALUE)
            CloseHandle (OIOHandle);

        return;
    } unguard;
}


CFastFile::ErrorType CFastFile::GetLastError (void)
{
    guard
    {
        return (LastError);
    } unguard;
}


DWORD CFastFile::GetLastWin32Error (void)
{
    guard
    {
        return (LastWin32Error);
    } unguard;
}


bool CFastFile::IsPrefetchAllowed (void)
{
    guard
    {
        return (Prefetching &&
            OwnerThread == GetCurrentThreadId() &&
            OIOHandle != INVALID_HANDLE_VALUE);
    } unguard;
}


uint32 CFastFile::GetCacheBlockSize (void)
{
    guard
    {
        return (BlockSize);
    } unguard;
}


// Assumes that CacheBlockSize and CacheBlockCount are already initialized
void CFastFile::ResetCache (void)
{
    guard
    {
        Lock ();

        WaitForOIOCompletion ();

        // If the cache exists, then let's free it all
        if (CacheBlocks != NULL)
        {   // Go through and release any memory associated with any cache block
            while (!CacheBlocks->IsEmpty())
                EjectOldestCacheBlock (true, NULL);

            delete CacheBlocks;
        }

        // Create the new cache.
        FileBlockCount = (FileSize + BlockSizeModAnd) >> BlockSizeLog2;
        CacheBlocks = new CacheType (CacheBlockCount);

        Unlock ();
        return;
    } unguard;
}


bool CFastFile::EjectOldestCacheBlock (bool FreeBuffer, uint8 **BufferResult)
{
    guard
    {
        CacheBlockData *HeadPtr;

        Lock ();

        HeadPtr = CacheBlocks->GetHead ();

        if (HeadPtr == NULL)
        {
            Unlock ();
            return (false);
        }

        if (!HeadPtr->Available)
            WaitForOIOCompletion ();

        // !! We must redo the Get operation
        HeadPtr = CacheBlocks->GetHead ();

        if (FreeBuffer)
        {
            //VirtualFree (HeadPtr->Buffer, BlockSize, MEM_DECOMMIT);
            delete HeadPtr->Buffer;
        }
        else
        {
            *BufferResult = HeadPtr->Buffer;
        }

        CacheBlocks->RemoveHead ();

        Unlock ();
        return (true);
    } unguard;
}


uint8 *CFastFile::GetNewCacheBuffer (void)
{
    guard
    {
        uint8 *Buffer;

        Lock ();

        if (CacheBlocks->IsFull())
            EjectOldestCacheBlock (false, &Buffer);
        else
            //Buffer = (uint8 *) VirtualAlloc (NULL, BlockSize, MEM_COMMIT, PAGE_READWRITE);
            Buffer = new uint8[BlockSize];

        Unlock ();
        return (Buffer);
    } unguard;
}


void CFastFile::ResetFileSize (void)
{
    guard
    {
        uint64 NewFileSize;
        uint64 OldFileSize;

        Lock ();

        OldFileSize = FileSize;
        NewFileSize = GetRealFileSize();

        if (NewFileSize != OldFileSize)
            ResetCache ();

        FileSize = NewFileSize;

        Unlock ();
        return;
    } unguard;
}


void CFastFile::ResetFileTime (void)
{
    guard
    {
        Lock ();
        FileTime = GetRealFileTime ();
        Unlock ();

        return;
    } unguard;
}


uint64 CFastFile::GetRealFileSize (void)
{
    guard
    {
        LARGE_INTEGER FileSize;

        FileSize.LowPart = ::GetFileSize (FileHandle, (DWORD *)&FileSize.HighPart);

        return ((uint64)FileSize.QuadPart);
    } unguard;
}


FILETIME CFastFile::GetRealFileTime (void)
{
     guard
     {
         FILETIME WriteTime;

        ::GetFileTime (FileHandle, NULL, NULL, &WriteTime);
        return (WriteTime);
    } unguard;
}


bool CFastFile::Seek (uint64 Location)
{
    guard
    {
        Lock ();

        if (Location >= FileSize)
        {
            SetLastError (SeekError);
            Unlock ();
            return (false);
        }

        FilePos = Location;
        SetLastError (NoError);
        Unlock ();
        return (true);
    } unguard;
}


uint64 CFastFile::GetFilePos (void)
{
    guard
    {
        uint64 Return;

        Lock ();
        Return = FilePos;
        Unlock ();

        return (Return);
    } unguard;
}


string CFastFile::GetFileName (void)
{
    guard
    {
        string Return;

        Lock ();
        Return = FileName;
        Unlock ();

        return (Return);
    } unguard;
}


uint64 CFastFile::GetFileSize (void)
{
    guard
    {
        uint64 Return;

        Lock ();
        Return = FileSize;
        Unlock ();

        return (Return);
    } unguard;
}


FILETIME CFastFile::GetFileTime (void)
{
    guard
    {
        return (FileTime);
    } unguard;
}


// This function needs to work analagous to the prefetch CPU instructions:
// * It's a *hint*. The prefetching doesn't actually have to take place 
//   (like if we're in Win9x, just ignore these commands)
//   Although in this implementation the "hint" is only ignored if the block is already in the cache
// * If the requested block is invalid (i.e. past EOF), then don't crash. Just return and do nothing.
void CFastFile::Prefetch (uint64 SeekLocation)
{
    guard
    {
        uint64 Ordinal;

        Lock ();

        Ordinal = SeekLocation >> BlockSizeLog2;

        if (Ordinal >= FileBlockCount)
        {
            Unlock ();
            return;
        }

        if (!IsPrefetchAllowed())
        {
            Unlock ();
            return;
        }

        if (!IsBlockInCache (Ordinal))
            CacheBlock (Ordinal, true);

        Unlock ();
        return;
    } unguard;
}


bool CFastFile::ReadByte (uint8 &Result)
{
    guard
    {
        return (ReadData (&Result, 1, NULL));
    } unguard;
}


bool CFastFile::ReadData (uint8 *Buffer, uint64 Length, uint64 *AmountReadResult)
{
    guard
    {
        bool Return = false;
        bool Result;
        uint64 i;
        uint64 FirstBlock;
        uint64 FirstOffset;
        uint64 LastBlock;
        uint64 FirstSize;
        uint8 *InBuf;
        uint8 *OutBuf;

        if (Length == 0)
        {
            if (AmountReadResult != NULL)
                *AmountReadResult = 0;

            return (true);
        }

        // Atomicize it! This is to ensure all our blocks are in the cache and do not get
        // bumped out while we are using them
        Lock ();

        // Past EOF? Can't do jack :(
        if (FilePos >= FileSize)
        {
            SetLastError (EOFError);
            Unlock ();
            return (false);
        }

        // Requesting bytes past the EOF? Ok, just don't give them those bytes.
        if (FilePos + Length >= FileSize)
            Length = FileSize - FilePos;

        // Now we go through and read the data ... 
        // First figure out the range of blocks we must read from the file
        FirstBlock = FilePos >> BlockSizeLog2;
        LastBlock = (FilePos + Length - 1) >> BlockSizeLog2;
        OutBuf = Buffer;

        // First block may not need all CacheBlockSize bytes ...
        Result = ReadBlockPtr (FirstBlock, &InBuf);
        if (!Result)
        {
            SetLastError (ReadError);
            Unlock ();
            return (false);
        }

        FirstOffset = FilePos & BlockSizeModAnd;
        FirstSize = min (Length, BlockSize - FirstOffset);
        StreamCopy (OutBuf, InBuf + FirstOffset, FirstSize);
        OutBuf += FirstSize;

        if (FirstBlock != LastBlock)
        {
            // Loop through and add the data for each subsequent cache block
            for (i = FirstBlock + 1; i < LastBlock - 1; i++)
            {
                Result = ReadBlockPtr (i, &InBuf);
                if (!Result)
                {
                    SetLastError (ReadError);
                    Unlock ();
                    return (false);
                }

                StreamCopy (OutBuf, InBuf, BlockSize);
                OutBuf += BlockSize;
            }

            // Last block may not need all CacheBlockSize bytes ...
            if (LastBlock != FirstBlock)
            {
                uint32 LastSize;

                LastSize = (FilePos + Length) - (LastBlock << BlockSizeLog2);
                Result = ReadBlockPtr (LastBlock, &InBuf);

                if (!Result)
                {
                    SetLastError (ReadError);
                    Unlock ();
                    return (false);
                }

                StreamCopy (OutBuf, InBuf, LastSize);
            }
        }

        // Everything is ok.
        if (AmountReadResult != NULL)
            *AmountReadResult = Length;

        FilePos += Length;

        SetLastError (NoError);
        Unlock ();
        return (true);
    } unguard;
}


bool CFastFile::RealReadData (uint8 *Buffer, uint64 SeekPoint, uint64 Length, uint64 *AmountReadResult)
{
    guard
    {
        DWORD AmountRead;
        bool Result;

        Lock ();

        Result = RealSeek (SeekPoint, false);
        if (!Result)
        {
            Unlock ();
            return (false);
        }

        Result = bool(ReadFile (FileHandle, (void *)Buffer, (DWORD)Length, &AmountRead, NULL));

        if (AmountReadResult != NULL)
            *AmountReadResult = (uint64)AmountRead;

    #ifdef CFASTFILE_STATS
        Stats.DataRead += AmountRead;
    #endif

        Unlock ();
        return (Result);
    } unguard;
}


bool CFastFile::RealSeek (uint64 Location, bool DoLock)
{
    guard
    {
        bool ReturnValue;
        
        if (DoLock)
            Lock ();

        if (_SetFilePointerEx != NULL)
        {
            // In Windows NT/2000/XP we can make one nice happy function call to set the file pointer.
            ReturnValue = bool(_SetFilePointerEx (FileHandle, *((PLARGE_INTEGER)&Location), NULL, FILE_BEGIN));
        }
        else
        {
            DWORD Result;

            // But in Windows 95/98/Me, things get bloody.
            // NOTE: 64-bit file pointers DO NOT WORK in Windows 95/98/Me. To quote from MSDN regarding
            // SetFilePointer() and the third parameter that it takes:
            //
            // lpDistanceToMoveHigh 
            // [in] Pointer to the high-order 32 bits of the signed 64-bit distance to move. If you do
            //      not need the high-order 32 bits, this pointer must be set to NULL. When non-NULL, 
            //      this parameter also receives the high-order DWORD of the new value of the file pointer.
            //      For more information, see the Remarks section later in this topic. 
            // (Here's the important part)
            //      Windows 95/98/Me: If the pointer lpDistanceToMoveHigh is not NULL, then it must point 
            //      to either 0, INVALID_SET_FILE_POINTER, or the sign extension of the value of 
            //      lDistanceToMove. Any other value will be rejected.
            //
            // So we can only move around in the file about 2^31 bytes at a time.
            // So what we do is first set the file pointer to location 0.
            // Then we move 1GB forward at a time until we are within <1GB of where we want to be.
            // Then we move the remainder of the distance.
            // Note that this makes file seek operations execute in O(n) time, although I doubt it
            // will ever become a performance issue on files that will ever be opened on computers
            // that are running these defunct operating systems. I doubt this loop will ever iterate
            // more than 100 times.

            Result = SetFilePointer (FileHandle, 0, NULL, FILE_BEGIN);

            if (Result == INVALID_SET_FILE_POINTER)
                ReturnValue = false; // if it bails on us, we bail
            else
            if (Location == 0)
                ReturnValue = true;
            else
            {
                const LONG PerLoopDelta = 1073741824; // 2^30 = 1GB
                uint64 DistanceLeft;
                LONG Delta;

                Delta = PerLoopDelta;
                DistanceLeft = Location;

                while (DistanceLeft >= PerLoopDelta)
                {
                    Result = SetFilePointer (FileHandle, Delta, 0, FILE_CURRENT);
                    if (Result == INVALID_SET_FILE_POINTER)
                        break;

                    DistanceLeft -= PerLoopDelta;
                }

                if (DistanceLeft > 0)
                    ReturnValue = SetFilePointer (FileHandle, (LONG)DistanceLeft, 0, FILE_CURRENT);

                if (Result == INVALID_SET_FILE_POINTER)
                    ReturnValue = false;
            }
        }

        if (DoLock)
            Unlock ();

        return (ReturnValue);
    } unguard;
}


uint64 CFastFile::GetRealFilePos (void)
{
    guard
    {
        uint64 Pos;
        LARGE_INTEGER Zero;

        Zero.QuadPart = 0;
        Lock ();

        if (_SetFilePointerEx != NULL)
            _SetFilePointerEx (FileHandle, Zero, (LARGE_INTEGER *)&Pos, FILE_CURRENT);
        else
            Pos = (uint64) SetFilePointer (FileHandle, 0, NULL, FILE_CURRENT);

        Unlock ();

        return (Pos);
    } unguard;
}


// This function will read in the desired data from disk and place it in the cache
// If it's already in the cache, nothing will be read from disk.
// If the block is in the progress of being prefetched (i.e. an overlapped I/O is
// already in progress) then it will return quickly
// If Overlapped is true, and the data isn't in the cache, then it will be read
// using overlapped I/O and the function will return immediately
// Otherwise the data will be read using normal blocking I/O
bool CFastFile::CacheBlock (uint64 BlockOrdinal, bool Overlapped)
{
    guard
    {
        bool Result = false;

        Lock ();

        // Bail out if they're asking for an out-of-bounds block
        if (BlockOrdinal >= FileBlockCount)
        {
            SetLastError (SeekError);
            Result = false;
        }
        else
        {
            CacheBlockData *OurBlock;
            CacheBlockData NewBlock;

            OurBlock = CacheBlocks->Get (BlockOrdinal);

            // Smile and return w/o doing anything if this block is already cached
            // (Also handles the case if the data is currently being read using
            // overlapped I/O ==> prefetching)
            if (OurBlock != NULL)
                Result = true;
            else
            // If the block is not in the cache, then read it from disk
            {   // First try to do it async, if they asked
                if (Overlapped)
                    Result = SetupBlockPrefetch (BlockOrdinal);

                // If we're not doing overlapped I/O, or if we can't set up the overlapped I/O, do blocking I/O
                if (Result == false)
                {
                    NewBlock.Buffer = GetNewCacheBuffer ();
                    Result = RealReadData (NewBlock.Buffer, BlockOrdinal << BlockSizeLog2, BlockSize, NULL);

                    // If the data was read successfully, then put it in the cache!
                    if (Result)
                    {
                        NewBlock.Available = true;
                        CacheBlocks->Push (BlockOrdinal, NewBlock);
                    }
                }
            }

            SetLastError (NoError);
        }

        Unlock ();
        return (Result);
    } unguard;
}


bool CFastFile::IsBlockInCache (uint64 BlockOrdinal)
{
    guard
    {
        bool Result = false;

        Lock ();

        if (CacheBlocks->Get (BlockOrdinal) != NULL)
            Result = true;

        Unlock ();
        return (Result);    
    } unguard;
}


bool CFastFile::ReadBlock (uint64 BlockOrdinal, uint8 *Buffer)
{
    guard
    {
        bool Result = false;
        uint8 *Buf;

        Lock ();

        Result = ReadBlockPtr (BlockOrdinal, &Buf);

        if (Result)
            StreamCopy (Buffer, Buf, BlockSize);

        Unlock ();
        return (Result);
    } unguard;
}


bool CFastFile::ReadBlockPtr (uint64 BlockOrdinal, uint8 **Buffer)
{
    guard
    {
        bool Result = false;

        Lock ();

        CacheBlock (BlockOrdinal, false);
        Result = ReadCacheBlockPtr (Buffer, BlockOrdinal);

        Unlock ();
        return (Result);
    } unguard;
}


bool CFastFile::ReadCacheBlock (uint8 *Buffer, uint64 BlockOrdinal)
{
    guard
    {
        bool Result = false;
        uint8 *Buf;

        Lock ();

        Result = ReadCacheBlockPtr (&Buf, BlockOrdinal);

        if (Result)
            StreamCopy (Buffer, Buf, BlockSize);

        Unlock ();
        return (Result);
    } unguard;
}


bool CFastFile::ReadCacheBlockPtr (uint8 **Buffer, uint64 BlockOrdinal)
{
    guard
    {
        bool Result = false;
        CacheBlockData *Block;

        Lock ();

        // If the data is in the cache, give it to them and return true.
        Block = CacheBlocks->Get (BlockOrdinal);

        if (Block != NULL)
        {   // May have to block for overlapped IO    
            if (!Block->Available)
                WaitForOIOCompletion ();

            // !! Must redo the Get. This is NOT redundant.
            Block = CacheBlocks->Get (BlockOrdinal);

            *Buffer = Block->Buffer;
            Result = true;
            CacheBlocks->BumpElement (BlockOrdinal);
        }

        Unlock ();
        return (Result);
    } unguard;
}


void CFastFile::Lock (void)
{
    guard
    {
        Mutex.Lock ();
        return;
    } unguard;
}


void CFastFile::Unlock (void)
{
    guard
    {
        Mutex.Unlock ();
        return;
    } unguard;
}


// Stuff used for prefetching
typedef struct
{
    CFastFile *ThisFile;
    uint32 BlockOrdinal; // the block that we read and we should set the Available flag on
} OIOContext;


// Sets up a block for prefetching. However, if it's already in the cache, nothing
// is done.
bool CFastFile::SetupBlockPrefetch (uint64 BlockOrdinal)
{
    guard
    {
        uint64 Location;
        OVERLAPPED *Over;
        OIOContext *Context;
        CacheBlockData Block;
        BOOL Result;

        Lock ();

        Location = BlockOrdinal << BlockSizeLog2;
        Over = (OVERLAPPED *) new uint8[sizeof(OVERLAPPED) + sizeof(OIOContext)];
        Context = (OIOContext *)(((uint8 *)Over) + sizeof(OVERLAPPED));
        Context->ThisFile = this;
        Context->BlockOrdinal = BlockOrdinal;
        
        Over->hEvent = NULL;
        Over->Internal = 0;
        Over->InternalHigh = 0;
        Over->Offset = (DWORD)(Location & 0xffffffff);
        Over->OffsetHigh = (DWORD)(Location >> 32);
        
        Block.Available = false;
        Block.Buffer = GetNewCacheBuffer ();
        CacheBlocks->Push (BlockOrdinal, Block);
        Result = ReadFileEx (OIOHandle, Block.Buffer, BlockSize, Over, OIOCompletion);

        if (Result == 0)
        {
            delete Over;
            SetLastError (ReadError);
            Unlock ();
            return (false);
        }

        OIOCount++;
        Unlock ();
        return (true);
    } unguard;
}


// Finishes/cleans up overlapped I/O: call this when you actually read a block
// that's being prefetched (i.e. Available = false)
void CFastFile::WaitForOIOCompletion (void)
{
    guard
    {
        while (OIOCount > 0)
            SleepEx (0, TRUE);

        return;
    } unguard;
}


void CALLBACK OIOCompletion (DWORD ErrorCode, DWORD BytesRead, LPOVERLAPPED Overlapped)
{
    guard
    {
        OVERLAPPED *Over;
        OIOContext *Context;
        CFastFile::CacheBlockData *Block;

        Over = Overlapped;
        Context = (OIOContext *)(((uint8 *)Over) + sizeof(OVERLAPPED));

        Context->ThisFile->Lock ();

        if (ErrorCode == ERROR_HANDLE_EOF)
            Context->ThisFile->SetLastError (CFastFile::EOFError);

        Block = Context->ThisFile->CacheBlocks->Get (Context->BlockOrdinal);
        Block->Available = true;
        Context->ThisFile->OIOCount--;

    #ifdef CFASTFILE_STATS
        Context->ThisFile->Stats.DataRead += BytesRead;
    #endif

        Context->ThisFile->Unlock ();

        delete (uint8 *)Over;
        return;
    } unguard;
}
