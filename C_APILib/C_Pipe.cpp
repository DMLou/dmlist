#include "C_Pipe.h"


// Reallocate in 1K chunks
#define CHUNKSIZE 1024


C_Pipe::C_Pipe (C_ObjectHeap *Heap)
: C_Object (Heap)
{
    ReadCursor = 0;
    WriteCursor = 0;
    CreateMemHeap ();
    PipeData = (char *) malloc (CHUNKSIZE);
    DataSize = CHUNKSIZE;
    return;
}


C_Pipe::~C_Pipe ()
{
    free (PipeData);
    return;
}


void C_Pipe::WriteString (char *String)
{
    DWORD Len;
//    PipeData += String;

    Len = strlen (String);

    // Are we going to add too much? If so, expand the buffer
    if (Len > (DataSize - WriteCursor))
    {
        DataSize = (((WriteCursor + Len) / CHUNKSIZE) + 1) * CHUNKSIZE;
        PipeData = (char *) realloc (PipeData, DataSize);
    }

    strcpy (&(PipeData[WriteCursor]), String);
    WriteCursor += Len;
    return;
}


void C_Pipe::WriteChar (char Char)
{
    char add[2];

    add[1] = '\0';
    add[0] = Char;
    WriteString (add);

    return;
}


bool C_Pipe::IsCharAvail (void)
{ 
    return (!(bool(ReadCursor == WriteCursor)));
}


bool C_Pipe::ReadChar (char *Result)
{
    if (!IsCharAvail())
        return (false);

    *Result = PipeData[ReadCursor];
    ReadCursor++;
    return (true);
}


string C_Pipe::GetAllPipeData (void)
{
    ReadCursor = WriteCursor;
    return (PipeData);
}


DWORD C_Pipe::GetMemUsage (void)
{ 
    return (C_Object::GetMemUsage() + WriteCursor + sizeof (C_Pipe));
}
