/*****************************************************************************

    AVLBTree

    Implements an AVL Binary Tree.
    The destructor will empty the tree first, thus releasing all memory used
    via the Allocator.

    The three functions you should be concerned with when using this class
    are Insert, Remove, and SearchPtr.

    This works as a mapping from a domain (key) to a range (data), and it
    you may only pair one data value with one key. Thus it is truly a 
    functional mapping.

    Adapted from http://www.purists.org/georg/avltree/

*****************************************************************************/


#ifndef _AVLBTREE_H
#define _AVLBTREE_H


#include "DynamicHeap.h"
#include <functional>
#include "guard.h"


typedef struct NullStruct
{

} NullStruct;


// Regular node; used by default
template<class _KeyType, class _DataType>
class AVLBTreeNode
{
public:
    typedef _KeyType KeyType;
    typedef _DataType DataType;
    typedef AVLBTreeNode<KeyType, DataType> MyType;

    const KeyType &GetKey (void)
    {
        guard
        {
            return (KnD.Key);
        } unguard;
    }

    void SetKey (const KeyType &NewKey)
    {
        guard
        {
            KnD.Key = NewKey;
            return;
        } unguard;
    }

    const DataType &GetData (void)
    {
        guard
        {
            return (KnD.Data);
        } unguard;
    }

    void SetData (const DataType &NewData)
    {
        guard
        {
            KnD.Data = NewData;
            return;
        } unguard;
    }

    DataType *GetDataPtr (void)
    {
        guard
        {
            return (&KnD.Data);
        } unguard;
    }

    // "Key 'n Data" ... we define this type so we can do atomic assignment (atomic in terms of lines of code)
    typedef struct MyKnD
    {
        KeyType Key;
        DataType Data;
    } MyKnD;

    MyType *Left;   // left child
    MyType *Right;  // right child
    int Balance;    // <0 for "left is heavier", >0 for "right is heavier", 0 for balanced
    MyKnD KnD;
};


// Node that stores no 'data' member. Can save memory if you need not associate data with your keys
template<class _KeyType>
class AVLBTreeNode_NoData
{
public:
    typedef _KeyType KeyType;
    typedef _KeyType DataType;
    typedef AVLBTreeNode_NoData<KeyType> MyType;

    const KeyType &GetKey (void) const
    {
        guard
        {
            return (KnD.Key);
        } unguard;
    }

    void SetKey (const KeyType &NewKey)
    {
        guard
        {
            KnD.Key = NewKey;
            return;
        } unguard;
    }

    // This will cause a compile-time error if you use it.
    const DataType &GetData (void) const
    {
        guard
        {
            return (KnD.Data);
        } unguard;
    }

    void SetData (const DataType &NewData)
    {
        guard
        {
            return;
        } unguard;
    }

    DataType *GetDataPtr (void)
    {
        guard
        {
            return (&MyKnD.Key);
        } unguard;
    }

    // "Key 'n Data" ... we define this type so we can do atomic assignment (atomic in terms of lines of code)
    typedef struct MyKnD
    {
        KeyType Key;
    } MyKnD;

    MyType *Left;   // left child
    MyType *Right;  // right child
    int Balance;    // <0 for "left is heavier", >0 for "right is heavier", 0 for balanced
    MyKnD KnD;
};


// The AVL binary tree class
// Requires you to provide a key type (domain), a data type (range), an allocator
// (not an STL allocator, but either StaticHeapType or DynamicHeapType), a LessThan
// predicate to provide ordering, and a NodeType
// If you use the data-less node type, then it doesn't matter what you use for
// DataType
template<class _KeyType, 
         class _DataType,
         class _AllocType = DynamicHeap<NullStruct>,  // we rebind this anyway
         class _LessThan = std::less<_KeyType>,
         class _NodeType = AVLBTreeNode<_KeyType, _DataType> >
class AVLBTree
{
public:
    typedef _KeyType KeyType;
    typedef _DataType DataType;
    typedef _NodeType NodeType;
    typedef _LessThan LessThan;
    typedef typename _AllocType::Rebind<NodeType>::ReboundType AllocType;
    typedef AVLBTree<KeyType, DataType, AllocType, LessThan, NodeType> MyType;

    // Rebind structure: You can use this to create rebound binary tree types
    template<class OtherKeyType, 
             class OtherDataType,
             class OtherAllocType = AllocType, 
             class OtherLessThan = LessThan,
             class OtherNodeType = NodeType>
    struct Rebind
    {
        AVLBTree<OtherKeyType, OtherDataType, OtherAllocType, OtherLessThan, OtherNodeType> ReboundType;
    };

protected:
    AllocType Allocator;
    NodeType *TreeRoot;
    LessThan LT;

public:
    AVLBTree (const int MaxCount = 0)
        : Allocator (MaxCount)
    {
        guard
        {
            TreeRoot = NULL;
            return;
        } unguard;
    }

    ~AVLBTree ()
    {
        guard
        {
            while (!IsEmpty())
                _Remove (TreeRoot, TreeRoot->GetKey());

            return;
        } unguard;
    }

    // Does not account for sizeof(*this) in return value ...
    // for "total" memory usage use sizeof(MyType) + GetMemUsage
    int GetMemUsage (void) const
    {
        guard
        {
            return (Allocator.GetMemUsage());
        } unguard;
    }

    unsigned int GetNodeCount (void) const
    {
        guard
        {
            return (Allocator.GetObjectCount());
        } unguard;
    }

    bool IsEmpty (void) const
    {
        guard
        {
            if (TreeRoot == NULL)
                return (true);

            return (false);
        } unguard;
    }

    NodeType *AllocateNode (const KeyType &Key, const DataType &Data)
    {
        guard
        {
            NodeType *Return;

            Return = Allocator.Allocate ();
            Return->Left = NULL;
            Return->Right = NULL;
            Return->Balance = 0;
            Return->SetKey (Key);
            Return->SetData (Data);

            return (Return);
        } unguard;
    }

    void FreeNode (NodeType *Node)
    {
        guard
        {
            Allocator.Free (Node);
            return;
        } unguard;
    }

    // Returns a pointer to the key associated with the root of the tree
    const KeyType *GetRootKey (void) const
    {
        guard
        {
            if (IsEmpty())
                return (NULL);
            else
                return (&TreeRoot->GetKey());
        } unguard;
    }

    // Used for navigating tree
    // Returns NULL if there is no left subtree
    const KeyType *GetLeftKey (const KeyType &Key) const
    {
        guard
        {
            NodeType *Node;

            Node = _SearchPtr (Key, TreeRoot);

            if (Node != NULL  &&  Node->Left != NULL) 
                return (&Node->Left->GetKey());

            return (NULL);
        } unguard;
    }

    const KeyType *GetRightKey (const KeyType &Key) const
    {
        guard
        {
            NodeType *Node;

            Node = _SearchPtr (Key, TreeRoot);

            if (Node != NULL  &&  Node->Right != NULL) 
                return (&Node->Right->GetKey());

            return (NULL);
        } unguard;
    }

    // Removes all elements from the tree
    void Clear (void)
    {
        guard
        {
            while (!IsEmpty())
                _Remove (TreeRoot, TreeRoot->GetKey());

            return;
        } unguard;
    }

    // *DataResult = pointer to data associated with requested key, if found
    // NULL otherwise. Using this pointer you may change the data associated
    // with a given key.
    DataType *SearchPtr (const KeyType &Key) const
    {
        guard
        {
            NodeType *Node;

            Node = _SearchPtr (Key, TreeRoot);
            
            if (Node != NULL)
                return (Node->GetDataPtr());

            return (NULL);
        } unguard;
    }

    // Returns a pointer to the node that has the requested key, or NULL if it's not in the tree
    NodeType *_SearchPtr (const KeyType &Key, NodeType *NodeStart) const
    {
        guard
        {
            NodeType *Node;

            Node = NodeStart;

            while (Node != NULL)
            {
                if (LT (Key, Node->KnD.Key))
                    Node = Node->Left;
                else
                if (LT (Node->KnD.Key, Key))
                    Node = Node->Right;
                else
                    break;
            }

            return (Node);
        } unguard;
    }

    // Returns the height of a given node. H(Node) = 1 + max (H(Node->Left), H(Node->Right))
    int _GetHeight (const NodeType *StartNode) const
    {
        guard
        {
            int LeftHeight;
            int RightHeight;

            if (StartNode == NULL)
                return (0);

            LeftHeight = 1 + _GetHeight (StartNode->Left);
            RightHeight = 1 + _GetHeight (StartNode->Right);

            if (LeftHeight > RightHeight)
                return (LeftHeight);

            return (RightHeight);            
        } unguard;
    }

    int GetHeight (void) const
    {
        return (_GetHeight (TreeRoot));
    }

    void SingleLeftRotation (NodeType *&Node)
    {
        guard
        {
            NodeType *Temp;

            Temp = Node;
            Node = Node->Right;
            Temp->Right = Node->Left;
            Node->Left = Temp;

            return;
        } unguard;
    }

    void SingleRightRotation (NodeType *&Node)
    {
        guard
        {
            NodeType *Temp;

            Temp = Node;
            Node = Node->Left;
            Temp->Left = Node->Right;
            Node->Right = Temp;

            return;
        } unguard;
    }

    bool FixLeftGrowth (NodeType *&Node)
    {
        guard
        {
            if (Node->Balance <= -1)
            {   // Left heavy
                if (Node->Left->Balance <= -1)
                {
                    Node->Balance = 0;
                    Node->Left->Balance = 0;
                    SingleRightRotation (Node);
                }
                else
                {
                    int NLRBalance;

                    NLRBalance = Node->Left->Right->Balance;

                    if (NLRBalance <= -1)
                    {
                        Node->Balance = +1;
                        Node->Left->Balance = 0;
                    }
                    else
                    if (NLRBalance >= +1)
                    {
                        Node->Balance = 0;
                        Node->Left->Balance = -1;
                    }
                    else
                    {
                        Node->Balance = 0;
                        Node->Left->Balance = 0;
                    }

                    Node->Left->Right->Balance = 0;
                    SingleLeftRotation (Node->Left);
                    SingleRightRotation (Node);
                }

                return (true);
            }
            else
            if (Node->Balance >= +1)
            {   // Right heavy
                Node->Balance = 0;
                return (true);
            }

            Node->Balance = -1;
            return (false);
        } unguard;
    }

    bool FixRightGrowth (NodeType *&Node)
    {
        guard
        {
            if (Node->Balance <= -1)
            {
                Node->Balance = 0;
                return (true);
            }
            else
            if (Node->Balance >= +1)
            {
                if (Node->Right->Balance >= +1)
                {
                    Node->Balance = 0;
                    Node->Right->Balance = 0;
                    SingleLeftRotation (Node);
                }
                else
                {
                    int NRLBalance;

                    NRLBalance = Node->Right->Left->Balance;

                    if (NRLBalance >= +1)
                    {
                        Node->Balance = -1;
                        Node->Right->Balance = 0;
                    }
                    else
                    if (NRLBalance <= -1)
                    {
                        Node->Balance = 0;
                        Node->Right->Balance = +1;
                    }
                    else
                    {
                        Node->Balance = 0;
                        Node->Right->Balance = 0;
                    }

                    Node->Right->Left->Balance = 0;
                    SingleRightRotation (Node->Right);
                    SingleLeftRotation (Node);
                }

                return (true);
            }

            Node->Balance = +1;
            return (false);
        } unguard;
    }

    bool Insert (const KeyType &Key, const DataType &Data, bool Update = true)
    {
        guard
        {
            return (_Insert (Key, Data, TreeRoot, Update));
        } unguard;
    }

    bool _Insert (const KeyType &Key, const DataType &Data, NodeType *&Node, bool Update)
    {
        guard
        {
            bool Result;

            if (Node == NULL)
            {
                Node = AllocateNode (Key, Data);
                return (false); // please rebalance the tree!
            }
            else
            if (LT (Key, Node->KnD.Key))
            {
                Result = _Insert (Key, Data, Node->Left, Update);

                if (!Result)
                    return (FixLeftGrowth (Node));

                return (Result);
            }
            else
            if (LT (Node->KnD.Key, Key))
            {
                Result = _Insert (Key, Data, Node->Right, Update);

                if (!Result)
                    return (FixRightGrowth (Node));

                return (Result);
            }
            else
            //if (Key == Node->KnD.Key)
            {
                if (Update)
                {
                    Node->SetKey (Key);
                    Node->SetData (Data);
                }

                return (true); // a-ok
            }

            return (false);
        } unguard;
    }

    bool FixLeftShrinkage (NodeType *&Node)
    {
        guard
        {
            if (Node->Balance <= -1)
            {
                Node->Balance = 0;
                return (false);
            }
            else
            if (Node->Balance >= +1)
            {
                if (Node->Right->Balance >= +1)
                {
                    Node->Balance = 0;
                    Node->Right->Balance = 0;
                    SingleLeftRotation (Node);
                    return (false);
                }
                else
                if (Node->Right->Balance == 0)
                {
                    Node->Balance = +1;
                    Node->Right->Balance = -1;
                    SingleLeftRotation (Node);
                    return (true);
                }
                else
                {
                    int NRLBalance;

                    NRLBalance = Node->Right->Left->Balance;
                    
                    if (NRLBalance <= -1)
                    {
                        Node->Balance = 0;
                        Node->Right->Balance = +1;
                    }
                    else
                    if (NRLBalance >= +1)
                    {
                        Node->Balance = -1;
                        Node->Right->Balance = 0;
                    }
                    else
                    {
                        Node->Balance = 0;
                        Node->Right->Balance = 0;
                    }

                    Node->Right->Left->Balance = 0;
                    SingleRightRotation (Node->Right);
                    SingleLeftRotation (Node);
                    return (false);
                }
            }

            Node->Balance = +1;
            return (true);
        } unguard;
    }    

    bool FixRightShrinkage (NodeType *&Node)
    {
        guard
        {
            if (Node->Balance >= +1)
            {
                Node->Balance = 0;
                return (false);
            }
            else
            if (Node->Balance <= -1)
            {
                if (Node->Left->Balance <= -1)
                {
                    Node->Balance = 0;
                    Node->Left->Balance = 0;
                    SingleRightRotation (Node);
                    return (false);
                }
                else
                if (Node->Left->Balance == 0)
                {
                    Node->Balance = -1;
                    Node->Left->Balance = +1;
                    SingleRightRotation (Node);
                    return (true);
                }
                else
                {
                    int NLRBalance;

                    NLRBalance = Node->Left->Right->Balance;
                    
                    if (NLRBalance >= +1)
                    {
                        Node->Balance = 0;
                        Node->Left->Balance = -1;
                    }
                    else
                    if (NLRBalance <= -1)
                    {
                        Node->Balance = +1;
                        Node->Left->Balance = 0;
                    }
                    else
                    {
                        Node->Balance = 0;
                        Node->Left->Balance = 0;
                    }

                    Node->Left->Right->Balance = 0;
                    SingleLeftRotation (Node->Left);
                    SingleRightRotation (Node);
                    return (false);
                }
            }

            Node->Balance = -1;
            return (true);
        } unguard;
    }    

    bool FindHighest (NodeType *Target, NodeType *&Node, bool *Result)
    {
        guard
        {
            NodeType *Temp;

            *Result = false;

            if (Node == NULL)
                return (false);

            if (Node->Right != NULL)
            {
                if (!FindHighest (Target, Node->Right, Result))
                    return (false);

                if (*Result == false)
                    *Result = FixRightShrinkage (Node);

                return (true);
            }

            Target->KnD = Node->KnD;
            Temp = Node;
            Node = Node->Left;
            FreeNode (Temp);

            return (true);
        } unguard;
    }

    bool FindLowest (NodeType *Target, NodeType *&Node, bool *Result)
    {
        guard
        {
            NodeType *Temp;

            *Result = false;

            if (Node == NULL)
                return (false);

            if (Node->Left != NULL)
            {
                if (!FindLowest (Target, Node->Left, Result))
                    return (false);

                if (*Result == false)
                    *Result = FixLeftShrinkage (Node);

                return (true);
            }

            Target->KnD = Node->KnD;
            Temp = Node;
            Node = Node->Right;
            FreeNode (Temp);

            return (true);
        } unguard;
    }

    bool Remove (const KeyType &Key)
    {
        guard
        {
            return (_Remove (TreeRoot, Key));
        } unguard;
    }

    bool _Remove (NodeType *&Node, const KeyType &Key)
    {
        guard
        {
            bool Result = false;

            if (Node == NULL)
                return (true);

            if (LT (Key, Node->KnD.Key))
            {
                if (Node->Left == NULL)
                    return (true);

                Result = _Remove (Node->Left, Key);

                if (!Result)
                    return (FixLeftShrinkage (Node));

                return (Result);
            }

            if (LT (Node->KnD.Key, Key))
            {
                if (Node->Right == NULL)
                    return (true);

                Result = _Remove (Node->Right, Key);

                if (!Result)
                    return (FixRightShrinkage (Node));

                return (Result);
            }

            // invariant: Key == Node->KnD.Key
            if (Node->Left != NULL)
            {
                if (FindHighest (Node, Node->Left, &Result))
                {
                    if (!Result)
                        Result = FixLeftShrinkage (Node);

                    return (Result);
                }
            }

            if (Node->Right != NULL)
            {
                if (FindLowest (Node, Node->Right, &Result))
                {
                    if (!Result)
                        Result = FixRightShrinkage (Node);

                    return (Result);
                }
            }

            FreeNode (Node);
            Node = NULL;
            return (false);
        } unguard;
    }
};


#endif // _AVLBTREE_H
