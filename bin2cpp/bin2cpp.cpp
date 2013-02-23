/*****************************************************************************

  bin2cpp

  Converts any binary file into a .h/.cpp file that can be #include-d into
  a program and thus incorporated into a project instead of lying around as
  a file dependency.

*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


// Returns true if the filesize could be obtained, and places the filesize 
// in the integer pointed to by Size. Otherwise false is returned.
bool GetFileSize (const char *Filename, unsigned int *Size)
{
	FILE *File;

	File = fopen (Filename, "rb");

	if (File == NULL) 
        return (false);

	fseek (File, 0, SEEK_END);
	*Size = ftell (File);
	fclose (File);

	return (true);
}


void PrintHelp (void)
{
    printf
    (
        "bin2cpp -- Convert binary file to .h/.cpp\n"
        "\n"
        "Usage: bin2cpp <input file> <output name> <variable name> [-c] [-f]\n"
        "\n"
        "The output files will be <output name>.h and <output name>.cpp, or if the -c\n"
        "option is used, <output name>.c will be used. In the header files, the array\n"
        "of byte files that represents the <input file> will be called <variable name>.\n"
        "The -f option will cause the output numbers to be the complement of the input\n"
        "file. In order to use the data properly you will need to flip all the bits of\n"
        "the array (i.e., j = ~array[i]). For convenience, a function called\n"
        "'void Decreypt<variable name> (void)' will be provided to decrypt the data.\n"
        "This function will keep track of whether it has decrypted the data or not,\n"
        "and will not decrypt it twice.\n"
    );

    return;
}


int main (int argc, char **argv)
{
    bool UseCExt = false;
    bool Encrypt = false;
    char *InputName = NULL;
    char *OutputPrefix = NULL;
    char *VariableName = NULL;
    char *UpcaseVariableName = NULL;
    char *HeaderFileName = NULL;
    char *DumpFileName = NULL;
    FILE *HeaderFile = NULL;
    FILE *DumpFile = NULL;
    FILE *InputFile = NULL;
    int c;
    unsigned int InputSize = 0;
    unsigned int i;

    if (argc < 4  ||  argc > 6)
    {
        PrintHelp ();
        return (0);
    }

    // Check for -c option
    if (argc >= 5)
    {
        for (int i = 4; i < argc; i++)
        {
            if (argv[i][0] == '-') // we have an option
            {
                char b;
                
                b = tolower (argv[i][1]);
                switch (b)
                {
                    case 'c':
                        UseCExt = true;
                        break;

                    case 'f':
                        Encrypt = true;
                        break;
                }
            }
        }
    }

    // Get filenames
    InputName = argv[1];
    OutputPrefix = argv[2];
    VariableName = argv[3];
    UpcaseVariableName = strdup (VariableName);
    UpcaseVariableName = strupr (UpcaseVariableName);

    if (!GetFileSize (InputName, &InputSize))
    {
        printf ("Could not open '%s' to obtain file size\n", InputName);
        return (-1);
    }

    printf ("Input file: %s (%u bytes)\n", InputName, InputSize);

    // Print header file
    HeaderFileName = (char *) malloc ((strlen (OutputPrefix) + strlen (".h") + 1) * sizeof (char));
    sprintf (HeaderFileName, "%s.h", OutputPrefix);

    printf ("Writing %s ... ", HeaderFileName);

    HeaderFile = fopen (HeaderFileName, "wt");
    if (HeaderFile == NULL)
    {
        printf ("Could not open '%s' for writing\n", HeaderFileName);
        free (HeaderFileName);
        return (-1);
    }

    fprintf (HeaderFile, "/*****************************************************************************\n");
    fprintf (HeaderFile, "\n");
    fprintf (HeaderFile, "    %s\n", HeaderFileName);
    fprintf (HeaderFile, "\n");
    fprintf (HeaderFile, "*****************************************************************************/\n");
    fprintf (HeaderFile, "\n");
    fprintf (HeaderFile, "#ifndef __%s__H\n", UpcaseVariableName);
    fprintf (HeaderFile, "#define __%s__H\n", UpcaseVariableName);
    fprintf (HeaderFile, "\n");
    fprintf (HeaderFile, "extern unsigned char %s[%u];\n", VariableName, InputSize);
    fprintf (HeaderFile, "static const unsigned int %sSize = %u;\n", VariableName, InputSize);
    if (Encrypt)
        fprintf (HeaderFile, "extern void Decrypt%s (void);\n", VariableName);
    fprintf (HeaderFile, "\n");
    fprintf (HeaderFile, "#endif /* __%s__H */\n", UpcaseVariableName);
    fprintf (HeaderFile, "\n");

    fclose (HeaderFile);

    // Print .cpp/.c file
    DumpFileName = (char *) malloc ((strlen (OutputPrefix) + strlen (".cpp") + 1) * sizeof (char));
    sprintf (DumpFileName ,"%s.%s", OutputPrefix, UseCExt ? "c" : "cpp");

    printf ("\nWriting %s ... ", DumpFileName);

    DumpFile = fopen (DumpFileName, "wt");
    if (DumpFile == NULL)
    {
        printf ("Could not open '%s' for writing\n", DumpFileName);
        return (-1);
    }

    fprintf (DumpFile, "/*****************************************************************************\n");
    fprintf (DumpFile, "\n");
    fprintf (DumpFile, "    %s\n", DumpFileName);
    fprintf (DumpFile, "\n");
    fprintf (DumpFile, "*****************************************************************************/\n");
    fprintf (DumpFile, "\n");
    fprintf (DumpFile, "#include \"%s\"\n", HeaderFileName);
    fprintf (DumpFile, "\n");
    if (Encrypt)
    {
        fprintf (DumpFile, "bool %sDecrypted = %s;\n", VariableName, UseCExt ? "0" : "false");
        fprintf (DumpFile, "\n");
    }
    fprintf (DumpFile, "unsigned char %s[%u] =\n", VariableName, InputSize);
    fprintf (DumpFile, "{");

    // Now open file for input and dump it all in here
    InputFile = fopen (InputName, "rb");

    // Not the most efficient way to do it, but it works and it's "fast enough"
    for (i = 0; i < InputSize; i++)
    {
        if ((i % 20) == 0)
            fprintf (DumpFile, "\n    ");

        c = fgetc (InputFile);
        if (Encrypt)
            c = 255 - c;

        fprintf (DumpFile, "%u", c);

        if (i != InputSize - 1)
            fprintf (DumpFile, ", ");

    }

    if ((InputSize % 20) != 0)
        fprintf (DumpFile, "\n");

    fprintf (DumpFile, "};");
    fprintf (DumpFile, "\n");

    if (Encrypt)
    {
        fprintf (DumpFile, "\n");
        fprintf (DumpFile, "void Decrypt%s (void)\n", VariableName);
        fprintf (DumpFile, "{\n");
        fprintf (DumpFile, "    unsigned int i;\n");
        fprintf (DumpFile, "\n");
        fprintf (DumpFile, "    if (%sDecrypted == %s)\n", VariableName, UseCExt ? "1" : "true");
        fprintf (DumpFile, "        return;\n");
        fprintf (DumpFile, "\n");
        fprintf (DumpFile, "    for (i = 0; i < %sSize; i++)\n", VariableName);
        fprintf (DumpFile, "        %s[i] = 255 - %s[i];\n", VariableName, VariableName);
        fprintf (DumpFile, "\n");
        fprintf (DumpFile, "    %sDecrypted = %s;\n", VariableName, UseCExt ? "1" : "true");
        fprintf (DumpFile, "    return;\n");
        fprintf (DumpFile, "};\n");
        fprintf (DumpFile, "\n");
    }

    fclose (DumpFile);

    free (DumpFileName);
    free (HeaderFileName);

    printf ("\n");

    return (0);
}
