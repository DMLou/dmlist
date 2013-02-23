/*****************************************************************************

	C_Memory

  A class used to make a private heap. Upon deletion all the memory that was
  allocated with this memory heap will be freed.

*****************************************************************************/

#ifndef C_MEMORY_H
#define C_MEMORY_H

#include "C_Linkage.h"
class CAPILINK C_Memory;

#include "C_ObjectHeap.h"
#include "C_ShObj.h"


typedef struct HeapNodeCFType
{
    DWORD  Size; // size of Node + Data
    struct HeapNodeCFType *NextNode;
    struct HeapNodeCFType *PrevNode;
    // data block follows
} HeapNodeCF;


class CAPILINK C_Memory
{
public:
	// if you pass 0 for the heap it'll create a new one
	C_Memory (int nada);
	~C_Memory ();

	void  LockHeap (void);
	void  UnlockHeap (void);
	BOOL  IsHeapLocked (void);

	DWORD GetBlockSize (void *block);

	void *Malloc (DWORD size);
	void *Realloc (void *block, DWORD size);
	void *Calloc (DWORD count, DWORD size);
	void  Free (void *block);
	void  FreeAll (void); // frees all memory and makes a new heap

    DWORD GetBlockCount    (void)  { return (BlockCount); }
	DWORD GetTotalHeapSize (void)  { return (MemUsage + (BlockCount * sizeof (HeapNodeCF))); }

private:
    void  PukeInBlock (void *block); // fill block with garbage
    void  CopyBlock   (void *dst, void *src, DWORD size);

	DWORD  MemUsage;
    DWORD  BlockCount;
	ShObj  MemLock;

    HeapNodeCF *Sentinel;
};


#endif // C_MEMORY_H