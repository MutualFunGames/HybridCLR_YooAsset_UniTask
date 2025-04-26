#pragma once

#include "Time.h"
#include "Lock.h"
#include <cstdint>

#if PLATFORM_HAS_NATIVE_FUTEX
#include "Internal/ConditionVariableData_FutexBased.inl.h"
#else
#include "Internal/ConditionVariableData_SemaphoreBased.inl.h"
#endif

namespace baselib
{
    BASELIB_CPP_INTERFACE
    {
        // Conceptually a condition variable is a queue of threads, associated with a monitor, on which a thread may wait for some condition to become true.
        //
        // Thus each condition variable c is associated with an assertion Pc. While a thread is waiting on a condition variable, that thread is not considered
        // to occupy the monitor, and so other threads may enter the monitor to change the monitor's state. In most types of monitors, these other threads may
        // signal the condition variable c to indicate that assertion Pc is true in the current state.
        //
        // "Monitor (synchronization)", Wikipedia: The Free Encyclopedia
        // https://en.wikipedia.org/w/index.php?title=Monitor_(synchronization)&oldid=914426020#Condition_variables_2
        //
        // For optimal performance, baselib::ConditionVariable should be stored at a cache aligned memory location.
        class ConditionVariable
        {
        public:
            // non-copyable
            ConditionVariable(const ConditionVariable& other) = delete;
            ConditionVariable& operator=(const ConditionVariable& other) = delete;

            // non-movable (strictly speaking not needed but listed to signal intent)
            ConditionVariable(ConditionVariable&& other) = delete;
            ConditionVariable& operator=(ConditionVariable&& other) = delete;

            // Creates a condition variable synchronization primitive.
            ConditionVariable(Lock& lock) : m_Lock(lock)
            {}

            // Reclaim resources and memory held by the condition variable.
            //
            // If threads are waiting on the condition variable, destructor will trigger an assert and may cause process abort.
            ~ConditionVariable()
            {
                BaselibAssert(!m_Data.HasWaiters(), "Destruction is not allowed when there are still threads waiting on the condition variable.");
                NotifyAll();
            }

            // Wait for the condition variable to become available.
            //
            // The lock must have been previously acquired.
            // For the duration of the wait the lock is released and then re-acquired upon exit.
            // This function is guaranteed to emit an acquire barrier.
            inline void Wait();

            // Wait for the condition variable to become available.
            //
            // The lock must have been previously acquired.
            // For the duration of the wait the lock is released and then re-acquired upon exit.
            // This function is guaranteed to emit an acquire barrier.
            //
            // TimedWait with a zero timeout is guaranteed to be a user space operation.
            //
            // \param timeoutInMilliseconds   Time to wait for condition variable to become available.
            // \returns true if the condition variable is available, false if timeout was reached.
            inline bool TimedWait(const timeout_ms timeoutInMilliseconds);

            // Post `count` number of tokens and wake up thread(s) waiting for the condition variable to become available.
            //
            // This function does *not* guarantee fairness so it is possible not all threads waiting are woken up if multiple tokens are consumed by one thread.
            // This function is guaranteed to emit a release barrier.
            //
            // \param count At most, `count` waiting threads will be notified, but never more than there are currently waiting.
            inline void Notify(uint16_t count);

            // Post maximum number of tokens and wake up thread(s) waiting for the condition variable to become available.
            //
            // This function does *not* guarantee fairness so it is possible not all threads waiting are woken up if multiple tokens are consumed by one thread.
            // This function is guaranteed to emit a release barrier.
            inline void NotifyAll()
            {
                Notify(std::numeric_limits<uint16_t>::max());
            }

        private:
            Lock& m_Lock;
            detail::ConditionVariableData m_Data;
        };
    }
}

#if PLATFORM_HAS_NATIVE_FUTEX
#include "Internal/ConditionVariable_FutexBased.inl.h"
#else
#include "Internal/ConditionVariable_SemaphoreBased.inl.h"
#endif
