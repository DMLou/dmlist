/*****************************************************************************

  CFastFile

  Provides an implementation for buffered and cached I/O for Win32 files.
  To do:
  1. Mechanism for locking a cache block so that it will not be ejected
  2. Ability to write data

*****************************************************************************/

#ifndef _CFASTFILE_H
#define _CFASTFILE_H


#include "List.h"
#include "TreeList.h"
#include "CMutex.h"


typedef BOOL (WINAPI *SFPExPointer) (HANDLE, LARGE_INTEGER, PLARGE_INTEGER, DWORD);


class CFastFile
{
public:
    CFastFile (string FileName, 
               uint32 BlockSizeLog2,
               uint32 CacheSize,
               DWORD DesiredAccess, 
               DWORD ShareMode,
               DWORD CreationDisposition, 
               DWORD FlagsAndAttributes,
               bool EnablePrefetching = true);

    virtual ~CFastFile ();

    typedef enum
    {
        NoError,
        EOFError,
        OpenError,
        ReadError,
        WriteError,
        SeekError,
        OtherError
    } ErrorType;

    // High level, standard I/O functionality

    // Reads data from the file. Data is placed into the buffer pointed at by Buffer. The number
    // of bytes read is at most equal to Length. The actual number of bytes read is placed into the
    // uint64 pointed at by AmountReadResult. If all goes OK, then the function returns true. Even
    // if *AmountReadResult < Length, this doesn't necessarily mean there was an error. Usually it
    // means you ran into the end of the file.
    // The file pointer will be incremented the appropriate distance upon successful return and
    // the last error will be set to NoError.
    // If AmountReadResult is NULL then the actual number of bytes it not returned.
    bool ReadData (uint8 *Buffer, uint64 Length, uint64 *AmountReadResult);

    // Convenience function. Returns true or false if a byte could be read. If true, then
    // Result will be set equal to the byte that was read.
    // This uses ReadData(), so any error conditions should be interpreted the same as if you
    // had called that function.
    bool ReadByte (uint8 &Result);

    // Will try to prefetch the data at the requested location. This function returns immediately
    // and does not block. Blocking will only occur when you try to read the data at that location 
    // (with ReadData/ReadByte) and it has not finished being read from disk yet.
    // If you try to prefetch an invalid location, nothing will happen and the function will return
    // immediately. If you try to prefetch in an OS that does not support overlapped I/O for disk 
    // files (Win95/98/Me) then nothing will happen. Because of this behaviour you may safely place
    // Prefetch() calls in your code without extra code to determine if the prefetch location is valid
    // or if prefetching is even supported.
    // Functionality of reading and writing is not affected by this function, only performance.
    void Prefetch (uint64 SeekLocation);

    // Sets the file pointer to the requested location. If this is not a valid location then
    // the function returns false and the file pointer is unchanged. Otherwise returns true
    // on success.
    // This function affects where ReadData/WriteData/ReadByte/WriteByte retrieve their data.
    bool Seek (uint64 Location);

    // Returns the current file pointer.
    uint64 GetFilePos (void);

    // Returns the file name (same as given to the constructor).
    string GetFileName (void);

    // Returns the cached file size. This may not reflect the "true" size of the file, only the
    // size of the file when it was opened. To make sure this is up to date you can use
    // ResetFileSize.
    uint64 GetFileSize (void);

    // Returns the cached file timestamp (last write time).
    FILETIME GetFileTime (void);

    // Returns the last error (or "NoError") that was encountered.
    ErrorType GetLastError (void);

    // Returns the last Win32 API error condition that was encountered.
    DWORD GetLastWin32Error (void); // Stores last ::GetLastError() value

    // Tells you whether prefetching will actually do anything.
    // Note: Results from this function govern all calls to Prefetch(). It is possible that
    //       calls to this function at different times (and esp. from different threads) will
    //       yield different return values.
    bool IsPrefetchAllowed (void);

    // This function will redetermine the size of the file and will flush the I/O cache if
    // the size has changed.
    void ResetFileSize (void); // the filesize info is cached ... so it must be reset if you think the file size has changed

    // This function will reset the cache file timestamp (write time). Does not do anything else!
    void ResetFileTime (void);

    // This will clear out the contents of the I/O cache.
    void ResetCache (void);

    // These functions return the Win32 creation parameters
    DWORD GetDesiredAccess (void)
    {
        guard
        {
            return (DesiredAccess);
        } unguard;
    }

    DWORD GetShareMode (void)
    {
        guard
        {
            return (ShareMode);
        } unguard;
    }

    DWORD GetCreationDisposition (void)
    {
        guard
        {
            return (CreationDisposition);
        } unguard;
    }

    DWORD GetFlagsAndAttributes (void)
    {
        guard
        {
            return (FlagsAndAttributes);
        } unguard;
    }
    
    // Serialize thread access.
    // If you need to make sure a sequence of actions is atomic, such as setting the file pointer
    // and then reading data, then place Lock()/Unlock() calls around those actions.
    // Example:
    //     uint64 OldSeek;
    //     File->Lock ();
    //     OldSeek = File->GetFilePos();
    //     File->Seek (wherever);
    //     File->ReadData (...);
    //     File->Seek (OldSeek);
    //     File->Unlock ();
    void Lock (void);
    void Unlock (void);

    // The Win32 file handle ... for super advanced use and stuff. Note that using ReadFile/WriteFile
    // on this returned HANDLE can cause the cached data managed by CFastFile to become out of date
    // It is best to call ResetCache before calling this.
    virtual HANDLE GetWin32Handle (void)
    {
        guard
        {
            return (FileHandle);
        } unguard;
    }

    // Returns a copy of this class instance, optionally with a different cache size
    // The two class instantiations maintain independent cache and file position information
    // So be careful that you don't write to one and then try to read that data from the
    // other instantiation. It won't return the right data!
    virtual CFastFile *GetSecondView (uint32 NewCacheSize = 0)
    {
        guard
        {
            return new CFastFile (FileName, BlockSizeLog2, NewCacheSize ? NewCacheSize : GetCacheSize(), 
                DesiredAccess, ShareMode, CreationDisposition, FlagsAndAttributes, Prefetching);
        } unguard;
    }

    // Cache functionality

    // Returns true if the block is within the file bounds (i.e. not past the EOF) and copies the data
    // into Buffer. Note that if you read the last block of the file that the actual number of bytes
    // read is not indicated (all other blocks use the full cache block size). You can do some simple 
    // math to figure out the true number of bytes copies.
    // The area of memory that Buffer points to should be able to hold at least GetCacheBlockSize() bytes.
    // If the requested block is in the cache, then the data will be copied from the cache and no
    // disk activity will take place.
    // Otherwise, the data will be read from disk, placed in the cache, and then placed in Buffer.
    // If Buffer is NULL then the data will not be copied.
    // Returns false if the block is past the EOF, or if there was some other error.
    // Use GetLastError() for exact information.
    bool ReadBlock (uint64 BlockOrdinal, uint8 *Buffer);

    // This function does the same thing, except it returns a pointer to the data in the cache.
    // This can increases performance if you know exactly what you're doing since it avoids the
    // memory copy performed by ReadBlock.
    // Note that you are retrieving a pointer to the data that is actually in the cache! It is
    // best that you treat this as read-only
    // Make sure to Lock() around operations using the data pointer returned from this, as
    // it's always possible another thread could eject the data just returned. This could change
    // the data or it could invalidate it completely (possibly causing an exception).
    bool ReadBlockPtr (uint64 BlockOrdinal, uint8 **Buffer);

    // Returns true if the block is in the cache. Buffer is then filled with the data from that block.
    // Returns false if the block is not in the cache or if the requested block is past the
    // end of the file.
    bool ReadCacheBlock (uint8 *Buffer, uint64 BlockOrdinal);

    // Same deal as ReadCacheBlock, except returns a pointer to the data instead (ala ReadBlockPtr)
    // Note that the ordering of the parameters here is intentionally backwards from ReadBlockPtr
    // so that you don't accidentally mix and match the two.
    bool ReadCacheBlockPtr (uint8 **Buffer, uint64 BlockOrdinal);

    // Caches the block from the disk file if it isn't already there. Also does FIFO/LRU queue
    // management (i.e. if the cache's queue is full, then the oldest element is removed to
    // make room for the new element).
    // Returns false if BlockOrdinal is out of bounds.
    // You can use Overlapped=true to use overlapped I/O and accomplish a sort of prefetching.
    // In this case the function will return immediately. The data will not be readable by
    // ReadBlock[Ptr] or ReadCacheBlock[Ptr] until it has actually been read from disk.
    // In the case those functions are called upon to read data that is not yet available,
    // they will block until that data has actually been read.
    // For prefetching, it is recommended that you use Prefetch() instead.
    bool CacheBlock (uint64 BlockOrdinal, bool Overlapped = false);

    // Will eject the oldest cache block. If FreeData is true then the Buffer associated
    // with that cache block will be freed. Otherwise it will not be freed and will
    // be returned in BufferResult (i.e. *BufferResult = CacheBlocks[oldest].Buffer)
    // However if the queue is empty then *BufferResult will be set to NULL and the
    // function will return false (in all other cases it returns true).
    // Setting FreeBuffer to false and passing in a pointer for BufferResult is one
    // way that internal functionality saves time by not reallocating buffers when
    // it is not necessary to do so.
    bool EjectOldestCacheBlock (bool FreeBuffer = true, uint8 **BufferResult = NULL);

    // Checks to see if a block is in the cache or not.
    bool IsBlockInCache (uint64 BlockOrdinal);

    // This will read data "for real" ... basically a syntax thunk down to ::ReadFile. This is
    // used by other functions and it is not necessary to use it for typical usage.
    // Please use ReadData() instead.
    bool RealReadData (uint8 *Buffer, uint64 SeekPoint, uint64 Length, uint64 *AmountReadResult);

    // This will set the actual file pointer ... basically a syntax thunk down to SetFilePointer
    // or SetFilePointerEx, depending on the OS type. SetFilePointerEx is used if it is available
    // and provides 64-bit file pointer capability (i.e. >4GB files)
    // Please use Seek() instead.
    bool RealSeek (uint64 Location, bool DoLock = true); // does an actual seek to FilePos

    // A syntax thunk down to Win32 functions that retrieve the file position associated with
    // the Win32 handle.
    // Please use GetFilePos() instead.
    uint64 GetRealFilePos (void);

    // A syntax thunk down to the GetFileSize (or whatever) Win32 calls.
    uint64 GetRealFileSize (void);

    // A syntax thunk down to retrieve the last file write time via GetFileInformationByHandle
    FILETIME GetRealFileTime (void);

    // Convenience function. Will return the cacheblock ordinal that contains the data at 
    // the requested seek point.
    uint64 SeekPointToCacheBlock (uint64 SeekPoint)
    {
        guard
        {
            return (SeekPoint >> BlockSizeLog2);
        } unguard;
    }

    // Stats type info

    // How many blocks can the cache hold? (CacheByteSize / CacheBlockSize)
    uint64 GetCacheBlockCount (void)
    {
        guard
        {
            return (CacheBlockCount);
        } unguard;
    }

    // How big is one cache block, in bytes?
    uint32 GetCacheBlockSize (void);

    // How many blocks are in the file total? (FileByteSize / CacheBlockSize)
    uint64 GetFileBlockCount (void)
    {
        guard
        {
            return (FileBlockCount);
        } unguard;
    }

    // How big, in bytes, is the I/O cache?
    uint32 GetCacheSize (void)
    {
        guard
        {
            return (CacheBlockCount << BlockSizeLog2);
        } unguard;
    }

    // How much memory is the I/O cache currently using?
    uint32 GetCacheMemUsage (void)
    {
        guard
        {
            uint32 Usage;

            Usage = sizeof(*this) + CacheBlocks->GetMemUsage() + (CacheBlocks->GetLength() << BlockSizeLog2);
            return (Usage);
        } unguard;
    }

    // How much memory is the I/O cache allowed to use?
    uint32 GetMaxCacheMemUsage (void)
    {
        guard
        {
            uint32 Usage;

            Usage = sizeof(*this) + CacheBlocks->GetMemUsage() + (CacheBlockCount << BlockSizeLog2);
            return (Usage);
        } unguard;
    }

protected:
    void SetLastError (ErrorType NewError)
    {
        guard
        {
            LastError = NewError;
            LastWin32Error = ::GetLastError ();
            return;
        } unguard;
    }

    // Returns a buffer to be used for a new cache entry
    // Will eject the oldest cache item if necessary, and return the buffer that was 
    // in use instead of freeing and then allocating a new block of memory
    // Otherwise it'll allocate a new block of memory
    uint8 *GetNewCacheBuffer (void);

    ErrorType LastError;
    DWORD LastWin32Error;

    FILETIME FileTime;   // last write time for file
    uint64 FileSize;
    uint64 FilePos;
    string FileName;
    HANDLE FileHandle;
    HANDLE OIOHandle;    // file handle for overlapped I/O

    uint32 BlockSize;
    uint32 BlockSizeLog2;
    uint32 BlockSizeModAnd;

    // Cache infos. Note that the cache doesn't "know" about the end of the file.
    // Bytes past the end of the file are not defined ... could be all zeros, or
    // some random pattern.
    // In other words, if the block size is 4096 bytes and you have a file that
    // is 7168 bytes, then bytes 7169 through 8192 are not defined but are still
    // lumped in with the 2nd block. So if you read the 2nd block with the cache
    // management functions, the last 1024 bytes of the buffer will not be defined.
    // Cache is FIFO, and blocks are bumped to the back of the queue when they
    // are touched. So: front = old, back = new
    typedef struct
    {
        uint8 *Buffer;          // If NULL, then this block is not
        bool   Available;       // used for prefetching/overlapped I/O. Using this we can indicate
                                // that a certain cache block is being read but is not yet available
    } CacheBlockData;

    typedef TreeList<uint64, CacheBlockData, TLIsNegativeOne<uint64>, StaticHeapType> CacheType;

    uint64 FileBlockCount;    // how many blocks are in the file that we can address via the cache?
    uint64 CacheBlockCount;   // the VectorQueue requires that we manually manage the queue size
    CacheType *CacheBlocks;

    // Overlapped I/O stuff

    // Sets up a block for prefetching. However, if it's already in the cache, nothing
    // is done.
    bool SetupBlockPrefetch (uint64 BlockOrdinal);

    // Finishes/cleans up overlapped I/O: call this when you actually read a block
    // that's being prefetched (i.e. Available = false)
    void WaitForOIOCompletion (void);

    // 
    friend void CALLBACK OIOCompletion (DWORD ErrorCode, DWORD BytesRead, LPOVERLAPPED Overlapped);

    int OIOCount; // how many OIO's are in progress?

    CMutex Mutex;      // Mutex to get into any of the library calls

    // In case we want to make a "copy" of ourself ... like to have two views into the same file
    DWORD DesiredAccess;
    DWORD ShareMode;
    DWORD CreationDisposition;
    DWORD FlagsAndAttributes;

    // If SetFilePointerEx is available (Win2K/XP), then use it
    // so we can support really large (4GB+) files
    HMODULE Kernel32DLL;
    SFPExPointer _SetFilePointerEx;

    // Allows us to enable/disable prefetching at will
    bool Prefetching;

    // Only the thread who created the instantiation may issue prefetch requests
    DWORD OwnerThread;
};


#endif // _CFASTFILE_H


