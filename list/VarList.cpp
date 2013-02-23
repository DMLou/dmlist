//#include "VarList.h"


// VarAnon code
VarAnon::VarAnon ()
{
    Set (VTInvalid, "");
    return;
}

VarAnon::VarAnon (const VarType _Type, const string &_Value)
{
    Set (_Type, _Value);
    return;
}

VarAnon::VarAnon (const VarAnon &lhs)
{
    Set (lhs.Type, lhs.Value);
    return;
}

void VarAnon::Set (const VarType _Type, const string &_Value)
{
    SetType (_Type);
    SetVal (_Value);
    return;
}

void VarAnon::SetType (const VarType _Type)
{
    Type = _Type;
    return;
}

void VarAnon::SetVal (const string &_Value)
{
    Value = _Value;
    return;
}

void VarAnon::SetValUint64 (const uint64 NewValue)
{
    char Temp[24];

    sprintf (Temp, "%I64u", NewValue);
    Value = string (Temp);
    Type = VTUint64;
    return;
}

void VarAnon::SetValUint32 (const uint32 NewValue)
{
    char Temp[12];

    sprintf (Temp, "%u", NewValue);
    Value = string (Temp);
    Type = VTUint32;
    return;
}

void VarAnon::SetValString (const string &NewValue)
{
    Value = NewValue;
    Type = VTString;
    return;
}

void VarAnon::SetValBinary (const ValBinary &NewValue)
{
    const char *HexList = "0123456789abcdef";
    int i;

    Value.resize (NewValue.size() * 3);

    for (i = 0; i < NewValue.size(); i++)
    {
        Value[i * 3] = HexList[NewValue[i] >> 8];
        Value[(i * 3) + 1] = HexList[NewValue[i] & 0xf];
        Value[(i * 3) + 2] = ' ';
    }

    Type = VTBinary;
    return;
}

void VarAnon::SetValBinary (const uint8 *Array, int Length)
{
    const char *HexList = "0123456789abcdef";
    int i;

    Value.resize (Length * 3);

    for (i = 0; i < Length; i++)
    {
        Value[i * 3] = HexList[Array[i] >> 8];
        Value[(i * 3) + 1] = HexList[Array[i] & 0xf];
        Value[(i * 3) + 2] = ' ';
    }

    Type = VTBinary;
    return;
}

VarType VarAnon::GetType (void) const
{
    return (Type);
}

string VarAnon::GetVal (void) const
{
    return (Value);
}

uint64 VarAnon::GetValUint64 (void) const
{
    return (_atoi64 (Value.c_str()));
}

uint32 VarAnon::GetValUint32 (void) const
{
    return (atoi (Value.c_str()));
}

string VarAnon::GetValString (void) const
{
    return (Value);
}

static int HexToInt (char c)
{
    if (c >= '0'  &&  c <= '9')
        return (c - '0');

    if (c >= 'a'  &&  c <= 'f')
        return (c - 'a' + 10);

    if (c >= 'A' &&  c <= 'F')
        return (c - 'A' + 10);

    return (0);
}


ValBinary VarAnon::GetValBinary (void) const
{
    ValBinary Result;
    int i;
    
    for (i = 0; i < Value.length() / 3; i++)
        Result.push_back (HexToInt(Value[(i * 3) + 1]) + (HexToInt(Value[i * 3]) << 8));

    return (Result);
}



// VarNamed code
VarNamed::VarNamed ()
{
    return;
}


VarNamed::VarNamed (const string &_Name, const VarAnon &_Value)
{
    Name = _Name;
    Type = _Value.Type;
    Value = _Value.Value;
    return;
}


VarNamed::VarNamed (const VarNamed &Named)
{
    Name = Named.Name;
    Type = Named.Type;
    Value = Named.Value;
    return;
}


VarNamed::VarNamed (const string &_Name, const VarType _Type, const string &_Value)
{
    Name = _Name;
    Type = _Type;
    Value = _Value;
    return;
}


// VarAnonExt code
VarAnonExt::VarAnonExt ()
{
    return;
}


VarAnonExt::VarAnonExt (const VarType _Type, const string &_Value)
{
    Type = _Type;
    Value = _Value;
    return;
}


VarAnonExt::VarAnonExt (const VarType _Type, const string &_Value, const VarList &_Attributes)
{
    Type = _Type;
    Value = _Value;
    Attributes = _Attributes;
    return;
}


VarAnonExt::VarAnonExt (const VarAnonExt &rhs)
{
    Type = rhs.Type;
    Value = rhs.Value;
    Attributes = rhs.Attributes;
    return;
}


// VarNamedExt code
VarNamedExt::VarNamedExt ()
{
    return;
}


VarNamedExt::VarNamedExt (const string &_Name, const VarAnon &_Value, const VarList &_Attributes)
{
    Name = _Name;
    Type = _Value.Type;
    Value = _Value.Value;
    Attributes = _Attributes;
    return;
}


VarNamedExt::VarNamedExt (const string &_Name, const VarType _Type, const string &_Value, const VarList &_Attributes)
{
    Name = _Name;
    Type = _Type;
    Value = _Value;
    Attributes = _Attributes;
    return;
}


VarNamedExt::VarNamedExt (const string &_Name, const VarAnonExt &_Anon)
{
    Name = _Name;
    Type = _Anon.Type;
    Value = _Anon.Value;
    Attributes = _Anon.Attributes;
    return;
}


VarNamedExt::VarNamedExt (const VarNamedExt &Named)
{
    Name = Named.Name;
    Type = Named.Type;
    Value = Named.Value;
    Attributes = Named.Attributes;
    return;
}


// VarHeap code
template<class Anon, class Named>
VarHeap<Anon,Named>::VarHeap ()
{
    Defaults = NULL;
    return;
}


template<class Anon, class Named>
VarHeap<Anon,Named>::VarHeap (const MyType &List)
{
    Heap = List.Heap;
    Defaults = List.Defaults;
    RootKey = List.RootKey;
    SubKey = List.SubKey;
    return;
}


template<class Anon, class Named>
VarHeap<Anon,Named>::MyType &VarHeap<Anon,Named>::operator= (const MyType &rhs)
{
    Heap = rhs.Heap;
    Defaults = rhs.Defaults;
    RootKey = rhs.RootKey;
    SubKey = rhs.SubKey;

    return (*this);
}


// Can be initialized from an array of VarNamed objects, which must be terminated by
// an element with type VTInvalid
template<class Anon, class Named>
VarHeap<Anon,Named>::VarHeap (const Named *List, const MyType *DefaultsList)
{
    Named *p;

    Clear ();
    p = (Named *)List;

    while (p->Type != VTInvalid)
    {
        SetVar (p->Name, p->GetAnon());
        p++;
    }

    Defaults = DefaultsList;
    return;
}


template<class Anon, class Named>
VarHeap<Anon,Named>::VarHeap (vector<Named> &List, const MyType *DefaultsList)
{
    vector<Named>::iterator it;

    Clear ();

    for (it = List.begin(); it < List.end(); it++)
    {
        SetVar (it->Name, it->GetAnon());
    }

    Defaults = (MyType *) DefaultsList;
    return;
}


template<class Anon, class Named>
void VarHeap<Anon,Named>::SetDefaultsList (const MyType *NewDefaults)
{
    Defaults = (MyType *) NewDefaults;
    return;
}

// Returns true if a variable by the given name is present, or false if it has not been defined
template<class Anon, class Named>
bool VarHeap<Anon,Named>::IsVarDefined (const string &Name) const
{
    if (Heap.Get (Name) != NULL)
        return (true);

    return (false);
}

// Sets a variable by the given name (NewVar.Name) to the given value (NewVar.Value)
// If the variable does not exist yet, it is added
// If the variable already exists, it is updated
template<class Anon, class Named>
bool VarHeap<Anon,Named>::SetVar (const string &Name, const Anon &NewVar)
{
    return (Heap.Push (Name, NewVar));
}


template<class Anon, class Named>
bool VarHeap<Anon,Named>::SetVal (const string &Name, const string &NewValue)
{
    Anon *Data;

    Data = Heap.Get (Name);

    if (Data == NULL)
        return (false);

    Data->SetVal (NewValue);
    return (true);
}


template<class Anon, class Named>
bool VarHeap<Anon,Named>::SetValUint32 (const string &Name, const uint32 NewValue)
{
    Anon *Data;

    Data = Heap.Get (Name);

    if (Data == NULL)
        return (false);

    Data->SetValUint32 (NewValue);
    return (true);
}


template<class Anon, class Named>
bool VarHeap<Anon,Named>::SetValUint64 (const string &Name, const uint64 NewValue)
{
    Anon *Data;

    Data = Heap.Get (Name);

    if (Data == NULL)
        return (false);

    Data->SetValUint64 (NewValue);
    return (true);
}


template<class Anon, class Named>
bool VarHeap<Anon,Named>::SetValString (const string &Name, const string &NewValue)
{
    Anon *Data;

    Data = Heap.Get (Name);

    if (Data == NULL)
        return (false);

    Data->SetValString (NewValue);
    return (true);
}


template<class Anon, class Named>
bool VarHeap<Anon,Named>::SetValBinary (const string &Name, const ValBinary &NewValue)
{
    Anon *Data;

    Data = Heap.Get (Name);

    if (Data == NULL)
        return (false);

    Data->SetValBinary (NewValue);
    return (true);
}


template<class Anon, class Named>
bool VarHeap<Anon,Named>::SetValBinary (const string &Name, const uint8 *Array, const int Length)
{
    Anon *Data;

    Data = Heap.Get (Name);

    if (Data == NULL)
        return (false);

    Data->SetValBinary (Array, Length);
    return (true);
}


// Looks up the value of a variable. If the variable does not exist, Default is returned
template<class Anon, class Named>
VarHeap<Anon,Named>::MyAnon VarHeap<Anon,Named>::GetVar (const string &Name) const
{
    Anon *Data;

    Data = Heap.Get (Name);

    if (Data == NULL  &&  Defaults != NULL)
        return (Defaults->GetVar (Name));           
    
    if (Data == NULL  &&  Defaults == NULL)
        return (MyAnon()); // return "invalid"/blank variable

    return (*Data);
}

// Returns the value of a variable
template<class Anon, class Named>
string VarHeap<Anon,Named>::GetVal (const string &Name) const
{
    return (GetVar (Name).GetVal());
}

template<class Anon, class Named>
uint32 VarHeap<Anon,Named>::GetValUint32 (const string &Name) const
{
    return (GetVar (Name).GetValUint32());
}

template<class Anon, class Named>
uint64 VarHeap<Anon,Named>::GetValUint64 (const string &Name) const
{
    return (GetVar (Name).GetValUint64());
}

template<class Anon, class Named>
ValBinary VarHeap<Anon,Named>::GetValBinary (const string &Name) const
{
    return (GetVar (Name).GetValBinary());
}

template<class Anon, class Named>
string VarHeap<Anon,Named>::GetValString (const string &Name) const
{
    return (GetVar (Name).GetValString());
}

// Updates from a given list. All variables in the given list are updated in our own list.
// That is, for all variables named X that are in both rhs and lhs, lhs[X] = rhs[X]
// Example: Given two lists, A and B with the following variables:
// A = [ ("x",0), ("y",1), ("z",2) ]
// B = [ ("a",5), ("x",2), ("y",7) ]
// The result of A.UpdateVars(B) is:
// A = [ ("x",2), ("y",7), ("z",2) ]
template<class Anon, class Named>
void VarHeap<Anon,Named>::Update (const MyType &rhs)
{
    string Key;

    Key = rhs.Heap.GetHeadKey ();

    while (Key != rhs.Heap.npos)
    {
        if (Heap.Get (Key) != NULL)
            SetVar (Key, *rhs.Heap.Get(Key));

        Key = rhs.Heap.GetNextKey (Key);
    }

    return;
}

// Copies all variables from rhs into our list, updating (with precedence to rhs) all those that are common
// Example: Given two lists, A and B with the following variables:
// A = [ ("x",0), ("y",1), ("z",2) ]
// B = [ ("a",5), ("x",2), ("y",7) ]
// The result of A.Union(B) is:
// A = [ ("a",5), ("x",2), ("y",7), ("z",2) ]
template<class Anon, class Named>
void VarHeap<Anon,Named>::Union (const MyType &rhs)
{
    string Key;

    Key = rhs.Heap.GetHeadKey ();

    while (Key != rhs.Heap.npos)
    {
        SetVar (Key, *rhs.Heap.Get (Key));
        Key = rhs.Heap.GetNextKey (Key);
    }

    return;
}


// Registry functions
template<class Anon, class Named>
void VarHeap<Anon,Named>::SetRegistryKey (const HKEY Root, const string &Sub)
{
    RootKey = Root;
    SubKey = Sub;
    return;
}


template<class Anon, class Named>
void VarHeap<Anon,Named>::GetRegistryKey (HKEY &RootResult, string &SubResult) const
{
    RootResult = RootKey;
    SubResult = SubKey;
    return;
}


// Removes all variables
template<class Anon, class Named>
void VarHeap<Anon,Named>::Clear (void)
{
    Heap.Clear();
    return;
}


// Any variables that we are keeping track of right now are updated from the registry
// If the value is not in the registry then we leave it alone
template<class Anon, class Named>
void VarHeap<Anon,Named>::UpdateFromRegistry (void)
{
    MyType RegLoaded;

    RegLoaded.SetRegistryKey (RootKey, SubKey);
    LoadVarsFromReg (RegLoaded);
    Update (RegLoaded);
    return;
}


// Regular C-style functionality
template<class VarArray>
bool LoadVarsFromReg (VarArray &ListResult)
{
    HKEY OpenKey;
    HKEY RootKey;
    string SubKey;
    LONG Result;
    DWORD Count;
    DWORD MaxNameLen;
    DWORD Index;
    DWORD MaxSize;

    ListResult.Clear ();

    ListResult.GetRegistryKey (RootKey, SubKey);
    Result = RegOpenKeyEx (RootKey, SubKey.c_str(), 0, KEY_READ, &OpenKey);

    if (Result != ERROR_SUCCESS)
        return (false);

    Result = RegQueryInfoKey (OpenKey, NULL, NULL, NULL, NULL, NULL, NULL, &Count, &MaxNameLen, &MaxSize, NULL, NULL);

    if (Result != ERROR_SUCCESS)
    {
        RegCloseKey (OpenKey);
        return (false);
    }

    for (Index = 0; Index < Count; Index++)
    {
        DWORD Type;
        DWORD Length;
        DWORD Size;
        VarArray::MyAnon Var;
        bool Add = true;
        char *NameBuffer = new char[MaxNameLen + 1];
        char *DataBuffer = new char[MaxSize];

        Length = MaxNameLen + 1;
        Size = MaxSize;
        Result = RegEnumValue (OpenKey, Index, NameBuffer, &Length, NULL, &Type, (LPBYTE)DataBuffer, &Size);

        if (Result != ERROR_SUCCESS)
        {
            ListResult.Clear ();
            delete NameBuffer;
            delete DataBuffer;
            RegCloseKey (OpenKey);
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
        {
            ListResult.SetVar (NameBuffer, Var);
        }

        delete NameBuffer;
        delete DataBuffer;
    }

    RegCloseKey (OpenKey);
    return (true);
}


template<class VarArray>
bool SaveVarsToReg (const VarArray &List)
{
    HKEY OpenKey;
    HKEY RootKey;
    string SubKey;
    LONG Result;
    int i;
    vector<VarArray::MyNamed> Vars;

    Vars = List.GetVector();
    List.GetRegistryKey (RootKey, SubKey);
    Result = RegOpenKeyEx (RootKey, SubKey.c_str(), 0, KEY_READ, &OpenKey);

    if (Result != ERROR_SUCCESS)
        return (false);

    for (i = 0; i < Vars.size(); i++)
    {
        switch (Vars[i].GetType())
        {
            case VTInvalid:
                // Do not save this variable.
                break;

            case VTUint64:
                uint64 QWord;

                QWord = Vars[i].GetValUint64();
                RegSetValueEx (OpenKey, Vars[i].Name.c_str(), NULL, REG_QWORD, (const unsigned char *)&QWord, sizeof (uint64));
                break;

            case VTUint32:
                DWORD DWord;

                DWord = Vars[i].GetValUint32();
                RegSetValueEx (OpenKey, Vars[i].Name.c_str(), NULL, REG_DWORD, (const unsigned char *)&DWord, sizeof (DWORD));
                break;

            case VTString:
                RegSetValue (OpenKey, Vars[i].Name.c_str(), REG_SZ, 
                    (LPCSTR)Vars[i].GetValString().c_str(), 
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

                    RegSetValueEx (OpenKey, Vars[i].Name.c_str(), NULL, REG_BINARY,
                        (const unsigned char *) Array, Binary.size());

                    delete Array;
                }
                break;
        }
    }

    return (true);
}


void TestIt (void)
{
    VarListExt List;
    VarNamedExt Var;
    uint64 Int64;
    uint32 Int32;

    Var.Name = "hi";
    Var.Value = "53";
    Var.Type = VTUint64;
    List.SetVar (Var.Name, Var.GetAnon());
    Int64 = List.GetValUint64 ("hi");        
    List.SetValUint32 ("hi", 53);
    Int32 = List.GetValUint32 ("hi");
    List.Union (List);
    SaveVarsToReg (List);
    LoadVarsFromReg (List);
    List.SetDefaultsList (NULL);
    List.SetRegistryKey (NULL, "");
}

