/*****************************************************************************

   Pipe.h

   Redirects data from a pipe to an output file. This is useful for taking
   data from the standard input stream. 

*****************************************************************************/

#ifndef _PIPE_H
#define _PIPE_H

#include "CThread.h"
#include "CSignal.h"
#include <fstream>

class Pipe : public CThread
{
public:
    Pipe (HANDLE InputHandle, const std::string &OutputName);
    ~Pipe ();

    bool ThreadFunction (void);
    std::string GetFileName (void);

protected:
    void DrainPipe (void);

private:
    HANDLE input;
    std::ofstream output;
    std::string filename;
    bool do_quit;
    CSignal done;
};

#endif // _PIPE_H