/*****************************************************************************

    CJobQueue
    NOTE: This code is not used! I was writing a type of thread pool for
    doing multithreaded searching, and didn't get around to finishing it.

*****************************************************************************/

#ifndef _CJOBQUEUE_H
#define _CJOBQUEUE_H


#include "CMutex.h"
#include "CSignal.h"
#include "guard.h"
#include <queue>
#include <list>


template<class JobType>
class CJobQueue
{
public:
    typedef JobType job_type;

    CJobQueue ()
        : job_mutex (false)
    {
        guard
        {
            return;
        } unguard;
    }

    virtual ~CJobQueue ()
    {
        guard
        {
            return;
        } unguard;
    }

    void AddJob (const job_type &NewJob)
    {
        guard
        {
            job_mutex.Lock ();
            job_queue.push (NewJob);
            job_signal.Signal ();
            job_mutex.Unlock ();
            return;
        } unguard;
    }

    // Returns true if a job was successfully placed into Result
    // Returns false if no job was available to place into Result
    bool GetJob (job_type &Result)
    {
        guard
        {
            bool ret;

            job_mutex.Lock ();

            if (job_queue.empty())
                ret = false;
            else
            {
                Result = job_queue.front ();
                job_queue.pop ();

                if (job_queue.empty())
                    job_signal.Unsignal ();

                ret = true;
            }

            job_mutex.Unlock ();
            return (ret);
        } unguard;
    }

    // Waits until a job is available in the queue
    // Does not guarantee that next call to GetJob will actually return a job!
    void WaitForJob (void)
    {
        guard
        {
            job_signal.WaitUntilSignaled();
            return;
        } unguard;
    }

    //
    bool IsJobAvailable (void)
    {
        guard
        {
            bool ret;

            job_mutex.Lock ();
            ret = job_queue.empty();
            job_mutex.Unlock ();

            return (ret);
        } unguard;
    }

private:
    CSignal job_signal;
    CMutex job_mutex;
    std::queue<job_type, list<job_type> > job_queue;
};


#endif // _CJOBQUEUE_H