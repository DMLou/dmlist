/*****************************************************************************

  VarHeap.h

  Preface a variable name with _ to make it volatile; that is, it will not
  be saved to the registry.

*****************************************************************************/


#ifndef _VARHEAP_H
#define _VARHEAP_H


#include "ListTypes.h"
#include "TreeList.h"
#include <windows.h>
#include "CMutexRW.h"
#include <set>
#include <iterator>


// Used for npos in the TreeList/AVLBTree instantiation for using a string as a key
class BadString
{
public:
    string operator() (void)
    {
        return (string("\xff"));
    }
};


typedef enum
{
    VTUint64,  // 64-bit unsigned integer (QWORD)
    VTUint32,  // 32-bit unsigned integer (DWORD)
    VTString,  // String!
    VTBinary,  // Array of bytes (vector<uint8>, or uint8*)

    VTInvalid  // Not initialized, or terminator for lists
} VarType;


template<class StreamType>
ostream &operator<< (ostream &stream, VarType type)
{
    guard
    {
        switch (type)
        {
            case VTUint64:  
                stream << "VTUint64"; 
                break;

            case VTUint32:  
                stream << "VTUint32"; 
                break;

            case VTString:  
                stream << "VTString"; 
                break;

            case VTBinary:  
                stream << "VTBinary"; 
                break;

            case VTInvalid: 
                stream << "VTInvalid"; 
                break;

            default:        
                stream << "(unknown)"; 
                break;
        }

        return (stream);
    } unguard;
}

typedef vector<uint8> ValBinary;

template<class StreamType>
ostream &operator<< (ostream &stream, const ValBinary &binary)
{
    guard
    {
        ValBinary::iterator it;

        for (it = binary.begin(); it != binary.end(); ++it)
        {
            stream << *it;

            if (it != --binary.end())
                stream << " ";
        }

        return (stream);
    } unguard;
}


class VarAnon
{
public:
    VarType Type;
    string Value;

    friend ostream &operator<< (ostream &stream, const VarAnon &rhs)
    {
        guard
        {
            stream << "VarAnon (val='" << rhs.Value << "', type=" << rhs.Type << ")";
            return (stream);
        } unguard;
    }

    VarAnon ()
    {
        guard
        {
            Set (VTInvalid, "");
            return;
        } unguard;
    }

    VarAnon (const VarType _Type, const string &_Value)
    {
        guard
        {
            Set (_Type, _Value);
            return;
        } unguard;
    }

    VarAnon (const VarAnon &lhs)
    {
        guard
        {
            Set (lhs.Type, lhs.Value);
            return;
        } unguard;
    }

    void Set (const VarType _Type, const string &_Value)
    {
        guard
        {
            SetType (_Type);
            SetVal (_Value);
            return;
        } unguard;
    }

    void SetType (const VarType _Type)
    {
        guard
        {
            Type = _Type;
            return;
        } unguard;
    }

    void SetVal (const string &_Value)
    {
        guard
        {
            Value = _Value;
            return;
        } unguard;
    }

    void SetValUint64 (const uint64 NewValue)
    {
        guard
        {
            char Temp[24];

            sprintf (Temp, "%I64u", NewValue);
            Value = string (Temp);
            Type = VTUint64;
            return;
        } unguard;
    }

    void SetValUint32 (const uint32 NewValue)
    {
        guard
        {
            char Temp[12];

            sprintf (Temp, "%u", NewValue);
            Value = string (Temp);
            Type = VTUint32;
            return;
        } unguard;
    }

    void SetValString (const string &NewValue)
    {
        guard
        {
            Value = NewValue;
            Type = VTString;
            return;
        } unguard;
    }

    void SetValBinary (const ValBinary &NewValue)
    {
        guard
        {
            const char *HexList = "0123456789abcdef";
            ValBinary::size_type i;

            Value.resize (NewValue.size() * 3);

            for (i = 0; i < NewValue.size(); i++)
            {
                Value[i * 3] = HexList[NewValue[i] >> 4];
                Value[(i * 3) + 1] = HexList[NewValue[i] & 0xf];
                Value[(i * 3) + 2] = ' ';
            }

            Type = VTBinary;
            return;
        } unguard;
    }

    void SetValBinary (const uint8 *Array, int Length)
    {
        guard
        {
            const char *HexList = "0123456789abcdef";
            int i;

            Value.resize (Length * 3);

            for (i = 0; i < Length; i++)
            {
                Value[i * 3] = HexList[Array[i] >> 4];
                Value[(i * 3) + 1] = HexList[Array[i] & 0xf];
                Value[(i * 3) + 2] = ' ';
            }

            Type = VTBinary;
            return;
        } unguard;
    }

    VarType GetType (void) const
    {
        guard
        {
            return (Type);
        } unguard;
    }

    string GetVal (void) const
    {
        guard
        {
            return (Value);
        } unguard;
    }

    uint64 GetValUint64 (void) const
    {
        guard
        {
            return (_atoi64 (Value.c_str()));
        } unguard;
    }

    uint32 GetValUint32 (void) const
    {
        guard
        {
            return (atoi (Value.c_str()));
        } unguard;
    }

    string GetValString (void) const
    {
        guard
        {
            return (Value);
        } unguard;
    }

    static uint8 HexToInt (char c)
    {
        guard
        {
            if (c >= '0'  &&  c <= '9')
                return (c - '0');

            if (c >= 'a'  &&  c <= 'f')
                return (c - 'a' + 10);

            if (c >= 'A' &&  c <= 'F')
                return (c - 'A' + 10);

            return (0);
        } unguard;
    }

    ValBinary GetValBinary (void) const
    {
        guard
        {
            ValBinary Result;
            ValBinary::size_type i;
            
            for (i = 0; i < Value.length() / 3; i++)
                Result.push_back (HexToInt(Value[(i * 3) + 1]) + (HexToInt(Value[i * 3]) << 4));

            return (Result);
        } unguard;
    }

private:

};


class VarNamed : public VarAnon
{
public:

    friend ostream &operator<< (ostream &stream, const VarNamed &rhs)
    {
        guard
        {
            stream << "VarNamed (name='" << rhs.Name << "', " << rhs.GetAnon() << ")";
            return (stream);
        } unguard;
    }

    VarNamed ()
    {
        guard
        {
            return;
        } unguard;
    }

    VarNamed (const string &_Name, const VarAnon &_Value)
        : Name (_Name),
          VarAnon (_Value)
    {
        guard
        {
            return;
        } unguard;
    }

    VarNamed (const VarNamed &Named)
        : Name (Named.Name),
          VarAnon (Named.Type, Named.Value)
    {
        guard
        {
            return;
        } unguard;
    }

    VarNamed (const string &_Name, const VarType _Type, const string &_Value)
        : Name (_Name),
          VarAnon (_Type, _Value)
    {
        guard
        {
            return;
        } unguard;
    }

    const VarAnon &GetAnon (void) const
    {
        guard
        {
            return (*(dynamic_cast<const VarAnon *> (this)));
        } unguard;
    }

    string Name;
};


template<class Anon = VarAnon, class Named = VarNamed>
class VarHeap
{
public:
    typedef VarHeap<Anon,Named> MyType;
    typedef Anon MyAnon;
    typedef Named MyNamed;

    // Use this to stream in names of volatiles
    class Volatile
    {
    public:
        Volatile () { }
        Volatile (const char *Name_init) : Name (Name_init) { }
        Volatile (const string &Name_init) : Name (Name_init) { }
        Volatile (const Volatile &CopyMe) : Name (CopyMe.Name) { }
        Volatile &operator= (const Volatile &rhs) { Name = rhs.Name; return (*this); }

        void SetName (const string &NewName) { Name = NewName; return; }
        const string &GetName (void) const { return (Name); }
    private:
        string Name;
    };

    VarHeap ()
        : Mutex (true)
    {
        guard
        {
            Defaults = NULL;
            SubKey = "";
            RootKey = NULL;
            Mutex.UnlockWrite ();
            return;
        } unguard;
    }

    VarHeap (const MyType &List)
        : Mutex (true),
          Heap (List.Heap),
          Defaults (List.Defaults),
          RootKey (List.RootKey),
          SubKey (List.SubKey),
          VolatileVarNames (List.VolatileVarNames)
    {
        guard
        {
            Mutex.UnlockWrite ();
            return;
        } unguard;
    }

    MyType &operator= (const MyType &rhs)
    {
        guard
        {
            if (this == &rhs)
                return (*this);

            Mutex.LockWrite ();
            Heap = rhs.Heap;
            Defaults = rhs.Defaults;
            RootKey = rhs.RootKey;
            SubKey = rhs.SubKey;
            VolatileVarNames = rhs.VolatileVarNames;
            Mutex.UnlockWrite ();

            return (*this);
        } unguard;
    }

    MyType &operator<< (const MyNamed &insert)
    {
        guard
        {
            SetVar (insert.Name, insert.GetAnon());
            return (*this);
        } unguard;
    }

    MyType &operator<< (const Volatile &NewVolatile)
    {
        guard
        {
            AddVolatile (NewVolatile.GetName());
            return (*this);
        } unguard;
    }

    friend ostream &operator<< (ostream &stream, const MyType &rhs)
    {
        guard
        {
            vector<MyNamed> Vars (rhs.GetVector());
            vector<MyNamed>::iterator it;

            rhs.Mutex.LockRead ();
            stream << typeid(MyType).name() << " (";
            for (it = Vars.begin(); it != Vars.end(); ++it)
            {
                stream << *it;
                if (it != --Vars.end())
                    stream << ", ";
            }

            rhs.Mutex.UnlockRead ();
            stream << ")";
            return (stream);
        } unguard;
    }


    // Can be initialized from an array of VarNamed[Ext] objects, which must be terminated by
    // an element with type VTInvalid
    VarHeap (const Named *List, MyType *DefaultsList = NULL)
    {
        guard
        {
            Named *p;

            Mutex.LockWrite ();
            VolatileVarNames.clear ();
            Clear ();
            p = (Named *)List;

            while (p->Type != VTInvalid)
            {
                SetVar (p->Name, p->GetAnon());
                p++;
            }

            Defaults = (MyType *)DefaultsList;
            Mutex.UnlockWrite ();
            return;
        } unguard;
    }

    // Can also initialize with a vector of Named objects
    VarHeap (const vector<MyNamed> &List, MyType *DefaultsList = NULL)
    {
        guard
        {
            vector<Named>::const_iterator it;

            Mutex.LockWrite ();
            VolatileVarNames.clear ();
            Clear ();

            for (it = List.begin(); it < List.end(); it++)
            {
                SetVar (it->Name, it->GetAnon());
            }

            Defaults = (MyType *) DefaultsList;
            Mutex.UnlockWrite ();
            return;
        } unguard;
    }

    ~VarHeap ()
    {
        guard
        {
            return;
        } unguard;
    }

    void SetDefaultsList (const MyType *NewDefaults)
    {
        guard
        {
            Mutex.LockWrite ();
            Defaults = (MyType *) NewDefaults;
            Mutex.UnlockWrite ();
            return;
        } unguard;
    }

    void AddVolatile (const string &Name)
    {
        guard
        {
            Mutex.LockWrite ();
            VolatileVarNames.erase (Name);
            Mutex.UnlockWrite ();
            return;
        } unguard;
    }

    bool IsVolatile (const string &Name)
    {
        guard
        {
            if (VolatileVarNames.find (Name) == VolatileVarNames.end())
                return (false);

            return (true);
        } unguard;
    }

    void RemoveVolatile (const string &Name)
    {
        guard
        {
            Mutex.LockWrite ();
            VolatileVarNames.remove (VolatileVarNames.find (Name));
            Mutex.UnlockWrite ();
            return;
        } unguard;
    }

    // Returns true if a variable by the given name is present, or false if it has not been defined
    bool IsVarDefined (const string &Name) const
    {
        guard
        {
            bool ReturnVal;

            Mutex.LockRead ();

            if (Heap.Get (Name) != NULL)
                ReturnVal = true;
            else
                ReturnVal = false;

            Mutex.UnlockRead ();
            return (ReturnVal);
        } unguard;
    }

    // Sets a variable by the given name (NewVar.Name) to the given value (NewVar.Value)
    // If the variable does not exist yet, it is added
    // If the variable already exists, it is updated
    MyType &SetVar (const string &Name, const MyAnon &NewVar)
    {
        guard
        {
            bool ReturnVal;

            Mutex.LockWrite ();
            ReturnVal = Heap.Push (Name, NewVar);
            Mutex.UnlockWrite ();
            return (*this);
        } unguard;
    }

    // Only updates the value portion of a variable; useful for VarListExt objects where you don't want to update any of the attributes
    // Will create the variable if it doesn't exist yet
    bool SetVal (const string &Name, const string &NewValue)
    {
        guard
        {
            Anon *Data;

            Mutex.LockWrite ();
            Data = Heap.Get (Name);

            if (Data != NULL)
                Data->SetVal (NewValue);
            else
            {
                Anon Var;

                Var.SetVal (NewValue);
                SetVar (Name, Var);
            }

            Mutex.UnlockWrite ();
            return (true);
        } unguard;
    }

    bool SetVarType (const string &Name, const VarType NewType)
    {
        guard
        {
            Anon *Data;

            Mutex.LockWrite ();
            Data = Heap.Get (Name);

            if (Data != NULL)
                Data->SetType (NewType);
            else
            {
                Anon Var;

                Var.SetType (NewType);
                SetVar (Name, Var);
            }

            Mutex.UnlockWrite ();
            return (true);
        } unguard;
    }

    bool SetValUint32 (const string &Name, const uint32 NewValue)
    {
        guard
        {
            Anon *Data;

            Mutex.LockWrite ();
            Data = Heap.Get (Name);

            if (Data != NULL)
                Data->SetValUint32 (NewValue);
            else
            {
                Anon Var;

                Var.SetValUint32 (NewValue);
                SetVar (Name, Var);
            }

            Mutex.UnlockWrite ();
            return (true);
        } unguard;
    }

    bool SetValUint64 (const string &Name, const uint64 NewValue)
    {
        guard
        {
            Anon *Data;

            Mutex.LockWrite ();
            Data = Heap.Get (Name);

            if (Data != NULL)
                Data->SetValUint64 (NewValue);
            else
            {
                Anon Var;

                Var.SetValUint64 (NewValue);
                SetVar (Name, Var);
            }

            Mutex.UnlockWrite ();
            return (true);
        } unguard;
    }

    bool SetValString (const string &Name, const string &NewValue)
    {
        guard
        {
            Anon *Data;

            Mutex.LockWrite ();
            Data = Heap.Get (Name);

            if (Data != NULL)
                Data->SetValString (NewValue);
            else
            {
                Anon Var;

                Var.SetValString (NewValue);
                SetVar (Name, Var);
            }

            Mutex.UnlockWrite ();
            return (true);
        } unguard;
    }

    bool SetValBinary (const string &Name, const ValBinary &NewValue)
    {
        guard
        {
            Anon *Data;

            Mutex.LockWrite ();
            Data = Heap.Get (Name);

            if (Data != NULL)
                Data->SetValBinary (NewValue);
            else
            {
                Anon Var;

                Var.SetValBinary (NewValue);
                SetVar (Name, Var);
            }

            Mutex.UnlockWrite ();
            return (true);
        } unguard;
    }

    bool SetValBinary (const string &Name, const uint8 *Array, const int Length)
    {
        guard
        {
            MyAnon *Data;

            Mutex.LockWrite ();
            Data = Heap.Get (Name);

            if (Data != NULL)
                Data->SetValBinary (Array, Length);
            else
            {   
                Anon Var;

                Var.SetValBinary (Array, Length);
                SetVar (Name, Var);
            }

            Mutex.UnlockWrite ();
            return (true);
        } unguard;
    }

    // Looks up the value (instance of VarAnon) of a variable. If the variable does not exist, the default value is returned
    MyAnon GetVar (const string &Name) const
    {
        guard
        {
            MyAnon *Data;
            MyAnon ReturnVal;

            Mutex.LockRead ();
            Data = Heap.Get (Name);

            if (Data == NULL  &&  Defaults != NULL)
                ReturnVal = Defaults->GetVar (Name);
            else
            if (Data == NULL  &&  Defaults == NULL)
                ReturnVal = MyAnon(); // return "invalid"/blank variable
            else
                ReturnVal = *Data;

            Mutex.UnlockRead ();
            return (ReturnVal);
        } unguard;
    }

    // Returns the value of a variable
    string GetVal (const string &Name) const
    {
        guard
        {
            return (GetVar (Name).GetVal());
        } unguard;
    }

    uint32 GetValUint32 (const string &Name) const
    {
        guard
        {
            return (GetVar (Name).GetValUint32());
        } unguard;
    }

    uint64 GetValUint64 (const string &Name) const
    {
        guard
        {
            return (GetVar (Name).GetValUint64());
        } unguard;
    }

    ValBinary GetValBinary (const string &Name) const
    {
        guard
        {
            return (GetVar (Name).GetValBinary());
        } unguard;
    }

    string GetValString (const string &Name) const
    {
        guard
        {
            return (GetVar (Name).GetValString());
        } unguard;
    }

    // Updates from a given list. All variables in the given list are updated in our own list.
    // That is, for all variables named X that are in both rhs and lhs, lhs[X] = rhs[X]
    // Example: Given two lists, A and B with the following variables:
    // A = [ ("x",0), ("y",1), ("z",2) ] + volatiles[ "x", "z" ]
    // B = [ ("a",5), ("x",2), ("y",7) ] + volatiles[ "a" ]
    // The result of A.UpdateVars(B) is:
    // A = [ ("x",2), ("y",7), ("z",2) ] + volatiles[ "x", "z" ]
    void Update (const MyType &rhs)
    {
        guard
        {
            string Key;

            Mutex.LockWrite ();
            rhs.LockRead ();

            Key = rhs.Heap.GetHeadKey ();

            while (Key != rhs.Heap.npos)
            {
                if (Heap.Get (Key) != NULL)
                    SetVar (Key, *rhs.Heap.Get(Key));

                Key = rhs.Heap.GetNextKey (Key);
            }

            rhs.UnlockRead ();
            Mutex.UnlockWrite ();
            return;
        } unguard;
    }

    // Copies all variables from rhs into our list, updating (with precedence to rhs) all those that are common
    // Example: Given two lists, A and B with the following variables:
    // A = [ ("x",0), ("y",1), ("z",2) ] + volatiles[ "x", "z" ]
    // B = [ ("a",5), ("x",2), ("y",7) ] + volatiles[ "a" ]
    // The result of A.Union(B) is:
    // A = [ ("a",5), ("x",2), ("y",7), ("z",2) ] + volatiles [ "a", "x", "z" ]
    // (This operation is also referred to as the *overriding* union, A oU B)
    // Also appends the volatile-list from B into A 
    void Union (const MyType &rhs)
    {
        guard
        {
            string Key;

            Mutex.LockWrite ();
            Key = rhs.Heap.GetHeadKey ();

            while (Key != rhs.Heap.npos)
            {
                SetVar (Key, *rhs.Heap.Get (Key));
                Key = rhs.Heap.GetNextKey (Key);
            }

            // Volatiles
            copy (rhs.VolatileVarNames.begin(), rhs.VolatileVarNames.end(), std::inserter (VolatileVarNames, VolatileVarNames.end()));

            Mutex.UnlockWrite ();
            return;
        } unguard;
    }

    // Registry functions
    void SetRegistryKey (const HKEY Root, const string &Sub)
    {
        guard
        {
            Mutex.LockWrite ();
            RootKey = Root;
            SubKey = Sub;
            Mutex.UnlockWrite ();
            return;
        } unguard;
    }

    void GetRegistryKey (HKEY &RootResult, string &SubResult) const
    {
        guard
        {
            Mutex.LockWrite ();
            RootResult = RootKey;
            SubResult = SubKey;
            Mutex.UnlockWrite ();
            return;
        } unguard;
    }

    // Removes all variables
    void Clear (void)
    {
        guard
        {
            Mutex.LockWrite ();
            Heap.Clear();
            Mutex.UnlockWrite ();
            return;
        } unguard;
    }

    // Any variables that we are keeping track of right now are updated from the registry
    // If the value is not in the registry then we leave it alone
    void UpdateFromRegistry (void)
    {
        guard
        {
            MyType RegLoaded;

            Mutex.LockWrite ();
            RegLoaded.SetRegistryKey (RootKey, SubKey);
            RegLoaded.LoadVarsFromReg ();
            Update (RegLoaded);
            Mutex.UnlockWrite ();
            return;
        } unguard;
    }


    void UnionFromRegistry (void)
    {
        guard
        {
            MyType RegLoaded;

            Mutex.LockWrite ();
            RegLoaded.SetRegistryKey (RootKey, SubKey);
            RegLoaded.LoadVarsFromReg ();
            Union (RegLoaded);
            Mutex.UnlockWrite ();
            return;
        } unguard;
    }


    // Returns a convenient vector of Anon type variables
    vector<MyNamed> GetVector (void) const
    {
        guard
        {
            vector<MyNamed> ReturnVec;
            string Key;
            MyAnon *Data;

            Mutex.LockRead ();
            Key = Heap.GetHeadKey ();

            while (Key != Heap.m_npos)
            {
                Data = Heap.Get (Key);
                MyNamed Var (Key, *Data);
                ReturnVec.push_back (Var);
                Key = Heap.GetNextKey (Key);
            }

            Mutex.UnlockRead ();
            return (ReturnVec);
        } unguard;
    }

    // !!! This will delete all variables that may be in the heap !!!
    bool LoadVarsFromReg (void)
    {
        guard
        {
            HKEY OpenKey;
            LONG Result;
            DWORD Count;
            DWORD MaxNameLen;
            DWORD Index;
            DWORD MaxSize;

            Mutex.LockWrite ();
            Clear ();

            Result = RegOpenKeyEx (RootKey, SubKey.c_str(), 0, KEY_READ, &OpenKey);

            if (Result != ERROR_SUCCESS)
            {
                Mutex.UnlockWrite ();
                return (false);
            }

            Result = RegQueryInfoKey (OpenKey, NULL, NULL, NULL, NULL, NULL, NULL, &Count, &MaxNameLen, &MaxSize, NULL, NULL);

            if (Result != ERROR_SUCCESS)
            {
                RegCloseKey (OpenKey);
                Mutex.UnlockWrite ();
                return (false);
            }

            for (Index = 0; Index < Count; Index++)
            {
                DWORD Type;
                DWORD Length;
                DWORD Size;
                MyAnon Var;
                bool Add = true;
                char *NameBuffer = new char[MaxNameLen + 1];
                char *DataBuffer = new char[MaxSize];

                Length = MaxNameLen + 1;
                Size = MaxSize;
                Result = RegEnumValue (OpenKey, Index, NameBuffer, &Length, NULL, &Type, (LPBYTE)DataBuffer, &Size);

                if (Result != ERROR_SUCCESS)
                {
                    Clear ();
                    delete NameBuffer;
                    delete DataBuffer;
                    RegCloseKey (OpenKey);
                    Mutex.UnlockWrite ();
                    return (false);
                }

                switch (Type)
                {
                    case REG_BINARY:
                        Var.SetValBinary ((uint8 *)DataBuffer, Size);
                        break;

                    case REG_DWORD:
                        Var.SetValUint32 (*((DWORD *)DataBuffer));
                        break;

                    case REG_QWORD:
                        Var.SetValUint64 (*((uint64 *)DataBuffer));
                        break;

                    case REG_SZ:
                        Var.SetValString (DataBuffer);
                        break;

                    default:
                        // Skip the value, essentially
                        Add = false;
                        break;
                }

                if (Add)
                    SetVar (NameBuffer, Var);

                delete NameBuffer;
                delete DataBuffer;
            }

            RegCloseKey (OpenKey);
            Mutex.UnlockWrite ();
            return (true);
        } unguard;
    }

    bool SaveVarsToReg (void)
    {
        guard
        {
            HKEY OpenKey;
            LONG Result;
            int i;
            vector<MyNamed> Vars;

            Mutex.LockRead ();
            Vars = GetVector();

            Result = 
                RegCreateKeyEx
                    (
                        RootKey,
                        SubKey.c_str(),
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL,
                        &OpenKey,
                        NULL
                    );

            if (Result != ERROR_SUCCESS)
            {
                Mutex.UnlockRead ();
                return (false);
            }

            for (i = 0; i < Vars.size(); i++)
            {
                if (Vars[i].Name[0] == '_')
                    continue;

                if (IsVolatile (Vars[i].Name))
                    continue;

                switch (Vars[i].GetType())
                {
                    case VTInvalid:
                        // Do not save this variable.
                        break;

                    case VTUint64:
                        uint64 QWord;

                        QWord = Vars[i].GetValUint64();
                        Result = RegSetValueEx (OpenKey, Vars[i].Name.c_str(), NULL, REG_QWORD, (const BYTE *)&QWord, sizeof (uint64));
                        break;

                    case VTUint32:
                        DWORD DWord;

                        DWord = Vars[i].GetValUint32();
                        Result = RegSetValueEx (OpenKey, Vars[i].Name.c_str(), NULL, REG_DWORD, (const BYTE *)&DWord, sizeof (DWORD));
                        break;

                    case VTString:
                        Result = RegSetValueEx (OpenKey, Vars[i].Name.c_str(), NULL, REG_SZ, 
                            (BYTE *)Vars[i].GetValString().c_str(), 
                            sizeof (char) * (1 + Vars[i].GetValString().length()));

                        break;

                    case VTBinary:
                        {
                            ValBinary Binary;
                            char *Array;
                            int j;

                            Binary = Vars[i].GetValBinary ();
                            Array = new char[Binary.size()];

                            for (j = 0; j < Binary.size(); j++)
                                Array[j] = Binary[j];

                            Result = RegSetValueEx (OpenKey, Vars[i].Name.c_str(), NULL, REG_BINARY,
                                (const BYTE *) Array, Binary.size());

                            delete Array;
                        }
                        break;
                }
            }

            Mutex.UnlockRead ();
            return (true);
        } unguard;
    }

private:
    typedef TreeList<string, MyAnon, BadString> VarMap;
    VarMap Heap;
    MyType *Defaults;

    // List of variables to NOT save to the registry
    std::set<string> VolatileVarNames;

    // Registry info: where to load/save from
    HKEY RootKey;  // Generally HKEY_CURRENT_USER
    string SubKey; // Something like SOFTWARE\\ListXP

    CMutexRW Mutex;
};


typedef VarHeap<VarAnon> VarList;


// Same as an anonymous variable but it can have attributes associated with it
class VarAnonExt : public VarAnon
{
public:
    VarAnonExt ()
    {
        guard
        {
            return;
        } unguard;
    }


    VarAnonExt (const VarType _Type, const string &_Value)
        : VarAnon (_Type, _Value)
    {
        guard
        {
            return;
        } unguard;
    }


    VarAnonExt (const VarType _Type, const string &_Value, const VarList &_Attributes)
        : VarAnon (_Type, _Value),
          Attributes (_Attributes)
    {
        guard
        {
            return;
        } unguard;
    }


    VarAnonExt (const VarAnonExt &rhs)
        : VarAnon (rhs.Type, rhs.Value),
          Attributes (rhs.Attributes)
    {
        guard
        {
            return;
        } unguard;
    }

    VarList Attributes;

    friend ostream &operator<< (ostream &stream, const VarAnonExt &rhs)
    {
        guard
        {
            stream << "VarAnonExt ("
                << *(dynamic_cast<const VarAnon *> (&rhs))
                << ", "
                << rhs.Attributes << ")";

            return (stream);
        } unguard;
    }
};


//
class VarNamedExt : public VarAnonExt
{
public:
    friend ostream &operator<< (ostream &stream, const VarNamedExt &rhs)
    {
        guard
        {
            stream << "VarNamedExt (name='" << rhs.Name << "', " << rhs.GetAnon() << ")";
            return (stream);
        } unguard;
    }

    VarNamedExt ()
    {
        guard
        {
            return;
        } unguard;
    }

    VarNamedExt (const string &_Name, const VarAnon &_Value, const VarList &_Attributes)
        : Name (_Name),
          VarAnonExt (_Value.Type, _Value.Value, _Attributes)
    {
        guard
        {
            return;
        } unguard;
    }

    VarNamedExt (const string &_Name, const VarType _Type, const string &_Value, const VarList &_Attributes)
        : Name (_Name),
          VarAnonExt (_Type, _Value, _Attributes)
    {
        guard
        {
            return;
        } unguard;
    }

    VarNamedExt (const string &_Name, const VarAnonExt &_Anon)
        : Name (_Name),
          VarAnonExt (_Anon)
    {
        guard
        {
            return;
        } unguard;
    }

    VarNamedExt (const VarNamedExt &Named)
        : Name (Named.Name),
          VarAnonExt (Named.GetAnon())
    {
        guard
        {
            return;
        } unguard;
    }

    const VarAnonExt &GetAnon (void) const
    {
        guard
        {
            return (*(dynamic_cast<const VarAnonExt *> (this)));
        } unguard;
    }

    string Name;
};


//
typedef VarHeap<VarAnonExt, VarNamedExt> VarListExt;


#endif // _VARHEAP_H


