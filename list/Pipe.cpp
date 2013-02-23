#include "Pipe.h"

using namespace std;

Pipe::Pipe (HANDLE InputHandle, const string &OutputName)
    : input (InputHandle),
      output (OutputName.c_str()),
      filename (OutputName),
      do_quit (false),
      done (false)
{
    return;
}


Pipe::~Pipe ()
{
    do_quit = true;
    done.WaitUntilSignaled ();
    output.flush ();
    output.close ();
    CloseHandle (input);
    return;
}


void Pipe::DrainPipe (void)
{
    char Buffer[512];
    DWORD AmountRead = 1;
    BOOL Result = TRUE;

    do
    {
        Result = ReadFile (input, (void *)Buffer, sizeof (Buffer) - 1, &AmountRead, NULL);

        if (AmountRead > 0)
        {
            Buffer[AmountRead] = '\0';
            output << Buffer;
            output.flush ();
        }

        DWORD gle;
        if (!Result && (gle = GetLastError()) != ERROR_MORE_DATA)
            break;
        /*
        {
            string Reason;
            DWORD WinError;
            char ErrorName[1024];

            // Figure out why
            //WinError = LC->FileContext->File->GetLastWin32Error ();

            FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM,
                           0,
                           gle,
                           0,
                           ErrorName,
                           sizeof (ErrorName),
                           NULL);

            MessageBox (NULL, ErrorName, "Pipe Read Error", MB_OK | MB_ICONERROR);
        }
        */

        if (do_quit)
            break;

    } while (Result); // repeat loop if ERROR_MORE_DATA

    return;
}


bool Pipe::ThreadFunction (void)
{
    SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_LOWEST);

    do
    {
        DWORD WaitResult;

        DrainPipe ();

        Sleep (250);
        WaitResult = WaitForSingleObject (input, 50);

        if (WaitResult == WAIT_ABANDONED)
            break;

    } while (!do_quit);

    done.Signal ();
    return (false);
}


string Pipe::GetFileName (void)
{
    return (filename);
}

