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
#include "guard.h"


template<class Type>
class DynamicHeap
{
public:
    typedef Type DataType;

    template<class Other>
    struct Rebind
    {
        typedef DynamicHeap<Other> ReboundType;
    };

    DynamicHeap (const int Maximum)
    {
        guard
        {
            ObjectCount = 0;
            return;
        } unguard;
    }

    ~DynamicHeap ()
    {
        guard
        {
            return;
        } unguard;
    }

    int GetMemUsage (void) const
    {
        guard
        {
            return (ObjectCount * sizeof(DataType));
        } unguard;
    }

    int GetObjectCount (void) const
    {
        guard
        {
            return (ObjectCount);
        } unguard;
    }

    Type *Allocate (void)
    {
        guard
        {
            ObjectCount++;
            return (new Type);
        } unguard;
    }

    void Free (Type *Ptr)
    {
        guard
        {
            delete Ptr;
            ObjectCount--;
            return;
        } unguard;
    }

protected:
    int ObjectCount;
};


typedef DynamicHeap<void *> DynamicHeapType;


#endif // _DYNAMICHEAP_H