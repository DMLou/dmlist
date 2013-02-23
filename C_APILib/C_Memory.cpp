#include "C_Memory.h"
#include <stdio.h>
#include <malloc.h>

#undef malloc
#undef free


#define GARBAGEb 0xde
#define GARBAGEw 0xdead
#define GARBAGEl 0xdeadbeef
const UCHAR GARBAGE[] = { 0xDE, 0xAD, 0xBE, 0xEF };


// Get node from block
#define GETNODE(block) (HeapNodeCF *)((char *)(block) - sizeof (HeapNodeCF))
// Get block from node
#define GETBLOCK(node) ((char *)(node) + sizeof (HeapNodeCF))


#define malloc(size) _aligned_malloc(size,16)
#define free(block) _aligned_free(block)
#define realloc(block,size) _aligned_realloc(block,size,16)


C_Memory::C_Memory (int nada)
{
    InitShObject (&MemLock, TRUE);

    // Allocate the sentinel node. This will be a circular doubly-linked list.
    Sentinel = new HeapNodeCF;
    Sentinel->NextNode = Sentinel;
    Sentinel->PrevNode = Sentinel;
    Sentinel->Size = 0;

	MemUsage = 0;
    BlockCount = 1;

    UnlockHeap ();
}


C_Memory::~C_Memory ()
{
    HeapNodeCF *Node;

	LockHeap ();

    // traverse node and free all blocks
    if (Sentinel != NULL)
    {
        Node = Sentinel->NextNode;
        while (Node != Sentinel)
        {
            HeapNodeCF *FreeMe;
            FreeMe = Node;
            Node = Node->NextNode;
            free (FreeMe);
        }
    
        delete Sentinel;
        Sentinel = NULL;
    }

    DestroyShObject (&MemLock);

    return;
}


void C_Memory::LockHeap (void)
{
    LockShObject (&MemLock);
}


void C_Memory::UnlockHeap (void)
{
    UnlockShObject (&MemLock);
}


// fills a block with garbage
void C_Memory::PukeInBlock (void *block)
{
    DWORD Size;

    Size = GetBlockSize (block);
    if (Size != 0 && block != NULL)
    {
        UCHAR *Dst;
        DWORD i;
        DWORD j = 0;

        Dst = (UCHAR *) block;
        for (i = 0; i < Size; i++)
        {
            j = i & 3;
            Dst[i] = GARBAGE[j];
        }
    }

    return;
}


void C_Memory::CopyBlock (void *dst, void *src, DWORD size)
{
    memcpy (dst, src, size);
    return;
}


void *C_Memory::Malloc (DWORD size)
{
    HeapNodeCF *NewNode;

    if (size == 0)
        return (NULL);

    LockHeap ();

    NewNode = (HeapNodeCF *) malloc (size + sizeof (HeapNodeCF));
    if (NewNode == NULL)
    {
        UnlockHeap ();
        return (NULL); 
    }

    NewNode->NextNode = Sentinel->NextNode;
    NewNode->PrevNode = Sentinel;
    Sentinel->NextNode->PrevNode = NewNode;
    Sentinel->NextNode = NewNode;

    NewNode->Size = size;
    MemUsage += size;
    BlockCount++;

    UnlockHeap ();

    return (GETBLOCK(NewNode));
}


void *C_Memory::Calloc (DWORD count, DWORD size)
{
    void *Block;

    Block = Malloc (size * count);
    memset (Block, 0, size * count);
    return (Block);
}


void C_Memory::Free (void *block)
{
    HeapNodeCF *Node;
    HeapNodeCF *FreeMe;

    if (block == NULL)
        return;

    if (block == Sentinel)
        return;

    LockHeap ();

    FreeMe = 
        Node = GETNODE(block);
    Node->PrevNode->NextNode = Node->NextNode;
    Node->NextNode->PrevNode = Node->PrevNode;

    MemUsage -= FreeMe->Size;
    BlockCount--;

    free (FreeMe);

    UnlockHeap ();

    return;
}


void *C_Memory::Realloc (void *block, DWORD newsize)
{
    HeapNodeCF *OldNode;
    HeapNodeCF *NewNode;

    if (block == NULL)
        return (Malloc (newsize));

    if (newsize == 0)
    {
        Free (block);
        return (NULL);
    }

    LockHeap ();

    OldNode = GETNODE(block);
    NewNode = (HeapNodeCF *) realloc (OldNode, newsize + sizeof (HeapNodeCF));
    if (OldNode != NewNode) // if block has moved
    {
        NewNode->PrevNode->NextNode = NewNode;
        NewNode->NextNode->PrevNode = NewNode;
    }

    MemUsage -= NewNode->Size;
    MemUsage += newsize;
    NewNode->Size = newsize;

    UnlockHeap ();

    return (GETBLOCK(NewNode));
}


DWORD C_Memory::GetBlockSize (void *block)
{
    HeapNodeCF *Node;
    DWORD    Size;

    LockHeap ();

    Node = GETNODE(block);
    Size = Node->Size;

    UnlockHeap ();

    return (Size);
}

#if 0

// Checks if the pointer is valid for *THIS* heap.
// This function won't be fast if you have a lot of blocks that you've allocated
bool C_Memory::IsPtrValid (void *block)
{
    bool      Valid = false;
    HeapNodeCF *Node;

    LockHeap ();

    Node = Sentinel->NextNode;
    while (Node != Sentinel)
    {
        if (block >= GETBLOCK(Node)  &&  block < (GETBLOCK(Node) + Node->Size))
        {
            Valid = true;
            break;
        }

        Node = Node->NextNode;
    }

    UnlockHeap ();
    return (Valid);
}


#endif