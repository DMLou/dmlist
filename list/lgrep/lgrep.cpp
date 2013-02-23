/*****************************************************************************

  lgrep

  Command-line filter that uses ListXP's boolean evaluator

*****************************************************************************/


#include "../CSearchBoolean.h"
#include "../List.h"
#include "../BuildNumber.h"
#include <stdio.h>
#include <string>
#include <windows.h>


//#define DEBUG

#define LGREPTITLE "lgrep"
#define LGREPVERSION "0.2"


typedef vector<string> stringvec;
typedef vector<WIN32_FIND_DATA> filevec;
typedef unsigned __int64 uint64;


// Lookup table to avoid using tolower(), which is a function with
// external linkage that can be replaced with this simple table lookup.
// Initialized in ListThread
wchar_t ToLowerTable[65536];
wchar_t IdentityTable[65536];


filevec GetFileNames (string FileSpec = "*.*", DWORD Attributes = 0xffffffff)
{
    WIN32_FIND_DATA FindData;
    HANDLE FindHandle;
    BOOL Result;
    filevec Return;

    FindHandle = FindFirstFile (FileSpec.c_str(), &FindData);
    Result = TRUE;

    do
    {
        if (FindData.cFileName[0] != '.'  &&  FindData.dwFileAttributes & Attributes)
            Return.push_back (FindData);

        Result = FindNextFile (FindHandle, &FindData);

    } while (Result == TRUE);

    return (Return);
}


filevec GetDirNames (void)
{
    return (GetFileNames ("*.*", FILE_ATTRIBUTE_DIRECTORY));
}


bool DoStreamMatch (string Pattern, 
                    bool MatchCase, 
                    FILE *stream, 
                    bool PrintMatches = true, 
                    bool PrintOnFM = false, 
                    string FMText = "")
{
    char *buffer;
    bool FirstMatch = false;
    bool AnyMatch = false;
    wchar_t *PatternW = StrDupA2W (Pattern.c_str());
    CSearchBoolean Engine (MatchCase);

    buffer = new char[65536];
    Engine.CompileSearch (PatternW);
    delete PatternW;

    while (!feof(stream))
    {
        wchar_t *BufferW;

        fgets (buffer, 65536, stream);
        BufferW = StrDupA2W (buffer);

        if (Engine.MatchPattern (BufferW, (int)strlen(buffer), NULL))
        {
            if (FirstMatch == false  &&  PrintOnFM)
            {
                FirstMatch = true;
                printf ("%s\r\n", FMText.c_str());
            }

            if (PrintMatches)
                printf ("%s", buffer);

            AnyMatch = true;

            if (!PrintMatches)
                break;
        }

        delete BufferW;
    }

    if (PrintOnFM && AnyMatch && PrintMatches)
        printf ("\n");

    delete buffer;
    return (AnyMatch);
}


void DoFilesMatch (string Pattern, 
                   bool MatchCase,
                   string DirName,
                   string FileSpec, 
                   bool Recursive,
                   bool ShowContents = true)
{
    filevec Dirs;
    filevec Files;
    filevec::size_type i;

    Dirs = GetDirNames ();
    Files = GetFileNames (FileSpec.c_str());

    for (i = 0; i < Files.size(); i++)
    {
        FILE *file;

        file = fopen (Files[i].cFileName, "rb");

        if (file != NULL)
        {
            string FileName;

            FileName = DirName + ((DirName.length() > 0) ? string("\\") : string ("")) + 
                string (Files[i].cFileName);

            DoStreamMatch (Pattern, MatchCase, file, ShowContents, true, FileName);
            fclose (file);
        }        
    }

    if (Recursive)
    {
        for (i = 0; i < Dirs.size(); i++)
        {
            BOOL Result;
            
            Result = SetCurrentDirectory (Dirs[i].cFileName);

            if (Result)
            {
                DoFilesMatch (Pattern, MatchCase, DirName + ((DirName.length() > 0) ? string("\\") : string ("")) + 
                    string(Dirs[i].cFileName), 
                    FileSpec, Recursive, ShowContents);

                SetCurrentDirectory ("..");
            }
        }

    }
    return;
}


int main (int argc, char **argv)
{
    string Pattern = string ("");
    string FileSpec = string ("");
    bool UseFileSpec = false;
    bool Recursive = false;
    bool MatchCase = false;
    bool ShowContents = true;
    int startarg = 1;
    int i;

    if (argc < 2)
    {
        printf ("You must specify some command-line arguments. Use 'lgrep -?' for help.\n");
        return (1);
    }

    // Initialize tables
    for (i = 0; i < 65536; i++)
    {
        ToLowerTable[i] = towlower((wchar_t)i);
        IdentityTable[i] = (wchar_t)i;
    }

    // Parse for arguments
    // -f [string]  = specify filespec
    // -r           = recursive
    // -l           = literal
    // -x           = regex
    // -i           = match case
    // -w           = match against whole file and just show filenames
    // -?           = help!
    // -m           = just show matching filenames
    i = 1;
    while (argv[i][0] == '-'  &&  i < argc)
    {
        switch (tolower(argv[i][1]))
        {
            case 'f':
                FileSpec = string (argv[i + 1]);
                UseFileSpec = true;
                i++;
                startarg++;
#ifdef DEBUG
                printf ("Using filespec: '%s'\n", FileSpec.c_str());
#endif
                break;

            case 'r':
                Recursive = true;
#ifdef DEBUG
                printf ("Recursive subdirectories\n");
#endif
                break;

            case 'i':
                MatchCase = true;
#ifdef DEBUG
                printf ("Enabling case sensitivity\n");
#endif
                break;

            case 'm':
                ShowContents = false;
                break;

            case 'l':
                break;

            case 'x':
                break;

            case '?':
                printf ("%s %s (ListXP Build %d), by Rick Brewster\n", LGREPTITLE, LGREPVERSION, ListXPBuildNumber);
                printf ("syntax: lgrep [-f <spec | -s> [-r]] [-i] boolean_expression\n");
                printf ("\n");
                printf ("-f spec  --  Read from files matching 'spec' (ala *.*) instead of stdin\n");
                printf ("-s       --  Read filenames from stdin\n");
                printf ("-r       --  Recurse through subdirectores\n");
                printf ("-i       --  Turn on case-sensitivity\n");
                printf ("-m       --  Only display filenames of files that match\n");
                printf ("             (useful with piping and -f -c)\n");
                printf ("\n");
                return (0);
        }

        i++;
        startarg++;
    }

#ifdef DEBUG
    printf ("Starting i for argv[i] is %d\n", startarg);
#endif

    // Build complete command line string
    Pattern = string ("");
    for (i = startarg; i < argc; i++)
    {
        if (i > startarg)
            Pattern += string (" ");

        Pattern += string (argv[i]);
    }

#ifdef DEBUG
    printf ("Matching against pattern: '%s'\n", Pattern.c_str());
#endif

    // Now go!
    if (!UseFileSpec)
        DoStreamMatch (Pattern, MatchCase, stdin);
    else
    {
        if (FileSpec == "-s"  ||  FileSpec == "-S")
        {
            while (!feof(stdin))
            {
                char Name[2048];
                FILE *file;

                fgets (Name, sizeof(Name), stdin);

                if (strchr (Name, '\r') != NULL)
                    *strchr (Name, '\r') = '\0';

                if (strchr (Name, '\n') != NULL)
                    *strchr (Name, '\n') = '\0';

                file = fopen (Name, "rb");

                if (file != NULL)
                {
                    if (DoStreamMatch (Pattern, MatchCase, file, ShowContents))
                        printf ("%s\n", Name);

                    fclose (file);
                }
            }
        }
        else
        {
           DoFilesMatch (Pattern, MatchCase, ".", FileSpec, Recursive, ShowContents);
        }
    }

    return (0);
}
