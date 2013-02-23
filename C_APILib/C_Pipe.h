/*****************************************************************************

  C_Pipe

  Pipes support for Consframe. Note that pipes are done "DOS style", in which
  piping is not done in real-time. For instance, the following command if
  issued in Unix or Win2K:

  dir | sort > blah.txt

  ... will run dir and sort and the file output simultaneously. In DOS (and in
  Consframe) the dir command is issued first with its output put in a
  container (temp file in DOS usually, memory with Consframe) and then run
  through sort (into another container) and then out to blah.txt.

*****************************************************************************/


#ifndef C_PIPE_H
#define C_PIPE_H


#include "C_Types.h"
#include "C_Object.h"
#include <string>


class CAPILINK C_Pipe : public C_Object
{
public:
    C_Pipe (C_ObjectHeap *Heap);
    ~C_Pipe ();

    void WriteString (char *String);
    void WriteChar   (char Char);

    bool IsCharAvail (void);
    bool ReadChar    (char *Result);

    string GetAllPipeData (void);

    DWORD GetMemUsage (void);

private:
    //string PipeData;

    char *PipeData;
    DWORD DataSize; // how much is allocated
    DWORD ReadCursor;   // for reading data
    DWORD WriteCursor;  // for writing data (duh?)
};


#endif // C_PIPE_H
