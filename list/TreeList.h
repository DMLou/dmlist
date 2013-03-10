/*****************************************************************************

    TreeList

    Maintains a linked list inside of a binary tree. Useful for having a
    random-access enumerable FIFO queue (in fact, that's what I use it for!)

*****************************************************************************/


#ifndef _TREELIST_H
#define _TREELIST_H


#include <map>
#include <list>
#include <memory>
#include "StaticHeap.h"


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
         class _AllocType = std::allocator<void *> >
class TreeList
{
public:
    typedef _KeyType KeyType;
    typedef _DataType DataType;
    typedef std::pair<KeyType, DataType> TLNodeType;
    typedef _InvalidKey InvalidKey;
    typedef std::list<TLNodeType> ListType;
    typedef std::map<KeyType, typename ListType::iterator, std::less<KeyType>, _AllocType> BTreeType;
    typedef TreeList<_KeyType, _DataType, _InvalidKey, _AllocType> MyType;

    KeyType m_npos;

protected:
    BTreeType m_tree;
    ListType m_list;
    int m_maxLength;

public:
    TreeList (int MaxLength = 2147483647) // not all allocators need a maximum # of nodes, but we need a value anyway
    {
        guard
        {
            m_maxLength = MaxLength;
            m_npos = InvalidKey() ();
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

            m_maxLength = Tree.m_maxLength;
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
            vector<DataType> vec;
            ListType::iterator it;

            for (it = m_list.begin(); it != m_list.end(); ++it)
            {
                vec.push_back(*it);
            }

            return (vec);
        } unguard;
    }

    // Returns a vector<KeyType> containing all the keys in-order
    vector<KeyType> GetKeyVector (void) const
    {
        guard
        {
            vector<KeyType> vec;
            BTreeType::iterator it;

            for (it = BTree.begin(); it != BTree.end(); ++it)
            {
                vec.push_back(it->first);
            }

            return (vec);
        } unguard;
    }

    // Performs lhs = rhs (assignment/copy)
    void CopyList (MyType &lhs, const MyType &rhs) const
    {
        guard
        {
            lhs.Clear ();

            // As the key is stored in both the list and the tree, we can use that
            // to make copying them much more efficient.
            MyType::ListType::const_iterator rhsIt;
            for (rhsIt = rhs.m_list.begin(); rhsIt != rhs.m_list.end(); ++rhsIt)
            {
                MyType::TLNodeType node = *rhsIt;
                lhs.m_list.push_back(node);
                MyType::ListType::iterator it = lhs.m_list.end();
                --it;
                lhs.m_tree.insert(std::pair<MyType::KeyType, typename MyType::ListType::iterator>(node.first, it));
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

            MyType::BTreeType::iterator lhsIt, rhsIt;
            lhsIt = BTree.begin();
            rhsIt = rhs.BTree.begin();

            while (lhsIt != BTree.end() && rhsIt != rhs.BTree.end())
            {
                // Check if keys match
                if (lhsIt->first != rhsIt->first)
                {
                    return false;
                }

                // Check if values match
                if (lhsIt->second != rhsIt->second)
                {
                    return false;
                }

                ++lhsIt;
                ++rhsIt;
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
            int treeSize = BTree.size() * (sizeof(KeyType) + sizeof(ListType::iterator));
            int listSize = m_list.size() * sizeof(TLNodeType);
            return (treeSize + listSize);
        } unguard;
    }

    int GetLength (void) const
    {
        guard
        {
            return m_list.size();
        } unguard;
    }

    bool IsEmpty (void) const
    {
        guard
        {
            if (m_list.empty() && BTree.empty())
            {
                return (true);
            }

            return (false);
        } unguard;
    }

    bool IsFull (void) const
    {
        guard
        {
            if (m_list.size() >= m_maxLength)
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
            BTreeType::iterator found = m_tree.find(Key);
            if (found == m_tree.end())
            {
                return false;
            }

            // Check if the key points to the tail
            if (found->second == (m_list.end() - 1))
            {
                return true;
            }

            TLNodeType node = *(found->second);
            DataType dataTemp = node.second;

            if (!Remove (Key))
                return (false);

            if (!Push (Key, dataTemp))
                return (false);

            return (true);
        } unguard;
    }

    // This one does update the data
    bool BumpElement (const KeyType Key, const DataType NewData)
    {
        guard
        {
            BTreeType::iterator found = m_tree.find(Key);
            if (found == m_tree.end())
            {
                return false;
            }

            // Check if the key points to the tail
            if (found->second == (m_list.end())--)
            {
                return true;
            }

            TLNodeType node = *(found->second);
            DataType dataTemp = node.second;

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
            TLNodeType node(Key, Data);

            if (IsFull())
                return (false);

            // 1) No items in queue. Create the queue pointing to this element.
            if (m_list.empty())
            {
                m_list.push_back(node);
                m_tree[Key] = m_list.begin(); // Rmember, only one on the list
            }
            else
            {
                BTreeType::iterator found = m_tree.find(Key);

                // 2) This item is not part of the queue. Add it to the end of the queue.
                if (found == m_tree.end())
                {
                    m_list.push_back(node);
                    ListType::iterator it = m_list.end();
                    --it;
                    m_tree[Key] = it;
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
            TLNodeType node = m_list.front();
            node.pop_front();
            m_tree.erase(node.first);
        } unguard;
    }

    // Removes any item from the queue
    bool Remove (const KeyType Key)
    {
        guard
        {
            // 1. Queue is empty. Do nothing.
            if (IsEmpty())
                return (false);

            BTreeType::iterator found = m_tree.find(Key);

            // 2. Item is not in the queue. Do nothing.
            if (found == m_tree.end())
                return (false);
            else
            {
                // 3. This item is somewhere in the queue. Delete it.
                m_list.erase(found->second);
                m_tree.erase(found);
            }

            return (true);
        } unguard;
    }

    KeyType GetHeadKey (void) const
    {
        guard
        {
            return m_list.front().first;
        } unguard;
    }

    KeyType GetTailKey (void) const
    {
        guard
        {
            return m_list.back().first;
        } unguard;
    }

    KeyType GetNextKey (const KeyType &Key) const
    {
        guard
        {
            if (Key == m_npos)
                return (m_npos);

            BTreeType::const_iterator found = m_tree.find(Key);
            
            if (found != m_tree.end())
            {
                ListType::iterator it = found->second;
                ++it;

                // If we don't go off the rails, we're good
                if (it != m_list.end())
                {
                    return it->first;
                }
            }

            return (m_npos);
        } unguard;
    }

    KeyType GetPrevKey (const KeyType &Key) const
    {
        guard
        {
            if (Key == m_npos)
                return (m_npos);

            BTreeType::const_iterator found = m_tree.find(Key);
            
            if (found != m_tree.end())
            {
                ListType::iterator it = found->second;

                // If we don't go off the rails, we're good
                if (it != m_list.begin())
                {
                    --it;
                    return it->first;
                }
            }

            return (npos);
        } unguard;
    }

    DataType *GetHead (void) const
    {
        guard
        {
            return &(m_list.front().second);
        } unguard;
    }

    DataType *GetTail (void) const
    {
        guard
        {
            return &(m_list.back().second);
        } unguard;
    }

    DataType *Get (const KeyType &Key) const
    {
        guard
        {
            TLNodeType Node;

            BTreeType::const_iterator found = m_tree.find(Key);
            if (found == m_tree.end())
            {
                return NULL;
            }

            return &(found->second->second);
        } unguard;
    }

    bool RemoveHead (void)
    {
        guard
        {
            return (Remove(m_list.front().first));
        } unguard;
    }

    bool RemoveTail (void)
    {
        guard
        {
            return (Remove(m_list.back().first));
        } unguard;
    }

    void Clear (void)
    {
        guard
        {
            m_list.clear();
            m_tree.clear();

            return;
        } unguard;
    }
};


#endif // _TREELIST_H
