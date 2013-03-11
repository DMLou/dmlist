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
    typedef Type value_type;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;

    template<class Other>
    struct rebind
    {
        typedef StaticHeap<Other> other;
    };

    StaticHeap (const int Maximum)
    {
        guard
        {
            int i;

            Heap = (value_type *) new uint8[sizeof(value_type) * Maximum];
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

    inline explicit StaticHeap(StaticHeap const&) { }
    
    template<typename Type>
    inline explicit StaticHeap(StaticHeap<Type> const&) { }

    // address
    inline pointer address(reference r) { return &r; }
    inline const_pointer address(const_reference r) { return &r; }

    // memory allocation
    inline pointer allocate(size_type cnt,
        typename std::allocator<void>::const_pointer = 0)
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

    void deallocate(pointer Ptr, size_type)
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

    // size
    inline size_type max_size() const
    {
        return MaxObjects * sizeof(Type);
    }

    // construction/destruction
    inline void construct(pointer p, const Type& t) { new(p) Type(t); }

    inline void destruct(pointer p) { p->~Type(); }

    inline bool operator==(StaticHeap const&) { return true; }
    inline bool operator!=(StaticHeap const& a) { return !operator==(a); }

    // Original code

    int GetMemUsage (void) const
    {
        guard
        {
            int HeapSize;

            // Reported memory usage is rounded up to the next 4096 byte boundary (x86 page size = 4K)
            HeapSize = (sizeof(bool) + sizeof(int) + sizeof(value_type)) * MaxObjects;
            return (sizeof(*this) + HeapSize);
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