#include "il2cpp-config.h"

#if (IL2CPP_THREADS_PTHREAD || IL2CPP_THREADS_WIN32)

#include "WaitObject.h"
#include "os/Time.h"
#if IL2CPP_THREADS_WIN32
#include "os/Win32/ThreadImpl.h"
#else
#include "os/Posix/ThreadImpl.h"
#endif


// Notes:
// ************************************ Old notes here ************************************
// - Situation
//   - None of the pthread APIs are interruptible (they all explicitly forbid returning EINTR).
//   - We cannot do any non-local transfers of control from signal handlers safely (C++ exceptions
//     or longjmp). Thus we cannot use signals to inject interruptions into a thread.
//   - Very few of the system APIs we have available support timeouts (at least not on all platforms).
// - Ergo: we need to roll our own synchronization primitives based on pthread condition variables
//     (they support timeouts and have the functionality needed to model the other primitives).
// - BUT: the condition variables still involve mutexes which we cannot lock in way that allows
//     interruptions. This means that there will be time windows where threads will wait and just
//     block and not allow interruption.
//
// ************************************ Old notes above ************************************
/*
    Pthread api functions has been replaced by baselib sync primitives

    Condition variable is simulated by using a baselib capped semaphore in the thread itself and a baselib lock
    which is part of our sync primitive. If a thread waits for a condition we first take the lock and tell
    current thread to save 'us' as a wait object and then go into the simulated condition variable.
    Our condition variable first registers the thread as a waiter to be used when waking up threads.
    This list is a shared resource but we already have acquired the lock. Then we acquire the capped semaphore
    on the thread side of things that will either put us to sleep or carry on as normal if the semaphore has been signalled already.
    Once the thread wakes up, either from an APC or otherwise, it will re-acquire the lock and check for pending APCs
    and then if the condition itself (m_Count is 0) has been met.

    If an APC is queued we save the request (a callback and context) on the thread side and release/signal the semaphore to wakeup the thread. Once awake we
    call into the thread to process the APC by invoking the callback which once done will throw an exception which is then caught by WaitObject::Wait()
    and we exit the wait.

*/


namespace il2cpp
{
namespace os
{
    WaitObject::WaitObject(Type type)
        : m_Type(type)
        , m_Count(0)
        , m_WaitingThreadCount(0)
    {
    }

    WaitObject::~WaitObject()
    {
    }

    WaitStatus WaitObject::Wait(bool interruptible)
    {
        return Wait(kNoTimeout, interruptible);
    }

    WaitStatus WaitObject::Wait(uint32_t timeoutMS, bool interruptible)
    {
        // IMPORTANT: This function must be exception-safe! APCs may throw.

        ThreadImpl* currentThread = ThreadImpl::GetCurrentThread();

        // Do up-front check about pending APC except this is a zero-timeout
        // wait (i.e. a wait that is never supposed to block and thus go into
        // an interruptible state).
        if (interruptible && timeoutMS != 0)
            currentThread->CheckForUserAPCAndHandle();

        // Lock object. We release this mutex during waiting.
        ReleaseOnDestroy lock(m_Mutex);

        // See if the object is in a state where we can acquire it right away.
        if (m_Count == 0)
        {
            // No, hasn't. If we're not supposed to wait, we're done.
            if (timeoutMS == 0)
                return kWaitStatusTimeout;

            try
            {
                // We should wait. Let the world know this thread is now waiting
                // on this object.
                if (interruptible)
                    currentThread->SetWaitObject(this);

                // Check APC queue again to avoid race condition.
                if (interruptible)
                    currentThread->CheckForUserAPCAndHandle();

                // Go into wait until we either have a release or timeout or otherwise fail.
                int32_t remainingWaitTime = (int32_t)timeoutMS;
                WaitStatus waitStatus = kWaitStatusSuccess;
                while (m_Count == 0)
                {
                    if (timeoutMS == kNoTimeout)
                    {
                        // Infinite wait. Can only be interrupted by APC.
                        ++m_WaitingThreadCount; // No synchronization necessary; we hold the mutex.
                        ConditionWait(currentThread);
                        --m_WaitingThreadCount;
                    }
                    else
                    {
                        // Timed wait. Can be interrupted by APC or timeout.
                        const int64_t waitStartTime = Time::GetTicks100NanosecondsMonotonic();

                        ++m_WaitingThreadCount;
                        bool wait_timedout = ConditionTimedWait(currentThread, remainingWaitTime);
                        --m_WaitingThreadCount; ////TODO: make this atomic for when we fail to reacquire the mutex

                        if (wait_timedout == false)
                        {
                            waitStatus = kWaitStatusTimeout;
                            break;
                        }

                        // Update time we have have left to wait.
                        const int32_t waitTimeThisRound = (int32_t)(Time::GetTicks100NanosecondsMonotonic() - waitStartTime) / 10000;
                        if (waitTimeThisRound > remainingWaitTime)
                            remainingWaitTime = 0;
                        else
                            remainingWaitTime -= waitTimeThisRound;
                    }

                    // We've received a signal but it may be because of an APC and not because
                    // the semaphore got signaled. If so, handle the APC and go back to waiting.
                    if (interruptible)
                        currentThread->CheckForUserAPCAndHandle();
                }

                // We're done waiting so untie us from the current thread.
                // NOTE: A thread may have grabbed us and then got paused. If we return now and then our owner
                //       tries to delete us, we would pull the rug from under the other thread. This is prevented by
                //       having a central lock on wait object deletion which any thread trying to deal with wait
                //       objects from other threads has to acquire.
                if (interruptible)
                {
                    currentThread->SetWaitObject(NULL);

                    // Avoid race condition by checking APC queue again after unsetting wait object.
                    currentThread->CheckForUserAPCAndHandle();
                }

                // If we failed, bail out now.
                if (waitStatus != kWaitStatusSuccess)
                    return waitStatus;
            }
            catch (...)
            {
                if (interruptible)
                    currentThread->SetWaitObject(NULL);
                throw;
            }
        }

        // At this point, we should be in signaled state and have the lock on
        // the object.

        // Object has been released. Acquire it for this thread.
        IL2CPP_ASSERT(m_Count > 0);
        switch (m_Type)
        {
            case kManualResetEvent:
                // Nothing to do.
                break;

            case kMutex:
            case kAutoResetEvent:
                m_Count = 0;
                break;

            case kSemaphore:
                if (m_Count > 0) // Defensive.
                {
                    --m_Count;
                    if (m_Count > 0)
                    {
                        // There's more releases on the semaphore. Signal the next thread in line.
                        if (HaveWaitingThreads())
                            WakeupOneThread();
                    }
                }
                break;
        }

        return kWaitStatusSuccess;
    }

    // Register this thread as a waiter to be notified
    void WaitObject::PushThreadToWaitersList(WaitObject* owner, ThreadImpl* thread)
    {
        SThreadPairPosix pair(thread, owner);
        m_WaitingThreads.push_back(pair);
    }

    // Unregister this thread
    void WaitObject::PopThreadFromWaitersList(ThreadImpl* thread)
    {
        auto it = m_WaitingThreads.begin();
        while (it != m_WaitingThreads.end())
        {
            if ((*it).thread == thread)
            {
                m_WaitingThreads.erase_swap_back(it);
                break;
            }
            else
                ++it;
        }
    }

    void WaitObject::ConditionWait(ThreadImpl* thread)
    {
        PushThreadToWaitersList(this, thread);
        m_Mutex.Release();

        thread->AcquireSemaphore();

        m_Mutex.Acquire();
        PopThreadFromWaitersList(thread);
    }

    bool WaitObject::ConditionTimedWait(ThreadImpl* thread, uint32_t timeout)
    {
        PushThreadToWaitersList(this, thread);
        m_Mutex.Release();

        bool ret = thread->TryTimedAcquireSemaphore(timeout);

        m_Mutex.Acquire();
        PopThreadFromWaitersList(thread);
        return ret;
    }

    void WaitObject::WakeupThreads(bool wakeupOneThread)
    {
        // Wake up threads.
        // We do this by iterating the waiters list and check if the owner (the semaphore, event or mutex) matches 'this'
        // ie who is waiting for us specifically

        // Mutex must be locked already by caller, see EventImpl::Set(), SemaphoreImpl::Post() and EventImpl::Set()
        IL2CPP_ASSERT(m_Mutex.TryAcquire() == false);

        int threadsWaiting = (int)m_WaitingThreads.size();
        int threadsNotified = 0;

        for (int i = 0; i < threadsWaiting; i++)
        {
            SThreadPairPosix* object = &m_WaitingThreads[i];

            if (object->owner == this)
            {
                // a thread is stuck waiting for us, signal the thread semaphore
                object->thread->ReleaseSemaphore();

                // if only one wakeup is requested we exit here
                if (wakeupOneThread)
                    break;
            }
        }
    }

    void* WaitObject::GetOSHandle()
    {
        IL2CPP_ASSERT(0 && "This function is not implemented and should not be called");
        return NULL;
    }
}
}

#endif // IL2CPP_TARGET_POSIX
