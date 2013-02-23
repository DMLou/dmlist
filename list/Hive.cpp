#include "Hive.h"
#include <shlobj.h>
#include <winioctl.h>


static string HivePath = string ("");
static bool HiveEnabled = false;
static uint64 HiveMinSize = 1024 * 1024; // 1MB ... note: this actually defaults to 20mb if you look at ListDefaults/GlobalSettings
static bool HiveCompression = false;


typedef BOOL (WINAPI *DeviceIoControlFnPtr) (HANDLE, DWORD, LPVOID, DWORD, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);


bool MakeCompressedFile (const string &FilePath)
{
    guard
    {
        HMODULE Kernel32;
        HANDLE File;
        DeviceIoControlFnPtr _DeviceIoControl;
        USHORT CType;
        DWORD Bytes;
        bool Result;

        Kernel32 = LoadLibrary ("kernel32.dll");

        if (Kernel32 == NULL)
            return (false);

        _DeviceIoControl = (DeviceIoControlFnPtr) GetProcAddress (Kernel32, "DeviceIoControl");

        if (_DeviceIoControl == NULL)
        {
            FreeLibrary (Kernel32);
            return (false);
        }

        File = CreateFile (FilePath.c_str(), 
                           GENERIC_READ | GENERIC_WRITE,
                           0,
                           NULL,
                           OPEN_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);

        if (File == INVALID_HANDLE_VALUE)
            return (false);

        CType = COMPRESSION_FORMAT_DEFAULT;

        Result = _DeviceIoControl (File,
                                   FSCTL_SET_COMPRESSION,
                                   &CType,
                                   sizeof(USHORT),
                                   NULL,
                                   0,
                                   &Bytes,
                                   NULL) ? true : false;

        CloseHandle (File);        
        FreeLibrary (Kernel32);
        return (Result);
    } unguard;
}


bool InitializeHivePath (const string &HivePath)
{
    guard
    {
        return (bool (CreateDirectory (HivePath.c_str(), NULL)));
    } unguard;
}


// Initializes the hive to use within the program
bool SetHivePath (const string &NewHivePath)
{
    guard
    {
        HivePath = NewHivePath;
        return (InitializeHivePath (HivePath));
    } unguard;
}


void SetHiveEnabled (bool OnOrOff)
{
    guard
    {
        HiveEnabled = OnOrOff;
        return;
    } unguard;
}


void SetHiveMinSize (uint64 MinSize)
{
    guard
    {
        HiveMinSize = MinSize;
        return;
    } unguard;
}


uint64 GetHiveMinSize (void)
{
    guard
    {
        return (HiveMinSize);
    } unguard;
}


bool IsHiveEnabled (void)
{
    guard
    {
        return (HiveEnabled);
    } unguard;
}


string MakeHashString (const HiveData::FileHashType &HashInput)
{
    guard
    {
        char Temp[5];
        string Result;
        uint8 *p;
        int i;

        p = (uint8 *)&HashInput;
        Result = string ("");
        for (i = 0; i < sizeof (HiveData::FileHashType); i++)
        {
            sprintf (Temp, "%02x", p[i]);
            Result += string (Temp);
        }        

        return (Result);
    } unguard;
}


bool FillInHiveHashData (HANDLE FileHandle, 
                         uint16 TabSize, 
                         uint16 MaxWidth, 
                         uint16 ParserOrdinal, 
                         uint16 SeekGranLog2,
                         HiveData *Data)
{
    guard
    {
        BY_HANDLE_FILE_INFORMATION Info;

        GetFileInformationByHandle (FileHandle, &Info);
        memset (&Data->Hash, 0, sizeof (Data->Hash));
        Data->Hash.FileIndexHigh = Info.nFileIndexHigh;
        Data->Hash.FileIndexLow = Info.nFileIndexLow;
        Data->Hash.VolumeSerial = Info.dwVolumeSerialNumber;
        Data->Hash.TabSize = uint8(TabSize);
        Data->Hash.MaxLineWidth = MaxWidth;
        Data->Hash.ParserOrdinal = uint8(ParserOrdinal);
        Data->Hash.SeekGranLog2 = uint8(SeekGranLog2);
        Data->Hash.HashRevision = HASHREVISION;

        return (true);
    } unguard;
}


bool FillInHiveSigData (HANDLE FileHandle, HiveData *Data)
{
    guard
    {
        BY_HANDLE_FILE_INFORMATION Info;

        GetFileInformationByHandle (FileHandle, &Info);
        memset (&Data->Signature, 0, sizeof (Data->Signature));
        Data->Signature.CreationTime = Info.ftCreationTime;
        Data->Signature.LastWriteTime = Info.ftLastWriteTime;
        Data->Signature.FileSize = uint64(Info.nFileSizeLow) + (uint64(Info.nFileSizeHigh) << 32);

        return (true);
    } unguard;
}


bool AreFilesEquivalent (HiveData *LHS, HiveData *RHS)
{
    guard
    {
        if (LHS->Hash.FileIndexHigh == RHS->Hash.FileIndexHigh &&
            LHS->Hash.FileIndexLow  == RHS->Hash.FileIndexLow  &&
            LHS->Hash.TabSize       == RHS->Hash.TabSize       &&
            LHS->Hash.MaxLineWidth  == RHS->Hash.MaxLineWidth  &&
            LHS->Hash.ParserOrdinal == RHS->Hash.ParserOrdinal &&
            LHS->Hash.SeekGranLog2  == RHS->Hash.SeekGranLog2  &&
            LHS->Hash.HashRevision  == RHS->Hash.HashRevision  &&
            LHS->Hash.VolumeSerial  == RHS->Hash.VolumeSerial  &&
            LHS->Signature.CreationTime.dwHighDateTime  == RHS->Signature.CreationTime.dwHighDateTime   &&
            LHS->Signature.CreationTime.dwLowDateTime   == RHS->Signature.CreationTime.dwLowDateTime    &&
            LHS->Signature.FileSize                     == RHS->Signature.FileSize                      &&
            LHS->Signature.LastWriteTime.dwHighDateTime == RHS->Signature.LastWriteTime.dwHighDateTime  &&
            LHS->Signature.LastWriteTime.dwLowDateTime  == RHS->Signature.LastWriteTime.dwLowDateTime)
        {
            return (true);
        }

        return (false);
    } unguard;
}


bool SaveHiveData (const HiveData *Data)
{
    guard
    {
        FILE *out;
        string FileName;
        vector<uint64>::size_type SeekPointCount;
        uint64 TempI64;
        uint8 TempByte;
        int i;

        FileName = HivePath + string("\\") + MakeHashString (Data->Hash) + string (".") + string(INDEXEXT);

        if (HiveCompression)
            MakeCompressedFile (FileName);

        out = fopen (FileName.c_str(), "wb");

        if (out == NULL)
            return (false);

        SeekPointCount = Data->SeekPoints.size();
        fwrite (&SeekPointCount, sizeof (SeekPointCount), 1, out);
        fwrite (&Data->Signature, sizeof (Data->Signature), 1, out);

        for (i = 0; i < SeekPointCount; i++)
        {
            TempI64 = Data->SeekPoints[i].Offset;
            fwrite (&TempI64, sizeof (TempI64), 1, out);
        }

        // Pack the boolean values 8 values to a byte
        for (i = 0; i < SeekPointCount; i += 8)
        {
            int j;
            int ord;

            TempByte = 0;

            // Pack the bytes down to bits
            for (j = i, ord = 0; 
                ord < 8  &&  j < SeekPointCount;
                j++, ord++)
            {
                TempByte |= Data->SeekPoints[j].Dependent ? (1 << ord) : 0;
            }

            //TempB = Data->SeekPoints[i].Dependent;
            fwrite (&TempByte, sizeof (TempByte), 1, out);
        }

        fclose (out);
        return (true);
    } unguard;
}


bool LoadHiveData (HiveData *DataResult)
{
    guard
    {
        FILE *in;
        string FileName;
        vector<uint64>::size_type SeekPointCount;
        uint64 TempI64;
        uint8 TempByte;
        int i;

        FileName = HivePath + string ("\\") + MakeHashString (DataResult->Hash) + string (".") + string(INDEXEXT);
        in = fopen (FileName.c_str(), "rb");

        if (in == NULL)
            return (false);

        fread (&SeekPointCount, sizeof (SeekPointCount), 1, in);
        DataResult->SeekPoints.resize (SeekPointCount);
        fread (&DataResult->Signature, sizeof (DataResult->Signature), 1, in);

        for (i = 0; i < SeekPointCount; i++)
        {
            fread (&TempI64, sizeof (TempI64), 1, in);
            DataResult->SeekPoints[i].Offset = TempI64;
        }

        for (i = 0; i < SeekPointCount; i += 8)
        {
            int ord;
            int j;

            fread (&TempByte, sizeof (TempByte), 1, in);

            // Unpack the bits into bytes
            for (j = i, ord = 0; 
                j < i + 8  &&  j < SeekPointCount;
                j++, ord++)
            {
                DataResult->SeekPoints[j].Dependent = ((TempByte & (1 << ord)) != 0) ? true : false;
            }

            /*
            fread (&TempB, sizeof (TempB), 1, in);
            DataResult->SeekPoints[i].Dependent = TempB;
            */
        }

        fclose (in);
        return (true);
    } unguard;
}


typedef HRESULT (WINAPI *SHGetFolderPathFunction) (HWND, int, HANDLE, DWORD, LPSTR);
#define HIVESUBDIR "ListXP"


bool GetModulePath (string &PathResult)
{
    guard
    {
        char Path[MAX_PATH + 1];

        GetModuleFileName (GetModuleHandle (NULL), Path, sizeof (Path));

        if (strrchr (Path, '\\') == NULL)
            return (false);

        *strrchr (Path, '\\') = '\0';
        PathResult = string (Path);
        return (true);
    } unguard;
}


bool GetDefaultHivePath (string &LocationResult)
{
    guard
    {
        HMODULE Module;
        SHGetFolderPathFunction SHGFP;
        char Path[2 * MAX_PATH + 1];
        string TempString;

        // If we are in Windows 2000/XP or later we want to put our hive into
        // ~/Application Data/ListXP
        // This is usually in a place like C:\Documents and Settings\*username*\ApplicationData\ListXP
        // If that doesn't exist then we try to find the place where My Documents is
        // And if THAT doesn't work then we try the same directory that we are located in (List.exe), plus \\ListXP
        // And, shit, if that doesn't work then we just try C:\ListXP
        Module = LoadLibrary ("shell32.dll");

        if (Module == NULL)
        {
            if (!GetModulePath (TempString))
                strcpy (Path, "C:\\" HIVESUBDIR);
            else
                sprintf (Path, "%s\\" HIVESUBDIR, TempString.c_str());
        }
        else
        {
            SHGFP = (SHGetFolderPathFunction) GetProcAddress (Module, "SHGetFolderPathA");

            if (SHGFP == NULL)
            {
                if (!GetModulePath (TempString))
                    strcpy (Path, "C:\\" HIVESUBDIR);
                else
                    strcpy (Path, TempString.c_str());
            }
            else
            {
                if (S_OK == SHGFP (NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, Path))
                    strcat (Path, "\\" HIVESUBDIR);
                else
                {
                    if (S_OK == SHGFP (NULL, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, Path))
                        strcat (Path, "\\" HIVESUBDIR);
                    else
                    {
                        if (!GetModulePath (TempString))
                            strcpy (Path, "C:\\" HIVESUBDIR);
                        else
                            strcpy (Path, TempString.c_str());
                    }
                }
            }

            FreeLibrary (Module);
        }

        LocationResult = string (Path);
        return (true);
    } unguard;
}


bool IsHiveCompressionSupported (void)
{
    guard
    {
        if (::GetVersion() < 0x80000000)
            return (true);
        else
            return (false);
    } unguard;
}


void SetHiveCompression (bool OnOrOff)
{
    guard
    {
        HiveCompression = OnOrOff;
    } unguard;
}

bool IsHiveCompressionEnabled (void)
{
    guard
    {
        return (HiveCompression);
    } unguard;
}
