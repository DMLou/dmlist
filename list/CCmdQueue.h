/*****************************************************************************

  CCmdQueue.h

  Implements a threadshared command queue (aka message queue)

*****************************************************************************/


#ifndef _CCMDQUEUE_H
#define _CCMDQUEUE_H


#include "../C_APILib/C_ShObj.h"
#include <deque>
#include "CSignal.h"


// Command/message structure
class CCommand
{
public:
    CCommand ()
    {
        guard
        {
            return;
        } unguard;
    }

    CCommand (uint32 _Message, uint32 _Parm1, uint32 _Parm2, uint32 _Parm3, uint32 _Parm4, void *_Ptr)
    {
        guard
        {
            Message = _Message;
            Parm1 = _Parm1;
            Parm2 = _Parm2;
            Parm3 = _Parm3;
            Parm4 = _Parm4;
            Ptr = _Ptr;
            return;
        } unguard;
    }

    CCommand (const CCommand &rhs)
    {
        guard
        {
            Message = rhs.Message;
            Parm1   = rhs.Parm1;
            Parm2   = rhs.Parm2;
            Parm3   = rhs.Parm3;
            Parm4   = rhs.Parm4;
            Ptr     = rhs.Ptr;
            return;
        } unguard;
    }

    uint32  Message;
    uint32  Parm1;
    uint32  Parm2;
    uint32  Parm3;
    uint32  Parm4;
    void   *Ptr;
};


class CCmdQueue
{
public:
    CCmdQueue ()
    {
        guard
        {
            Block = 0;
            InitShObject (&QueueMutex, FALSE);
            return;
        } unguard;
    }

    ~CCmdQueue ()
    {
        guard
        {
            Signal.Unsignal ();
            DestroyShObject (&QueueMutex);
            return;
        } unguard;
    }

    void Lock (void)
    {
        guard
        {
            LockShObject (&QueueMutex);
            return;
        } unguard;
    }

    void Unlock (void)
    {
        guard
        {
            UnlockShObject (&QueueMutex);
            return;
        } unguard;
    }

    // Places a command at the end of the queue
    void SendCommandAsync (CCommand &Command)
    {
        guard
        {
            if (Block)
                return;

            Lock ();
            Queue.push_back (Command);
            Signal.Signal ();
            Unlock ();
            return;
        } unguard;
    }

    // Places a command at the front of the queue
    void SendCommandUrgentAsync (CCommand &Command)
    {
        guard
        {
            if (Block)
                return;

            Lock ();
            Queue.push_front (Command);
            Signal.Signal ();
            Unlock ();
            return;
        } unguard;
    }

    // Searches the command queue for the given message, and if found
    // replaces it with the new command
    // Otherwise simply pushes the command onto the queue
    void ReplaceCommandAsync (uint32 Message, CCommand &Command)
    {
        guard
        {
            QueueType::size_type i;
            bool Found = false;

            if (Block)
                return;

            Lock ();

            for (i = 0; i < Queue.size(); i++)
            {
                if (Queue[i].Message == Message)
                {
                    Queue[i] = Command;
                    Found = true;
                    break;
                }
            }

            if (!Found)
                SendCommandAsync (Command);

            Unlock ();
            return;
        } unguard;
    }

    // Removes all instances of a certain command
    void EraseCommandAll (uint32 Message)
    {
        guard
        {
            Lock ();

            QueueType::iterator it;

            it = Queue.begin();
            while (it != Queue.end())
            {
                if (it->Message == Message)
                {
                    Queue.erase(it);
                    it = Queue.begin();
                }
                else
                {
                    ++it;
                }
            }

            Unlock ();
            return;
        } unguard;
    }

    // Same as ReplaceCommandAsync but it also bumps the command to the front of the queue (it will be processed next)
    void ReplaceCommandUrgentAsync (uint32 Message, CCommand &Command)
    {
        guard
        {
            QueueType::size_type i;
            bool Found = false;

            if (Block)
                return;

            Lock ();

            for (i = 0; i < Queue.size(); i++)
            {
                if (Queue[i].Message == Message)
                {
                    Queue.erase (Queue.begin() + i);
                    Queue.push_front (Command);
                    Found = true;
                    break;
                }
            }

            if (!Found)
                SendCommandUrgentAsync (Command);

            Unlock ();
            return;
        } unguard;
    }

    // Same as ReplaceCommandAsync, but it also bumps the command to the end of the queue (it will be processed last)
    void BumpCommandAsync (uint32 Message, CCommand &Command)
    {
        guard
        {
            QueueType::size_type i;
            bool Found = false;

            if (Block)
                return;

            Lock ();

            for (i = 0; i < Queue.size(); i++)
            {
                if (Queue[i].Message == Message)
                {
                    Queue.erase (Queue.begin() + i);
                    Queue.push_back (Command);
                    Found = true;
                    break;
                }
            }

            if (!Found)
                SendCommandAsync (Command);

            Unlock ();
            return;
        } unguard;
    }

    // Will block until a command is available in the queue
    void ReceiveCommandBlocking (CCommand *CommandResult)
    {
        guard
        {
            bool GotIt;

            GotIt = false;

            while (!GotIt)
            {
                Lock ();
                if (Queue.empty())
                {
                    Unlock ();
                    Sleep (0);
                }
                else
                {
                    *CommandResult = *Queue.begin();
                    Queue.pop_front();
                    GotIt = true;

                    if (Queue.size() == 0)
                        Signal.Unsignal ();

                    Unlock ();
                }
            }

            return;
        } unguard;
    }

    // If a command is available, it is placed into CommandResult, popped off the queue, and the return value is true
    // Otherwise the function immediately returns false
    bool ReceiveCommandAsync (CCommand *CommandResult)
    {
        guard
        {
            Lock ();

            if (IsCommandAvailable())
            {
                *CommandResult = *Queue.begin();
                Queue.pop_front();

                if (Queue.size() == 0)
                    Signal.Unsignal ();

                UnlockShObject (&QueueMutex);
                return (true);
            }

            Unlock ();
            return (false);
        } unguard;
    }

    // Returns true or false, depending on whether a command was available in the queue.
    // Note that the result is not invariant; it is possible that the status could
    // change between the time this result is obtained and when it is used.
    bool IsCommandAvailable (void)
    {
        guard
        {
            bool ReturnVal;

            Lock ();

            if (Queue.empty())
                ReturnVal = false;
            else
                ReturnVal = true;

            Unlock ();
            return (ReturnVal);
        } unguard;
    }

    HANDLE GetSignal (void)
    {
        guard
        {
            return (Signal.GetEventObject ());
        } unguard;
    }

    void Disable (void)
    {
        guard
        {
            Block++;
            return;
        } unguard;
    }

    void Enable (void)
    {
        guard
        {
            Block = max (Block - 1, 0);
            return;
        } unguard;
    }

    bool IsEnabled (void)
    {
        guard
        {
            return (Block == 0);   
        } unguard;
    }

private:
    typedef std::deque<CCommand> QueueType;
    QueueType Queue;
    SharedObject QueueMutex;
    CSignal Signal;
    int Block; // block commands? will refuse any Replace/SendMessage
};


#endif // _CCMDQUEUE_H