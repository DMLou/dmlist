/*****************************************************************************

    CWorkerThread

*****************************************************************************/

#ifndef _CWORKERTHREAD_H
#define _CWORKERTHREAD_H


#include "CJobQueue.h"
#include "CSignal.h"
#include "guard.h"


template<class JobType>
class CWorkerThread : public CThread
{
public:
    typedef JobType job_type;
    typedef ResultType result_type;

    CWorkerThread (CJobQueue<job_type> &JobQueue)
        : job_queue (JobQueue),
          terminate (false)
    {
        guard
        {
            return;
        } unguard;
    }

    virtual ~CWorker ()
    {
        guard
        {
            FinishUp ();
            return;
        } unguard;
    }

    void FinishUp (void)
    {
        guard
        {
            terminate = true;
            done_signal.WaitUntilSignaled ();
            return;
        } unguard;
    }

    bool ThreadFunction (void)
    {
        guard
        {
            terminate = false;
            done_signal.Unsignal ();

            while (!terminate)
            {
                bool HaveJob;
                job_type Job;

                job_queue.WaitForJob ();
                HaveJob = job_queue.GetJob (Job);

                if (HaveJob)
                    DoJob (Job);
            }

            done_signal.Signal ();
            return (false);            
        } unguard;
    }

    virtual void DoJob (job_type &job) = NULL; // You must implement this!

private:
    CJobQueue<job_type> job_queue;
    CSignal done_signal;
    volatile bool terminate;
};


#endif // _CWORKER_H