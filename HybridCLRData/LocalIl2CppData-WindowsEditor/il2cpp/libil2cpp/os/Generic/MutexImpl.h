#pragma once

#if (IL2CPP_THREADS_PTHREAD || IL2CPP_THREADS_WIN32)

#include "os/ErrorCodes.h"
#include "os/WaitStatus.h"
#include "os/Generic/WaitObject.h"
#include "Baselib.h"
#include "Cpp/ReentrantLock.h"
#include "os/Thread.h"


namespace il2cpp
{
namespace os
{
    class Thread;

    class MutexImpl : public WaitObject
    {
    public:
        MutexImpl()
            : WaitObject(kMutex)
            , m_OwningThread(NULL)
            , m_RecursionCount(0)
        {
            // For a mutex, 1 means unlocked.
            m_Count = 1;
        }

        void Lock(bool interruptible)
        {
            TryLock(kNoTimeout, interruptible);
        }

        bool TryLock(uint32_t milliseconds, bool interruptible)
        {
            Thread* currentThread = Thread::GetCurrentThread();
            if (m_OwningThread == currentThread)
            {
                IL2CPP_ASSERT(m_Count == 0);
                ++m_RecursionCount;
                return true;
            }

            if (Wait(milliseconds, interruptible) == kWaitStatusSuccess)
            {
                m_OwningThread = currentThread;
                m_RecursionCount = 1;
                return true;
            }

            return false;
        }

        void Unlock()
        {
            IL2CPP_ASSERT(m_OwningThread == Thread::GetCurrentThread());

            // Undo one locking level.
            --m_RecursionCount;
            if (m_RecursionCount > 0)
            {
                // Still locked.
                return;
            }

            // Ok, we're releasing the mutex. Lock and signal. We don't absolutely
            // need the lock as we are already owning the mutex here but play it safe.
            WaitObject::ReleaseOnDestroy lock(m_Mutex);

            IL2CPP_ASSERT(m_Count == 0);
            m_Count = 1; // Unintuitive but 1 means unlocked.
            m_OwningThread = NULL;

            // Signal condition so that either a thread that's already waiting or a thread that
            // comes around later and waits can claim the mutex.
            if (HaveWaitingThreads())
                WakeupOneThread();
        }

    private:
        /// Thread that currently owns the object. Used for recursion checks.
        Thread* m_OwningThread;

        /// Number of recursive locks on the owning thread.
        uint32_t m_RecursionCount;
    };
}
}

#endif
