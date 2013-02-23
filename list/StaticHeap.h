/*****************************************************************************

    StaticHeap

    Provides a static heap from which to allocate objects of a certain type
    (that is, all objects are the same size, in bytes). When you create the
    heap you must specify a maximum number of objects that can be allocated.

    Used in the cache systems.

    All items in the heap are destroyed and freed upon destruction of the
    heap.

*****************************************************************************/


#ifndef _STATICHEAP_H
#define _STATICHEAP_H


#include <stack>
#include <memory>


template<class Type>
class StaticHeap
{
public:
    typedef Type DataType;

    template<class Other>
    struct Rebind
    {
        typedef StaticHeap<Other> ReboundType;
    };

    StaticHeap (const int Maximum)
    {
        guard
        {
            int i;

            Heap = (DataType *) new uint8[sizeof(DataType) * Maximum];
            FreeList = new int[Maximum];
            DestroyList = new bool[Maximum];

            MaxObjects = Maximum;

            for (i = 0; i < Maximum; i++)
            {
                DestroyList[i] = false;
                FreeList[Maximum - i - 1] = i;
            }

            FreeToS = Maximum;

            return;
        } unguard;
    }

    ~StaticHeap ()
    {
        guard
        {
            int i;

            for (i = 0; i < MaxObjects; i++)
            {
                if (DestroyList[i])
                    Allocator.destroy (&Heap[i]);
            }

            delete DestroyList;
            delete FreeList;
            delete ((uint8 *)Heap);

            return;
        } unguard;
    }

    int GetMemUsage (void) const
    {
        guard
        {
            int HeapSize;

            // Reported memory usage is rounded up to the next 4096 byte boundary (x86 page size = 4K)
            HeapSize = (sizeof(bool) + sizeof(int) + sizeof(DataType)) * MaxObjects;
            return (sizeof(*this) + HeapSize);
        } unguard;
    }

    Type *Allocate (void)
    {
        guard
        {
            int indice;

            if (FreeListSize() == 0)
                return (NULL);
            else
            {
                indice = FreeListTop();
                FreeListPop ();
            }

            Allocator.construct (&Heap[indice], Type());
            DestroyList[indice] = true;
            
            return (&Heap[indice]);
        } unguard;
    }

    void Free (Type *Ptr)
    {
        guard
        {
            int indice;

            indice = (int)(Ptr - Heap);
            Allocator.destroy (&Heap[indice]);
            DestroyList[indice] = false;
            FreeListPush (indice);

            return;
        } unguard;
    }

protected:
    int FreeListSize (void) const
    {
        guard
        {
            return ((int)(&FreeList[FreeToS] - FreeList));
        } unguard;
    }

    void FreeListPush (int New)
    {
        guard
        {
            FreeList[FreeToS] = New;
            FreeToS++;
            return;
        } unguard;
    }

    void FreeListPop (void)
    {
        guard
        {
            FreeToS--;
            return;
        } unguard;
    }

    int &FreeListTop (void) const
    {
        guard
        {
            return (FreeList[FreeToS - 1]);
        } unguard;
    }

    std::allocator<Type> Allocator;
    Type *Heap;
    int *FreeList;
    bool *DestroyList;
    int FreeToS; // FreeList Top of Stack indice
    int MaxObjects;
};


typedef StaticHeap<void *> StaticHeapType;


#endif // _STATICHEAP_H