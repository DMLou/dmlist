/*****************************************************************************

    TreeList

    Maintains a linked list inside of a binary tree. Useful for having a
    random-access enumerable FIFO queue (in fact, that's what I use it for!)

*****************************************************************************/


#ifndef _TREELIST_H
#define _TREELIST_H


#include <map>
#include "StaticHeap.h"
#include "DynamicHeap.h"


template<class _KeyType, class _DataType>
class TLNodeClass
{
public:
    typedef _KeyType KeyType;
    typedef _DataType DataType;

    KeyType Next;
    KeyType Prev;
    DataType Data;
};


template<class Type>
class TLIsNegativeOne
{
public:
    Type operator() (void) const
    {
        return (Type(-1));
    }
};


template<class _KeyType, 
         class _DataType, 
         class _InvalidKey = TLIsNegativeOne<_KeyType>, 
         class _AllocType = DynamicHeapType>
class TreeList
{
public:
    typedef _KeyType KeyType;
    typedef _DataType DataType;
    typedef TLNodeClass<KeyType, DataType> TLNodeType;
    typedef _InvalidKey InvalidKey;
    typedef std::map<KeyType, TLNodeType, std::less<KeyType>, _AllocType> BTreeType;
    typedef TreeList<_KeyType, _DataType, _InvalidKey, _AllocType> MyType;

    KeyType npos;

protected:
    BTreeType BTree;
    KeyType Head;
    KeyType Tail;
    int MaxLength;
    int Length;

public:
    TreeList (int MaxLength = 2147483647) // not all allocators need a maximum # of nodes, but we need a value anyway
    : BTree (MaxLength)
    {
        guard
        {
            TreeList::MaxLength = MaxLength;
            Length = 0;
            npos = InvalidKey() ();
            Head = npos;
            Tail = npos;
            return;
        } unguard;
    }

    // Copy constructor: guarantees same data in the tree, but not necessarily the exact
    // same binary tree organization
    TreeList (const MyType &Tree)
    {
        guard
        {
            if (this == &Tree)
                return;

            MaxLength = Tree.MaxLength;
            Length = 0;
            npos = InvalidKey() ();
            Head = npos;
            Tail = npos;
            CopyList (*this, Tree);
            return;
        } unguard;
    }

    MyType &operator= (const MyType &rhs)
    {
        guard
        {
            if (this == &rhs)
                return (*this);

            CopyList (*this, rhs);
            return (*this);
        } unguard;
    }

    ~TreeList ()
    {
        guard
        {
            return;
        } unguard;
    }

    // Returns a vector<DataType> containing all the data elements in-order
    vector<DataType> GetDataVector (void) const
    {
        guard
        {
            vector<DataType> Vec;
            KeyType Node;

            Node = GetHeadKey ();

            while (Node != npos)
            {
                Vec.push_back (*Get (Node));
                Node = GetNextKey (Node);
            }

            return (Vec);
        } unguard;
    }

    // Returns a vector<KeyType> containing all the keys in-order
    vector<KeyType> GetKeyVector (void) const
    {
        guard
        {
            vector<KeyType> Vec;
            KeyType Node;

            Node = GetHeadKey ();
            
            while (Node != npos)
            {
                Vec.push_back (Node);
                Node = GetNextKey (Node);
            }

            return (Vec);
        } unguard;
    }

    // Performs lhs = rhs (assignment/copy)
    void CopyList (MyType &lhs, const MyType &rhs) const
    {
        guard
        {
            KeyType NodeKey;

            lhs.Clear ();
            NodeKey = rhs.GetHeadKey ();

            while (NodeKey != rhs.npos)
            {
                MyType::DataType *NodeData;

                NodeData = rhs.Get (NodeKey);
                lhs.Push (NodeKey, *NodeData);
                NodeKey = rhs.GetNextKey (NodeKey);
            }

            return;
        } unguard;
    }

    bool operator== (const MyType &rhs) const
    {
        guard
        {
            KeyType lhsNode;
            KeyType rhsNode;

            if (this == &rhs)
                return (true);

            if (GetLength() != rhs.GetLength())
                return (false);

            lhsNode = GetHeadKey ();
            rhsNode = rhs.GetHeadKey ();

            while (lhsNode != npos  &&  rhsNode != npos)
            {
                if (lhsNode != rhsNode)
                    return (false);

                if (*Get(lhsNode) != *rhs.Get(rhsNode))
                    return (false);

                lhsNode = GetNextKey (lhsNode);
                rhsNode = rhs.GetNextKey (rhsNode);
            }

            return (true);
        } unguard;
    }

    bool operator!= (const MyType &rhs) const
    {
        guard
        {
            return (!(*this == rhs));
        } unguard;
    }

    int GetMemUsage (void) const
    {
        guard
        {
            return (BTree.size() * (sizeof(KeyType) + sizeof(DataType)));
        } unguard;
    }

    int GetLength (void) const
    {
        guard
        {
            return (Length);
        } unguard;
    }

    bool IsEmpty (void) const
    {
        guard
        {
            if (Length == 0)
                return (true);

            return (false);
        } unguard;
    }

    bool IsFull (void) const
    {
        guard
        {
            if (Length >= MaxLength)
                return (true);

            return (false);
        } unguard;
    }

    int GetMaxLength (void) const
    {
        guard
        {
            return (MaxLength);
        } unguard;
    }

    // Moves an item from its current queue position to the end
    // This one does not change the data associated with this key
    bool BumpElement (const KeyType Key)
    {
        guard
        {
            TLNodeType *DataPtr;
            DataType DataTemp;

            if (Tail == Key)
                return (true);

            BTreeType::iterator found = BTree.find(Key);
            DataPtr = &(found->second);

            if (DataPtr == NULL)
                return (false);

            DataTemp = DataPtr->Data;

            if (!Remove (Key))
                return (false);

            if (!Push (Key, DataTemp))
                return (false);

            return (true);
        } unguard;
    }

    // This one does update the data
    bool BumpElement (const KeyType Key, const DataType NewData)
    {
        guard
        {
            TLNodeType *DataPtr;

            if (Tail == Key)
                return (true);

            BTreeType::iterator found = BTree.find(Key);
            DataPtr = &(found->second);

            if (DataPtr == NULL)
                return (false);

            if (!Remove (Key))
                return (false);

            if (!Push (Key, NewData))
                return (false);

            return (true);
        } unguard;
    }

    // Adds this element to the end of the queue
    bool Push (const KeyType Key, const DataType Data)
    {
        guard
        {
            TLNodeType E;

            if (IsFull())
                return (false);

            // 1) No items in queue. Create the queue pointing to this element.
            if (Head == npos)
            {
                Head = Key;
                Tail = Key;
                E.Next = npos;
                E.Prev = npos;
                E.Data = Data;
                Length = 1;
                BTree[Key] = E;
            }
            else
            {
                TLNodeType *P = NULL;
                TLNodeType *TailPtr;

                BTreeType::iterator found = BTree.find(Key);
                if (found != BTree.end())
                {
                    P = &(found->second);
                }

                // 2) This item is not part of the queue. Add it to the end of the queue.
                if (P == NULL)
                {
                    BTreeType::iterator found = BTree.find(Tail);
                    TailPtr = &(found->second);
                    TailPtr->Next = Key;
                    E.Prev = Tail;
                    E.Next = npos;
                    E.Data = Data;
                    Tail = Key;
                    BTree[Key] = E;
                    Length++;
                }
                else
                // 3) This item is already part of the queue. Remove it from the queue,
                //    and re-add it.
                {
                    BumpElement (Key, Data);
                }
            }

            return (true);
        } unguard;
    }

    // Removes element at front of queue. Returns true on success, false on failure (list is empty)
    bool Pop (void)
    {
        guard
        {
            return (Remove (GetHeadKey ()));
        } unguard;
    }

    // Removes any item from the queue
    bool Remove (const KeyType Key)
    {
        guard
        {
            TLNodeType *E;
            TLNodeType *P;
            TLNodeType *N;
            TLNodeType *H;
            TLNodeType *T;

            // 1. Queue is empty. Do nothing.
            if (IsEmpty())
                return (false);

            BTreeType::iterator found = BTree.find(Key);
            E = &(found->second);

            // 2. Item is not in the queue. Do nothing.
            if (E == NULL)
                return (false);
            else
            // 3. This item is the head of the queue and/or this item is the only item in the queue
            if (Head == Key)
            {
                if (E->Next == npos) // item was the only item in the queue!
                    Tail = npos; // queue is now empty!
                else
                {
                    BTreeType::iterator found = BTree.find(E->Next);
                    H = &(found->second);
                    H->Prev = npos;
                }

                Head = E->Next;
            }
            else
            // 4. This item is the tail of the queue with previous items in the queue
            if (Tail == Key)
            {
                Tail = E->Prev;
                BTreeType::iterator found = BTree.find(Tail);
                T = &(found->second);
                T->Next = npos;
            }
            else
            // 5. This item is in the middle of the queue somewhere.
            {
                BTreeType::iterator foundP = BTree.find(E->Prev);
                P = &(foundP->second);
                P->Next = E->Next;
                BTreeType::iterator foundN = BTree.find(E->Next);
                N = &(foundN->second);
                N->Prev = E->Prev;
            }

            BTree.erase(Key);
            Length--;
            return (true);
        } unguard;
    }

    KeyType GetHeadKey (void) const
    {
        guard
        {
            return (Head);
        } unguard;
    }

    KeyType GetTailKey (void) const
    {
        guard
        {
            return (Tail);
        } unguard;
    }

    KeyType GetNextKey (const KeyType &Key) const
    {
        guard
        {
            const TLNodeType *Node = NULL;

            if (Key == npos)
                return (npos);

            BTreeType::const_iterator found = BTree.find(Key);
            if (found != BTree.end())
            {
                Node = &(found->second);
            }

            if (Node != NULL)
                return (Node->Next);

            return (npos);
        } unguard;
    }

    KeyType GetPrevKey (const KeyType &Key) const
    {
        guard
        {
            TLNodeType *Node;

            if (Key == npos)
                return (npos);

            Node = BTree.SearchPtr (Key);

            if (Node != NULL)
                return (Node->Prev);

            return (npos);
        } unguard;
    }

    DataType *GetHead (void) const
    {
        guard
        {
            return (Get (Head));
        } unguard;
    }

    DataType *GetTail (void) const
    {
        guard
        {
            return (Get (Qail));
        } unguard;
    }

    DataType *Get (const KeyType &Key) const
    {
        guard
        {
            const TLNodeType *Node = NULL;

            BTreeType::const_iterator found = BTree.find(Key);

            if (found != BTree.end())
            {
                Node = &(found->second);
            }

            if (Node != NULL)
                return const_cast<DataType*>(&Node->Data);

            return (NULL);
        } unguard;
    }

    bool RemoveHead (void)
    {
        guard
        {
            return (Remove (Head));
        } unguard;
    }

    bool RemoveTail (void)
    {
        guard
        {
            return (Remove (Tail));
        } unguard;
    }

    void Clear (void)
    {
        guard
        {
            while (!IsEmpty())
                Remove (Tail);

            return;
        } unguard;
    }
};


#endif // _TREELIST_H
