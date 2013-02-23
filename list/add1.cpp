#include <stdio.h>
#include <stdlib.h>

void main (int argc, char **argv)
{
    int x;
    char buf[1024];
    char Name[1024];
    FILE *out;

    fgets (buf, 1024, stdin);
    x = 1 + atoi (buf);

    sprintf (Name, "%s.h", argv[1]);
    out = fopen (Name, "wt");

    // Use: type BuildNumber.txt | add1 BuildNumber ListXP _BUILDNUMBER_H > BuildNumber.txt

    fprintf (out, 
        "/*****************************************************************************\n"
        "\n"
        "    %s\n"
        "\n"
        "    Keeps track of the %s build number\n"
        "\n"
        "*****************************************************************************/\n"
        "\n"
        "#ifndef %s\n"
        "#define %s\n"
        "\n"
        "extern unsigned int %sBuildNumber;\n"
        "\n"
        "#endif // %s\n",

        argv[1], argv[2], argv[3], argv[3], argv[2], argv[3]);

    fclose (out);

    sprintf (Name, "%s.cpp", argv[1]);
    out = fopen (Name, "wt");

    fprintf (out,
        "#include \"%s.h\"\n"
        "\n"
        "unsigned int %sBuildNumber = %d;\n",

        argv[1], argv[2], x);

    fclose (out);

    printf ("%d\n", x);        
    return;
}
