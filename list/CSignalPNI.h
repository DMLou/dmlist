/*****************************************************************************

    CSignalPNI.h

    Provides a signal object optimized for PNI (Prescott New Instructions)
    taking advantage of the MONITOR and MWAIT instructions.

    Note: This will not compile until I have a compiler that supports PNI!
          Not testable until I have a Prescott, or a tester with one!

    NOTE: This code is not used!

*****************************************************************************/

#ifndef _CSIGNALPNI_H
#define _CSIGNALPNI_H

class CSignalPNI
{
public:
    CSignalPNI ()
    {
        guard
        {
            Flag = 0;
            return;
        } unguard;
    }

    ~CSignal ()
    {
        guard
        {
            return;
        } unguard;
    }

    void Signal (void)
    {
        guard
        {
            Flag = 1;
            return;
        } unguard;
    }

    void Unsignal (void)
    {
        guard
        {
            Flag = 0;
            return;
        } unguard;
    }

    HANDLE GetEventObject (void)
    {
        guard
        {
            guard_throw (range_error ("CSignalPNI does not support GetEventObject method"));
            return (NULL);
        } unguard;
    }

    DWORD WaitUntilSignaled (DWORD TimeOut = INFINITE)
    {
        guard
        {
            if (TimeOut != INFINITE)
                guard_throw (domain_error ("CSignalPNI::WaitUntilSignaled does not support TimeOut != INFINITE"));

            __asm
            {
                lea esi, Flag

            pniwaitloop:
                mov eax, esi
                xor ecx, ecx
                xor edx, edx
                monitor

                xor eax, eax
                mwait

                mov ebx, [esi]
                jz pniwaitloop
            }

            return (WAIT_OBJECT_0);
        } unguard;
    }

protected:
    int Flag;
};

#endif // _CSIGNALPNI_H