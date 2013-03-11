/*****************************************************************************

    DynamicHeap

    Provides a dynamic heap from which to allocate objects of a certain type.
    All items in the heap are NOT destroyed and freed upon destruction of the
    heap. That is the client's responsibility.

    Conforms to the STL allocator class

*****************************************************************************/


#ifndef _DYNAMICHEAP_H
#define _DYNAMICHEAP_H


#include <memory>
#include <limits>
#include "guard.h"


template<class Type>
class DynamicHeap
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
        typedef DynamicHeap<Other> other;
    };

    inline DynamicHeap (const int Maximum = 0)
    {
        guard
        {
            ObjectCount = 0;
            return;
        } unguard;
    }

    inline ~DynamicHeap ()
    {
        guard
        {
            return;
        } unguard;
    }

    inline explicit DynamicHeap(DynamicHeap const&) { }
    
    template<typename Type>
    inline explicit DynamicHeap(DynamicHeap<Type> const&) { }

    // address
    inline pointer address(reference r) { return &r; }
    inline const_pointer address(const_reference r) { return &r; }

    // memory allocation
    inline pointer allocate(size_type cnt,
        typename std::allocator<void>::const_pointer = 0)
    {
        guard
        {
            ObjectCount++;
            return (new Type);
        } unguard;
    }

    inline void deallocate(pointer Ptr, size_type)
    {
        guard
        {
            delete Ptr;
            ObjectCount--;
            return;
        } unguard;
    }

    // size
    inline size_type max_size() const
    {
#undef max
        return std::numeric_limits<size_type>::max() / sizeof(Type);
    }

    // construction/destruction
    inline void construct(pointer p, const Type& t) { new(p) Type(t); }

    inline void destruct(pointer p) { p->~Type(); }

    inline bool operator==(DynamicHeap const&) { return true; }
    inline bool operator!=(DynamicHeap const& a) { return !operator==(a); }

    // Original code
    int GetMemUsage (void) const
    {
        guard
        {
            return (ObjectCount * sizeof(value_type));
        } unguard;
    }

    int GetObjectCount (void) const
    {
        guard
        {
            return (ObjectCount);
        } unguard;
    }

protected:
    int ObjectCount;
};


typedef DynamicHeap<void *> DynamicHeapType;


#endif // _DYNAMICHEAP_H