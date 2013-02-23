/*****************************************************************************

    CJobManager
    NOTE: This code is not used! I was writing a type of thread pool for
    doing multithreaded searching, and didn't get around to finishing it.

*****************************************************************************/

#ifndef _CJOBMANAGER_H
#define _CJOBMANAGER_H


#include "CMutex.h"
#include "CSignal.h"
#include "guard.h"
#include <queue>
#include <list>
#include "CJobQueue.h"
#include "CWorkerThread.h"


template<class JobType>
class CJobManager
{
public:
    typedef JobType job_type;
    typedef CWorkerThread<job_type> worker_type;

    CJobManager (void)
    {
        guard
        {
            return;
        } unguard;
    }

    virtual ~CJobManager
    {
        guard
        {
            SetWorkerCount (0);
            return;
        } unguard;
    }

    void AddJob (const job_type &job)
    {
        job_queue.AddJob (job);
        return;
    }

    void SetWorkerCount (int new_count)
    {
        guard
        {
            int i;
            int old_count = workers.size();

            if (new_count == old_count)
                return;

            // Removing worker threads
            if (new_count < old_count)
            {   
                for (i = new_count; i < old_count; i++)
                    delete workers[i];

                workers.resize (new_count);
            }

            // Adding worker threads
            if (new_count > old_count)
            {
                workers.resize (new_count);

                for (i = old_count; i < new_count; i++)
                    workers[i] = CreateWorker ();
            }

            return;
        } unguard;
    }

    int GetWorkerCount (void)
    {
        return (workers.size());
    }

    // Must implement this function
    virtual worker_type *CreateWorker (void) = NULL;

private:
    CJobQueue<job_type> job_queue;
    vector<WorkerType> workers;
};


#endif // _CJOBMANAGER_H