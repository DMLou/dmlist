/*****************************************************************************

  CScannerThread

*****************************************************************************/


#ifndef _CSCANNERTHREAD_H
#define _CSCANNERTHREAD_H


#include "ListTypes.h"
#include "CThread.h"


class CFastFile;
class CParser2;
class CFileLinesDB;

// Callback function that is called when the scanner thread finished.
// It passes one parameter to indicate whether it completed correctly (true) or
// aborted due to an error (false)
// And one parameter for context's sake
typedef void (*CSTCompletionCallback) (bool, void *);


class CScannerThread : public CThread
{
public:
    CScannerThread (CFileLinesDB *LinesDB, // pointer to our 'host' class
                    uint64 StartLine,      // which line # to start scanning from (usualy 0)
                    CSTCompletionCallback CallBack,
                    void *CallbackContext
                    );     

    ~CScannerThread ();

    bool ThreadFunction (void);

private:
    void LoadFromHive (void);
    void SaveToHive (void);

    uint64 StartLine;

    CFileLinesDB *LinesDB;
    uint64 HiveTotalLines;
    uint32 HiveLongestLine;
    bool   DoHiveSave;
    CFastFile *TheFile;
    CParser2 *Parser;
    bool   SkipReading;
    uint32 TabSize;
    width_t MaxWidth;

    CSTCompletionCallback Callback;
    void *CallbackContext;
};


#endif // _CSCANNERTHREAD_H