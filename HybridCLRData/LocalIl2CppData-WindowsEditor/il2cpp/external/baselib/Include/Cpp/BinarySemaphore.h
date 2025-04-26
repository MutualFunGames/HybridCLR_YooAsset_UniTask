#pragma once

#include "../C/Baselib_BinarySemaphore.h"

namespace baselib
{
    BASELIB_CPP_INTERFACE
    {
        // In computer science, a semaphore is a variable or abstract data type used to control access to a common resource by multiple processes in a concurrent
        // system such as a multitasking operating system. A semaphore is simply a variable. This variable is used to solve critical section problems and to achieve
        // process synchronization in the multi processing environment. A trivial semaphore is a plain variable that is changed (for example, incremented or
        // decremented, or toggled) depending on programmer-defined conditions.
        //
        // A useful way to think of a semaphore as used in the real-world system is as a record of how many units of a particular resource are available, coupled with
        // operations to adjust that record safely (i.e. to avoid race conditions) as units are required or become free, and, if necessary, wait until a unit of the
        // resource becomes available.
        //
        // "Semaphore (programming)", Wikipedia: The Free Encyclopedia
        // https://en.wikipedia.org/w/index.php?title=Semaphore_(programming)&oldid=872408126
        //
        // For optimal performance, baselib::BinarySemaphore should be stored at a cache aligned memory location.
        class BinarySemaphore
        {
        public:
            // non-copyable
            BinarySemaphore(const BinarySemaphore& other) = delete;
            BinarySemaphore& operator=(const BinarySemaphore& other) = delete;

            // non-movable (strictly speaking not needed but listed to signal intent)
            BinarySemaphore(BinarySemaphore&& other) = delete;
            BinarySemaphore& operator=(BinarySemaphore&& other) = delete;

            // Creates a binary semaphore (another commonly used name is AutoResetEvent) synchronization primitive.
            // Binary means the semaphore can at any given time have at most one token available for consummation.
            //
            // If there are not enough system resources to create a semaphore, process abort is triggered.
            BinarySemaphore()
            {
                Baselib_BinarySemaphore_CreateInplace(&m_SemaphoreData);
            }

            // Reclaim resources and memory held by the semaphore.
            //
            // If threads are waiting on the semaphore, destructor will trigger an assert and may cause process abort.
            ~BinarySemaphore()
            {
                Baselib_BinarySemaphore_FreeInplace(&m_SemaphoreData);
            }

            // Wait for semaphore token to become available
            //
            // This function is guaranteed to emit an acquire barrier.
            //
            // \param maxSpinCount  Max number of times to spin in user space before falling back to the kernel. The actual number
            //                      may differ depending on the underlying implementation but will never exceed the maxSpinCount
            //                      value.
            inline void Acquire(const uint32_t maxSpinCount = 0)
            {
                if (maxSpinCount && Baselib_BinarySemaphore_TrySpinAcquire(&m_SemaphoreData, maxSpinCount))
                    return;

                return Baselib_BinarySemaphore_Acquire(&m_SemaphoreData);
            }

            // Try to consume a token.
            //
            // When successful this function is guaranteed to emit an acquire barrier.
            //
            // \param maxSpinCount  Max number of times to spin in user space before falling back to the kernel. The actual number
            //                      may differ depending on the underlying implementation but will never exceed the maxSpinCount
            //                      value.
            // \returns             true if token was consumed. false if not.
            inline bool TryAcquire(const uint32_t maxSpinCount = 0)
            {
                return Baselib_BinarySemaphore_TrySpinAcquire(&m_SemaphoreData, maxSpinCount);
            }

            // Wait for semaphore token to become available
            //
            // When successful this function is guaranteed to emit an acquire barrier.
            //
            // TryAcquire with a zero timeout differs from TryAcquire() in that TryAcquire() is guaranteed to be a user space operation
            // while Acquire with a zero timeout may enter the kernel and cause a context switch.
            //
            // Timeout passed to this function may be subject to system clock resolution.
            // If the system clock has a resolution of e.g. 16ms that means this function may exit with a timeout error 16ms earlier than originally scheduled.
            //
            // \param timeout       Time to wait for token to become available.
            // \param maxSpinCount  Max number of times to spin in user space before falling back to the kernel. The actual number
            //                      may differ depending on the underlying implementation but will never exceed the maxSpinCount
            //                      value.
            // \returns             true if token was consumed. false if timeout was reached.
            inline bool TryTimedAcquire(const timeout_ms timeoutInMilliseconds, const uint32_t maxSpinCount = 0)
            {
                if (maxSpinCount && Baselib_BinarySemaphore_TrySpinAcquire(&m_SemaphoreData, maxSpinCount))
                    return true;

                return Baselib_BinarySemaphore_TryTimedAcquire(&m_SemaphoreData, timeoutInMilliseconds.count());
            }

            // Release semaphore and make it available to other threads.
            //
            // If semaphore was previously released and is currently open, calling this function result in a no-op.
            //
            // When the semaphore is released this function is guaranteed to emit a release barrier.
            //
            // \returns          true if a token was submitted, false otherwise (meaning the BinarySemaphore already has a token)
            inline bool Release()
            {
                return Baselib_BinarySemaphore_Release(&m_SemaphoreData);
            }

            // Sets the semaphore token count to zero and release all waiting threads.
            //
            // When successful this function is guaranteed to emit a release barrier.
            //
            // Return:          number of released threads.
            inline uint64_t ResetAndReleaseWaitingThreads()
            {
                return Baselib_BinarySemaphore_ResetAndReleaseWaitingThreads(&m_SemaphoreData);
            }

        private:
            Baselib_BinarySemaphore m_SemaphoreData;
        };
    }
}
