/*****************************************************************************

  CSignal

  A class for an event object (a signal).

*****************************************************************************/


#ifndef _CSIGNAL_H
#define _CSIGNAL_H


#include <windows.h>


class CSignal
{
public:
    CSignal ()
    {
        guard
        {
            Event = CreateEvent (NULL, TRUE, FALSE, NULL);
            return;
        } unguard;
    }

    CSignal (bool Signaled)
    {
        guard
        {
            Event = CreateEvent (NULL, TRUE, Signaled ? TRUE : FALSE, NULL);
            return;
        } unguard;
    }

    ~CSignal ()
    {
        guard
        {
            CloseHandle (Event);
            return;
        } unguard;
    }

    void Signal (void)
    {
        guard
        {
            SetEvent (Event);
            return;
        } unguard;
    }

    void Unsignal (void)
    {
        guard
        {
            ResetEvent (Event);
            return;
        } unguard;
    }

    HANDLE GetEventObject (void)
    {
        guard
        {
            return (Event);
        } unguard;
    }

    DWORD WaitUntilSignaled (DWORD TimeOut = INFINITE)
    {
        guard
        {
            return (WaitForSingleObject (Event, TimeOut));
        } unguard;
    }

protected:
    HANDLE Event;
};


#endif // _CSIGNAL_H